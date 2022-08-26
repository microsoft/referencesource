//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: NLregC.hpp
// @Owner: petergv, nantu
// @test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="true">nantu</owner>
//
// Purpose: Internal Header File for Registry Manipulation 
//			Routines (Client Side)
//
// Notes: Currently the code does not support removal or shrinking of FPVs.
//			If this needs to happen, the corresponding logic 
//			must be implemented.
//          
// @EndHeader@
//****************************************************************************

#ifndef _NLregC_Hpp
#define _NLregC_Hpp

#include "NLregC.h"
#include "shlwapi.h"
#include "stdio.h"
#include "malloc.h"
#include "..\include\sqlhdr.h"

#ifndef SNIX
#include "commonver.h"
#else
#define VER_SQL_MAJOR 10
#endif

#define CS_ELEMENTS_OF_ARRAY(array)	(sizeof(array) / sizeof((array)[0]))

#define VER_PRE90_MAJOR 8		// define the version number used internally for MDACs

#define TCP_PORT		1433
#define TCP_HID_PORT	2433
#define SPX_PORT		33854
#define SPX_HID_PORT	34102
#define SPX_SAPID		0xf503
#define SPX_HID_SAPID	0x9604

#define KEEPALIVETIME		30000
#define KEEPALIVEINTERVAL	1000

#define MAX_CLUST_ADDR	1024
#define MAX_SOCKETS		64

#define CLIENTREG		TEXT("SOFTWARE\\Microsoft\\MSSQLServer\\Client\\")
#define LOCALDB_INSTALLEDVERSIONS_REG 	TEXT("SOFTWARE\\Microsoft\\Microsoft SQL Server Local DB\\Installed Versions\\")

#define SNIROOTKEYPREFIX	TEXT("SNI")
#define SNIROOTKEY_PRE90	TEXT("SuperSocketNetLib")

#define DEFAULT_PROTOCOLS	TEXT("sm\0")  \
							TEXT("tcp\0") \
							TEXT("np\0")

#define DEFAULT_PROTOCOLS_WIN9X	TEXT("tcp\0") \
										TEXT("np\0")

#define DEFAULT_PROTOCOLS_SUPPORTED	TEXT("sm\0")  \
										TEXT("tcp\0") \
										TEXT("np\0")  \
										TEXT("via\0")

#define DEFAULT_PROTOCOLS_SUPPORTED_WIN9X "tcp\0" \
										  "np\0"

#define DEFAULT_NAMEDPIPE 				TEXT("sql\\query")
#define DEFAULT_VIA_PORT 				TEXT("0:1433")
#define DEFAULT_VIA_NIC					TEXT("0")
#define DEFAULT_TCP_PORT				1433
#define DEFAULT_TCP_KEEPALIVE 			30000
#define DEFAULT_TCP_KEEPALIVEINTEVAL 	1000

#define SNACDLLNAME 					TEXT("SQLNCLI")

#define DLLNAME							TEXT("DLLname")
#define NUMOFFLAGS 						TEXT("NumberOfFlags")
#define NUMOFPROPERTIES 				TEXT("NumberOfProperties")
#define PROTOCOLNAME 					TEXT("ProtocolName")
#define TCPPROTOCOLNAME 				TEXT("TCP/IP")
#define NPPROTOCOLNAME					TEXT("Named Pipes")
#define SMPROTOCOLNAME					TEXT("Shared Memory")
#define VIAPROTOCOLNAME 				TEXT("VIA")

//General Values
const TCHAR szProtocolOrder[] =  TEXT("ProtocolOrder");
const TCHAR szProtocolSupported[] = TEXT("ProtocolsSupported");

//Value for every protocols
const TCHAR szNumberOfFlags[] = TEXT("NumberOfFlags");
const TCHAR szNumberOfProperties[] = TEXT("NumberOfProperties");
const TCHAR szDllName[] = TEXT("DLLname");
const TCHAR szProtocolName[] = TEXT("ProtocolName");

//Flags of "Genereal Flag"
const TCHAR szForceProtocolEncryption[] =  TEXT("Force protocol encryption");
const TCHAR szTrustServerCertificiate[] = TEXT("Trust Server Certificate");

//Properties of NP
const TCHAR szDefaultPipe[] = TEXT("Default Pipe");
const TCHAR szDefaultNamedPipe[] = TEXT("sql\\query");

//Properties of TCP
const TCHAR szDefaultPort[] = TEXT("Default Port");
const TCHAR szKeepAlive[] =  TEXT("KEEPALIVE (in milliseconds)");
const TCHAR szKeepAliveInterval[] = TEXT("KEEPALIVEINTERVAL (in milliseconds)");

//Properties of VIA
const TCHAR szDefaultServerPort[] = TEXT("Default Server Port");
const TCHAR szDefaultClientNic[] = TEXT("Default Client NIC");

//MDAC Values
const TCHAR szMDACSm[] = TEXT("SharedMemoryOn");
const TCHAR szMDACEncrypt[] = TEXT("Encrypt");
const TCHAR szMDACProtocolOrder[] = TEXT("ProtocolOrder");
const TCHAR szMDACDefaultPipe[] =  DEFAULT_NAMEDPIPE;
const TCHAR szMDACDefaultPort[] = TEXT("DefaultPort");

//Names of top level keys
const TCHAR szRootPath[] = TEXT("");
const TCHAR szGeneralFlagPath[] = TEXT("GeneralFlags");
const TCHAR szLastConnectionPath[] = TEXT("\\LastConnect");
const TCHAR szNpPath[] = TEXT("np");
const TCHAR szSmPath[] = TEXT("sm");
const TCHAR szTcpPath[] = TEXT("tcp");
const TCHAR szViaPath[] = TEXT("VIA");

#ifdef UNICODE
  #define _itoa _itow
#endif

//Enum of Flag, Property or Value (FPV)
//
enum CSFPVTYPE
{
	eFlag,
	eProperty,
	eValue,
	eFPVMax,
	eFPVUnknown=eFPVMax
};


// MDAC FPV index. The order must be the same as csMDACFPVs.
//
enum MDACFPV
{
	eMDACProtocolOrder,
	eMDACEncrypt,
	eMDACDefaultPort,
	eMDACDefaultPipe,
	eMDACMax,
	eMDACUnknown = eMDACMax
};	

//
// This struct describes a specific entry in SNI registry. 
// It is only used for configuration of default values. CS_FPV
// is a compact version used internally. See CS_FPV.
//
struct CS_FPV_DEFAULT			
{
	//It can either be a flag, a property or a value. 
	//
	CSFPVTYPE 	type;
	
	//Index to MDAC fpv array used during migration.	
	//
	MDACFPV 	mdacNode;

	//Indicate whether this entry can be migrate or not. 
	//
	BOOL 		fmigrate;

	//The relative path to SNI root key. It can be "GeneralFlags",
	//"sm", "tcp", "np", "via".
	//
	const TCHAR* pszRelativePath;

	//Index of eFlag or eProperty. Not used for eValue.
	//
	DWORD 		dwFPIndex;						

	//It is overloaded depending on "type". If type is eFlag, 
	//it is the label of the flag; if type is eProperty,it is the name 
	//of the property; if type is eValue, it is the registry value's 
	//name.  
	//
	const TCHAR* pszName;	
	
	//Length in byte of the value.
	DWORD 		dwcbName;

	//Type of a registry value. It can be REG_DWORD, REG_SZ or 
	//REG_MULTI_SZ.
	DWORD 		dwValueType;

	//Value of a registry value. Can't use union for dwValue & pszValue
	//because it dosn't allow initialization.
	//
	const DWORD dwValue;
	const TCHAR* pszValue;
	
	//Length in byte of the value.
	//
	const DWORD dwcbValue;

	// Always overwrite this value in registry
	// regardless of the flag passed from setup
	//
	const BOOL fOverwriteAlways;
};


//
//FPV array for MDAC registry setting.
//The order should be the same as MDACFPV
//
const CS_FPV_DEFAULT csMDACFPVs[]=		
{
{eValue, eMDACUnknown, FALSE, szRootPath, -1, szMDACProtocolOrder, sizeof(szMDACProtocolOrder),REG_MULTI_SZ, 0,TEXT(""), sizeof(""), FALSE},  //eMDACProtocolOrder
{eValue, eMDACUnknown, FALSE, szRootPath, -1, szMDACEncrypt, sizeof(szMDACEncrypt),REG_DWORD, 0, NULL, sizeof(DWORD), FALSE},		//eMDACEncrypt
{eValue, eMDACUnknown, FALSE, szTcpPath, -1, szMDACDefaultPort, sizeof(szMDACDefaultPort),REG_DWORD, DEFAULT_TCP_PORT,NULL, sizeof(DWORD), FALSE},			//eMDACDefaultPort
{eValue, eMDACUnknown, FALSE, szNpPath, -1, szMDACDefaultPipe, sizeof(szMDACDefaultPipe),REG_SZ, 0, TEXT(""),sizeof(""), FALSE},			//eMDACDefaultPipe
};


//
//FPV array for SNI registry setting.
//
//Note: Currently the code does not support removal or modification of FPVs.
const CS_FPV_DEFAULT csDefaultSNIFPVs[]=
{
{eValue, eMDACUnknown, TRUE, szRootPath, -1, szProtocolSupported, sizeof(szProtocolSupported),REG_MULTI_SZ, 0, DEFAULT_PROTOCOLS_SUPPORTED, sizeof(DEFAULT_PROTOCOLS_SUPPORTED), FALSE},
{eValue, eMDACProtocolOrder, TRUE, szRootPath, -1, szProtocolOrder, sizeof(szProtocolOrder), REG_MULTI_SZ, 0, DEFAULT_PROTOCOLS, sizeof(DEFAULT_PROTOCOLS), FALSE},
{eValue, eMDACUnknown, TRUE,szGeneralFlagPath, -1, szNumberOfFlags, sizeof(szNumberOfFlags),REG_DWORD, 2, NULL, -1, TRUE},
{eFlag, eMDACEncrypt, TRUE, szGeneralFlagPath, 1, szForceProtocolEncryption, sizeof(szForceProtocolEncryption),REG_DWORD, 0, NULL, -1, FALSE},
{eFlag, eMDACUnknown, TRUE, szGeneralFlagPath, 2, szTrustServerCertificiate, sizeof(szTrustServerCertificiate),REG_DWORD, 0, NULL, -1, FALSE},
{eValue, eMDACUnknown, FALSE, szNpPath, -1, szDllName, sizeof(szDllName), REG_SZ,0,SNACDLLNAME, sizeof(SNACDLLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szNpPath, -1,szProtocolName,sizeof(szProtocolName), REG_SZ,0,NPPROTOCOLNAME, sizeof(NPPROTOCOLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szNpPath, -1,szNumberOfFlags,sizeof(szNumberOfFlags),REG_DWORD,0,NULL,-1, TRUE},
{eValue, eMDACUnknown, TRUE, szNpPath, -1,szNumberOfProperties,sizeof(szNumberOfProperties),REG_DWORD,1,NULL,-1, TRUE},
{eProperty, eMDACDefaultPipe, TRUE, szNpPath, 1, szDefaultPipe,sizeof(szDefaultPipe), REG_SZ,0,DEFAULT_NAMEDPIPE, sizeof(DEFAULT_NAMEDPIPE), FALSE},
{eValue, eMDACUnknown, FALSE, szSmPath, -1,szDllName,sizeof(szDllName),REG_SZ,0,SNACDLLNAME,sizeof(SNACDLLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szSmPath, -1,szProtocolName,sizeof(szProtocolName),REG_SZ,0,SMPROTOCOLNAME, sizeof(SMPROTOCOLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szSmPath, -1,szNumberOfFlags,sizeof(szNumberOfFlags),REG_DWORD,0,NULL,-1, TRUE},
{eValue, eMDACUnknown, TRUE, szSmPath, -1,szNumberOfProperties,sizeof(szNumberOfProperties),REG_DWORD,0, NULL,-1, TRUE},
{eValue, eMDACUnknown, FALSE, szTcpPath, -1,szDllName,sizeof(szDllName),REG_SZ,0,SNACDLLNAME,sizeof(SNACDLLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szTcpPath, -1,szProtocolName,sizeof(szProtocolName),REG_SZ,0,TCPPROTOCOLNAME,sizeof(TCPPROTOCOLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szTcpPath, -1,szNumberOfFlags,sizeof(szNumberOfFlags),REG_DWORD,0,NULL,-1, TRUE},
{eValue, eMDACUnknown, TRUE, szTcpPath, -1,szNumberOfProperties,sizeof(szNumberOfProperties),REG_DWORD,3,NULL,-1, TRUE},
{eProperty, eMDACDefaultPort, TRUE, szTcpPath, 1, szDefaultPort, sizeof(szDefaultPort), REG_DWORD,DEFAULT_TCP_PORT,NULL,-1, FALSE},
{eProperty, eMDACUnknown, TRUE, szTcpPath, 2, szKeepAlive, sizeof(szKeepAlive),REG_DWORD,DEFAULT_TCP_KEEPALIVE,NULL, -1, FALSE},
{eProperty, eMDACUnknown, TRUE, szTcpPath, 3, szKeepAliveInterval, sizeof(szKeepAliveInterval), REG_DWORD, DEFAULT_TCP_KEEPALIVEINTEVAL,NULL,-1, FALSE},
{eValue, eMDACUnknown, FALSE, szViaPath, -1,szDllName,sizeof(szDllName),REG_SZ,0,SNACDLLNAME,sizeof(SNACDLLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szViaPath, -1,szProtocolName,sizeof(szProtocolName),REG_SZ,0,VIAPROTOCOLNAME,sizeof(VIAPROTOCOLNAME), TRUE},
{eValue, eMDACUnknown, TRUE, szViaPath, -1,szNumberOfFlags,sizeof(szNumberOfFlags),REG_DWORD,0,NULL,-1, TRUE},
{eValue, eMDACUnknown, TRUE, szViaPath, -1,szNumberOfProperties,sizeof(szNumberOfProperties),REG_DWORD,2,NULL,-1, TRUE},
{eProperty, eMDACUnknown, TRUE, szViaPath, 1, szDefaultServerPort, sizeof(szDefaultServerPort), REG_SZ,0,DEFAULT_VIA_PORT, sizeof(DEFAULT_VIA_PORT), FALSE},
{eProperty, eMDACUnknown, TRUE, szViaPath, 2, szDefaultClientNic,sizeof(szDefaultClientNic), REG_SZ,0,DEFAULT_VIA_NIC,sizeof(DEFAULT_VIA_NIC), FALSE}
};


//
// This struct describes a specific entry in SNI registry. 
//
class CS_FPV
{
	friend class NLregC;
	friend class CS_VALUE;

	//It can either be a flag, a property or a value. 
	//
	CSFPVTYPE 	type;	

	//The relative path to SNI root key. It can be "GeneralFlags",
	//"sm", "tcp", "np", "via".
	//
	const TCHAR* pszRelativePath;			

	//Index of eFlag or eProperty. Not used for eValue.
	//
	DWORD 		dwFPIndex;						

	//It is overloaded depending on "type". If type is eFlag, 
	//it is the label of the flag; if type is eProperty,it is the name 
	//of the property; if type is eValue, it is the registry value's 
	//name.  
	//
	TCHAR 		szName[CS_MAX];

	//Type of a registry value. It can be REG_DWORD, REG_SZ or 
	//REG_MULTI_SZ.
	//
	DWORD 		dwValueType;
	
	//Value of a registry value.
	//
	union
	{
		BYTE bValue[CS_MAX*sizeof(TCHAR)];
		DWORD dwValue;
		TCHAR szValue[CS_MAX];
	};
	
	CS_FPV(CSFPVTYPE t, const TCHAR* rp, DWORD fpindex,  DWORD vt )
			:type(t), pszRelativePath(rp), dwFPIndex(fpindex), dwValueType(vt)
	{};

	//Initialize based on a CS_FPV_DEFAULT. Do deep copy if needed.
	//
	LONG Copy(const CS_FPV_DEFAULT* csfpv);

	private:
		
	// declare as private to prevent use of the default ctor.	
	//
	CS_FPV(){}; 
};


//
//This class describe a registry entry (name, value)
//
class CS_VALUE
{
	private:

	//The name of a registry entry.
	//
	const TCHAR* 	m_pszName;

	//The value type of a registry entry.	
	//It can be REG_DWORD, REG_SZ, REG_MULTI_SZ.
	//
	DWORD 			m_dwValueType;

	//The value length of a registry entry.
	//
	DWORD 			m_dwcbValue;

	//The value of a registry entry.
	//
	union
	{
		const BYTE* 		m_pbValue;
		const DWORD*   	m_pdwValue;
		const TCHAR*		m_pszValue;
	};		

	//Ctor for REG_DWORD value.
	//
	CS_VALUE(const TCHAR* pszn, const DWORD* dwv){InitDw(pszn, dwv);};	

	//Ctor for REG_SZ or REG_MULTI_SZ value.
	//
	CS_VALUE(const TCHAR* pszn, DWORD dwt, BYTE* pszv, DWORD dwcbv)
	{
		InitBytes(pszn, dwt, pszv, dwcbv);
	};

	//Ctor based on CS_FPV
	//
	CS_VALUE(const CS_FPV* pcsFPV);

	//Intenally used initialization functions.
	//
	void InitDw(const TCHAR* pszn, const DWORD* dwv);
	void InitBytes(const TCHAR* pszn, DWORD dwt, const BYTE* pszv, DWORD dwcbv);

	//Set or retrieve the entry from registry.
	//
	LONG SetValue(HKEY hk);
	LONG GetValue(HKEY hk);

	public:

	//Set or retrieve a REG_DWORD entry from registry.
	//
	inline static LONG SetDw(HKEY hk, const TCHAR* pszn, const DWORD dwv){ CS_VALUE csv(pszn, &dwv);return csv.SetValue(hk);}
	inline static LONG GetDw(HKEY hk, const TCHAR* pszn, DWORD* pdwv){CS_VALUE csv(pszn, pdwv);return csv.GetValue(hk);}

	//Set or retrieve a REG_SZ or REG_MULTI_SZ entry from registry.
	//
	inline static LONG SetString(HKEY hk, const TCHAR* pszn, DWORD dwt, const TCHAR* pszv, DWORD dwcbv)
	{
		CS_VALUE csv(pszn, dwt, (BYTE*)(pszv), dwcbv);
		return csv.SetValue(hk);
	}	
	inline static LONG GetString(HKEY hk, const TCHAR* pszn, DWORD dwt, TCHAR* pszv, DWORD* pdwcbv)
	{
		CS_VALUE csv(pszn, dwt, (BYTE*)pszv, *pdwcbv);
		LONG lResult = csv.GetValue(hk);
		*pdwcbv = csv.m_dwcbValue;
		return lResult;
	}

	//Set or retrieve any entry from registry.
	//
	inline static LONG GetBytes(HKEY hk, const TCHAR* pszn, DWORD* dwt, TCHAR* pszv, DWORD* pdwcbv)
	{
		CS_VALUE csv(pszn, *dwt, (BYTE*)pszv, *pdwcbv);
		LONG lResult = csv.GetValue(hk);
		*pdwcbv = csv.m_dwcbValue;
		*dwt = csv.m_dwValueType;
		
		return lResult;
	}
	inline static LONG SetValueWithFPV(HKEY hk, const CS_FPV* pfpv)
	{
		if( eValue != pfpv->type )
			return ERROR_INVALID_PARAMETER;

		CS_VALUE csv(pfpv);
		return csv.SetValue(hk);
	}

	//
	//Set or retrieve a entry described by CS_FPV from registry. 
	//The type of the CS_FPV must be eValue.
	//
	inline static LONG GetValueWithFPV(HKEY hk, CS_FPV* pfpv)
	{
		if( eValue != pfpv->type )
			return ERROR_INVALID_PARAMETER;

		CS_VALUE csv(pfpv);
		return csv.GetValue(hk);
	}

	//Delete the registry entry.
	//
	inline static LONG DeleteValue(HKEY hk, TCHAR* pszName)
	{
		return RegDeleteValue(hk, pszName);
	}
};


class NLregC
{
	friend class CS_VALUE;
	
	//utility functions
	//
	LONG CSStringToDW(const TCHAR* pszStr, 
						USHORT* pdwOut);

	LONG CSgetVersion(__in_bcount(dwcbPath) TCHAR* pszPath, 
						__in DWORD dwcbPath, 
						__out USHORT* pusMajor);

	LONG CSgetSNIClientRootPath(TCHAR* pszRootPath, 
								DWORD dwcbRootPath, 
								USHORT usMajor);

	LONG CSgetLatestSNIClientRootPathAndVersion(TCHAR* pszRootPath, 
												DWORD dwcbRootPath, 
												USHORT* pdwMajor );

	LONG CSisEmptyRegSubKey(HKEY hRootRegKey,
									TCHAR* pszPath, 
									bool *pfIsEmpty);

	LONG CSgetRegSubKey(const HKEY hRootKey,
						const TCHAR* pszPath, 
						HKEY* phSubKey, 
						REGSAM samDesired, 
						BOOL fCreate);

	LONG CSgetSNIRootRegKey(HKEY* phKey, 
							REGSAM samDesired, 
							BOOL fCreate);

	LONG CSgetSNISubRegKey(const TCHAR* pszPath, 
							HKEY* phSubKey, 
							REGSAM samDesired,
							BOOL fCreate);

	void CScloseRegKey(HKEY hKey);

	//Migration functions
	//
	LONG CSmigrateMDACProtocolOrder(TCHAR* pszSNIPO,
									DWORD dwcbSNIPO, 
									TCHAR* pszMdacPO);

	LONG CSdefaultOrmigrateMDACFPV(__in const DWORD dwIndex, 
									__in const HKEY hMdacRoot,
									__in const HKEY hSNIRoot);

	LONG CSdefaultOrmigrateSNIFPV(const DWORD dwIndex, 
									const HKEY hSNIRootNew, 
									const HKEY hSNIRootOld,
									BOOL fOverWrite);

	//Flag, Property, value functions
	//
	LONG CSgetProtocolDwValue(const TCHAR* pszProtocol,
								const TCHAR* pszName,
								DWORD* pdwValue);

	LONG CSgetProcolStringValue(const TCHAR* pszProtocol, 
								const TCHAR* pszName, 
								TCHAR* pszValue,
								DWORD dwcbValue);

	LONG CSgetFlagOrProperty(__in HKEY hRootKey, 
							__in CSFPVTYPE eType,
							__in DWORD dwIndex,
							__out_bcount_opt(dwcbLabel) __nullterminated TCHAR * pszLabel, 
							__in DWORD dwcbLabel, 
							__in DWORD* pdwType,
							__out_bcount(*pdwcbValue) BYTE * pbValue, 
							__inout DWORD* pdwcbValue);

	LONG CSsetFlagOrProperty(HKEY hRootKey,
							CSFPVTYPE eType,
							DWORD dwIndex, 
							const TCHAR * pszLabel,
							DWORD dwType,
							const BYTE* pbValue,
							DWORD dwcbValue, 
							BOOL fCreate);

	LONG CSgetFPV(HKEY hKey,
					CS_FPV* p_csfpv);

	LONG CSsetFPV(HKEY hKey, 
					const CS_FPV* p_csfpv,
					BOOL fCreate);

	// LocalDB Functions
	//
	DWORD CSgetLocalDBVersion(__in __nullterminated TCHAR* pszVersion,
								__out USHORT* pusMajor,
								__out USHORT* pusMinor);

	//Current major version of SNI.
	//
	USHORT m_curVer;

	NLregC(){};

	LONG CSgetCachedValueList( __in __nullterminated TCHAR  * szRootKey, 
								__out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szCacheNameList,
								__inout DWORD * pdwBufferSize, 
								__out_opt DWORD * pdwMaxValueLen );

	LONG CSgetCachedValue( TCHAR * szRootKey, 
						   TCHAR * szCacheName,
						   __out_bcount(dwValueBufferSize) TCHAR * szCacheValue, 
						   DWORD   dwValueBufferSize );
	
	
public:

	LONG CSsetDefaults( BOOL fOverWriteAll );

	LONG CSgetNumberOfGeneralFlags( DWORD * pdwNumberOfFlags );

	LONG CSgetGeneralFlagProperty( __in DWORD   dwFlagIndex,
								   __out_bcount(dwcbLabel) __nullterminated TCHAR   szLabel[CS_MAX],
								   __in DWORD dwcbLabel,
								   __out DWORD * pdwFlagState );

	LONG CSsetGeneralFlagProperty( DWORD dwFlagIndex,
								   DWORD dwFlagState );

	LONG CSgetProtocolsSupported( __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szProtocolsSupported,
								  __inout DWORD * pdwBufferSize );

	LONG CSgetProtocolOrder(  __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szProtocolOrder,
							 __inout DWORD * pdwBufferSize );

	LONG CSsetProtocolOrder( __in const TCHAR * pszProtocolOrder );

	LONG CSgetNumberOfProtocolFlags( TCHAR * szProtocol,
									 DWORD * pdwNumberOfFlags );

	LONG CSgetNumberOfProtocolProperties( TCHAR	* szProtocol,
										  DWORD * pdwNumberOfProperties );

	LONG CSgetProtocolInfo( TCHAR            * szProtocol,
							CS_PROTOCOL_INFO * pPropertyInfo );

	LONG CSgetProtocolFlag( __in __nullterminated TCHAR * szProtocol,
							__in DWORD   dwPropertyIndex,
							__out_bcount_opt(dwcbFlagLabel) __nullterminated TCHAR * szFlagLabel,
							__in_range(0,CS_MAX * sizeof (TCHAR)) DWORD dwcbFlagLabel,
							__out DWORD * pdwFlagValue );
	
	LONG CSsetProtocolFlag( TCHAR * szProtocol,
							DWORD   dwFlagIndex,
							DWORD   dwFlagValue );

	LONG CSgetProtocolProperty( __in __nullterminated TCHAR				 * szProtocol,
								__in DWORD				   dwPropertyIndex,
						        __out CS_PROTOCOL_PROPERTY * pProtocolProperty );

	LONG CSsetProtocolProperty( __in __nullterminated TCHAR				 * szProtocol,
								__in DWORD				   dwPropertyIndex,
								__in CS_PROTOCOL_PROPERTY * pPropertyProperty );

	LONG CSgetDBLIBinfo( __out CS_DBLIBINFO * infoDBLIB );

	LONG CSsetDBLIBinfo( __in CS_DBLIBINFO * infoDBLIB );

	LONG CSgetNETLIBinfo( __in __nullterminated TCHAR		    * szProtocolName,
						  __out CS_NETLIBINFO * infoNETLIB );

	LONG CSgetAliases( __out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * pszAliasesPresent,
						   __inout DWORD * pdwBufferSize );
	LONG CSgetAlias( TCHAR    * szAliasName,
					 CS_ALIAS * pCSalias );

	LONG CSaddAlias( TCHAR	  * szAliasName,
					 CS_ALIAS * pCSalias,
					 BOOL       fOverWrite );

	LONG CSdeleteAlias( TCHAR * szAliasName );


	LONG CSdeleteAllCachedValues( void );
	
	LONG CSdeleteLastConnectionCache(void);

	LONG CScreateLastConnectionCache(BOOL fOverwrite); 

	LONG CSgetCachedValueList(__out_bcount_opt(*pdwBufferSize) __nullnullterminated TCHAR * szCacheNameList,
							__inout DWORD * pdwBufferSize, 
							__out_opt DWORD * pdwMaxValueLen );

	LONG CSgetCachedValue(TCHAR * szCacheName,
							__out_bcount(dwValueBufferSize) TCHAR * szCacheValue, 
							DWORD   dwValueBufferSize );


	LONG CSsetCachedValue( TCHAR * szCacheName,
						   TCHAR * szCacheValue );


	LONG CSdeleteCachedValue( TCHAR * szCachedName );

	LONG CSdeleteHive(); 

	//LocalDB_Functions
	//
	LONG CSgetUserInstanceDllPath(__out_bcount(cchDllPathSize) LPSTR szDllPath, 
										__in DWORD cchDllPathSize,
										__out LocalDBErrorState* pErrorState);
	
	NLregC(USHORT usMajor);

	~NLregC();

private:

	LONG CSgetProtocolsSupportedOrOrder( __in __nullterminated TCHAR * pszRegistryItem,
										__out_bcount_opt(*pdwBufferSize) __nullterminated TCHAR * pszProtocolsSupportedOrOrder,
										__in DWORD * pdwBufferSize );

	LONG GetDateSizeVersion( TCHAR * szFileName,
							 DWORD * dwDate,
							 DWORD * dwSize,
							 DWORD * dwProductVersionMS,
							 DWORD * dwProductVersionLS,
							 BOOL    fDBLIB ); 

	LONG CSisSupportedAlias( TCHAR * szAliasName, 
							 BOOL  * pfSupported ); 

	LONG CSdeleteChildNodes( const TCHAR * szRegLocation ); 

	LONG CSappendString( TCHAR		** ppchAppendLocation,
						 DWORD		 * pdwcSpaceBeyondAppendLocation,
						 const TCHAR * szStringToAppend ); 

	LONG CScopyString( TCHAR	   * szDestBuffer,
					   DWORD		 dwcDestBufferSize,
					   const TCHAR * szStringToCopy ); 
	
	static LONG CSstringNullTerminate( LONG	lResult, 
								__inout_bcount(dwBufferSize) TCHAR * szBuffer,
								DWORD   dwBufferSize, 
								DWORD   dwWrittenSize );
	
	static LONG CSmultiStringNullTerminate( LONG	 lResult, 
									 __inout_bcount(dwBufferSize) TCHAR * szBuffer,
									 DWORD   dwBufferSize, 
									 DWORD   dwWrittenSize ); 
};

#endif // #ifndef _NLregC_Hpp
