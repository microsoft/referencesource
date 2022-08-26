//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: NLregC.cpp
// @Owner: petergv, nantu
// @test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: Registry Manipulation Routines (Client Side)
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#include "NLregC.hpp"
#include <strsafe.h>
#include <intsafe.h>
#include <assert.h>

#include "IO.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof (a) / sizeof ((a) [0]))
#endif

#ifndef CHKL_GOTO
#define CHKL_GOTO(l, label)		{if (ERROR_SUCCESS!=l) {goto label;}}
#endif

#ifndef CHKL_RETURN
#define CHKL_RETURN(l)			{if (ERROR_SUCCESS!=l) {return l;}}
#endif

#ifndef Assert
#define Assert(b) assert(b)
#endif

#define MAX_LEN_CACHEVALUENAME 16383 // this value is max available length of the reg value name define by windows. 
#ifndef ULONG_MAX
#define ULONG_MAX 0xffffffff
#endif

const BOOL CS_CREATE	= TRUE;
const BOOL CS_OPEN 		= FALSE;

// Maximum number of bytes per character supported by the ASCII version
// of NLregC.  Set to 2 to account for double-byte character sets (DBCS).  
//
#define CS_ASCII_MAX_BYTES_PER_CHAR 2

NLregC * g_pRegInfo;

typedef ULONG (FAR WINAPI *CONNECTIONVER_FN)();

INT LibMain (HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
    switch (ul_reason_being_called) 
	{
	  case DLL_PROCESS_ATTACH:
	  	
		g_pRegInfo = new NLregC(VER_SQL_MAJOR);
	
		break;

	  case DLL_PROCESS_DETACH:
		delete g_pRegInfo;
		break;
    }

    return TRUE;                                                                
                                                                                
    
} /* LibMain */                                                                 

///////////////////////////////////////////////////
//
// Implementation of NLregC public APIs defined in NLregC.h
//
//
//======== Version-Dependent NLregC APIs =======
//
LONG __stdcall CSsetDefaults( BOOL fOverWriteAll )
{
	NLregC nlrc(VER_SQL_MAJOR);
	
	return nlrc.CSsetDefaults( fOverWriteAll );
}


LONG __stdcall CSgetNumberOfGeneralFlags( DWORD * pdwNumberOfFlags )
{
	NLregC nlrc(VER_SQL_MAJOR);	

	return nlrc.CSgetNumberOfGeneralFlags( pdwNumberOfFlags );
}


LONG __stdcall CSgetGeneralFlagProperty( __in DWORD   dwFlagIndex,
										 __out_bcount( CS_MAX * sizeof(TCHAR) ) __nullterminated TCHAR   szLabel[CS_MAX],
										 __out DWORD * pdwFlagState )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetGeneralFlagProperty( dwFlagIndex,
									szLabel,
									CS_MAX * sizeof( TCHAR ),
									pdwFlagState );
}


LONG __stdcall CSgetGeneralFlagPropertyEx( __in DWORD   dwFlagIndex,
										 __out_bcount(dwcbLabel) __nullterminated TCHAR   szLabel[CS_MAX],
										 __in DWORD dwcbLabel,
										 __out DWORD * pdwFlagState )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetGeneralFlagProperty( dwFlagIndex,
									szLabel,
									dwcbLabel,
									pdwFlagState );
}


LONG __stdcall CSsetGeneralFlagProperty( DWORD dwFlagIndex,
									     DWORD dwFlagState )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSsetGeneralFlagProperty( dwFlagIndex,
									dwFlagState );

}


LONG __stdcall CSgetProtocolsSupported( __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szProtocolsSupported,
									    __inout DWORD * pdwBufferSize )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetProtocolsSupported( szProtocolsSupported,
								       pdwBufferSize );
}


LONG __stdcall CSgetProtocolOrder(  __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szProtocolOrder,
								   __inout DWORD * pdwBufferSize )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetProtocolOrder( szProtocolOrder,
							      pdwBufferSize );
}


LONG __stdcall CSsetProtocolOrder(__in TCHAR * szProtocolOrder )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSsetProtocolOrder( szProtocolOrder );
}


LONG __stdcall CSgetNumberOfProtocolFlags( TCHAR * szProtocol,
										   DWORD * pdwNumberOfFlags )
{
	NLregC nlrc(VER_SQL_MAJOR);
	
	return nlrc.CSgetNumberOfProtocolFlags( szProtocol,
							 		      pdwNumberOfFlags );
}


LONG __stdcall CSgetNumberOfProtocolProperties( TCHAR * szProtocol,
											    DWORD * pdwNumberOfProperties )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetNumberOfProtocolProperties( szProtocol,
								 		       pdwNumberOfProperties );
}


LONG __stdcall CSgetProtocolInfo( TCHAR            * szProtocol,
								  CS_PROTOCOL_INFO * pProtocolInfo )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetProtocolInfo( szProtocol, pProtocolInfo );
}


LONG __stdcall CSgetProtocolFlag( __in __nullterminated TCHAR * szProtocol,
								  __in DWORD   dwPropertyIndex,
								  __out_bcount( CS_MAX * sizeof( TCHAR ) ) __nullterminated TCHAR * szFlagLabel,
								  __out DWORD * dwFlagValue )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetProtocolFlag( szProtocol,
							 dwPropertyIndex,
							 szFlagLabel,
							 CS_MAX * sizeof( TCHAR ),
							 dwFlagValue );
}

LONG __stdcall CSgetProtocolFlagEx( __in __nullterminated TCHAR * szProtocol,
								  __in DWORD   dwPropertyIndex,
								  __out_bcount_opt(dwcbFlagLabel) __nullterminated TCHAR * szFlagLabel,
								  __in_range(0,CS_MAX * sizeof (TCHAR)) DWORD dwcbFlagLabel,
								  __out DWORD * dwFlagValue )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetProtocolFlag( szProtocol,
							 dwPropertyIndex,
							 szFlagLabel,
							 dwcbFlagLabel,
							 dwFlagValue );
}

LONG __stdcall CSsetProtocolFlag( TCHAR * szProtocol,
								  DWORD   dwFlagIndex,
								  DWORD   dwFlagValue )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSsetProtocolFlag( szProtocol, dwFlagIndex, dwFlagValue );
}


LONG __stdcall CSgetProtocolProperty( __in __nullterminated TCHAR			       * szProtocol,
									  __in DWORD			         dwPropertyIndex,
									  __out CS_PROTOCOL_PROPERTY * pProtocolProperty )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetProtocolProperty( szProtocol,
								 dwPropertyIndex,
								 pProtocolProperty );
}


LONG __stdcall CSsetProtocolProperty( __in __nullterminated TCHAR				   * szProtocol,
						 			  __in DWORD			         dwPropertyIndex,
									  __in CS_PROTOCOL_PROPERTY * pProtocolProperty )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSsetProtocolProperty( szProtocol,
								 dwPropertyIndex,
								 pProtocolProperty );
}


LONG __stdcall CSgetNETLIBinfo( __in __nullterminated TCHAR		  * szProtocol,
							    __out CS_NETLIBINFO * infoNETLIB )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetNETLIBinfo( szProtocol,   infoNETLIB );
}


LONG __stdcall CSdeleteLastConnectionCache(void)
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSdeleteLastConnectionCache();
}


LONG __stdcall CScreateLastConnectionCache( BOOL fOverwrite )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CScreateLastConnectionCache(fOverwrite);
}


LONG __stdcall CSdeleteAllCachedValues( void )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSdeleteAllCachedValues();
}


LONG __stdcall CSgetCachedValueList( __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szCacheNameList,
									 __inout DWORD * pdwBufferSize, 
									 __out_opt DWORD * pdwMaxValueLen )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetCachedValueList( szCacheNameList,
									pdwBufferSize,
									pdwMaxValueLen );
}


LONG __stdcall CSgetCachedValue( TCHAR * szCacheName,
								 __out_bcount(dwValueBufferSize) TCHAR * szCacheValue, 
								 DWORD	 dwValueBufferSize )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSgetCachedValue(szCacheName,
								szCacheValue, 
								dwValueBufferSize );
}


LONG __stdcall CSsetCachedValue( TCHAR * szCacheName,
							     TCHAR * szCacheValue )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSsetCachedValue( szCacheName, szCacheValue );
}


LONG __stdcall CSdeleteCachedValue( TCHAR * szCachedName )
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSdeleteCachedValue( szCachedName );
}


LONG __stdcall CSdeleteHive()
{
	NLregC nlrc(VER_SQL_MAJOR);	
	
	return nlrc.CSdeleteHive();
}

//
// ========== Non-Version-Dependent NLregC APIs ======
//

LONG __stdcall CSgetDBLIBinfo( __out CS_DBLIBINFO * infoDBLIB )
{
	return  g_pRegInfo->CSgetDBLIBinfo( infoDBLIB );
}


LONG __stdcall CSsetDBLIBinfo( __in CS_DBLIBINFO * infoDBLIB )
{
	return  g_pRegInfo->CSsetDBLIBinfo( infoDBLIB );
}


LONG __stdcall CSgetAliases
( 
	__out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szAliasesPresent,
	__inout						DWORD * pdwBufferSize 
)
{
	return g_pRegInfo->CSgetAliases( szAliasesPresent, pdwBufferSize );
}


LONG __stdcall CSgetAlias( TCHAR    * szAliasName, CS_ALIAS * pCSalias )
{

	return g_pRegInfo->CSgetAlias( szAliasName,  pCSalias );
}


LONG __stdcall CSaddAlias
( 
	TCHAR    * szAliasName,
	CS_ALIAS * pCSalias,
	BOOL       fOverWrite 
)
{
	return g_pRegInfo->CSaddAlias( szAliasName,  pCSalias,  fOverWrite );
}


LONG __stdcall CSdeleteAlias( TCHAR * szAliasName )
{
	return g_pRegInfo->CSdeleteAlias( szAliasName );
}

LONG _stdcall CSgetUserInstanceDllPath(__out_bcount(cchDllPathSize) LPSTR szDllPath, 
											__in DWORD cchDllPathSize,
											__out LocalDBErrorState* pErrorState)
{
	return g_pRegInfo->CSgetUserInstanceDllPath(szDllPath, cchDllPathSize, pErrorState);
}

//
// ============ End of NLregC public APIs ========
////////////////////////////////////////////////////


NLregC::NLregC(USHORT usMajor)
{
	m_curVer = usMajor;
}

NLregC::~NLregC()
{
}

/////////////////////////////////////////////////////
// Initialize CS_FPV based on a CS_FPV_DEFAULT. Deep copy if
// needed.
//
// inputs:
//		csfpv[in]: a CS_FPV_DEFAULT.
//
// returns:
//		ERROR_SUCCESS if no error.
//		Otherwise, a win error code.
//
// Note:
//		
//
LONG CS_FPV::Copy(const CS_FPV_DEFAULT* csfpv)
{
	Assert(csfpv && csfpv->pszRelativePath);
	
	type =  csfpv->type;
	pszRelativePath = csfpv->pszRelativePath;
	dwFPIndex = csfpv->dwFPIndex;
		
	if(csfpv->dwcbName > sizeof(szName))
		return ERROR_BUFFER_OVERFLOW;

	memcpy( szName, csfpv->pszName, csfpv->dwcbName);

	//Initialized value based on its type
	dwValueType = csfpv->dwValueType;
	switch(dwValueType)
	{
		case REG_DWORD:
			dwValue = csfpv->dwValue;			
			break;
		case REG_SZ:
		case REG_MULTI_SZ:	
			Assert(csfpv->pszValue && csfpv->dwcbValue>0);

			if(csfpv->dwcbValue > sizeof(bValue))
				return ERROR_BUFFER_OVERFLOW;
			
			//Initialized DllName based on current version.
			if( !_tcscmp(szName, szDllName))
			{
				if( FAILED(StringCchPrintf(szValue, sizeof(bValue)/sizeof(TCHAR),
						SNACDLLNAME TEXT("%d"), VER_SQL_MAJOR)))
					return ERROR_INVALID_PARAMETER;
			}
			else
			{
				memcpy( szValue, csfpv->pszValue, csfpv->dwcbValue);
			}

			break;
		default:
			return ERROR_INVALID_PARAMETER;
			
	}
	
	return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////
// Init CS_VALUE as a DWORD
//
// inputs:
//		pszn[in]: the name of the registry entry.
//		pdwv[in]: the dword value.
// return: void
//
void CS_VALUE::InitDw(const TCHAR* pszn, const DWORD* pdwv)
{
	Assert(pszn && pdwv);
	
	m_pszName= pszn;
	m_dwValueType = REG_DWORD;
	m_pdwValue = pdwv;
	m_dwcbValue = sizeof(DWORD);
}

//////////////////////////////////////////////////////////
// Init CS_VALUE
// inputs:
//		pszn[in]: the name of the regitry entry.
//		dwt[in]: the type of the value
//		pbv[in]: the point to the buffer that stores the regitry value.
//		dwcbv[in]: the length of the buffer that stores the registry value.
//
// return: void
//
// Note: pbv can be NULL when ask for the size required.
//
void CS_VALUE::InitBytes
(
	const TCHAR* pszn, 
	DWORD dwt, 
	const BYTE* pbv, 
	DWORD dwcbv
)
{
	Assert( pszn );
	m_pszName = pszn;
	m_dwValueType= dwt;
	m_pbValue= pbv;
	m_dwcbValue = dwcbv ;			
}

///////////////////////////////////////////////////////////
// Ctor based on CS_FPV.
// Inputs: 
//		pcsFPV: a CS_FPV that store the value.
// returns:
//		NONE.
//
CS_VALUE::CS_VALUE
(
	const CS_FPV* pcsFPV
)
{
	Assert(pcsFPV);
	
	switch(pcsFPV->dwValueType)
	{
		case REG_DWORD: 
			InitDw(pcsFPV->szName, &pcsFPV->dwValue);
			break;
		case REG_SZ:
		case REG_MULTI_SZ: 
			InitBytes(pcsFPV->szName, pcsFPV->dwValueType, &pcsFPV->bValue[0], sizeof(pcsFPV->bValue));
			break;
		default:
			InitBytes(pcsFPV->szName, pcsFPV->dwValueType, &pcsFPV->bValue[0], sizeof(pcsFPV->bValue));
	}
}

////////////////////////////////////////////////////////////
// Set the value into the registry
// Inputs:
//		hKey [in]: the parent registry key.
// returns:
//		ERROR_SUCCESS if no error.
//		Otherwise, a win error code.
// Note: The m_dwcbValue is the bound of the buffer size. 
// This function figure the size of the buffer by it self depends on the 
// m_dwValueType. 
//
LONG CS_VALUE::SetValue(HKEY hKey)
{
	LONG  lResult = ERROR_SUCCESS;
	DWORD dwcchBufferSize = 0, dwcbBufferSize = 0 ;
	const TCHAR *pszn = m_pszName;
	const BYTE *pValue = m_pbValue;
	DWORD dwcchValue = m_dwcbValue/sizeof(TCHAR);
	DWORD dwType = m_dwValueType;

	if( NULL == hKey || NULL == pszn || NULL == pValue)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}
	
	switch( dwType )
	{
		case REG_DWORD:
			
			dwcbBufferSize = sizeof( DWORD );
			break;

		case REG_SZ:
			{
			const TCHAR* psz = m_pszValue;
			while(( dwcchValue > dwcchBufferSize )
				&& ( psz[dwcchBufferSize] != 0x00 ) )
			{
				dwcchBufferSize++;
			}

			if( dwcchValue == dwcchBufferSize )
			{
				lResult = ERROR_INVALID_PARAMETER;
				goto Exit;				
			}
			
			dwcchBufferSize++; // include NULL terminator			
			dwcbBufferSize = dwcchBufferSize*sizeof( TCHAR );
			}
			break;

		case REG_MULTI_SZ:
			{
			const TCHAR* psz = m_pszValue;
			while( TRUE )
			{
				if( dwcchValue <=	dwcchBufferSize )
				{
					lResult = ERROR_INVALID_PARAMETER;
					goto Exit;				
				}

				if( psz[dwcchBufferSize] == 0x00 )
				{
					dwcchBufferSize++;

					if( dwcchValue <= dwcchBufferSize )
					{
						lResult = ERROR_INVALID_PARAMETER;
						goto Exit;				
					}

					if( psz[dwcchBufferSize] == 0x00 )
						break;
				}

				dwcchBufferSize++;
			}

			dwcchBufferSize++;	// include NULL terminator
			dwcbBufferSize = dwcchBufferSize*sizeof( TCHAR );
			}
			break;

		default:
			lResult = ERROR_INVALID_PARAMETER;
			goto Exit;
	}

	lResult = RegSetValueEx( hKey,
						 pszn,
						 NULL,
						 dwType,
						 pValue,
						 dwcbBufferSize );

Exit:

	return lResult;
}


////////////////////////////////////////////////////////////
// Retrieve the value from registry.
// inputs:
//		hKey [in]: the parent registry key.
// returns:
//		ERROR_SUCCESS if no error.
//		Otherwise, a win error code.
// Note:
//	1.	m_dwcbValue is initialized to the size of the buffer in bytes.
//		if pValue is NULL, this function retrieves the required 
// 		length for storing the registry value, in m_dwcbValue, in bytes.
//
//	2.	if m_dwValueType is REG_NONE, this function retrieves
//		the type of the value, in m_dwValueType.
//	
//	3.	if m_dwValueType is not REG_NONE, if retrieved value is not
//		of the specified type, return ERROR_INVALID_PARAMETER.		
//
//	4.	if returning ERROR_SUCCESS and m_pbValue is non-null,
//		The buffer is null-terminated if the m_dwType is REG_SZ
//		or REG_MULTI_SZ.
//
LONG CS_VALUE::GetValue(HKEY hKey)
{
	LONG  lResult = ERROR_SUCCESS;
	BYTE* pValue = (BYTE*)m_pbValue;
	DWORD dwcbBufferSize = m_dwcbValue;
	DWORD dwType = m_dwValueType;
	
	if( NULL == hKey || NULL == m_pszName	||
		(REG_DWORD == m_dwValueType 
		&& sizeof(DWORD) != dwcbBufferSize))
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	if(pValue && 0 < dwcbBufferSize)
	{
		pValue[0] = 0x00;
	}

	//if pValue is zero, RegQueryValueEx returns the buffer size required.
	//

	lResult = RegQueryValueEx( hKey,
							m_pszName,
							NULL,
							&dwType,
							pValue,
							&dwcbBufferSize );
	CHKL_GOTO(lResult, Exit);

	if(REG_NONE != m_dwValueType && dwType != m_dwValueType )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}
	else if(REG_NONE == m_dwValueType)
	{
		m_dwValueType = dwType;
	}		

	if( pValue )
	{	
		if( REG_SZ == dwType )
		{
			lResult = NLregC::CSstringNullTerminate(lResult, 
									 (TCHAR*)pValue, 
									 m_dwcbValue, 
									 dwcbBufferSize ); 

			CHKL_GOTO(lResult, Exit);
		}
		else if( REG_MULTI_SZ == dwType )
		{
			lResult = NLregC::CSmultiStringNullTerminate( lResult, 
										  (TCHAR*)pValue, 
										  m_dwcbValue, 
										  dwcbBufferSize ); 
			CHKL_GOTO(lResult, Exit); 
		}			
	}

	m_dwcbValue = dwcbBufferSize;

Exit:

	return lResult;

}


///////////////////////////////////////////////////////////
// Given the relative path to hRootkey, return the subkey.
//
// inputs:
//		hRootKey [in]: the parent registry key
//		pszPath [in]: the relative path to the parent registry key.
//		phSubKey [out]: the key to open.
//		samDesired [in]: option bitmask for reg functions.
//		fCreate [in]: to use RegCreateKeyEx or to use RegOpenKeyEx.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
LONG NLregC::CSgetRegSubKey
(
	const HKEY hRootKey, 
	const TCHAR* pszPath,
	HKEY* phSubKey, 
	REGSAM samDesired, 
	BOOL fCreate
)
{
	DWORD dwDisposition;
	HKEY  hPK = hRootKey?hRootKey:HKEY_LOCAL_MACHINE;
		
	Assert(pszPath&&phSubKey);
	
	if(fCreate)
		return  RegCreateKeyEx( hPK,
							   pszPath,
							   0,
							   NULL,
							   REG_OPTION_NON_VOLATILE,
							   samDesired, //KEY_WRITE | KEY_QUERY_VALUE,
							   NULL,
							   phSubKey,
							   &dwDisposition );

	else
		return RegOpenKeyEx( hPK,
							pszPath, 
							0,
							samDesired,
							phSubKey );

}


///////////////////////////////////////////////////////////
// Create or open the SNI root key.
//
// inputs:
//		phKey[out]: key to open
//		samDesired[in]: bitmask for reg functions.
//		fCreate: whether to create the key if it doesn't exit.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetSNIRootRegKey
(
	HKEY* phKey, 
	REGSAM samDesired, 
	BOOL fCreate
)
{
	TCHAR   szRegLocation[CS_MAX];
	LONG lResult = ERROR_SUCCESS;

	Assert(phKey);

	lResult = CSgetSNIClientRootPath(szRegLocation, sizeof(szRegLocation), m_curVer);
	CHKL_RETURN(lResult);

	return CSgetRegSubKey(NULL,szRegLocation, phKey, samDesired, fCreate);
}

///////////////////////////////////////////////////////////
// Create or open SNI sub key based on relative path pszPath.
//
// inputs:
//		pszPath[in]: relative path to SNI root key.
//		phSubKey[out]: key to open
//		samDesired[in]: bidmask for reg functions.
//		fCreate: whether to create the key if it doesn't exit.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetSNISubRegKey
(
	const TCHAR* pszPath,
	HKEY* phSubKey, 
	REGSAM samDesired, 
	BOOL fCreate
)
{	
	TCHAR   szRegLocation[CS_MAX];
	LONG lResult = ERROR_SUCCESS;

	Assert( pszPath && phSubKey);

	lResult = CSgetSNIClientRootPath(szRegLocation, sizeof(szRegLocation), m_curVer);
	CHKL_RETURN(lResult);
	
	if( FAILED(StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), TEXT("\\"))))
	{
		return ERROR_INVALID_PARAMETER;
	}

	if( FAILED(StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), pszPath)))
	{
		return ERROR_INVALID_PARAMETER;
	}

	return CSgetRegSubKey(NULL,szRegLocation, phSubKey, samDesired, fCreate);
	
}

///////////////////////////////////////////////////////////
// Close a reg key.
//
// inputs:
//		hKey[in]: key to close. If it is 0, this is nop.
//
// returns: void
//
// notes:
//
void NLregC::CScloseRegKey(HKEY hKey)
{
	if( hKey )
		RegCloseKey( hKey );
}

///////////////////////////////////////////////////////////
// Retrieve a SNI flag or property.
//
// inputs:
//		hRootKey[in]: The root key of the flag of property.
//		eType[in]: either eFlag of eProperty.
//		dwIndex[in]: index of the flag or property.
//		pszLabel[out]: the Label of flag or Name of property.
//		dwcblabel[in]: the buffer size of pszLabel.
//		pdwTyep[inout]: the type of the pbValue in the registry. If it is
//		not REG_NONE, use typed query. Otherwise, non-typed query.
//		pbValue[out]: the point the to buffer that hold the value.
//		pdwcbValue[inout]: It is initialized to the size of the buffer. 
//		On return, it is the size of buffer filled or required.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetFlagOrProperty
(
	__in const HKEY hRootKey, 
	__in const CSFPVTYPE eType, 
	__in const DWORD dwIndex,
	__out_bcount_opt(dwcbLabel) __nullterminated TCHAR * pszLabel,
	__in DWORD dwcbLabel,
	__in DWORD* pdwType,
	__out_bcount(*pdwcbValue) BYTE * pbValue,
	__inout DWORD* pdwcbValue
)
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;
	TCHAR szRegLocation[CS_MAX];

	if( NULL == hRootKey || NULL == pdwcbValue)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	if( pszLabel != NULL && ( dwcbLabel > CS_MAX  * sizeof (TCHAR) || dwcbLabel == 0 ) )
	{
		lResult = ERROR_INVALID_PARAMETER ; 
		goto Exit; 
	}

	if( pbValue != NULL && ( *pdwcbValue > CS_MAX  * sizeof (TCHAR) || pdwcbValue == 0 ) )
	{
		lResult = ERROR_INVALID_PARAMETER ; 
		goto Exit; 
	}

	if( eType != eFlag && eType != eProperty)
	{
		lResult = ERROR_INVALID_PARAMETER ; 
		goto Exit; 
	}

	if( REG_DWORD != *pdwType && REG_SZ != *pdwType && REG_NONE !=*pdwType)
	{
		lResult = ERROR_INVALID_PARAMETER ; 
		goto Exit; 
	}

	if( FAILED (StringCchPrintf(szRegLocation, 
							ARRAYSIZE(szRegLocation),				
							eType==eFlag?TEXT("Flag%d"):TEXT("Property%d"),
							dwIndex) ))
		goto Exit;

	lResult = CSgetRegSubKey(hRootKey, szRegLocation, &hRegKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult,Exit);

	//query the Name/Label
	//
	if( pszLabel && pszLabel[0] == 0 )
	{
		lResult = CS_VALUE::GetString(hRegKey, 
						eType==eFlag?TEXT("Label"):TEXT("Name"), 
						REG_SZ, 
						pszLabel, &dwcbLabel);
		
		CHKL_GOTO(lResult,Exit);
	}
	
	//query the Value
	//
	switch(*pdwType)
	{
		case REG_DWORD:
			lResult = CS_VALUE::GetDw(hRegKey, TEXT("Value"), (DWORD*)pbValue);
			break;
		case REG_SZ:
			lResult = CS_VALUE::GetString(hRegKey,TEXT("Value"), *pdwType, (TCHAR*)pbValue, pdwcbValue);			
			break;
			
		default:	//uknown type.
			lResult = CS_VALUE::GetBytes(hRegKey,TEXT("Value"), pdwType, (TCHAR*)pbValue, pdwcbValue);			
			break;
	}	
	
Exit:

	CScloseRegKey(hRegKey);

	return lResult;
}

///////////////////////////////////////////////////////////
// Set a flag or property in the registry.
//
// inputs:
//		hRootKey[in]: the root key of the flag or property.
//		eType[in]: either eFlag or eProperty.
//		dwIndex[in]: the index of flag of property.
//		pszLabel[in]; the point to the null-term string that hold the 
//			label of flag or name of property. It can be null. 
//		dwType[in]: type the the pbValue, i.e. REG_xx.
//		pbValue[in]: point the the buffer that hold the value.
//		dwcbValue[in]: the buffer length in bytes.
//		fCreate[in]: whether to create if it doesn't exit.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetFlagOrProperty
(
	const HKEY hRootKey, 
	const CSFPVTYPE eType, 
	const DWORD dwIndex,
	const TCHAR * pszLabel,
	const DWORD dwType,
	const BYTE * pbValue,
	const DWORD dwcbValue,
	const BOOL fCreate
)
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;
	TCHAR szRegLocation[CS_MAX];

	if( NULL == hRootKey || NULL == pbValue )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	if( eType != eFlag && eType != eProperty)
	{
		lResult = ERROR_INVALID_PARAMETER ; 
		goto Exit; 
	}

	if( REG_DWORD != dwType && REG_SZ != dwType && REG_NONE !=dwType)
	{
		lResult = ERROR_INVALID_PARAMETER ; 
		goto Exit; 
	}

	if( FAILED (StringCchPrintf(szRegLocation, 
							ARRAYSIZE(szRegLocation),				
							eType==eFlag?TEXT("Flag%d"):TEXT("Property%d"),
							dwIndex) ))
		goto Exit;

	lResult = CSgetRegSubKey(hRootKey, szRegLocation, &hRegKey,  KEY_WRITE, fCreate);
	CHKL_GOTO(lResult,Exit);

	// Set the Name
	//
	if( pszLabel )
	{
		if(CS_MAX <= _tcslen(pszLabel))
		{
			lResult = ERROR_INVALID_PARAMETER ; 
			goto Exit; 
		}
	
		lResult = CS_VALUE::SetString(hRegKey,
					eType==eFlag?TEXT("Label"):TEXT("Name"), 
					REG_SZ, 
					pszLabel, (DWORD) (_tcslen(pszLabel)+1)*sizeof(TCHAR));
		CHKL_GOTO(lResult,Exit);
	}

	// Set the Value of the Property
	switch(dwType)
	{
		case REG_DWORD:
				lResult = CS_VALUE::SetDw(hRegKey,TEXT("Value"), *(DWORD*)pbValue);
				break;
		case REG_SZ:
				lResult = CS_VALUE::SetString(hRegKey, TEXT("Value"), dwType, (TCHAR*)pbValue, dwcbValue);
				break;
		default:
			lResult = ERROR_INVALID_PARAMETER; 
			break;
	}	
	
Exit:

	CScloseRegKey(hRegKey);

	return lResult;

}

///////////////////////////////////////////////////////////
// Retrive SNI flag, property or value based on a CS_FPV.
//
// inputs:
//		hRootRegKey[in]: the root key of SNI
//		pcsfpv[inout]: the CS_FPV.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetFPV(HKEY hRootRegKey, CS_FPV* pcsfpv)
{
	HKEY hRegKey = NULL;
	LONG lResult =  ERROR_SUCCESS;
	DWORD dwcbValue = 0;
	if( NULL == hRootRegKey ||NULL == pcsfpv ||NULL == pcsfpv->pszRelativePath)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetRegSubKey(hRootRegKey, 
						pcsfpv->pszRelativePath,
						&hRegKey, 
						KEY_QUERY_VALUE, CS_OPEN);

	CHKL_GOTO(lResult, Exit);
	
	switch( pcsfpv->type )
	{
		case eFlag:			
			
				dwcbValue = sizeof(DWORD);
				lResult = CSgetFlagOrProperty(hRegKey, eFlag, pcsfpv->dwFPIndex,
									pcsfpv->szName, sizeof(pcsfpv->szName),
									&pcsfpv->dwValueType,
									pcsfpv->bValue, &dwcbValue);

			break;
		case eProperty:

				dwcbValue = sizeof(pcsfpv->bValue);
				lResult =  CSgetFlagOrProperty(hRegKey, eProperty, pcsfpv->dwFPIndex,
									pcsfpv->szName, sizeof(pcsfpv->szName),
									&pcsfpv->dwValueType,
									pcsfpv->bValue, &dwcbValue);
				
			break;
		case eValue:
			
				lResult = CS_VALUE::GetValueWithFPV(hRegKey, pcsfpv);
				
			break;
		default:	
			
				lResult = ERROR_INVALID_PARAMETER; 
			
			break;
	}

Exit:

	CScloseRegKey(hRegKey);

	return lResult;

}


///////////////////////////////////////////////////////////
// Set flag, property or value based on CS_FPV.
//
// inputs:
//		hRootRegKey[in]: the root key of SNI.
//		pcsfpv[in]: the CS_FPV that hold needed info.
//		fCreate[in]: whether to create key needed if the key doesn't exit.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetFPV(HKEY hRootRegKey, const CS_FPV* pcsfpv, BOOL fCreate)
{
	HKEY hRegKey = NULL;
	LONG lResult =  ERROR_SUCCESS;

	if( NULL == hRootRegKey ||NULL == pcsfpv ||NULL == pcsfpv->pszRelativePath)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult =  CSgetRegSubKey( hRootRegKey,
						pcsfpv->pszRelativePath,						
						&hRegKey,
						KEY_WRITE,
						fCreate);

	CHKL_GOTO(lResult, Exit);
	
	switch( pcsfpv->type )
	{
		case eFlag:			
				lResult = CSsetFlagOrProperty(hRegKey, eFlag,pcsfpv->dwFPIndex,
									pcsfpv->szName, REG_DWORD,
									&pcsfpv->bValue[0], sizeof(DWORD), fCreate);
			break;
		case eProperty:

				lResult = CSsetFlagOrProperty(hRegKey, eProperty,pcsfpv->dwFPIndex,
										pcsfpv->szName, pcsfpv->dwValueType,
										pcsfpv->bValue, sizeof(pcsfpv->bValue), fCreate);
			break;
		case eValue:
				lResult = CS_VALUE::SetValueWithFPV(hRegKey, pcsfpv);
			break;
		default:
			lResult = ERROR_INVALID_PARAMETER; 
			break;
	}

Exit:
	
	CScloseRegKey(hRegKey);
	
	return lResult;

}

///////////////////////////////////////////////////////////
// Compose the SNI root path based major version.
//
// inputs:
//		pszRootPath[out]: the buffer that hold the composed the
//				null-term path string.
//		dwcbRootPath[in]: the buffer size.
//		usMajor[in]: the major version of SNI.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetSNIClientRootPath(TCHAR* pszRootPath, DWORD dwcbRootPath, USHORT usMajor)
{
	LONG lResult = ERROR_SUCCESS;

	if( VER_PRE90_MAJOR == usMajor)
	{
		if( FAILED ( StringCchPrintf(pszRootPath, dwcbRootPath/sizeof(TCHAR),					   
						   CLIENTREG SNIROOTKEY_PRE90)))
		{
			lResult = ERROR_INSUFFICIENT_BUFFER ;
		}
	}
	else if (VER_PRE90_MAJOR < usMajor)
	{
		if( FAILED ( StringCchPrintf(pszRootPath, dwcbRootPath/sizeof(TCHAR),					   
						   CLIENTREG SNIROOTKEYPREFIX TEXT("%d.0"),
						   usMajor)))
		{
			lResult = ERROR_INSUFFICIENT_BUFFER ;
		}
	}
	else
	{
		lResult =  ERROR_INVALID_PARAMETER;
	}

	return lResult;
}


///////////////////////////////////////////////////////////
// Convert a string to dword.
//	
// inputs:
//		pszStr[in]: String to covert.
//		dwout
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSStringToDW( const TCHAR* pszStr, USHORT* pdwOut)
{	
	const TCHAR* pszTmp = pszStr;
	TCHAR ch;
	USHORT retVal  = 0;
	
	if( NULL == pszStr || NULL == pdwOut )
	{
		return ERROR_INVALID_PARAMETER;
	}

	while( ch = *pszTmp++)
	{
		if(ch >= L'0' && ch <=L'9' )
			ch-='0';
		else
			return ERROR_INVALID_PARAMETER;	//invalid string

		if (FAILED (UShortMult(retVal, 10, &retVal)))
			return ERROR_ARITHMETIC_OVERFLOW;
		if (FAILED (UShortAdd(retVal, (UINT)ch, &retVal)))
			return ERROR_ARITHMETIC_OVERFLOW;
	}

	*pdwOut = retVal;

	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////
// Parse out the version given a registry path string.
//
// inputs:
//		pszPath[in]: the path to be evaluated. the content
//			of the buffer may be modified by this function and should
//			not be reused after calling this function.
//		dwcbPath[in]: size of the pszPath in bytes, including null-terminator.
//		pusMajor[out]: the version parsed out.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//	1.	version string should be in form of SNImm.n, mm is the Major, 
//		n is the minor, or SuperSocketNetLib for MDAC. If version
//		string is not well formed, ERROR_INVALID_PARAMETER is 
//		returned and *pusMajor is undefined.
//
//	2.	pszPath can be modified and should not be used afterward.
//
LONG NLregC::CSgetVersion(__in_bcount(dwcbPath) TCHAR* pszPath, __in DWORD dwcbPath, __out USHORT* pusMajor)
{
	LONG lResult = ERROR_SUCCESS;
	TCHAR* pszTmp = pszPath;
	TCHAR szVersion[CS_MAX];
	USHORT usMinor = 0;
	DWORD dwcchLen = dwcbPath/sizeof(TCHAR);
	
	if( NULL == pszPath || NULL == pusMajor || CS_MAX < dwcbPath )
	{
		return ERROR_INVALID_PARAMETER;
	}
	
	if( dwcchLen >= ARRAYSIZE(SNIROOTKEY_PRE90) && !_tcsicmp(pszPath, SNIROOTKEY_PRE90))
	{
		*pusMajor = VER_PRE90_MAJOR;
	}
	else if(dwcchLen >= ARRAYSIZE(SNIROOTKEYPREFIX) && !_tcsnicmp(pszPath, SNIROOTKEYPREFIX, ARRAYSIZE(SNIROOTKEYPREFIX)-1))
	{
		dwcchLen -= ARRAYSIZE(SNIROOTKEYPREFIX)-1;  //not including the null terminator

		if(FAILED(StringCchCopyN(szVersion, ARRAYSIZE(szVersion), 
			pszTmp+ARRAYSIZE(SNIROOTKEYPREFIX)-1, dwcchLen)))
		{
			return ERROR_INVALID_PARAMETER;
		}

		szVersion[dwcchLen-1]=0x00;
			
		pszTmp = _tcschr(szVersion, L'.');
			
		if( pszTmp )
		{
			*pszTmp = 0;
			lResult = CSStringToDW(szVersion, pusMajor);
			CHKL_RETURN(lResult);

			pszTmp++;
			lResult = CSStringToDW(pszTmp, &usMinor);
			CHKL_RETURN(lResult);

			if(0 != usMinor)
			{
				return ERROR_INVALID_PARAMETER;
			}					
		}
		else
		{
			return ERROR_INVALID_PARAMETER;
		}
	}
	else
		return ERROR_INVALID_PARAMETER;

	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////
// Search the registry location for Latest SNI registry.
//	
// inputs:
//		pszRootPath[inout]: the buffer that, on return, hold the latest 
//			SNI registry path. Blank if nothing is found.
//		dwcbRootPath[in]: the buffer size.
//		pdwMajor[out]: the version number the latest SNI registry. 0 if 
//			nothing is found.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//		It searches for SNI that is earlier or equal to current version.
//
//
LONG NLregC::CSgetLatestSNIClientRootPathAndVersion
(
	TCHAR* pszRootPath, 
	DWORD dwcbRootPath, 
	USHORT* pdwMajor 
)
{
	HKEY hRegKey =  NULL;
	TCHAR szPath[CS_MAX];
	LONG lResult = ERROR_SUCCESS;
	DWORD dwIndex = 0;
	USHORT usMajor= 0 , usMajorLatest= 0;
	if( NULL == pszRootPath || NULL == pdwMajor )
	{
		return ERROR_INVALID_PARAMETER;
	}
	
	lResult = CSgetRegSubKey(NULL,CLIENTREG, &hRegKey, KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	*pszRootPath = 0x00;
	*pdwMajor=0;

	for(dwIndex = 0;dwIndex<ULONG_MAX; dwIndex++)
	{		
		bool fIsEmpty = true;
		DWORD dwcchBufferSize = ARRAYSIZE(szPath);
		
		if(ERROR_SUCCESS != RegEnumKeyEx( hRegKey,
							dwIndex,			// index of value to query
							szPath,			// value buffer
							&dwcchBufferSize,	// size of value buffer
							NULL,			// reserved
							NULL,			// type buffer
							NULL,			// data buffer
							NULL ))			// size of data buffer
		{
			break;		
		}

		if(ARRAYSIZE(szPath) <= dwcchBufferSize )
		{
			lResult = ERROR_INVALID_DATA;
			goto Exit;			
		}

		if (ERROR_SUCCESS != CSisEmptyRegSubKey(hRegKey, szPath, &fIsEmpty) || fIsEmpty)
		{
			// First check: we failed to retrieve or query the subkey - 
			// no need to fail entirely, we can just continue to the next subkey

			// Second check: if the key is completely empty, act like it doesn't exist 
			// and skip to the next one, because the root SNI key is written 
			// earlier in the SNAC MSI. If we detect latest version from that lone, empty
			// SNI key, then we won't actually port any downlevel settings.
			// See VSTS 280794 for details.
			continue;
		}

		if(ERROR_SUCCESS == CSgetVersion(szPath, (dwcchBufferSize+1)*sizeof(TCHAR), &usMajor))
		{
			if( usMajor > usMajorLatest 
				&& usMajor <= VER_SQL_MAJOR 
				&& usMajor >= VER_PRE90_MAJOR )
			{
				usMajorLatest = usMajor;
			}
		}
	}

	if(usMajorLatest)
	{
		lResult = CSgetSNIClientRootPath(pszRootPath, dwcbRootPath, usMajorLatest);
		CHKL_GOTO(lResult, Exit);
		*pdwMajor = usMajorLatest;
	}

Exit:

	CScloseRegKey(hRegKey);

	return lResult;	

}

// CSisEmptyRegSubKey
//
//	A helper function that determines whether a registry key's sub key has any child keys or values
//
//	Inputs:
//		hRootRegKey     = [in] A handle to the root registry key
//
//		pszPath         = [in] A pointer to a buffer containing the 
//							relative path to a child reg key
//
//		pfIsEmpty   	= [out] A bool pointer that holds the result of the 
// 							check for no children keys or values; used 
//							only if original lResult is success
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure
//
//
LONG NLregC::CSisEmptyRegSubKey
(
	HKEY hRootRegKey,
	TCHAR* pszPath, 
	bool *pfIsEmpty
)
{
	HKEY hRegSubKey = NULL;
	DWORD lResult;
	if (NULL == pfIsEmpty || NULL == pszPath || NULL == hRootRegKey)
	{
		lResult = ERROR_INVALID_PARAMETER;
		goto Exit;
	}
	// Default to report an empty key if we are unable to read anything; 
	// caller should check the return code before relying on the bool
	*pfIsEmpty = true;
	
	DWORD dwcSubKeys;
	DWORD dwcValues;
	
	lResult = CSgetRegSubKey(hRootRegKey, pszPath, &hRegSubKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);
	
	lResult = RegQueryInfoKey( hRegSubKey,
							   NULL,
							   NULL,
							   NULL,
							   &dwcSubKeys,
							   NULL,
							   NULL,
							   &dwcValues,
							   NULL,
							   NULL,
							   NULL,
							   NULL ); 
	CHKL_GOTO(lResult, Exit);
	
	if (0 != dwcSubKeys || 0 != dwcValues)
	{
		*pfIsEmpty = false;
	}

	
Exit:
	CScloseRegKey(hRegSubKey);
	
	return lResult;
}

///////////////////////////////////////////////////////////
// Set default or migrate SNI registy values.
//
// inputs:
//		dwIndex[in]: the index to the value array that is to be set.
//		hSNIRootNew[in]: the current SNI registry root key.
//		hSNIRootOld[in]: the latest SNI registry root key if any. It
//				should not point to the same location as the 
//				hSNIRootNew	does.
//		fOverWrite[in]: whether to overwrite or migrate.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//		See spec for detailed logic.
//
LONG NLregC::CSdefaultOrmigrateSNIFPV
(
	const DWORD dwIndex, 
	const HKEY hSNIRootNew, 
	const HKEY hSNIRootOld,
	BOOL fOverWrite
)
{
	LONG lResult = ERROR_SUCCESS;
	
	if (dwIndex >= sizeof(csDefaultSNIFPVs)/sizeof(CS_FPV_DEFAULT) ||!hSNIRootNew)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	{	// Block needed to prevent compiler warning for csfpv init skipped by goto Exit above
	
	CS_FPV csfpv(eFPVUnknown, NULL, -1, REG_NONE);

	lResult = csfpv.Copy(&csDefaultSNIFPVs[dwIndex]);
	CHKL_GOTO(lResult, Exit);
	
	if(!fOverWrite && !csDefaultSNIFPVs[dwIndex].fOverwriteAlways)
	{
		// Initialize result in case we don't get to read old value
		// so the reinit with default is covered.
		lResult = ERROR_SUCCESS;

		if( !hSNIRootOld)
		{
			// !hSNIRootOld means to update current registry.
			// Check if there already is a value in the hive of current version.
			// If so, use that value instead of the default value.
			lResult = CSgetFPV(hSNIRootNew, &csfpv);
		}
		else
		{
			//Check if need to migrate from old hive.
			//
			if( csDefaultSNIFPVs[dwIndex].fmigrate )
			{
				lResult = CSgetFPV(hSNIRootOld, &csfpv);
			}					
		}

		// Reset the value to default if fail to query existing value.
		//
		if(ERROR_SUCCESS != lResult )
		{				
			lResult = csfpv.Copy(&csDefaultSNIFPVs[dwIndex]);
			CHKL_GOTO(lResult, Exit);
		}	
	}

	lResult = CSsetFPV(hSNIRootNew, &csfpv, CS_CREATE);	

	}

Exit:

	return lResult;

}


///////////////////////////////////////////////////////////
// Migrate MDAC protocolorder
//
// inputs:
//		pszSNIPO[out]: SNI protocol order that, on success, will be
//			filled in.
//		dwcbSNIPO[in]: size of the buffer that hold the SNI protocol
//			order.
//		pszMdacPO[in]: MDAC protocol order.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//	1.	Only migrate supported protocol order.
//	2.	Always set SM on.
//
LONG NLregC::CSmigrateMDACProtocolOrder
(
	TCHAR* pszSNIPO,
	DWORD dwcbSNIPO,
	TCHAR* pszMdacPO
)
{
	DWORD dwCchBufferFilledSize = 0, dwCchBufferLeft =dwcbSNIPO/sizeof(TCHAR) ;
	LONG lResult =  ERROR_SUCCESS;
	const TCHAR szSm[]=TEXT("sm");
		
	if ( NULL == pszSNIPO || NULL == pszMdacPO || 0 == dwcbSNIPO )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

       pszSNIPO[0] = 0x00;
	   
	//always add sm in the protocol list if migrat from MDAC.
	//
	if( FAILED (StringCchCopy(pszSNIPO, dwCchBufferLeft, szSm)))
	{
		lResult = ERROR_INVALID_PARAMETER;
		goto Exit;
	}
	
	dwCchBufferFilledSize += sizeof(szSm)/sizeof(TCHAR);	
	dwCchBufferLeft -= sizeof(szSm)/sizeof(TCHAR);
		
	for( TCHAR* pszProtocolOrderMdac = pszMdacPO;
		*pszProtocolOrderMdac != 0x00;							    // are we at end of protocol list?
		pszProtocolOrderMdac += _tcslen( pszProtocolOrderMdac ) + 1 )  // position to next protocol
	{
	        for( TCHAR* pszProtocolSupported = DEFAULT_PROTOCOLS_SUPPORTED;               // get start of protocol list
	        	*pszProtocolSupported != 0x00;							        // are we at end of protocol list?
	        	pszProtocolSupported += _tcslen( pszProtocolSupported ) + 1 )	// position to next protocol
            {
                if( !_tcsicmp(pszProtocolOrderMdac, pszProtocolSupported) && _tcsicmp(pszProtocolOrderMdac, szSm))
                {                	
                	(void) StringCchCopy( &pszSNIPO[dwCchBufferFilledSize], 
								dwCchBufferLeft, 
								pszProtocolOrderMdac );
			dwCchBufferFilledSize += (DWORD) _tcslen( pszProtocolOrderMdac ) + 1;
			dwCchBufferLeft	-= (DWORD) _tcslen( pszProtocolOrderMdac ) + 1;
	
			break;
                }
            }

	  }	// for(;;)

        pszSNIPO[dwCchBufferFilledSize] = 0x00;
		
Exit:
	
	return lResult;

}


///////////////////////////////////////////////////////////
// Set default or migrate MDAC registry entries.
//
// inputs:
//		dwIndex[in]: index to value array of current version.
//		hSNIRoot[in]: SNI root registry handle.
//		hMdacRoot[in]: MDAC root registry handle.
//		
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSdefaultOrmigrateMDACFPV
(
	__in const DWORD dwIndex,
	__in const HKEY hSNIRoot,
	__in const HKEY hMdacRoot
)
{
	LONG lResult = ERROR_SUCCESS;
	
	if (dwIndex >= sizeof(csDefaultSNIFPVs)/sizeof(CS_FPV_DEFAULT) 
		||!hSNIRoot ||!hMdacRoot)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	{
	CS_FPV csfpv(eFPVUnknown, NULL, -1, REG_NONE);

	lResult = csfpv.Copy(&csDefaultSNIFPVs[dwIndex]);
	CHKL_GOTO(lResult, Exit);

	//Retrieve MDAC value and translate if needed
	//	
	MDACFPV  mdacnodeindex = csDefaultSNIFPVs[dwIndex].mdacNode;	
	switch(mdacnodeindex)
	{
		case eMDACProtocolOrder:
		case eMDACEncrypt:
		case eMDACDefaultPort:
		case eMDACDefaultPipe:
			{
			CS_FPV csfpvmdac(eFPVUnknown, NULL, -1, REG_NONE);

			lResult = csfpvmdac.Copy(&csMDACFPVs[mdacnodeindex]);
			if(ERROR_SUCCESS != lResult)
				break;

			//Retrieve the value from MDAC hive.
			lResult = CSgetFPV(hMdacRoot, &csfpvmdac);
			if(ERROR_SUCCESS != lResult)
				break;

			switch(csfpvmdac.dwValueType)
			{
				case REG_DWORD:					
					csfpv.dwValue = csfpvmdac.dwValue;
					break;
				case REG_SZ:
				case REG_MULTI_SZ:
					if(eMDACDefaultPipe == mdacnodeindex)
					{
						memcpy(csfpv.szValue, csfpvmdac.szValue, sizeof(csfpv.bValue));
					}
					else if(eMDACProtocolOrder == mdacnodeindex)
					{
						lResult = CSmigrateMDACProtocolOrder(csfpv.szValue, sizeof(csfpv.bValue), csfpvmdac.szValue);
					}											
					break;
				default:
					lResult = ERROR_INVALID_PARAMETER;
					break;
			}
			}
		default:	
			//do not migrate such value from MDAC.
			break;
	}

	if(ERROR_SUCCESS != lResult )
	{
		//If any error in translation, reset to default value.
		 lResult = csfpv.Copy(&csDefaultSNIFPVs[dwIndex]);	
		CHKL_GOTO(lResult, Exit);
	}					

	lResult = CSsetFPV(hSNIRoot, &csfpv, CS_CREATE);
	}
	
Exit:

	return lResult;

}


///////////////////////////////////////////////////////////
// Set or Migrate registry hive of current SNI version.
//
// inputs:
//		fOverWriteAll[in]: whether to use default of migrate.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//		See functional/design spec for details.
//
// To reviewer:		
//		Ignore failure and use default in some cases???
//
LONG NLregC::CSsetDefaults( BOOL fOverWriteAll )
{
	LONG lResult = ERROR_SUCCESS;
	TCHAR szSNILatestRootPath[CS_MAX];
	DWORD dwcbSNILatestRootPath = sizeof(szSNILatestRootPath);
	USHORT usLatestMajor = 0;
	DWORD iFPV=0;
	HKEY hRootNew = NULL, hRootOld = NULL;
	
	if(fOverWriteAll)
	{
		lResult = CSgetSNIRootRegKey( &hRootNew, KEY_WRITE | KEY_QUERY_VALUE, CS_CREATE);
		CHKL_GOTO(lResult, Exit);

		for( iFPV = 0; iFPV < sizeof(csDefaultSNIFPVs)/sizeof(CS_FPV_DEFAULT); iFPV++)
		{
			lResult = CSdefaultOrmigrateSNIFPV(iFPV, hRootNew, NULL, fOverWriteAll);
			CHKL_GOTO(lResult, Exit);
		}

		CScreateLastConnectionCache(TRUE);
	}
	else
	{
		lResult = CSgetLatestSNIClientRootPathAndVersion(szSNILatestRootPath, dwcbSNILatestRootPath, &usLatestMajor);
		CHKL_GOTO(lResult, Exit);

		if( VER_PRE90_MAJOR <= usLatestMajor && VER_SQL_MAJOR > usLatestMajor)
		{
			lResult = CSgetRegSubKey(NULL,szSNILatestRootPath, &hRootOld, KEY_QUERY_VALUE, CS_OPEN);
			CHKL_GOTO(lResult, Exit);
		}

		lResult = CSgetSNIRootRegKey( &hRootNew, KEY_WRITE | KEY_QUERY_VALUE, CS_CREATE);
		CHKL_GOTO(lResult, Exit);

		if( VER_PRE90_MAJOR == usLatestMajor )
		{
			Assert(hRootOld);
			for( iFPV = 0; iFPV < sizeof(csDefaultSNIFPVs)/sizeof(CS_FPV_DEFAULT); iFPV++)
			{
				lResult = CSdefaultOrmigrateMDACFPV(iFPV, hRootNew, hRootOld);
				CHKL_GOTO(lResult, Exit);
			}	
		}
		else
		{
			for( iFPV = 0; iFPV < sizeof(csDefaultSNIFPVs)/sizeof(CS_FPV_DEFAULT); iFPV++)
			{
				lResult = CSdefaultOrmigrateSNIFPV(iFPV, hRootNew, hRootOld, fOverWriteAll);
				CHKL_GOTO(lResult, Exit);
			}
		}

		if( VER_PRE90_MAJOR <= usLatestMajor )
		{
			CScreateLastConnectionCache(TRUE);
		}
	}
	
Exit:

	CScloseRegKey(hRootNew);
	CScloseRegKey(hRootOld);

	return lResult;
}


///////////////////////////////////////////////////////////
// Get number of flags of GeneralFlags
//
// inputs:
//		pdwNumberOfFlags[out]: buffer to the number of flag returned.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetNumberOfGeneralFlags( DWORD * pdwNumberOfFlags )
{
	return CSgetNumberOfProtocolFlags((TCHAR*)szGeneralFlagPath, pdwNumberOfFlags);
}

///////////////////////////////////////////////////////////
// Retrive value and label of a flag of GeneralFlags
//
// inputs:
//		dwFlagIndex[in]: the index to the flag.
//		szLabel[out]: the buffer that, on success, hold the Label of the flag.
//		dwcbLabel[in]: the size of the buffer.
//		*pdwFlagState[out]: the value of the flag.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetGeneralFlagProperty( __in DWORD   dwFlagIndex,
									   __out_bcount(dwcbLabel) __nullterminated TCHAR   szLabel[CS_MAX],
									   __in DWORD dwcbLabel,
									   __out DWORD * pdwFlagState )
{
	return
	CSgetProtocolFlag( (TCHAR*)szGeneralFlagPath, dwFlagIndex, szLabel, dwcbLabel, pdwFlagState);
}

///////////////////////////////////////////////////////////
// Set value of a flag of GeneralFlags
//
// inputs:
//		dwFlagIndex[in]: the index to the flag.
//		dwFlagState[in]: the value to be set.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetGeneralFlagProperty( DWORD  dwFlagIndex,
									   DWORD  dwFlagState )
{
	return CSsetProtocolFlag( (TCHAR*)szGeneralFlagPath,dwFlagIndex, dwFlagState);
}

///////////////////////////////////////////////////////////
// Retrive supported protocols.
//
// inputs:
//		pszProtocolsSupprted[out]: the buffer that, on success, hold
//			the supported protocol.
//		pdwBufferSize[inout]: in with the buffer size, out with buffer
//			size including the null-terminator, in bytes.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//	1. 	if pszProtocolsSupported is null, this function retrive the size 
//		of the buffer needed.
//
LONG NLregC::CSgetProtocolsSupported( __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * pszProtocolsSupported,
									  __inout DWORD * pdwBufferSize )
{
	return CSgetProtocolsSupportedOrOrder( TEXT("ProtocolsSupported"),
									   pszProtocolsSupported,
									   pdwBufferSize );
}


///////////////////////////////////////////////////////////
// Retrive protocol orders.
//
// inputs:
//		pszProtocolOrder[out]: the buffer that, non success, hold
//			the protocol order.
//		pdwBufferSize[inout]: in with buffer size, out with buffer
//		size including the null-terminator, in bytes.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//	1. 	if pszProtocolOrder is null, this function retrive the size 
//		of the buffer needed.
//
LONG NLregC::CSgetProtocolOrder(  __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * pszProtocolOrder,
								 __inout DWORD * pdwBufferSize )
{
	return CSgetProtocolsSupportedOrOrder( TEXT("ProtocolOrder"),
									   pszProtocolOrder,
									   pdwBufferSize );
}


///////////////////////////////////////////////////////////
// Retrieve protocol order of supported protocol.
//
// inputs:
//		pszRegistryItem[in]: weather it is ProtocolOrder or ProtocolsSupported
//		pszProtocolsSupportedOrOrder[out]: the buffer.
//		pdwBufferSize[inout]: in with buffer size, out with buffer
//			size including the null-terminator, in bytes.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
// 	1.	the code guarantees that query missing value returns the 
//		proper defaults.
//
LONG NLregC::CSgetProtocolsSupportedOrOrder
( 
	 __in __nullterminated TCHAR * pszRegistryItem,
	 __out_bcount_opt(*pdwBufferSize) __nullterminated TCHAR * pszProtocolsSupportedOrOrder,
	 __in DWORD * pdwBufferSize
)
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;

	if( NULL == pszRegistryItem || NULL == pdwBufferSize )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNIRootRegKey( &hRegKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);
	
	lResult = CS_VALUE::GetString(hRegKey,pszRegistryItem, REG_MULTI_SZ, pszProtocolsSupportedOrOrder, pdwBufferSize);

	if( lResult == ERROR_FILE_NOT_FOUND )
	{
		//Registry key is missing, use default value.
		//
		if (NULL ==  pszProtocolsSupportedOrOrder )
		{
			*pdwBufferSize = sizeof( DEFAULT_PROTOCOLS );	//	Returning ERROR_FILE_NOT_FOUND? 
		}
		else
		{
			if( *pdwBufferSize < sizeof(DEFAULT_PROTOCOLS) )
			{
				lResult = ERROR_BUFFER_OVERFLOW; 
				goto Exit; 
			}
			
			if( *pdwBufferSize > sizeof(DEFAULT_PROTOCOLS) )
				*pdwBufferSize = sizeof( DEFAULT_PROTOCOLS );

			memcpy( pszProtocolsSupportedOrOrder,
					DEFAULT_PROTOCOLS,
					*pdwBufferSize );
		}
	}

Exit:

	CScloseRegKey(hRegKey);

	return lResult;
}


///////////////////////////////////////////////////////////
// Set the protocol order.
//
// inputs:
//		pszProtocolOrder[in]: null-terminated multi-str.
//	
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetProtocolOrder( __in const TCHAR * pszProtocolOrder )
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;

	if( NULL == pszProtocolOrder )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNIRootRegKey( &hRegKey, KEY_WRITE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CS_VALUE::SetString(hRegKey, TEXT("ProtocolOrder"), REG_MULTI_SZ, pszProtocolOrder, CS_MAX);

Exit:

	CScloseRegKey(hRegKey);

	return lResult;
}

///////////////////////////////////////////////////////////
// Retrive a DWORD Value of a specified protocol.
//
// inputs:
//		pszProtocol[in]: the protocol name.
//		pszName[in]: the value name.
//		pdwValue[out]: the DWORD value buffer pointer.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetProtocolDwValue(const TCHAR* pszProtocol, const TCHAR* pszName, DWORD* pdwValue)
{

	LONG lResult = ERROR_SUCCESS;
	HKEY hRegSubKey = NULL;

	if( NULL == pszProtocol || NULL == pszName || NULL == pdwValue )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}
	
	lResult  = CSgetSNISubRegKey(pszProtocol, &hRegSubKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CS_VALUE::GetDw(hRegSubKey,pszName, pdwValue);

Exit:

	CScloseRegKey(hRegSubKey);
	
	return lResult;
}

///////////////////////////////////////////////////////////
// Retrieve str value of a specified protocol.
//
// inputs:
//		pszProtocol[in]: the protocol name.
//		pszName[in]: the value name.
//		pszValue[out]: the str value buffer pointer.
//		dwcbValue[in]: the str buffer size.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetProcolStringValue
(
	const TCHAR* pszProtocol, 
	const TCHAR* pszName, 
	TCHAR* pszValue, 
	DWORD dwcbValue
)
{
	LONG  lResult = ERROR_SUCCESS;
	HKEY hRegSubKey = NULL;
	
	if( NULL == pszProtocol || NULL == pszName || NULL ==  pszValue || 0 == dwcbValue)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult  = CSgetSNISubRegKey(pszProtocol, &hRegSubKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CS_VALUE::GetString(hRegSubKey, pszName, REG_SZ, pszValue, &dwcbValue);
	
Exit:
	
	CScloseRegKey(hRegSubKey);
	
	return lResult;

}

///////////////////////////////////////////////////////////
// Retrieve number of flag of a specified protocol.
//
// inputs:
//		szProtocol[in]: the protocol name.
//		pdwNumberOfFlags[out]: the value buffer of the number of flags.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetNumberOfProtocolFlags(TCHAR * szProtocol,
										 DWORD * pdwNumberOfFlags )
{
	return CSgetProtocolDwValue(szProtocol, szNumberOfFlags, pdwNumberOfFlags);
}

///////////////////////////////////////////////////////////
// Retrieve number of properties of a specified protocol.
//
// inputs:
//		szProtocol[in]: the protocol name.
//		pdwNumberOfProperties[out]: the value buffer of the number of properties.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetNumberOfProtocolProperties( TCHAR	* szProtocol,
											  DWORD * pdwNumberOfProperties )
{
	return CSgetProtocolDwValue(szProtocol, szNumberOfProperties, pdwNumberOfProperties);
}

///////////////////////////////////////////////////////////
// Retrieve a protocol info of a specified protocol.
//
// inputs:
//		szProtocol[in]: the protcol name.
//		pProtocolInfo[in]: pointer to protocol structure.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetProtocolInfo( TCHAR            * szProtocol,
								CS_PROTOCOL_INFO * pProtocolInfo )
{
	LONG  lResult = ERROR_SUCCESS;

	if( NULL == szProtocol || NULL == pProtocolInfo )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetProcolStringValue(szProtocol, szDllName, 
								pProtocolInfo->szDLLname,sizeof(pProtocolInfo->szDLLname));
	CHKL_GOTO(lResult, Exit);

	lResult = CSgetProcolStringValue(szProtocol, szProtocolName, 
								pProtocolInfo->szProtocolName,sizeof(pProtocolInfo->szProtocolName));
	CHKL_GOTO(lResult, Exit);
	

	lResult = CSgetProtocolDwValue(szProtocol, szNumberOfFlags, &pProtocolInfo->dwNumberOfFlags);
	CHKL_GOTO(lResult, Exit);

	lResult = CSgetProtocolDwValue(szProtocol, szNumberOfProperties, &pProtocolInfo->dwNumberOfProperties);
	CHKL_GOTO(lResult, Exit);

Exit:

	return lResult;
}


///////////////////////////////////////////////////////////
// Get the name and value of a flag of specified protocol.
//
// inputs:
//		szProtocol[in]: name of the protocol.
//		dwFlagIndex[in]: index to a flag.
//		szFlagLabel[out]: the label of a flag, can be null.
//		dwcbFlagLabel[in]: size of buffer pointed by szFlagLabel.
//		pdwFlagValue[out]: the buffer of the flag value.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetProtocolFlag( __in __nullterminated TCHAR * szProtocol,
								__in DWORD   dwFlagIndex,
								__out_bcount_opt(dwcbFlagLabel) __nullterminated TCHAR * szFlagLabel,
								__in_range(0,CS_MAX * sizeof (TCHAR)) DWORD dwcbFlagLabel,
								__out DWORD * pdwFlagValue )
{
	HKEY  hRegKey = NULL;
	LONG  lResult = 0;
	DWORD dwType= REG_DWORD;
	DWORD dwcbFlag= sizeof(DWORD);

	if( szFlagLabel != NULL && ( dwcbFlagLabel > CS_MAX * sizeof (TCHAR) || dwcbFlagLabel == 0 ) )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit; 
	}

	lResult = CSgetSNISubRegKey( szProtocol, &hRegKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	if( szFlagLabel != NULL )
	{
		szFlagLabel[0] = 0;
	}
	
	lResult = CSgetFlagOrProperty(hRegKey, eFlag, dwFlagIndex, 
							szFlagLabel, dwcbFlagLabel, &dwType, 
							(BYTE*)pdwFlagValue, &dwcbFlag);

Exit:

	CScloseRegKey( hRegKey );

	return lResult;
}

///////////////////////////////////////////////////////////
// Set a flag value of a specified protocol.
//
// inputs:
//		szProtocol[in]: the protocol name.
//		dwFlagIndex[in]: index to a flag.
//		dwFlagValue[in]: flag value to be set.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetProtocolFlag( TCHAR * szProtocol,
								DWORD   dwFlagIndex,
								DWORD   dwFlagValue )
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;

	if( NULL == szProtocol )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNISubRegKey( szProtocol, &hRegKey, KEY_WRITE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CSsetFlagOrProperty(hRegKey, eFlag, dwFlagIndex, NULL, REG_DWORD, 
							(BYTE*)&dwFlagValue, sizeof(DWORD), CS_OPEN);

Exit:

	CScloseRegKey(hRegKey);

	return lResult;
}

						
///////////////////////////////////////////////////////////
// Retrieve protocol property of a specified protocol.
//
// inputs:
//		szProtocol[in]: protocol name.
//		dwPropertyIndex[in]: index to a property.
//		pProtocolProperty[out]: the structure that hold the property 
//			of a protocol.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetProtocolProperty( __in __nullterminated TCHAR			 	 * szProtocol,
									__in DWORD				   dwPropertyIndex,
									__out CS_PROTOCOL_PROPERTY * pProtocolProperty )
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;
	DWORD dwcbValue = sizeof(pProtocolProperty->PropertyValue.szStringValue);
	
	if( NULL == szProtocol || NULL == pProtocolProperty)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNISubRegKey( szProtocol, &hRegKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	memset(pProtocolProperty, 0, sizeof(CS_PROTOCOL_PROPERTY));

	pProtocolProperty->dwPropertyType = REG_NONE;
	lResult = CSgetFlagOrProperty(hRegKey, eProperty, dwPropertyIndex, 
			pProtocolProperty->szPropertyName,sizeof(pProtocolProperty->szPropertyName),
			&pProtocolProperty->dwPropertyType, 
			(BYTE*)pProtocolProperty->PropertyValue.szStringValue,
			&dwcbValue);
Exit:

	CScloseRegKey(hRegKey);

	return lResult;
}

///////////////////////////////////////////////////////////
// Set protocol property of a specified protocol.
//
// inputs:
//		szProtocol[in]: the protocol name.
//		dwPropertyIndex[in]: the index to the protocol.
//		pProtocolProperty[in]: the protocol property structure.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetProtocolProperty( TCHAR			     * szProtocol,
									DWORD				   dwPropertyIndex,
									CS_PROTOCOL_PROPERTY * pProtocolProperty )
{
	HKEY  hRegKey = NULL;
	LONG  lResult = ERROR_SUCCESS;

	if( NULL == szProtocol || NULL == pProtocolProperty)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNISubRegKey(szProtocol,  &hRegKey, KEY_WRITE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CSsetFlagOrProperty(hRegKey, eProperty, dwPropertyIndex,
					pProtocolProperty->szPropertyName, 
					pProtocolProperty->dwPropertyType,
					(BYTE*)pProtocolProperty->PropertyValue.szStringValue, 	sizeof(pProtocolProperty->PropertyValue.szStringValue), 
					CS_OPEN);

Exit:

	CScloseRegKey(hRegKey);

	return lResult;
}

///////////////////////////////////////////////////////////
// Retrive the DBLIB info.
//
// inputs:
//		infoDBLIB[out]: CS_DBLIBINFO structure.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetDBLIBinfo( __out CS_DBLIBINFO * infoDBLIB )
{
	HKEY   hRegKey = NULL;
	LONG   lResult = 0;

	DWORD  dwBufferSize;
	DWORD  dwHandle      = 0;
	DWORD  dwVersionSize = 0;
	TCHAR szRegLocation[CS_MAX];

	TCHAR szAutoAnsiOem    [CS_MAX];
	TCHAR szUseIntlSettings[CS_MAX];

	DWORD dwRegType; 

	if( NULL == infoDBLIB )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	(void) StringCchCopy( infoDBLIB->szFileName, ARRAYSIZE(infoDBLIB->szFileName), TEXT("NTWDBLIB.DLL") );   

	infoDBLIB->dwProductVersionMS		 = 0;
	infoDBLIB->dwProductVersionLS		 = 0;
	infoDBLIB->dwDate				     = 0;
	infoDBLIB->dwSize				     = 0;
	infoDBLIB->fANSItoOEM			     = 0;
	infoDBLIB->fUseInternationalSettings = 0;

	// Registry settings
	//
	(void) StringCchCopy( szRegLocation, ARRAYSIZE(szRegLocation), CLIENTREG );   
	(void) StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), TEXT("DB-Lib\\") );   

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							szRegLocation,
							0,
							KEY_QUERY_VALUE,
							&hRegKey );

	if( lResult == ERROR_SUCCESS )
	{
		// Get AutoAnsiToOem
		//
		dwBufferSize = sizeof( szAutoAnsiOem );

		lResult = RegQueryValueEx( hRegKey,
								   TEXT("AutoAnsiToOem"),
								   NULL,
								   &dwRegType,
								   (UCHAR *)szAutoAnsiOem,
								   &dwBufferSize );

		lResult = CSstringNullTerminate( lResult, 
										 szAutoAnsiOem, 
										 sizeof( szAutoAnsiOem ), 
										 dwBufferSize ); 

		if( ( ERROR_SUCCESS == lResult ) && ( REG_SZ != dwRegType ) )
		{
			lResult = ERROR_INVALID_DATA; 
		}

		if( lResult == ERROR_SUCCESS )
		{
			if( !_tcsicmp(szAutoAnsiOem, TEXT("ON")) )
			{
				infoDBLIB->fANSItoOEM = TRUE;
			}
		}

		// Get UserIntlSettings
		//
		dwBufferSize = sizeof( szUseIntlSettings );

		lResult = RegQueryValueEx( hRegKey,
								   TEXT("UseIntlSettings"),
								   NULL,
								   &dwRegType,
								   (UCHAR *)szUseIntlSettings,
								   &dwBufferSize );

		lResult = CSstringNullTerminate( lResult, 
										 szUseIntlSettings, 
										 sizeof( szUseIntlSettings ), 
										 dwBufferSize ); 

		if( ( ERROR_SUCCESS == lResult ) && ( REG_SZ != dwRegType ) )
		{
			lResult = ERROR_INVALID_DATA; 
		}

		if( lResult == ERROR_SUCCESS )
		{
			if( !_tcsicmp(szUseIntlSettings, TEXT("ON")) )
			{
				infoDBLIB->fUseInternationalSettings = TRUE;
			}
		}
	}

	lResult = GetDateSizeVersion( infoDBLIB->szFileName,
								  &infoDBLIB->dwDate,
								  &infoDBLIB->dwSize,
								  &infoDBLIB->dwProductVersionMS,
								  &infoDBLIB->dwProductVersionLS,
                                  TRUE );

Exit:

	if( hRegKey )
		RegCloseKey( hRegKey );
	
	return lResult;
}


///////////////////////////////////////////////////////////
// Retrive the date, size, version of a file.
//
// inputs:
//		szFileName[in]: the file name.
//		dwDate[out]: the date returned.
//		dwSize[out]: the size returned.
//		dwProductVeresionMS[out]:
//		dwProductVersionLS[out]:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::GetDateSizeVersion( TCHAR * szFileName,
								 DWORD * dwDate,
								 DWORD * dwSize,
								 DWORD * dwProductVersionMS,
								 DWORD * dwProductVersionLS,
                                 BOOL    fDBLIB )
{
	LONG                      lResult;
	TCHAR                     szPathName[MAX_PATH + 1];
	WIN32_FILE_ATTRIBUTE_DATA FileAttritubes;

	szPathName[0]        = 0x00;
	szPathName[MAX_PATH] = 0x00;

	if( szFileName )
		(void) StringCchCopy( szPathName, ARRAYSIZE(szPathName), szFileName);   

	if( !PathFindOnPath(szPathName, NULL) )
	{
		lResult = ERROR_FILE_NOT_FOUND;
		goto Exit;
	}

	// Get the file size and date
	//
	if( !GetFileAttributesEx(szPathName,
							 GetFileExInfoStandard,
							 &FileAttritubes) )
	{
		lResult = GetLastError();
		goto Exit;
	}

    // GetFileTime(hFile,NULL,NULL,&ftLastWrite);


	*dwDate = FileAttritubes.ftLastWriteTime.dwHighDateTime;
	*dwSize = FileAttritubes.nFileSizeLow;

    // Load Net-Library DLL and get the version info
    //
	*dwProductVersionMS = 0;
	*dwProductVersionLS = 0;

	DWORD  dwHandle      = 0;
	DWORD  dwVersionSize = 0;

    TCHAR              szVersionInfo[CS_MAX];
    VS_FIXEDFILEINFO * pVersionResults;

	// Get the file Version
	//
	dwVersionSize = GetFileVersionInfoSize( szPathName,
										    &dwHandle );
	if( !dwVersionSize )
	{
		lResult = ERROR_FILE_NOT_FOUND;
		goto Exit;
	}

	// Retrieve version info buffer
	//
	if( !GetFileVersionInfo(szPathName,
							dwHandle,
							sizeof(szVersionInfo),
							szVersionInfo) )
	{
		lResult = GetLastError();
		goto Exit;
	}

	// Extract the version
	//
	if( !VerQueryValue(szVersionInfo,
					   TEXT("\\"),
					   (void **)&pVersionResults,
					   (UINT *)&dwVersionSize) )
	{
		lResult = GetLastError();
		goto Exit;
	}

	if( fDBLIB )
	{
    	*dwProductVersionLS = ((pVersionResults->dwProductVersionMS & 0x00FF0000) << 8)
            |  ((((pVersionResults->dwProductVersionLS & 0x00FF0000) >> 16) * 100)
                +  (pVersionResults->dwProductVersionLS & 0x0000FFFF));
	}
	else
	{
		//	Komodo'd WMI provider expects *dwProductVersionLS to contain 
		//	0xAABBCCCC, where
		//	- 0xAA = major, 
		//	- 0xBB = minor, 
		//	- 0xCCCC = "level" (we'll put build number).  
		//
    	*dwProductVersionLS = 
    		((pVersionResults->dwProductVersionMS & 0x00FF0000) << 8)
    		| ((pVersionResults->dwProductVersionMS & 0x000000FF) << 16)
            | ((pVersionResults->dwProductVersionLS & 0xFFFF0000) >> 16); 
	}

	lResult = ERROR_SUCCESS;

Exit:
	return lResult;
}


///////////////////////////////////////////////////////////
// Set DBLIB info.
// inputs:
//		infoDBLIB[in]: CS_DBLIBINFO structure.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetDBLIBinfo( __in CS_DBLIBINFO * infoDBLIB )
{
	HKEY   hRegKey = NULL;
	LONG   lResult = 0;
	TCHAR szRegLocation[CS_MAX];
	DWORD  dwBufferSize;
	TCHAR szAutoAnsiOem    [CS_MAX];
	TCHAR szUseIntlSettings[CS_MAX];

	if( NULL == infoDBLIB )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	if( infoDBLIB->fANSItoOEM )
		(void) StringCchCopy( szAutoAnsiOem, ARRAYSIZE(szAutoAnsiOem), TEXT("ON") );   
	else
		(void) StringCchCopy( szAutoAnsiOem, ARRAYSIZE(szAutoAnsiOem), TEXT("OFF") );   

	if( infoDBLIB->fUseInternationalSettings )
		(void) StringCchCopy( szUseIntlSettings, ARRAYSIZE(szUseIntlSettings), TEXT("ON") );   
	else
		(void) StringCchCopy( szUseIntlSettings, ARRAYSIZE(szUseIntlSettings), TEXT("OFF") );   

	// Registry settings
	//
	(void) StringCchCopy( szRegLocation, ARRAYSIZE(szRegLocation), CLIENTREG );   
	(void) StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), TEXT("DB-Lib\\") );   

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							szRegLocation,
							0,
							KEY_WRITE,
							&hRegKey );

	if( lResult != ERROR_SUCCESS )
		goto Exit;

	// Set AutoAnsiToOem
	//
	dwBufferSize = (DWORD)_tcslen( szAutoAnsiOem ) * sizeof( TCHAR );

	lResult = RegSetValueEx( hRegKey,
							 TEXT("AutoAnsiToOem"),
							 NULL,
							 REG_SZ,
							 (UCHAR *)szAutoAnsiOem,
							 dwBufferSize );

	if( lResult != ERROR_SUCCESS )
		goto Exit;

	// Set UserIntlSettings
	//
	dwBufferSize = (DWORD)_tcslen( szUseIntlSettings ) * sizeof( TCHAR );

	lResult = RegSetValueEx( hRegKey,
							 TEXT("UseIntlSettings"),
							 NULL,
							 REG_SZ,
							 (UCHAR *)szUseIntlSettings,
							 dwBufferSize );

Exit:

	if( hRegKey )
		RegCloseKey( hRegKey );

	return lResult;
}

///////////////////////////////////////////////////////////
// Retrive NETLIB info.
//	
// inputs:
//		szProtocol[in]: the protocol name.
//		infoNETLIB[out]: the CS_NETLIBINFO structure.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetNETLIBinfo( __in __nullterminated TCHAR		    * szProtocol,
							  __out CS_NETLIBINFO * infoNETLIB )
{
	LONG			 lResult = 0;
	CS_PROTOCOL_INFO ProtocolInfo;

	if( NULL == infoNETLIB || NULL == szProtocol)
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetProtocolInfo( szProtocol, &ProtocolInfo );
	CHKL_GOTO(lResult, Exit);

	(void) StringCchCopy( infoNETLIB->szProtocolName, ARRAYSIZE(infoNETLIB->szProtocolName), ProtocolInfo.szProtocolName );   

	if (FAILED ( StringCchPrintf( (TCHAR*) infoNETLIB->szDLLname ,
							ARRAYSIZE(infoNETLIB->szDLLname),
							TEXT("%s%s"),
							ProtocolInfo.szDLLname,
							TEXT(".DLL"))))
	{
		lResult = ERROR_INVALID_DATA;
		goto Exit;
	}					

	infoNETLIB->dwDate = 0;
	infoNETLIB->dwSize = 0;

	lResult = GetDateSizeVersion( infoNETLIB->szDLLname,
							&infoNETLIB->dwDate,
							&infoNETLIB->dwSize,
							&infoNETLIB->dwProductVersionMS,
							&infoNETLIB->dwProductVersionLS,
							FALSE );

Exit:
	return lResult;
}


///////////////////////////////////////////////////////////
// Get Aliases.
//
// inputs:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetAliases( __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * pszAliasesPresent,
						   __inout DWORD * pdwBufferSize )
{
	DWORD  dwIndex			 = 0;
	DWORD  dwValueBufferSize = 0;
	HKEY   hRegKey			 = NULL;
	LONG   lResult			 = 0;
	TCHAR  szRegLocation[CS_MAX];
	TCHAR  szAliasName[CS_MAX];

	if( NULL == pdwBufferSize )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	// Get Registry settings
	//
	(void) StringCchCopy( szRegLocation, ARRAYSIZE(szRegLocation), CLIENTREG );   
	(void) StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), TEXT("ConnectTo") );   

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							szRegLocation,
							0,
							KEY_QUERY_VALUE,
							&hRegKey );

	if( lResult != ERROR_SUCCESS )
	{
		// Preserving 2.x cliconfg behavior: if the "ConnecTo" key is
		// missing return an empty list.  Note: we are not creating 
		// this key during Setup because (a) it is not owed by NLregC/SNI, 
		// (b) it is shared with MDAC 2.x and System.Data.SqlClient.  
		//
		// Note: we'll do this on any error (e.g. lack of permission), 
		// not just missing key.  
		//
		lResult = ERROR_SUCCESS;

		if( !pszAliasesPresent )	// If we are getting buffer size requirement
		{
			*pdwBufferSize = sizeof( TCHAR );	// double NULL terminator
		}
		//	Check if there is space for the NULL-termination.  
		//
		else if( 0 >= *pdwBufferSize / sizeof( TCHAR ) )
		{
			*pdwBufferSize = 0;	// To preserve behavior with an existing but empty key

			lResult = ERROR_MORE_DATA; 
			goto Exit;
		}
		else	// return an empty list
		{
			// NULL terminate
			//
			*pszAliasesPresent = 0x00;				
			*pdwBufferSize     = sizeof( TCHAR );
		}

		goto Exit;
	}


	if( !pszAliasesPresent )	// If we are getting buffer size requirement
	{
		*pdwBufferSize    = 0;

		do
		{
			dwValueBufferSize = CS_MAX;

			lResult = RegEnumValue( hRegKey,
									dwIndex,			// index of value to query
									szAliasName,		// value buffer
									&dwValueBufferSize,	// size of value buffer
									NULL,				// reserved
									NULL,				// type buffer
									NULL,				// data buffer
									NULL );				// size of data buffer

			if( lResult == ERROR_SUCCESS )
			{
				dwIndex++;

				if( !_tcsicmp(szAliasName, TEXT("DSQUERY")) )
					continue;

				BOOL fSupportedAlias;
				
				lResult = CSisSupportedAlias(szAliasName, &fSupportedAlias); 

				//	Skip invalid or unsupported aliases.  
				//
				if( ERROR_SUCCESS != lResult )
				{
					lResult = ERROR_SUCCESS; 
					continue; 
				}
				else if( !fSupportedAlias )
				{
					continue; 
				}

				*pdwBufferSize += ((DWORD)_tcslen( szAliasName ) + 1) * sizeof( TCHAR );
			}
		}
		while( lResult == ERROR_SUCCESS );

		if( lResult == ERROR_NO_MORE_ITEMS )
			lResult = ERROR_SUCCESS;

		*pdwBufferSize += sizeof( TCHAR );	// double NULL terminator
	}
	else	// Let's retrive the actual Alias list
	{
		DWORD dwTcharSize;
		DWORD dwTcharSizeRemaining = *pdwBufferSize / sizeof( TCHAR );

		*pdwBufferSize = 0;

		do
		{
			dwTcharSize = sizeof( szAliasName ) / sizeof ( szAliasName[0] ); 

			//	Read into temporary buffer since "DSQUERY" may not 
			//	fit into the user-supplied buffer.  
			//
			lResult = RegEnumValue( hRegKey,
									dwIndex,			// index of value to query
									szAliasName,		// value buffer
									&dwTcharSize,		// size of value buffer
									NULL,				// reserved
									NULL,				// type buffer
									NULL,				// data buffer
									NULL );				// size of data buffer

			if( lResult == ERROR_SUCCESS )
			{
				dwIndex++;

				if( !_tcsicmp(szAliasName, TEXT("DSQUERY")) )
					continue;

				BOOL fSupportedAlias;
				
				lResult = CSisSupportedAlias(szAliasName, &fSupportedAlias); 

				//	Skip invalid or unsupported aliases.  
				//
				if( ERROR_SUCCESS != lResult )
				{
					lResult = ERROR_SUCCESS; 
					continue; 
				}
				else if( !fSupportedAlias )
				{
					continue; 
				}

				//	Copy the alias name into the caller-supplied buffer.  
				//	Make sure we avoid buffer overruns.  
				//
				if ( FAILED ( StringCchPrintf( pszAliasesPresent,
											dwTcharSizeRemaining,												
											TEXT("%s"),
											szAliasName )))

				{
					lResult = ERROR_MORE_DATA; 
					goto Exit;
				}

				SIZE_T cchWritten = _tcslen( pszAliasesPresent );

				if( cchWritten != dwTcharSize )
				{
					lResult = ERROR_MORE_DATA; 
					goto Exit;
				}

				*pdwBufferSize       += (dwTcharSize + 1) * sizeof( TCHAR );
				dwTcharSizeRemaining -= dwTcharSize + 1;
				pszAliasesPresent    += dwTcharSize + 1;

			}
		}
		while( lResult == ERROR_SUCCESS );

		if( lResult == ERROR_NO_MORE_ITEMS )
			lResult = ERROR_SUCCESS;

		//	Check if there is space for the double 
		//	NULL-termination.  
		//
		if( 0 >= dwTcharSizeRemaining )
		{
			lResult = ERROR_MORE_DATA; 
			goto Exit;
		}

		// Double NULL terminate
		//
		*pszAliasesPresent = 0x00;				
		*pdwBufferSize    += sizeof( TCHAR );
	}

Exit:
	if( hRegKey )
		RegCloseKey( hRegKey );

	return lResult;
}


///////////////////////////////////////////////////////////
// Get an alias.
//
// inputs:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
LONG NLregC::CSgetAlias( TCHAR	  * pszAliasName,
						 CS_ALIAS * pCSalias )
{
	HKEY    hRegKey = NULL;
	LONG    lResult = 0;
	DWORD   dwBufferSize;
	DWORD	dwType; 
	TCHAR   szREG_SZvalue[CS_MAX * 3];	//	3 times: szTarget, 
										//	szConnectionString, plus 
										//	extra stuff added by us
										//	(actually, much less than
										//	additional CS_MAX)
										//
	TCHAR   szRegLocation[CS_MAX];
	TCHAR * pszConnectString;

	(void) StringCchCopy( szRegLocation, ARRAYSIZE(szRegLocation), CLIENTREG );   
	(void)StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), TEXT("ConnectTo") );   

	// Get Registry settings
	//
    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							szRegLocation,
							0,
							KEY_QUERY_VALUE,
							&hRegKey );

	if( lResult != ERROR_SUCCESS )
		goto Exit;

	if( NULL == pszAliasName )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	// Get ConnectionString
	//
	dwBufferSize = sizeof( szREG_SZvalue );

	lResult = RegQueryValueEx( hRegKey,
							   pszAliasName,
							   NULL,
							   &dwType,
							   (UCHAR *)szREG_SZvalue,
							   &dwBufferSize );

	lResult = CSstringNullTerminate( lResult, 
									 szREG_SZvalue, 
									 sizeof( szREG_SZvalue ), 
									 dwBufferSize ); 

	if( lResult != ERROR_SUCCESS )
		goto Exit;

	if ( REG_SZ != dwType )
	{
		lResult = ERROR_INVALID_DATA; 
		goto Exit; 
	}


	pszConnectString = _tcschr( szREG_SZvalue, TEXT(',') );

	if( !pszConnectString )
	{
		lResult = ERROR_FILE_NOT_FOUND;
		goto Exit;
	}

	// Null terminate Netlib name
	//
	*pszConnectString = 0x00;
	pszConnectString++;

	if( NULL == pCSalias )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

    pCSalias->szConnectionString[0] = 0x00;     // Initialize string to NULL


	// Determine Protocol based on Netlib name found
	//
	if( !_tcsicmp(szREG_SZvalue,TEXT("DBMSLPCN")) )
	{
		TCHAR * pszEnd;

		// Copy protocol name
		//
		(void) StringCchCopy( pCSalias->szProtocol, ARRAYSIZE(pCSalias->szProtocol), TEXT("sm") );   

		// Find next comma and replace with NULL character
		//
		pszEnd = _tcschr( pszConnectString, TEXT(',') );

		if( pszEnd )
        {
			*pszEnd = 0x00;
            pszEnd++;

			lResult = CScopyString( pCSalias->szConnectionString,
									CS_ELEMENTS_OF_ARRAY(pCSalias->szConnectionString), 
									pszEnd ); 

			if( ERROR_SUCCESS != lResult )
				goto Exit; 

        }

		lResult = CScopyString( pCSalias->szTarget,
								CS_ELEMENTS_OF_ARRAY(pCSalias->szTarget), 
								pszConnectString ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit; 

	}
	else if( !_tcsicmp(szREG_SZvalue,TEXT("DBNMPNTW")) )
	{
		TCHAR * pszEnd;

		// Copy protocol name
		//
		(void) StringCchCopy( pCSalias->szProtocol, ARRAYSIZE(pCSalias->szProtocol), TEXT("np") );   

		// Skip over first two backslashes
		//
		if( TEXT('\\') != *pszConnectString )
		{
			lResult = ERROR_INVALID_DATA; 
			goto Exit; 
		}

		pszConnectString++;

		if( TEXT('\\') != *pszConnectString )
		{
			lResult = ERROR_INVALID_DATA; 
			goto Exit; 
		}

		pszConnectString++;
		
		// Find next backslash and replace with NULL character
		//
		pszEnd = _tcschr( pszConnectString, TEXT('\\') );

		if( !pszEnd )
		{
			lResult = ERROR_FILE_NOT_FOUND;
			goto Exit; 
		}

		*pszEnd = 0x00;
        pszEnd++;

		// Find next backslash past PIPE and replace with NULL character
		//
		pszEnd = _tcschr( pszEnd, TEXT('\\') );

		if( !pszEnd )
		{
			lResult = ERROR_FILE_NOT_FOUND;
			goto Exit; 
		}

		*pszEnd = 0x00;
        pszEnd++;

		lResult = CScopyString( pCSalias->szTarget,
								CS_ELEMENTS_OF_ARRAY(pCSalias->szTarget), 
								pszConnectString ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit; 

		lResult = CScopyString( pCSalias->szConnectionString,
								CS_ELEMENTS_OF_ARRAY(pCSalias->szConnectionString), 
								pszEnd ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit; 
		
	}
	else if( !_tcsicmp(szREG_SZvalue,TEXT("DBMSSOCN")) )
	{
		TCHAR * pszEnd;

		// Copy protocol name
		//
		(void) StringCchCopy( pCSalias->szProtocol, ARRAYSIZE(pCSalias->szProtocol), TEXT("tcp") );   

		// Find next comma and replace with NULL character
		//
		pszEnd = _tcschr( pszConnectString, TEXT(',') );

		if( pszEnd )
        {
			*pszEnd = 0x00;
            pszEnd++;

			lResult = CScopyString( pCSalias->szConnectionString,
									CS_ELEMENTS_OF_ARRAY(pCSalias->szConnectionString), 
									pszEnd ); 

			if( ERROR_SUCCESS != lResult )
				goto Exit; 
        }

		lResult = CScopyString( pCSalias->szTarget,
								CS_ELEMENTS_OF_ARRAY(pCSalias->szTarget), 
								pszConnectString ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit; 
		
	}
	else if( !_tcsicmp(szREG_SZvalue,TEXT("DBMSGNET")) )
	{
		// NLregC stores VIA aliases as DBMSGNET,server,port.  
		//
		TCHAR * pszEnd;

		// Copy protocol name
		//
		(void) StringCchCopy( pCSalias->szProtocol, ARRAYSIZE(pCSalias->szProtocol), TEXT("via") );   

		// Find next comma and replace with NULL character
		//
		pszEnd = _tcschr( pszConnectString, TEXT(',') );

		if( pszEnd )
        {
			*pszEnd = 0x00;
            pszEnd++;
			lResult = CScopyString( pCSalias->szConnectionString,
									CS_ELEMENTS_OF_ARRAY(pCSalias->szConnectionString), 
									pszEnd ); 

			if( ERROR_SUCCESS != lResult )
				goto Exit; 
        }

		lResult = CScopyString( pCSalias->szTarget,
								CS_ELEMENTS_OF_ARRAY(pCSalias->szTarget), 
								pszConnectString ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit; 

	}
	else if( !_tcsicmp(szREG_SZvalue,TEXT("DBNETLIB")) &&
		!_tcsnicmp(pszConnectString, TEXT("via:"), 4) )
	{
		// 2.x cliconfg stores VIA aliases as "DBNETLIB,via:server,port".  
		//
		TCHAR * pszEnd;

		// Skip the "via:" prefix if present.  We know it's is there.  
		//
		pszConnectString += 4; 

		// Copy protocol name
		//
		(void) StringCchCopy( pCSalias->szProtocol, ARRAYSIZE(pCSalias->szProtocol), TEXT("via") );   

		// Find next comma and replace with NULL character
		//
		pszEnd = _tcschr( pszConnectString, TEXT(',') );

		if( pszEnd )
        {
			*pszEnd = 0x00;
            pszEnd++;
			lResult = CScopyString( pCSalias->szConnectionString,
									CS_ELEMENTS_OF_ARRAY(pCSalias->szConnectionString), 
									pszEnd ); 

			if( ERROR_SUCCESS != lResult )
				goto Exit; 
        }

		lResult = CScopyString( pCSalias->szTarget,
								CS_ELEMENTS_OF_ARRAY(pCSalias->szTarget), 
								pszConnectString ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit; 

	}
    else    // Unknown protocol
    {
		// Mark protocol as Unknown and copy connection string
		//
		(void) StringCchCopy( pCSalias->szProtocol, ARRAYSIZE(pCSalias->szProtocol), TEXT("Unknown") );   
		pCSalias->szTarget[0] = 0x00;
    }

	// SQL BU DT 298598: 
	// The per-alias "Encrypt" flag is not respected by any client.  
	// Until Komodo removes its dependency on it, always return FALSE.  
	//
    pCSalias->fEncryptionOn = FALSE;

	RegCloseKey( hRegKey );

    hRegKey = NULL;


Exit:
	if( hRegKey )
		RegCloseKey( hRegKey );

	return lResult;
}


///////////////////////////////////////////////////////////
// Add alias.
//
// inputs:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSaddAlias( TCHAR    * szAliasName,
						 CS_ALIAS * pCSalias,
					     BOOL       fOverWrite )
{
	HKEY    hRegKey		 = NULL;
	LONG    lResult		 = 0;
	DWORD   dwDisposition;
	DWORD   dwSize;
	TCHAR   szRegString  [CS_MAX * 3];	//	3 times: szTarget, 
										//	szConnectionString, plus 
										//	extra stuff added by us
										//	(actually, much less than
										//	additional CS_MAX)
										//
	TCHAR   szRegLocation[CS_MAX];
	TCHAR * pszAliases = NULL;

	//	Reject empty alias name.  
	//
	if ( !szAliasName
	|| !szAliasName[0] )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit; 
	}

	// Get Registry settings
	//
	{
		szRegLocation[0] = 0x00;

		TCHAR * pchRegLocationWriteLocation = szRegLocation; 
		DWORD	dwcBufferLeft = sizeof(szRegLocation) / sizeof(szRegLocation[0]) - 1; 

		lResult = CSappendString( &pchRegLocationWriteLocation, 
								  &dwcBufferLeft, 
								  CLIENTREG ); 
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

		lResult = CSappendString( &pchRegLocationWriteLocation, 
								  &dwcBufferLeft, 
								  TEXT("ConnectTo") );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 
	}

	lResult =  RegCreateKeyEx( HKEY_LOCAL_MACHINE,
							   szRegLocation,
							   0,
							   NULL,
							   REG_OPTION_NON_VOLATILE,
							   KEY_WRITE | KEY_QUERY_VALUE,
							   NULL,
							   &hRegKey,
							   &dwDisposition );

	if( lResult != ERROR_SUCCESS )
		goto Exit;

	// See if Value exists
	//
	dwSize = 0;

	lResult = RegQueryValueEx( hRegKey,
						 	   szAliasName,
							   NULL,
							   NULL,
							   NULL,
							   &dwSize );

	if( lResult == ERROR_SUCCESS )
	{
		// Entry exists, should we overwrite?
		//
		if( !fOverWrite )
		{
			lResult = ERROR_FILE_EXISTS;
			goto Exit;
		}
	}

	if( NULL == pCSalias )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	// SQL BU DT bug 299087: 
	// Reject blank server name as MDAC 2.8 Client Network Utility 
	// (cliconfg) used to.  
	//
	if( pCSalias->szTarget[0] == 0x00 )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	// Construct Connection String to store
	//
	if( !_tcsicmp(pCSalias->szProtocol,TEXT("sm")) )
	{
        szRegString[0] = 0x00;

		TCHAR * pchRegStringWriteLocation = szRegString; 
		DWORD	dwcBufferLeft = sizeof(szRegString) / sizeof(szRegString[0]) - 1; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  TEXT("DBMSLPCN,") ); 
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  pCSalias->szTarget );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

		//	SM does not take any protocol-specifuc parameters.  
		//
        if( pCSalias->szConnectionString[0] != 0x00 )
		{
			lResult = ERROR_INVALID_PARAMETER; 
			goto Exit;	// Unknown protocol
		}
	}
	else if( !_tcsicmp(pCSalias->szProtocol,TEXT("np")) )
	{
        szRegString[0] = 0x00;

		TCHAR * pchRegStringWriteLocation = szRegString; 
		DWORD	dwcBufferLeft = sizeof(szRegString) / sizeof(szRegString[0]) - 1; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  TEXT("DBNMPNTW,\\\\") );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

	    lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  pCSalias->szTarget );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  TEXT("\\PIPE\\") );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

        if( pCSalias->szConnectionString[0] != 0x00 )
		{
		    lResult = CSappendString( &pchRegStringWriteLocation, 
									  &dwcBufferLeft, 
									  pCSalias->szConnectionString );
			if ( ERROR_SUCCESS != lResult )
				goto Exit; 
		}
		else
		{
		    lResult = CSappendString( &pchRegStringWriteLocation, 
									  &dwcBufferLeft, 
									  TEXT("SQL\\query") );
			if ( ERROR_SUCCESS != lResult )
				goto Exit; 
		}
	}
	else if( !_tcsicmp(pCSalias->szProtocol,TEXT("tcp")) )
	{
        szRegString[0] = 0x00;

		TCHAR * pchRegStringWriteLocation = szRegString; 
		DWORD	dwcBufferLeft = sizeof(szRegString) / sizeof(szRegString[0]) - 1; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  TEXT("DBMSSOCN,") );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  pCSalias->szTarget );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

        if( pCSalias->szConnectionString[0] != 0x00 )
		{
		    lResult = CSappendString( &pchRegStringWriteLocation, 
									  &dwcBufferLeft, 
									  TEXT(",") );
			if ( ERROR_SUCCESS != lResult )
				goto Exit; 

		    lResult = CSappendString( &pchRegStringWriteLocation, 
									  &dwcBufferLeft, 
									  pCSalias->szConnectionString );
			if ( ERROR_SUCCESS != lResult )
				goto Exit; 
		}
	}
	else if( !_tcsicmp(pCSalias->szProtocol,TEXT("via")) )
	{
        szRegString[0] = 0x00;

		TCHAR * pchRegStringWriteLocation = szRegString; 
		DWORD	dwcBufferLeft = sizeof(szRegString) / sizeof(szRegString[0]) - 1; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  TEXT("DBMSGNET,") );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

		lResult = CSappendString( &pchRegStringWriteLocation, 
								  &dwcBufferLeft, 
								  pCSalias->szTarget );
		if ( ERROR_SUCCESS != lResult )
			goto Exit; 

        if( pCSalias->szConnectionString[0] != 0x00 )
		{
		    lResult = CSappendString( &pchRegStringWriteLocation, 
									  &dwcBufferLeft, 
									  TEXT(",") );
			if ( ERROR_SUCCESS != lResult )
				goto Exit; 

		    lResult = CSappendString( &pchRegStringWriteLocation, 
									  &dwcBufferLeft, 
									  pCSalias->szConnectionString );
			if ( ERROR_SUCCESS != lResult )
				goto Exit; 
		}
	}
	else
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;	// Unknown protocol
	}

	// Set ConnectionString
	//
	lResult = RegSetValueEx( hRegKey,
							 szAliasName,
							 NULL,
							 REG_SZ,
							 (UCHAR *)szRegString,
							 ((DWORD)_tcslen(szRegString) * sizeof(TCHAR))
								+ sizeof(TCHAR) );

	if ( ERROR_SUCCESS != lResult )
		goto Exit; 

	RegCloseKey( hRegKey );

    hRegKey = NULL;


Exit:
	if( hRegKey )
		RegCloseKey( hRegKey );

	return lResult;
}


///////////////////////////////////////////////////////////
// Delete an alias.
//
// inputs:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSdeleteAlias( TCHAR * szAliasName )
{
	HKEY    hRegKey		 = NULL;
	LONG    lResult		 = 0;
	TCHAR   szRegLocation[CS_MAX];
	TCHAR * pszAliases = NULL;

	// Get Registry settings
	//
	(void) StringCchCopy( szRegLocation, ARRAYSIZE(szRegLocation), CLIENTREG );   
	(void) StringCchCat( szRegLocation, ARRAYSIZE(szRegLocation), TEXT("ConnectTo") );   

	lResult =  RegOpenKeyEx( HKEY_LOCAL_MACHINE,
							 szRegLocation,
							 0,
							 KEY_WRITE,
							 &hRegKey );

	if( lResult != ERROR_SUCCESS )
		goto Exit;

	if( NULL == szAliasName )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult =  RegDeleteValue( hRegKey,
							   szAliasName );

	if( lResult != ERROR_SUCCESS )
		goto Exit;

Exit:
	if( hRegKey )
		RegCloseKey( hRegKey );

	return lResult;
}


///////////////////////////////////////////////////////////
// Evaluate whether a alias is supported or not.
//
// inputs:
//		szAliasName[in]: alias name.
//		pfSupported[out]: return whether the alias is supported.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSisSupportedAlias( TCHAR * szAliasName, 
								 BOOL  * pfSupported )
{
	LONG lResult = ERROR_SUCCESS; 
	CS_ALIAS csAlias; 

	lResult = CSgetAlias( szAliasName,
						  &csAlias );

	if( ERROR_SUCCESS != lResult )
		goto Exit; 

	*pfSupported = FALSE; 

	//	Yukon (SNAC) supports only SM, TCP, NP, and VIA.  
	//
	if( !_tcsicmp( csAlias.szProtocol, TEXT("sm") ) ||
		!_tcsicmp( csAlias.szProtocol, TEXT("np") ) ||
		!_tcsicmp( csAlias.szProtocol, TEXT("tcp") ) || 
		!_tcsicmp( csAlias.szProtocol, TEXT("via") ) )
	{
		*pfSupported = TRUE; 
	}

	
Exit: 

	return lResult;
}


///////////////////////////////////////////////////////////
// FUNCTION:
// LONG NLregC::CScreateLastConnectionCache(BOOL fOverwrite)
//
// __in BOOL fOverwrite:
//		When LastConnectionCache exit, set fOverwrite true will overwrite it; otherwise
//		this function will leave the LastConnectionCache intact.
// 
// Error return:
// 	ERROR_SUCCESS The key did not exist and was created. 
//	ERROR_ALREADY_EXISTS The key existed. 
//	Any other is sytem error of RegCreateKeyEx().
//

LONG NLregC::CScreateLastConnectionCache(BOOL fOverwrite)
{
	HKEY  hRegKey		 = NULL;
	LONG  lResult		 = 0;
	DWORD dwDisposition  = 0;

	//test weather SNI root key already exist.
	//
	lResult = CSgetSNIRootRegKey(&hRegKey, KEY_WRITE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);
	CScloseRegKey(hRegKey);

	if(fOverwrite)
	{
		(void)CSdeleteLastConnectionCache();
	}

	lResult = CSgetSNISubRegKey(TEXT("LastConnect"), &hRegKey, KEY_WRITE, CS_CREATE);	
	CHKL_GOTO(lResult, Exit);

Exit:
	CScloseRegKey(hRegKey);
	
	return lResult;
}

///////////////////////////////////////////////////////////
// Delete connection cache.
//
// inputs:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSdeleteLastConnectionCache(void)
{
	HKEY  hRegKey		 = NULL;
	LONG  lResult		 = 0;

	lResult = CSgetSNIRootRegKey(&hRegKey, KEY_WRITE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	// Delete subkey
	//
	lResult =  RegDeleteKey(hRegKey, TEXT("LastConnect"));

Exit:
	
	CScloseRegKey(hRegKey);
	
	return lResult;
}

///////////////////////////////////////////////////////////
// Clear connection cache.
//
// inputs:
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSdeleteAllCachedValues( void )
{
	return CScreateLastConnectionCache(TRUE);
}

///////////////////////////////////////////////////////////
// Retrieve cached connection strings in multi-str form.
//
// inputs:
//		szCacheNameList[out]: buffer to same the connection strings.
//				If it is null, this function is to find the size of buffer
//				required.
//		pdwBufferSize[inout]: in with buffer size, out with resulted string
//				size or buffer size required.
//		pdwMaxValueLen[out]: max one string length. Can be NULL.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetCachedValueList(   __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szCacheNameList,
								   __inout DWORD * pdwBufferSize, 
								   __out_opt DWORD * pdwMaxValueLen )
{
	TCHAR   szRegLocation[CS_MAX];
	LONG lResult = ERROR_SUCCESS;

	lResult = CSgetSNIClientRootPath(szRegLocation, sizeof(szRegLocation), m_curVer);
	CHKL_RETURN(lResult);	
	
	return CSgetCachedValueList(szRegLocation,szCacheNameList, pdwBufferSize, pdwMaxValueLen);
}

///////////////////////////////////////////////////////////
// Retrieve cached connection strings in multi-str form.
//
// inputs:
//		szRootKey[in]: the root path of cache.
//		szCacheNameList[out]: buffer to same the connection strings.
//				If it is null, this function is to find the size of buffer
//				required.
//		pdwBufferSize[inout]: in with buffer size, out with resulted string
//				size or buffer size required.
//		pdwMaxValueLen[out]: max one string length. Can be NULL, if the 
//				the value is not interested.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetCachedValueList( __in __nullnullterminated TCHAR * szRootKey, 
								   __out_bcount_opt(*pdwBufferSize) __nullterminated TCHAR * szCacheNameList,
								   __inout DWORD * pdwBufferSize, 
								   __out_opt DWORD * pdwMaxValueLen )
{
	LONG    lResult		   = 0;
	HKEY    hRegKey = NULL, hRegSubKey =  NULL;

	if( NULL == szRootKey || NULL == pdwBufferSize )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetRegSubKey(NULL,szRootKey, &hRegKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CSgetRegSubKey(hRegKey,TEXT("LastConnect"), &hRegSubKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);
	

	if( !szCacheNameList )	// If we are getting buffer size requirement
	{
		DWORD dwcValues = 0;
		DWORD dwcMaxValueNameLen = 0; 

		lResult = RegQueryInfoKey( hRegSubKey,                
								   NULL,          
								   NULL,      
								   NULL,      
								   NULL,       
								   NULL,  
								   NULL,  
								   &dwcValues,        
								   &dwcMaxValueNameLen,  // not including the null-terminator
								   pdwMaxValueLen,  
								   NULL,
								   NULL ); 

		if( ERROR_SUCCESS != lResult )
			goto Exit;

		// RegQueryInfoKey() returns the size of the key's longest 
		// value name in "characters", not TCHAR's.  An implication 
		// for the ASCII version is that if some of the characters 
		// are multi-byte multiplying the length by sizeof(TCHAR) 
		// (which is 1 in that case) is not sufficient.  
		// We assume no characters are represented by more than 2 
		// bytes, and will multiply the obtained value by 2 to make 
		// room for 2-byte characters.  
		//
		if( sizeof(char) == sizeof(TCHAR) )
		{
			// This is executed only for the ASCII version of NLregC.  
			//
			if( ( ULONG_MAX / CS_ASCII_MAX_BYTES_PER_CHAR ) < dwcMaxValueNameLen )
			{
				lResult = ERROR_BUFFER_OVERFLOW;
				goto Exit;
			}

			dwcMaxValueNameLen *= CS_ASCII_MAX_BYTES_PER_CHAR; 
		}

		// To prevent possible DWORD overflow later on mulitiplication
		if ( dwcMaxValueNameLen+1 > MAX_LEN_CACHEVALUENAME 
			||dwcValues > ( (ULONG_MAX /sizeof(TCHAR))-1) /(dwcMaxValueNameLen+1) )
		{
			lResult = ERROR_BUFFER_OVERFLOW;
			goto Exit;
		}

		if( NULL == pdwBufferSize )
		{
			lResult = ERROR_INVALID_PARAMETER; 
			goto Exit;
		}

		// Add one to each size to alloc space for '\0', and extra one 
		// for double-terminaion at the end.  
		//
		
		*pdwBufferSize = ((( dwcMaxValueNameLen + 1 ) * dwcValues ) + 1 ) * sizeof(TCHAR);
	}
	else	// Let's retrive the actual Alias list
	{
		if( NULL == pdwBufferSize )
		{
			lResult = ERROR_INVALID_PARAMETER; 
			goto Exit;
		}

		DWORD dwTcharSize;
		DWORD dwTcharSizeRemaining = *pdwBufferSize / sizeof(TCHAR);
		DWORD dwIndex = 0;


		*pdwBufferSize = 0;

		do
		{
			dwTcharSize = dwTcharSizeRemaining;

			lResult = RegEnumValue( hRegSubKey,
									dwIndex,			// index of value to query
									szCacheNameList,	// value buffer
									&dwTcharSize,		// size of value buffer
									NULL,				// reserved
									NULL,				// type buffer
									NULL,				// data buffer
									NULL );				// size of data buffer

			if( lResult == ERROR_SUCCESS )
			{
				dwIndex++;

				if ( dwTcharSize >= dwTcharSizeRemaining )
				{
					lResult = ERROR_MORE_DATA; 
					goto Exit; 
				}

				*pdwBufferSize       += (dwTcharSize + 1) * sizeof( TCHAR );
				dwTcharSizeRemaining -= dwTcharSize + 1;
				szCacheNameList		 += dwTcharSize + 1;
			}
		}
		while( lResult == ERROR_SUCCESS );

		if( lResult == ERROR_NO_MORE_ITEMS )
			lResult = ERROR_SUCCESS;

		//	Check if there is space for the double 
		//	NULL-termination.  
		//
		if( 0 >= dwTcharSizeRemaining )
		{
			lResult = ERROR_MORE_DATA; 
			goto Exit;
		}

		// Double NULL terminate
		//
		*szCacheNameList  = 0x00;				
		*pdwBufferSize	 += sizeof( TCHAR );
	}

Exit:
	
	CScloseRegKey(hRegSubKey);
	CScloseRegKey(hRegKey);
		
	return lResult;
}

///////////////////////////////////////////////////////////
// Get a cached connection string.
//
// inputs:
//		szCacheName[in]: the name of the cache entry.
//		szCacheValue[out]: the value of the cache.
//		dwValueBufferSize[in]: the size of the value buffer.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetCachedValue(  TCHAR * szCacheName,
							   __out_bcount(dwValueBufferSize) TCHAR * szCacheValue, 
							   DWORD   dwValueBufferSize )
{
	TCHAR   szRegLocation[CS_MAX];
	LONG lResult = ERROR_SUCCESS;

	lResult = CSgetSNIClientRootPath(szRegLocation, sizeof(szRegLocation), m_curVer);
	CHKL_RETURN(lResult);
	
	return CSgetCachedValue(szRegLocation, szCacheName, szCacheValue, dwValueBufferSize);
}

///////////////////////////////////////////////////////////
// Get a cached connection string.
//
// inputs:
//		szRootKey[in]: the cache root path.
//		szCacheName[in]: the name of the cache entry.
//		szCacheValue[out]: the value of the cache.
//		dwValueBufferSize[in]: the size of the value buffer.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSgetCachedValue( TCHAR * szRootKey, 
							   TCHAR * szCacheName,
							   __out_bcount(dwValueBufferSize) TCHAR * szCacheValue, 
							   DWORD   dwValueBufferSize )
{
	HKEY    hRegKey = NULL, hRegSubKey =  NULL;
	LONG    lResult = 0;

	if( NULL == szRootKey || NULL == szCacheName || NULL == szCacheValue )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetRegSubKey(NULL,szRootKey, &hRegKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CSgetRegSubKey(hRegKey,TEXT("LastConnect"), &hRegSubKey, KEY_QUERY_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CS_VALUE::GetString(hRegSubKey, szCacheName, REG_SZ, szCacheValue, &dwValueBufferSize);

Exit:
	
	CScloseRegKey(hRegKey);
	CScloseRegKey(hRegSubKey);
	
	return lResult;
}


///////////////////////////////////////////////////////////
// Set a cache value.
//
// inputs:
//		szCacheName[in]: the cache entry name.
//		szCacheValue[in]: the cache value.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSsetCachedValue( TCHAR * szCacheName,
							   TCHAR * szCacheValue )
{
	HKEY     hRegSubKey	 = NULL;
	LONG     lResult		 = ERROR_SUCCESS;

	if( NULL == szCacheName || NULL == szCacheValue )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNISubRegKey(TEXT("LastConnect"), &hRegSubKey, KEY_WRITE, CS_CREATE);
	CHKL_GOTO(lResult, Exit);

	lResult = CS_VALUE::SetString(hRegSubKey, szCacheName, REG_SZ, 	szCacheValue,  
								((DWORD)_tcslen(szCacheValue) + 1) * sizeof(TCHAR));

Exit:
	
	CScloseRegKey(hRegSubKey);
	
	return lResult;
}


///////////////////////////////////////////////////////////
// Delete a cache entry.
//
// inputs:
//		szCacheName[in]: cache entry name.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//
LONG NLregC::CSdeleteCachedValue( TCHAR * szCachedName )
{
	HKEY  hRegSubKey = NULL;
	LONG  lResult = ERROR_SUCCESS;

	if( NULL == szCachedName )
	{
		lResult = ERROR_INVALID_PARAMETER; 
		goto Exit;
	}

	lResult = CSgetSNISubRegKey(TEXT("LastConnect"), &hRegSubKey, KEY_WRITE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	lResult = CS_VALUE::DeleteValue(hRegSubKey, szCachedName);
	
Exit:

	CScloseRegKey(hRegSubKey);

	return lResult;
}

// NLregC::CSdeleteChildNodes
//
//	This function deletes all children of the specified key.  
//
//	Inputs:
//		szRegLocation = [in] The key's registry location.  
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure
//
//
LONG NLregC::CSdeleteChildNodes( const TCHAR * szRegLocation )
{
	LONG lResult = ERROR_SUCCESS; 
	HKEY hRegKey= NULL; 
	TCHAR * szChildren = NULL; 

	lResult = CSgetRegSubKey(NULL,szRegLocation,&hRegKey, KEY_ENUMERATE_SUB_KEYS | KEY_SET_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	//	First get the necessary buffer length.  
	//			
	TCHAR szKeyName[CS_MAX];

	DWORD dwBufferSize = 0;

	for ( DWORD dwChildIndex = 0; dwChildIndex<ULONG_MAX ; dwChildIndex++ )
	{
		DWORD dwKeyBufferSize = sizeof( szKeyName ) / sizeof( szKeyName[0] );

		lResult = RegEnumKeyEx( hRegKey,
								dwChildIndex,		// index of subkey to query
								szKeyName,			// subkey name buffer
								&dwKeyBufferSize,	// size of subkey name buffer
								NULL,				// reserved
								NULL,				// class buffer
								NULL,				// size of class buffer
								NULL );				// last write time

		if( ERROR_SUCCESS != lResult )
			break; 

		//	Check for arithmetic overflows.  
		//
		if( dwKeyBufferSize + 1 < dwKeyBufferSize )
		{
			lResult = ERROR_INVALID_DATA; 
			goto Exit; 
		}

		if( dwBufferSize + dwKeyBufferSize + 1 < dwBufferSize )
		{
			lResult = ERROR_INVALID_DATA; 
			goto Exit; 
		}

		dwBufferSize += (dwKeyBufferSize + 1);
	}

	//	Check for arithmetic overflows.  
	//
	if( dwBufferSize + 1 < dwBufferSize )
	{
		lResult = ERROR_INVALID_DATA; 
		goto Exit; 
	}
	
	dwBufferSize += 1;	// double NULL terminator

	if( lResult == ERROR_NO_MORE_ITEMS )
		lResult = ERROR_SUCCESS;
			
	if ( ERROR_SUCCESS != lResult)
		goto Exit; 


	//	Now read all child nodes into a buffer so that we do not
	//	need to enumerate subkeys while deleting them.  
	//					
	szChildren = new TCHAR[dwBufferSize];

	if ( !szChildren )
	{
		lResult = ERROR_OUTOFMEMORY;
		goto Exit; 
	}

	DWORD dwTcharSizeRemaining = dwBufferSize;
	TCHAR * szChild = szChildren; 

	for ( DWORD dwChildIndex = 0; dwChildIndex<ULONG_MAX ; dwChildIndex++ )
	{
		DWORD dwTcharSize = sizeof( szKeyName ) / sizeof( szKeyName[0] );

		lResult = RegEnumKeyEx( hRegKey,
								dwChildIndex,	// index of subkey to query
								szKeyName,		// subkey name buffer
								&dwTcharSize,	// size of subkey name buffer
								NULL,			// reserved
								NULL,			// class buffer
								NULL,			// size of class buffer
								NULL );			// last write time

		if( ERROR_SUCCESS != lResult )
			break; 

		if( 0 == dwTcharSizeRemaining )
		{
			lResult = ERROR_MORE_DATA; 
			goto Exit;
		}

		szChild[dwTcharSizeRemaining - 1] = 0x00; 


		if( FAILED( StringCchPrintf( szChild,
									 dwTcharSizeRemaining,												
									 TEXT("%s"),
									 szKeyName ) ) )

		{
			lResult = ERROR_MORE_DATA; 
			goto Exit;
		}

		SIZE_T cchWritten = _tcslen( szChild );

		if( cchWritten != dwTcharSize )
		{
			lResult = ERROR_INVALID_DATA; 
			goto Exit; 
		}

		dwTcharSizeRemaining -= dwTcharSize + 1;
		szChild        		 += dwTcharSize + 1;
	}

	//	Check if there is space for the double 
	//	NULL-termination.  
	//
	if ( 0 >= dwTcharSizeRemaining )
	{
		lResult = ERROR_MORE_DATA; 
		goto Exit;  
	}

	// Double NULL terminate
	//
	*szChild = 0x00; 

	if ( lResult == ERROR_NO_MORE_ITEMS )
		lResult = ERROR_SUCCESS;

	if ( ERROR_SUCCESS != lResult)
		goto Exit; 


	//	Delete them recursively one by one.  
	//
	for ( TCHAR * szChildNodeToDelete = szChildren; 
		szChildNodeToDelete <= szChild && NULL != szChildNodeToDelete[0]; //prefast does not know szChildNodeToDelete <= szChild is alway true here, adding it to suppress warning 26000 and be more robust
		szChildNodeToDelete += _tcslen( szChildNodeToDelete ) + 1 )
	{
		TCHAR  szLocationChild[CS_MAX]; 

		//	Create the string for the registry location of the child.  
		//
		if( FAILED( StringCchPrintf( szLocationChild, 
									 ARRAYSIZE(szLocationChild),
									 TEXT("%s\\%s"), 
									 szRegLocation, 
									 szChildNodeToDelete ) ) )
		{
			lResult = ERROR_INSUFFICIENT_BUFFER;
			goto Exit;
		}

		//	Ignore result to clean as much as we can.  
		//
		(void)CSdeleteChildNodes( szLocationChild ); 


		//	Delete the child node itself.  
		//	Ignore result to clean as much as we can.  
		//
		(void)RegDeleteKey( hRegKey, 
							szChildNodeToDelete ); 
	}


	if ( ERROR_SUCCESS != lResult)
		goto Exit; 


Exit:

	if ( hRegKey )
		RegCloseKey( hRegKey );

	if ( szChildren )
		delete [] szChildren; 

	return lResult; 
}


// NLregC::CSdeleteHive
//
//	This function recursively deletes the whole client-side SNI hive.  
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure
//
//
LONG NLregC::CSdeleteHive()
{
	LONG lResult = ERROR_SUCCESS; 
	HKEY hRegKey= NULL; 

	TCHAR   szSNIRelativePath[CS_MAX];

	// Get SNI path in form of "SOFTWARE\Microsoft\MSSQLServer\Client\SNImm.n"
	lResult = CSgetSNIClientRootPath(szSNIRelativePath, sizeof(szSNIRelativePath), m_curVer);
	CHKL_RETURN(lResult);

	// Delete children.
	//
	(void)CSdeleteChildNodes(szSNIRelativePath); 

	lResult = CSgetRegSubKey(NULL,CLIENTREG, &hRegKey, KEY_SET_VALUE, CS_OPEN);
	CHKL_GOTO(lResult, Exit);

	// Compose relative path to CLIENTREG.
	//
	if( FAILED (StringCchPrintf(szSNIRelativePath, 
							ARRAYSIZE(szSNIRelativePath),
							SNIROOTKEYPREFIX TEXT("%d.0"),
							m_curVer)))
	{
		lResult = ERROR_BUFFER_OVERFLOW;
		goto Exit;
	}

	// Delete the SNI root node itself.  
	//
	lResult = RegDeleteKey(hRegKey, szSNIRelativePath); 

Exit:

	CScloseRegKey(hRegKey);

	return lResult; 
}


// CSappendString
//
//	This function is a helper function for concatenation of string (typical
//	usage is repetitive invocation to build up a longer string by appending
//	strings at its end).  It writes the given string at the given "append"
//	position if there is sufficient space in the buffer for the string to 
//	be appended INCLUDING its NULL-terminator.  On success, the passed-in 
//	"append" pointer is updated to point to the NULL-terminator of the 
//	newly appended string, and the passed-in buffer size is decremented.  
//	On error, an error code is returned, the character at the specified
//	append location is preserved but the content of the rest of the buffer 
//	is undefined and might be modified.  
//
//	Inputs:
//		ppchAppendLocation = [in/out] The pointer to the pointer holding the 
//						buffer where to copy the string szStringToAppend.  
//						For string concatenation, this should point to the 
//						NULL-terminator character of the first string.  
//
//						On output in case of success, this will point to the 
//						NULL-terminator of the string that was copied into 
//						the buffer (suitable for further concatenation).  
//
//						In case of error, the character at the specified
//						append location is preserved but the content of 
//						the rest of the buffer is undefined and might be 
//						modified.  
//
//		pdwcSpaceBeyondAppendLocation = [in/out] Remaining space in the buffer 
//						counted in TCHARs NOT including the first character 
//						in the buffer.  To successfully append a the string 
//						szStringToAppend the value on input must be at least 
//						_tcslen(szStringToAppend).  
//
//						On output in case of success, the value will be 
//						decremented by _tcslen(szStringToAppend).  
//
//						In case of error, the value is unchanged.  
//		
//		szStringToAppend = [in] The string to copy ("append") into the 
//						specifed buffer location.  		
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure
//
//
LONG NLregC::CSappendString( TCHAR		** ppchAppendLocation,
							 DWORD		 * pdwcSpaceBeyondAppendLocation,
							 const TCHAR * szStringToAppend )
{
	LONG  lResult = ERROR_SUCCESS;

	TCHAR chOrigAtAppendLocation = (*ppchAppendLocation)[0]; 

	(*ppchAppendLocation)[*pdwcSpaceBeyondAppendLocation] = 0x00;

	if ( (*pdwcSpaceBeyondAppendLocation + 1) < *pdwcSpaceBeyondAppendLocation )
	{
		lResult = ERROR_BUFFER_OVERFLOW;
		goto Exit;
	}
	
	if ( FAILED ( StringCchPrintf( *ppchAppendLocation, 
								*pdwcSpaceBeyondAppendLocation+1,								
								TEXT("%s"),
								szStringToAppend ))) 

	{
		(*ppchAppendLocation)[0] = chOrigAtAppendLocation; 

		lResult = ERROR_BUFFER_OVERFLOW;
		goto Exit;
	}

	
	DWORD  cchWritten = static_cast<DWORD>(_tcslen( *ppchAppendLocation ));
	(*pdwcSpaceBeyondAppendLocation) -= cchWritten; 

	(*ppchAppendLocation) += cchWritten; 


Exit:

	return lResult; 

}

// CScopyString
//
//	This function is a helper function for "safe" copying of strings 
//	It copies the given string to the given buffer if there is sufficient 
//	space in the buffer for the string to be copied INCLUDING its 
//	NULL-terminator.  
//	On error, an error code is returned, the destination buffer contains
//	at least one NULL-terminating character but its location or the content 
//	of the rest of the buffer are undefined (the content might be modified).  
//
//	Inputs:
//		szDestBuffer = 	[in/out] The pointer to the destination buffer
//						where to copy the string.  
//
//		dwcDestBufferSize = [in] The size of the destination buffer 
//						counted in TCHARs (this isze must be sufficient
//						for the string INCLUDING the NULL-terminating character 
//						in the buffer.  To successfully copy the string 
//						szStringToCopy the value must be at least 
//						_tcslen(szStringToCopy) + 1.  
//
//		szStringToAppend = [in] The string to copy into the specifed buffer 
//						location.  		
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure
//
//
LONG NLregC::CScopyString( TCHAR	   * szDestBuffer,
						   DWORD		 dwcDestBufferSize,
						   const TCHAR * szStringToCopy )
{
	LONG  lResult = ERROR_SUCCESS;

	TCHAR * pchRegLocationWriteLocation = szDestBuffer; 
	DWORD	dwcBufferLeft = dwcDestBufferSize - 1; 

	szDestBuffer[0] = 0x00;  

	lResult = CSappendString( &pchRegLocationWriteLocation, 
							  &dwcBufferLeft, 
							  szStringToCopy ); 

	if ( ERROR_SUCCESS != lResult )
		goto Exit; 

Exit:

	return lResult; 

}


// CSstringNullTerminate
//
//	A helper function that ensures that a string is NULL-terminated.  
//
//	Inputs:
//		lResult         = [in] Original error code, e.g. obtained from a 
//							   registry API call.  
//
//		szBuffer        = [in/out] A pointer to a buffer containing the 
//								   string of interest.  May be modified
//								   by inserting NULL-termination in case 
//								   of failure.  
//
//		dwBufferSize    = [in] The size of the buffer holding the string
//							   counted in bytes (may be larger than the
//							   string itself).  
//
//		dwWrittenSize   = [in] The number of bytes written when writing
//							   the string including any NULL-terminating
//							   character(s); used only if original lResult 
//							   is success.  
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure or original value of lResult
//
//
LONG NLregC::CSstringNullTerminate( LONG	lResult, 
									__inout_bcount(dwBufferSize) TCHAR * szBuffer,
									DWORD   dwBufferSize, 
									DWORD   dwWrittenSize )
{
	if ( NULL == szBuffer )
	{
		goto Exit; 
	}
			
	//	Verify that the string is NULL-terminated.  
	//
	if ( ERROR_SUCCESS == lResult )
	{
		if ( dwWrittenSize > dwBufferSize )
		{
			//	If the written size really comes from registry API
			//	that was passed the buffer size, this should never happen.  
			//
			lResult = ERROR_BUFFER_OVERFLOW;
		}
		else if ( sizeof(TCHAR) <= dwWrittenSize )
		{
			if ( szBuffer[dwWrittenSize / sizeof(TCHAR) - 1] != 0x00 )
			{
				lResult = ERROR_INVALID_DATA; 

				//	Fall through to NULL-terminate
				//
			}
		}
		else
		{
			lResult = ERROR_INVALID_DATA;

			//	Fall through to NULL-terminate
			//
		}
	}

	if ( ERROR_SUCCESS != lResult )
	{
		//	To be on the safe side even in case of error,
		//	NULL-terminate in case registry API did not.  
		//
		if ( sizeof(TCHAR) <= dwBufferSize )
		{
			szBuffer[dwBufferSize / sizeof(TCHAR) - 1] = 0x00; 
		}

		goto Exit; 
	}


Exit:

	return lResult; 

}


// CSmultiStringNullTerminate
//
//	A helper function that ensures that a multi-string string is 
//	doubly NULL-terminated (empty multi-string is fine with single
//	NULL-termination as well).  
//
//	Inputs:
//		lResult         = [in] Original error code, e.g. obtained from a 
//							   registry API call.  
//
//		szBuffer        = [in/out] A pointer to a buffer containing the 
//								   multi-string of interest.  May be 
//								   modified by inserting NULL-termination 
//								   in case of failure.  
//
//		dwBufferSize    = [in] The size of the buffer holding the 
//							   multi-string counted in bytes (may be 
//							   larger than the multi-string itself).  
//
//		dwWrittenSize   = [in] The number of bytes written when writing
//							   the multi-string including any 
//							   NULL-terminating character(s);used only 
//							   if original lResult is success.  
//
//	Returns:
//		ERROR_SUCCESS if the call succeeds
//		else, error value of failure or original value of lResult
//
//
LONG NLregC::CSmultiStringNullTerminate( LONG	 lResult, 
										 __inout_bcount (dwBufferSize) TCHAR * szBuffer,
										 DWORD   dwBufferSize, 
										 DWORD   dwWrittenSize )
{
	if ( NULL == szBuffer )
	{
		goto Exit; 
	}
			
	//	Verify that the multi-string is properly terminated.  
	//
	if ( ERROR_SUCCESS == lResult )
	{
		if ( dwWrittenSize > dwBufferSize )
		{
			//	If the written size really comes from registry API
			//	that was passed the buffer size, this should never happen.  
			//
			lResult = ERROR_BUFFER_OVERFLOW;
		}
		else if ( sizeof(TCHAR) <= dwWrittenSize )
		{
			if ( szBuffer[dwWrittenSize / sizeof(TCHAR) - 1] != 0x00 )
			{
				lResult = ERROR_INVALID_DATA; 

				//	Fall through to double NULL-terminate
				//
			}
			else if ( 2 * sizeof(TCHAR) <= dwWrittenSize )
			{
				if ( szBuffer[dwWrittenSize / sizeof(TCHAR) - 2] != 0x00 )
				{
					lResult = ERROR_INVALID_DATA; 

					//	Fall through to double NULL-terminate
					//
				}
			}
			//	Single NULL-termination is fine for an empty string
			//
		}
		else
		{
			lResult = ERROR_INVALID_DATA;

			//	Fall through to double NULL-terminate
			//
		}
	}

	if ( ERROR_SUCCESS != lResult )
	{
		//	To be on the safe side even in case of error, double 
		//	NULL-terminate in case registry API did not.  
		//
		if ( sizeof(TCHAR) <= dwBufferSize )
		{
			szBuffer[dwBufferSize / sizeof(TCHAR) - 1] = 0x00; 

			if ( 2 * sizeof(TCHAR) <= dwBufferSize )
			{
				szBuffer[dwBufferSize / sizeof(TCHAR) - 2] = 0x00; 
			}
		}

		goto Exit; 
	}


Exit:

	return lResult; 

}

#pragma region LocalDB_Functions


///////////////////////////////////////////////////////////
// Parse out the version given a version string.
//
// inputs:
//		pszVersion[in]: the version string.
//		pusMajor[out]: the major version parsed out.
//		pusMinor[out]: the minor version parsed out.
//
// returns:
//		ERROR_SUCCESS if no error.
//		otherwise, a winerr code.
//
// notes:
//	1.	version string should be in form of mm.n, mm is the Major, 
//		n is the minor. If version string is not well formed, 
// 		ERROR_INVALID_PARAMETER is returned and 
//		*pusMajor and *pusMinor are undefined.
//
DWORD NLregC::CSgetLocalDBVersion(__in __nullterminated TCHAR* pszVersion, __out USHORT* pusMajor, __out USHORT* pusMinor)
{
	DWORD dwRet = ERROR_SUCCESS;
	TCHAR* pszTmp = pszVersion;
	TCHAR szVersionCopy[CS_MAX];

	Assert(pszVersion);
	Assert(pusMajor);
	Assert(pusMinor);
	
	if( NULL == pszVersion || NULL == pusMajor || NULL == pusMinor)
	{
		// invalid version
		return ERROR_INVALID_PARAMETER;
	}
			
	if(ERROR_SUCCESS != (dwRet = StringCchCopy( szVersionCopy, ARRAYSIZE(szVersionCopy), pszVersion)))
		return dwRet;
		
	pszTmp = _tcschr(szVersionCopy, TEXT('.'));
			
	if( pszTmp )
	{
		*pszTmp = 0;
		dwRet = CSStringToDW(szVersionCopy, pusMajor);
		if(ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}
		pszTmp++;
		dwRet = CSStringToDW(pszTmp, pusMinor);
		if(ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}
	}
	else
	{
		return ERROR_INVALID_PARAMETER;
	}

	return ERROR_SUCCESS;
}


// CSgetUserInstanceDllPath
//
//   This function retrieves the Dll Path of the latest LocalDB instance installed
//
//   Inputs:
//		pszDllPath = Pointer to a buffer that receives the dll path.
//		cchDllPathSize = The size of buffer pszDllPath
//		pErrorState = Pointer to ErrorState
//
//   Returns:
//		ERROR_SUCCESS if the call succeeds along with appropriate pszDllPath
//		else, error value of failure, corresponding error state 
//		and pszDllPath=NULL 
//
//   notes:
//
LONG NLregC::CSgetUserInstanceDllPath(__out_bcount(cchDllPathSize) LPSTR szDllPath,  
											__in DWORD cchDllPathSize,
											__out LocalDBErrorState* pErrorState)
{
	HKEY hKey = NULL;
	HKEY hKeyLatestVersion = NULL;
	LONG dwRet = 0;
	DWORD dwIndex = 0;
	DWORD dwcKeys = 0;
	DWORD dwKeyType;
	DWORD cchDllPathBuf = MAX_PATH;
	CHAR szDllPathBuf[MAX_PATH];	
	TCHAR szInstanceVersion[CS_MAX];
	TCHAR szLatestInstanceVersion[CS_MAX];
	TCHAR szRegLocation[CS_MAX];

	szLatestInstanceVersion[0] = TEXT('\0');
	
	USHORT usMajor = 0;
	USHORT usMinor = 0;
	USHORT usMajorLatest = 0;
	USHORT usMinorLatest = 0;
				
	memcpy(szRegLocation, LOCALDB_INSTALLEDVERSIONS_REG, sizeof(LOCALDB_INSTALLEDVERSIONS_REG));
		
	// Open Regkey SOFTWARE\Microsoft\Microsoft SQL Server Local DB\Installed Versions\
	//
	dwRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
							szRegLocation,
							0,
							KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, 
							&hKey);
	
	if(dwRet != ERROR_SUCCESS)
	{
		*pErrorState = NO_INSTALLATION; 
		goto Exit;	
	}

	// Check if the Reg value is empty
	//		
	dwRet = RegQueryInfoKey( hKey,
							   NULL,
							   NULL,
							   NULL,
							   &dwcKeys,
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL,
							   NULL ); 
						
	if(dwRet != ERROR_SUCCESS)
	{
		goto Exit;	
	}
			
	if (0 == dwcKeys)
	{
		// No registry subkeys found
		//
		dwRet = ERROR_INVALID_DATA;
		*pErrorState = NO_INSTALLATION; 
		goto Exit;	
	}
					
	// Enumerate all the values to find the latest version and use that value to load the key
	//
	for(dwIndex = 0;dwIndex<dwcKeys; dwIndex++)
	{		
		DWORD cchInstanceVersionSize = ARRAYSIZE(szInstanceVersion);
			
		dwRet = RegEnumKeyEx( hKey,
								dwIndex,			// index of value to query
								szInstanceVersion,		// null-terminated subkey name buffer
								&cchInstanceVersionSize, // size of subkey name buffer
								NULL,				// reserved
								NULL,				
								NULL,				
								NULL ); 			
			
		if(dwRet != ERROR_SUCCESS)
		{
			goto Exit;	
		}
					
		if(ARRAYSIZE(szInstanceVersion) <= cchInstanceVersionSize)
		{
			// The subkey name excceeds the max size
			//
			dwRet = ERROR_INVALID_DATA;
			*pErrorState = INVALID_CONFIG;
			goto Exit;			
		}	
			
		if(ERROR_SUCCESS == (dwRet = CSgetLocalDBVersion(szInstanceVersion, &usMajor, &usMinor)))
		{
			// Compare major and minor versions to find the latest
			//
			if(usMajor > usMajorLatest ||(usMajor == usMajorLatest && usMinor > usMinorLatest))
			{
				usMajorLatest = usMajor;
				usMinorLatest = usMinor;
				memcpy(szLatestInstanceVersion, szInstanceVersion, sizeof(szInstanceVersion));
			}				
		}
		else
		{
			//error converting the string to version
			//
			*pErrorState = INVALID_CONFIG;
			goto Exit;
		}
	}

	// If no valid Instance version was found.
	// This is possible if UsMajor = UsMinor = 0
	//
	if(!szLatestInstanceVersion[0])
	{
		dwRet = ERROR_INVALID_DATA;
		*pErrorState = INVALID_CONFIG;
		goto Exit;	
	}
	
	// Open Regkey SOFTWARE\Microsoft\Microsoft SQL Server Local DB\InstalledVersions\
	//			    <latest Version>\
	//
	dwRet = RegOpenKeyEx( hKey, 
							szLatestInstanceVersion,
							0,
							KEY_QUERY_VALUE, 
							&hKeyLatestVersion);
		
	if(dwRet != ERROR_SUCCESS)
	{
		*pErrorState = INVALID_CONFIG;
		goto Exit;	
	}

	// Query RegValue SOFTWARE\Microsoft\Microsoft SQL Server Local DB\InstalledVersions\
	//			    <latest Version>\InstanceAPIPath
	//
	dwRet = RegQueryValueEx( hKeyLatestVersion, 
								TEXT("InstanceAPIPath"),
								NULL,
								&dwKeyType,
								(LPBYTE)szDllPathBuf,
								&cchDllPathBuf);
	if(ERROR_SUCCESS != dwRet)
	{
		*pErrorState = NO_SQLUSERINSTANCEDLL_PATH;
		goto Exit;
	}

	if((cchDllPathBuf < MAX_PATH) && (cchDllPathBuf < cchDllPathSize) && (dwKeyType == REG_SZ))
	{
		szDllPathBuf[cchDllPathBuf] = 0;

		memcpy(szDllPath, szDllPathBuf, cchDllPathBuf);
		szDllPath[cchDllPathBuf] = 0;
	}
	else
	{
		dwRet = ERROR_INVALID_DATA;
		*pErrorState = INVALID_SQLUSERINSTANCEDLL_PATH;
	}
	
Exit: 

	CScloseRegKey(hKey);
	CScloseRegKey(hKeyLatestVersion);

	return dwRet;
}
	
#pragma endregion LocalDB_Functions


