/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File:       BidApi_impl.h
//
//  Copyright:  (c) Microsoft Corporation
//
//  Contents:   Built-In Diagnostics Interface implementation.
//
//  Comments:   To be included at the bottom of the main .c/.cpp file
//              // MyApp.cpp
//              #include "precomp.h"        // BidApi.h included
//              ....
//              int _tmain()                // or any other EntryPoint
//              {     ...
//              }
//
//              #include "BidApi_impl.h"
//
//              Last Modified: 15-May-2005
//
//              <owner current="true" primary="true">Microsoft</owner>
//
#ifndef __BIDAPI_IMPL_H__ ///////////////////////////////////////////////////////////////////////
#define __BIDAPI_IMPL_H__

#ifndef _BID_DECLARED
  #error BidApi.h must be included before BidApi_impl.h
#endif
#ifndef __BIDAPI_H__
  //
  //  Built-In Diagnostics API is declared, but not via BidApi.h.
  //  Possible mismatch between Interface Declaration and Interface Implementation.
  //
  #pragma message("NOTE: BidApi.h wasn't used. Consider replacement for BidApi_impl.h")
#endif

#define _BIDIMPL_INCLUDED

#ifndef _VA_LIST_DEFINED
  #include  <stdarg.h>
#endif
#if !defined( _ONEXIT_T_DEFINED ) || !defined( _MAX_PATH )
  #include  <stdlib.h>
#endif

#ifndef _BID_EXTERNAL_DLL_NAME
  #define _BID_EXTERNAL_DLL_NAME        BID_T("<NotFound>")
#endif

#ifndef _BID_EXTERNAL_DLL_LOOKUP
  #ifdef _BID_STATIC_BINDING
    #define _BID_EXTERNAL_DLL_LOOKUP    FALSE
  #else
    #define _BID_EXTERNAL_DLL_LOOKUP    TRUE
  #endif
#endif

#ifndef _BID_IDENTITY_A
  #define _BID_IDENTITY_A               ""
#endif

#ifndef _BID_HMODULE
  #define _BID_HMODULE                  (HMODULE)BID_NOHANDLE
#endif

#ifndef _BID_MODULE_ADDRESS
  #define _BID_MODULE_ADDRESS           BidSetApiGroupBits
#endif

#ifndef _BID_CFG_USER
  #define _bidCFG_USER                  0
#else
  #define _bidCFG_USER                  ((_BID_CFG_USER)|BID_CFG_ACTIVE_USER)
#endif

#ifndef _BID_APIGROUP_INIT
  #define _BID_APIGROUP_INIT            0
#endif


#if defined( _BID_EXTERNAL_DEFAULTS ) && defined( _BID_STATIC_BINDING )
  #undef _BID_STATIC_BINDING

  #ifndef _NO_COMPILE_NOTES
  #pragma message("NOTE: BidApi_impl.h: _BID_STATIC_BINDING has been undefined.")
  #endif
#endif

//
//  Define (uncomment) the symbol below if your multi-threaded module is experiencing
//  problems at loading phase.
//
//#define _BID_SYNC_LOAD

//
//  Define (uncomment) the symbol(s) below in order to disable automatic loading/unloading
//  of the implementation DLL. In this case, the client code is responsible for calling
//  BidLoad / BidUnload functions.
//
//#define _BID_NO_AUTO_LOAD
//#define _BID_NO_AUTO_UNLOAD

#ifdef _BID_NO_CRT
  #ifndef _BID_NO_AUTO_LOAD
  #define _BID_NO_AUTO_LOAD
  #endif
  #ifndef _BID_NO_AUTO_UNLOAD
  #define _BID_NO_AUTO_UNLOAD
  #endif
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Abstraction for WINAPI functions used in dynamic loader (Part II)
//
#ifndef _BID_EXTERNAL_WINAPI

  #ifdef _BID_UNICODE_LOADER
    #ifndef _BID_WINAPI
    #define _BID_WINAPI(apiName)        apiName ## W
    #endif
  #else
    #undef  _BID_WINAPI
    #define _BID_WINAPI(apiName)        apiName ## A
  #endif

  //
  //  Dynamic Binding:  LoadLibrary, FreeLibrary, GetProcAddress, VirtualQuery
  //  Sync. Load:       InterlockedExchange, InterlockedIncrement, Sleep
  //  Debug Only:       OutputDebugString, GetModuleFileName
  //  Self Diag:        GetModuleHandle
  //
  #pragma comment(lib, "Kernel32.lib")

  #ifndef BID_LoadLibrary
    #define BID_LoadLibrary             _BID_WINAPI(LoadLibrary)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_LoadLibrary defined externally.")
    #endif
  #endif

  #ifndef BID_FreeLibrary
    #define BID_FreeLibrary             FreeLibrary
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_FreeLibrary defined externally.")
    #endif
  #endif

  #ifndef BID_GetProcAddress
    #define BID_GetProcAddress          GetProcAddress
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_GetProcAddress defined externally.")
    #endif
  #endif

  #ifndef BID_VirtualQuery
    #define BID_VirtualQuery            VirtualQuery
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_VirtualQuery defined externally.")
    #endif
  #endif

  #ifndef BID_GetVersion
    #define BID_GetVersion              GetVersion
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_GetVersion defined externally.")
    #endif
  #endif

  #ifndef BID_Sleep
    #define BID_Sleep                   Sleep
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_Sleep defined externally.")
    #endif
  #endif

  #ifndef BID_OutputDebugString
    #define BID_OutputDebugString       _BID_WINAPI(OutputDebugString)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_OutputDebugString defined externally.")
    #endif
  #endif

  #ifndef BID_GetModuleFileName
    #define BID_GetModuleFileName       _BID_WINAPI(GetModuleFileName)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_GetModuleFileName defined externally.")
    #endif
  #endif

  #ifndef BID_GetModuleHandle
    #define BID_GetModuleHandle         _BID_WINAPI(GetModuleHandle)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_GetModuleHandle defined externally.")
    #endif
  #endif

  #ifndef BID_lstrlenA
    #define BID_lstrlenA                lstrlenA
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_lstrlenA defined externally.")
    #endif
  #endif

  #ifndef BID_lstrlenW
    #define BID_lstrlenW                lstrlenW
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_lstrlenW defined externally.")
    #endif
  #endif

  //
  //  Backward compatibility issue with 'volatile'
  //
  #ifndef BID_InterlockedExchange
    #if _MSC_VER >= 1300
      #define BID_InterlockedExchange(ptr,val)  InterlockedExchange(ptr,val)
    #else
      #define BID_InterlockedExchange(ptr,val)  InterlockedExchange((LONG*)ptr,val)
    #endif
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_InterlockedExchange defined externally.")
    #endif
  #endif

  #ifndef BID_InterlockedIncrement
    #if _MSC_VER >= 1300
      #define BID_InterlockedIncrement(pval)    InterlockedIncrement(pval)
    #else
      #define BID_InterlockedIncrement(pval)    InterlockedIncrement((LONG*)pval)
    #endif
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_InterlockedIncrement defined externally.")
    #endif
  #endif

#else

  #ifndef _NO_COMPILE_NOTES
  #pragma message("NOTE: _BID_EXTERNAL_WINAPI;  BID_<WinApi> defined externally.")
  #endif

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Default implementation for BidAssert
//
#ifndef BID_DefaultAssertA

  #if defined(_ASSERT_ON) && (!defined(_BID_NO_CRT) || defined(_BID_FORCE_DEFAULT_ASSERT))

    #ifndef _BID_CRTIMP
      #ifdef  _DLL
        #define _BID_CRTIMP __declspec(dllimport)
      #else
        #define _BID_CRTIMP
      #endif
    #endif

    //
    //  Standard CRT assert
    //
    #ifdef __cplusplus
    extern "C"
    #endif
    _BID_CRTIMP void __cdecl _assert(const char*, const char*, unsigned);

    static BOOL _bidDefaultAssertA(const char* expr, const char* fname, unsigned lineNum)
    {
        _assert(expr, fname, lineNum);
        //
        //  _assert() calls _debugbreak by itself or returns if assertion is ignored.
        //  So if we've got here, the answer should be "Continue Execution".
        //
        return TRUE;
    }
    #define BID_DefaultAssertA   _bidDefaultAssertA

  #endif

#else

  #ifndef _NO_COMPILE_NOTES
  #pragma message("NOTE: BID_DefaultAssertA defined externally.")
  #endif

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Forward declarations
//
static void _bidCallEntryPoint          (int, INT_PTR);
static void _bidReplaceStubs            (void);
static void _bidLoad                    (HANDLE*);

static void WINAPI _bidUnloadCallback   (BOOL);

#ifdef _BID_STATIC_BINDING
  #define _bidLoadEntryPoint()          _bidpEntryPoint = DllBidEntryPoint
  #define _bidUnloadEntryPoint()        _bidpEntryPoint = ImplBidEntryPoint
#else
  static void _bidLoadEntryPoint        (void);
  static void _bidUnloadEntryPoint      (void);
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                   LOADING SYNCHRONIZATION                                   //
/////////////////////////////////////////////////////////////////////////////////////////////////

volatile static LONG _bidInitialized  = 0;

#if !defined( _BID_SYNC_LOAD )

    _bid_INLINE BOOL _bidIsLoaded( void )
    {
        LONG tmp = _bidInitialized;
        _bidInitialized = 1;
        return (tmp != 0);
    }

    #define _bidIsUnloaded()            (0 == _bidInitialized)
    #define _bidLoadingDone()           ((void)0)
    #define _bidUnloadingDone()         _bidInitialized = 0

#else  // _BID_SYNC_LOAD

    volatile static LONG _bidLoading   = 0;
    volatile static LONG _bidUnloading = 0;

    static LONG _bidWait4LoadingComplete( void )
    {
        LONG reptCnt = 500;

        while( (_bidLoading != 0) && (--reptCnt > 0) )
        {
            BID_Sleep( 1 );
        }

       #ifdef _DEBUG_TRACE_ON       // 
        if( _bidLoading != 0 )
        {
            //
            //  Loading processed by another thread has timed out; possible deadlock.
            //  The chance to get here is too small, so we do nothing and will load
            //  the defaults later. However, we need to be informed about such a bad thing.
            //
            BID_OutputDebugString( BID_T("\n*** Loading of BID has timed out.\n\n") );
        }
       #endif

        return reptCnt;
    }

    static BOOL _bidIsLoaded( void )
    {
        LONG tmp = BID_InterlockedExchange( &_bidInitialized, 1 );

        if( tmp == 0 )
        {
            if( _bidUnloading != 0 )
            {
                //
                //  Race condition: another thread is UNLOADING the subsystem,
                //  so we won't proceed with loading.
                //
                tmp = 1;
            }
            else
            {
                //
                //  Normal loading path. The function will return FALSE,
                //  which means "Proceed with loading".
                //
                BID_InterlockedExchange( &_bidLoading, 1 );
            }
        }
        else if( _bidLoading != 0 )
        {
            //
            //  Race condition: another thread is loading the subsystem right now.
            //  Wait for reasonable time, but don't try to load again anyway.
            //  DASSERT( tmp != 0 );
            //
            _bidWait4LoadingComplete();
        }
        return (tmp != 0);  // tmp == 0 means "Ok to start loading"
    }

    static BOOL _bidIsUnloaded( void )
    {
        LONG tmp = BID_InterlockedExchange( &_bidInitialized, 0 );

        if( tmp != 0 )
        {
            if( _bidLoading != 0 )
            {
                tmp = _bidWait4LoadingComplete();
            }
            if( tmp > 0 )
            {
                BID_InterlockedExchange( &_bidUnloading, 1 );
            }
        }
        return (tmp == 0);  // tmp != 0 means "Ok to start unloading"
    }

    #define _bidLoadingDone()       BID_InterlockedExchange( &_bidLoading, 0 )
    #define _bidUnloadingDone()     BID_InterlockedExchange( &_bidUnloading, 0 )

#endif // _BID_SYNC_LOAD


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                            STUBS                                            //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Passive (blocking) stubs; shouldn't be called.
//
#define BID_FUNCTION_NOT_LOADED     0

static BOOL WINAPI stbBidPutStrA(HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  str)
{
    hID; src; info; str;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}
static BOOL WINAPI stbBidPutStrW(HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR str)
{
    hID; src; info; str;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}

static BOOL WINAPI stbBidTraceVA(HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  fmt, va_list args)
{
    hID; src; info; fmt; args;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}
static BOOL WINAPI stbBidTraceVW(HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR fmt, va_list args)
{
    hID; src; info; fmt; args;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}

static BOOL WINAPI stbBidScopeEnterVA(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp,
                                      PCSTR stf, va_list va)
{
    hID; src; info; pHScp; stf; va;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}
static BOOL WINAPI stbBidScopeEnterVW(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp,
                                      PCWSTR stf, va_list va)
{
    hID; src; info; pHScp; stf; va;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}
static BOOL WINAPI stbBidScopeLeave(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp)
{
    hID; src; info; pHScp;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}

static BOOL WINAPI stbBidEnabledA(HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  tcs)
{
    hID; src; info; tcs;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}
static BOOL WINAPI stbBidEnabledW(HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR tcs)
{
    hID; src; info; tcs;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return FALSE;
}
static int  WINAPI stbBidIndent(HANDLE hID, int nIdx)
{
    hID; nIdx;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return 0;
}

static INT_PTR WINAPI stbBidSnap( HANDLE hID, INT_PTR evtID, INT_PTR arg1, INT_PTR arg2 )
{
    hID; evtID; arg1; arg2;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return 0;
}

static INT_PTR WINAPI stbBidCtlProc(HANDLE hID, INT_PTR cmdSpaceID, int cmd,
                                    INT_PTR a1, INT_PTR a2, INT_PTR a3)
{
    hID; cmdSpaceID; cmd; a1; a2; a3;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return 0;
}

static INT_PTR WINAPI stbBidTouch( HANDLE hID, UINT_PTR scope, UINT code, INT_PTR arg1, INT_PTR arg2 )
{
    hID; scope; code; arg1; arg2;
    DASSERT( BID_FUNCTION_NOT_LOADED );
    return 0;
}

#undef BID_FUNCTION_NOT_LOADED


//
//  Active stub; causes automatic loading & initialization
//
static BOOL WINAPI stbBidAssert(HANDLE hID, UINT_PTR arg, UINT_PTR info)
{
    _bidLoad( &hID );
    if( _bidPfn.BidAssert == stbBidAssert ) return FALSE;   // Request a debugbreak
    return (*_bidPfn.BidAssert)( hID, arg, info );
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DYNAMIC LOADER
//
static BOOL WINAPI stbBidEntryPoint(HANDLE* pID, int bInitAndVer, PCSTR sIdentity,
                                    DWORD cfgBits, DWORD* pGblFlags, BID_CTLCALLBACK fAddr,
                                    PBIDEXTINFO pExtInfo, PBIDHOOKS pHooks, PBIDSECTHDR pHdr)
{
    BOOL bOk;

    if( bInitAndVer == 0 || _bidIsLoaded() )
    {
        //
        //  Race Condition or logic error.
        //  All function hooks that still point to initialization stubs
        //  will be loaded with default implementation.
        //
        _bidReplaceStubs();
        return FALSE;
    }

    _bidLoadEntryPoint();

    DASSERT( _bidpEntryPoint != stbBidEntryPoint );

    bOk = (*_bidpEntryPoint)( pID, bInitAndVer, sIdentity, cfgBits, pGblFlags,
                              fAddr, pExtInfo, pHooks, pHdr );
    if( bOk )
    {
        _bidReplaceStubs();

       #ifndef _BID_NO_AUTO_UNLOAD
        _onexit( BidUnload );
       #endif
    }
    else
    {
        *pID = BID_NOHANDLE;
    }

    _bidLoadingDone();
    return bOk;

} // stbBidEntryPoint



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  LOADABLE FUNCTION HOOKS
//
#define _BID_APIENTRY(name)     stb##name,

_bidHooks _bidPfn =
{
    (-1),                       // INT_PTR      BindCookie
    _bidUnloadCallback,         // BIDUNLOADCB  UnloadCallback
    BID_LIST_API_ENTRIES        // BidXxxx_t    BidXxxx = stbBidXxxx ...
    0, 0, 0, 0,                 // FARPROC      Reserved [_bidHOOKS_RESERVED_SPOTS]
    BID_SANITY_CHECK            // INT_PTR      SanityCheck
};

BidEntryPoint_t _bidpEntryPoint = stbBidEntryPoint;

#undef  _BID_APIENTRY

//
//  Predefined and User-defined ApiGroup control bits for entire component
//
DWORD   _bidGblFlags = _BID_APIGROUP_INIT;

//
//  Module ID
//
HANDLE  _bidID = BID_NOHANDLE;

//
//  Holder for hModule (for internal use only)
//
HANDLE _bidHDLL = BID_NOHANDLE;

#ifndef xBidHDLL
  #ifdef _BID_GET_HDLL
    #define xBidHDLL    ((HMODULE)_bidHDLL)
  #else
    #define xBidHDLL    ((HMODULE)BID_NOHANDLE)
  #endif
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                 OUTLINE INTERFACE FUNCTIONS                                 //
/////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI BidSetApiGroupBits(DWORD mask, DWORD bits)
{
    DWORD tmpBits = _bidGblFlags;
    if( mask != 0 ){
        _bidGblFlags ^= (bits ^ tmpBits) & mask;
    }
    return tmpBits;
}

//
//  In some functions, the first argument is named '_bidID'. This is on purpose!
//
//  xBidXxxx macros take hidden argument _bidID, which usually refers to the module-wide variable.
//  By specifying function argument with exactly same name, we make existing macro take
//  function argument instead of the global variable.
//
void __cdecl xBidWriteA(HANDLE _bidID, PCSTR fmt, ... )
{
    va_list argptr;
    va_start( argptr, fmt );
    xBidTraceVA( 0, 0, fmt, argptr );
}

void __cdecl xBidWriteW(HANDLE _bidID, PCWSTR fmt, ... )
{
    va_list argptr;
    va_start( argptr, fmt );
    xBidTraceVW( 0, 0, fmt, argptr );
}

/////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR WINAPI _bidCtlProc(INT_PTR cmdSpaceID, int cmd, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
    return ((cmdSpaceID != 0) && (_bidID_IF))
            ? (*_bidPfn.BidCtlProc)(_bidID, cmdSpaceID, cmd, arg1, arg2, arg3)
            : 0;
}


BOOL __cdecl _bidWriteBin(HANDLE _bidID, UINT_PTR info, ... /*PCVOID pItem, UINT size, INT_PTR id*/)
{
    va_list argptr;
    LPCTSTR fmtStr;

    BID_TCFS(_fmtStr1, _T("<Trace|BLOB> %p %u\n"))
    BID_TCFS(_fmtStr2, _T("<Trace|BLOB2> %p %u %p\n"))
    BID_TCFS(_fmtStr3, _T("<Trace|BLOB|COPY> %p %u"))        // No '\n' here, intentionally.
    BID_TCFS(_fmtStr4, _T("<Trace|BLOB2|COPY> %p %u %p\n"))
    BID_TCFS(_fmtStr5, _T("<Trace|BLOB|BINMODE> %p %u\n"))

    switch( BID_InfoBlobType(info) )
    {
        default:
            DASSERT( BAD_ENUM );
            info = BID_BLOB;
            // Fall through...

        case BID_BLOB:          fmtStr = _fmtStr1;  break;
        case BID_BLOB2:         fmtStr = _fmtStr2;  break;
        case BID_BLOB_COPY:     fmtStr = _fmtStr3;  break;
        case BID_BLOB2_COPY:    fmtStr = _fmtStr4;  break;
        case BID_BLOB_BIN_MODE: fmtStr = _fmtStr5;  break;
    }

    va_start(argptr, info);

    return xBidTraceV(0, info, fmtStr, argptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

BOOL __cdecl _bidTraceA( UINT_PTR src, UINT_PTR info, PCSTR fmt, ... )
{
    va_list argptr;
    va_start( argptr, fmt );
    return xBidTraceVA( src, info, fmt, argptr );
}

BOOL __cdecl _bidTraceW( UINT_PTR src, UINT_PTR info, PCWSTR fmt, ... )
{
    va_list argptr;
    va_start( argptr, fmt );
    return xBidTraceVW( src, info, fmt, argptr );
}

BOOL __cdecl _bidScopeEnterA( HANDLE* pHScp, PCSTR stf, ... )
{
    va_list argptr;
    va_start( argptr, stf );
    return  xBidScopeEnterVA( pHScp, stf, argptr );
}

BOOL __cdecl _bidScopeEnterW( HANDLE* pHScp, PCWSTR stf, ... )
{
    va_list argptr;
    va_start( argptr, stf );
    return  xBidScopeEnterVW( pHScp, stf, argptr );
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int _bidObtainItemIDA(PCSTR textID, INT_PTR invariant, INT_PTR associate)
{
    return  _bidID_IF
            ? (int)_bidTouch(_bidID, textID, BID_TOUCH_OBTAIN_ITEM_IDA, invariant, associate)
            : 0;
}

int _bidObtainItemIDW(PCWSTR textID, INT_PTR invariant, INT_PTR associate)
{
    return  _bidID_IF
            ? (int)_bidTouch(_bidID, textID, BID_TOUCH_OBTAIN_ITEM_IDW, invariant, associate)
            : 0;
}

int _bidUpdateItemIDA(int itemID, PCSTR textID, INT_PTR associate)
{
    if( _bidID_IF ){
        (void)_bidTouch(_bidID, textID, BID_TOUCH_UPDATE_ITEM_IDA, &itemID, associate);
    }
    return itemID;
}

int _bidUpdateItemIDW(int itemID, PCWSTR textID, INT_PTR associate)
{
    if( _bidID_IF ){
        (void)_bidTouch(_bidID, textID, BID_TOUCH_UPDATE_ITEM_IDW, &itemID, associate);
    }
    return itemID;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Scope Tracking Automatic Wrapper
//
#if defined( __cplusplus )

BOOL _bidCAutoScopeAnchor::Out()
{
    if( m_hScp == BID_NOHANDLE ){
        return TRUE;
    }

    if( !_bidScpON ){
        m_hScp = BID_NOHANDLE;
        return TRUE;
    }

    return xBidScopeLeave_(&m_hScp);
}

#endif // __cplusplus


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  TRACING HRESULTs, RETURN CODEs AND SOURCE MARKERs
//
HRESULT WINAPI _bidTraceHR(UINT_PTR src, UINT_PTR info, HRESULT hr)
{
    BID_TCFS(_fmtStr, _T("<Trace|HR> 0x%08X{HRESULT} line %d\n"))

    //
    //  We intentionally compare hr with S_OK instead of using FAILED(hr).
    //  By default we also want to trace success codes (other than S_OK) along with error codes.
    //
    if( (hr != S_OK || BidIsOn(BID_APIGROUP_S_OK))  &&  _fmtStr != NULL )
    {
        if( FAILED(hr) ) info |= BID_TRIGGER;
        if( !xBidTrace( src, info, _fmtStr, hr, BID_SrcLine(info)) ) DBREAK();
    }
    return hr;
}


DWORD WINAPI _bidTraceErr(UINT_PTR src, UINT_PTR info, DWORD errCode)
{
    BID_TCFS(_fmtStr, _T("<Trace|ERR> %u{WINERR} line %d\n"))

    if( errCode == BID_AUTOERRCODE )
    {
        errCode = GetLastError();
    }

    if( (errCode != ERROR_SUCCESS || BidIsOn(BID_APIGROUP_S_OK)) &&  _fmtStr != NULL )
    {
        DWORD tmpErrCode = GetLastError();

        if( errCode != ERROR_SUCCESS ) info |= BID_TRIGGER;
        if( !xBidTrace( src, info, _fmtStr, errCode, BID_SrcLine(info)) ) DBREAK();

        if( tmpErrCode != GetLastError() )
        {
            SetLastError(tmpErrCode);
        }
    }
    return errCode;
}


BOOL WINAPI _bidTraceMark(UINT_PTR src, UINT_PTR info)
{
    BID_TCFS(_fmtStr, _T("<Trace|MARK> line %d\n"))

    return (_fmtStr != NULL) ? xBidTrace( src, info, _fmtStr, BID_SrcLine(info) )
                             : TRUE;
}


HRESULT WINAPI _bidScopeLeaveHR(UINT_PTR src, UINT_PTR info, HANDLE* pHScp, HRESULT hr)
{
    if( BidIsOn(BID_APIGROUP_TRACE) ){
        _bidTraceHR(src, info, hr);
    }
    info &= ~BID_DEMAND_SRC;    // See notes at BidScopeLeaveXxxx macro declaration.
    xBidScopeLeave(src, info, pHScp);
    return hr;
}


BOOL WINAPI _bidScopeLeaveErr(UINT_PTR src, UINT_PTR info, HANDLE* pHScp, BOOL bApi)
{
    if( !bApi && BidIsOn(BID_APIGROUP_TRACE) ){
        _bidTraceErr(src, info, BID_AUTOERRCODE);
    }
    info &= ~BID_DEMAND_SRC;    // See notes at BidScopeLeaveXxxx macro declaration.
    xBidScopeLeave(src, info, pHScp);
    return bApi;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  String Digest
//
#if defined( _BID_NO_CRT ) && !defined( _BID_NO_STRDIGEST )
  //
  //  BidStrDigest uses __try/__except block which requires CRT.
  //  If we define _BID_NO_CRT but still get linked with the CRT and really want to use
  //  BidStrDigest, then we can define _BID_FORCE_STRDIGEST in order to suppress this warning.
  //
  #ifndef _BID_FORCE_STRDIGEST
    #pragma message("WARN: _BID_NO_CRT defined, BidStrDigest won't link without CRT.")
  #endif
#endif

#ifndef _BID_NO_STRDIGEST

void WINAPI BID_InitCtrlCharSubstA(__out BID_CtrlCharSubstA* pCCS)
{
    pCCS->CR    = '\xB6';
    pCCS->LF    = '\xB0';
    pCCS->HT    = '\xB1';
    pCCS->Lead  = '\\';
}

void WINAPI BID_InitCtrlCharSubstW(__out BID_CtrlCharSubstW* pCCS)
{
    pCCS->CR    = L'\xB6';
    pCCS->LF    = L'\xB0';
    pCCS->HT    = L'\xB1';
    pCCS->Lead  = L'\\';
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL _bidStrDigest_FilterA(__in char ch, __in const BID_CtrlCharSubstA* pCCS, __out char* pCh)
{
    if( ch == '\n' )        *pCh = pCCS->LF;
    else if( ch == '\t' )   *pCh = pCCS->HT;
    else if( ch == '\r' )   *pCh = pCCS->CR;
    else {
        *pCh = pCCS->Lead;
        return FALSE;
    }
    return TRUE;    // first three cases
}

static BOOL _bidStrDigest_FilterW(__in WCHAR ch, __in const BID_CtrlCharSubstW* pCCS, __out WCHAR* pCh)
{
    if( ch == L'\n' )       *pCh = pCCS->LF;
    else if( ch == L'\t' )  *pCh = pCCS->HT;
    else if( ch == L'\r' )  *pCh = pCCS->CR;
    else {
        *pCh = pCCS->Lead;
        return FALSE;
    }
    return TRUE;    // first three cases
}

static char _bidHexDigit(__in int ch)
{
    static const char table[] = "0123456789ABCDEF";
    return table[ ch & 0x0F ];
}

/////////////////////////////////////////////////////////////////////////////////////////////////

PCSTR WINAPI xBidStrDigestExA(__out_ecount(outBufCapacity) PSTR outBuf,
                              __in int outBufCapacity,
                              __in PCSTR textStr, __in int textStrLen,
                              __in_opt const BID_CtrlCharSubstA* pCCS)
{
    char    ch;
    BOOL    bCut;
    UINT    strLen;
    PCSTR   inpPtr;
    PCSTR   inpEnd;
    PSTR    outEnd;
    PSTR    outPtr;

    BID_CtrlCharSubstA  defaultSubst;

    if( outBufCapacity <= 0 || textStr == NULL )
    {
        outPtr = NULL;
        goto labelExit;
    }

    outBuf[0] = '\0';
    outPtr    = "";
    strLen    = 0;      // to make Prefast happy

    __try
    {
        if( textStrLen < 0 )
        {
            PCSTR pp = textStr;
            textStrLen = 0;
            while( *pp++ != '\0' ) textStrLen++;
        }
        strLen = textStrLen;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        outPtr = "<BadPtr>";
        strLen = 0;
    }

    if( strLen == 0 )
    {
        goto labelExit;
    }

    inpPtr = textStr;
    inpEnd = &textStr [strLen];

    outPtr = outBuf;
    outEnd = &outBuf [outBufCapacity-1];
    *outEnd = '\0';

    if( pCCS == NULL )
    {
        BID_InitCtrlCharSubstA( &defaultSubst );
        pCCS = &defaultSubst;
    }

    bCut = TRUE;
    while( outPtr < outEnd )
    {
        ch = *inpPtr;
        if( (UINT)ch >= (UINT)0x20 )
        {
            *outPtr = ch;   // Printable character
        }
        else if( _bidStrDigest_FilterA(ch, pCCS, outPtr) )
        {
            /* Control character processed; don't need anything else */;
        }
        else if( (UINT)(outPtr - outEnd) >= 3 )
        {
            *(++outPtr) = 'x';
            *(++outPtr) = _bidHexDigit( (int)ch >> 4 );
            *(++outPtr) = _bidHexDigit( (int)ch );
        }
        else
        {
            bCut = TRUE;    /* No Room for 'x00' */;
        }

        outPtr++;
        inpPtr++;

        if( --strLen == 0 )
        {
            bCut = FALSE;
            break;
        }
    }

    *outPtr = '\0';

    if( bCut )
    {
        outPtr -= 3;
        if( outPtr > outBuf ){
            outPtr[0] = '.';
            outPtr[1] = '.';
            outPtr[2] = '\0';
        }
    }

    outPtr = outBuf;

 labelExit:
    return outPtr;

} // xBidStrDigestExA


PCWSTR  WINAPI xBidStrDigestExW(__out_ecount(outBufCapacity) PWSTR outBuf,
                                __in int outBufCapacity,
                                __in PCWSTR textStr, __in int textStrLen,
                                __in_opt const BID_CtrlCharSubstW* pCCS)
{
    WCHAR   ch;
    BOOL    bCut;
    UINT    strLen;
    PCWSTR  inpPtr;
    PCWSTR  inpEnd;
    PWSTR   outEnd;
    PWSTR   outPtr;

    BID_CtrlCharSubstW  defaultSubst;

    if( outBufCapacity <= 0 || textStr == NULL )
    {
        outPtr = NULL;
        goto labelExit;
    }

    outBuf[0] = L'\0';
    outPtr    = L"";
    strLen    = 0;      // to make Prefast happy

    __try
    {
        if( textStrLen < 0 )
        {
            PCWSTR pp = textStr;
            textStrLen = 0;
            while( *pp++ != L'\0' ) textStrLen++;
        }
        strLen = textStrLen;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        outPtr = L"<BadPtr>";
        strLen = 0;
    }

    if( strLen == 0 )
    {
        outPtr = L"";
        goto labelExit;
    }

    inpPtr = textStr;
    inpEnd = &textStr [strLen];

    outPtr = outBuf;
    outEnd = &outBuf [outBufCapacity-1];
    *outEnd = L'\0';

    if( pCCS == NULL )
    {
        BID_InitCtrlCharSubstW( &defaultSubst );
        pCCS = &defaultSubst;
    }

    bCut = TRUE;
    while( outPtr < outEnd )
    {
        ch = *inpPtr;
        if( (UINT)ch >= (UINT)0x20 )
        {
            *outPtr = ch;   // Printable character
        }
        else if( _bidStrDigest_FilterW(ch, pCCS, outPtr) )
        {
            /* Control character processed; don't need anything else */;
        }
        else if( (UINT)(outPtr - outEnd) >= 3 )
        {
            *(++outPtr) = L'x';
            *(++outPtr) = _bidHexDigit( (int)ch >> 4 );
            *(++outPtr) = _bidHexDigit( (int)ch );
        }
        else
        {
            bCut = TRUE;    /* No Room for L'x00' */;
        }

        outPtr++;
        inpPtr++;

        if( --strLen == 0 )
        {
            bCut = FALSE;
            break;
        }
    }

    *outPtr = L'\0';

    if( bCut )
    {
        outPtr -= 3;
        if( outPtr > outBuf ){
            outPtr[0] = L'.';
            outPtr[1] = L'.';
            outPtr[2] = L'\0';
        }
    }

    outPtr = outBuf;

 labelExit:
    return outPtr;

} // xBidStrDigestExW


#endif // _BID_NO_STRDIGEST


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Outline Implementation for the service functions
//
#if defined( _BID_NO_INLINE )

_bid_INLINE DWORD WINAPI BidGetApiGroupBits(DWORD mask)
{
    return _bidGblFlags & mask;
}

_bid_INLINE DWORD WINAPI _bidSetGetCtlBitsEx(INT_PTR cmdSpaceID, int cmd, DWORD mask, DWORD bits)
{
    return (DWORD) xBidCtlProcEx(cmdSpaceID, cmd, mask, bits, 0);
}

_bid_INLINE int WINAPI xBidGetNumOfCmdSpaces(void)
{
    return (int)xBidCtlProc(BID_DCSCMD_CMDSPACE_COUNT, 0, 0, 0);
}

_bid_INLINE int WINAPI xBidGetCmdSpaceNameA(__in int idx,
                                            __out_ecount(strBufCapacity) PSTR strBuf,
                                            __in int strBufCapacity)
{
    return (int)xBidCtlProc(BID_DCSCMD_CMDSPACE_ENUM, idx, strBuf, strBufCapacity);
}
_bid_INLINE INT_PTR WINAPI xBidGetCmdSpaceIDA(__in PCSTR textID)
{
    return xBidCtlProc(BID_DCSCMD_CMDSPACE_QUERY, 0, textID, 0);
}

_bid_INLINE void WINAPI xBidGetEventIDA(INT_PTR* pEvtID, DWORD flags, PCSTR textID)
{
    xBidCtlProc(BID_DCSCMD_GET_EVENT_ID, flags, textID, pEvtID);
}
_bid_INLINE void WINAPI xBidGetEventIDW(INT_PTR* pEvtID, DWORD flags, PCWSTR textID)
{
    xBidCtlProc(BID_DCSCMD_GET_EVENT_ID+BID_CMD_UNICODE, flags, textID, pEvtID);
}
_bid_INLINE void WINAPI xBidRemoveEvent(INT_PTR* pEvtID, DWORD flags)
{
    xBidCtlProc(BID_DCSCMD_GET_EVENT_ID+BID_CMD_REVERSE, flags, 0, pEvtID);
}

_bid_INLINE void WINAPI xBidAddResHandle(DWORD flags, HMODULE hRes)
{
    xBidCtlProc(BID_DCSCMD_ADD_RESHANDLE, flags, hRes, 0);
}
_bid_INLINE void WINAPI xBidRemoveResHandle(DWORD flags, HMODULE hRes)
{
    xBidCtlProc(BID_DCSCMD_ADD_RESHANDLE+BID_CMD_REVERSE, flags, hRes, 0);
}

_bid_INLINE PCSTR WINAPI xBidParseStringA(DWORD flags, PCSTR tcfs)
{
    return (PCSTR) xBidCtlProc(BID_DCSCMD_PARSE_STRING, flags, tcfs, 0);
}
_bid_INLINE PCWSTR WINAPI xBidParseStringW(DWORD flags, PCWSTR tcfs)
{
    return (PCWSTR) xBidCtlProc(BID_DCSCMD_PARSE_STRING+BID_CMD_UNICODE, flags, tcfs, 0);
}

_bid_INLINE void WINAPI xBidAddExtensionA(PCSTR  textID, BIDEXTPROC extProc, INT_PTR dataEx)
{
    xBidCtlProc(BID_DCSCMD_ADD_EXTENSION, dataEx, textID, extProc);
}
_bid_INLINE void WINAPI xBidAddExtensionW(PCWSTR textID, BIDEXTPROC extProc, INT_PTR dataEx)
{
    xBidCtlProc(BID_DCSCMD_ADD_EXTENSION+BID_CMD_UNICODE, dataEx, textID, extProc);
}

_bid_INLINE void WINAPI xBidAddMetaTextA(PCSTR metaStr)
{
    xBidCtlProc(BID_DCSCMD_ADD_METATEXT, 0, metaStr, NULL);
}
_bid_INLINE void WINAPI xBidAddMetaTextW(PCWSTR metaStr)
{
    xBidCtlProc(BID_DCSCMD_ADD_METATEXT+BID_CMD_UNICODE, 0, metaStr, NULL);
}

_bid_INLINE void WINAPI BidShutdown(INT_PTR arg)
{
    xBidCtlProc(BID_DCSCMD_SHUTDOWN, arg, 0,0);
}


#endif // _BID_NO_INLINE


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Basic Code Address Validation
//
#ifndef _BID_STATIC_BINDING

static BOOL _bidIsBadCodeAddress(FARPROC codePtr)
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD   protectMask;
    BOOL    bOk;

    mbi.State   = 0;
    mbi.Protect = 0;
    protectMask = PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY;

   #ifndef _BID_UNICODE_LOADER
    if( (BID_GetVersion() & 0x80000000) != 0 )
    {
        //
        //  Windows 9x
        //
        protectMask |= PAGE_READONLY;
    }
   #endif

    bOk = (BID_VirtualQuery( (PCVOID)(UINT_PTR)codePtr, &mbi, sizeof(mbi) ) == sizeof(mbi));

    return !bOk  ||  (mbi.State & MEM_COMMIT) == 0  ||  (mbi.Protect & protectMask) == 0;
}

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Default implementation - NoOps
//
#if !defined( _BID_EXTERNAL_DEFAULTS )

BOOL WINAPI ImplBidPutStrA(HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  str)
{
    hID; src; info; str;
    return TRUE;
}
BOOL WINAPI ImplBidPutStrW(HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR str)
{
    hID; src; info; str;
    return TRUE;
}

BOOL WINAPI ImplBidTraceVA(HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  fmt, va_list args)
{
    hID; src; info; fmt; args;
    return TRUE;
}
BOOL WINAPI ImplBidTraceVW(HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR fmt, va_list args)
{
    hID; src; info; fmt; args;
    return TRUE;
}

BOOL WINAPI ImplBidScopeEnterVA(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp,
                                PCSTR stf, va_list va)
{   hID; src; info; stf; va;
    *pHScp = NULL;
    return TRUE;
}
BOOL WINAPI ImplBidScopeEnterVW(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp,
                                PCWSTR stf, va_list va)
{   hID; src; info; stf; va;
    *pHScp = NULL;
    return TRUE;
}
BOOL WINAPI ImplBidScopeLeave(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp)
{
    hID; src; info;
    *pHScp = BID_NOHANDLE;
    return TRUE;
}

BOOL WINAPI ImplBidEnabledA(HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR tcs)
{
    hID; src; info;
    return (tcs != NULL);
}
BOOL WINAPI ImplBidEnabledW(HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR tcs)
{
    hID; src; info;
    return (tcs != NULL);
}

INT_PTR WINAPI ImplBidSnap( HANDLE hID, INT_PTR evtID, INT_PTR arg1, INT_PTR arg2 )
{
    hID; evtID; arg1; arg2;
    return 0;
}

int  WINAPI ImplBidIndent(HANDLE hID, int nIdx)
{
    hID; nIdx;
    return 0;
}

INT_PTR WINAPI ImplBidTouch( HANDLE hID, UINT_PTR scope, UINT code, INT_PTR arg1, INT_PTR arg2 )
{
    hID; scope; code; arg1; arg2;
    return 0;
}


INT_PTR WINAPI ImplBidCtlProc(HANDLE hID, INT_PTR cmdSpaceID, int cmd,
                                INT_PTR a1, INT_PTR a2, INT_PTR a3)
{
    hID; cmdSpaceID; a1; a2;
    if( (cmd & ~BID_CMD_UNICODE) == BID_DCSCMD_GET_EVENT_ID ){
        **((INT_PTR**)&a3) = 0;
    }
    return 0;
}

BOOL WINAPI ImplBidEntryPoint ( HANDLE* pID, int bInitAndVer, PCSTR sIdentity,
                                DWORD cfgBits, DWORD* pGblFlags, BID_CTLCALLBACK fAddr,
                                PBIDEXTINFO pExtInfo, PBIDHOOKS pHooks, PBIDSECTHDR pHdr )
{
    bInitAndVer; sIdentity; cfgBits; fAddr; pExtInfo; pHooks; pHdr;
    *pID        = NULL;
    *pGblFlags  = 0;
    return TRUE;
}


  #if !defined( BID_DefaultAssertA ) || defined( _BID_NO_DEFAULT_ASSERT )

    BOOL WINAPI ImplBidAssert(HANDLE hID, UINT_PTR arg, UINT_PTR info)
    {
        hID; arg; info;
        return FALSE;
    }

  #else

    BOOL WINAPI ImplBidAssert(HANDLE hID, UINT_PTR arg, UINT_PTR info)
    {
        #define _bid_UNSUPPORTED    "<Unsupported Format>"
        #define _bid_STRIPPED       "<Stripped Text>"
        #define _bid_UNAVAILABLE    "<Not Available>"

        PCSTR expr    = NULL;
        PCSTR srcFile = _bid_UNAVAILABLE;
        UINT  srcLine = BID_SrcLine(info);

        hID;    // unused

        if( BID_IsAssertArgText(arg) )
        {
            expr = BID_IsAssertTextA(info) ? BID_AssertTextA(arg) : _bid_UNSUPPORTED;
        }
        else if( BID_IsAssertArgStruct(arg) )
        {
            PBIDASSERTARG pArg = BID_AssertStruct(arg);

            if( !BID_IsAssertSrcFilePtr(pArg) )
            {
                srcFile = _bid_STRIPPED;
            }
            else if( !BID_IsAssertSrcFileA(info) )
            {
                srcFile = _bid_UNSUPPORTED;
            }
            else
            {
                srcFile = BID_AssertSrcFileA(pArg);
            }

            if( !BID_IsAssertStructTextPtr(pArg) )
            {
                expr = _bid_STRIPPED;
            }
            else if( !BID_IsAssertTextA(info) )
            {
                expr = _bid_UNSUPPORTED;
            }
            else
            {
                expr = BID_AssertStructTextA(pArg);
            }
        }
        else
        {
            expr = _bid_STRIPPED;
        }

        //
        //  Make sure BID_DefaultAssertA corresponds to a function with the following prototype:
        //
        //  BOOL Func(const char* expression, const char* fileName, unsigned lineNumber);
        //
        //  where return value FALSE means: request debug break, and TRUE: continue execution.
        //
        return BID_DefaultAssertA(expr, srcFile, srcLine);

        #undef _bid_UNSUPPORTED
        #undef _bid_STRIPPED
        #undef _bid_UNAVAILABLE

    } // ImplBidAssert

  #endif // BID_DefaultAssertA

static BidCtlProc_t  _bidStubCP = ImplBidCtlProc;

#else

static BidCtlProc_t  _bidStubCP = (BidCtlProc_t)(UINT_PTR)(~0);

#endif // _BID_EXTERNAL_DEFAULTS


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  To call the function from a debugger (make sure that private symbols are used):
//  WinDbg:
//      0:000> .call xBidFlush()
//      0:000> g
//      0:000>
//
//  Visual Studio:
//      Watch window:    'xBidFlush()'
//      Command window: > ?xBidFlush()
//
#if _MSC_VER >= 1300
__declspec(noinline)
#endif
int WINAPI xBidFlush()
{
    return (int)xBidCtlProc(BID_DCSCMD_FLUSH_BUFFERS, 0, 0, 0);
}

#if !defined(_X86_)
  #pragma comment(linker, "/include:xBidFlush")
#else
  #pragma comment(linker, "/include:_xBidFlush@0")
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                    DYNAMIC LOADER STUFF                                     //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Helpers
//
#define _BID_APIENTRY(name)  if(_bidPfn.##name == stb##name) _bidPfn.##name = Impl##name;

static void _bidReplaceStubs( void )
{
    BID_LIST_API_ENTRIES
}

#undef  _BID_APIENTRY
#define _BID_APIENTRY(name)  _bidPfn.##name = Impl##name;

static void WINAPI _bidUnloadCallback( BOOL bDisableFutureCalls )
{
    if( bDisableFutureCalls )
    {
        _bidGblFlags = 0;
        _bidID       = BID_NOHANDLE;
    }
    BID_LIST_API_ENTRIES
}

#undef  _BID_APIENTRY


static void _bidLoad( HANDLE* pID )
{
    if( _bidInitialized != 0 ) return;

    _bidCallEntryPoint( BID_VER, 0 );

    if( (*pID = _bidID) == BID_NOHANDLE )
    {
        _bidUnloadCallback( TRUE );
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  API functions
//
#ifndef _BID_LOAD_REDEFINE

    int __cdecl BidLoad( void )
    {
        _bidLoad( &_bidID );
        return 0;
    }

#else

  #ifndef _NO_COMPILE_NOTES
  #pragma message("NOTE: BidLoad defined externally.")
  #endif

#endif


#ifndef _BID_UNLOAD_REDEFINE

    int __cdecl BidUnload( void )
    {
        if( !_bidIsUnloaded() )
        {
            if( _bidID != BID_NOHANDLE )
            {
                _bidCallEntryPoint( 0, 0 );
            }

            _bidUnloadCallback( TRUE );
            _bidUnloadEntryPoint();
            _bidUnloadingDone();
        }
        return 0;
    }

#else

  #ifndef _NO_COMPILE_NOTES
  #pragma message("NOTE: BidUnload defined externally.")
  #endif

#endif


BOOL WINAPI BidNotAvailable( void )
{
    return (_bidPfn.BidCtlProc == _bidStubCP);
}

BOOL WINAPI BidNotLoaded( void )
{
    return (_bidPfn.BidCtlProc == ImplBidCtlProc);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DLL INITIALIZATION
//
#if !defined( _BID_STATIC_BINDING )

    static  HMODULE _bidhDll = NULL;

    static void _bidUnloadEntryPoint( void )
    {
        _bidpEntryPoint = ImplBidEntryPoint;
        if( _bidhDll != NULL )
        {
            BID_FreeLibrary( _bidhDll );
            _bidhDll = NULL;
        }
    }

    static void _bidLoadEntryPoint( void )
    {
        //
        //  Make sure that the _BID_EXTERNAL_DLL_NAME symbol is defined using BID_T() macro
        //  or explicitly: as UNICODE string if _BID_UNICODE_LOADER is defined,
        //  and as ANSI string otherwise.
        //
        BID_PCTSTR sDllName = _BID_EXTERNAL_DLL_NAME;

        BOOL    bOk2LoadDll = _BID_EXTERNAL_DLL_LOOKUP;

       #ifdef _DEBUG_TRACE_ON
        if( _bidhDll != NULL )
        {
            BID_OutputDebugString( BID_T("\n*** \"") );
            BID_OutputDebugString( sDllName );
            BID_OutputDebugString( BID_T("\" already connected.\n\n") );
        }
       #endif

        if( bOk2LoadDll )
        {
            //UINT save = BID_SetErrorMode( SEM_NOOPENFILEERRORBOX );
            _bidhDll = BID_LoadLibrary( sDllName );
            //BID_SetErrorMode( save );
        }
        if( _bidhDll == NULL )
        {
           #ifdef _DEBUG_TRACE_ON
            if( bOk2LoadDll )
            {
                BID_OutputDebugString( BID_T("\n*** \"") );
                BID_OutputDebugString( sDllName );
                BID_OutputDebugString( BID_T("\" is not available.\n\n") );
            }
           #endif

            _bidpEntryPoint = ImplBidEntryPoint;
        }
        else
        {
            _bidpEntryPoint = (BidEntryPoint_t)BID_GetProcAddress( _bidhDll, "DllBidEntryPoint" );

            if( _bidpEntryPoint == NULL )
            {
               #ifdef _DEBUG_TRACE_ON
                BID_TCHAR Buf [_MAX_PATH];

                if( 0 == BID_GetModuleFileName(_bidhDll, Buf, _MAX_PATH - 1) )
                {
                    Buf [0] = BID_T('\0');
                }
                else
                {
                    Buf [_MAX_PATH - 1] = BID_T('\0');
                }

                BID_OutputDebugString( BID_T("\n*** \"") );
                BID_OutputDebugString( Buf );
                BID_OutputDebugString( BID_T("\" declined.\n\n") );
               #endif

                _bidUnloadEntryPoint();
            }
        }

    } // _bidLoadEntryPoint


#endif  // _BID_STATIC_BINDING


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Build Configuration.
//
//  NOTE: A component can be built from object files and/or libraries that can potentially be
//  compiled with different preprocessor symbols defined. In that case the config bits may lie.
//
//  






#ifdef _BID_STUB_HIDDEN
  #define _bidCFG_SYS   _bidCFG_SUM | BID_CFG_HIDDEN
#else
  #define _bidCFG_SYS   _bidCFG_SUM
#endif

DWORD _bidCfgBits = _bidCFG_SYS | _bidCFG_USER;

//
//  _BID_CFG_USER bits must be defined in BID_CFG_MASK_USER range
//
DASSERT_COMPILER( 0 == (_bidCFG_USER & ~(BID_CFG_MASK_USER|BID_CFG_ACTIVE_USER)) );

#undef  _bidCFG_USER
#undef  _bidCFG_SUM
#undef  _bidCFG_SYS

#undef  _bidCFG_UTF8
#undef  _bidCFG_DBGBRK
#undef  _bidCFG_DBGTRC
#undef  _bidCFG_NOALCK
#undef  _bidCFG_NOSRC
#undef  _bidCFG_PKSRC
#undef  _bidCFG_PKLFS
#undef  _bidCFG_PKTCFS
#undef  _bidCFG_PKSTF
#undef  _bidCFG_PKEDS
#undef  _bidCFG_PKEXAR
#undef  _bidCFG_PKMETATEXT
#undef  _bidCFG_PKAAD
#undef  _bidCFG_PKXFS


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DIRECT ACCESS TO SPECIALLY ALLOCATED DATA
//
typedef struct __bidNoBlk
{
    BIDLBL  x1;         //
    PCVOID  x3;         //  Empty stub for _BID_NO_SPECIAL_ALLOCATION mode.
} BIDNOBLK;             //

#define _bidStbV(val)   (PCVOID)(INT_PTR)(val)
#define _bidStbX(val)   {_bidStbV(val), 0}

#ifdef _BID_NO_SRCINFO

  static  BIDNOBLK      _bidA010 = { _bidStbX(0xFF8AFFBD), _bidStbV(0xFF93FF96) };
  #define _bidA011      _bidA010.x1
  #define _bidA013      _bidA010.x3

  static  BIDNOBLK      _bidW010 = { _bidStbX(0xFFD2FF8B), _bidStbV(0xFF91FFB6) };
  #define _bidW011      _bidW010.x1
  #define _bidW013      _bidW010.x3

#else

  #ifdef _WIN64
    #pragma section("BID$A011",long,read,write)
    #pragma section("BID$A013",long,read,write)
    #pragma section("BID$W011",long,read,write)
    #pragma section("BID$W013",long,read,write)
  #else
    #pragma data_seg("BID$A011")
    #pragma data_seg("BID$A013")
    #pragma data_seg("BID$W011")
    #pragma data_seg("BID$W013")
    #pragma data_seg()
  #endif

  __declspec(allocate("BID$A011")) BIDLBL _bidA011 = _bidStbX(0xFF8AFFBD);
  __declspec(allocate("BID$A013")) PCVOID _bidA013 = _bidStbV(0xFF93FF96);
  __declspec(allocate("BID$W011")) BIDLBL _bidW011 = _bidStbX(0xFFD2FF8B);
  __declspec(allocate("BID$W013")) PCVOID _bidW013 = _bidStbV(0xFF91FFB6);

#endif

#ifdef _BID_NO_SPECIAL_ALLOCATION

  static  BIDNOBLK      _bid000 = { _bidStbX(~0), _bidStbV(~0) };

  #define _bidA021      _bid000.x1
  #define _bidA023      _bid000.x3
  #define _bidW021      _bid000.x1
  #define _bidW023      _bid000.x3

  #define _bidA031      _bid000.x1
  #define _bidA033      _bid000.x3
  #define _bidW031      _bid000.x1
  #define _bidW033      _bid000.x3

  #define _bidA041      _bid000.x1
  #define _bidA043      _bid000.x3
  #define _bidW041      _bid000.x1
  #define _bidW043      _bid000.x3

  #define _bidA051      _bid000.x1
  #define _bidA053      _bid000.x3
  #define _bidW051      _bid000.x1
  #define _bidW053      _bid000.x3

  #define _bidA061      _bid000.x1
  #define _bidA063      _bid000.x3
  #define _bidW061      _bid000.x1
  #define _bidW063      _bid000.x3

  #define _bidA071      _bid000.x1
  #define _bidA073      _bid000.x3
  #define _bidW071      _bid000.x1
  #define _bidW073      _bid000.x3

  #define _bidA081      _bid000.x1
  #define _bidA083      _bid000.x3
  #define _bidW081      _bid000.x1
  #define _bidW083      _bid000.x3

  #define _bidA091      _bid000.x1
  #define _bidA093      _bid000.x3
  #define _bidW091      _bid000.x1
  #define _bidW093      _bid000.x3

  #define _bidA101      _bid000.x1
  #define _bidA103      _bid000.x3
  #define _bidW101      _bid000.x1
  #define _bidW103      _bid000.x3

#else

  #ifdef _WIN64

    #pragma section("BIDL$A021",long,read,write)
    #pragma section("BIDL$A023",long,read,write)
    #pragma section("BIDL$W021",long,read,write)
    #pragma section("BIDL$W023",long,read,write)

    #pragma section("BID$A031",long,read,write)
    #pragma section("BID$A033",long,read,write)
    #pragma section("BID$W031",long,read,write)
    #pragma section("BID$W033",long,read,write)

    #pragma section("BID$A041",long,read,write)
    #pragma section("BID$A043",long,read,write)
    #pragma section("BID$W041",long,read,write)
    #pragma section("BID$W043",long,read,write)

    #pragma section("BID$A051",long,read,write)
    #pragma section("BID$A053",long,read,write)
    #pragma section("BID$W051",long,read,write)
    #pragma section("BID$W053",long,read,write)

    #pragma section("BID$A061",long,read,write)
    #pragma section("BID$A063",long,read,write)
    #pragma section("BID$W061",long,read,write)
    #pragma section("BID$W063",long,read,write)

    #pragma section("BID$A071",long,read,write)
    #pragma section("BID$A073",long,read,write)
    #pragma section("BID$W071",long,read,write)
    #pragma section("BID$W073",long,read,write)

    #pragma section("BID$A081",long,read,write)
    #pragma section("BID$A083",long,read,write)
    #pragma section("BID$W081",long,read,write)
    #pragma section("BID$W083",long,read,write)

    #pragma section("BID$A091",long,read,write)
    #pragma section("BID$A093",long,read,write)
    #pragma section("BID$W091",long,read,write)
    #pragma section("BID$W093",long,read,write)

    #pragma section("BID$A101",long,read,write)
    #pragma section("BID$A103",long,read,write)
    #pragma section("BID$W101",long,read,write)
    #pragma section("BID$W103",long,read,write)

  #else

    #pragma data_seg("BIDL$A021")
    #pragma data_seg("BIDL$A023")
    #pragma data_seg("BIDL$W021")
    #pragma data_seg("BIDL$W023")

    #pragma data_seg("BID$A031")
    #pragma data_seg("BID$A033")
    #pragma data_seg("BID$W031")
    #pragma data_seg("BID$W033")

    #pragma data_seg("BID$A041")
    #pragma data_seg("BID$A043")
    #pragma data_seg("BID$W041")
    #pragma data_seg("BID$W043")

    #pragma data_seg("BID$A051")
    #pragma data_seg("BID$A053")
    #pragma data_seg("BID$W051")
    #pragma data_seg("BID$W053")

    #pragma data_seg("BID$A061")
    #pragma data_seg("BID$A063")
    #pragma data_seg("BID$W061")
    #pragma data_seg("BID$W063")

    #pragma data_seg("BID$A071")
    #pragma data_seg("BID$A073")
    #pragma data_seg("BID$W071")
    #pragma data_seg("BID$W073")

    #pragma data_seg("BID$A081")
    #pragma data_seg("BID$A083")
    #pragma data_seg("BID$W081")
    #pragma data_seg("BID$W083")

    #pragma data_seg("BID$A091")
    #pragma data_seg("BID$A093")
    #pragma data_seg("BID$W091")
    #pragma data_seg("BID$W093")

    #pragma data_seg("BID$A101")
    #pragma data_seg("BID$A103")
    #pragma data_seg("BID$W101")
    #pragma data_seg("BID$W103")

    #pragma data_seg()

  #endif

  __declspec(allocate("BIDL$A021")) BIDLBL _bidA021 = _bidStbX(0xFFBBFFDF);
  __declspec(allocate("BIDL$A023")) PCVOID _bidA023 = _bidStbV(0xFF9EFF96);
  __declspec(allocate("BIDL$W021")) BIDLBL _bidW021 = _bidStbX(0xFF91FF98);
  __declspec(allocate("BIDL$W023")) PCVOID _bidW023 = _bidStbV(0xFF8CFF90);

  __declspec(allocate("BID$A031")) BIDLBL _bidA031 = _bidStbX(0xFF96FF8B);
  __declspec(allocate("BID$A033")) PCVOID _bidA033 = _bidStbV(0xFF8CFF9C);
  __declspec(allocate("BID$W031")) BIDLBL _bidW031 = _bidStbX(0xFFB6FFDF);
  __declspec(allocate("BID$W033")) PCVOID _bidW033 = _bidStbV(0xFF8BFF91);

  __declspec(allocate("BID$A041")) BIDLBL _bidA041 = _bidStbX(0xFF8DFF9A);
  __declspec(allocate("BID$A043")) PCVOID _bidA043 = _bidStbV(0xFF9EFF99);
  __declspec(allocate("BID$W041")) BIDLBL _bidW041 = _bidStbX(0xFF9AFF9C);
  __declspec(allocate("BID$W043")) PCVOID _bidW043 = _bidStbV(0xFFDFFFD1);

  __declspec(allocate("BID$A051")) BIDLBL _bidA051 = _bidStbX(0xFF90FFBC);
  __declspec(allocate("BID$A053")) PCVOID _bidA053 = _bidStbV(0xFF86FF8F);
  __declspec(allocate("BID$W051")) BIDLBL _bidW051 = _bidStbX(0xFF96FF8D);
  __declspec(allocate("BID$W053")) PCVOID _bidW053 = _bidStbV(0xFF97FF98);

  __declspec(allocate("BID$A061")) BIDLBL _bidA061 = _bidStbX(0xFFDFFF8B);
  __declspec(allocate("BID$A063")) PCVOID _bidA063 = _bidStbV(0xFFDFFF56);
  __declspec(allocate("BID$W061")) BIDLBL _bidW061 = _bidStbX(0xFF96FFB2);
  __declspec(allocate("BID$W063")) PCVOID _bidW063 = _bidStbV(0xFF8DFF9C);

  __declspec(allocate("BID$A071")) BIDLBL _bidA071 = _bidStbX(0xFF8CFF90);
  __declspec(allocate("BID$A073")) PCVOID _bidA073 = _bidStbV(0xFF99FF90);
  __declspec(allocate("BID$W071")) BIDLBL _bidW071 = _bidStbX(0xFFDFFF8B);
  __declspec(allocate("BID$W073")) PCVOID _bidW073 = _bidStbV(0xFF90FFBC);

  __declspec(allocate("BID$A081")) BIDLBL _bidA081 = _bidStbX(0xFF8FFF8D);
  __declspec(allocate("BID$A083")) PCVOID _bidA083 = _bidStbV(0xFF8DFF90);
  __declspec(allocate("BID$W081")) BIDLBL _bidW081 = _bidStbX(0xFF8BFF9E);
  __declspec(allocate("BID$W083")) PCVOID _bidW083 = _bidStbV(0xFF90FF96);

  __declspec(allocate("BID$A091")) BIDLBL _bidA091 = _bidStbX(0xFFD1FF91);
  __declspec(allocate("BID$A093")) PCVOID _bidA093 = _bidStbV(0xFFB4FFDF);
  __declspec(allocate("BID$W091")) BIDLBL _bidW091 = _bidStbX(0xFFCBFFB6);
  __declspec(allocate("BID$W093")) PCVOID _bidW093 = _bidStbV(0xFFCEFFCF);

  __declspec(allocate("BID$A101")) BIDLBL _bidA101 = _bidStbX(0xFFBDFFC9);
  __declspec(allocate("BID$A103")) PCVOID _bidA103 = _bidStbV(0xFFCCFFCA);
  __declspec(allocate("BID$W101")) BIDLBL _bidW101 = _bidStbX(0xFFCAFFC9);
  __declspec(allocate("BID$W103")) PCVOID _bidW103 = _bidStbV(0xFFC7FFBB);

#endif

#undef  _bidStbV


/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN64
  #pragma section("BID$A000",long,read,write)
  #pragma section("BID$A001",long,read,write)
  #pragma section("BID$A002",long,read,write)
#else
  #pragma data_seg("BID$A000")
  #pragma data_seg("BID$A001")
  #pragma data_seg("BID$A002")
  #pragma data_seg()
#endif

__declspec(allocate("BID$A002"))
BIDMARKER _bidMarkers [BID_SE_COUNT] =
{
    { &_bidA011, &_bidA013 },   { &_bidW011, &_bidW013 },   // SrcInfo
    { &_bidA021, &_bidA023 },   { &_bidW021, &_bidW023 },   // ()
    { &_bidA031, &_bidA033 },   { &_bidW031, &_bidW033 },   // TCFS
    { &_bidA041, &_bidA043 },   { &_bidW041, &_bidW043 },   // STF
    { &_bidA051, &_bidA053 },   { &_bidW051, &_bidW053 },   // EDS
    { &_bidA061, &_bidA063 },   { &_bidW061, &_bidW063 },   // EXAR
    { &_bidA071, &_bidA073 },   { &_bidW071, &_bidW073 },   // EAR
    { &_bidA081, &_bidA083 },   { &_bidW081, &_bidW083 },   // METATEXT
    { &_bidA091, &_bidA093 },   { &_bidW091, &_bidW093 },   // AAD
    { &_bidA101, &_bidA103 },   { &_bidW101, &_bidW103 }    // XFS
};

__declspec(allocate("BID$A002"))
static const char _bidIdentity[] = _BID_IDENTITY_A;


//
//  Api Version Tag and DataHeader Size Tag.
//  Keep synchronized with BID_HdrAttrVersion() and BID_HdrAttrSize() in BidImplApi.h
//
#define _bidVerTag              (BID_VER << 16)
#define _bidSizeTag             ((sizeof(BIDSECTHDR) / sizeof(DWORD)) << 8)
#define _bidAttributes          (_bidVerTag | _bidSizeTag | BID_SE_COUNT)

DASSERT_COMPILER( (sizeof(BIDSECTHDR) / sizeof(DWORD)) <= 0xFF );
DASSERT_COMPILER( (sizeof(BIDSECTHDR) % sizeof(DWORD)) == 0 );
DASSERT_COMPILER( BID_SE_COUNT <= 0xFF );
DASSERT_COMPILER( BID_VER <= 0xFFFF );

__declspec(allocate("BID$A000"))
BIDSECTHDR _bidDataHeader =
{
    BID_HEADER_SIGNATURE,       //  Signature (char[8]);
    _bidMarkers,                //  Marker;
    _bidIdentity,               //  Identity;
    &_bidCfgBits,               //  pCfgBits;
    _bidAttributes,             //  Attributes;
    0,                          //  Reserved;
    0, 0, 0,                    //  Reserved;
    BID_SANITY_CHECK            //  SanityCheck;
};

#undef _bidVerTag
#undef _bidSizeTag
#undef _bidAttributes

//
//  BID DataHeader block EndMarker
//
#ifdef __cplusplus
extern "C"
#endif
__declspec(allocate("BID$A001"))
struct __bidDataBlockEnd
{
    char  BlockEnd [8];
}
_bidDataBlockEnd =
{
    BID_HEADER_BLOCKEND
};
#if !defined(_X86_)
  #pragma comment(linker, "/include:_bidDataBlockEnd")
#else
  #pragma comment(linker, "/include:__bidDataBlockEnd")
#endif

#pragma data_seg()

#ifndef _BID_EXTERNAL_SECTION_CONFIG
  #pragma comment(linker, "/merge:BIDL=.data")
  #pragma comment(linker, "/merge:BID=.sdbid")
  #pragma comment(linker, "/section:.sdbid,D")
#endif

#ifndef _BID_NO_AUTO_LOAD
  #ifndef _BID_CRTSECT_DECLARED
    #ifdef _WIN64
      #pragma section(".CRT$XCBid",long,read)
    #elif defined(_ARM_)
      #pragma section(".CRT$XCBid",read)
    #else
      #pragma data_seg(".CRT$XCBid")
      #pragma data_seg()
    #endif
    #define _BID_CRTSECT_DECLARED
  #endif
  #ifdef __cplusplus
  extern "C"
  #endif
  #ifdef _BIDLDR_INCLUDED
    __declspec(allocate(".CRT$XCBid")) int (__cdecl* _bidLoadPtr)(void) = BidSelectAndLoad;
  #else
    __declspec(allocate(".CRT$XCBid")) int (__cdecl* _bidLoadPtr)(void) = BidLoad;
  #endif
  #if !defined(_X86_)
    #pragma comment(linker, "/include:_bidLoadPtr")
  #else
    #pragma comment(linker, "/include:__bidLoadPtr")
  #endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

static void _bidCallEntryPoint( int bInitAndVer, INT_PTR reserved1 )
{
    BIDEXTINFO          extInfo;
    BID_CTLCALLBACK     fAddr;
    DWORD               dwConfig;

    extInfo.hModule     = _BID_HMODULE;
    extInfo.DomainName  = (PCWSTR)reserved1;
    extInfo.Reserved2   = 0;
    extInfo.Reserved    = 0;
    extInfo.ModulePath  = NULL;
    extInfo.ModulePathA = NULL;
    extInfo.pBindCookie = &_bidPfn.BindCookie;

    fAddr    = (BID_CTLCALLBACK) _BID_MODULE_ADDRESS;
    dwConfig = *_bidDataHeader.pCfgBits;

    if( BidSetApiGroupBits == fAddr )
    {
        dwConfig |= BID_CFG_CTLCALLBACK;
    }

   #ifndef _BID_STATIC_BINDING
    if( !_bidIsBadCodeAddress( (FARPROC)_bidpEntryPoint) )
   #endif
    {
        (*_bidpEntryPoint)( &_bidID, bInitAndVer, _bidDataHeader.Identity, dwConfig,
                            &_bidGblFlags, fAddr, &extInfo, &_bidPfn, &_bidDataHeader );
    }

} // _bidCallEntryPoint


#endif //////////////////////////////////////////////////////////////////////////////////////////
//                                 End of file "BidApi_impl.h"                                 //
/////////////////////////////////////////////////////////////////////////////////////////////////
