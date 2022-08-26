//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: sni_common.hpp
// @Owner:  nantu | petergv
// @Test: milu
//
// <owner current="true" primary="false">nantu</owner>
// <owner current="true" primary="true">petergv</owner>
//
// Purpose: 
//		File that abstracts the host specific details for SNI. 
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#ifndef _SNI_COMMON_HPP_
#define _SNI_COMMON_HPP_

#ifndef DBG_ONLY
	#ifndef NDEBUG
		#define DBG_ONLY(exp)       exp
	#else
		#define DBG_ONLY(exp)       
	#endif //DEBUG
#endif

#ifndef SNI_BASED_CLIENT
#define _BID_UNICODE_LOADER
#endif
#ifdef	SNIX
#define _BID_EXPLICIT_EXPORT
#endif
#include "BidApi.h"
#include "BidApiEx.h"

#ifdef SNI_BASED_CLIENT

#ifdef SNIX

C_ASSERT ( sizeof (TCHAR) == sizeof (WCHAR));

#define SNIMemObj void
#define ISOSHost void
#define ISOSHost_MemObj void
#define NewNoX(gpmo) new
#define Assert(x) assert(x)

#define ALIGNMENT_DEFAULT 8

#define wcslenInWChars wcslen
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof (a) / sizeof ((a) [0]))
#endif


#ifndef CCH_ANSI_STRING
#define CCH_ANSI_STRING(x) sizeof (x)
#endif

//
// The following macros and inline functions are used to route the 
// locale sensitive functions used in Yukon to their locale insensitive 
// counterpart avaiable in normal WinSDK. SNI uses system
// locale exclusively in string related functions both in YUKON 
// and SNIX. Hence, it is ok to do so. 
//
// getaddrinfo_l
// getnameinfo_l
// StringCchPrintf_lA
// StringCchPrintf_lW
// StringCchPrintf_lExW
//


#define getaddrinfo_l(a,b,c,d,e) getaddrinfo(a,b,c,d)
#define getnameinfo_l(a,b,c,d,e,f,g,h) getnameinfo(a,b,c,d,e,f,g)

#define GetAddrInfoW_l(a,b,c,d,e) GetAddrInfoW(a,b,c,d)
#define GetNameInfoW_l(a,b,c,d,e,f,g,h) GetNameInfoW(a,b,c,d,e,f,g)

_locale_t __stdcall GetDefaultLocale();


#else	// #ifdef SNIX

#define SNIMemObj ISOSHost_MemObj

#endif	// #ifdef SNIX

extern SNIMemObj *gpmo;

typedef LPTHREAD_START_ROUTINE WaitThreadRoutine;

extern HANDLE ghIoCompletionPort;

class SOS_IOCompRequest;
typedef void  SOS_IOCompRoutine (SOS_IOCompRequest*  pVoid);	// Callback function for SOS to call into SNI

typedef LONG_PTR 	Counter;

#else	// #ifdef SNI_BASED_CLIENT

#define gpmo ((SNIMemObj *)SOS_Node::GetCurrent ()->RetrieveObject (NLS_SNI_MEMOBJ))

typedef SOS_Task::Func WaitThreadRoutine;

#endif	// #ifdef SNI_BASED_CLIENT

#pragma region BreadCrumbs
//2 Breadcrumbs - for finding decisions which were made based on an OS version.
//2 When support for the OS in question is removed, define the breadcrumb to be a compiler assert.
#define WIN2K3_DEPENDENCY(dependency)

#pragma endregion BreadCrumbs

#define SNIAPI_TAG 			BID_TAG1("API|SNI") 
#define ERROR_TAG			BID_TAG1("ERR|SNI")
#define RETURN_TAG 			BID_TAG1("RET|SNI")
#define CALLBACK_TAG		BID_TAG1("CALLBACK|SNI")
#define COMPLETION_TAG		BID_TAG1("COMPLETION|SNI")
#define CATCH_ERR_TAG		BID_TAG1("CATCH|ERR|SNI")
// Bid[Obtain|Update]ItemID are explicitly ANSI to avoid risks of 
// TCHAR mismatch
#define SNI_ID_TAG			BID_TAG1A("ID|SNI")
#define SNI_TAG 			BID_TAG1("SNI") 
#define SNI_SMUX_HEADER_TAG	BID_TAG1("SNI|SMUX_HEADER")
#define SNI_VIA_DESC_TAG	BID_TAG1("SNI|VIA_DESC")

// Define the SMUX packet control bit.
// Spare bits (per dll) are under 0xFFFFF000 mask.
// Bits 0x0003F000 belong to SNI.  
//
// !!! Important: the bits specified below must match 
// the corresponding BID_METATEXT definitions.  
//
// See sni.cpp for BID_METATEXT definition of BIDX_APIGROUP_SNI
//
#define BIDX_APIGROUP_SNI			0x00020000
// See smux.cpp for BID_METATEXT definition of BIDX_APIGROUP_SMUX_HEADER
//
#define BIDX_APIGROUP_SMUX_HEADER	0x00001000
// See viahelper.cpp for BID_METATEXT definition of BIDX_APIGROUP_VIA_DESC
//
#define BIDX_APIGROUP_VIA_DESC		0x00002000	


#define SNI_BID_TRACE_ON 	BidAreOn( BID_APIGROUP_TRACE | BIDX_APIGROUP_SNI )
#define SNI_BID_SCOPE_ON    BidAreOn( BID_APIGROUP_SCOPE | BIDX_APIGROUP_SNI )


//
//	UPDATE NOTE:
//	These SNI specific flavors of BidScopeXxxx macros do not follow general
//	rule to not use directly anything that starts with '_bid' prefix.
//	Therefore, the macros below can potentially need to be updated in order 
//	to get compiled with the next version(s) of BID.
//
#define	BidxScopeEnterSNI0A(stf)						_bidCT;	_bid_C0(A,SNI_BID_SCOPE_ON,&_bidScp,stf)
#define	BidxScopeEnterSNI1A(stf,a)						_bidCT;	_bid_C1(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a)
#define	BidxScopeEnterSNI2A(stf,a,b)					_bidCT;	_bid_C2(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b)
#define	BidxScopeEnterSNI3A(stf,a,b,c)					_bidCT;	_bid_C3(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c)
#define	BidxScopeEnterSNI4A(stf,a,b,c,d)				_bidCT;	_bid_C4(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d)
#define	BidxScopeEnterSNI5A(stf,a,b,c,d,e)				_bidCT;	_bid_C5(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e)
#define	BidxScopeEnterSNI6A(stf,a,b,c,d,e,f)			_bidCT;	_bid_C6(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f)
#define	BidxScopeEnterSNI7A(stf,a,b,c,d,e,f,g)			_bidCT;	_bid_C7(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g)
#define	BidxScopeEnterSNI8A(stf,a,b,c,d,e,f,g,h)		_bidCT;	_bid_C8(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h)
#define	BidxScopeEnterSNI9A(stf,a,b,c,d,e,f,g,h,i)		_bidCT;	_bid_C9(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h,i)
#define	BidxScopeEnterSNI10A(stf,a,b,c,d,e,f,g,h,i,j)	_bidCT;	_bid_C10(A,SNI_BID_SCOPE_ON,&_bidScp,stf, a,b,c,d,e,f,g,h,i,j)  

#define	BidxScopeEnterSNI0W(stf)						_bidCT;	_bid_C0(W,SNI_BID_SCOPE_ON,&_bidScp,stf)
#define	BidxScopeEnterSNI1W(stf,a)						_bidCT;	_bid_C1(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a)
#define	BidxScopeEnterSNI2W(stf,a,b)					_bidCT;	_bid_C2(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b)
#define	BidxScopeEnterSNI3W(stf,a,b,c)					_bidCT;	_bid_C3(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c)
#define	BidxScopeEnterSNI4W(stf,a,b,c,d)				_bidCT;	_bid_C4(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d)
#define	BidxScopeEnterSNI5W(stf,a,b,c,d,e)				_bidCT;	_bid_C5(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e)
#define	BidxScopeEnterSNI6W(stf,a,b,c,d,e,f)			_bidCT;	_bid_C6(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f)
#define	BidxScopeEnterSNI7W(stf,a,b,c,d,e,f,g)			_bidCT;	_bid_C7(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g)
#define	BidxScopeEnterSNI8W(stf,a,b,c,d,e,f,g,h)		_bidCT;	_bid_C8(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h)
#define	BidxScopeEnterSNI9W(stf,a,b,c,d,e,f,g,h,i)		_bidCT;	_bid_C9(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h,i)
#define	BidxScopeEnterSNI10W(stf,a,b,c,d,e,f,g,h,i,j)	_bidCT;	_bid_C10(W,SNI_BID_SCOPE_ON,&_bidScp,stf, a,b,c,d,e,f,g,h,i,j)  

#define	BidxScopeAutoSNI0A(stf)							_bidCTA; _bid_C0(A,SNI_BID_SCOPE_ON,&_bidScp,stf)
#define	BidxScopeAutoSNI1A(stf,a)						_bidCTA; _bid_C1(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a)
#define	BidxScopeAutoSNI2A(stf,a,b)						_bidCTA; _bid_C2(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b)
#define	BidxScopeAutoSNI3A(stf,a,b,c)					_bidCTA; _bid_C3(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c)
#define	BidxScopeAutoSNI4A(stf,a,b,c,d)					_bidCTA; _bid_C4(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d)
#define	BidxScopeAutoSNI5A(stf,a,b,c,d,e)				_bidCTA; _bid_C5(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e)
#define	BidxScopeAutoSNI6A(stf,a,b,c,d,e,f)				_bidCTA; _bid_C6(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f)
#define	BidxScopeAutoSNI7A(stf,a,b,c,d,e,f,g)			_bidCTA; _bid_C7(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g)
#define	BidxScopeAutoSNI8A(stf,a,b,c,d,e,f,g,h)			_bidCTA; _bid_C8(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h)
#define	BidxScopeAutoSNI9A(stf,a,b,c,d,e,f,g,h,i)		_bidCTA; _bid_C9(A,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h,i)
#define	BidxScopeAutoSNI10A(stf,a,b,c,d,e,f,g,h,i,j)	_bidCTA;	_bid_C10(A,SNI_BID_SCOPE_ON,&_bidScp,stf, a,b,c,d,e,f,g,h,i,j)  

#define	BidxScopeAutoSNI0W(stf)							_bidCTA; _bid_C0(W,SNI_BID_SCOPE_ON,&_bidScp,stf)
#define	BidxScopeAutoSNI1W(stf,a)						_bidCTA; _bid_C1(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a)
#define	BidxScopeAutoSNI2W(stf,a,b)						_bidCTA; _bid_C2(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b)
#define	BidxScopeAutoSNI3W(stf,a,b,c)					_bidCTA; _bid_C3(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c)
#define	BidxScopeAutoSNI4W(stf,a,b,c,d)					_bidCTA; _bid_C4(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d)
#define	BidxScopeAutoSNI5W(stf,a,b,c,d,e)				_bidCTA; _bid_C5(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e)
#define	BidxScopeAutoSNI6W(stf,a,b,c,d,e,f)				_bidCTA; _bid_C6(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f)
#define	BidxScopeAutoSNI7W(stf,a,b,c,d,e,f,g)			_bidCTA; _bid_C7(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g)
#define	BidxScopeAutoSNI8W(stf,a,b,c,d,e,f,g,h)			_bidCTA; _bid_C8(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h)
#define	BidxScopeAutoSNI9W(stf,a,b,c,d,e,f,g,h,i)		_bidCTA; _bid_C9(W,SNI_BID_SCOPE_ON,&_bidScp,stf,a,b,c,d,e,f,g,h,i)
#define	BidxScopeAutoSNI10W(stf,a,b,c,d,e,f,g,h,i,j)	_bidCTA;	_bid_C10(W,SNI_BID_SCOPE_ON,&_bidScp,stf, a,b,c,d,e,f,g,h,i,j)  

#if	defined( _UNICODE )
	#define	BidxScopeEnterSNI0		BidxScopeEnterSNI0W
	#define	BidxScopeEnterSNI1		BidxScopeEnterSNI1W
	#define	BidxScopeEnterSNI2		BidxScopeEnterSNI2W
	#define	BidxScopeEnterSNI3		BidxScopeEnterSNI3W
	#define	BidxScopeEnterSNI4		BidxScopeEnterSNI4W
	#define	BidxScopeEnterSNI5		BidxScopeEnterSNI5W
	#define	BidxScopeEnterSNI6		BidxScopeEnterSNI6W
	#define	BidxScopeEnterSNI7		BidxScopeEnterSNI7W
	#define	BidxScopeEnterSNI8		BidxScopeEnterSNI8W
	#define	BidxScopeEnterSNI9		BidxScopeEnterSNI9W
	#define	BidxScopeEnterSNI10		BidxScopeEnterSNI10W

	#define	BidxScopeAutoSNI0		BidxScopeAutoSNI0W
	#define	BidxScopeAutoSNI1		BidxScopeAutoSNI1W
	#define	BidxScopeAutoSNI2		BidxScopeAutoSNI2W
	#define	BidxScopeAutoSNI3		BidxScopeAutoSNI3W
	#define	BidxScopeAutoSNI4		BidxScopeAutoSNI4W
	#define	BidxScopeAutoSNI5		BidxScopeAutoSNI5W
	#define	BidxScopeAutoSNI6		BidxScopeAutoSNI6W
	#define	BidxScopeAutoSNI7		BidxScopeAutoSNI7W
	#define	BidxScopeAutoSNI8		BidxScopeAutoSNI8W
	#define	BidxScopeAutoSNI9		BidxScopeAutoSNI9W
	#define	BidxScopeAutoSNI10		BidxScopeAutoSNI10W	
#else
	#define	BidxScopeEnterSNI0		BidxScopeEnterSNI0A
	#define	BidxScopeEnterSNI1		BidxScopeEnterSNI1A
	#define	BidxScopeEnterSNI2		BidxScopeEnterSNI2A
	#define	BidxScopeEnterSNI3		BidxScopeEnterSNI3A
	#define	BidxScopeEnterSNI4		BidxScopeEnterSNI4A
	#define	BidxScopeEnterSNI5		BidxScopeEnterSNI5A
	#define	BidxScopeEnterSNI6		BidxScopeEnterSNI6A
	#define	BidxScopeEnterSNI7		BidxScopeEnterSNI7A
	#define	BidxScopeEnterSNI8		BidxScopeEnterSNI8A
	#define	BidxScopeEnterSNI9		BidxScopeEnterSNI9A
	#define	BidxScopeEnterSNI10		BidxScopeEnterSNI10A

	#define	BidxScopeAutoSNI0		BidxScopeAutoSNI0A
	#define	BidxScopeAutoSNI1		BidxScopeAutoSNI1A
	#define	BidxScopeAutoSNI2		BidxScopeAutoSNI2A
	#define	BidxScopeAutoSNI3		BidxScopeAutoSNI3A
	#define	BidxScopeAutoSNI4		BidxScopeAutoSNI4A
	#define	BidxScopeAutoSNI5		BidxScopeAutoSNI5A
	#define	BidxScopeAutoSNI6		BidxScopeAutoSNI6A
	#define	BidxScopeAutoSNI7		BidxScopeAutoSNI7A
	#define	BidxScopeAutoSNI8		BidxScopeAutoSNI8A
	#define	BidxScopeAutoSNI9		BidxScopeAutoSNI9A
	#define	BidxScopeAutoSNI10		BidxScopeAutoSNI10A
#endif


#define ASSERT_CONNECTION_ON_CORRECT_NODE(pConn) \
	Assert( pConn->m_NodeId == SOS_Node::GetCurrent()->GetNodeId())
	
#define ALIGNMENT_DEFAULT 8 //no of bytes


extern "C"  inline SNI_Packet *SNIPacketAllocateEx( __in SNI_Conn * pConn, 
									SNI_Packet_IOType IOType,
								   	SOS_IOCompRoutine pfunComp);


extern "C" LPSTR StrStrA_SYS(DWORD dwCmpFlags, LPCSTR lpstring1,int cchCount1, LPCSTR lpstring2, int cchCount2);
extern "C" LPSTR StrChrA_SYS(__in_ecount(cchCount1) LPCSTR lpstring1, int cchCount1/* total char count*/, char character);
extern "C" LPWSTR StrChrW_SYS(__in_ecount(cchCount1) LPCWSTR lpwstring1, int cchCount1/* total char count*/, WCHAR character);
extern "C" LPWSTR StrStrW_SYS(DWORD dwCmpFlags, LPCWSTR lpwstring1,int cchCount1, LPCWSTR lpwstring2, int cchCount2);
extern "C" DWORD StrTrimBothW_Sys( __inout_ecount(cchOrigin) LPWSTR  pwszOrigin /* input string */,
						size_t cchOrigin	/* total char count */, 
						__out size_t *cchReturnCount /* total char count after removal */,
						__in_ecount(cchTargets) LPCWSTR pwszTargets /* chars for removal */,
						int cchTargets);
extern "C" DWORD StrTrimBoth_Sys( __inout_ecount(cchOrigin) LPSTR  pszOrigin /* input string */,
				size_t cchOrigin	/* total char count */, 
				__out size_t *cchReturnCount /* total char count after removal */,
				__in_ecount(cchTargets) LPCSTR pszTargets /* chars for removal */,
				int cchTargets);

extern "C" void StrTrimLeftW_Sys(__inout LPWSTR wszConnect);
extern "C" DWORD SNICreateWaitThread(WaitThreadRoutine pfnAsyncWait, PVOID pParam);
extern "C" inline DWORD SNIRegisterWithIOCP(HANDLE hNwk);
extern "C" HMODULE SNILoadSystemLibA(LPCSTR szDllName); 

extern "C" void IncrementPendingAccepts();
extern "C" void DecrementPendingAccepts();

extern "C"	inline void SNITraceErrorToConsumer(	__in SNI_Conn * pConn, DWORD dwOSError, DWORD dwSNIError);


#if defined(SNI_BASED_CLIENT) || defined(STANDALONE_SNI)
#define SNILoadSystemLibraryA(szDllName) SNILoadSystemLibA(szDllName)
#else	//	#ifdef SNI_BASED_CLIENT
#define SNILoadSystemLibraryA(szDllName)  DBLoadLibraryA( szDllName, fTrue )
#endif	//	#ifdef SNI_BASED_CLIENT #else ...

#if defined (STANDALONE_SNI)
#define DBLoadLibraryA(A,B) 	SNILoadSystemLibA(A)
#endif

#ifdef SNI_BASED_CLIENT

int SNIExceptionFilter(LPEXCEPTION_POINTERS pExc);

#define SNI_TRY \
    __try \
    {

#define SNI_CATCH \
    } \
    __except (SNIExceptionFilter(GetExceptionInformation())) \
    {

#define SNI_ENDCATCH \
    } \

#ifndef SNIX

class CCriticalSectionNT_SNI
{
protected:
	// constructor  
	CCriticalSectionNT_SNI()
	{
		m_fInitialized = false;
		cs = NULL;
	}
	
	DWORD FInit()
	{
		if( MPInitializeCriticalSectionAndSpinCount(&cs, 0) )
		{
			m_fInitialized = true;
			return ERROR_SUCCESS;
		}
		return ERROR_OUTOFMEMORY;
	}

public:

	// locking -- enter CS
	void Enter()
	{
		Assert(m_fInitialized);
		MPEnterCriticalSection(&cs);
	}

	// locking -- try to enter CS
	bool TryEnter()
	{
		Assert(m_fInitialized);
		return (TRUE == MPTryEnterCriticalSection(&cs));
	}

	// locking -- leave CS
	void Leave()
	{
		Assert(m_fInitialized);
		MPLeaveCriticalSection(&cs);
	}

	static DWORD Initialize(__out CCriticalSectionNT_SNI **pcs)
	{
		DWORD dwErr;
		
		if(NULL == (*pcs = NewNoX(gpmo) CCriticalSectionNT_SNI()))
			return ERROR_OUTOFMEMORY;
		
		if(ERROR_SUCCESS != (dwErr = (*pcs)->FInit()))
		{
			delete (*pcs);
			*pcs = NULL;
			return dwErr;
		}

		return ERROR_SUCCESS;
	}


	// destructor:  delete member CS
	virtual ~CCriticalSectionNT_SNI()
	{
		if(m_fInitialized)
		{
			MPDeleteCriticalSection(&cs);
		}
		else
		{
			Assert(!cs);
		}
	}


private:

	bool m_fInitialized;
	MP_CRITICAL_SECTION cs;
};

#else	// #ifndef SNIX

class CCriticalSectionNT_SNI
{
protected:
	// constructor  
	CCriticalSectionNT_SNI()
	{
		m_fInitialized = false;
	}
	
	DWORD FInit()
	{
		InitializeCriticalSection(&cs);
		m_fInitialized = true;
		return ERROR_SUCCESS;
	}

public:

	// locking -- enter CS
	void Enter()
	{
		assert(m_fInitialized);
		EnterCriticalSection(&cs);
	}

	// locking -- try to enter CS
	bool TryEnter()
	{
		assert(m_fInitialized);
		return (TRUE == TryEnterCriticalSection(&cs));
	}

	// locking -- leave CS
	void Leave()
	{
		assert(m_fInitialized);
		LeaveCriticalSection(&cs);
	}

	static DWORD Initialize(CCriticalSectionNT_SNI **pcs)
	{
		DWORD dwErr;
		
		if(NULL == (*pcs = NewNoX(gpmo) CCriticalSectionNT_SNI()))
			return ERROR_OUTOFMEMORY;
		
		if(ERROR_SUCCESS != (dwErr = (*pcs)->FInit()))
		{
			delete (*pcs);
			*pcs = NULL;
			return dwErr;
		}

		return ERROR_SUCCESS;
	}


	// destructor:  delete member CS
	virtual ~CCriticalSectionNT_SNI()
	{
		DeleteCriticalSection(&cs);
	}


private:

	bool m_fInitialized;
	CRITICAL_SECTION cs;
};

#endif	// #ifndef SNIX

typedef  CCriticalSectionNT_SNI SNICritSec;

#else

#include <assert.h>

int SNIExceptionFilter(LPEXCEPTION_POINTERS pExc);

#define SNI_TRY \
    __try \
    {

#define SNI_CATCH \
    } \
    __except (SNIExceptionFilter(GetExceptionInformation())) \
    {

#define SNI_ENDCATCH \
    } \

class CCriticalSectionSOS
{
protected:
	// constructor:  initialize member CS
	CCriticalSectionSOS():
		m_pMutex(NULL)
	{		
	}
		
	DWORD FInit()
	{
		m_pMutex = NewNoX(gpmo)SOS_UnfairRecursiveMutexExtendedGuarantee();
		
		return (NULL != m_pMutex ? ERROR_SUCCESS:ERROR_OUTOFMEMORY);
	}

public:
	// locking -- enter CS
	void Enter()
	{
		DELAYABORT_IN_THIS_SCOPE;
		Assert(m_pMutex);
		
		SOS_WaitInfo waitInfo(PWAIT_SNI_CRITICAL_SECTION); 
		
		HRESULT hr = m_pMutex->Wait(INFINITE, &waitInfo);
	}

	// locking -- try to enter CS
	bool TryEnter()
	{
		// Note: SOS will never give us abort if the wait 
		// time is 0, hence there is no need to delay abort 
		// in this scope.  
		Assert(m_pMutex);
		
		SOS_WaitInfo waitInfo(PWAIT_SNI_CRITICAL_SECTION); 

		Assert(SOS_OK == SOS_WAIT_OK);
		return (SOS_OK == m_pMutex->Wait(0, &waitInfo) );

	}

	// locking -- leave CS
	void Leave()
	{
		// Note: SOS will never give us abort from ReleaseMutex, 
		// hence there is no need to delay abort in this scope.  
		Assert(m_pMutex);
		m_pMutex->Release();
	}

	static DWORD Initialize(CCriticalSectionSOS **pcs)
	{
		DWORD dwErr;
		
		if(NULL == (*pcs = NewNoX(gpmo) CCriticalSectionSOS()))
			return ERROR_OUTOFMEMORY;
		
		if(ERROR_SUCCESS != (dwErr = (*pcs)->FInit()))
		{
			delete (*pcs);
			*pcs = NULL;
			return dwErr;
		}

		return ERROR_SUCCESS;
	}

	// constructor:  destroy member CS
	virtual ~CCriticalSectionSOS()
	{
		if (m_pMutex)
		{
			delete m_pMutex;//->Destroy();
			m_pMutex = NULL;
		}
	}
		
		
private:
	SOS_UnfairRecursiveMutexExtendedGuarantee * m_pMutex;
	DWORD_PTR m_OwningId;
	LONG LockCount; //I expect SOS to provide a method to get recursion count
					//then this will not be needed
};

typedef CCriticalSectionSOS SNICritSec;
#endif

inline void MyEnterCriticalSection(SNICritSec **pcs)
{
	(*pcs)->Enter();
}

inline void MyLeaveCriticalSection(SNICritSec **pcs)
{
	(*pcs)->Leave();
}

inline void MyDeleteCriticalSection(SNICritSec **pcs)
{
	delete *pcs;
	(*pcs) = NULL;
}

inline bool MyTryEnterCriticalSection(SNICritSec **pcs)
{
	return (*pcs)->TryEnter();
}

#undef EnterCriticalSection
#undef LeaveCriticalSection
#undef InitializeCriticalSection
#undef DeleteCriticalSection
#undef TryEnterCriticalSection

#define EnterCriticalSection MyEnterCriticalSection
#define LeaveCriticalSection MyLeaveCriticalSection
//#define InitializeCriticalSection MyInitializeCriticalSection
#define DeleteCriticalSection MyDeleteCriticalSection
#define TryEnterCriticalSection MyTryEnterCriticalSection


class SNI_Provider;


//----------------------------------------------------------------------------
// Name: 	SNI_REF
//
// Purpose:	Reference counting at SNI_Conn.
//
// IMPORTANT!!! Should any enum member be added or removed, please update
// Sql\Ntdbms\dscript\scripts\sniconns.js with appropriate REF_Max and details
//----------------------------------------------------------------------------
typedef enum {
	REF_Active,				// this is decremented when the connection is closed
	REF_InternalActive,		// sni providers increments this ref count to keep object around
	REF_Packet,				// packet allocations
	REF_Read,				// user read requests
	REF_InternalRead,		// reads posted by sni providers for their own purposes
	REF_Write,				// user write requests
	REF_InternalWrite,		// reads posted by sni providers for their own purposes
	REF_ActiveCallbacks,	// Ref'd by SNIReadDone and SNIWriteDone during read and write completion calls.	
	REF_PacketNotOwningBuf,	// SNI_Packet_KeyHolderNoBuf or SNI_Packet_VaryingBuffer*
	REF_Max
} SNI_REF;

#ifndef SNI_BASED_CLIENT
extern InstrFlags g_InstrBegin;
#endif

//----------------------------------------------------------------------------
// Name: 	SNITicksWrapper
//
// Purpose:	Wrapper used to hold time information on the server side.
//			
//----------------------------------------------------------------------------
class SNITicksWrapper
{
public:
#ifndef SNI_BASED_CLIENT
	SOS_TicksFast64 m_ticks;
#endif
};

//----------------------------------------------------------------------------
// Name: 	SNITime
//
// Purpose:	Class for time related stuff
//			
//----------------------------------------------------------------------------
class SNITime
{
	friend DWORD SNIGetInfo (SNI_Conn *, UINT, VOID *);
	
private:
	ULARGE_INTEGER m_StartTime;

public:
	SNITicksWrapper m_StartTick;
	SNITicksWrapper m_ReadDone;
	SNITicksWrapper m_WriteDone;

	SNITime()
	{
#ifndef SNI_BASED_CLIENT
		m_ReadDone.m_ticks.SetZero ();
		m_WriteDone.m_ticks.SetZero ();
#endif
	}
	
	~SNITime()
		{}

	DWORD FInit()
	{
		FILETIME StartTime;
		DWORD dwError = ERROR_SUCCESS;
		
		GetSystemTimeAsFileTime( &StartTime );
		m_StartTime.LowPart = StartTime.dwLowDateTime;
		m_StartTime.HighPart = StartTime.dwHighDateTime;

#ifndef SNI_BASED_CLIENT
		// No timestamp information is available on SNI client.
		//
		m_StartTick.m_ticks.LoadTicks ();

		m_ReadDone.m_ticks = m_StartTick.m_ticks;
		m_WriteDone.m_ticks = m_StartTick.m_ticks;
#endif //#ifndef SNI_BASED_CLIENT		

		return dwError;
	}

	static void GetTick(SNITicksWrapper * v)
	{
#ifndef SNI_BASED_CLIENT	
		// For performance reasons, time 
		// statistics only available on server side.
		if( g_InstrFlags.gCollectingStatistics )
			v->m_ticks.LoadTicks ();
#endif		
	}
	
	void GetTime(SNITicksWrapper v, SYSTEMTIME * sys)
	{
		ULARGE_INTEGER t;
		SYSTEMTIME tmp = {0};

#ifdef SNI_BASED_CLIENT

		// For performance reasons, time statistics only available on server side.
		//
		t.QuadPart = 0;

#else
		SNITicksWrapper d;

		// Overflow's here are going to be problematic in terms
		// of date reporting, but if we overflow it means that we
		// reached the end of the range of FILETIME structures.
		// Which currently amounts to the end of the world.
		//
		d.m_ticks = v.m_ticks.GetInterval (m_StartTick.m_ticks);

		// convert to 100-nanoseconds, which is the unit of m_StartTime and FILETIME
		//
		t.QuadPart = m_StartTime.QuadPart + SOS_Time::GetTime<HundredNanosec> (d.m_ticks);
#endif

		FileTimeToSystemTime( (const FILETIME*) &t, &tmp );
		SystemTimeToTzSpecificLocalTime( 0, &tmp, sys);
	}
	
};

//----------------------------------------------------------------------------
// Name: 	SNI_CONN_INFO
//
// Purpose:	Struct for connection information - used for stats, etc.
//			
//----------------------------------------------------------------------------
typedef struct 
{ 
	ULONG  SentPackets; 
	ULONG  RecdPackets; 
	ULONG  ConsBufferSize; 
	ULONG  ProvBufferSize;
	ULONG  ProvOffset;
	ProviderNum TransportProvider;
	SNITime Timer;
} SNI_CONN_INFO, *PSNI_CONN_INFO; 

//----------------------------------------------------------------------------
// Name: 	SNI_Conn
//
// Purpose:	Class for SNI Connection.
//
// Notes:	This is the primary SNI object. Almost all calls to/from SNI require an SNI_Conn
//			object as the parameter. SNI creates an SNI_Conn object in SNIOpen and 
//			SNIAcceptDone and destroys it in SNIClose.
//			
//----------------------------------------------------------------------------
class SNI_Conn
{
	friend DWORD SNIWaitForSSLHandshakeToComplete( SNI_Conn *, DWORD);
	friend DWORD SNIGetInfo (SNI_Conn *, UINT, VOID *);
	friend DWORD SNISetInfo(SNI_Conn*, UINT, VOID*);
	friend DWORD SNIAddProvider (SNI_Conn * pConn, ProviderNum ProvNum, LPVOID pInfo);
	friend DWORD SNIRemoveProvider (SNI_Conn * pConn, ProviderNum ProvNum);
	friend DWORD SNIClose(SNI_Conn * pConn);
	friend DWORD SNISecImpersonateContext(SNI_Conn * );
	friend DWORD SNISecRevertContext(SNI_Conn * );
	
private:
	// Unique connection information - GUID and connection id from consumer (when available)
	// For engine, ConsumerConnId is the spid
	SNIUCI m_Uci;

	const static DWORD MAX_PSM_ARRAY	= 2048;
	static SNI_Conn  * rgSniConn[MAX_PSM_ARRAY];
	static DWORD iSniConnIndex;
	DWORD m_dwConnIndex;

	// IMPORTANT: The following critical section is not for "general" protection
	// of the SNI_Conn.  It has a very specific usage.  Namely, it protects the 
	// provider list consistency.  The SNIAddProvider/SNIRemoveProvider API's
	// can collide with SNIGetInfo which can/will happen on different threads
	// than the SNI processing -- the connection virtual table is the primary
	// place where this is an issue.  The contention should be very low, so this
	// is not a huge bottle-neck.  However, it is important to not mistake this
	// member as an indication that SNI_Conn objects are completely thread-safe.
	SNICritSec * m_csProvList;

	// Initialized to zero and set to 1 by SNIX when SNIClose is entered.
	// Checked by SNIX SNIReadDone and SNIWriteDone to avoid queuing new
	// managed callbacks when the packet's connection is closing.
	LONG m_lSNIClosePending;

	int m_iBidId; 
	
	SNI_Conn();
	~SNI_Conn();

	DWORD AllocAndSetName(__out WCHAR **pwszTarget, __in __nullterminated WCHAR *wszSource, bool fFailEmptySource);
public:

	// 

	WCHAR *m_pwszServer;

	// used for SSL certificate matching.
	WCHAR *m_pwszOriginalServer;
	SNI_Provider * m_pProvHead;
	SNI_CONSUMER_INFO m_ConsumerInfo;
	LPVOID m_ConsKey;
	SNI_CONN_INFO m_ConnInfo;
	DWORD m_MemTag;
	bool m_fSync:1;
	bool m_fClient:1;
	bool m_fBindingsCheckFailed:1;
	SNI_Sec * m_pSec;
	UINT m_cid;
	void *m_pvChannelBindings;

	LONG m_cRefTotal;
	LONG m_rgRefCount[REF_Max];

#ifndef SNI_BASED_CLIENT
	//Compiler complain about if we define it as Counter instead. 
	//basicly, due to SOS defination. I can't trace it to there yet.
	
	USHORT m_NodeId;  
#else
	Counter m_NodeId;
#endif

	SNI_EPInfo * m_pEPInfo;

	static DWORD InitObject(__out SNI_Conn **ppConn, BOOL fServer = FALSE);
	
	inline LONG AddRef(SNI_REF refType);
	inline LONG Release(SNI_REF refType);	

	DWORD SetServerName( __in __nullterminated WCHAR *wszServer, __in __nullterminated WCHAR *wszOriginalServerName);

	GUID GetConnId() const { return m_Uci.Id; }
	
	GUID GetConnPeerId() const {return m_Uci.PeerId;}


	void WaitForActiveCallbacks();
	bool FAllowCallback();
	void SafeReadComp( SNI_Packet* pPacket, DWORD dwProvError );
	void SafeWriteComp( SNI_Packet* pPacket, DWORD dwProvError );

	void ReleaseChannelBindings();

	inline int GetBidId() const { return m_iBidId; }; 
	inline int * GetBidIdPtr() { return &m_iBidId; }; 
};

#ifndef SNI_BASED_CLIENT
#define SNI_TRACE_FLAG(TRACE_TYPE, TRACE_FLAG) (TRACE_TYPE(TRACE_FLAG))
#else
#define SNI_TRACE_FLAG(TRACE_TYPE, TRACE_FLAG) (0)
#endif

#ifndef SNI_BASED_CLIENT
#ifdef DEBUG
#define SNI_ASSERT_ON_INVALID_PACKET if (!GLOBALTRACE_DEBUG(TRCFLG_DEBUG_SNI_PACKET_VALIDATION_ASSERT_OFF)){Assert( 0 && " Invalid SNI packet!\n" );}
#else
#define SNI_ASSERT_ON_INVALID_PACKET {}
#endif
#else
#define SNI_ASSERT_ON_INVALID_PACKET {}
#endif

// Make the thread preemptive if it is not yet.
#ifndef SNI_BASED_CLIENT 
#define THREAD_PREEMPTIVE_ON_START(PWAIT_PREEMPTIVE_REASON) 	{	\
		SOS_Task::AutoSwitchPreemptive a_su (PWAIT_PREEMPTIVE_REASON);
#else
#define THREAD_PREEMPTIVE_ON_START(PWAIT_PREEMPTIVE_REASON) 	{		
#endif
#define THREAD_PREEMPTIVE_ON_END }

//Define SmtPause() for SNAC
#ifdef SNI_BASED_CLIENT
//-----------------------------------------------------------------------------
// Function: SmtPause
//
// Description: 
//		Routine to call when spinning on Simultaneous Multi Threaded CPU's
//
// Returns:
//		none
//
// Notes:
//
// Definitive explanation:
//
// ftp:// download.intel.com/design/perftool/cbts/appnotes/sse2/w_spinlock.pdf
//
// The __yield() intrinsic should be defined for all CPUs and should be 
// available with Everett (cl 13.10) and later compilers.
//
// However, its not actually showing up on the 14.00 AMD64 compiler, perhaps
// because it has no meaning.
//
#if _MSC_VER >= 1310 && defined (_M_IA64)
extern "C" 
{
    void __yield();
} // extern "C"
#pragma intrinsic (__yield)
#define SmtPause() __yield ()

#else

#if defined (_X86_) || defined (_AMD64_)

extern "C" void _mm_pause(void);
#pragma intrinsic (_mm_pause)

#endif

inline void
SmtPause ()
{
#if defined (_X86_) || defined (_AMD64_)
	// PAUSE instruction (REP NOP) helps SMT scheduling
	//
	_mm_pause ();
#endif // _X86_
}
#endif // _MSC_VER >= 1310

#endif  // SNI_BASED_CLINET

#endif// SNI_COMMON_STUFF



