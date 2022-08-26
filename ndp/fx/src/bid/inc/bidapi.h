/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File:       BidApi.h
//
//  Copyright:  (c) Microsoft Corporation
//
//  Contents:   Built-In Diagnostics Interface declaration.
//
//  Comments:
//              Last Modified: 15-May-2005
//
//              <owner current="true" primary="true">Microsoft</owner>
//
#ifndef __BIDAPI_H__ ////////////////////////////////////////////////////////////////////////////
#define __BIDAPI_H__

#ifndef _BID_DECLARED

#define BID_VER             9210
#define _BID_DECLARED       BID_VER

#if _MSC_VER < 1020
  #error Compiler not supported. Must be VC 4.2 or newer.
#endif
#ifdef WINAPI
  #ifndef _BID_OSAPI_INCLUDED
  #define _BID_OSAPI_INCLUDED
  #endif
#endif
#ifndef _BID_OSAPI_INCLUDED
  #error Windows.h must be included before BidApi.h
#endif

#if defined( _UNICODE ) && !defined( UNICODE )
  #pragma message("WARN: _UNICODE is defined without UNICODE. Possible configuration problem")
#endif
#if defined( UNICODE ) && !defined( _UNICODE )
  #define _UNICODE
#endif

#if _MSC_VER >= 1400
  #ifndef _BID_FORCE_INCLUDE
  #define _BID_FORCE_INCLUDE
  #endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                                     NAMING CONVENTIONS
//
//  BidXxxx
//      General pattern for API functions/macros.
//      BidPutStr, BidTrace1, BidScopeAuto2A, ...
//
//  DXXXX
//      Debug-Only API.
//      DPUTSTR, DTRACE1, DSCOPE_AUTO2A, DASSERT, ...
//
//  BID_XXXX
//      Constants, auto-registration statements, etc.
//      BID_APIGROUP_PERF, BID_EVENT, BID_NOHANDLE, ...
//
//  _BID_XXXX
//      Preprocessor symbols that customize the interface.
//      _BID_EXTERNAL_DLL_NAME, _BID_NO_AUTO_LOAD, _BID_STATIC_BINDING, ...
//
//  xBidXxxx
//      Raw access to the underlying function hooks.
//      Use with care, make sure you know what you're doing.
//      xBidPutStr, xBidTraceV, xBidEnabled, ...
//
//  BIDXXXX, BID_Xxxx
//      Implementation details and helpers. To be used by subsystem implementation.
//      BIDPTRTAG_PTR, BID_GetIndex, ...
//
//  BidxXxxx, xBidxXxxx, BIDX_XXXX
//      Macros with such pattern will never be defined by future versions of BID.
//      Recommended for project-specific BID customizations in order to prevent name conflicts.
//      BidxTraceEx2, xBidxEnabledEx1, BIDX_MY_APIGROUP_BIT ...
//
//  _bidXxxx
//      Interface internal details. Should never be accessed directly.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1200
  #include "basetsd.h"
#else
  #ifndef INT_PTR
  #define INT_PTR           int
  #endif
  #ifndef UINT_PTR
  #define UINT_PTR          unsigned int
  #endif
#endif

#ifndef TCHAR
  #include <tchar.h>
#endif
#ifndef _T
  #define _T(quote)         __TEXT(quote)
#endif
#ifndef PCTSTR
  #define PCTSTR            LPCTSTR
#endif
#ifndef PCVOID
  #define PCVOID            LPCVOID
#endif
#ifndef _countof
  #define _countof(obj)     (sizeof(obj)/sizeof(obj[0]))
#endif

#if defined( DEBUG ) && !defined( _DEBUG )
  #define _DEBUG
#endif
#if defined( _DEBUG ) && !defined( DEBUG )
  #define DEBUG
#endif

#if (defined( _DEBUG ) || defined( DBG )) && !defined( _NO_DEBUG_TRACE )
  #ifndef _DEBUG_TRACE_ON
  #define _DEBUG_TRACE_ON
  #endif
#endif

#if !defined( NDEBUG ) && !defined( _NO_ASSERT )
  #ifndef _ASSERT_ON
  #define _ASSERT_ON
  #endif
#endif

#if (defined( _DEBUG ) || defined( _ASSERT_ON )) && !defined( _NO_DEBUG_BREAK )
  #ifndef _DEBUG_BREAK_ON
  #define _DEBUG_BREAK_ON
  #endif
#endif

#if defined( _DEBUG ) && !defined( _BID_NO_INLINE )
  #define _BID_NO_INLINE
#endif

#if defined( _BID_NO_INLINE )
  #define _bid_INLINE
#elif defined( __cplusplus )
  #define _bid_INLINE       inline
#else
  #define _bid_INLINE       __inline
#endif

#ifndef _INSTRUMENT_ON
  #define _INSTRUMENT_ON
#endif

#if defined( _WIN64 ) && !defined( _BID_NO_UNICODE_LOADER )
  #ifndef _BID_UNICODE_LOADER
  #define _BID_UNICODE_LOADER
  #endif
#endif

#ifndef _HRESULT_DEFINED
  #define _HRESULT_DEFINED
  typedef LONG HRESULT;
#endif

//
//  Back compat declaration of SAL (Structued Annotation Language) keywords used in BID API files
//
#ifndef __specstrings
  #ifndef   __in
    #define __in
  #endif
  #ifndef   __in_opt
    #define __in_opt
  #endif
  #ifndef   __out
    #define __out
  #endif
  #ifndef   __out_ecount
    #define __out_ecount(arg)
  #endif
  #ifndef   __inout
    #define __inout
  #endif
  #ifndef   __inout_ecount
    #define __inout_ecount(arg)
  #endif
#endif

#if defined( _BID_NO_SPECIAL_ALLOCATION )

  #undef _BID_PACK_SRCINFO
  #undef _BID_PACK_LFS
  #undef _BID_PACK_TCFS
  #undef _BID_PACK_STF
  #undef _BID_PACK_EDS
  #undef _BID_PACK_EXAR
  #undef _BID_PACK_METATEXT
  #undef _BID_PACK_AAD
  #undef _BID_PACK_XFS

  #undef  _BID_ASSERT_STRUCT
  #ifndef _BID_NO_ASSERT_STRUCT
  #define _BID_NO_ASSERT_STRUCT
  #endif

#elif defined( _BID_PACK_STRINGS ) && !defined( _BID_NO_PACK_STRINGS )

  #if !defined( _BID_PACK_SRCINFO ) && !defined( _BID_NO_PACK_SRCINFO )
  #define _BID_PACK_SRCINFO
  #endif
  #if !defined( _BID_PACK_LFS ) && !defined( _BID_NO_PACK_LFS )
  #define _BID_PACK_LFS
  #endif
  #if !defined( _BID_PACK_TCFS ) && !defined( _BID_NO_PACK_TCFS )
  #define _BID_PACK_TCFS
  #endif
  #if !defined( _BID_PACK_STF ) && !defined( _BID_NO_PACK_STF )
  #define _BID_PACK_STF
  #endif
  #if !defined( _BID_PACK_EDS ) && !defined( _BID_NO_PACK_EDS )
  #define _BID_PACK_EDS
  #endif
  #if !defined( _BID_PACK_EXAR ) && !defined( _BID_NO_PACK_EXAR )
  #define _BID_PACK_EXAR
  #endif
  #if !defined( _BID_PACK_METATEXT ) && !defined( _BID_NO_PACK_METATEXT )
  #define _BID_PACK_METATEXT
  #endif
  #if !defined( _BID_PACK_AAD ) && !defined( _BID_NO_PACK_AAD )
  #define _BID_PACK_AAD
  #endif
  #if !defined( _BID_PACK_XFS ) && !defined( _BID_NO_PACK_XFS )
  #define _BID_PACK_XFS
  #endif

#endif

#if !defined(_BID_ASSERT_STRUCT) && !defined(_BID_NO_ASSERT_STRUCT)
  #define _BID_ASSERT_STRUCT
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

//
//  Diagnostic API Groups
//
#define BID_APIGROUP_DEFAULT        0x00000001  // BidTraceU (Alyaws ON)
#define BID_APIGROUP_TRACE          0x00000002  // BidTrace, BidPutStr
#define BID_APIGROUP_SCOPE          0x00000004  // BidScope{Enter|Leave|Auto}
#define BID_APIGROUP_PERF           0x00000008  // BidSnap{Start|Stop|Auto}
#define BID_APIGROUP_RSRC           0x00000010  // BidSnap[2|Inc|Dec|Set|Reset]
#define BID_APIGROUP_MEM            0x00000020  // TBD...
#define BID_APIGROUP_S_OK           0x00000040  // BidTrace{HR|Err}, BidScopeLeave{HR|Err}
#define BID_APIGROUP_ADV            0x00000080  // BidTraceU

#define BID_APIGROUP_RESERVED1      0x00000100
#define BID_APIGROUP_RESERVED2      0x00000200
#define BID_APIGROUP_RESERVED3      0x00000400
#define BID_APIGROUP_RESERVED4      0x00000800

#define BID_APIGROUP_MASK_BID       0x00000FFF
#define BID_APIGROUP_MASK_USER      0xFFFFF000
#define BID_APIGROUP_MASK_ALL       0xFFFFFFFF

//
//  Access to Component-Wide ApiGroup Control Bits
//
_bid_INLINE DWORD WINAPI BidGetApiGroupBits(DWORD mask);
extern      DWORD WINAPI BidSetApiGroupBits(DWORD mask, DWORD bits);

typedef     DWORD (WINAPI* BID_CTLCALLBACK)(DWORD mask, DWORD bits);

//
//  ApiGroup Control Bits Implementation Details
//
extern  DWORD               _bidGblFlags;
#define _bidT(bit)          (_bidGblFlags & (bit))

#define _bidTrON            _bidT(BID_APIGROUP_TRACE)
#define _bidScpON           _bidT(BID_APIGROUP_SCOPE)
#define _bidPerfON          _bidT(BID_APIGROUP_PERF)
#define _bidRsrcON          _bidT(BID_APIGROUP_RSRC)
#define _bidMemON           _bidT(BID_APIGROUP_MEM)
#define _bidSOkON           _bidT(BID_APIGROUP_S_OK)

#define _bidPutsON          _bidT(BID_APIGROUP_TRACE)
#define _bidScpTrON         _bidT(BID_APIGROUP_TRACE|BID_APIGROUP_SCOPE)


//
//  Managed / Native mixed mode requires explicit export declaration
//
#ifdef _BID_EXPLICIT_EXPORT
  #ifndef _BIDLDR_EXPORT_HOOKS
  #define _BIDLDR_EXPORT_HOOKS
  #endif
  #define _bidAPI_EXPORT    __declspec(dllexport)
#else
  #define _bidAPI_EXPORT
#endif

//
//  Prototypes for loadable hooks
//
#define _bid_DECL(rv_t, name, args)             \
        typedef rv_t (WINAPI* name##_t) args;   \
        rv_t WINAPI Impl##name args

#define _bid_DECL_EXPORT(rv_t, name, args)      \
        _bid_DECL(rv_t, name, args);            \
        _bidAPI_EXPORT rv_t WINAPI Dll##name args

#define _bid_CDECL(rv_t, name, args)            \
        typedef rv_t (__cdecl* name##_t) args


//
//  Unique local name generator
//
#define _bidNx2(a1,a2,a3)   a1##a2##a3
#define _bidNx1(a1,a2,a3)   _bidNx2(a1,a2,a3)
#define _bidN(name)         _bidNx1(_bidPtrSA_, name##_, __LINE__)


//
//  Opening and closing brackets for Trace Control String.
//  NOTEs:
//    - These are strings, not chars;
//    - Closing bracket comes with trailing space;
//    - Both symbols should be re-defined together.
//
#ifndef _BID_TCS_OPEN
  #define _BID_TCS_OPEN     "<"
  #define _BID_TCS_CLOSE    "> "
#endif

//
//  Keyword separator for Trace Control String
//  Note this is also a string, not char.
//
#ifndef _BID_TCS_SEPARATOR
  #define _BID_TCS_SEPARATOR "|"
#endif

//
//  Abstraction for Function/Method Name (available since VC7)
//
#if _MSC_VER >= 1300
  #define _bid_FNAME        __FUNCTION__
#elif defined( _BID_ERROR_ON_FNAME )
  #define _bid_FNAME        _Automatic_FuncName__VC7_Required_
#else
  #define _bid_FNAME        "MustBeVC7::OrNewer"
#endif

#define _bidFNxA(name)      name
#define _bidFNxW(name)      _bidFNx1(name)
#define _bidFNx1(name)      L##name

#define BID_FNAMEA          _bidFNxA(_bid_FNAME)
#define BID_FNAMEW          _bidFNxW(_bid_FNAME)

#define BID_TAG1A(kwds)     _bidFNxA(_BID_TCS_OPEN) _bidFNxA(_bid_FNAME) \
                            _bidFNxA(_BID_TCS_SEPARATOR) _bidFNxA(kwds) _bidFNxA(_BID_TCS_CLOSE)

#define BID_TAG1W(kwds)     _bidFNxW(_BID_TCS_OPEN) _bidFNxW(_bid_FNAME) \
                            _bidFNxW(_BID_TCS_SEPARATOR) _bidFNxW(kwds) _bidFNxW(_BID_TCS_CLOSE)

#define BID_TAGA            _bidFNxA(_BID_TCS_OPEN) _bidFNxA(_bid_FNAME) _bidFNxA(_BID_TCS_CLOSE)
#define BID_TAGW            _bidFNxW(_BID_TCS_OPEN) _bidFNxW(_bid_FNAME) _bidFNxW(_BID_TCS_CLOSE)

#ifdef _UNICODE
  #define BID_TAG           BID_TAGW
  #define BID_TAG1          BID_TAG1W
  #define BID_FNAME         BID_FNAMEW
#else
  #define BID_TAG           BID_TAGA
  #define BID_TAG1          BID_TAG1A
  #define BID_FNAME         BID_FNAMEA
#endif

//
//  It is common practice to keep macro expressions inside do-while operator,
//  but older compilers may have minor problems with optimization.
//
#ifndef  _BID_NO_DOWHILE0
  #define _bid_DO           do{
  #define _bid_WHILE0       }while(0)
#else
  #define _bid_DO
  #define _bid_WHILE0
#endif

//
//  Agressive optimization may remove BID metadata because usually it doesn't have
//  explicit references. Pragma comment(linker, "/include:<SYMBOL>") explicitly tells the linker
//  to keep <SYMBOL> in the output binary.
//
#ifdef _BID_FORCE_INCLUDE
  #ifdef __cplusplus
    #define _bid_EXTERN_C   extern "C"
  #else
    #define _bid_EXTERN_C
  #endif
  #define _bid_V            volatile
  #define _bid_FORCEINC(V)  _bid_FORCEx1(V)
  #if defined(_WIN64) || defined(_ARM_)
    #define _bid_FORCEx1(V) _bid_FORCEx2(/include:##V)
  #else
    #define _bid_FORCEx1(V) _bid_FORCEx2(/include:_##V)
  #endif
  #define _bid_FORCEx2(V)   __pragma(comment(linker, #V))
#else
  #define _bid_V            static volatile
  #define _bid_EXTERN_C
  #define _bid_FORCEINC(V)
#endif

//
//  This W4-level warning is caused by 'do-while' wrappers and BidAssert(<constant_expression>)
//
#if !defined( _ALL_WARNINGS )
  #pragma warning(disable: 4127)    // conditional expression is constant
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  General services for BID Pointers (to be used by subsystem implementation)
//
//  PORTING NOTE:
//  The solution below utilizes the fact that compiler aligns string constants at 4 bytes
//  boundary (at least), so initial values of pointers to that strings have 2 free lower bits
//  and they can be used as flags. Since the precondition applies to initial value only, it
//  does not affect normal pointer arithmetic: once initialized, the pointer can be incremented
//  per 'char' or 'wchar_t' as usual.
//
#define BIDPTRTAG_PTR       0
#define BIDPTRTAG_REFPTR    1
#define BIDPTRTAG_INDEX     2
#define BIDPTRTAG_STUB      3

#define BID_GetPtrTag(ptr)  (int)((UINT_PTR)(ptr) & 0x3)
#define BID_GetPtr(ptr)     (PVOID)((UINT_PTR)(ptr) & ~0x3)

#define BID_IsAPointer(ptr) (BID_GetPtrTag(ptr) == BIDPTRTAG_PTR)
#define BID_IsARefPtr(ptr)  (BID_GetPtrTag(ptr) == BIDPTRTAG_REFPTR)
#define BID_IsAnIndex(ptr)  (BID_GetPtrTag(ptr) == BIDPTRTAG_INDEX)
#define BID_IsAStub(ptr)    (BID_GetPtrTag(ptr) == BIDPTRTAG_STUB)
#define BID_NotAPointer(p)  (((UINT_PTR)(p) & ~0xFFFF) == 0)

#define BID_GetIndex(ptr)   (UINT)((UINT_PTR)(ptr) >> 2)
#define BID_SetIndex(idx)   (((UINT_PTR)(idx) << 2) | BIDPTRTAG_INDEX)

//
//  To be used with BID_IsAPointer(src) and BID_IsSrcFile{A|W}(info)
//
#define BID_SrcFileA(src)   (PCSTR)BID_GetPtr(src)
#define BID_SrcFileW(src)   (PCWSTR)BID_GetPtr(src)

//
//  Static index-like stubs.
//
#define _bidP               (INT_PTR)(((__LINE__ << 2) & 0xFFFF) | BIDPTRTAG_STUB)
#define _bidPA              (PCSTR)_bidP
#define _bidPW              (PCWSTR)_bidP

#define _bidSV              static volatile


/////////////////////////////////////////////////////////////////////////////////////////////////
//                           SOURCE FILE, LINE NUMBER AND TRIM INFO                            //
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _BID_NO_SPECIAL_ALLOCATION
  #ifndef _BID_NO_SRCINFO
  #define _BID_NO_SRCINFO
  #endif
#endif

#ifdef _BID_NO_SRCINFO_ALL

  #ifndef _BID_NO_SRCINFO
  #define _BID_NO_SRCINFO
  #endif

  #define _bidSRC2_A        _bidPA
  #define _bidSF2           0
  #define _bid_SRCINFO2
  #define _bidSrcFile2A     0

#else

  #ifdef _BID_NO_SPECIAL_ALLOCATION

    #define _bidSRC2_A      __FILE__
    #define _bid_SRCINFO2   _bidSV PCSTR _bidSrcFile2A = _bidSRC2_A;

  #else

    #ifdef _BID_PACK_SRCINFO
      #define _bidSRC2_A    _bidPA
    #else
      #define _bidSRC2_A    __FILE__
    #endif
    #define _bid_SRCINFO2   __declspec(allocate("BID$A012")) _bidSV PCSTR _bidSrcFile2A = _bidSRC2_A;

  #endif

  #define _bidSF2           (UINT_PTR)_bidSrcFile2A

#endif


#ifdef _BID_NO_SRCINFO

  #define _bidSRC_A         _bidPA
  #define _bid_SRCINFO
  #define _bidSF            0

#else

  #ifdef _BID_PACK_SRCINFO
    #define _bidSRC_A       _bidPA
  #else
    #define _bidSRC_A       __FILE__
  #endif

  #define _bid_SRCINFO      __declspec(allocate("BID$A012")) _bidSV PCSTR _bidSrcFileA = _bidSRC_A;
  #define _bidSF            (UINT_PTR)_bidSrcFileA

#endif

#define BID_SRCFILE         _bid_SRCINFO2
#define xBID_SRCFILE        _bid_SRCINFO2


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SLN - "Source Line Number"
//  LF  - "Linenum and Flags"
//  LF2 - "Forced" Linenum and Flags.
//
//  NOTE: for 32 bit code, the limit is 4,194,303 (003FFFFF) lines in a single source file.
//
#define _bidSLN                     ((UINT_PTR)__LINE__ << 10)
#define _bidLF(flg)                 (_bidSLN | (UINT_PTR)(flg))
#define _bidLF2(flg)                (_bidSLN | (UINT_PTR)(flg) | BID_DEMAND_SRC)

//
//  Access to multi-purpose argument "info"
//
#define BID_IsSrcFileA(info)        (((info) & 0x100) == 0)
#define BID_IsSrcFileW(info)        (((info) & 0x100) != 0)
    #define BID_SRCFILE_W           0x100

#define BID_SrcLine(info)           (int)((info) >> 10)
#define BID_InfoFlags(info)         (DWORD)((info) & 0xFF)

#define BID_InfoLn(info)            (int)((info) & 0x3)
    #define BID_STR                 0
    #define BID_SLN                 1
    #define BID_LN                  2
  //#define BID_???                 3

#define BID_InfoIsEnabled(info)     (((info) & 0x04) != 0)
    #define BID_CHK                 0x00
    #define BID_ENA                 0x04

#define BID_InfoIsDemandSrc(info)   (((info) & 0x08) != 0)
    #define BID_DEMAND_SRC          0x08

#define BID_InfoBlobType(info)      (int)((info) & 0x17)
#define BID_InfoIsBlob(info)        (((info) & 0x10) != 0)
#define BID_InfoIsBlobCopy(info)    (((info) & 0x12) == 0x12)
#define BID_InfoIsBlobBinMode(info) (BID_InfoBlobType(info) == BID_BLOB_BIN_MODE)
    #define BID_BLOB                0x10
    #define BID_BLOB2               0x11
    #define BID_BLOB_COPY           0x12
    #define BID_BLOB2_COPY          0x13
    #define BID_BLOB_BIN_MODE       0x14

#define BID_InfoIsTrigger(info)     (((info) & 0x20) != 0)
    #define BID_TRIGGER             0x20

//  BID_Info reserved:              0x40, 0x80

//
//  This flag should be used in parameter 'info'
//  when Unicode version of BidAssert function is implemented.
//
#define BID_ASSERT_TEXT_W           0x200


//
//  Naming Convention consistency ..
//
#define xBidSRC                     _bidSF2
#define xBidSRC2                    _bidSF2

#define xBidFLAGS(flg)              _bidLF(flg)
#define xBidFLAGS2(flg)             _bidLF2(flg)



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                     SPECIAL ALLOCATION                                      //
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _BID_NO_SPECIAL_ALLOCATION

  //
  // Reserved.
  //
  #define _bid_LFS_A(ptr, str)      PCSTR  ptr = str;
  #define _bid_LFS_W(ptr, str)      PCWSTR ptr = str;

  //
  // TCFS stands for "Trace Control & Format String"
  //
  #define _bid_TCFS_A(ptr, str)     PCSTR  ptr = str;
  #define _bid_TCFS_W(ptr, str)     PCWSTR ptr = str;

  //
  // STF stands for "Scope Tracking Formatter"
  //
  #define _bid_STF_A(ptr, str)      PCSTR  ptr = str;
  #define _bid_STF_W(ptr, str)      PCWSTR ptr = str;

  //
  // EDS stands for "Event Definition String"
  //
  #define _bid_EDS_A(ptr, str)      PCSTR  ptr = str;
  #define _bid_EDS_W(ptr, str)      PCWSTR ptr = str;

  //
  // XFS stands for "eXtension Format String"
  //
  #define _bid_XFS_A(ptr, str)      PCSTR  ptr = str;
  #define _bid_XFS_W(ptr, str)      PCWSTR ptr = str;

#else

  #ifdef _BID_PACK_LFS
    #define _bid_LFS_A(ptr, str)    __declspec(allocate("BIDL$A022")) _bidSV PCSTR  ptr = _bidPA;
    #define _bid_LFS_W(ptr, str)    __declspec(allocate("BIDL$W022")) _bidSV PCWSTR ptr = _bidPW;
  #else
    #define _bid_LFS_A(ptr, str)    __declspec(allocate("BIDL$A022")) _bidSV PCSTR  ptr = str;
    #define _bid_LFS_W(ptr, str)    __declspec(allocate("BIDL$W022")) _bidSV PCWSTR ptr = str;
  #endif

  #ifdef _BID_PACK_TCFS
    #define _bid_TCFS_A(ptr, str)   __declspec(allocate("BID$A032")) _bidSV PCSTR  ptr = _bidPA;
    #define _bid_TCFS_W(ptr, str)   __declspec(allocate("BID$W032")) _bidSV PCWSTR ptr = _bidPW;
  #else
    #define _bid_TCFS_A(p,str)      __declspec(allocate("BID$A032")) _bidSV PCSTR  p = str;
    #define _bid_TCFS_W(p,str)      __declspec(allocate("BID$W032")) _bidSV PCWSTR p = str;
  #endif

  #ifdef _BID_PACK_STF
    #define _bid_STF_A(ptr, str)    __declspec(allocate("BID$A042")) _bidSV PCSTR  ptr = _bidPA;
    #define _bid_STF_W(ptr, str)    __declspec(allocate("BID$W042")) _bidSV PCWSTR ptr = _bidPW;
  #else
    #define _bid_STF_A(p,str)       __declspec(allocate("BID$A042")) _bidSV PCSTR  p = str;
    #define _bid_STF_W(p,str)       __declspec(allocate("BID$W042")) _bidSV PCWSTR p = str;
  #endif

  #ifdef _BID_PACK_EDS
    #define _bid_EDS_A(ptr, str)    __declspec(allocate("BID$A052")) _bidSV PCSTR  ptr = _bidPA;
    #define _bid_EDS_W(ptr, str)    __declspec(allocate("BID$W052")) _bidSV PCWSTR ptr = _bidPW;
  #else
    #define _bid_EDS_A(ptr, str)    __declspec(allocate("BID$A052")) _bidSV PCSTR  ptr = str;
    #define _bid_EDS_W(ptr, str)    __declspec(allocate("BID$W052")) _bidSV PCWSTR ptr = str;
  #endif

  #ifdef _BID_PACK_XFS
    #define _bid_XFS_A(ptr, str)    __declspec(allocate("BID$A102")) _bidSV PCSTR  ptr = _bidPA;
    #define _bid_XFS_W(ptr, str)    __declspec(allocate("BID$W102")) _bidSV PCWSTR ptr = _bidPW;
  #else
    #define _bid_XFS_A(ptr, str)    __declspec(allocate("BID$A102")) _bidSV PCSTR  ptr = str;
    #define _bid_XFS_W(ptr, str)    __declspec(allocate("BID$W102")) _bidSV PCWSTR ptr = str;
  #endif

#endif

//
//  API to allocate TCFS for customized tracepoints (BidTraceExV, BidTraceEV, xBidEnabled)
//  NOTE: Instead of BID_TCFS, use
//      xBID_XFS with xBidWrite
//      xBID_IDS with xBid{Obtain|Update|Recycle}ItemID
//
#ifdef _BID_NO_SPECIAL_ALLOCATION
  #define BID_TCFSA(name, str)      static char  name[] = str;
  #define BID_TCFSW(name, str)      static WCHAR name[] = str;
#else
  #define BID_TCFSA(name, str)      _bid_TCFS_A(name,str)
  #define BID_TCFSW(name, str)      _bid_TCFS_W(name,str)
#endif

//
//  Naming Convention consistency ..
//
#define xBID_TCFSA                  BID_TCFSA
#define xBID_TCFSW                  BID_TCFSW
#define xBidID                      _bidID

#ifdef _UNICODE
  #define BID_TCFS                  BID_TCFSW
  #define xBID_TCFS                 xBID_TCFSW
#else
  #define BID_TCFS                  BID_TCFSA
  #define xBID_TCFS                 xBID_TCFSA
#endif


//
//  Module Instance Handle
//
extern  HANDLE                      _bidID;

#define BID_NOHANDLE                (HANDLE)(-1)
#define _bidID_IF                   (_bidID != BID_NOHANDLE)


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                         DEBUG BREAK                                         //
/////////////////////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1300                // This also covers _WIN64
  #define   xBidBreak()             __debugbreak()
#else
  #define   xBidBreak()             __asm { int 3 }
#endif

#ifdef _DEBUG_BREAK_ON
  #define   DBREAK                  xBidBreak
#else
  #define   DBREAK()                ((void)0)
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                       ASSERT & VERIFY                                       //
/////////////////////////////////////////////////////////////////////////////////////////////////

_bid_DECL( BOOL, BidAssert, (HANDLE hID, UINT_PTR arg, UINT_PTR info) );

typedef struct __bidAssertArg
{
    PCSTR       txtID;
    PCSTR       srcInfo;
    INT_PTR     Tag;

} BIDASSERTARG, * PBIDASSERTARG;


#ifdef _BID_ASSERT_STRUCT

  //
  //  AAD stands for "Assert Arguments Descriptor"
  //
  #ifdef _BID_PACK_AAD
    #define _bid_AAD_A(txt)                      \
                __declspec(allocate("BID$A092")) \
                _bidSV BIDASSERTARG _bidAST = { (PCSTR)_bidP, (PCSTR)_bidSRC2_A, (INT_PTR)_bidSLN };
  #else
    #define _bid_AAD_A(txt)                      \
                __declspec(allocate("BID$A092")) \
                _bidSV BIDASSERTARG _bidAST = { txt,  (PCSTR)_bidSRC2_A, (INT_PTR)_bidSLN };
  #endif

  #define _bidAP(txt)           ((UINT_PTR)&_bidAST | BIDPTRTAG_REFPTR)
  #define _bidSkipAssertHook    (0 == _bidAST.Tag)

#else

  #define _bid_AAD_A(txt)

  #define _bidAP(txt)           (UINT_PTR)(PCSTR)txt
  #define _bidSkipAssertHook    FALSE

#endif


#define xBidAssert(args,info)   (*_bidPfn.BidAssert)(_bidID,args,info)


#define BidAssertSz(exp,sz)                                                 \
        do {                                                                \
            _bid_AAD_A(sz)                                                  \
            if( !(exp) && (_bidSkipAssertHook ||                            \
                           !xBidAssert( _bidAP(sz), _bidLF2(BID_SLN) )) )   \
            {                                                               \
                DBREAK();                                                   \
            }                                                               \
        } while(0)

#define BidAssert(exp)          BidAssertSz(exp, #exp)


#ifndef DASSERT
  #ifdef _ASSERT_ON
    #define DASSERT             BidAssert
  #else
    #define DASSERT(exp)        ((void)0)
  #endif
#endif

#undef  DVERIFY
#ifdef _ASSERT_ON
  #define DVERIFY               BidAssert
#else
  #define DVERIFY(exp)          (void)(exp)
#endif

#define BAD_ENUM                FALSE
#define BAD_CODE_PATH           FALSE
#define NOT_IMPLEMENTED_YET     FALSE

//
//  COMPILE-TIME ASSERT.
//  If compile time assertion failed, compiler generates
//  error C2466: "cannot allocate an array of constant size 0"
//
#define BidAssertCompiler(expr) typedef int _bidN(CompileTimeAssert) [(int)(expr)];

#ifdef _ASSERT_ON
  #define DASSERT_COMPILER      BidAssertCompiler
#else
  #define DASSERT_COMPILER(exp)
#endif

//
//  Implementation Helpers
//
#define BID_IsAssertTextA(info)             (0 == ((info) & BID_ASSERT_TEXT_W))
#define BID_IsAssertTextW(info)             (0 != ((info) & BID_ASSERT_TEXT_W))

#define BID_IsAssertSrcFileA(info)          BID_IsSrcFileA(info)
#define BID_IsAssertSrcFileW(info)          BID_IsSrcFileW(info)

#define BID_IsAssertArgText(arg)            BID_IsAPointer(arg)
#define BID_IsAssertArgStruct(arg)          BID_IsARefPtr(arg)

#define BID_AssertTextA(arg)                (PCSTR)BID_GetPtr(arg)
#define BID_AssertTextW(arg)                (PCWSTR)BID_GetPtr(arg)
#define BID_AssertStruct(arg)               (PBIDASSERTARG)BID_GetPtr(arg)

#define BID_IsAssertStructTextPtr(pArg)     BID_IsAPointer((pArg)->txtID)
#define BID_IsAssertStructTextIndex(pArg)   BID_IsAnIndex((pArg)->txtID)

#define BID_IsAssertSrcFilePtr(pArg)        BID_IsAPointer((pArg)->srcInfo)
#define BID_IsAssertSrcFileIndex(pArg)      BID_IsAnIndex((pArg)->srcInfo)

#define BID_AssertStructTextA(pArg)         (PCSTR)BID_GetPtr((pArg)->txtID)
#define BID_AssertStructTextW(pArg)         (PCWSTR)BID_GetPtr((pArg)->txtID)

#define BID_AssertSrcFileA(pArg)            (PCSTR)BID_GetPtr((pArg)->srcInfo)
#define BID_AssertSrcFileW(pArg)            (PCWSTR)BID_GetPtr((pArg)->srcInfo)

#define BID_AssertTag(pArg)                 (pArg)->Tag


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                     PLAIN STRING OUTPUT                                     //
/////////////////////////////////////////////////////////////////////////////////////////////////

_bid_DECL(BOOL, BidPutStrA, (HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  str) );
_bid_DECL(BOOL, BidPutStrW, (HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR str) );

#define xBidPutStrA(a,b,c)  (_bidID_IF ? (*_bidPfn.BidPutStrA)(_bidID,a,b,c) : TRUE)
#define xBidPutStrW(a,b,c)  (_bidID_IF ? (*_bidPfn.BidPutStrW)(_bidID,a,b,c) : TRUE)

#define BidPutStrExA(ln,str)                                \
  _bid_DO                                                   \
    if( _bidPutsON )                                        \
    {   _bid_SRCINFO                                        \
        if( !xBidPutStrA(_bidSF, _bidLF(ln), str)) DBREAK();\
    }                                                       \
  _bid_WHILE0

#define BidPutStrExW(ln,str)                                \
  _bid_DO                                                   \
    if( _bidPutsON )                                        \
    {   _bid_SRCINFO                                        \
        if( !xBidPutStrW(_bidSF, _bidLF(ln), str)) DBREAK();\
    }                                                       \
  _bid_WHILE0

#define BidPutStrA(str)     BidPutStrExA(BID_STR,str)
#define BidPutStrLineA(str) BidPutStrExA(BID_SLN,str)

#define BidPutStrW(str)     BidPutStrExW(BID_STR,str)
#define BidPutStrLineW(str) BidPutStrExW(BID_SLN,str)

#if defined( _UNICODE )
  #define   BidPutStr       BidPutStrW
  #define   BidPutStrEx     BidPutStrExW
  #define   BidPutStrLine   BidPutStrLineW
  #define   xBidPutStr      xBidPutStrW
#else
  #define   BidPutStr       BidPutStrA
  #define   BidPutStrEx     BidPutStrExA
  #define   BidPutStrLine   BidPutStrLineA
  #define   xBidPutStr      xBidPutStrA
#endif

#define BidPutNewLine()     BidPutStrEx(BID_LN,NULL)


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                    MAIN TRACING FACILITY                                    //
/////////////////////////////////////////////////////////////////////////////////////////////////

_bid_DECL(BOOL, BidTraceVA, (HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  fmt, va_list args) );
_bid_DECL(BOOL, BidTraceVW, (HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR fmt, va_list args) );

#define xBidTraceExVA(hID,src,info,fmt,args)    \
                            (_bidID_IF ? (*_bidPfn.BidTraceVA)(hID,src,info,fmt,args) : TRUE)

#define xBidTraceExVW(hID,src,info,fmt,args)    \
                            (_bidID_IF ? (*_bidPfn.BidTraceVW)(hID,src,info,fmt,args) : TRUE)

#define xBidTraceVA(a,b,c,d) xBidTraceExVA(_bidID,a,b,c,d)
#define xBidTraceVW(a,b,c,d) xBidTraceExVW(_bidID,a,b,c,d)

BOOL __cdecl _bidTraceA     (UINT_PTR src, UINT_PTR info, PCSTR  fmt, ... );
BOOL __cdecl _bidTraceW     (UINT_PTR src, UINT_PTR info, PCWSTR fmt, ... );

#define xBidTraceA          _bidTraceA
#define xBidTraceW          _bidTraceW

#if defined( _UNICODE)
  #define   _bidTrace       _bidTraceW
  #define   xBidTraceV      xBidTraceVW
  #define   xBidTraceExV    xBidTraceExVW
  #define   xBidTrace       xBidTraceW
#else
  #define   _bidTrace       _bidTraceA
  #define   xBidTraceV      xBidTraceVA
  #define   xBidTraceExV    xBidTraceExVA
  #define   xBidTrace       xBidTraceA
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

#define BidTraceExVA(ON, flg, tcfs, va)                                     \
  _bid_DO                                                                   \
    if( ON )                                                                \
    {   _bid_SRCINFO                                                        \
        if( tcfs && !xBidTraceVA(_bidSF, _bidLF(flg), tcfs, va) ) DBREAK(); \
    }                                                                       \
  _bid_WHILE0

#define BidTraceExVW(ON, flg, tcfs, va)                                     \
  _bid_DO                                                                   \
    if( ON )                                                                \
    {   _bid_SRCINFO                                                        \
        if( tcfs && !xBidTraceVW(_bidSF, _bidLF(flg), tcfs, va) ) DBREAK(); \
    }                                                                       \
  _bid_WHILE0

#define BidTraceEVA(fmt,va) BidTraceExVA( _bidTrON, BID_ENA, fmt, va)
#define BidTraceEVW(fmt,va) BidTraceExVW( _bidTrON, BID_ENA, fmt, va)

#if defined( _UNICODE )
  #define   BidTraceEV      BidTraceEVW
  #define   BidTraceExV     BidTraceExVW
#else
  #define   BidTraceEV      BidTraceEVA
  #define   BidTraceExV     BidTraceExVA
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

#define _bid_T0(x,ON,y,tcfmts)                                                  \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !xBidTraceV##x(_bidSF, _bidLF(y),_bidN(030),0))          DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T1(x,ON,y,tcfmts,a)                                                \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a) )          DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T2(x,ON,y,tcfmts,a,b)                                              \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b))         DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T3(x,ON,y,tcfmts,a,b,c)                                            \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c) )      DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T4(x,ON,y,tcfmts,a,b,c,d)                                          \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c,d))     DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T5(x,ON,y,tcfmts,a,b,c,d,e)                                        \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c,d,e))   DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T6(x,ON,y,tcfmts,a,b,c,d,e,f)                                      \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c,d,e,f)) DBREAK();  \
    }                                                                           \
  _bid_WHILE0

#define _bid_T7(x,ON,y,tcfmts,a,b,c,d,e,f,g)                                    \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
            !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c,d,e,f,g)) DBREAK();\
    }                                                                           \
  _bid_WHILE0

#define _bid_T8(x,ON,y,tcfmts,a,b,c,d,e,f,g,h)                                  \
  _bid_DO                                                                       \
    if( ON )                                                                    \
    {   _bid_SRCINFO                                                            \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                       \
        if( _bidN(030) &&                                                       \
          !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c,d,e,f,g,h)) DBREAK();\
    }                                                                           \
  _bid_WHILE0

#define _bid_T9(x,ON,y,tcfmts,a,b,c,d,e,f,g,h,i)                                    \
  _bid_DO                                                                           \
    if( ON )                                                                        \
    {   _bid_SRCINFO                                                                \
        _bid_TCFS_##x(_bidN(030), tcfmts)                                           \
        if( _bidN(030) &&                                                           \
          !_bidTrace##x(_bidSF, _bidLF(y),_bidN(030),a,b,c,d,e,f,g,h,i)) DBREAK();  \
    }                                                                               \
  _bid_WHILE0


#define BidTraceEx0A(bit,flg,tcfs)                  _bid_T0(A,bit,flg,tcfs)
#define BidTraceEx1A(bit,flg,tcfs,a)                _bid_T1(A,bit,flg,tcfs,a)
#define BidTraceEx2A(bit,flg,tcfs,a,b)              _bid_T2(A,bit,flg,tcfs,a,b)
#define BidTraceEx3A(bit,flg,tcfs,a,b,c)            _bid_T3(A,bit,flg,tcfs,a,b,c)
#define BidTraceEx4A(bit,flg,tcfs,a,b,c,d)          _bid_T4(A,bit,flg,tcfs,a,b,c,d)
#define BidTraceEx5A(bit,flg,tcfs,a,b,c,d,e)        _bid_T5(A,bit,flg,tcfs,a,b,c,d,e)
#define BidTraceEx6A(bit,flg,tcfs,a,b,c,d,e,f)      _bid_T6(A,bit,flg,tcfs,a,b,c,d,e,f)
#define BidTraceEx7A(bit,flg,tcfs,a,b,c,d,e,f,g)    _bid_T7(A,bit,flg,tcfs,a,b,c,d,e,f,g)
#define BidTraceEx8A(bit,flg,tcfs,a,b,c,d,e,f,g,h)  _bid_T8(A,bit,flg,tcfs,a,b,c,d,e,f,g,h)
#define BidTraceEx9A(bit,flg,tcfs,a,b,c,d,e,f,g,h,i) _bid_T9(A,bit,flg,tcfs,a,b,c,d,e,f,g,h,i)

#define BidTraceEx0W(bit,flg,tcfs)                  _bid_T0(W,bit,flg,tcfs)
#define BidTraceEx1W(bit,flg,tcfs,a)                _bid_T1(W,bit,flg,tcfs,a)
#define BidTraceEx2W(bit,flg,tcfs,a,b)              _bid_T2(W,bit,flg,tcfs,a,b)
#define BidTraceEx3W(bit,flg,tcfs,a,b,c)            _bid_T3(W,bit,flg,tcfs,a,b,c)
#define BidTraceEx4W(bit,flg,tcfs,a,b,c,d)          _bid_T4(W,bit,flg,tcfs,a,b,c,d)
#define BidTraceEx5W(bit,flg,tcfs,a,b,c,d,e)        _bid_T5(W,bit,flg,tcfs,a,b,c,d,e)
#define BidTraceEx6W(bit,flg,tcfs,a,b,c,d,e,f)      _bid_T6(W,bit,flg,tcfs,a,b,c,d,e,f)
#define BidTraceEx7W(bit,flg,tcfs,a,b,c,d,e,f,g)    _bid_T7(W,bit,flg,tcfs,a,b,c,d,e,f,g)
#define BidTraceEx8W(bit,flg,tcfs,a,b,c,d,e,f,g,h)  _bid_T8(W,bit,flg,tcfs,a,b,c,d,e,f,g,h)
#define BidTraceEx9W(bit,flg,tcfs,a,b,c,d,e,f,g,h,i) _bid_T9(W,bit,flg,tcfs,a,b,c,d,e,f,g,h,i)


#define BidTrace0A(tcfs)                            _bid_T0(A,_bidTrON,0,tcfs)
#define BidTrace1A(tcfs,a)                          _bid_T1(A,_bidTrON,0,tcfs,a)
#define BidTrace2A(tcfs,a,b)                        _bid_T2(A,_bidTrON,0,tcfs,a,b)
#define BidTrace3A(tcfs,a,b,c)                      _bid_T3(A,_bidTrON,0,tcfs,a,b,c)
#define BidTrace4A(tcfs,a,b,c,d)                    _bid_T4(A,_bidTrON,0,tcfs,a,b,c,d)
#define BidTrace5A(tcfs,a,b,c,d,e)                  _bid_T5(A,_bidTrON,0,tcfs,a,b,c,d,e)
#define BidTrace6A(tcfs,a,b,c,d,e,f)                _bid_T6(A,_bidTrON,0,tcfs,a,b,c,d,e,f)
#define BidTrace7A(tcfs,a,b,c,d,e,f,g)              _bid_T7(A,_bidTrON,0,tcfs,a,b,c,d,e,f,g)
#define BidTrace8A(tcfs,a,b,c,d,e,f,g,h)            _bid_T8(A,_bidTrON,0,tcfs,a,b,c,d,e,f,g,h)
#define BidTrace9A(tcfs,a,b,c,d,e,f,g,h,i)          _bid_T9(A,_bidTrON,0,tcfs,a,b,c,d,e,f,g,h,i)

#define BidTrace0W(tcfs)                            _bid_T0(W,_bidTrON,0,tcfs)
#define BidTrace1W(tcfs,a)                          _bid_T1(W,_bidTrON,0,tcfs,a)
#define BidTrace2W(tcfs,a,b)                        _bid_T2(W,_bidTrON,0,tcfs,a,b)
#define BidTrace3W(tcfs,a,b,c)                      _bid_T3(W,_bidTrON,0,tcfs,a,b,c)
#define BidTrace4W(tcfs,a,b,c,d)                    _bid_T4(W,_bidTrON,0,tcfs,a,b,c,d)
#define BidTrace5W(tcfs,a,b,c,d,e)                  _bid_T5(W,_bidTrON,0,tcfs,a,b,c,d,e)
#define BidTrace6W(tcfs,a,b,c,d,e,f)                _bid_T6(W,_bidTrON,0,tcfs,a,b,c,d,e,f)
#define BidTrace7W(tcfs,a,b,c,d,e,f,g)              _bid_T7(W,_bidTrON,0,tcfs,a,b,c,d,e,f,g)
#define BidTrace8W(tcfs,a,b,c,d,e,f,g,h)            _bid_T8(W,_bidTrON,0,tcfs,a,b,c,d,e,f,g,h)
#define BidTrace9W(tcfs,a,b,c,d,e,f,g,h,i)          _bid_T9(W,_bidTrON,0,tcfs,a,b,c,d,e,f,g,h,i)


#if defined( _UNICODE)
  #define   BidTraceEx0     BidTraceEx0W
  #define   BidTraceEx1     BidTraceEx1W
  #define   BidTraceEx2     BidTraceEx2W
  #define   BidTraceEx3     BidTraceEx3W
  #define   BidTraceEx4     BidTraceEx4W
  #define   BidTraceEx5     BidTraceEx5W
  #define   BidTraceEx6     BidTraceEx6W
  #define   BidTraceEx7     BidTraceEx7W
  #define   BidTraceEx8     BidTraceEx8W
  #define   BidTraceEx9     BidTraceEx9W

  #define   BidTrace0       BidTrace0W
  #define   BidTrace1       BidTrace1W
  #define   BidTrace2       BidTrace2W
  #define   BidTrace3       BidTrace3W
  #define   BidTrace4       BidTrace4W
  #define   BidTrace5       BidTrace5W
  #define   BidTrace6       BidTrace6W
  #define   BidTrace7       BidTrace7W
  #define   BidTrace8       BidTrace8W
  #define   BidTrace9       BidTrace9W
#else
  #define   BidTraceEx0     BidTraceEx0A
  #define   BidTraceEx1     BidTraceEx1A
  #define   BidTraceEx2     BidTraceEx2A
  #define   BidTraceEx3     BidTraceEx3A
  #define   BidTraceEx4     BidTraceEx4A
  #define   BidTraceEx5     BidTraceEx5A
  #define   BidTraceEx6     BidTraceEx6A
  #define   BidTraceEx7     BidTraceEx7A
  #define   BidTraceEx8     BidTraceEx8A
  #define   BidTraceEx9     BidTraceEx9A

  #define   BidTrace0       BidTrace0A
  #define   BidTrace1       BidTrace1A
  #define   BidTrace2       BidTrace2A
  #define   BidTrace3       BidTrace3A
  #define   BidTrace4       BidTrace4A
  #define   BidTrace5       BidTrace5A
  #define   BidTrace6       BidTrace6A
  #define   BidTrace7       BidTrace7A
  #define   BidTrace8       BidTrace8A
  #define   BidTrace9       BidTrace9A
#endif



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                       SCOPE TRACKING                                        //
/////////////////////////////////////////////////////////////////////////////////////////////////

#if defined( _BID_NO_INIT_HSCOPE )
  #define BID_HSCOPE(h)         HANDLE h
#else
  #define BID_HSCOPE(h)         HANDLE h = BID_NOHANDLE
#endif

_bid_DECL(BOOL, BidScopeEnterVA,(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp, PCSTR  stf, va_list va));
_bid_DECL(BOOL, BidScopeEnterVW,(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp, PCWSTR stf, va_list va));
_bid_DECL(BOOL, BidScopeLeave,  (HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp) );

BOOL __cdecl _bidScopeEnterA    (HANDLE* pHScp, PCSTR  stf, ... );
BOOL __cdecl _bidScopeEnterW    (HANDLE* pHScp, PCWSTR stf, ... );


#define xBidScopeEnterVA(h,a,b) (_bidID_IF ? (*_bidPfn.BidScopeEnterVA)(_bidID,0,0,h,a,b) : TRUE)
#define xBidScopeEnterVW(h,a,b) (_bidID_IF ? (*_bidPfn.BidScopeEnterVW)(_bidID,0,0,h,a,b) : TRUE)
#define xBidScopeLeave_(h)      (_bidID_IF ? (*_bidPfn.BidScopeLeave)(_bidID,0,0,h) : TRUE)
#define xBidScopeLeave(s,i,h)   (_bidID_IF ? (*_bidPfn.BidScopeLeave)(_bidID,s,i,h) : TRUE)


#if defined( _BID_NO_INIT_HSCOPE )
  #define _bidINIT_HSCP(h)      *(h) = BID_NOHANDLE;
  #define _bidCHK_HSCP_AND(h)
#else
  #define _bidINIT_HSCP(h)
  #define _bidCHK_HSCP_AND(h)   (*(h) != BID_NOHANDLE) &&
#endif

//
//  ISSUE: Change _bid_CX logic to invoke _bidINIT_HSCP(H) if(ON && !_bidN(040))
//
#ifdef _BID_NO_INIT_HSCOPE
  #error _BID_NO_INIT_HSCOPE disabled.
#endif

#define _bid_C0(x,ON,H,stf)                                                         \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !xBidScopeEnterV##x(H,_bidN(040),0))            DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C1(x,ON,H,stf,a)                                                       \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a))             DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C2(x,ON,H,stf,a,b)                                                     \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b))           DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C3(x,ON,H,stf,a,b,c)                                                   \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c))         DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C4(x,ON,H,stf,a,b,c,d)                                                 \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c,d))       DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C5(x,ON,H,stf,a,b,c,d,e)                                               \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c,d,e))     DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C6(x,ON,H,stf,a,b,c,d,e,f)                                             \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c,d,e,f))   DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C7(x,ON,H,stf,a,b,c,d,e,f,g)                                           \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
        _bid_STF_##x(_bidN(040), stf)                                               \
        if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c,d,e,f,g)) DBREAK(); \
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C8(x,ON,H,stf,a,b,c,d,e,f,g,h)                                         \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
       _bid_STF_##x(_bidN(040), stf)                                                \
       if( _bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c,d,e,f,g,h)) DBREAK();\
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0

#define _bid_C9(x,ON,H,stf,a,b,c,d,e,f,g,h,i)                                       \
  _bid_DO                                                                           \
    if( ON ){                                                                       \
      _bid_STF_##x(_bidN(040), stf)                                                 \
      if(_bidN(040) && !_bidScopeEnter##x(H,_bidN(040),a,b,c,d,e,f,g,h,i)) DBREAK();\
    }else{                                                                          \
        _bidINIT_HSCP(H)                                                            \
    }                                                                               \
  _bid_WHILE0


#define _bid_COut(ON,H)                                                             \
  _bid_DO                                                                           \
    if( ON && _bidCHK_HSCP_AND(H) !xBidScopeLeave_(H) ) DBREAK();                   \
  _bid_WHILE0


#define BidScopeEnterExA(H,stf)                     _bid_C0(A,_bidScpON,H,stf)
#define BidScopeEnterEx0A(H,stf)                    _bid_C0(A,_bidScpON,H,stf)
#define BidScopeEnterEx1A(H,stf,a)                  _bid_C1(A,_bidScpON,H,stf,a)
#define BidScopeEnterEx2A(H,stf,a,b)                _bid_C2(A,_bidScpON,H,stf,a,b)
#define BidScopeEnterEx3A(H,stf,a,b,c)              _bid_C3(A,_bidScpON,H,stf,a,b,c)
#define BidScopeEnterEx4A(H,stf,a,b,c,d)            _bid_C4(A,_bidScpON,H,stf,a,b,c,d)
#define BidScopeEnterEx5A(H,stf,a,b,c,d,e)          _bid_C5(A,_bidScpON,H,stf,a,b,c,d,e)
#define BidScopeEnterEx6A(H,stf,a,b,c,d,e,f)        _bid_C6(A,_bidScpON,H,stf,a,b,c,d,e,f)
#define BidScopeEnterEx7A(H,stf,a,b,c,d,e,f,g)      _bid_C7(A,_bidScpON,H,stf,a,b,c,d,e,f,g)
#define BidScopeEnterEx8A(H,stf,a,b,c,d,e,f,g,h)    _bid_C8(A,_bidScpON,H,stf,a,b,c,d,e,f,g,h)
#define BidScopeEnterEx9A(H,stf,a,b,c,d,e,f,g,h,i)  _bid_C9(A,_bidScpON,H,stf,a,b,c,d,e,f,g,h,i)

#define BidScopeEnterExW(H,stf)                     _bid_C0(W,_bidScpON,H,stf)
#define BidScopeEnterEx0W(H,stf)                    _bid_C0(W,_bidScpON,H,stf)
#define BidScopeEnterEx1W(H,stf,a)                  _bid_C1(W,_bidScpON,H,stf,a)
#define BidScopeEnterEx2W(H,stf,a,b)                _bid_C2(W,_bidScpON,H,stf,a,b)
#define BidScopeEnterEx3W(H,stf,a,b,c)              _bid_C3(W,_bidScpON,H,stf,a,b,c)
#define BidScopeEnterEx4W(H,stf,a,b,c,d)            _bid_C4(W,_bidScpON,H,stf,a,b,c,d)
#define BidScopeEnterEx5W(H,stf,a,b,c,d,e)          _bid_C5(W,_bidScpON,H,stf,a,b,c,d,e)
#define BidScopeEnterEx6W(H,stf,a,b,c,d,e,f)        _bid_C6(W,_bidScpON,H,stf,a,b,c,d,e,f)
#define BidScopeEnterEx7W(H,stf,a,b,c,d,e,f,g)      _bid_C7(W,_bidScpON,H,stf,a,b,c,d,e,f,g)
#define BidScopeEnterEx8W(H,stf,a,b,c,d,e,f,g,h)    _bid_C8(W,_bidScpON,H,stf,a,b,c,d,e,f,g,h)
#define BidScopeEnterEx9W(H,stf,a,b,c,d,e,f,g,h,i)  _bid_C9(W,_bidScpON,H,stf,a,b,c,d,e,f,g,h,i)


#define _bidCT                                  BID_HSCOPE(_bidScp)

#define BidScopeEnterA(stf)                     _bidCT; BidScopeEnterExA(&_bidScp,stf)
#define BidScopeEnter0A(stf)                    _bidCT; BidScopeEnterEx0A(&_bidScp,stf)
#define BidScopeEnter1A(stf,a)                  _bidCT; BidScopeEnterEx1A(&_bidScp,stf,a)
#define BidScopeEnter2A(stf,a,b)                _bidCT; BidScopeEnterEx2A(&_bidScp,stf,a,b)
#define BidScopeEnter3A(stf,a,b,c)              _bidCT; BidScopeEnterEx3A(&_bidScp,stf,a,b,c)
#define BidScopeEnter4A(stf,a,b,c,d)            _bidCT; BidScopeEnterEx4A(&_bidScp,stf,a,b,c,d)
#define BidScopeEnter5A(stf,a,b,c,d,e)          _bidCT; BidScopeEnterEx5A(&_bidScp,stf,a,b,c,d,e)
#define BidScopeEnter6A(stf,a,b,c,d,e,f)        _bidCT; BidScopeEnterEx6A(&_bidScp,stf,a,b,c,d,e,f)
#define BidScopeEnter7A(stf,a,b,c,d,e,f,g)      _bidCT; BidScopeEnterEx7A(&_bidScp,stf,a,b,c,d,e,f,g)
#define BidScopeEnter8A(stf,a,b,c,d,e,f,g,h)    _bidCT; BidScopeEnterEx8A(&_bidScp,stf,a,b,c,d,e,f,g,h)
#define BidScopeEnter9A(stf,a,b,c,d,e,f,g,h,i)  _bidCT; BidScopeEnterEx9A(&_bidScp,stf,a,b,c,d,e,f,g,h,i)

#define BidScopeEnterW(stf)                     _bidCT; BidScopeEnterExW(&_bidScp,stf)
#define BidScopeEnter0W(stf)                    _bidCT; BidScopeEnterEx0W(&_bidScp,stf)
#define BidScopeEnter1W(stf,a)                  _bidCT; BidScopeEnterEx1W(&_bidScp,stf,a)
#define BidScopeEnter2W(stf,a,b)                _bidCT; BidScopeEnterEx2W(&_bidScp,stf,a,b)
#define BidScopeEnter3W(stf,a,b,c)              _bidCT; BidScopeEnterEx3W(&_bidScp,stf,a,b,c)
#define BidScopeEnter4W(stf,a,b,c,d)            _bidCT; BidScopeEnterEx4W(&_bidScp,stf,a,b,c,d)
#define BidScopeEnter5W(stf,a,b,c,d,e)          _bidCT; BidScopeEnterEx5W(&_bidScp,stf,a,b,c,d,e)
#define BidScopeEnter6W(stf,a,b,c,d,e,f)        _bidCT; BidScopeEnterEx6W(&_bidScp,stf,a,b,c,d,e,f)
#define BidScopeEnter7W(stf,a,b,c,d,e,f,g)      _bidCT; BidScopeEnterEx7W(&_bidScp,stf,a,b,c,d,e,f,g)
#define BidScopeEnter8W(stf,a,b,c,d,e,f,g,h)    _bidCT; BidScopeEnterEx8W(&_bidScp,stf,a,b,c,d,e,f,g,h)
#define BidScopeEnter9W(stf,a,b,c,d,e,f,g,h,i)  _bidCT; BidScopeEnterEx9W(&_bidScp,stf,a,b,c,d,e,f,g,h,i)


#define BidScopeLeaveEx(pScp)                   _bid_COut(_bidScpON,pScp)
#define BidScopeLeave()                         BidScopeLeaveEx(&_bidScp)

#define BidScopeLeaveEx0A(hScp,stf)             BidTrace0A(stf); BidScopeLeaveEx(hScp)
#define BidScopeLeaveEx1A(hScp,stf,a)           BidTrace1A(stf,a); BidScopeLeaveEx(hScp)
#define BidScopeLeaveEx2A(hScp,stf,a,b)         BidTrace2A(stf,a,b); BidScopeLeaveEx(hScp)
#define BidScopeLeaveEx3A(hScp,stf,a,b,c)       BidTrace3A(stf,a,b,c); BidScopeLeaveEx(hScp)

#define BidScopeLeaveEx0W(hScp,stf)             BidTrace0W(stf); BidScopeLeaveEx(hScp)
#define BidScopeLeaveEx1W(hScp,stf,a)           BidTrace1W(stf,a); BidScopeLeaveEx(hScp)
#define BidScopeLeaveEx2W(hScp,stf,a,b)         BidTrace2W(stf,a,b); BidScopeLeaveEx(hScp)
#define BidScopeLeaveEx3W(hScp,stf,a,b,c)       BidTrace3W(stf,a,b,c); BidScopeLeaveEx(hScp)

#define BidScopeLeave0A(stf)                    BidTrace0A(stf); BidScopeLeave()
#define BidScopeLeave1A(stf,a)                  BidTrace1A(stf,a); BidScopeLeave()
#define BidScopeLeave2A(stf,a,b)                BidTrace2A(stf,a,b); BidScopeLeave()
#define BidScopeLeave3A(stf,a,b,c)              BidTrace3A(stf,a,b,c); BidScopeLeave()

#define BidScopeLeave0W(stf)                    BidTrace0W(stf); BidScopeLeave()
#define BidScopeLeave1W(stf,a)                  BidTrace1W(stf,a); BidScopeLeave()
#define BidScopeLeave2W(stf,a,b)                BidTrace2W(stf,a,b); BidScopeLeave()
#define BidScopeLeave3W(stf,a,b,c)              BidTrace3W(stf,a,b,c); BidScopeLeave()


#if defined( _UNICODE )
  #define BidScopeEnter         BidScopeEnterW
  #define BidScopeEnter0        BidScopeEnter0W
  #define BidScopeEnter1        BidScopeEnter1W
  #define BidScopeEnter2        BidScopeEnter2W
  #define BidScopeEnter3        BidScopeEnter3W
  #define BidScopeEnter4        BidScopeEnter4W
  #define BidScopeEnter5        BidScopeEnter5W
  #define BidScopeEnter6        BidScopeEnter6W
  #define BidScopeEnter7        BidScopeEnter7W
  #define BidScopeEnter8        BidScopeEnter8W
  #define BidScopeEnter9        BidScopeEnter9W

  #define BidScopeEnterEx       BidScopeEnterExW
  #define BidScopeEnterEx0      BidScopeEnterEx0W
  #define BidScopeEnterEx1      BidScopeEnterEx1W
  #define BidScopeEnterEx2      BidScopeEnterEx2W
  #define BidScopeEnterEx3      BidScopeEnterEx3W
  #define BidScopeEnterEx4      BidScopeEnterEx4W
  #define BidScopeEnterEx5      BidScopeEnterEx5W
  #define BidScopeEnterEx6      BidScopeEnterEx6W
  #define BidScopeEnterEx7      BidScopeEnterEx7W
  #define BidScopeEnterEx8      BidScopeEnterEx8W
  #define BidScopeEnterEx9      BidScopeEnterEx9W

  #define xBidScopeEnterV       xBidScopeEnterVW

  #define BidScopeLeave0        BidScopeLeave0W
  #define BidScopeLeave1        BidScopeLeave1W
  #define BidScopeLeave2        BidScopeLeave2W
  #define BidScopeLeave3        BidScopeLeave3W

  #define BidScopeLeaveEx0      BidScopeLeaveEx0W
  #define BidScopeLeaveEx1      BidScopeLeaveEx1W
  #define BidScopeLeaveEx2      BidScopeLeaveEx2W
  #define BidScopeLeaveEx3      BidScopeLeaveEx3W

#else
  #define BidScopeEnter         BidScopeEnterA
  #define BidScopeEnter0        BidScopeEnter0A
  #define BidScopeEnter1        BidScopeEnter1A
  #define BidScopeEnter2        BidScopeEnter2A
  #define BidScopeEnter3        BidScopeEnter3A
  #define BidScopeEnter4        BidScopeEnter4A
  #define BidScopeEnter5        BidScopeEnter5A
  #define BidScopeEnter6        BidScopeEnter6A
  #define BidScopeEnter7        BidScopeEnter7A
  #define BidScopeEnter8        BidScopeEnter8A
  #define BidScopeEnter9        BidScopeEnter9A

  #define BidScopeEnterEx       BidScopeEnterExA
  #define BidScopeEnterEx0      BidScopeEnterEx0A
  #define BidScopeEnterEx1      BidScopeEnterEx1A
  #define BidScopeEnterEx2      BidScopeEnterEx2A
  #define BidScopeEnterEx3      BidScopeEnterEx3A
  #define BidScopeEnterEx4      BidScopeEnterEx4A
  #define BidScopeEnterEx5      BidScopeEnterEx5A
  #define BidScopeEnterEx6      BidScopeEnterEx6A
  #define BidScopeEnterEx7      BidScopeEnterEx7A
  #define BidScopeEnterEx8      BidScopeEnterEx8A
  #define BidScopeEnterEx9      BidScopeEnterEx9A

  #define xBidScopeEnterV       xBidScopeEnterVA

  #define BidScopeLeave0        BidScopeLeave0A
  #define BidScopeLeave1        BidScopeLeave1A
  #define BidScopeLeave2        BidScopeLeave2A
  #define BidScopeLeave3        BidScopeLeave3A

  #define BidScopeLeaveEx0      BidScopeLeaveEx0A
  #define BidScopeLeaveEx1      BidScopeLeaveEx1A
  #define BidScopeLeaveEx2      BidScopeLeaveEx2A
  #define BidScopeLeaveEx3      BidScopeLeaveEx3A
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Scope Tracking Automatic Wrapper
//
#if defined( __cplusplus )

    struct _bidCAutoScopeAnchor
    {
        _bidCAutoScopeAnchor()      { m_hScp = BID_NOHANDLE; }
        ~_bidCAutoScopeAnchor()     { if( !Out() ) DBREAK(); }
        BOOL    Out();
        HANDLE* operator &()        { return &m_hScp; }

     private:
        HANDLE  m_hScp;
    };

    #define _bidCTA                                 _bidCAutoScopeAnchor _bidScp

    #define BidScopeAutoA(stf)                      _bidCTA; BidScopeEnterExA(&_bidScp,stf)
    #define BidScopeAuto0A(stf)                     _bidCTA; BidScopeEnterEx0A(&_bidScp,stf)
    #define BidScopeAuto1A(stf,a)                   _bidCTA; BidScopeEnterEx1A(&_bidScp,stf,a)
    #define BidScopeAuto2A(stf,a,b)                 _bidCTA; BidScopeEnterEx2A(&_bidScp,stf,a,b)
    #define BidScopeAuto3A(stf,a,b,c)               _bidCTA; BidScopeEnterEx3A(&_bidScp,stf,a,b,c)
    #define BidScopeAuto4A(stf,a,b,c,d)             _bidCTA; BidScopeEnterEx4A(&_bidScp,stf,a,b,c,d)
    #define BidScopeAuto5A(stf,a,b,c,d,e)           _bidCTA; BidScopeEnterEx5A(&_bidScp,stf,a,b,c,d,e)
    #define BidScopeAuto6A(stf,a,b,c,d,e,f)         _bidCTA; BidScopeEnterEx6A(&_bidScp,stf,a,b,c,d,e,f)
    #define BidScopeAuto7A(stf,a,b,c,d,e,f,g)       _bidCTA; BidScopeEnterEx7A(&_bidScp,stf,a,b,c,d,e,f,g)
    #define BidScopeAuto8A(stf,a,b,c,d,e,f,g,h)     _bidCTA; BidScopeEnterEx8A(&_bidScp,stf,a,b,c,d,e,f,g,h)
    #define BidScopeAuto9A(stf,a,b,c,d,e,f,g,h,i)   _bidCTA; BidScopeEnterEx9A(&_bidScp,stf,a,b,c,d,e,f,g,h,i)

    #define BidScopeAutoW(stf)                      _bidCTA; BidScopeEnterExW(&_bidScp,stf)
    #define BidScopeAuto0W(stf)                     _bidCTA; BidScopeEnterEx0W(&_bidScp,stf)
    #define BidScopeAuto1W(stf,a)                   _bidCTA; BidScopeEnterEx1W(&_bidScp,stf,a)
    #define BidScopeAuto2W(stf,a,b)                 _bidCTA; BidScopeEnterEx2W(&_bidScp,stf,a,b)
    #define BidScopeAuto3W(stf,a,b,c)               _bidCTA; BidScopeEnterEx3W(&_bidScp,stf,a,b,c)
    #define BidScopeAuto4W(stf,a,b,c,d)             _bidCTA; BidScopeEnterEx4W(&_bidScp,stf,a,b,c,d)
    #define BidScopeAuto5W(stf,a,b,c,d,e)           _bidCTA; BidScopeEnterEx5W(&_bidScp,stf,a,b,c,d,e)
    #define BidScopeAuto6W(stf,a,b,c,d,e,f)         _bidCTA; BidScopeEnterEx6W(&_bidScp,stf,a,b,c,d,e,f)
    #define BidScopeAuto7W(stf,a,b,c,d,e,f,g)       _bidCTA; BidScopeEnterEx7W(&_bidScp,stf,a,b,c,d,e,f,g)
    #define BidScopeAuto8W(stf,a,b,c,d,e,f,g,h)     _bidCTA; BidScopeEnterEx8W(&_bidScp,stf,a,b,c,d,e,f,g,h)
    #define BidScopeAuto9W(stf,a,b,c,d,e,f,g,h,i)   _bidCTA; BidScopeEnterEx9W(&_bidScp,stf,a,b,c,d,e,f,g,h,i)

    #define BidScopeAuto_Term()                     if( !_bidScp.Out() ){ DBREAK();  }

    #if defined( _UNICODE )
      #define BidScopeAuto      BidScopeAutoW
      #define BidScopeAuto0     BidScopeAuto0W
      #define BidScopeAuto1     BidScopeAuto1W
      #define BidScopeAuto2     BidScopeAuto2W
      #define BidScopeAuto3     BidScopeAuto3W
      #define BidScopeAuto4     BidScopeAuto4W
      #define BidScopeAuto5     BidScopeAuto5W
      #define BidScopeAuto6     BidScopeAuto6W
      #define BidScopeAuto7     BidScopeAuto7W
      #define BidScopeAuto8     BidScopeAuto8W
      #define BidScopeAuto9     BidScopeAuto9W
    #else
      #define BidScopeAuto      BidScopeAutoA
      #define BidScopeAuto0     BidScopeAuto0A
      #define BidScopeAuto1     BidScopeAuto1A
      #define BidScopeAuto2     BidScopeAuto2A
      #define BidScopeAuto3     BidScopeAuto3A
      #define BidScopeAuto4     BidScopeAuto4A
      #define BidScopeAuto5     BidScopeAuto5A
      #define BidScopeAuto6     BidScopeAuto6A
      #define BidScopeAuto7     BidScopeAuto7A
      #define BidScopeAuto8     BidScopeAuto8A
      #define BidScopeAuto9     BidScopeAuto9A
    #endif

#endif  // __cplusplus


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                       OUTPUT CONTROL                                        //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Explicit TCS check (TCS stands for "Trace Control String")
//
_bid_DECL(BOOL, BidEnabledA, (HANDLE hID, UINT_PTR src, UINT_PTR info, PCSTR  tcs));
_bid_DECL(BOOL, BidEnabledW, (HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR tcs));

#define xBidEnabledA(src,info,tcs)  (_bidID_IF ? (*_bidPfn.BidEnabledA)(_bidID,src,info,tcs) : FALSE)
#define xBidEnabledW(src,info,tcs)  (_bidID_IF ? (*_bidPfn.BidEnabledW)(_bidID,src,info,tcs) : FALSE)

#define BidIsEnabledExUA(var,bit,flg,tcs)                                               \
        do {                                                                            \
            _bid_SRCINFO                                                                \
            _bid_TCFS_A(_bidN(034), tcs)                                                \
            (var) = (bit && _bidN(034) && xBidEnabledA(_bidSF,_bidLF(flg),_bidN(034))); \
        } while(0)

#define BidIsEnabledExUW(var,bit,flg,tcs)                                               \
        do {                                                                            \
            _bid_SRCINFO                                                                \
            _bid_TCFS_W(_bidN(034), tcs)                                                \
            (var) = (bit && _bidN(034) && xBidEnabledW(_bidSF,_bidLF(flg),_bidN(034))); \
        } while(0)

#define BidIsEnabledUA(var,bit,tcs)         BidIsEnabledExUA(var,bit,0,tcs)
#define BidIsEnabledUW(var,bit,tcs)         BidIsEnabledExUW(var,bit,0,tcs)

#define BidIsEnabledExA(var,f,tcs)          BidIsEnabledExUA(var,_bidTrON,f,tcs)
#define BidIsEnabledExW(var,f,tcs)          BidIsEnabledExUW(var,_bidTrON,f,tcs)

#define BidIsEnabledA(var,tcs)              BidIsEnabledExA(var,0,tcs)
#define BidIsEnabledW(var,tcs)              BidIsEnabledExW(var,0,tcs)

#define IF_BidEnabledExUA(var,bit,flg,tcs)  BidIsEnabledExUA(var,bit,flg,tcs); if(var)
#define IF_BidEnabledExUW(var,bit,flg,tcs)  BidIsEnabledExUW(var,bit,flg,tcs); if(var)

#define IF_BidEnabledUA(var,bit,tcs)        IF_BidEnabledExUA(var,bit,0,tcs)
#define IF_BidEnabledUW(var,bit,tcs)        IF_BidEnabledExUW(var,bit,0,tcs)

#define IF_BidEnabledExA(var,flg,tcs)       IF_BidEnabledExUA(var,_bidTrON,flg,tcs)
#define IF_BidEnabledExW(var,flg,tcs)       IF_BidEnabledExUW(var,_bidTrON,flg,tcs)

#define IF_BidEnabledA(var,tcs)             IF_BidEnabledExA(var,0,tcs)
#define IF_BidEnabledW(var,tcs)             IF_BidEnabledExW(var,0,tcs)


#if defined( _UNICODE )
  #define xBidEnabled               xBidEnabledW
  #define BidIsEnabledExU           BidIsEnabledExUW
  #define BidIsEnabledU             BidIsEnabledUW
  #define BidIsEnabledEx            BidIsEnabledExW
  #define BidIsEnabled              BidIsEnabledW
  #define IF_BidEnabledExU          IF_BidEnabledExUW
  #define IF_BidEnabledU            IF_BidEnabledUW
  #define IF_BidEnabledEx           IF_BidEnabledExW
  #define IF_BidEnabled             IF_BidEnabledW
#else
  #define xBidEnabled               xBidEnabledA
  #define BidIsEnabledExU           BidIsEnabledExUA
  #define BidIsEnabledU             BidIsEnabledUA
  #define BidIsEnabledEx            BidIsEnabledExA
  #define BidIsEnabled              BidIsEnabledA
  #define IF_BidEnabledExU          IF_BidEnabledExUA
  #define IF_BidEnabledU            IF_BidEnabledUA
  #define IF_BidEnabledEx           IF_BidEnabledExA
  #define IF_BidEnabled             IF_BidEnabledA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Indentation
//
_bid_DECL( int, BidIndent, (HANDLE hID, int nIdx) );

#define BID_INDENT_IN               -1
#define BID_INDENT_OUT              -3

#define xBidIndent(hID,a)           (*_bidPfn.BidIndent)(hID,a)
#define _bidIndent(f,nIdx)          (((f) && _bidID_IF) ? xBidIndent(_bidID,nIdx) : 0)

#define BidIndentU_In(f)            (void)_bidIndent(f, BID_INDENT_IN)
#define BidIndentU_Out(f)           (void)_bidIndent(f, BID_INDENT_OUT)

#define BidIndent_In()              BidIndentU_In(_bidTrON)
#define BidIndent_Out()             BidIndentU_Out(_bidTrON)



/////////////////////////////////////////////////////////////////////////////////////////////////
//                              PERFORMANCE / RESOURCE MONITORING                              //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Event Registration Flags
//
#define BID_SNAPTYPE_FLAGS          0xFFFFF000
#define BID_SNAPTYPE_ENUM           0x00000FFF
#define BID_SNAPTYPE_SUBENUM        0x00000007

#define BID_SNAPTYPE_PERF           (1 * 8)
#define BID_SNAPTYPE_RSRC           (2 * 8)
#define BID_SNAPTYPE_MEM            (3 * 8)

#define BID_SNAPTYPE_COUNTER        (BID_SNAPTYPE_RSRC + 1)
  #define BID_SNAPTYPE_COUNTER_MO   0x00001000


//
//  EvtIDs
//
#define BID_MakeDynamicEvtID(id)    (INT_PTR)((id) << 2)
#define BID_GetEvtID(id)            ((INT_PTR)(id) >> 2)

#define BID_EVTID_DYNAMIC_MODE2     0x2

//
//  Initial value for EvtIDs
//
#ifdef _DEBUG_TRACE_ON
  #define _bidD                     BID_MakeDynamicEvtID(-1)
#else
  #define _bidD                     0
#endif

//
//  Static EvtIDs
//
#define BID_EVTID_BASE_USER         0
#define BID_EVTID_BASE_USER_MAX     0x0FFFFFFF
#define BID_EVTID_BASE_BID          0x10000000

#define BID_MakeStaticEvtID(id)     (BID_MakeDynamicEvtID(id) | 0x1)
#define BID_IsStaticEvt(id)         (((id) & 0x1) != 0)



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Event Registration
//
_bid_INLINE void WINAPI xBidGetEventIDA(INT_PTR* pEvtID, DWORD flags, PCSTR  textID);
_bid_INLINE void WINAPI xBidGetEventIDW(INT_PTR* pEvtID, DWORD flags, PCWSTR textID);
_bid_INLINE void WINAPI xBidRemoveEvent(INT_PTR* pEvtID, DWORD flags);

#define BidGetEventIDExA(pID,flg,txt)   do{ _bid_EDS_A(_bidN(050), txt)           \
                                            xBidGetEventIDA(pID, flg, _bidN(050));\
                                        }while(0)

#define BidGetEventIDExW(pID,flg,txt)   do{ _bid_EDS_W(_bidN(050), txt)           \
                                            xBidGetEventIDW(pID, flg, _bidN(050));\
                                        }while(0)

#define BidRemoveEventEx            xBidRemoveEvent

#if defined( _UNICODE )
  #define BidGetEventIDEx           BidGetEventIDExW
  #define xBidGetEventID            xBidGetEventIDW
#else
  #define BidGetEventIDEx           BidGetEventIDExA
  #define xBidGetEventID            xBidGetEventIDA
#endif



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Event Auto-Registration
//
typedef struct __bidEventRegA
{
    PCSTR       TextID;
    INT_PTR*    pEvtID;
    DWORD       Flags;

} BIDEVENTREGA, * PBIDEVENTREGA;

typedef struct __bidEventRegW
{
    PCWSTR      TextID;
    INT_PTR*    pEvtID;
    DWORD       Flags;

} BIDEVENTREGW, * PBIDEVENTREGW;


#ifdef _BID_NO_SPECIAL_ALLOCATION

  #define _bid_EVENT_A(pEvtID,flags,textID)  _erEr  _BID_NO_SPECIAL_ALLOCATION__EVENT_wont_work
  #define _bid_EVENT_W(pEvtID,flags,textID)  _erEr  _BID_NO_SPECIAL_ALLOCATION__EVENT_wont_work

#else

  #ifdef _BID_PACK_EDS

    #define _bid_EVENT_A(pEvtID,flags,textID)                                       \
                _bid_EXTERN_C                                                       \
                __declspec(allocate("BID$A072"))                                    \
                _bid_V BIDEVENTREGA _bidN(070) = {(PCSTR)_bidP,  pEvtID, (flags)};  \
                _bid_FORCEINC(_bidN(070))

    #define _bid_EVENT_W(pEvtID,flags,textID)                                       \
                _bid_EXTERN_C                                                       \
                __declspec(allocate("BID$W072"))                                    \
                _bid_V BIDEVENTREGW _bidN(070) = {(PCWSTR)_bidP, pEvtID, (flags)};  \
                _bid_FORCEINC(_bidN(070))
  #else

    #define _bid_EVENT_A(pEvtID,flags,textID)                                       \
                _bid_EXTERN_C                                                       \
                __declspec(allocate("BID$A072"))                                    \
                _bid_V BIDEVENTREGA _bidN(070) = { textID, pEvtID, (flags) };       \
                _bid_FORCEINC(_bidN(070))

    #define _bid_EVENT_W(pEvtID,flags,textID)                                       \
                _bid_EXTERN_C                                                       \
                __declspec(allocate("BID$W072"))                                    \
                _bid_V BIDEVENTREGW _bidN(070) = { textID, pEvtID, (flags) };       \
                _bid_FORCEINC(_bidN(070))

  #endif

#endif

#define BID_EVENT_EXA(pEvt,flg,txt) _bid_EVENT_A(pEvt,flg,txt)
#define BID_EVENT_EXW(pEvt,flg,txt) _bid_EVENT_W(pEvt,flg,txt)

#if defined( _UNICODE )
  #define PBIDEVENTREG              PBIDEVENTREGW
  #define BID_EVENT_EX              BID_EVENT_EXW
#else
  #define PBIDEVENTREG              PBIDEVENTREGA
  #define BID_EVENT_EX              BID_EVENT_EXA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Event Publishing
//
_bid_DECL( INT_PTR, BidSnap,        (HANDLE hID, INT_PTR evtID, INT_PTR arg1, INT_PTR arg2) );

#define _bidSnap(h,a,b,c)           (*_bidPfn.BidSnap)(h,a,b,c)
#define xBidSnap(a,b,c)             (_bidID_IF ? _bidSnap(_bidID,a,b,c) : 0)

#define BidSnapEx(bON,evtID,b,c)    _bid_DO                                                     \
                                    if(bON && evtID){ xBidSnap(evtID, (INT_PTR)b, (INT_PTR)c); }\
                                    _bid_WHILE0

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  General Resource-Tracking Events
//
#define BidGetEventIDA(pID,txt)     BidGetEventIDExA(pID, BID_SNAPTYPE_RSRC, txt)
#define BidGetEventIDW(pID,txt)     BidGetEventIDExW(pID, BID_SNAPTYPE_RSRC, txt)
#define BidRemoveEvent(pID)         BidRemoveEventEx(pID, BID_SNAPTYPE_RSRC)

#define BID_EVENTA(evt,txt)         static INT_PTR evt = _bidD; \
                                    BID_EVENT_EXA(&evt, BID_SNAPTYPE_RSRC, txt)

#define BID_EVENTW(evt,txt)         static INT_PTR evt = _bidD; \
                                    BID_EVENT_EXW(&evt, BID_SNAPTYPE_RSRC, txt)

#if defined( _UNICODE )
  #define BidGetEventID             BidGetEventIDW
  #define BID_EVENT                 BID_EVENTW
#else
  #define BidGetEventID             BidGetEventIDA
  #define BID_EVENT                 BID_EVENTA
#endif

#define BidSnap(evtID,arg)          BidSnapEx(_bidRsrcON, evtID, arg, 0)
#define BidSnap2(evtID,arg1,arg2)   BidSnapEx(_bidRsrcON, evtID, arg1, arg2)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Specialization for Resource/Performace Counters
//
#define BidGetCounterIDA(pEvt,txt)  BidGetEventIDExA(pEvt, BID_SNAPTYPE_COUNTER, txt)
#define BidGetCounterIDW(pEvt,txt)  BidGetEventIDExW(pEvt, BID_SNAPTYPE_COUNTER, txt)
#define BidRemoveCounter(pEvt)      BidRemoveEventEx(pEvt, BID_SNAPTYPE_COUNTER)

#define BID_COUNTER_EXA(pEvt,txt)   BID_EVENT_EXA(pEvt, BID_SNAPTYPE_COUNTER, txt)
#define BID_COUNTER_EXW(pEvt,txt)   BID_EVENT_EXW(pEvt, BID_SNAPTYPE_COUNTER, txt)

#define BID_COUNTERA(evt,txt)       static INT_PTR evt = _bidD; \
                                    BID_COUNTER_EXA(&evt, txt)

#define BID_COUNTERW(evt,txt)       static INT_PTR evt = _bidD; \
                                    BID_COUNTER_EXW(&evt, txt)

#if defined( _UNICODE )
  #define BidGetCounterID           BidGetCounterIDW
  #define BID_COUNTER               BID_COUNTERW
  #define BID_COUNTER_EX            BID_COUNTER_EXW
#else
  #define BidGetCounterID           BidGetCounterIDA
  #define BID_COUNTER               BID_COUNTERA
  #define BID_COUNTER_EX            BID_COUNTER_EXA
#endif

#define BidSnapIncEx(evtID,N,objID) BidSnapEx(_bidRsrcON, evtID, N, objID)
#define BidSnapInc(evtID,N)         BidSnapEx(_bidRsrcON, evtID, N, 0)

#define BidSnapDecEx(evtID,N,objID) BidSnapEx(_bidRsrcON, evtID, -(N), objID)
#define BidSnapDec(evtID,N)         BidSnapEx(_bidRsrcON, evtID, -(N), 0)

#define BidSnapSetEx(evtID,N,objID)                                                         \
            _bid_DO                                                                         \
                if(_bidRsrcON && evtID){                                                    \
                    xBidSnap((evtID)|BID_EVTID_DYNAMIC_MODE2, (INT_PTR)N, (INT_PTR)objID);  \
                }                                                                           \
            _bid_WHILE0

#define BidSnapSet(evtID,N)         BidSnapSetEx(evtID,N,0)

#define BidSnapResetEx(evtID,objID) BidSnapSetEx(evtID,0,objID)
#define BidSnapReset(evtID)         BidSnapSetEx(evtID,0,0)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Specialization for Standard Performance Profiling (SPP)
//
#define BID_SNAP_START              1
#define BID_SNAP_SUSPEND            2
#define BID_SNAP_RESUME             3
#define BID_SNAP_STOP               4

#define BID_EVENT_PERFA(evt,txt)    static INT_PTR evt = _bidD; \
                                    BID_EVENT_EXA(&evt, BID_SNAPTYPE_PERF, txt)

#define BID_EVENT_PERFW(evt,txt)    static INT_PTR evt = _bidD; \
                                    BID_EVENT_EXW(&evt, BID_SNAPTYPE_PERF, txt)

#if defined( _UNICODE )
  #define BID_EVENT_PERF            BID_EVENT_PERFW
#else
  #define BID_EVENT_PERF            BID_EVENT_PERFA
#endif

//
//  TBD...
//
#define BidSnapStartEx(evt,id)      BidSnapEx(_bidPerfON, evt, BID_SNAP_START, id)
#define BidSnapSuspendEx(evt,id)    BidSnapEx(_bidPerfON, evt, BID_SNAP_SUSPEND, id)
#define BidSnapResumeEx(evt,id)     BidSnapEx(_bidPerfON, evt, BID_SNAP_RESUME, id)
#define BidSnapStopEx(evt,id)       BidSnapEx(_bidPerfON, evt, BID_SNAP_STOP, id)

#if 0   // ++ ToBe Removed...

#ifdef _BID_EVTREF_FIXED
  #define _bid_EVTREF(evtID)        evtID
#else
  #define _bid_EVTREF(evtID)        * _bidN(070).pEvtID
#endif

#define BidSnapDeclAndStartEx(evtID,txt,id)                                         \
                                    static INT_PTR evtID = _bidD;                   \
                                    BID_EVENT_EX(&evtID, BID_SNAPTYPE_PERF, txt)    \
                                    if( _bidPerfON && evtID ){                      \
                                        xBidSnap( _bid_EVTREF(evtID),               \
                                                    BID_SNAP_START, (INT_PTR)id );  \
                                    }

#define BidSnapDeclAndStartExA(evtID,txt,id)                                        \
                                    static INT_PTR evtID = _bidD;                   \
                                    BID_EVENT_EXA(&evtID, BID_SNAPTYPE_PERF, txt)   \
                                    if( _bidPerfON && evtID ){                      \
                                        xBidSnap( _bid_EVTREF(evtID),               \
                                                    BID_SNAP_START, (INT_PTR)id );  \
                                    }

//
//  These wrappers utilize the static variable '_bidSppEvt' and are supposed to be used
//  in the local scope of a function.
//
#define BidSnapDeclAndStart(txt,id) BidSnapDeclAndStartEx(_bidSppEvt,txt,id)
#define BidSnapDeclAndStartA(txt,id) BidSnapDeclAndStartExA(_bidSppEvt,txt,id)

#define BidSnapSuspend(id)          BidSnapSuspendEx(_bidSppEvt,id)
#define BidSnapResume(id)           BidSnapResumeEx(_bidSppEvt,id)
#define BidSnapStop(id)             BidSnapStopEx(_bidSppEvt,id)

//
//  Synonyms
//
#define BID_SNAP_YIELD              BID_SNAP_SUSPEND
#define BID_SNAP_CONTINUE           BID_SNAP_RESUME
#define BID_SNAP_END                BID_SNAP_STOP

#define BidSnapYield                BidSnapSuspend
#define BidSnapContinue             BidSnapResume
#define BidSnapEnd                  BidSnapStop

#define BidSnapYieldEx              BidSnapSuspendEx
#define BidSnapContinueEx           BidSnapResumeEx
#define BidSnapEndEx                BidSnapStopEx

#endif  // -- ToBe Removed.

//
//  Automatic wrapper for performance profiling (TBD..)
//
#if defined( __cplusplus )

    struct _bidCAutoPerf
    {
        inline _bidCAutoPerf(INT_PTR& rEvt, INT_PTR obj);   // implementation
        inline ~_bidCAutoPerf();                            // at the bottom of file

        INT_PTR ObjID() const       { return m_ID;     }
        INT_PTR EvtID() const       { return *m_pEvt;  }
        operator INT_PTR() const    { return *m_pEvt;  }

     private:
        INT_PTR*    m_pEvt;
        INT_PTR     m_ID;
    };

    #define BidSnapAuto(evt,id)     _bidCAutoPerf _bidN(074)((INT_PTR)(evt),(INT_PTR)(id))

    #define BidSnapDeclAutoEx(evt,txt,id)                                                   \
                                    static INT_PTR _bidN(075) = _bidD;                      \
                                    BID_EVENT_EX(&_bidN(075), BID_SNAPTYPE_PERF, txt)       \
                                    _bidCAutoPerf evt (_bid_EVTREF(_bidN(075)),(INT_PTR)(id))

    #define BidSnapDeclAutoExA(evt,txt,id)                                                  \
                                    static INT_PTR _bidN(075) = _bidD;                      \
                                    BID_EVENT_EXA(&_bidN(075), BID_SNAPTYPE_PERF, txt)      \
                                    _bidCAutoPerf evt (_bid_EVTREF(_bidN(075)),(INT_PTR)(id))

    #define BidSnapDeclAuto(txt,id)  BidSnapDeclAutoEx(_bidSppEvt, txt,id)
    #define BidSnapDeclAutoA(txt,id) BidSnapDeclAutoExA(_bidSppEvt,txt,id)

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                          SERVICES                                           //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  TRACING HRESULTs, RETURN CODEs AND SOURCE MARKERs
//
//  BID_SRCFILE statement must be visible in the scope of use of the APIs listed in this section.
//
#define BidTraceHR(hr)      (_bidTrON ? _bidTraceHR(_bidSF2, _bidLF2(BID_SLN), (hr)) : (hr))

HRESULT WINAPI _bidTraceHR  (UINT_PTR src, UINT_PTR info, HRESULT hr);

//
//  Assumes that Win32 never uses ErrCode 0x7FFFFFFF.
//
#define BID_AUTOERRCODE     (DWORD)(0x7FFFFFFF)

#define BidTraceErr(dw)     (_bidTrON ? _bidTraceErr(_bidSF2, _bidLF2(BID_SLN), (dw)) : (dw))

#define BidCHK(api)         ((api) ? TRUE : _bidTrON                                        \
                            ? _bidTraceErr(_bidSF2,_bidLF2(BID_SLN),BID_AUTOERRCODE), FALSE \
                            : FALSE)

DWORD   WINAPI _bidTraceErr (UINT_PTR src, UINT_PTR info, DWORD errCode);

//
//  Traces the fact of visiting a certain line of source code
//
BOOL    WINAPI _bidTraceMark(UINT_PTR src, UINT_PTR info);

#define BidTraceMarkExU(bit) if((bit) && !_bidTraceMark(_bidSF2, _bidLF2(BID_SLN))){ DBREAK(); }
#define BidTraceMarkU(bit)  do{ BidTraceMarkExU(bit); }while(0)
#define BidTraceMark()      BidTraceMarkU(_bidTrON)

//
//  Helpers that combine retcode tracing and leaving the scope (useful in plain C code)
//
HRESULT WINAPI _bidScopeLeaveHR(UINT_PTR src, UINT_PTR info, HANDLE* pHScp, HRESULT hr);
BOOL    WINAPI _bidScopeLeaveErr(UINT_PTR src, UINT_PTR info, HANDLE* pHScp, BOOL bApi);

//
//  The following macros use the 2nd flavor of _bidSF and _bidLF macros (_bidSF2, _bidLF2)
//  which means "Demand SrcFile/LineNum info". However, the support functions _bidScopeLeaveXX
//  turn off the flag BID_DEMAND_SRC. Doing that, we have SrcInfo available for BidScopeLeave
//  even if the module was compiled with _BID_NO_SRCINFO, but it's turned off by default.
//
#define BidScopeLeaveHREx(pHScp, hr)    (_bidScpTrON                                        \
                                        ? _bidScopeLeaveHR(_bidSF2, _bidLF2(0), pHScp, hr)  \
                                        : hr)

#define BidScopeLeaveHR(hr)             BidScopeLeaveHREx(&_bidScp, hr)

#define BidScopeLeaveErrEx(pHScp, api)  (_bidScpTrON                                        \
                                        ? _bidScopeLeaveErr(_bidSF2, _bidLF2(0), pHScp, api)\
                                        : api)

#define BidScopeLeaveErr(api)           BidScopeLeaveErrEx(&_bidScp, api)



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  String Digest
//
#ifndef _BID_NO_STRDIGEST

typedef struct _bidCtrlCharSubstA
{
    char    CR;
    char    LF;
    char    HT;
    char    Lead;

} BID_CtrlCharSubstA;

typedef struct _bidCtrlCharSubstW
{
    WCHAR   CR;
    WCHAR   LF;
    WCHAR   HT;
    WCHAR   Lead;

} BID_CtrlCharSubstW;

void WINAPI BID_InitCtrlCharSubstA( BID_CtrlCharSubstA* );
void WINAPI BID_InitCtrlCharSubstW( BID_CtrlCharSubstW* );

#ifdef _UNICODE
  #define BID_CtrlCharSubst         BID_CtrlCharSubstW
  #define BID_InitCtrlCharSubst     BID_InitCtrlCharSubstW
#else
  #define BID_CtrlCharSubst         BID_CtrlCharSubstA
  #define BID_InitCtrlCharSubst     BID_InitCtrlCharSubstA
#endif


PCSTR   WINAPI xBidStrDigestExA(__out_ecount(outBufCapacity) PSTR outBuf,
                                __in int outBufCapacity,
                                __in PCSTR textStr, __in int textStrLen,
                                __in_opt const BID_CtrlCharSubstA* pCCS);

PCWSTR  WINAPI xBidStrDigestExW(__out_ecount(outBufCapacity) PWSTR outBuf,
                                __in int outBufCapacity,
                                __in PCWSTR textStr, __in int textStrLen,
                                __in_opt const BID_CtrlCharSubstW* pCCS);

#define BidStrDigestExA(outBuf,outBufCapacity,textStr,strLen) \
                                    xBidStrDigestExA(outBuf,outBufCapacity,textStr,strLen,NULL)

#define BidStrDigestExW(outBuf,outBufCapacity,textStr,strLen) \
                                    xBidStrDigestExW(outBuf,outBufCapacity,textStr,strLen,NULL)

#define BidStrDigestA(outBuf,outBufCapacity,textStr) \
                                    BidStrDigestExA(outBuf,outBufCapacity,textStr,-1)

#define BidStrDigestW(outBuf,outBufCapacity,textStr) \
                                    BidStrDigestExW(outBuf,outBufCapacity,textStr,-1)

#ifdef _UNICODE
  #define xBidStrDigestEx           xBidStrDigestExW
  #define BidStrDigestEx            BidStrDigestExW
  #define BidStrDigest              BidStrDigestW
#else
  #define xBidStrDigestEx           xBidStrDigestExA
  #define BidStrDigestEx            BidStrDigestExA
  #define BidStrDigest              BidStrDigestA
#endif


#endif // _BID_NO_STRDIGEST


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  RESOURCE DLL LOADING NOTIFICATION (such as Language-Specific Resource DLL)
//
_bid_INLINE void WINAPI xBidAddResHandle    (DWORD flags, HMODULE hRes);
_bid_INLINE void WINAPI xBidRemoveResHandle (DWORD flags, HMODULE hRes);

#define BidAddResHandleEx           xBidAddResHandle
#define BidRemoveResHandleEx        xBidRemoveResHandle

#define BidAddResHandle(hRes)       BidAddResHandleEx(0, hRes)
#define BidRemoveResHandle(hRes)    BidRemoveResHandleEx(0, hRes)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  EXPLICIT STRING PARSING
//
//  Note: C/C++ code usually doesn't need to call these functions,
//  unless _BID_NO_SPECIAL_ALLOCATION symbol is defined.
//
_bid_INLINE PCSTR  WINAPI xBidParseStringA(DWORD flags, PCSTR  tcfs);
_bid_INLINE PCWSTR WINAPI xBidParseStringW(DWORD flags, PCWSTR tcfs);

#define BidParseStringExA           xBidParseStringA
#define BidParseStringExW           xBidParseStringW

#define BidParseStringA(txt)        BidParseStringExA(0,txt)
#define BidParseStringW(txt)        BidParseStringExW(0,txt)

#if defined( _UNICODE )
  #define BidParseString            BidParseStringW
  #define BidParseStringEx          BidParseStringExW
  #define xBidParseString           xBidParseStringW
#else
  #define BidParseString            BidParseStringA
  #define BidParseStringEx          BidParseStringExA
  #define xBidParseString           xBidParseStringA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  STRUCTURED TRACING (BidExtensions)
//
typedef void (WINAPI* BIDEXTPROC)(HANDLE hCtx, PCVOID pItem, DWORD attr, INT_PTR dataEx);

_bid_INLINE void WINAPI xBidAddExtensionA(PCSTR  textID, BIDEXTPROC extProc, INT_PTR dataEx);
_bid_INLINE void WINAPI xBidAddExtensionW(PCWSTR textID, BIDEXTPROC extProc, INT_PTR dataEx);

#define BidAddExtensionExA(txt,f,d) xBidAddExtensionA(txt,f,(INT_PTR)(d))
#define BidAddExtensionExW(txt,f,d) xBidAddExtensionW(txt,f,(INT_PTR)(d))

#define BidAddExtensionA(txt,f)     xBidAddExtensionA(txt,f,0)
#define BidAddExtensionW(txt,f)     xBidAddExtensionW(txt,f,0)

#if defined( _UNICODE )
  #define BidAddExtension           BidAddExtensionW
  #define BidAddExtensionEx         BidAddExtensionExW
  #define xBidAddExtension          xBidAddExtensionW
#else
  #define BidAddExtension           BidAddExtensionA
  #define BidAddExtensionEx         BidAddExtensionExA
  #define xBidAddExtension          xBidAddExtensionA
#endif

//
//  'attr' layout
//
#define BID_LevelOfDetailsEx(attr)  (int)(attr & 0x07)
  #define BID_DETAILS_MIN           1
  #define BID_DETAILS_STD           2
  #define BID_DETAILS_MAX           7

#define BID_AttrFlagsEx(attr)       (DWORD)(attr & ~0x1F)

#define BID_InBinaryModeEx(attr)    ((attr & BID_DETAILS_MODE_BINARY) != 0)
#define BID_InDiscoveryModeEx(attr) ((attr & BID_DETAILS_MODE_DISCO) != 0)
  #define BID_DETAILS_MODE_BINARY   0x08
  #define BID_DETAILS_MODE_DISCO    0x10

#define BID_LevelOfDetails()        BID_LevelOfDetailsEx(_bidExt_attr)
#define BID_AttrFlags()             BID_AttrFlagsEx(_bidExt_attr)

#define BID_InBinaryMode()          BID_InBinaryModeEx(_bidExt_attr)
#define BID_InDiscoveryMode()       BID_InDiscoveryModeEx(_bidExt_attr)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidExtension Auto Registration
//
typedef struct __bidExtRegA
{
    PCSTR       TextID;
    BIDEXTPROC  ExtProc;
    INT_PTR     DataEx;

} BIDEXTREGA, * PBIDEXTREGA;

typedef struct __bidExtRegW
{
    PCWSTR      TextID;
    BIDEXTPROC  ExtProc;
    INT_PTR     DataEx;

} BIDEXTREGW, * PBIDEXTREGW;


#ifdef _BID_NO_SPECIAL_ALLOCATION

  #define _bid_EXT_A(textID,fnEx,dataEx)  _erEr  _BID_NO_SPECIAL_ALLOCATION__BID_EXT_wont_work
  #define _bid_EXT_W(textID,fnEx,dataEx)  _erEr  _BID_NO_SPECIAL_ALLOCATION__BID_EXT_wont_work

#else

  #ifdef _BID_PACK_EXAR

    #define _bid_EXT_A(textID,fnEx,dataEx)          \
                _bid_FORCEINC(_bidN(060)_##fnEx)    \
                _bid_EXTERN_C                       \
                __declspec(allocate("BID$A062"))    \
                _bid_V BIDEXTREGA _bidN(060)_##fnEx = { (PCSTR)_bidP,  fnEx, (INT_PTR)(dataEx) };

    #define _bid_EXT_W(textID,fnEx,dataEx)          \
                _bid_FORCEINC(_bidN(060)_##fnEx)    \
                _bid_EXTERN_C                       \
                __declspec(allocate("BID$W062"))    \
                _bid_V BIDEXTREGW _bidN(060)_##fnEx = { (PCWSTR)_bidP, fnEx, (INT_PTR)(dataEx) };
  #else

    #define _bid_EXT_A(textID,fnEx,dataEx)          \
                _bid_FORCEINC(_bidN(060)_##fnEx)    \
                _bid_EXTERN_C                       \
                __declspec(allocate("BID$A062"))    \
                _bid_V BIDEXTREGA _bidN(060)_##fnEx = { textID, fnEx, (INT_PTR)(dataEx) };

    #define _bid_EXT_W(textID,fnEx,dataEx)          \
                _bid_FORCEINC(_bidN(060)_##fnEx)    \
                _bid_EXTERN_C                       \
                __declspec(allocate("BID$W062"))    \
                _bid_V BIDEXTREGW _bidN(060)_##fnEx = { textID, fnEx, (INT_PTR)(dataEx) };
  #endif

#endif

#define BID_EXT_EXA(txt,fn,data)    _bid_EXT_A(txt,fn,data)
#define BID_EXT_EXW(txt,fn,data)    _bid_EXT_W(txt,fn,data)

#define BID_EXTA(txt,fn)            _bid_EXT_A(txt,fn,0)
#define BID_EXTW(txt,fn)            _bid_EXT_W(txt,fn,0)

#if defined( _UNICODE )
  #define PBIDEXTREG                PBIDEXTREGW
  #define BID_EXT                   BID_EXTW
  #define BID_EXT_EX                BID_EXT_EXW
#else
  #define PBIDEXTREG                PBIDEXTREGA
  #define BID_EXT                   BID_EXTA
  #define BID_EXT_EX                BID_EXT_EXA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Output helpers to be used in BIDEXTPROC implementation
//
void __cdecl xBidWriteA(HANDLE hCtx, PCSTR  fmt, ... );
void __cdecl xBidWriteW(HANDLE hCtx, PCWSTR fmt, ... );

#define _bid_W0(x,hCtx,fmts)                                    \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidTraceExV##x(hCtx,0,0, _bidN(100), NULL);  \
        } while(0)

#define _bid_W1(x,hCtx,fmts,a)                                  \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a);            \
        } while(0)

#define _bid_W2(x,hCtx,fmts,a,b)                                \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b);          \
        } while(0)

#define _bid_W3(x,hCtx,fmts,a,b,c)                              \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c);        \
        } while(0)

#define _bid_W4(x,hCtx,fmts,a,b,c,d)                            \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c,d);      \
        } while(0)

#define _bid_W5(x,hCtx,fmts,a,b,c,d,e)                          \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c,d,e);    \
        } while(0)

#define _bid_W6(x,hCtx,fmts,a,b,c,d,e,f)                        \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c,d,e,f);  \
        } while(0)

#define _bid_W7(x,hCtx,fmts,a,b,c,d,e,f,g)                      \
        do{ _bid_XFS_##x(_bidN(100), fmts)                      \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c,d,e,f,g);\
        } while(0)

#define _bid_W8(x,hCtx,fmts,a,b,c,d,e,f,g,h)                        \
        do{ _bid_XFS_##x(_bidN(100), fmts)                          \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c,d,e,f,g,h);  \
        } while(0)

#define _bid_W9(x,hCtx,fmts,a,b,c,d,e,f,g,h,i)                      \
        do{ _bid_XFS_##x(_bidN(100), fmts)                          \
            (void)xBidWrite##x(hCtx, _bidN(100), a,b,c,d,e,f,g,h,i);\
        } while(0)

#define BidWriteEx0A(hCtx,fmts)                     _bid_W0(A,hCtx,fmts)
#define BidWriteEx1A(hCtx,fmts,a)                   _bid_W1(A,hCtx,fmts,a)
#define BidWriteEx2A(hCtx,fmts,a,b)                 _bid_W2(A,hCtx,fmts,a,b)
#define BidWriteEx3A(hCtx,fmts,a,b,c)               _bid_W3(A,hCtx,fmts,a,b,c)
#define BidWriteEx4A(hCtx,fmts,a,b,c,d)             _bid_W4(A,hCtx,fmts,a,b,c,d)
#define BidWriteEx5A(hCtx,fmts,a,b,c,d,e)           _bid_W5(A,hCtx,fmts,a,b,c,d,e)
#define BidWriteEx6A(hCtx,fmts,a,b,c,d,e,f)         _bid_W6(A,hCtx,fmts,a,b,c,d,e,f)
#define BidWriteEx7A(hCtx,fmts,a,b,c,d,e,f,g)       _bid_W7(A,hCtx,fmts,a,b,c,d,e,f,g)
#define BidWriteEx8A(hCtx,fmts,a,b,c,d,e,f,g,h)     _bid_W8(A,hCtx,fmts,a,b,c,d,e,f,g,h)
#define BidWriteEx9A(hCtx,fmts,a,b,c,d,e,f,g,h,i)   _bid_W9(A,hCtx,fmts,a,b,c,d,e,f,g,h,i)

#define BidWriteEx0W(hCtx,fmts)                     _bid_W0(W,hCtx,fmts)
#define BidWriteEx1W(hCtx,fmts,a)                   _bid_W1(W,hCtx,fmts,a)
#define BidWriteEx2W(hCtx,fmts,a,b)                 _bid_W2(W,hCtx,fmts,a,b)
#define BidWriteEx3W(hCtx,fmts,a,b,c)               _bid_W3(W,hCtx,fmts,a,b,c)
#define BidWriteEx4W(hCtx,fmts,a,b,c,d)             _bid_W4(W,hCtx,fmts,a,b,c,d)
#define BidWriteEx5W(hCtx,fmts,a,b,c,d,e)           _bid_W5(W,hCtx,fmts,a,b,c,d,e)
#define BidWriteEx6W(hCtx,fmts,a,b,c,d,e,f)         _bid_W6(W,hCtx,fmts,a,b,c,d,e,f)
#define BidWriteEx7W(hCtx,fmts,a,b,c,d,e,f,g)       _bid_W7(W,hCtx,fmts,a,b,c,d,e,f,g)
#define BidWriteEx8W(hCtx,fmts,a,b,c,d,e,f,g,h)     _bid_W8(W,hCtx,fmts,a,b,c,d,e,f,g,h)
#define BidWriteEx9W(hCtx,fmts,a,b,c,d,e,f,g,h,i)   _bid_W9(W,hCtx,fmts,a,b,c,d,e,f,g,h,i)

#define BidWrite0A(fmts)                        BidWriteEx0A(_bidExt_hCtx,fmts)
#define BidWrite1A(fmts,a)                      BidWriteEx1A(_bidExt_hCtx,fmts,a)
#define BidWrite2A(fmts,a,b)                    BidWriteEx2A(_bidExt_hCtx,fmts,a,b)
#define BidWrite3A(fmts,a,b,c)                  BidWriteEx3A(_bidExt_hCtx,fmts,a,b,c)
#define BidWrite4A(fmts,a,b,c,d)                BidWriteEx4A(_bidExt_hCtx,fmts,a,b,c,d)
#define BidWrite5A(fmts,a,b,c,d,e)              BidWriteEx5A(_bidExt_hCtx,fmts,a,b,c,d,e)
#define BidWrite6A(fmts,a,b,c,d,e,f)            BidWriteEx6A(_bidExt_hCtx,fmts,a,b,c,d,e,f)
#define BidWrite7A(fmts,a,b,c,d,e,f,g)          BidWriteEx7A(_bidExt_hCtx,fmts,a,b,c,d,e,f,g)
#define BidWrite8A(fmts,a,b,c,d,e,f,g,h)        BidWriteEx8A(_bidExt_hCtx,fmts,a,b,c,d,e,f,g,h)
#define BidWrite9A(fmts,a,b,c,d,e,f,g,h,i)      BidWriteEx9A(_bidExt_hCtx,fmts,a,b,c,d,e,f,g,h,i)

#define BidWrite0W(fmts)                        BidWriteEx0W(_bidExt_hCtx,fmts)
#define BidWrite1W(fmts,a)                      BidWriteEx1W(_bidExt_hCtx,fmts,a)
#define BidWrite2W(fmts,a,b)                    BidWriteEx2W(_bidExt_hCtx,fmts,a,b)
#define BidWrite3W(fmts,a,b,c)                  BidWriteEx3W(_bidExt_hCtx,fmts,a,b,c)
#define BidWrite4W(fmts,a,b,c,d)                BidWriteEx4W(_bidExt_hCtx,fmts,a,b,c,d)
#define BidWrite5W(fmts,a,b,c,d,e)              BidWriteEx5W(_bidExt_hCtx,fmts,a,b,c,d,e)
#define BidWrite6W(fmts,a,b,c,d,e,f)            BidWriteEx6W(_bidExt_hCtx,fmts,a,b,c,d,e,f)
#define BidWrite7W(fmts,a,b,c,d,e,f,g)          BidWriteEx7W(_bidExt_hCtx,fmts,a,b,c,d,e,f,g)
#define BidWrite8W(fmts,a,b,c,d,e,f,g,h)        BidWriteEx8W(_bidExt_hCtx,fmts,a,b,c,d,e,f,g,h)
#define BidWrite9W(fmts,a,b,c,d,e,f,g,h,i)      BidWriteEx9W(_bidExt_hCtx,fmts,a,b,c,d,e,f,g,h,i)

#if defined( _UNICODE)
  #define xBidWrite     xBidWriteW

  #define BidWrite0     BidWrite0W
  #define BidWrite1     BidWrite1W
  #define BidWrite2     BidWrite2W
  #define BidWrite3     BidWrite3W
  #define BidWrite4     BidWrite4W
  #define BidWrite5     BidWrite5W
  #define BidWrite6     BidWrite6W
  #define BidWrite7     BidWrite7W
  #define BidWrite8     BidWrite8W
  #define BidWrite9     BidWrite9W

  #define BidWriteEx0   BidWriteEx0W
  #define BidWriteEx1   BidWriteEx1W
  #define BidWriteEx2   BidWriteEx2W
  #define BidWriteEx3   BidWriteEx3W
  #define BidWriteEx4   BidWriteEx4W
  #define BidWriteEx5   BidWriteEx5W
  #define BidWriteEx6   BidWriteEx6W
  #define BidWriteEx7   BidWriteEx7W
  #define BidWriteEx8   BidWriteEx8W
  #define BidWriteEx9   BidWriteEx9W
#else
  #define xBidWrite     xBidWriteA

  #define BidWrite0     BidWrite0A
  #define BidWrite1     BidWrite1A
  #define BidWrite2     BidWrite2A
  #define BidWrite3     BidWrite3A
  #define BidWrite4     BidWrite4A
  #define BidWrite5     BidWrite5A
  #define BidWrite6     BidWrite6A
  #define BidWrite7     BidWrite7A
  #define BidWrite8     BidWrite8A
  #define BidWrite9     BidWrite9A

  #define BidWriteEx0   BidWriteEx0A
  #define BidWriteEx1   BidWriteEx1A
  #define BidWriteEx2   BidWriteEx2A
  #define BidWriteEx3   BidWriteEx3A
  #define BidWriteEx4   BidWriteEx4A
  #define BidWriteEx5   BidWriteEx5A
  #define BidWriteEx6   BidWriteEx6A
  #define BidWriteEx7   BidWriteEx7A
  #define BidWriteEx8   BidWriteEx8A
  #define BidWriteEx9   BidWriteEx9A
#endif

//
//  Indentation to be used in BidExtension
//
#define BidWriteIndentEx_In(hCtx)       xBidIndent(hCtx, BID_INDENT_IN)
#define BidWriteIndentEx_Out(hCtx)      xBidIndent(hCtx, BID_INDENT_OUT)

#define BidWriteIndent_In()             BidWriteIndentEx_In(_bidExt_hCtx)
#define BidWriteIndent_Out()            BidWriteIndentEx_Out(_bidExt_hCtx)


//
//  Binary output
//
BOOL __cdecl _bidWriteBin(HANDLE hCtx, UINT_PTR info, ... /*PCVOID pItem, UINT size, INT_PTR id*/);

#define _bid_WB1                        BID_BLOB
#define _bid_WB2                        BID_BLOB2
#define _bid_WB1C                       BID_BLOB_COPY
#define _bid_WB2C                       BID_BLOB2_COPY

#define BidWriteBinIDEx(h,p,sz,id)      (void)_bidWriteBin(h,_bid_WB2, (PCVOID)(p),(UINT)sz,(INT_PTR)(id))
#define BidWriteBinCopyIDEx(h,p,sz,id)  (void)_bidWriteBin(h,_bid_WB2C,(PCVOID)(p),(UINT)sz,(INT_PTR)(id))

#define BidWriteBinEx(h,p,sz)           (void)_bidWriteBin(h,_bid_WB1, (PCVOID)(p),(UINT)sz)
#define BidWriteBinCopyEx(h,p,sz)       (void)_bidWriteBin(h,_bid_WB1C,(PCVOID)(p),(UINT)sz)

#define BidWriteBinID(p,sz,id)          BidWriteBinIDEx(_bidExt_hCtx,p,sz,id)
#define BidWriteBinCopyID(p,sz,id)      BidWriteBinCopyIDEx(_bidExt_hCtx,p,sz,id)

#define BidWriteBin(p,sz)               BidWriteBinEx(_bidExt_hCtx,p,sz)
#define BidWriteBinCopy(p,sz)           BidWriteBinCopyEx(_bidExt_hCtx,p,sz)


//
//  Special version of BidWriteBin to be used in Binary Streaming Mode
//
#define BidWriteInBinaryModeEx(h,p,sz)  (void)_bidWriteBin(h,BID_BLOB_BIN_MODE,(PCVOID)(p),(UINT)sz)
#define BidWriteInBinaryMode(p,sz)      BidWriteInBinaryModeEx(_bidExt_hCtx,p,sz)

//
//  Naming Convention consistency.. (to be used with xBidWrite[A|W])
//
#define xBidHCtx                        _bidExt_hCtx

//
//  API to allocate format strings for low-level write statements xBidWrite
//
#ifdef _BID_NO_SPECIAL_ALLOCATION
  #define xBID_XFSA(name, str)          static char  name[] = str;
  #define xBID_XFSW(name, str)          static WCHAR name[] = str;
#else
  #define xBID_XFSA(name, str)          _bid_XFS_A(name,str)
  #define xBID_XFSW(name, str)          _bid_XFS_W(name,str)
#endif

#ifdef _UNICODE
  #define xBID_XFS                      xBID_XFSW
#else
  #define xBID_XFS                      xBID_XFSA
#endif


//
//  BID_REF macro helps pass ref variables by reference. We need this because
//  __cdecl(...) assumes var_args to be passed by value.
//  For example,
//
//      BidTrace1(_T("%p{REFIID}\n"), riid);
//
//  takes the guid itself instead of its address.
//  The correct trace statement:
//
//      BidTrace1(_T("%p{REFIID}\n"), BID_REF(riid));
//
//  will work the same way as regular COM API, i.e. QuieryInterface(riid, ..);
//
#ifdef __cplusplus
  #define BID_REF(rObj)                 (UINT_PTR)&(rObj)
#else
  #define BID_REF(rObj)                 (UINT_PTR)(rObj)
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidExtension Macro Wrappers
//
//  Names of hidden arguments:
//      _bidExt_hCtx    _bidExt_pItem   _bidExt_attr    _bidExt_dataEx
//

#ifdef _ARM_
// ARMTODO: This macro results in producing a function prototype that the linker fails to resolve.
// I am not quite sure where it is expected to be implemented in and thus, I am disabling this for _ARM_.
#define BID_EXTENSION_ALIASA(txtID,typName)     
#define BID_EXTENSION_ALIASW(txtID,typName)     
#else // !_ARM_
#define BID_EXTENSION_ALIASA(txtID,typName)     BID_EXTA(txtID, typName##_bidExt)
#define BID_EXTENSION_ALIASW(txtID,typName)     BID_EXTW(txtID, typName##_bidExt)
#endif // _ARM_

#define BID_EXTENSION_EXA(txtID,typName,args)                                   \
            void WINAPI typName##_bidExt(HANDLE,PCVOID,DWORD,INT_PTR);          \
            BID_EXTENSION_ALIASA(txtID,typName);                                \
            void WINAPI typName##_bidExt args

#define BID_EXTENSION_EXW(txtID,typName,args)                                   \
            void WINAPI typName##_bidExt(HANDLE,PCVOID,DWORD,INT_PTR);          \
            BID_EXTENSION_ALIASW(txtID,typName);                                \
            void WINAPI typName##_bidExt args

#define BID_EXTENSION2A(txtID,typName)                                          \
    BID_EXTENSION_EXA(txtID,typName,(HANDLE _bidExt_hCtx, PCVOID _bidExt_pItem, \
                                    DWORD _bidExt_attr, INT_PTR _bidExt_dataEx))

#define BID_EXTENSION2W(txtID,typName)                                          \
    BID_EXTENSION_EXW(txtID,typName,(HANDLE _bidExt_hCtx, PCVOID _bidExt_pItem, \
                                    DWORD _bidExt_attr, INT_PTR _bidExt_dataEx))

#define BID_EXTENSIONA(Name)        BID_EXTENSION2A(#Name,Name)
#define BID_EXTENSIONW(Name)        BID_EXTENSION2W(L ## #Name,Name)

#if defined( _UNICODE )
  #define BID_EXTENSION             BID_EXTENSIONW
  #define BID_EXTENSION2            BID_EXTENSION2W
  #define BID_EXTENSION_EX          BID_EXTENSION_EXW
  #define BID_EXTENSION_ALIAS       BID_EXTENSION_ALIASW
#else
  #define BID_EXTENSION             BID_EXTENSIONA
  #define BID_EXTENSION2            BID_EXTENSION2A
  #define BID_EXTENSION_EX          BID_EXTENSION_EXA
  #define BID_EXTENSION_ALIAS       BID_EXTENSION_ALIASA
#endif

#define BID_EXTENSION_PTR_EX(typName,ptr,pItem)                 \
            const typName* ptr = (const typName*)pItem

#define BID_EXTENSION_PTR(typName,ptr)                          \
            BID_EXTENSION_PTR_EX(typName,ptr,_bidExt_pItem)

#define BID_EXTENSION2_PTR(typName,ptr)                         \
            BID_EXTENSION_PTR(typName,ptr);                     \
            BID_EXTENSION_UNUSED_ARGS()

#define BID_EXTENSION_UNUSED_ARGS() _bidExt_attr; _bidExt_dataEx

#define BID_EXTENSION_CALL_EX(typName,hCtx,pItem,attr,dataEx)   \
            typName##_bidExt(hCtx,pItem,attr,dataEx)

#define BID_EXTENSION_CALL(typName,pItem)                       \
            BID_EXTENSION_CALL_EX(typName,_bidExt_hCtx, pItem, _bidExt_attr,_bidExt_dataEx)


#if defined(__cplusplus)

  #define BID_EXTENSION_DECLARE(typName)                        \
            friend void WINAPI typName##_bidExt(HANDLE,PCVOID,DWORD,INT_PTR);

  #define BID_EXTENSION_REF_EX(typName,r,pItem)                 \
            const typName& r = *(const typName*)pItem

  #define BID_EXTENSION_REF(typName,rObj)                       \
            BID_EXTENSION_REF_EX(typName,rObj,_bidExt_pItem);   \
            _bidExt_attr; _bidExt_dataEx

  #define BID_EXTENSION_DERIVE_EX(typName,baseType,rObj,hCtx,pItem,attr,dataEx) \
            BID_EXTENSION_REF_EX(typName,rObj,pItem);                           \
            BID_EXTENSION_CALL_EX(baseType,hCtx,pItem,attr,dataEx)

  #define BID_EXTENSION_DERIVE(typName,baseType,rObj)           \
            BID_EXTENSION_DERIVE_EX(typName,baseType,rObj,      \
                                    _bidExt_hCtx,_bidExt_pItem,_bidExt_attr,_bidExt_dataEx)

#endif

//
//  Access to BidExtension Data field
//
#define BID_EXTENSION_DATA_EX(typName,var,dataEx)               \
            const typName var = (const typName)dataEx

#define BID_EXTENSION_DATA(typName,var)                         \
            BID_EXTENSION_DATA_EX(typName,var,_bidExt_dataEx)

#define BID_EXTENSION2_DATA(typName,var)                        \
            BID_EXTENSION_DATA(typName,var);                    \
            BID_EXTENSION_UNUSED_ARGS()

#define xBidAttributes              _bidExt_attr
#define xBidDataEx                  _bidExt_dataEx



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidTraceBin[ID][U] to be used as a regular tracepoint outside of BIDEXTPROC
//
#define BidTraceBinEx(ON,info,p,sz,id)                                              \
        _bid_DO                                                                     \
        if( ON ){                                                                   \
            if( !_bidWriteBin(_bidID, info, (PCVOID)(p), (UINT)sz, (INT_PTR)(id)) ) \
                DBREAK();                                                           \
        }                                                                           \
        _bid_WHILE0

#define BidTraceBinIDU(ON,p,sz,id)  BidTraceBinEx(ON,       _bid_WB2, p,sz, id)
#define BidTraceBinU(ON,p,sz)       BidTraceBinEx(ON,       _bid_WB1, p,sz,  0)
#define BidTraceBinIDE(p,sz,id)     BidTraceBinEx(TRUE,     _bid_WB2, p,sz, id)
#define BidTraceBinE(p,sz)          BidTraceBinEx(TRUE,     _bid_WB1, p,sz,  0)
#define BidTraceBinID(p,sz,id)      BidTraceBinEx(_bidTrON, _bid_WB2, p,sz, id)
#define BidTraceBin(p,sz)           BidTraceBinEx(_bidTrON, _bid_WB1, p,sz,  0)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  TEXT METADATA
//  (Descriptions of Trace Control String Keywords, Namespace Aliases, Enums, etc.)
//
#if defined( _BID_NO_SPECIAL_ALLOCATION )

  #define BID_METATEXTA(metaStr)    _erEr  _BID_NO_SPECIAL_ALLOCATION__BID_METATEXT_wont_work
  #define BID_METATEXTW(metaStr)    _erEr  _BID_NO_SPECIAL_ALLOCATION__BID_METATEXT_wont_work

#elif defined( _BID_PACK_METATEXT )

  #define BID_METATEXTA(metaStr)
  #define BID_METATEXTW(metaStr)

#else

  #define BID_METATEXTA(metaStr)                                                        \
            __declspec(allocate("BID$Z002")) static const char _bidN(Z002)[] = metaStr; \
            _bid_EXTERN_C                                                               \
            __declspec(allocate("BID$A082")) _bid_V PCSTR _bidN(080) = _bidN(Z002);     \
            _bid_FORCEINC(_bidN(080))

  #define BID_METATEXTW(metaStr)                                                        \
            __declspec(allocate("BID$Z005")) static const WCHAR _bidN(Z005)[] = metaStr;\
            _bid_EXTERN_C                                                               \
            __declspec(allocate("BID$W082")) _bid_V PCWSTR _bidN(080) = _bidN(Z005);    \
            _bid_FORCEINC(_bidN(080))

#endif

_bid_INLINE void WINAPI xBidAddMetaTextA(PCSTR  metaStr);
_bid_INLINE void WINAPI xBidAddMetaTextW(PCWSTR metaStr);

#define BidAddMetaTextA(s)      xBidAddMetaTextA(s)
#define BidAddMetaTextW(s)      xBidAddMetaTextW(s)

#if defined( _UNICODE )
  #define xBidAddMetaText       xBidAddMetaTextW
  #define BidAddMetaText        BidAddMetaTextW
  #define BID_METATEXT          BID_METATEXTW
#else
  #define xBidAddMetaText       xBidAddMetaTextA
  #define BidAddMetaText        BidAddMetaTextA
  #define BID_METATEXT          BID_METATEXTA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  COMMAND SPACES FOR CONTROL AND INFORMATION COMMANDS
//
_bid_INLINE int     WINAPI      xBidGetNumOfCmdSpaces(void);
_bid_INLINE int     WINAPI      xBidGetCmdSpaceNameA( __in int idx,
                                                      __out_ecount(strBufCapacity) PSTR strBuf,
                                                      __in int strBufCapacity);
_bid_INLINE INT_PTR WINAPI      xBidGetCmdSpaceIDA(__in PCSTR textID);

#define BidGetNumOfCmdSpaces    xBidGetNumOfCmdSpaces

#define xBidGetCmdSpaceName     xBidGetCmdSpaceNameA
#define BidGetCmdSpaceNameA     xBidGetCmdSpaceNameA
#define BidGetCmdSpaceName      xBidGetCmdSpaceNameA

#define xBidGetCmdSpaceID       xBidGetCmdSpaceIDA
#define BidGetCmdSpaceIDA       xBidGetCmdSpaceIDA
#define BidGetCmdSpaceID        xBidGetCmdSpaceIDA

//
//  Standard modifiers for command codes.
//
#define BID_CMD_REVERSE         1
#define BID_CMD_UNICODE         2

//
//  Macros that help define commands within Command Spaces.
//
#define BID_CMD_MULTIPLY        4
#define BID_CMD(code)           ((code) * BID_CMD_MULTIPLY)
#define BID_CMD_R(code)         (BID_CMD(code) + BID_CMD_REVERSE)
#define BID_CMD_U(code)         (BID_CMD(code) + BID_CMD_UNICODE)
#define BID_CMD_UR(code)        (BID_CMD(code) + BID_CMD_UNICODE + BID_CMD_REVERSE)

#define BID_CMD_IS_REVERSE(cmd) (((cmd) & BID_CMD_REVERSE) != 0)
#define BID_CMD_IS_UNICODE(cmd) (((cmd) & BID_CMD_UNICODE) != 0)

//
//  Predefined commands are in range [BID_CMD(BID_DCSCMD_BASE) .. BID_CMD(BID_DCSCMD_MAX)]
//  'DCS' stands for 'Default Command Space'
//
#define BID_DCSCMD_BASE         268435456   // 0x10000000
#define BID_DCSCMD_MAX          402653183   // 0x17FFFFFF

//
//  Control Panel commands are in range [BID_CMD(BID_CPLCMD_BASE) .. BID_CMD(BID_CPLCMD_MAX)]
//
#define BID_CPLCMD_BASE         402653184   // 0x18000000
#define BID_CPLCMD_MAX          536870911   // 0x1FFFFFFF

//
//  Commands within a Command Space are in range [0 .. BID_CMD(BID_CMD_MAX)]
//
#define BID_CMD_MAX             (BID_DCSCMD_BASE - 1)

//
//  Predefined command code (get/set control bits) within every Command Space.
//
#define BID_CMD_STDBITS         (BID_CMD_MAX - 1)


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SERVICE & CONTROL CENTRAL POINT
//
#define BID_CMDSPACE_DEFAULT    (INT_PTR)(-1)

_bid_DECL( INT_PTR, BidCtlProc, (HANDLE hID, INT_PTR cmdSpaceID, int cmd,
                                INT_PTR arg1, INT_PTR arg2, INT_PTR arg3) );

INT_PTR WINAPI  _bidCtlProc(INT_PTR cmdSpaceID, int cmd, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3);

#define xBidCtlProcEx(cmdSpaceID, cmd, a1, a2, a3) \
                _bidCtlProc(cmdSpaceID, cmd, (INT_PTR)(a1),(INT_PTR)(a2),(INT_PTR)(a3))

#define xBidCtlProc(c,a1,a2,a3) xBidCtlProcEx(BID_CMDSPACE_DEFAULT,c,a1,a2,a3)

#define BidCtlProcEx            xBidCtlProcEx
#define BidCtlProc              xBidCtlProcEx


//
//  Standard wrappers for commonly used operation - get/set control bits
//
_bid_INLINE DWORD WINAPI _bidSetGetCtlBitsEx(INT_PTR cmdSpaceID, int cmd, DWORD mask, DWORD bits);

#define _bidCTL_STDBITS                     BID_CMD(BID_CMD_STDBITS)

#define BidGetCtlBitsEx(csID,cmd,mask)      _bidSetGetCtlBitsEx(csID, (cmd)+BID_CMD_REVERSE, mask, 0)
#define BidSetCtlBitsEx(csID,cmd,mask,bits) _bidSetGetCtlBitsEx(csID, cmd, mask, bits)

#define BidGetCtlBits(cmdSpaceID,mask)      BidGetCtlBitsEx(cmdSpaceID, _bidCTL_STDBITS, mask)
#define BidSetCtlBits(cmdSpaceID,mask,bits) BidSetCtlBitsEx(cmdSpaceID, _bidCTL_STDBITS, mask,bits)

#define BID_CTLBITS_ALL                     0xFFFFFFFF

//
//  Predefined commands (have wrapper functions)
//
#define BID_DCSCMD_CMDSPACE_COUNT           BID_CMD(0  + BID_DCSCMD_BASE)
#define BID_DCSCMD_CMDSPACE_ENUM            BID_CMD(1  + BID_DCSCMD_BASE)
#define BID_DCSCMD_CMDSPACE_QUERY           BID_CMD(2  + BID_DCSCMD_BASE)

#define BID_DCSCMD_FLUSH_BUFFERS            BID_CMD(4  + BID_DCSCMD_BASE)
#define BID_DCSCMD_GET_EVENT_ID             BID_CMD(5  + BID_DCSCMD_BASE)
#define BID_DCSCMD_PARSE_STRING             BID_CMD(6  + BID_DCSCMD_BASE)
#define BID_DCSCMD_ADD_EXTENSION            BID_CMD(7  + BID_DCSCMD_BASE)
#define BID_DCSCMD_ADD_METATEXT             BID_CMD(8  + BID_DCSCMD_BASE)
#define BID_DCSCMD_ADD_RESHANDLE            BID_CMD(9  + BID_DCSCMD_BASE)
#define BID_DCSCMD_SHUTDOWN                 BID_CMD(10 + BID_DCSCMD_BASE)


//
//  Supposed to be used in a debugger to flush trace buffers in case of crash.
//  Use from instrumented code on special occasions only, such as before re-throwing exception.
//
int WINAPI xBidFlush();


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidTouch: Fast Communication with the subsystem
//
_bid_DECL(INT_PTR, BidTouch, (HANDLE hID, UINT_PTR scope, UINT code, INT_PTR arg1, INT_PTR arg2));

#define _bidTouch(h,s,c,a,b)        (*_bidPfn.BidTouch)(h,(UINT_PTR)(s),c,(INT_PTR)(a),(INT_PTR)(b))
#define xBidTouch(s,c,a,b)          (_bidID_IF ? _bidTouch(_bidID,s,c,a,b) : 0)


//
//  Command Layout (also see BidImplApi.h for implementation helpers)
//
#define BID_TOUCHCODE_MASK          0x0000FFFF
#define BID_TOUCHCODE_REVERSE       0x00000001
#define BID_TOUCHCODE_UNICODE       0x00000002

#define BID_TOUCHCODE(code)         (code * 4)
#define BID_TOUCHCODE_R(code)       (code * 4 + BID_TOUCHCODE_REVERSE)
#define BID_TOUCHCODE_U(code)       (BID_TOUCHCODE(code) + BID_TOUCHCODE_UNICODE)
#define BID_TOUCHCODE_UR(code)      (BID_TOUCHCODE_R(code) + BID_TOUCHCODE_UNICODE)


//
//  Command Codes (API wrappers are provided in corresponding sections)
//
#define BID_TOUCH_EXTENSION         BID_TOUCHCODE   (0)

#define BID_TOUCH_OBTAIN_ITEM_IDA   BID_TOUCHCODE   (1)
#define BID_TOUCH_OBTAIN_ITEM_IDW   BID_TOUCHCODE_U (1)

#define BID_TOUCH_RECYCLE_ITEM_IDA  BID_TOUCHCODE_R (1)
#define BID_TOUCH_RECYCLE_ITEM_IDW  BID_TOUCHCODE_UR(1)

#define BID_TOUCH_UPDATE_ITEM_IDA   BID_TOUCHCODE   (2)
#define BID_TOUCH_UPDATE_ITEM_IDW   BID_TOUCHCODE_U (2)



/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Instance-Tracking IDs
//

//
//  ObtainItemID
//
int     _bidObtainItemIDA           (PCSTR  textID, INT_PTR invariant, INT_PTR associate);
int     _bidObtainItemIDW           (PCWSTR textID, INT_PTR invariant, INT_PTR associate);

#define xBidObtainItemID2A(x,v,a)   _bidObtainItemIDA(x, (INT_PTR)(v), (INT_PTR)(a))
#define xBidObtainItemID2W(x,v,a)   _bidObtainItemIDW(x, (INT_PTR)(v), (INT_PTR)(a))

#define xBidObtainItemIDA(x,v)      xBidObtainItemID2A(x,v,0)
#define xBidObtainItemIDW(x,v)      xBidObtainItemID2W(x,v,0)

#define BidObtainItemID2A(p,x,v,a)  do{ _bid_EDS_A(_bidN(051), x)                   \
                                        *p = xBidObtainItemID2A(_bidN(051), v, a);  \
                                    }while(0)

#define BidObtainItemID2W(p,x,v,a)  do{ _bid_EDS_W(_bidN(051), x)                   \
                                        *p = xBidObtainItemID2W(_bidN(051), v, a);  \
                                    }while(0)

#define BidObtainItemIDA(p,x,a)     BidObtainItemID2A(p,x,a,0)
#define BidObtainItemIDW(p,x,a)     BidObtainItemID2W(p,x,a,0)


//
//  UpdateItemID
//
int     _bidUpdateItemIDA           (int itemID, PCSTR  textID, INT_PTR associate);
int     _bidUpdateItemIDW           (int itemID, PCWSTR textID, INT_PTR associate);

#define xBidUpdateItemIDA(id,x,a)   _bidUpdateItemIDA(id, x, (INT_PTR)(a))
#define xBidUpdateItemIDW(id,x,a)   _bidUpdateItemIDW(id, x, (INT_PTR)(a))

#define BidUpdateItemIDA(p,x,a)     do{ _bid_EDS_A(_bidN(052), x)                   \
                                        *p = xBidUpdateItemIDA(*p, _bidN(052), a);  \
                                    }while(0)

#define BidUpdateItemIDW(p,x,a)     do{ _bid_EDS_W(_bidN(052), x)                   \
                                        *p = xBidUpdateItemIDW(*p, _bidN(052), a);  \
                                    }while(0)

//
//  RecycleItemID
//
#define xBidRecycleItemIDA(id,txt)  if(id){ xBidTouch(txt, BID_TOUCH_RECYCLE_ITEM_IDA, id, 0); }
#define xBidRecycleItemIDW(id,txt)  if(id){ xBidTouch(txt, BID_TOUCH_RECYCLE_ITEM_IDW, id, 0); }

#define BidRecycleItemIDA(p,txt)    do{ _bid_EDS_A(_bidN(053), txt)                 \
                                        xBidRecycleItemIDA(*p, _bidN(053));         \
                                        *p = 0;                                     \
                                    }while(0)

#define BidRecycleItemIDW(p,txt)    do{ _bid_EDS_W(_bidN(053), txt)                 \
                                        xBidRecycleItemIDW(*p, _bidN(053));         \
                                        *p = 0;                                     \
                                    }while(0)


#if defined( _UNICODE )
  #define xBidObtainItemID          xBidObtainItemIDW
  #define xBidObtainItemID2         xBidObtainItemID2W
  #define BidObtainItemID           BidObtainItemIDW
  #define BidObtainItemID2          BidObtainItemID2W

  #define xBidUpdateItemID          xBidUpdateItemIDW
  #define BidUpdateItemID           BidUpdateItemIDW

  #define xBidRecycleItemID         xBidRecycleItemIDW
  #define BidRecycleItemID          BidRecycleItemIDW
#else
  #define xBidObtainItemID          xBidObtainItemIDA
  #define xBidObtainItemID2         xBidObtainItemID2A
  #define BidObtainItemID           BidObtainItemIDA
  #define BidObtainItemID2          BidObtainItemID2A

  #define xBidUpdateItemID          xBidUpdateItemIDA
  #define BidUpdateItemID           BidUpdateItemIDA

  #define xBidRecycleItemID         xBidRecycleItemIDA
  #define BidRecycleItemID          BidRecycleItemIDA
#endif


//
//  API to allocate Item Definition Strings (IDS) for xBid{Obtain|Update|Recycle}ItemID
//
#ifdef _BID_NO_SPECIAL_ALLOCATION
  #define xBID_IDSA(name,str)       static char  name[] = str;
  #define xBID_IDSW(name,str)       static WCHAR name[] = str;
#else
  #define xBID_IDSA(name,str)       _bid_EDS_A(name,str)
  #define xBID_IDSW(name,str)       _bid_EDS_W(name,str)
#endif

#if defined( _UNICODE )
  #define xBID_IDS                  xBID_IDSW
#else
  #define xBID_IDS                  xBID_IDSA
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ADDITIONAL CDECL EXPORTS (Not used in C/C++ interface)
//
_bid_CDECL(BOOL, BidScopeEnterCW,(HANDLE hID, UINT_PTR src, UINT_PTR info, HANDLE* pHScp, PCWSTR stf, ... ));
_bid_CDECL(BOOL, BidTraceCW,     (HANDLE hID, UINT_PTR src, UINT_PTR info, PCWSTR fmt,  ... ));



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                      DEBUG-ONLY STUFF                                       //
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG_TRACE_ON

  #define DPUTSTR           BidPutStr
  #define DPUTSTRLINE       BidPutStrLine
  #define DPUTNEWLINE       BidPutNewLine

  #define DTRACE0           BidTrace0
  #define DTRACE1           BidTrace1
  #define DTRACE2           BidTrace2
  #define DTRACE3           BidTrace3
  #define DTRACE4           BidTrace4
  #define DTRACE5           BidTrace5
  #define DTRACE6           BidTrace6
  #define DTRACE7           BidTrace7
  #define DTRACE8           BidTrace8
  #define DTRACE9           BidTrace9

  #define DSCOPE_ENTER      BidScopeEnter
  #define DSCOPE_ENTER0     BidScopeEnter0
  #define DSCOPE_ENTER1     BidScopeEnter1
  #define DSCOPE_ENTER2     BidScopeEnter2
  #define DSCOPE_ENTER3     BidScopeEnter3
  #define DSCOPE_ENTER4     BidScopeEnter4
  #define DSCOPE_ENTER5     BidScopeEnter5
  #define DSCOPE_ENTER6     BidScopeEnter6
  #define DSCOPE_ENTER7     BidScopeEnter7
  #define DSCOPE_ENTER8     BidScopeEnter8
  #define DSCOPE_ENTER9     BidScopeEnter9

  #define DSCOPE_LEAVE      BidScopeLeave
  #define DSCOPE_LEAVE_HR   BidScopeLeaveHR
  #define DSCOPE_LEAVE_ERR  BidScopeLeaveErr

  #define DSCOPE_LEAVE0     BidScopeLeave0
  #define DSCOPE_LEAVE1     BidScopeLeave1
  #define DSCOPE_LEAVE2     BidScopeLeave2
  #define DSCOPE_LEAVE3     BidScopeLeave3

  #define DTRACE_HR         BidTraceHR
  #define DTRACE_ERR        BirTraceErr
  #define DTRACE_MARK       BidTraceMark
  #define DCHK              BidCHK

  #define DPUTSTRA          BidPutStrA
  #define DPUTSTRLINEA      BidPutStrLineA

  #define DTRACE0A          BidTrace0A
  #define DTRACE1A          BidTrace1A
  #define DTRACE2A          BidTrace2A
  #define DTRACE3A          BidTrace3A
  #define DTRACE4A          BidTrace4A
  #define DTRACE5A          BidTrace5A
  #define DTRACE6A          BidTrace6A
  #define DTRACE7A          BidTrace7A
  #define DTRACE8A          BidTrace8A
  #define DTRACE9A          BidTrace9A

  #define DSCOPE_ENTERA     BidScopeEnterA
  #define DSCOPE_ENTER0A    BidScopeEnter0A
  #define DSCOPE_ENTER1A    BidScopeEnter1A
  #define DSCOPE_ENTER2A    BidScopeEnter2A
  #define DSCOPE_ENTER3A    BidScopeEnter3A
  #define DSCOPE_ENTER4A    BidScopeEnter4A
  #define DSCOPE_ENTER5A    BidScopeEnter5A
  #define DSCOPE_ENTER6A    BidScopeEnter6A
  #define DSCOPE_ENTER7A    BidScopeEnter7A
  #define DSCOPE_ENTER8A    BidScopeEnter8A
  #define DSCOPE_ENTER9A    BidScopeEnter9A

  #define DSCOPE_LEAVE0A    BidScopeLeave0A
  #define DSCOPE_LEAVE1A    BidScopeLeave1A
  #define DSCOPE_LEAVE2A    BidScopeLeave2A
  #define DSCOPE_LEAVE3A    BidScopeLeave3A

 #if defined( __cplusplus )
  #define DSCOPE_AUTO       BidScopeAuto
  #define DSCOPE_AUTO0      BidScopeAuto0
  #define DSCOPE_AUTO1      BidScopeAuto1
  #define DSCOPE_AUTO2      BidScopeAuto2
  #define DSCOPE_AUTO3      BidScopeAuto3
  #define DSCOPE_AUTO4      BidScopeAuto4
  #define DSCOPE_AUTO5      BidScopeAuto5
  #define DSCOPE_AUTO6      BidScopeAuto6
  #define DSCOPE_AUTO7      BidScopeAuto7
  #define DSCOPE_AUTO8      BidScopeAuto8
  #define DSCOPE_AUTO9      BidScopeAuto9

  #define DSCOPE_AUTOA      BidScopeAutoA
  #define DSCOPE_AUTO0A     BidScopeAuto0A
  #define DSCOPE_AUTO1A     BidScopeAuto1A
  #define DSCOPE_AUTO2A     BidScopeAuto2A
  #define DSCOPE_AUTO3A     BidScopeAuto3A
  #define DSCOPE_AUTO4A     BidScopeAuto4A
  #define DSCOPE_AUTO5A     BidScopeAuto5A
  #define DSCOPE_AUTO6A     BidScopeAuto6A
  #define DSCOPE_AUTO7A     BidScopeAuto7A
  #define DSCOPE_AUTO8A     BidScopeAuto8A
  #define DSCOPE_AUTO9A     BidScopeAuto9A

  #define DSCOPE_AUTO_TERM  BidScopeAuto_Term
 #endif

#else

  #define DPUTSTR(str)                          ((void)0)
  #define DPUTSTRLINE(str)                      ((void)0)
  #define DPUTNEWLINE()                         ((void)0)

  #define DTRACE0(tcfs)                         ((void)0)
  #define DTRACE1(tcfs,a)                       ((void)0)
  #define DTRACE2(tcfs,a,b)                     ((void)0)
  #define DTRACE3(tcfs,a,b,c)                   ((void)0)
  #define DTRACE4(tcfs,a,b,c,d)                 ((void)0)
  #define DTRACE5(tcfs,a,b,c,d,e)               ((void)0)
  #define DTRACE6(tcfs,a,b,c,d,e,f)             ((void)0)
  #define DTRACE7(tcfs,a,b,c,d,e,f,g)           ((void)0)
  #define DTRACE8(tcfs,a,b,c,d,e,f,g,h)         ((void)0)
  #define DTRACE9(tcfs,a,b,c,d,e,f,g,h,i)       ((void)0)

  #define DSCOPE_ENTER(stf)                     ((void)0)
  #define DSCOPE_ENTER0(stf)                    ((void)0)
  #define DSCOPE_ENTER1(stf,a)                  ((void)0)
  #define DSCOPE_ENTER2(stf,a,b)                ((void)0)
  #define DSCOPE_ENTER3(stf,a,b,c)              ((void)0)
  #define DSCOPE_ENTER4(stf,a,b,c,d)            ((void)0)
  #define DSCOPE_ENTER5(stf,a,b,c,d,e)          ((void)0)
  #define DSCOPE_ENTER6(stf,a,b,c,d,e,f)        ((void)0)
  #define DSCOPE_ENTER7(stf,a,b,c,d,e,f,g)      ((void)0)
  #define DSCOPE_ENTER8(stf,a,b,c,d,e,f,g,h)    ((void)0)
  #define DSCOPE_ENTER9(stf,a,b,c,d,e,f,g,h,i)  ((void)0)

  #define DSCOPE_LEAVE()                        ((void)0)
  #define DSCOPE_LEAVE_HR(hr)                   (hr)
  #define DSCOPE_LEAVE_ERR(api)                 (api)

  #define DSCOPE_LEAVE0(stf)                    ((void)0)
  #define DSCOPE_LEAVE1(stf,a)                  ((void)0)
  #define DSCOPE_LEAVE2(stf,a,b)                ((void)0)
  #define DSCOPE_LEAVE3(stf,a,b,c)              ((void)0)

  #define DTRACE_HR(hr)                         (hr)
  #define DTRACE_ERR(code)                      (code)
  #define DTRACE_MARK()                         ((void)0)
  #define DCHK(api)                             (api)

  #define DPUTSTRA(str)                         ((void)0)
  #define DPUTSTRLINEA(str)                     ((void)0)

  #define DTRACE0A(tcfs)                        ((void)0)
  #define DTRACE1A(tcfs,a)                      ((void)0)
  #define DTRACE2A(tcfs,a,b)                    ((void)0)
  #define DTRACE3A(tcfs,a,b,c)                  ((void)0)
  #define DTRACE4A(tcfs,a,b,c,d)                ((void)0)
  #define DTRACE5A(tcfs,a,b,c,d,e)              ((void)0)
  #define DTRACE6A(tcfs,a,b,c,d,e,f)            ((void)0)
  #define DTRACE7A(tcfs,a,b,c,d,e,f,g)          ((void)0)
  #define DTRACE8A(tcfs,a,b,c,d,e,f,g,h)        ((void)0)
  #define DTRACE9A(tcfs,a,b,c,d,e,f,g,h,i)      ((void)0)

  #define DSCOPE_ENTERA(stf)                    ((void)0)
  #define DSCOPE_ENTER0A(stf)                   ((void)0)
  #define DSCOPE_ENTER1A(stf,a)                 ((void)0)
  #define DSCOPE_ENTER2A(stf,a,b)               ((void)0)
  #define DSCOPE_ENTER3A(stf,a,b,c)             ((void)0)
  #define DSCOPE_ENTER4A(stf,a,b,c,d)           ((void)0)
  #define DSCOPE_ENTER5A(stf,a,b,c,d,e)         ((void)0)
  #define DSCOPE_ENTER6A(stf,a,b,c,d,e,f)       ((void)0)
  #define DSCOPE_ENTER7A(stf,a,b,c,d,e,f,g)     ((void)0)
  #define DSCOPE_ENTER8A(stf,a,b,c,d,e,f,g,h)   ((void)0)
  #define DSCOPE_ENTER9A(stf,a,b,c,d,e,f,g,h,i) ((void)0)

  #define DSCOPE_LEAVE0A(stf)                   ((void)0)
  #define DSCOPE_LEAVE1A(stf,a)                 ((void)0)
  #define DSCOPE_LEAVE2A(stf,a,b)               ((void)0)
  #define DSCOPE_LEAVE3A(stf,a,b,c)             ((void)0)


 #if defined( __cplusplus )
  #define DSCOPE_AUTO(stf)                      ((void)0)
  #define DSCOPE_AUTO0(stf)                     ((void)0)
  #define DSCOPE_AUTO1(stf,a)                   ((void)0)
  #define DSCOPE_AUTO2(stf,a,b)                 ((void)0)
  #define DSCOPE_AUTO3(stf,a,b,c)               ((void)0)
  #define DSCOPE_AUTO4(stf,a,b,c,d)             ((void)0)
  #define DSCOPE_AUTO5(stf,a,b,c,d,e)           ((void)0)
  #define DSCOPE_AUTO6(stf,a,b,c,d,e,f)         ((void)0)
  #define DSCOPE_AUTO7(stf,a,b,c,d,e,f,g)       ((void)0)
  #define DSCOPE_AUTO8(stf,a,b,c,d,e,f,g,h)     ((void)0)
  #define DSCOPE_AUTO9(stf,a,b,c,d,e,f,g,h,i)   ((void)0)

  #define DSCOPE_AUTOA(stf)                     ((void)0)
  #define DSCOPE_AUTO0A(stf)                    ((void)0)
  #define DSCOPE_AUTO1A(stf,a)                  ((void)0)
  #define DSCOPE_AUTO2A(stf,a,b)                ((void)0)
  #define DSCOPE_AUTO3A(stf,a,b,c)              ((void)0)
  #define DSCOPE_AUTO4A(stf,a,b,c,d)            ((void)0)
  #define DSCOPE_AUTO5A(stf,a,b,c,d,e)          ((void)0)
  #define DSCOPE_AUTO6A(stf,a,b,c,d,e,f)        ((void)0)
  #define DSCOPE_AUTO7A(stf,a,b,c,d,e,f,g)      ((void)0)
  #define DSCOPE_AUTO8A(stf,a,b,c,d,e,f,g,h)    ((void)0)
  #define DSCOPE_AUTO9A(stf,a,b,c,d,e,f,g,h,i)  ((void)0)

  #define DSCOPE_AUTO_TERM()                    ((void)0)
 #endif

#endif // _DEBUG_TRACE_ON


/////////////////////////////////////////////////////////////////////////////////////////////////
//                             CUSTOM MACRO WRAPPERS FOR BidTrace                              //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidTraceE - "Enabled" trace statement. Subsystem will not spend time to analyze TCS.
//
#define BidTraceE0A(tcfs)                   _bid_T0(A,TRUE,BID_ENA,tcfs)
#define BidTraceE1A(tcfs,a)                 _bid_T1(A,TRUE,BID_ENA,tcfs,a)
#define BidTraceE2A(tcfs,a,b)               _bid_T2(A,TRUE,BID_ENA,tcfs,a,b)
#define BidTraceE3A(tcfs,a,b,c)             _bid_T3(A,TRUE,BID_ENA,tcfs,a,b,c)
#define BidTraceE4A(tcfs,a,b,c,d)           _bid_T4(A,TRUE,BID_ENA,tcfs,a,b,c,d)
#define BidTraceE5A(tcfs,a,b,c,d,e)         _bid_T5(A,TRUE,BID_ENA,tcfs,a,b,c,d,e)
#define BidTraceE6A(tcfs,a,b,c,d,e,f)       _bid_T6(A,TRUE,BID_ENA,tcfs,a,b,c,d,e,f)
#define BidTraceE7A(tcfs,a,b,c,d,e,f,g)     _bid_T7(A,TRUE,BID_ENA,tcfs,a,b,c,d,e,f,g)
#define BidTraceE8A(tcfs,a,b,c,d,e,f,g,h)   _bid_T8(A,TRUE,BID_ENA,tcfs,a,b,c,d,e,f,g,h)
#define BidTraceE9A(tcfs,a,b,c,d,e,f,g,h,i) _bid_T9(A,TRUE,BID_ENA,tcfs,a,b,c,d,e,f,g,h,i)

#define BidTraceE0W(tcfs)                   _bid_T0(W,TRUE,BID_ENA,tcfs)
#define BidTraceE1W(tcfs,a)                 _bid_T1(W,TRUE,BID_ENA,tcfs,a)
#define BidTraceE2W(tcfs,a,b)               _bid_T2(W,TRUE,BID_ENA,tcfs,a,b)
#define BidTraceE3W(tcfs,a,b,c)             _bid_T3(W,TRUE,BID_ENA,tcfs,a,b,c)
#define BidTraceE4W(tcfs,a,b,c,d)           _bid_T4(W,TRUE,BID_ENA,tcfs,a,b,c,d)
#define BidTraceE5W(tcfs,a,b,c,d,e)         _bid_T5(W,TRUE,BID_ENA,tcfs,a,b,c,d,e)
#define BidTraceE6W(tcfs,a,b,c,d,e,f)       _bid_T6(W,TRUE,BID_ENA,tcfs,a,b,c,d,e,f)
#define BidTraceE7W(tcfs,a,b,c,d,e,f,g)     _bid_T7(W,TRUE,BID_ENA,tcfs,a,b,c,d,e,f,g)
#define BidTraceE8W(tcfs,a,b,c,d,e,f,g,h)   _bid_T8(W,TRUE,BID_ENA,tcfs,a,b,c,d,e,f,g,h)
#define BidTraceE9W(tcfs,a,b,c,d,e,f,g,h,i) _bid_T9(W,TRUE,BID_ENA,tcfs,a,b,c,d,e,f,g,h,i)

#if defined( _UNICODE)
  #define   BidTraceE0      BidTraceE0W
  #define   BidTraceE1      BidTraceE1W
  #define   BidTraceE2      BidTraceE2W
  #define   BidTraceE3      BidTraceE3W
  #define   BidTraceE4      BidTraceE4W
  #define   BidTraceE5      BidTraceE5W
  #define   BidTraceE6      BidTraceE6W
  #define   BidTraceE7      BidTraceE7W
  #define   BidTraceE8      BidTraceE8W
  #define   BidTraceE9      BidTraceE9W
#else
  #define   BidTraceE0      BidTraceE0A
  #define   BidTraceE1      BidTraceE1A
  #define   BidTraceE2      BidTraceE2A
  #define   BidTraceE3      BidTraceE3A
  #define   BidTraceE4      BidTraceE4A
  #define   BidTraceE5      BidTraceE5A
  #define   BidTraceE6      BidTraceE6A
  #define   BidTraceE7      BidTraceE7A
  #define   BidTraceE8      BidTraceE8A
  #define   BidTraceE9      BidTraceE9A
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidTraceU - Uses user-defined, module-specific control flag.
//
#define BidTraceU0A(bit,tcfs)                   _bid_T0(A,bit,0,tcfs)
#define BidTraceU1A(bit,tcfs,a)                 _bid_T1(A,bit,0,tcfs,a)
#define BidTraceU2A(bit,tcfs,a,b)               _bid_T2(A,bit,0,tcfs,a,b)
#define BidTraceU3A(bit,tcfs,a,b,c)             _bid_T3(A,bit,0,tcfs,a,b,c)
#define BidTraceU4A(bit,tcfs,a,b,c,d)           _bid_T4(A,bit,0,tcfs,a,b,c,d)
#define BidTraceU5A(bit,tcfs,a,b,c,d,e)         _bid_T5(A,bit,0,tcfs,a,b,c,d,e)
#define BidTraceU6A(bit,tcfs,a,b,c,d,e,f)       _bid_T6(A,bit,0,tcfs,a,b,c,d,e,f)
#define BidTraceU7A(bit,tcfs,a,b,c,d,e,f,g)     _bid_T7(A,bit,0,tcfs,a,b,c,d,e,f,g)
#define BidTraceU8A(bit,tcfs,a,b,c,d,e,f,g,h)   _bid_T8(A,bit,0,tcfs,a,b,c,d,e,f,g,h)
#define BidTraceU9A(bit,tcfs,a,b,c,d,e,f,g,h,i) _bid_T9(A,bit,0,tcfs,a,b,c,d,e,f,g,h,i)

#define BidTraceU0W(bit,tcfs)                   _bid_T0(W,bit,0,tcfs)
#define BidTraceU1W(bit,tcfs,a)                 _bid_T1(W,bit,0,tcfs,a)
#define BidTraceU2W(bit,tcfs,a,b)               _bid_T2(W,bit,0,tcfs,a,b)
#define BidTraceU3W(bit,tcfs,a,b,c)             _bid_T3(W,bit,0,tcfs,a,b,c)
#define BidTraceU4W(bit,tcfs,a,b,c,d)           _bid_T4(W,bit,0,tcfs,a,b,c,d)
#define BidTraceU5W(bit,tcfs,a,b,c,d,e)         _bid_T5(W,bit,0,tcfs,a,b,c,d,e)
#define BidTraceU6W(bit,tcfs,a,b,c,d,e,f)       _bid_T6(W,bit,0,tcfs,a,b,c,d,e,f)
#define BidTraceU7W(bit,tcfs,a,b,c,d,e,f,g)     _bid_T7(W,bit,0,tcfs,a,b,c,d,e,f,g)
#define BidTraceU8W(bit,tcfs,a,b,c,d,e,f,g,h)   _bid_T8(W,bit,0,tcfs,a,b,c,d,e,f,g,h)
#define BidTraceU9W(bit,tcfs,a,b,c,d,e,f,g,h,i) _bid_T9(W,bit,0,tcfs,a,b,c,d,e,f,g,h,i)

#if defined( _UNICODE)
  #define   BidTraceU0      BidTraceU0W
  #define   BidTraceU1      BidTraceU1W
  #define   BidTraceU2      BidTraceU2W
  #define   BidTraceU3      BidTraceU3W
  #define   BidTraceU4      BidTraceU4W
  #define   BidTraceU5      BidTraceU5W
  #define   BidTraceU6      BidTraceU6W
  #define   BidTraceU7      BidTraceU7W
  #define   BidTraceU8      BidTraceU8W
  #define   BidTraceU9      BidTraceU9W
#else
  #define   BidTraceU0      BidTraceU0A
  #define   BidTraceU1      BidTraceU1A
  #define   BidTraceU2      BidTraceU2A
  #define   BidTraceU3      BidTraceU3A
  #define   BidTraceU4      BidTraceU4A
  #define   BidTraceU5      BidTraceU5A
  #define   BidTraceU6      BidTraceU6A
  #define   BidTraceU7      BidTraceU7A
  #define   BidTraceU8      BidTraceU8A
  #define   BidTraceU9      BidTraceU9A
#endif

//
//  Use the following macros to access User-Defined ApiGroup Control bits.
//  BidIsOn() performs logical OR, BidAreOn() - logical AND. In other words,
//      BidIsOn  - means "trace if at least one of the specified groups is enabled"
//      BidAreOn - means "trace only if all of the specified groups are enabled"
//
//  Example:
//      #define MY_SPECIAL_TRACE    0x00001000
//      #define MY_APIGROUP         0x20000000
//      ...
//      BidTraceU1( BidIsOn(MY_SPECIAL_TRACE), _T("My Special data is %d\n"), MyData );
//
//      BidTraceU1( BidAreOn(MY_APIGROUP|BID_APIGROUP_TRACE),
//                  _T("My ApiGroup-specific data is %d\n"), MyData );
//
#define BidIsOn(Flag)       (_bidT(Flag) != 0)
#define BidAreOn(Flags)     (_bidT(Flags) == (Flags))

//
//  Alias for referencing BID_APIGROUP_ADV.
//  Trace Control String should include "ADV" keyword to help identify tracepoints
//  controlled by BID_APIGROUP_ADV.
//
//  Example:
//      BidTraceU1A( BID_ADV, BID_TAG1A("ADV") "Advanced Output %d\n", dwData );
//
#define BID_ADV             BidIsOn(BID_APIGROUP_ADV)

//
//  BID_APIGROUP_DEFAULT
//
//      BidTraceU1W(BID_DEFAULT, L"<FATAL|ERR> %ls\n", message);
//
#define BID_DEFAULT         BidIsOn(BID_APIGROUP_DEFAULT)


/////////////////////////////////////////////////////////////////////////////////////////////////
//                             DYNAMIC BINDING AND INITIALIZATION                              //
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  List of exportable functions. Assumes that _BID_APIENTRY macro is defined accordingly.
//
#define BID_LIST_API_ENTRIES        \
    _BID_APIENTRY( BidCtlProc )     \
    BID_API_ENTRIES_BUT_CTLPROC

#define BID_API_ENTRIES_BUT_CTLPROC \
    _BID_APIENTRY( BidPutStrA     ) \
    _BID_APIENTRY( BidPutStrW     ) \
    _BID_APIENTRY( BidTraceVA     ) \
    _BID_APIENTRY( BidTraceVW     ) \
    _BID_APIENTRY( BidScopeEnterVA) \
    _BID_APIENTRY( BidScopeEnterVW) \
    _BID_APIENTRY( BidScopeLeave  ) \
    _BID_APIENTRY( BidEnabledA    ) \
    _BID_APIENTRY( BidEnabledW    ) \
    _BID_APIENTRY( BidIndent      ) \
    _BID_APIENTRY( BidSnap        ) \
    _BID_APIENTRY( BidAssert      ) \
    _BID_APIENTRY( BidTouch       )


#define _bidHOOKS_RESERVED_SPOTS    (4)

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Loadable Function Hooks
//
typedef void (WINAPI* BIDUNLOADCB)(BOOL);

#define _BID_APIENTRY(name)     name##_t name;

typedef struct __bidHooks
{
    INT_PTR     BindCookie;
    BIDUNLOADCB UnloadCallback;

    BID_LIST_API_ENTRIES

    FARPROC     Reserved [_bidHOOKS_RESERVED_SPOTS];
    INT_PTR     SanityCheck;

} _bidHooks, * PBIDHOOKS;

extern  _bidHooks   _bidPfn;
extern  HANDLE      _bidHDLL;

#undef  _BID_APIENTRY


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Special allocation for string constants and auto-registration structures
//
typedef enum __bidSectionsEnum
{
    BID_SE_A010,    BID_SE_W010,    //  Source File
    BID_SE_A020,    BID_SE_W020,    //  Reserved.                       ()
    BID_SE_A030,    BID_SE_W030,    //  Trace Control Format Strings    (TCFS)
    BID_SE_A040,    BID_SE_W040,    //  Scope Tracking Formatters       (STF)
    BID_SE_A050,    BID_SE_W050,    //  Event Definition Strings        (EDS)
    BID_SE_A060,    BID_SE_W060,    //  EXtension Auto-Registration     (EXAR)
    BID_SE_A070,    BID_SE_W070,    //  Event Auto-Registration         (EAR)
    BID_SE_A080,    BID_SE_W080,    //  Text Strings Metadata           (METATEXT)
    BID_SE_A090,    BID_SE_W090,    //  Assert Argument Descriptors     (AAD)
    BID_SE_A100,    BID_SE_W100,    //  eXtension Format Strings        (XFS)

    BID_SE_COUNT                    //  Total number of Markers in the array

} BIDSECTIONSENUM;


typedef struct __bidLbl
{
    PCVOID      Label;
    INT_PTR     Hint;

} BIDLBL, * PBIDLBL;

typedef struct __bidMarker
{
    BIDLBL*     Begin;
    PCVOID*     End;

} BIDMARKER, * PBIDMARKER;


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  EntryPoint
//
#define BID_HEADER_SIGNATURE    "*sdbid*"
#define BID_HEADER_BLOCKEND     "-sdbid-"
#define BID_SANITY_CHECK        ((INT_PTR)0x6B5365D8)

typedef struct __bidSectHdr
{
    char        Signature[8];   //  Header Signature.
    PBIDMARKER  Marker;         //  Array of Markers.
    PCSTR       Identity;       //  Module Identification string (ASCII/UTF-8).
    DWORD*      pCfgBits;       //  Build Configuration bits.
    DWORD       Attributes;     //  BID_SE_COUNT(8); SizeInDWORDs(8); Version(16);
    DWORD       Reserved1;      //  Reserved.
    INT_PTR     Reserved2;      //  Reserved.
    INT_PTR     Reserved3;      //  Reserved.
    INT_PTR     Reserved4;      //  Reserved.
    INT_PTR     SanityCheck;    //  Constant to be checked along with Signature.

} BIDSECTHDR, * PBIDSECTHDR;


typedef struct __bidExtInfo
{
    HMODULE     hModule;        //  Module handle (or BID_NOHANDLE if not specified).
    PCWSTR      DomainName;     //  Text Identity of execution context (i.e. managed AppDomain)
    int         Reserved2;      //  Reserved.
    int         Reserved;       //  Reserved.
    PCWSTR      ModulePath;     //  Path to file where the module was loaded from.
    PCSTR       ModulePathA;    //  ASCII/UTF-8 version (optional).
    INT_PTR*    pBindCookie;    //  Address of Binding Cookie.

} BIDEXTINFO, * PBIDEXTINFO;


_bid_DECL_EXPORT(BOOL, BidEntryPoint, (HANDLE* pID, int bInitAndVer, PCSTR sIdentity,
                                DWORD cfgBits, DWORD* pGblFlags, BID_CTLCALLBACK fAddr,
                                PBIDEXTINFO pExtInfo, PBIDHOOKS pHooks, PBIDSECTHDR pHdr));

extern  BidEntryPoint_t         _bidpEntryPoint;

//
//  Modules that do not use C Runtime Library must call the functions below at
//  initialization/finalization phases. In this case, the symbol _BID_NO_CRT must be defined.
//  Also, automatic initialization and/or finalization can be explicitly declined by defining
//  _BID_NO_AUTO_LOAD and/or _BID_NO_AUTO_UNLOAD symbols.
//
int  __cdecl    BidLoad( void );
int  __cdecl    BidUnload( void );

//
//  The BidNotAvailable function tells that there is no available implementation
//  and all hooks were loaded with empty stubs.
//
BOOL WINAPI     BidNotAvailable( void );
#define         BidAvailable()  !BidNotAvailable()

//
//  The BidNotLoaded function tells that external (loadable) implementation is not available
//  and all hooks were set to static defaults (empty stubs or statically linked implementation).
//
BOOL WINAPI     BidNotLoaded( void );

//
//  Forward declaration for selective loader implemented in BidApi_ldr.h
//
int  __cdecl    BidSelectAndLoad( void );

//
//  Explicit shutdown of the diagnostic backend. To be used by apps that may not accurately
//  perform Win32 process shutdown (aka calling TerminateProcess).
//  Note that it's up to BID implementation how to handle this command; it can be just ignored.
//
void WINAPI     BidShutdown(INT_PTR arg);


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Inline implementation for the service functions.
//
#if !defined( _BID_NO_INLINE )

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


#if defined( __cplusplus )

inline _bidCAutoPerf::_bidCAutoPerf(INT_PTR& rEvt, INT_PTR obj)
{
    m_pEvt = &rEvt;
    m_ID   = obj;
    BidSnapStartEx( *m_pEvt, m_ID );
}

inline _bidCAutoPerf::~_bidCAutoPerf()
{
    BidSnapStopEx( *m_pEvt, m_ID );
}

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Diagnostics-related Module Configuration: 'DllBidEntryPoint(.., cfgBits, ..)'
//
#define BID_CFG_MASK_USER           0x0000007F
#define BID_CFG_ACTIVE_USER         0x00000080

#define BID_CFG_ACTIVE_BID          0x80000000
#define BID_CFG_CTLCALLBACK         0x40000000
#define BID_CFG_UTF8                0x20000000

#define BID_CFG_MASK_PAGE           0x10000000
#define BID_CFG_MASK_BID            0x0FFFFF00

#define BID_CFG_DEBUG_BREAK         0x00000100
#define BID_CFG_DEBUG_TRACE         0x00000200
#define BID_CFG_NO_SRCINFO          0x00000400
#define BID_CFG_NO_SPECIAL_ALLOC    0x00000800

#define BID_CFG_PACK_SRCINFO        0x00001000
#define BID_CFG_PACK_LFS            0x00002000
#define BID_CFG_PACK_TCFS           0x00004000
#define BID_CFG_PACK_STF            0x00008000
#define BID_CFG_PACK_EDS            0x00010000
#define BID_CFG_PACK_EXAR           0x00020000
#define BID_CFG_PACK_METATEXT       0x00040000
#define BID_CFG_PACK_AAD            0x00080000
#define BID_CFG_PACK_XFS            0x00100000
#define BID_CFG_PACK_MASK           0x001FF000
//
//  BID_CFG_RESERVED:
//  0x00200000 0x00400000 0x00800000
//  0x01000000 0x02000000 0x04000000
//
#define BID_CFG_HIDDEN              0x08000000

#ifdef _BID_UTF8
  #define _bidCFG_UTF8              BID_CFG_UTF8
#else
  #define _bidCFG_UTF8              0x0
#endif
#ifdef _DEBUG_BREAK_ON
  #define _bidCFG_DBGBRK            BID_CFG_DEBUG_BREAK
#else
  #define _bidCFG_DBGBRK            0x0
#endif
#ifdef _DEBUG_TRACE_ON
  #define _bidCFG_DBGTRC            BID_CFG_DEBUG_TRACE
#else
  #define _bidCFG_DBGTRC            0x0
#endif
#ifdef _BID_NO_SRCINFO
  #define _bidCFG_NOSRC             BID_CFG_NO_SRCINFO
#else
  #define _bidCFG_NOSRC             0x0
#endif
#ifdef _BID_NO_SPECIAL_ALLOCATION
  #define _bidCFG_NOALCK            BID_CFG_NO_SPECIAL_ALLOC
#else
  #define _bidCFG_NOALCK            0x0
#endif

#ifdef _BID_PACK_SRCINFO
  #define _bidCFG_PKSRC             BID_CFG_PACK_SRCINFO
#else
  #define _bidCFG_PKSRC             0x0
#endif
#ifdef _BID_PACK_LFS
  #define _bidCFG_PKLFS             BID_CFG_PACK_LFS
#else
  #define _bidCFG_PKLFS             0x0
#endif
#ifdef _BID_PACK_TCFS
  #define _bidCFG_PKTCFS            BID_CFG_PACK_TCFS
#else
  #define _bidCFG_PKTCFS            0x0
#endif
#ifdef _BID_PACK_STF
  #define _bidCFG_PKSTF             BID_CFG_PACK_STF
#else
  #define _bidCFG_PKSTF             0x0
#endif
#ifdef _BID_PACK_EDS
  #define _bidCFG_PKEDS             BID_CFG_PACK_EDS
#else
  #define _bidCFG_PKEDS             0x0
#endif
#ifdef _BID_PACK_EXAR
  #define _bidCFG_PKEXAR            BID_CFG_PACK_EXAR
#else
  #define _bidCFG_PKEXAR            0x0
#endif
#ifdef _BID_PACK_METATEXT
  #define _bidCFG_PKMETATEXT        BID_CFG_PACK_METATEXT
#else
  #define _bidCFG_PKMETATEXT        0x0
#endif
#ifdef _BID_PACK_AAD
  #define _bidCFG_PKAAD             BID_CFG_PACK_AAD
#else
  #define _bidCFG_PKAAD             0x0
#endif
#ifdef _BID_PACK_XFS
  #define _bidCFG_PKXFS             BID_CFG_PACK_XFS
#else
  #define _bidCFG_PKXFS             0x0
#endif

#define _bidCFG_SUM                 (BID_CFG_ACTIVE_BID|_bidCFG_UTF8            \
                                    |_bidCFG_DBGBRK|_bidCFG_DBGTRC              \
                                    |_bidCFG_NOALCK|_bidCFG_NOSRC               \
                                    |_bidCFG_PKSRC|_bidCFG_PKLFS|_bidCFG_PKTCFS \
                                    |_bidCFG_PKSTF|_bidCFG_PKEDS|_bidCFG_PKEXAR \
                                    |_bidCFG_PKMETATEXT|_bidCFG_PKAAD|_bidCFG_PKXFS)

extern DWORD _bidCfgBits;


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Custom ANSI/UNICODE abstraction for WINAPI functions used in dynamic loader (Part I)
//
#ifdef _BID_UNICODE_LOADER

  #define BID_T(literal)    L ## literal
  #define BID_TCHAR         WCHAR
  #define BID_PTSTR         LPWSTR
  #define BID_PCTSTR        LPCWSTR

#else

  #define BID_T(literal)    literal
  #define BID_TCHAR         char
  #define BID_PTSTR         LPSTR
  #define BID_PCTSTR        LPCSTR

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN64

  #ifndef _BID_NO_SRCINFO_ALL
    #pragma section("BID$A012",long,read,write)
    #pragma section("BID$W012",long,read,write)
  #endif

  #ifndef _BID_NO_SPECIAL_ALLOCATION
    #pragma section("BIDL$A022",long,read,write)
    #pragma section("BIDL$W022",long,read,write)

    #pragma section("BID$A032",long,read,write)
    #pragma section("BID$W032",long,read,write)
    #pragma section("BID$A042",long,read,write)
    #pragma section("BID$W042",long,read,write)
    #pragma section("BID$A052",long,read,write)
    #pragma section("BID$W052",long,read,write)
    #pragma section("BID$A062",long,read,write)
    #pragma section("BID$W062",long,read,write)
    #pragma section("BID$A072",long,read,write)
    #pragma section("BID$W072",long,read,write)
    #pragma section("BID$A082",long,read,write)
    #pragma section("BID$W082",long,read,write)
    #pragma section("BID$A092",long,read,write)
    #pragma section("BID$W092",long,read,write)
    #pragma section("BID$A102",long,read,write)
    #pragma section("BID$W102",long,read,write)

    #pragma section("BID$Z002",long,read,write)
    #pragma section("BID$Z005",long,read,write)
  #endif

#else   // _WIN32

  #ifndef _BID_NO_SRCINFO_ALL
    #pragma data_seg("BID$A012")
    #pragma data_seg("BID$W012")
    #pragma data_seg()
  #endif

  #ifndef _BID_NO_SPECIAL_ALLOCATION
    #pragma data_seg("BIDL$A022")
    #pragma data_seg("BIDL$W022")

    #pragma data_seg("BID$A032")
    #pragma data_seg("BID$W032")
    #pragma data_seg("BID$A042")
    #pragma data_seg("BID$W042")
    #pragma data_seg("BID$A052")
    #pragma data_seg("BID$W052")
    #pragma data_seg("BID$A062")
    #pragma data_seg("BID$W062")
    #pragma data_seg("BID$A072")
    #pragma data_seg("BID$W072")
    #pragma data_seg("BID$A082")
    #pragma data_seg("BID$W082")
    #pragma data_seg("BID$A092")
    #pragma data_seg("BID$W092")
    #pragma data_seg("BID$A102")
    #pragma data_seg("BID$W102")

    #pragma data_seg("BID$Z002")
    #pragma data_seg("BID$Z005")
    #pragma data_seg()
  #endif

#endif

#undef  _bid_DECL
#undef  _bid_DECL_EXPORT
#undef  _bid_CDECL

#ifdef __cplusplus
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BidApi Customization.
//  Usage: cl ... -D_BID_CUSTOMIZE=\"YourPath/YourProfile.Ext\"
//  YourProfile.Ext usually #define/#undef some symbols to customize particular build
//
#ifdef _BID_CUSTOMIZE
  #ifndef _BID_CUSTOMIZE_INCLUDED_
    #define _BID_CUSTOMIZE_INCLUDED_
    #pragma message("NOTE: BidApi.h includes _BID_CUSTOMIZE=" _BID_CUSTOMIZE)
    #include _BID_CUSTOMIZE
  #endif
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////

#else   // _BID_DECLARED
  //
  //  Built-In Diagnostics API already declared, but not via BidApi.h.
  //  Consider replacement for BidApi.h & BidApi_impl.h
  //
  #pragma message("NOTE: Built-In Diagnostics API already declared.")

#endif  // _BID_DECLARED

#endif //////////////////////////////////////////////////////////////////////////////////////////
//                                   End of file "BidApi.h"                                    //
/////////////////////////////////////////////////////////////////////////////////////////////////
