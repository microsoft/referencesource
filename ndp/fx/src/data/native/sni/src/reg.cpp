//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: open.hpp
// @Owner: nantu, petergv
// @Test: milu
//
// <owner current="true" primary="true">nantu</owner>
// <owner current="true" primary="false">petergv</owner>
//
// Purpose: Connection string parsing
//
// Notes: Everything in this file is shared with dbnetlib.dll ( dbnetlib has a separate copy of this file ).
//        So, special attention is needed whenever anything in this file needs to be changed.
//
//          
// @EndHeader@
//****************************************************************************

#include "snipch.hpp"
#include "ssrp.hpp"
#include "NLregC.h"

#define MAX_CACHEENTRY_LENGTH ( 11 + 1                         \
							    + MAX_PROTOCOLNAME_LENGTH + 1  \
							    + MAX_NAME_SIZE + 1            \
                                + 11 )		

#define DEFAULT_PROTOCOLS	TEXT("sm\0")  \
							TEXT("tcp\0") \
							TEXT("np\0")

#define DEFAULT_PROTOCOLS_SANDBOX	TEXT("np\0")

namespace LastConnectCache 
{

// Linked List impl. for LastConnectCache
//
class Cache;

class CacheItem
{
	friend class Cache;

    LPWSTR m_wszValName;  // Name of the cache value
    LPWSTR m_wszValue;    // Cache value content
    CacheItem * m_pNext;       // Pointer to next element in list

public:

	CacheItem():
		m_pNext(0),
		m_wszValName(0),
		m_wszValue(0)
	{
	}
	~CacheItem()
	{
		delete m_wszValName;	//szValue is inside this also
	}

	BOOL SetValue( const WCHAR *wszValName, const WCHAR *wszValue)
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszValName: \"%s\", wszValue: \"%s\"\n"), 
						wszValName, wszValue);
	
		int cchValName=(int)wcslen(wszValName) + 1;
		int cchVal=(int)wcslen(wszValue) + 1;

   		m_wszValName = NewNoX(gpmo) WCHAR[ cchValName + cchVal];
		if( !m_wszValName )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
			return FALSE;
		}
		
		m_wszValue = m_wszValName + cchValName;
		(void)StringCchCopyW(m_wszValName,cchValName,wszValName);
		(void)StringCchCopyW(m_wszValue,cchVal,wszValue);

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);

		return TRUE;
	}
	
	inline BOOL CopyValue(__out_ecount(cchDest) WCHAR *wszDest, DWORD cchDest)
	{
		return SUCCEEDED(StringCchCopyW(wszDest, cchDest, m_wszValue));
	}
};


class Cache{

	CacheItem *pHead;
public:
	Cache():
		pHead(0)
	{
	}
	
	~Cache()
	{
		Cleanup();
	};

	void Cleanup()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n") );
		
		CacheItem *pNext=pHead;
		while(pHead){
			pNext = pHead->m_pNext;
			delete pHead;
			pHead=pNext;
		}
	}
	BOOL Insert( const WCHAR *wszValName, const WCHAR *wszValue)
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszValName: \"%s\", wszValue: \"%s\"\n"), 
						wszValName, wszValue);
		
		if(MAX_CACHEENTRY_LENGTH<wcslen(wszValue))
		{			
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
			return FALSE;
		}
	
	    CacheItem * pNewItem = NewNoX(gpmo) CacheItem();

		if( !pNewItem )
		{
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
			return FALSE;
		}

	    	if( !pNewItem->SetValue(wszValName, wszValue ))
	    	{
	    		delete pNewItem;
			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
	    		return FALSE;
	    	}

		pNewItem->m_pNext = pHead;			//it points to old first link
		pHead = pNewItem;			//now first points to this

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);
		return TRUE;
	}

	// Find item in the cache
	CacheItem * Find( const WCHAR * wszValName)   
	{                           
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "wszValName: \"%s\"\n"), wszValName);

		CacheItem *pCurrent;

		for ( pCurrent = pHead; pCurrent; pCurrent = pCurrent->m_pNext )
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			if ( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNORECASE|NORM_IGNOREWIDTH,
									 pCurrent->m_wszValName, -1,
									 wszValName, -1))
OACR_WARNING_POP
				break;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p{CacheItem *}\n"), pCurrent);
		
		return pCurrent;
	}

	// Remove item from the cache
	BOOL Remove(const WCHAR * wszValName)           
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "wszValName: \"%s\"\n"), wszValName);
		
		CacheItem ** ppCurrentItem = &pHead;

		for ( ; *ppCurrentItem ; ppCurrentItem = &(*ppCurrentItem)->m_pNext)
		{

OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			if ( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNORECASE|NORM_IGNOREWIDTH,
									 (*ppCurrentItem)->m_wszValName, -1,
									 wszValName, -1))
OACR_WARNING_POP
			{
				CacheItem * pDeleteItem = *ppCurrentItem;

				*ppCurrentItem = (*ppCurrentItem)->m_pNext;
				delete pDeleteItem;
				BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);
				return TRUE;
			}
		}

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);
		
		return FALSE;
	}
	
};

Cache *pgLastConnectCache;         // the LastConnectCache

SNICritSec 	* critsecCache = 0;

#ifndef SNIX
//	For SNIX version, see below
//
void Initialize()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	Assert( !pgLastConnectCache );

	LPWSTR wszValueNameList = 0;
	LPWSTR wszValue = 0;

	// Initialize in memory Cache
	pgLastConnectCache = NewNoX(gpmo) Cache();

	if( !pgLastConnectCache )
	{
		goto ErrorExit;
	}
	
	if( ERROR_SUCCESS != SNICritSec::Initialize(&critsecCache))
	{
		goto ErrorExit;
	}

	// Initialize persistent cache in registry.
	// First, create LastConnectionCache if it doesnt exist yet.
	// In case of failure, we go ahead silently.
	CScreateLastConnectionCache(FALSE);
	
	//Second, Read in cached values from the registry. 
	DWORD dwcbValueNameListLen = 0; 
	DWORD dwcbMaxValueLen = 0;

	if ( ERROR_SUCCESS != CSgetCachedValueList( NULL,
											    &dwcbValueNameListLen, 
												&dwcbMaxValueLen ) )
	{
		goto ErrorExit1;
	}

	 // Bump up the size to alloc space for '\0'
	if (FAILED (DWordAdd(dwcbMaxValueLen, 1, &dwcbMaxValueLen)))
		goto ErrorExit1;

	wszValueNameList = NewNoX(gpmo) WCHAR[ dwcbValueNameListLen ];
	if( NULL == wszValueNameList )
	{
		goto ErrorExit1;
	}

	if ( ERROR_SUCCESS != CSgetCachedValueList( wszValueNameList,
												&dwcbValueNameListLen, 
												NULL ) )
	{
		goto ErrorExit1;
	}

	wszValue     = NewNoX(gpmo) WCHAR [ dwcbMaxValueLen ];
	if( NULL == wszValue )
	{
		goto ErrorExit1;
	}

	for ( WCHAR * wszValueName = wszValueNameList; 
		0 != *wszValueName; 
		wszValueName += wcslen( wszValueName ) + 1 )
	{
		if ( ERROR_SUCCESS == CSgetCachedValue( wszValueName,
												wszValue, 
												dwcbMaxValueLen ) )
        {
            // There is a valid registry entry
            // So, insert into the LastConnectCache
            if( !pgLastConnectCache->Insert( wszValueName, wszValue) )
            {
				goto ErrorExit1;
            }
        }       
    }

ErrorExit1:
	
	delete [] wszValueNameList;
	delete [] wszValue;

	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("success\n"));

	return;

ErrorExit:

	DeleteCriticalSection(&critsecCache);
	if(pgLastConnectCache)
		delete pgLastConnectCache;

	pgLastConnectCache = 0;
		
	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("fail\n"));

	return;
}
#endif	//	#ifndef SNIX

void Shutdown()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	if( !pgLastConnectCache )
	{
		return;
	}

	DeleteCriticalSection(&critsecCache);

	delete pgLastConnectCache;

	pgLastConnectCache = 0;
	
	return;
}

#ifndef SNIX
//	For SNIX version, see below
//
void RemoveEntry( const WCHAR *wszAlias)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "wszAlias: '%s'\n"), wszAlias);

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	if( !pgLastConnectCache )
	{
		return;
	}
	
	CAutoSNICritSec a_csCache( critsecCache, SNI_AUTOCS_DO_NOT_ENTER );

	a_csCache.Enter();

    if( pgLastConnectCache->Remove( wszAlias) )
    {
		LONG ret;
		
	    // Remove from registry
		ret = CSdeleteCachedValue( const_cast<WCHAR *>(wszAlias) );

		if( ERROR_SUCCESS != ret )
    	{
			BidTrace1(ERROR_TAG _T("registry: %d{WINERR}\n"), ret);
	    }
	}

	a_csCache.Leave(); 

	return;
}
#endif	//	#ifndef SNIX

BOOL GetEntry( const WCHAR *wszAlias, __out ProtElem *pProtElem)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszAlias: '%s', pProtElem: %p\n"), wszAlias, pProtElem);

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	if( !pgLastConnectCache )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);

		return FALSE;
	}
	

	CAutoSNICritSec a_csCache( critsecCache, SNI_AUTOCS_DO_NOT_ENTER );

	a_csCache.Enter();

    WCHAR wszCacheInfo[MAX_CACHEENTRY_LENGTH+1];
    CacheItem * pItem;

    // Look for item in the cache
    if( (pItem = pgLastConnectCache->Find(wszAlias)) == NULL)
    {
		a_csCache.Leave(); 
        goto ErrorExit;
    }
    else    
    {
        BOOL fSuccess = pItem->CopyValue(wszCacheInfo, 
			sizeof(wszCacheInfo)/sizeof(wszCacheInfo[0]));  
		
		a_csCache.Leave(); 

		if( !fSuccess )
		{
			goto ErrorExit;
		}
    }

    // We may have a blank value, check for that
    if( !wszCacheInfo[0] )
        goto ErrorExit;

	WCHAR *pwPtr;
	WCHAR *pwEnd;

	pwPtr=wszCacheInfo;

    // This is a check to ensure we do not break clients
    // which have older entries in their LastConnect cache
    if( pwPtr[0] < L'A' )     // Okay, its a version number
    {
        // Get version info
		pwEnd=wcschr(pwPtr,L':');
		if(!pwEnd)
			goto ErrorExit;
        
        *pwEnd = 0;
        pwPtr = pwEnd+1;

    }
    // Otherwise return ERROR_FAIL - this will force it to be
    // set in the cache the next time
    else
    {
        goto ErrorExit;
    }

	WCHAR *pwszProtName = 0;
	
	if(pwEnd=wcschr(pwPtr,L':')){
		*pwEnd=0;
		if(wcslen(pwPtr)>MAX_PROTOCOLNAME_LENGTH)
			goto ErrorExit;

		pwszProtName = pwPtr;
		
		pwPtr = pwEnd+1;
	}
	else
		goto ErrorExit;

    switch(	 pwszProtName[0] )
    {
        case L'n':   // Named Pipe

            pProtElem->SetProviderNum(NP_PROV);
            if( FAILED( StringCchCopyW(
				pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), pwPtr ) ) )
            {
				goto ErrorExit;
            }

            break;

        case L'v':   // VIA

            pProtElem->SetProviderNum(VIA_PROV);
            if( FAILED( StringCchCopyW(
				pProtElem->Via.Host, ARRAYSIZE(pProtElem->Via.Host), pwPtr ) ) )
            {
				goto ErrorExit;
            }

            break;

        case L't':   // Tcp

			pProtElem->SetProviderNum(TCP_PROV);
			if(pwEnd=wcschr(pwPtr,L','))
			{
				if( ARRAYSIZE(pProtElem->Tcp.wszPort) <= wcslen(pwEnd+1))
					goto ErrorExit;

				if(Wcstoi(pwEnd+1)==0)
					goto ErrorExit;

				// We verified the destination size above, so ignore 
				// the return value.  
				(void)StringCchCopyW( pProtElem->Tcp.wszPort, ARRAYSIZE(pProtElem->Tcp.wszPort), pwEnd+1);
			}
			else
				goto ErrorExit;

            break;

        case L'l':   // Lpc

            pProtElem->SetProviderNum(SM_PROV);
            if( FAILED( StringCchCopyW( pProtElem->Sm.Alias, ARRAYSIZE(pProtElem->Sm.Alias), pwPtr ) ) )
            {
				goto ErrorExit;
            }

            break;
    
        default:
            goto ErrorExit;
    }

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), TRUE);

	return TRUE;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{BOOL}\n"), FALSE);

    return FALSE;
}

#ifndef SNIX
//	For SNIX version, see below
//
void SetEntry( const WCHAR *wszAlias, __in ProtElem *pProtElem)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszAlias: '%s', pProtElem: %p\n"), wszAlias, pProtElem);

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	if( !pgLastConnectCache )
	{
		return;
	}

	Assert( wcschr(wszAlias,L'\\'));
	
    WCHAR wszCacheVal[MAX_CACHEENTRY_LENGTH+1];

    // Enter critical section
	CAutoSNICritSec a_csCache( critsecCache, SNI_AUTOCS_DO_NOT_ENTER );

	a_csCache.Enter();

	Assert(pProtElem);
	
    switch(pProtElem->GetProviderNum())
    {
        case TCP_PROV:   // Tcp
            if( FAILED( StringCchPrintfW(
				wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:tcp:%s,%s", pProtElem->m_wszServerName, pProtElem->Tcp.wszPort ) ) )
            {
				goto ErrorExit;
            }
            break;

        case NP_PROV:   // Named Pipe
            if( FAILED( StringCchPrintfW(
				wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:np:%s", pProtElem->Np.Pipe ) ) )
            {
				goto ErrorExit;
            }
            break;

        case SM_PROV:   //Lpc
			///does shared memory use cache?
            if( FAILED( StringCchPrintfW(
				wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:lpc:%s", pProtElem->Sm.Alias ) ) )
            {
				goto ErrorExit;
            }
            break;

        case VIA_PROV:   // Via
        	/// this is not useful information to cache
            if( FAILED( StringCchPrintfW(wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:via:%s", pProtElem->Via.Host ) ) )
            {
				goto ErrorExit;
            }
            break;

        default:
			// this assertion is used to catch improper function usages.
			Assert( 0 && "providerNum: invalid value\n" );
			BidTrace0(ERROR_TAG _T("providerNum: invalid value\n"));
			goto ErrorExit;

    }

    unsigned int chCacheVal;

    chCacheVal=(unsigned int)wcslen(wszCacheVal);

    Assert(chCacheVal<=MAX_CACHEENTRY_LENGTH);

    if(MAX_CACHEENTRY_LENGTH<chCacheVal)
		goto ErrorExit;
    
	pgLastConnectCache->Remove(wszAlias);

    if( !pgLastConnectCache->Insert( wszAlias, wszCacheVal) )
    {
    	goto ErrorExit;
    }

    // Set in registry
    DWORD dwValueLen;

    dwValueLen = (DWORD)(wcslen(wszCacheVal) * sizeof(WCHAR));


	if( ERROR_SUCCESS != CSsetCachedValue( const_cast<WCHAR *>(wszAlias), 
										   wszCacheVal ) )											
    {
        goto ErrorExit;
    }

	BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("success\n"));
	
ErrorExit:

	a_csCache.Leave(); 

    return;

}
#endif	//	#ifndef SNIX


#ifdef SNIX
//	SNIX version, which does not use or manipulate registry
//
void Initialize()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	Assert( !pgLastConnectCache );

    // Initialize Cache
    pgLastConnectCache = NewNoX(gpmo) Cache();

	if( !pgLastConnectCache )
	{
		goto ErrorExit;
	}
	
	if( ERROR_SUCCESS != SNICritSec::Initialize(&critsecCache))
	{
		goto ErrorExit;
	}
	
	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("success\n"));

	return;

ErrorExit:

	DeleteCriticalSection(&critsecCache);

	delete pgLastConnectCache;

	pgLastConnectCache = 0;
		
	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("fail\n"));

	return;
}

//	SNIX version, which does not use or manipulate registry
//
void RemoveEntry( const WCHAR *wszAlias)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "wszAlias: '%s'\n"), wszAlias);

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	if( !pgLastConnectCache )
	{
		return;
	}
	
	CAutoSNICritSec a_csCache( critsecCache, SNI_AUTOCS_DO_NOT_ENTER );

	a_csCache.Enter();

    pgLastConnectCache->Remove( wszAlias); 

	a_csCache.Leave(); 

	return;
}

//	SNIX version, which does not use or manipulate registry
//
void SetEntry( const WCHAR *wszAlias, ProtElem *pProtElem)
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszAlias: '%s', pProtElem: %p\n"), wszAlias, pProtElem);

	BidTraceU1( SNI_BID_TRACE_ON, SNI_TAG _T("pgLastConnectCache: %p\n"), pgLastConnectCache);

	if( !pgLastConnectCache )
	{
		return;
	}

	Assert( wcschr(wszAlias,L'\\'));
	
    WCHAR wszCacheVal[MAX_CACHEENTRY_LENGTH+1];

    // Enter critical section
	CAutoSNICritSec a_csCache( critsecCache, SNI_AUTOCS_DO_NOT_ENTER );

	a_csCache.Enter();

	Assert(pProtElem);
	
    switch(pProtElem->GetProviderNum())
    {
        case TCP_PROV:   // Tcp
            if( FAILED( StringCchPrintfW(
				wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:tcp:%s,%s", pProtElem->m_wszServerName, pProtElem->Tcp.wszPort ) ) )
            {
				goto ErrorExit;
            }
            break;

        case NP_PROV:   // Named Pipe
            if( FAILED( StringCchPrintfW(
				wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:np:%s", pProtElem->Np.Pipe ) ) )
            {
				goto ErrorExit;
            }
            break;

        case SM_PROV:   //Lpc
			///does shared memory use cache?
            if( FAILED( StringCchPrintfW(
				wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:lpc:%s", pProtElem->Sm.Alias ) ) )
            {
				goto ErrorExit;
            }
            break;

        case VIA_PROV:   // Via
        	/// this is not useful information to cache
            if( FAILED( StringCchPrintfW(wszCacheVal, ARRAYSIZE(wszCacheVal), L"0:via:%s", pProtElem->Via.Host ) ) )
            {
				goto ErrorExit;
            }
            break;

        default:
			// this assertion is used to catch improper function usages.
			Assert( 0 && "providerNum: invalid value\n" );
			BidTrace0(ERROR_TAG _T("providerNum: invalid value\n"));
			goto ErrorExit;

    }

    unsigned int chCacheVal;

    chCacheVal=(unsigned int)wcslen(wszCacheVal);

    Assert(chCacheVal<=MAX_CACHEENTRY_LENGTH);

    if(MAX_CACHEENTRY_LENGTH<chCacheVal)
		goto ErrorExit;
    
	pgLastConnectCache->Remove(wszAlias);

    if( !pgLastConnectCache->Insert( wszAlias, wszCacheVal) )
    {
    	goto ErrorExit;
    }

	BidTraceU0( SNI_BID_TRACE_ON, SNI_TAG _T("success\n"));
	
ErrorExit:

	a_csCache.Leave(); 

    return;

}
#endif	//	#ifdef SNIX


} // namespace LastConnectCache 


DWORD GetProtocolEnum( const WCHAR    * pwszProtocol,
						   __out ProviderNum * pProtNum )
{
	BidxScopeAutoSNI2( SNIAPI_TAG _T( "pwszProtocol: '%s', pProtNum: %p\n"), pwszProtocol, pProtNum);

	if( !_wcsicmp(L"TCP", pwszProtocol) )
	{
		*pProtNum = TCP_PROV;
	}
	else if( !_wcsicmp(L"NP", pwszProtocol) )
	{
		*pProtNum = NP_PROV;
	}
	else if( !_wcsicmp(L"SM", pwszProtocol) )
	{
		*pProtNum = SM_PROV;
	}
	else if( !_wcsicmp(L"VIA", pwszProtocol) )
	{
		*pProtNum = VIA_PROV;
	}
	else
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

		return ERROR_FAIL;
	}
	
	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;
}

#ifndef SNIX
//	For SNIX version, see below
//
DWORD GetProtocolDefaults( 	__out ProtElem * pProtElem,
									const WCHAR * pwszProtocol,
									const WCHAR * wszServer )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pProtElem: %p, pwszProtocol: '%s', wszServer: '%s'\n"), pProtElem, pwszProtocol, wszServer);

	LONG       lResult;
	WCHAR     * wszFoundChar = NULL;
	ProviderNum    eProviderNum;
	DWORD  nlReturn = ERROR_FAIL;
	LPWSTR wszClusEnv = NULL;
	bool  fSetDefaultValues = false;

	CS_PROTOCOL_PROPERTY ProtocolProperty;

	nlReturn = GetProtocolEnum( pwszProtocol,
							    &eProviderNum );

	if( nlReturn != ERROR_SUCCESS )
		goto ErrorExit;

	pProtElem->SetProviderNum(eProviderNum);

	//
	// some of default parameters below only apply to default server instance
	// for example tcp port is one of them
	// In cases where default parameter doesn't make sense we skip processing
	//
	switch( eProviderNum )
	{
		case TCP_PROV:
				
			if( g_fSandbox )
			{
				fSetDefaultValues = true;
			}
			else		
			{	
				// Get the protocol Properties
				//
				lResult = CSgetProtocolProperty( (WCHAR *)pwszProtocol,
												 CS_PROP_TCP_DEFAULT_PORT,
												 &ProtocolProperty );

				if( (ERROR_SUCCESS == lResult) && 
					(REG_DWORD == ProtocolProperty.dwPropertyType) )
				{
					(void)StringCchPrintfW(pProtElem->Tcp.wszPort,ARRAYSIZE(pProtElem->Tcp.wszPort),L"%d",ProtocolProperty.PropertyValue.dwDoubleWordValue);
				}
				else	// We had a problem so just use known default
				{
					fSetDefaultValues = true;
				}
			}

			if( fSetDefaultValues )
			{
				(void)StringCchCopyW(pProtElem->Tcp.wszPort,ARRAYSIZE(pProtElem->Tcp.wszPort),L"1433");
			}

			break;

		case NP_PROV:	
			// _dupenv_s() returns non-zero for failure.  
			//
			if( _wdupenv_s(&wszClusEnv, NULL, L"_CLUSTER_NETWORK_NAME_" ) )
			{
				goto ErrorExit; 
			}
			
			(void)StringCchCopyW( pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"\\\\" );

			// If string passed in matches Computername AND
			// the cluster env var is NOT set, then put "." in pipe name
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
			if( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNORECASE|NORM_IGNOREWIDTH,
									 wszServer, -1,
									 gwszComputerName, -1)&&
				((NULL == wszClusEnv) || !_wcsicmp(wszClusEnv, L""))
			  )
OACR_WARNING_POP
			{
				(void)StringCchCatW( pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"." );
			}

			// In all other cases leave as is
			else
			{
				if( FAILED( StringCchCatW( 
					pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), wszServer ) ) )
				{
					goto ErrorExit; 
				}
			}

			if( FAILED( StringCchCatW( 
				pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"\\PIPE\\" ) ) )
			{
				goto ErrorExit; 
			}
			
			if( g_fSandbox )
			{
				// If registry access is set to false, set the default values.
				fSetDefaultValues = true;
			}
			else
			{
				// Get the protocol Properties
				//
				lResult = CSgetProtocolProperty( (WCHAR *)pwszProtocol,
                                                 CS_PROP_NP_DEFAULT_PIPE,
                                                 &ProtocolProperty );

				if( (ERROR_SUCCESS == lResult) &&
					(REG_SZ == ProtocolProperty.dwPropertyType) )
				{
					if( FAILED( StringCchCatW( pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), 
						    ProtocolProperty.PropertyValue.szStringValue ) ) )
					{
						goto ErrorExit; 
					}					    
				}
				else	// We had a problem so just use known default
				{
					fSetDefaultValues = true;	    
				}
			}
		
			if( fSetDefaultValues )
			{
				if( FAILED( StringCchCatW( pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"sql\\query" ) ) )
				{
					goto ErrorExit; 
				}		
			}

			break;

		case SM_PROV:
			if( FAILED( StringCchCopyW( 
				pProtElem->Sm.Alias, ARRAYSIZE(pProtElem->Sm.Alias), wszServer ) ) )
			{
				goto ErrorExit; 
			}				

			break;

		case VIA_PROV:	// Can be Giganet or Servernet

			// Get the protocol Properties
			//

			//	Hard-code the vendor name to "QLogic".  
			//	In Yukon/Whidbey (SNAC/SNIX) the only supported 
			//	vendor is QLogic.  
			//
			(void)StringCchCopyW( pProtElem->Via.Vendor, ARRAYSIZE(pProtElem->Via.Vendor), CS_VALUE_VIA_VENDOR_NAME_QLOGIC );
			
			// Copy the server name into the host
			if( FAILED( StringCchCopyW( 
				pProtElem->Via.Host, ARRAYSIZE(pProtElem->Via.Host), wszServer ) ) )
			{
				goto ErrorExit; 
			}					    				

			if( g_fSandbox )
			{
				fSetDefaultValues = true;
			}
			else
			{
				lResult = CSgetProtocolProperty( (WCHAR *)pszProtocol,
												 CS_PROP_VIA_DEFAULT_PORT,
												 &ProtocolProperty );

				if( (ERROR_SUCCESS == lResult) && 
					(REG_SZ == ProtocolProperty.dwPropertyType) &&
					ProtocolProperty.PropertyValue.szStringValue[0] &&
					wcschr(ProtocolProperty.PropertyValue.szStringValue, L':') )
				{
					LPWSTR szTmp = wcschr(ProtocolProperty.PropertyValue.szStringValue, L':');
					int iPort = _wtoi(szTmp+1);

					if( (0 <= iPort) && (USHRT_MAX >= iPort) )
					{
						pProtElem->Via.Port = static_cast<USHORT>(iPort); 
					}
					else
					{
						fSetDefaultValues = true;
					}
				}
				else
				{
					fSetDefaultValues = true;
				}
			}

			if( fSetDefaultValues )
			{
				pProtElem->Via.Port = 1433;

				// XXX - this is currently an empty string
				pProtElem->Via.Param[0] = 0;
			}			

			break;

		default:
			// this assertion is used to catch improper function usages.
			Assert( 0 && "providerNum: invalid value\n" );
			BidTrace0(ERROR_TAG _T("providerNum: invalid value\n"));
			goto ErrorExit;
	}

	if( NULL != wszClusEnv )
	{
		free(wszClusEnv); 
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	if( NULL != wszClusEnv )
	{
		free(wszClusEnv); 
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;
}


//	For SNIX versions, see below
//
DWORD GetProtocolList( 	__inout ProtList * pProtList, 
							const WCHAR * wszServer,
							const WCHAR * wszOriginalServer )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pProtList: %p, wszServer: '%s', wszOriginalServer: '%s'\n"), pProtList, wszServer, wszOriginalServer);

	LONG       lResult      = 0;
	DWORD      dwBufferSize = 0;
	TCHAR     * pszProtocolOrder = 0;
	TCHAR		* pszProt;
	ProtElem * newProtElem;
	DWORD  nlReturn;
	DWORD dwRead = 0;
	DWORD dwLen = 0;
	HRESULT hr;

	if( g_fSandbox )
	{
		dwBufferSize = sizeof ( DEFAULT_PROTOCOLS_SANDBOX );
		pszProt = DEFAULT_PROTOCOLS_SANDBOX;
	}
	else
	{
		lResult = CSgetProtocolOrder( NULL, 
			                          &dwBufferSize );

		//	If the protocol order list was not found or it's 
		//	empty, use a default list.  cliconfg sets an empty 
		//	list to a single NULL-terminating character, 
		//	Yukon's WMI-based snap-in based on NLregC sets it 
		//	to two NULL-terminating characters.  
		//
		if( (lResult != ERROR_SUCCESS) || (dwBufferSize < 3 * sizeof(TCHAR)) )
		{
			pszProt = DEFAULT_PROTOCOLS; 
			lResult = ERROR_SUCCESS; 
		}
		else
		{
			//	dwBufferSize is in bytes.  Divide it by the character 
			//	size rounding it up.  
			//
			pszProt = pszProtocolOrder = 
				NewNoX(gpmo) TCHAR[ (dwBufferSize + sizeof(TCHAR) - 1) / sizeof(TCHAR) ];

			if( pszProtocolOrder == NULL )
			{
				pszProt = DEFAULT_PROTOCOLS; 
				lResult = ERROR_SUCCESS; 
			}
			else
			{
				lResult = CSgetProtocolOrder( pszProtocolOrder,
					                          &dwBufferSize );

				if( lResult != ERROR_SUCCESS )
				{
					pszProt = DEFAULT_PROTOCOLS; 
					lResult = ERROR_SUCCESS; 
				}
			}
		}
	} // else of if( g_fSandbox )

	// Go through each protocol and add to our protocol list
	// Checking that we don't read beyond the data is ONLY for PREfix.  This check is
	// guaranteed NOT to fire because we add the terminators to the string ourselves.
	// Apply the length check, then check for terminator.
	for( ; dwRead < dwBufferSize && *pszProt != 0x00; )
	{
		// If the protocol is shared memory and its NOT a 
		// local host, then just continue
		if( !_wcsicmp(pszProt, L"sm") && !IsLocalHost(wszServer) )
		{
			// Do nothing.
		}
		else
		{
			// Do not insert the same protocol multiple times
			ProviderNum eProviderNum;

			nlReturn = GetProtocolEnum( pszProt,
										&eProviderNum );

			// Skip unknown protocols.  
			if( nlReturn == ERROR_SUCCESS && NULL == pProtList->Find(eProviderNum) )
			{
				newProtElem = NewNoX(gpmo) ProtElem();

				if( !newProtElem )
					goto ErrorExit;

				if( ERROR_SUCCESS != newProtElem->Init( wszServer, wszOriginalServer ) )
				{
					delete newProtElem;
					goto ErrorExit;
				}

				nlReturn = GetProtocolDefaults( newProtElem,
												   pszProt,
												    wszServer );

				if( nlReturn == ERROR_FAIL )
				{
					delete newProtElem;
					goto ErrorExit;
				}
					
				pProtList->AddTail( newProtElem );
			}
		}
		// move to next protocol in the null-null-terminated list
		dwLen = _tcslen( pszProt ) + 1;
		pszProt += dwLen;
		// dwBufferSize is a byte-count, so multiply by sizeof TCHAR
		hr = DWordMult(dwLen,sizeof(TCHAR),&dwLen);
		if (FAILED(hr))
		{
			BidTraceU1( SNI_BID_TRACE_ON, ERROR_TAG _T("dwLen: %d{DWORD}, "), dwLen);
			goto ErrorExit;
		}
		hr = DWordAdd(dwRead, dwLen, &dwRead);
		if (FAILED(hr))
		{
			BidTraceU2( SNI_BID_TRACE_ON, ERROR_TAG _T("dwLen: %d{DWORD}, dwRead: %d{DWORD}"), dwLen, dwRead);
			goto ErrorExit;
		}

	}	// for(;;)

	delete [] pszProtocolOrder;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	delete [] pszProtocolOrder;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;

}

#else	//	#ifndef SNIX

/////////////////////////// SNIX ///////////////////////////
//
//	Below are SNIX versions of functions which use or manipulate 
//	registry entries.  The SNIX versions do not use or manipulate 
//	registry, and use hard-coded defaults if needed.  Function 
//	which do not access registry are common, in the upper parts 
//	of the file.    
//
////////////////////////////////////////////////////////////


//	SNIX uses hard-coded values for protocol order instead of 
//	reading registry via NLreg libraries.  The non-Win9x default
//	is used also by MDAC in case the registry list is not 
//	present, or if it is empty.  
//
#define DEFAULT_PROTOCOLS_WIN9X	TEXT("tcp\0") \
								TEXT("np\0")

extern BOOL gfIsWin9x;


//	SNIX version, which does not use or manipulate registry
//
DWORD GetProtocolDefaults( 	ProtElem * pProtElem,
									const WCHAR * pwszProtocol,
									const WCHAR * wszServer )
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pProtElem: %p, pwszProtocol: '%s', wszServer: '%s'\n"), pProtElem, pwszProtocol, wszServer);

	LONG       lResult;
	WCHAR     * wszFoundChar = NULL;
	ProviderNum    eProviderNum;
	DWORD  nlReturn = ERROR_FAIL;
	LPWSTR wszClusEnv = NULL;

	CS_PROTOCOL_PROPERTY ProtocolProperty;

	nlReturn = GetProtocolEnum( pwszProtocol,
							    &eProviderNum );

	if( nlReturn != ERROR_SUCCESS )
		goto ErrorExit;

	pProtElem->SetProviderNum(eProviderNum);

	//
	// some of default parameters below only apply to default server instance
	// for example tcp port is one of them
	// In cases where default parameter doesn't make sense we skip processing
	//

	switch( eProviderNum )
	{
		case TCP_PROV:
			(void)StringCchCopyW(pProtElem->Tcp.wszPort,ARRAYSIZE(pProtElem->Tcp.wszPort),L"1433");
			break;

		case NP_PROV:
			// _dupenv_s() returns non-zero for failure.  
			//
			if( _wdupenv_s(&wszClusEnv, NULL, L"_CLUSTER_NETWORK_NAME_" ) )
			{
				goto ErrorExit; 
			}
			
			(void)StringCchCopyW( pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"\\\\" );

			// If string passed in matches Computername AND
			// the cluster env var is NOT set, then put "." in pipe name
			if( CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNORECASE|NORM_IGNOREWIDTH,
									 wszServer, -1,
									 gwszComputerName, -1) &&
				((NULL == wszClusEnv) || !_wcsicmp(wszClusEnv, L""))
			  )
			{
				(void)StringCchCatW( pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"." );
			}

			// In all other cases leave as is
			else
			{
				if( FAILED( StringCchCatW( 
					pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), wszServer ) ))
				{
					goto ErrorExit; 
				}
			}

			if( FAILED( StringCchCatW( 
				pProtElem->Np.Pipe, ARRAYSIZE(pProtElem->Np.Pipe), L"\\PIPE\\sql\\query" ) ) )
			{
				goto ErrorExit; 
			}
			break;

		case SM_PROV:
			if( FAILED( StringCchCopyW( 
				pProtElem->Sm.Alias, ARRAYSIZE(pProtElem->Sm.Alias), wszServer ) ) )
			{
				goto ErrorExit; 
			}					    				
			break;

		case VIA_PROV:	// Can be Giganet or Servernet

			// 






			(void)StringCchCopyW( pProtElem->Via.Vendor, ARRAYSIZE(pProtElem->Via.Vendor), CS_VALUE_VIA_VENDOR_NAME_QLOGIC );
			
			// Copy the server name into the host
			if( FAILED( StringCchCopyW( 
				pProtElem->Via.Host, ARRAYSIZE(pProtElem->Via.Host), wszServer ) ) )
			{
				goto ErrorExit; 
			}					    				

			lResult = CSgetProtocolProperty( (WCHAR *)pwszProtocol,
										 CS_PROP_VIA_DEFAULT_PORT,
										 &ProtocolProperty );

			if( (ERROR_SUCCESS == lResult) && 
				(REG_SZ == ProtocolProperty.dwPropertyType) &&
				ProtocolProperty.PropertyValue.szStringValue[0] &&
				wcschr(ProtocolProperty.PropertyValue.szStringValue, L':') )
			{
				LPWSTR wszTmp = wcschr(ProtocolProperty.PropertyValue.szStringValue, L':');
				int iPort = _wtoi(wszTmp+1);
				
				if( (0 <= iPort) && (USHRT_MAX >= iPort) )
				{
					pProtElem->Via.Port = static_cast<USHORT>(iPort); 
				}
				else
				{
					pProtElem->Via.Port = 1433;
				}
			}
			else
				pProtElem->Via.Port = 1433;

			break;
		default:
			// this assertion is used to catch improper function usages.
			Assert( 0 && "providerNum: invalid value\n" );
			BidTrace0(ERROR_TAG _T("providerNum: invalid value\n"));
			goto ErrorExit;
	}

	if( NULL != wszClusEnv )
	{
		free(wszClusEnv); 
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	if( NULL != wszClusEnv )
	{
		free(wszClusEnv); 
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;
}

//	SNIX version, which does not use or manipulate registry
//
DWORD GetProtocolList( 	ProtList * pProtList, 
							const WCHAR * wszServer,
							const WCHAR * wszOriginalServer )
{
	return GetProtocolList(pProtList, wszServer, wszOriginalServer, NULL);
}

DWORD GetProtocolList( 	ProtList * pProtList, 
							const WCHAR * wszServer,
							const WCHAR * wszOriginalServer,
							TCHAR * pszProt)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "pProtList: %p, wszServer: '%s', wszOriginalServer: '%s'\n"), pProtList, wszServer, wszOriginalServer);

	LONG       lResult      = 0;
	ProtElem * newProtElem;
	DWORD  nlReturn;

	if(pszProt == NULL)
	{
		if( !gfIsWin9x )
			pszProt = DEFAULT_PROTOCOLS; 
		else
			pszProt = DEFAULT_PROTOCOLS_WIN9X; 		
	}

	// Go through each protocol and add to our protocol list
	//
	for( ;*pszProt != 0x00;							// are we at end of protocol list?
		  pszProt += _tcslen( pszProt ) + 1 )	// position to next protocol
	{
		// If the protocol is shared memory and its NOT a 
		// local host, then just continue
		if( !_wcsicmp(pszProt, L"sm") && !IsLocalHost(wszServer) )
			continue;
		
		newProtElem = NewNoX(gpmo) ProtElem();

		if( !newProtElem )
			goto ErrorExit;

		if( ERROR_SUCCESS != newProtElem->Init( wszServer, wszOriginalServer ) )
		{
			delete newProtElem;
			goto ErrorExit;
		}

		nlReturn = GetProtocolDefaults( newProtElem,
										   pszProt,
										    wszServer );

		if( nlReturn == ERROR_FAIL )
		{
			delete newProtElem;
			goto ErrorExit;
		}
			
		pProtList->AddTail( newProtElem );

	}	// for(;;)

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);

	return ERROR_SUCCESS;

ErrorExit:

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_FAIL);

	return ERROR_FAIL;

}
#endif	//	#ifdef SNIX

// Local DB Functions

//	Gets the dll path of the latest sqlUserInstance.dll installed
//	from the registry.
//
DWORD GetUserInstanceDllPath(LPSTR szDllPath, DWORD cchDllPathSize, DWORD* pErrorState)
{
	return CSgetUserInstanceDllPath(szDllPath, cchDllPathSize,(LocalDBErrorState*)pErrorState);
}



