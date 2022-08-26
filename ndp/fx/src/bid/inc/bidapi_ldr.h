/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File:       BidApi_ldr.h
//
//  Copyright:  (c) Microsoft Corporation
//
//  Contents:   Built-In Diagnostics Selector/Loader.
//
//  Comments:   To be included to the main source file, usually near the entry point function.
//
//              A DLL-container should export at least "DllBidEntryPoint" in its DEF file
//              to be able to propagate diagnostics functionality from the loaded implementation
//              to the other DLLs, so they won't have to do the same loader work over and over.
//
//              Last Modified: 15-May-2005
//
//              <owner current="true" primary="true">Microsoft</owner>
//
#ifndef __BIDAPI_LDR_H__ ////////////////////////////////////////////////////////////////////////
#define __BIDAPI_LDR_H__

#define _BIDLDR_INCLUDED

#ifndef _BIDLDR_DEFAULT_FLAGS
  #define _BIDLDR_DEFAULT_FLAGS     BID_APIGROUP_MASK_BID
#endif

#ifdef _BID_STATIC_BINDING
  #undef _BID_STATIC_BINDING

  #ifndef _NO_COMPILE_NOTES
  #pragma message("NOTE: BidApi_ldr.h used; _BID_STATIC_BINDING undefined.")
  #endif
#endif
#undef  _BID_EXTERNAL_DLL_NAME
#undef  _BID_EXTERNAL_DLL_LOOKUP


static  BID_TCHAR _bidBlankStr[]    = BID_T("");

//
//  The path to Implementation DLL is supposed to be loaded from the Registry.
//
static  BID_PTSTR _bidExtDllName    = _bidBlankStr;
static  BOOL      _bidExtDllAllowed = FALSE;

#define _BID_EXTERNAL_DLL_NAME      _bidExtDllName
#define _BID_EXTERNAL_DLL_LOOKUP    _bidExtDllAllowed
#define _BID_APIGROUP_INIT          0

//
//  INTERFACE IMPLEMENTATION IS INCLUDED HERE
//
#ifdef __BIDAPI_IMPL_H__
  #error  BidApi_Impl.h cannot be included before BidApi_Ldr.h
#endif
#include "BidApi_Impl.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Abstraction for WINAPI functions used in dynamic loader (Part III)
//
#ifndef _BID_EXTERNAL_WINAPI

  //
  //  From Kernel32.lib:
  //
  #ifndef BID_ExpandEnvironmentStrings
    #define BID_ExpandEnvironmentStrings _BID_WINAPI(ExpandEnvironmentStrings)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_ExpandEnvironmentStrings defined externally.")
    #endif
  #endif

  #ifndef BID_GetDriveType
    #define BID_GetDriveType            _BID_WINAPI(GetDriveType)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_GetDriveType defined externally.")
    #endif
  #endif

  #ifndef BID_GetFullPathName
    #define BID_GetFullPathName         _BID_WINAPI(GetFullPathName)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_GetFullPathName defined externally.")
    #endif
  #endif

  #ifndef BID_SearchPath
    #define BID_SearchPath              _BID_WINAPI(SearchPath)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_SearchPath defined externally.")
    #endif
  #endif

  #ifndef BID_lstrcpyn
    #define BID_lstrcpyn                _BID_WINAPI(lstrcpyn)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_lstrcpyn defined externally.")
    #endif
  #endif

  #ifndef BID_lstrlen
    #define BID_lstrlen                 _BID_WINAPI(lstrlen)
  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: BID_lstrlen defined externally.")
    #endif
  #endif

  //
  //  From AdvAPI32.lib:
  //
  #ifndef _BID_EXTERNAL_REGISTRY_API

    #define BID_RegOpenKeyEx            _BID_WINAPI(RegOpenKeyEx)
    #define BID_RegQueryValueEx         _BID_WINAPI(RegQueryValueEx)
    #define BID_RegCloseKey             RegCloseKey

    #pragma comment(lib, "advapi32.lib")

  #else
    #ifndef _NO_COMPILE_NOTES
    #pragma message("NOTE: _BID_EXTERNAL_REGISTRY_API; BID_RegXxxx defined externally.")
    #endif
  #endif

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  EXPORTS
//
#if !defined( _BIDIMPL_LDR_INCLUDED )

  #ifdef __cplusplus
    extern "C" {
  #endif

  #if defined( _BIDLDR_EXPORT_HOOKS )
    //
    //  REDIRECTION HOOKS
    //
    #define xx(BidXxx)      (*_bidPfn.##BidXxx)

    BOOL WINAPI DllBidPutStrA( HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR str )
    {
        return xx(BidPutStrA)( hID, src, info, str );
    }

    _bidAPI_EXPORT
    BOOL WINAPI DllBidPutStrW( HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR str )
    {
        return xx(BidPutStrW)( hID, src, info, str );
    }


    BOOL WINAPI DllBidTraceVA( HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR tcfs, va_list argptr )
    {
        return xx(BidTraceVA)( hID, src, info, tcfs, argptr );
    }

    BOOL WINAPI DllBidTraceVW( HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR tcfs, va_list argptr )
    {
        return xx(BidTraceVW)( hID, src, info, tcfs, argptr );
    }


    BOOL WINAPI DllBidScopeEnterVA( HANDLE hID, UINT_PTR src, UINT_PTR info,
                                    HANDLE* pHScp, PCSTR stf, va_list va )
    {
        return xx(BidScopeEnterVA)( hID, src, info, pHScp, stf, va );
    }

    BOOL WINAPI DllBidScopeEnterVW( HANDLE hID, UINT_PTR src, UINT_PTR info,
                                    HANDLE* pHScp, PCWSTR stf, va_list va )
    {
        return xx(BidScopeEnterVW)( hID, src, info, pHScp, stf, va );
    }

    _bidAPI_EXPORT
    BOOL WINAPI DllBidScopeLeave( HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp )
    {
        return xx(BidScopeLeave)( hID, src, info, pHScp );
    }


    BOOL WINAPI DllBidEnabledA( HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR tcs )
    {
        return xx(BidEnabledA)( hID, src, info, tcs );
    }

    _bidAPI_EXPORT
    BOOL WINAPI DllBidEnabledW( HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR tcs )
    {
        return xx(BidEnabledW)( hID, src, info, tcs );
    }

    _bidAPI_EXPORT
    int  WINAPI DllBidIndent( HANDLE hID, int nIndent )
    {
        return xx(BidIndent)( hID, nIndent );
    }


    _bidAPI_EXPORT
    INT_PTR WINAPI DllBidSnap( HANDLE hID, INT_PTR evtID, INT_PTR arg1, INT_PTR arg2 )
    {
        return xx(BidSnap)( hID, evtID, arg1, arg2 );
    }

    _bidAPI_EXPORT
    BOOL WINAPI DllBidAssert( HANDLE hID, UINT_PTR arg, UINT_PTR info )
    {
        return xx(BidAssert)( hID, arg, info );
    }

    _bidAPI_EXPORT
    INT_PTR WINAPI DllBidCtlProc( HANDLE hID, INT_PTR cmdSpaceID, int cmd,
                                  INT_PTR a1, INT_PTR a2, INT_PTR a3 )
    {
        return xx(BidCtlProc)( hID, cmdSpaceID, cmd, a1, a2, a3 );
    }

    _bidAPI_EXPORT
    INT_PTR WINAPI DllBidTouch( HANDLE hID, UINT_PTR scope, UINT code, INT_PTR arg1, INT_PTR arg2 )
    {
        return xx(BidTouch)( hID, scope, code, arg1, arg2 );
    }

    _bidAPI_EXPORT
    BOOL __cdecl DllBidScopeEnterCW( HANDLE hID, UINT_PTR src, UINT_PTR info,
                                     HANDLE* pHScp, PCWSTR stf, ... )
    {
        va_list argptr;
        va_start( argptr, stf );
        return  xx(BidScopeEnterVW)( hID, src, info, pHScp, stf, argptr );
    }

    _bidAPI_EXPORT
    BOOL __cdecl DllBidTraceCW( HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR tcfs, ... )
    {
        va_list argptr;
        va_start( argptr, tcfs );
        return xx(BidTraceVW)( hID, src, info, tcfs, argptr );
    }

  #endif  // !_BIDLDR_EXPORT_HOOKS

  ///////////////////////////////////////////////////////////////////////////////////////////////
  #ifndef _BIDLDR_ENTRYPOINT_DEFINED

    static BOOL WINAPI _bidEntryPointNullStub( HANDLE* pID, int bInitAndVer, PCSTR sIdentity,
                                DWORD cfgBits, DWORD* pGblFlags, BID_CTLCALLBACK fAddr,
                                PBIDEXTINFO pExtInfo, PBIDHOOKS pHooks, PBIDSECTHDR pHdr )
    {
        pID; bInitAndVer; sIdentity; cfgBits; pGblFlags; fAddr; pExtInfo; pHooks; pHdr;
        return FALSE;
    }


    _bidAPI_EXPORT
    BOOL WINAPI DllBidEntryPoint( HANDLE* pID, int bInitAndVer, PCSTR sIdentity,
                                DWORD cfgBits, DWORD* pGblFlags, BID_CTLCALLBACK fAddr,
                                PBIDEXTINFO pExtInfo, PBIDHOOKS pHooks, PBIDSECTHDR pHdr )
    {
        volatile static LONG busy = 0;

        //
        //  Registry key may contain path to the loader itself or cause more complex, indirect
        //  recursion. This is meaningless, but we must consider this to prevent recursive calls.
        //
        BOOL    bRet;
        LONG    tmp  = BID_InterlockedExchange( &busy, 1 );

        if( tmp != 0 || _bidIsBadCodeAddress( (FARPROC)_bidpEntryPoint) )
        {
            _bidpEntryPoint = _bidEntryPointNullStub;
            bRet = FALSE;
        }
        else
        {
            bRet = (*_bidpEntryPoint) ( pID, bInitAndVer, sIdentity, cfgBits, pGblFlags,
                                        fAddr, pExtInfo, pHooks, pHdr );
        }

        BID_InterlockedExchange( &busy, 0 );
        return bRet;

    } // DllBidEntryPoint

  #endif // _BIDLDR_ENTRYPOINT_DEFINED

  #ifdef __cplusplus
    }
  #endif

#endif // _BIDIMPL_LDR_INCLUDED


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
//                                    CONFIGURATION PROFILE                                    //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT:  The name of this registry key cannot be changed.
//
//  Almost every application runs components developed by different teams. While all teams most
//  likely have their preferred diagnostics facilities, it's critically important to be able to
//  connect all components, running together as a process, to a single subsystem of sysadmin's
//  choice.
//
//  If any component uses any version of the BID Interface, then it must query this key with
//  exactly same name. This is the "central point" where all these different modules get
//  together to consistently deliver their output to the single, process-wide subsystem.
//
#define _BIDLDR_REGKEY              BID_T("SOFTWARE\\Microsoft\\BidInterface\\Loader")
#define _BIDLDR_MSG                 BID_T(":LdrMsg")
#define _BIDLDR_PATH                BID_T(":Path")

#ifdef _DEBUG_TRACE_ON
  #ifndef _BIDLDR_ENABLE_DEFAULT
  #define _BIDLDR_ENABLE_DEFAULT    TRUE
  #endif
  #ifndef _BIDLDR_MSG_DEFAULT
  #define _BIDLDR_MSG_DEFAULT       TRUE
  #endif
#else
  #ifndef _BIDLDR_ENABLE_DEFAULT
  #define _BIDLDR_ENABLE_DEFAULT    FALSE
  #endif
  #ifndef _BIDLDR_MSG_DEFAULT
  #define _BIDLDR_MSG_DEFAULT       FALSE
  #endif
#endif

//
//  BID implementation dll usually uses the same loader for self-diagnostics.
//  In this case we just use different subkey under the BidInterface key.
//  Code that fills _bidImplLdrRegKey buffer is implemented in BidImplApi_ldr.h
//
#ifdef _BIDIMPL_LDR_INCLUDED
  #undef  _BIDLDR_REGKEY
  #define _BIDLDR_REGKEY            _bidImplLdrRegKey
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  A software component may define proprietary way to negotiate with implementation dll and
//  decline connection if the challenge wasn't properly responded.
//  The BidGetCmdSpaceID and BidCtlProc functions are supposed to be used in order to
//  establish private negotiation.
//
#ifndef BIDLDR_GetChallengeResponse

    static  BOOL  _bidLdrGetChallengeResponse(void)
    {
        return TRUE;    // By default there are no special restrictions
    }

    #define BIDLDR_GetChallengeResponse     _bidLdrGetChallengeResponse

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct __bidLdrArgs
{
    HKEY        hKey;
    BID_PCTSTR  KeyName;
    BID_PCTSTR  ValueName;
    BID_PTSTR   StrBuf;
    int         BufSize;
    BOOL        bEnabled;
    BOOL        bLdrMsg;

} _bidLdrARGS, * PBIDLDRARGS;


static BOOL _bidLdrReadStr( __inout PBIDLDRARGS pArgs )
{
    BID_PTSTR   strBuf;
    int         bufLen;
    LONG        lRet;
    DWORD       dwSize;
    DWORD       regType  = REG_NONE;

    DASSERT( pArgs != NULL );
    DASSERT( pArgs->ValueName != NULL );

    strBuf  = pArgs->StrBuf;
    bufLen  = pArgs->BufSize / sizeof(strBuf[0]);
    dwSize  = (DWORD)pArgs->BufSize;

    DASSERT( strBuf != NULL );
    DASSERT( bufLen > 1 );
    DASSERT( dwSize > sizeof(BID_TCHAR) );

    lRet = BID_RegQueryValueEx( pArgs->hKey, pArgs->ValueName,
                                NULL, &regType, (LPBYTE)strBuf, &dwSize );

    if( lRet == ERROR_SUCCESS )
    {
        if( dwSize > sizeof(BID_TCHAR) )
        {
            switch( regType )
            {
             case REG_EXPAND_SZ:
                {
                    BID_TCHAR tmpBuf [_MAX_PATH + 10];
                    DWORD     dwRet;

                    dwRet = BID_ExpandEnvironmentStrings( strBuf, tmpBuf, _countof(tmpBuf)-1 );
                    if( dwRet == 0 || dwRet >= _countof(tmpBuf) )
                    {
                        tmpBuf[0] = BID_T('\0');
                    }
                    BID_lstrcpyn( strBuf, tmpBuf, bufLen );
                }
                // Fall Thru..

             case REG_SZ:
                pArgs->bEnabled = (strBuf[0] != BID_T(':'));
                break;

             default:
                pArgs->bEnabled = FALSE;

                strBuf[0] = BID_T('\0');
                lRet      = ERROR_INVALID_DATA;

            } // switch
        }

        strBuf [bufLen-1] = BID_T('\0');
    }

    //
    //  Note that we DO NOT modify pArgs->bEnabled if (lRet != SUCCESS)
    //
    return (lRet == ERROR_SUCCESS);

} // _bidLdrReadStr


static BOOL _bidLdrIsPathLocal( __inout_ecount(pathCapacity) BID_PTSTR sPath,
                                __in int pathCapacity )
{
    UINT        drvType;
    BID_TCHAR   chTemp;

    if( BID_lstrlen(sPath) < 3 )        // cannot be valid path
    {
        return FALSE;
    }

    //
    //  Make sure that the dll comes from the local hard drive.
    //  If the filename was specified with no path or with relative path,
    //  then we get the full path, and after that do the verification.
    //
    //  The following IF statement assumes that:
    //  *  Relative path starts with period: "..\..\path" or ".\..\path";
    //  *  Filename without path cannot start with '\' or '/' or "D:"
    //
    if( (sPath[0] == BID_T('.'))  ||
        (sPath[0] != BID_T('\\')  &&  sPath[0] != BID_T('/') && sPath[1] != BID_T(':')) )
    {
        DWORD       dwLen;
        BID_PTSTR   dummyPtr;
        BID_TCHAR   tmpBuf [_MAX_PATH + 10];

        if( sPath[0] == BID_T('.') )    // Relative path
        {
            dwLen = BID_GetFullPathName( sPath, _countof(tmpBuf), tmpBuf, &dummyPtr );
        }
        else                            // Filename without path
        {
            dwLen = BID_SearchPath( NULL, sPath, NULL, _countof(tmpBuf), tmpBuf, &dummyPtr );
        }

        if( dwLen == 0 || dwLen >= _countof(tmpBuf) )
        {
            //
            //  Cannot resolve the path or result doesn't fit.
            //  We'll do nothing and simply deny connection.
            //
            return FALSE;
        }

        tmpBuf [_countof(tmpBuf)-1] = BID_T('\0');
        BID_lstrcpyn( sPath, tmpBuf, pathCapacity );
    }

    //
    //  Only standard form of local path is acceptable: "d:\path\DiagImpl.dll".
    //  Any other, even valid path will be rejected. So we check only first 3 characters.
    //
    chTemp    = sPath [3];
    sPath [3] = BID_T('\0');

    drvType   = BID_GetDriveType( sPath );
    sPath [3] = chTemp;

    return (drvType == DRIVE_FIXED);

} // _bidLdrIsPathLocal


static void _bidLdrReadProfile( __out_ecount(dllPathCapacity) BID_PTSTR sDllPath, __in int dllPathCapacity,
                                __out BOOL* pbEnabled, __out BOOL* pbLdrMsg )
{
    LONG            lRet;
    _bidLdrARGS     args;
    BID_TCHAR       modPath [_MAX_PATH + 10];

    static BID_TCHAR sNotFound[] = BID_T("<NotFound>");

    DASSERT( dllPathCapacity > 2 );

    args.hKey       = NULL;
    args.ValueName  = sNotFound;
    args.StrBuf     = sDllPath;
    args.BufSize    = (dllPathCapacity * sizeof(sDllPath[0])) - 2;  // size in bytes, not chars.
    args.bEnabled   = _BIDLDR_ENABLE_DEFAULT;
    args.bLdrMsg    = _BIDLDR_MSG_DEFAULT;
    args.KeyName    = _BIDLDR_REGKEY;


    lRet = BID_RegOpenKeyEx( HKEY_LOCAL_MACHINE, args.KeyName, 0, KEY_READ, &args.hKey );

    if( lRet != ERROR_SUCCESS )
    {
        BID_lstrcpyn( sDllPath, sNotFound, dllPathCapacity-1 );
        args.bEnabled = false;
    }
    else
    {
        DWORD       dwRet;

        //
        //  0)  Set internal flag that enables/disables simple messages from the loader itself.
        //
        DWORD   dwValue = 0;
        DWORD   dwSize  = sizeof(dwValue);
        DWORD   regType = REG_NONE;

        lRet = BID_RegQueryValueEx( args.hKey, _BIDLDR_MSG, NULL, &regType,
                                    (LPBYTE) &dwValue, &dwSize );
        if( lRet != ERROR_SUCCESS || regType != REG_DWORD )
        {
            args.bLdrMsg = _BIDLDR_MSG_DEFAULT;
        }
        else
        {
            args.bLdrMsg = (dwValue != 0);
        }

        //  1)  Try to read application-specific configuration.
        //      The name of the key's value is "d:\path\app.exe"
        //
        dwRet = BID_GetModuleFileName( NULL, modPath, _countof(modPath)-1 );

        if( dwRet > 0  &&  dwRet < _countof(modPath)-1 )
        {
            modPath [_countof(modPath)-1] = BID_T('\0');

            args.ValueName = modPath;
            if( !_bidLdrReadStr( &args ) )
            {
                //  2)  Try configuration that may be provided for entire directory.
                //      The name of the key's value is "d:\path\*"
                //
                BID_PTSTR   pTmp = modPath + BID_lstrlen( modPath );
                BOOL        bDir = FALSE;

                DASSERT( pTmp != NULL && *pTmp == BID_T('\0') );

                while( --pTmp >= modPath )
                {
                    if( *pTmp == BID_T('\\') || *pTmp == BID_T('/') )
                    {
                        //
                        //  Last separator found. Put '*' instead of filename
                        //  at the end of path.
                        //
                        ++pTmp;
                        if( pTmp < modPath + _countof(modPath)-1 )
                        {
                            *pTmp++ = BID_T('*');
                            *pTmp   = BID_T('\0');
                            bDir    = TRUE;
                        }
                        break;              // <<== EXIT WHILE
                    }
                } // while

                if( bDir )
                {
                    bDir = _bidLdrReadStr( &args );
                }

                //  3)  Try the default configuration.
                //      The name of the key's value is ":Path"
                //
                if( !bDir )
                {
                    args.ValueName = _BIDLDR_PATH;
                    if( !_bidLdrReadStr( &args ) )
                    {
                        //  4)  Default configuration wasn't found in the registry
                        args.ValueName = sNotFound;
                        BID_lstrcpyn( sDllPath, sNotFound, dllPathCapacity-1 );
                        args.bEnabled = false;
                    }
                }

            } // if

        } // if dwRet

        BID_RegCloseKey( args.hKey );
        args.hKey = NULL;

    } // if RegOpenKey

    sDllPath [dllPathCapacity-1] = BID_T('\0');

    if( args.bEnabled )
    {
        //
        //  Verify the path: dll must be on local non-removable disk.
        //
        args.bEnabled = _bidLdrIsPathLocal( sDllPath, dllPathCapacity );
    }

    if( args.bLdrMsg )
    {
        //
        //  If the loader's self-diagnostics is enabled, then we can see in DebugOutput which
        //  Registry Entry was taken. Very helpful when the main diagnostics wasn't
        //  loaded and we don't understand why.
        //
        BID_OutputDebugString( BID_T("*** [HKLM\\") );
        BID_OutputDebugString( args.KeyName );
        BID_OutputDebugString( BID_T("\\\"") );
        BID_OutputDebugString( args.ValueName );
        BID_OutputDebugString( BID_T("\"] \n*** \"") );
        BID_OutputDebugString( sDllPath );
        BID_OutputDebugString( BID_T("\"\n") );
    }

    *pbLdrMsg   = args.bLdrMsg;
    *pbEnabled  = args.bEnabled;

} // _bidLdrReadProfile


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Reads the Registry, validates the path and loads the implementation DLL.
//
//  NOTE:   BidSelectAndLoad() is non-reentrant, so it must be called from synchronized context.
//          Usually it's not an issue, because we want to get diagnostics initialized as
//          early as possible, even before any additional thread is created. Automatic
//          initialization is safe because it's done in context of CRT startup routine.
//          It's also Ok to explicitly call BidSelectAndLoad()/BidUnload() in DllMain, because
//          the OS always serializes DLL initialization.
//

int __cdecl BidSelectAndLoad( void )
{
    BID_TCHAR   extDllName [_MAX_PATH + 10];
    BOOL        bReportFailure = _BIDLDR_MSG_DEFAULT;

    //
    //  Initialization should be done only once. The BidLoad() function sets this flag.
    //
    if( _bidInitialized != 0 )
    {
        return 0;
    }

   #ifdef _BIDIMPL_LDR_INCLUDED
    _bidImplLdrGetRegKeyName();
   #endif

    //
    //  Obtain the reference info about diagnostic dll to be loaded.
    //
    _bidLdrReadProfile( extDllName, _countof(extDllName), &_bidExtDllAllowed, &bReportFailure );
    _bidExtDllName = extDllName;

   #ifdef _BIDIMPL_LDR_INCLUDED
    _bidImplLdrVerifyModuleName( extDllName, _countof(extDllName), &_bidExtDllAllowed );
   #endif

    //
    //  Enable the default diagnostic groups (everything was disabled before
    //  this point). Note that these flags affect only tracepoints within
    //  the module-loader itself.
    //
    if( _BID_APIGROUP_INIT == BidGetApiGroupBits(BID_APIGROUP_MASK_ALL) )
    {
        BidSetApiGroupBits( _BIDLDR_DEFAULT_FLAGS, _BIDLDR_DEFAULT_FLAGS );
    }

    //
    //  Get the selected implementation loaded (if available, allowed and accepted).
    //  NOTE: BidLoad() knows about _bidExtDllName and _bidExtDllAllowed because
    //        they are defined as _BID_EXTERNAL_DLL_NAME and _BID_EXTERNAL_DLL_LOOKUP.
    //
    BidLoad();

    if( !BIDLDR_GetChallengeResponse() )
    {
        BidUnload();
    }

    if( BidNotLoaded() && bReportFailure )
    {
        //
        //  Report the problem if the DLL wasn't loaded and if messages are enabled.
        //  This would be very helpful, because in case of failure all function
        //  hooks will be loaded with empty stubs, and trace events won't come out at all.
        //  The message below will help us understand why we don't see anything.
        //
        BID_OutputDebugString( BID_T("*** \"") );
        BID_OutputDebugString( _bidExtDllName );
        BID_OutputDebugString( BID_T("\" could not be loaded.\n") );
    }

    //
    //  Just a good practice to not leave a pointer to the stack-based buffer.
    //
    _bidExtDllName = _bidBlankStr;

    //
    //  CRT startup routine may check the return value; must be 0 to proceed.
    //
    return 0;

} // BidSelectAndLoad


#undef _BIDLDR_REGKEY
#undef _BIDLDR_MSG
#undef _BIDLDR_PATH

#undef _BIDLDR_MSG_DEFAULT
#undef _BIDLDR_ENABLE_DEFAULT

#endif //////////////////////////////////////////////////////////////////////////////////////////
//                                 End of file "BidApi_ldr.h"                                  //
/////////////////////////////////////////////////////////////////////////////////////////////////
