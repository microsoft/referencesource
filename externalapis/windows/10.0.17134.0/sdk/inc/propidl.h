

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __propidl_h__
#define __propidl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPropertyStorage_FWD_DEFINED__
#define __IPropertyStorage_FWD_DEFINED__
typedef interface IPropertyStorage IPropertyStorage;

#endif 	/* __IPropertyStorage_FWD_DEFINED__ */


#ifndef __IPropertySetStorage_FWD_DEFINED__
#define __IPropertySetStorage_FWD_DEFINED__
typedef interface IPropertySetStorage IPropertySetStorage;

#endif 	/* __IPropertySetStorage_FWD_DEFINED__ */


#ifndef __IEnumSTATPROPSTG_FWD_DEFINED__
#define __IEnumSTATPROPSTG_FWD_DEFINED__
typedef interface IEnumSTATPROPSTG IEnumSTATPROPSTG;

#endif 	/* __IEnumSTATPROPSTG_FWD_DEFINED__ */


#ifndef __IEnumSTATPROPSETSTG_FWD_DEFINED__
#define __IEnumSTATPROPSETSTG_FWD_DEFINED__
typedef interface IEnumSTATPROPSETSTG IEnumSTATPROPSETSTG;

#endif 	/* __IEnumSTATPROPSETSTG_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_propidl_0000_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------
#if ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(push)
#pragma warning(disable:4820)    /* padding added after data member */
#endif
#pragma warning(disable:4201)    /* Nameless struct/union */
#pragma warning(disable:4237)    /* obsolete member named 'bool' */
#endif
#if ( _MSC_VER >= 1020 )
#pragma once
#endif
#include <winapifamily.h>
#ifndef _PROPIDLBASE_
#pragma region Application Family or OneCore Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM)



typedef struct tagVersionedStream
    {
    GUID guidVersion;
    IStream *pStream;
    } 	VERSIONEDSTREAM;

typedef struct tagVersionedStream *LPVERSIONEDSTREAM;


// Flags for IPropertySetStorage::Create
#define	PROPSETFLAG_DEFAULT	( 0 )

#define	PROPSETFLAG_NONSIMPLE	( 1 )

#define	PROPSETFLAG_ANSI	( 2 )

//   (This flag is only supported on StgCreatePropStg & StgOpenPropStg
#define	PROPSETFLAG_UNBUFFERED	( 4 )

//   (This flag causes a version-1 property set to be created
#define	PROPSETFLAG_CASE_SENSITIVE	( 8 )


// Flags for the reserved PID_BEHAVIOR property
#define	PROPSET_BEHAVIOR_CASE_SENSITIVE	( 1 )

#ifdef MIDL_PASS
// This is the PROPVARIANT definition for marshaling.
typedef struct tag_inner_PROPVARIANT PROPVARIANT;

#else
// This is the standard C layout of the PROPVARIANT.
typedef struct tagPROPVARIANT PROPVARIANT;
#endif
typedef struct tagCAC
    {
    ULONG cElems;
    /* [size_is] */ CHAR *pElems;
    } 	CAC;

typedef struct tagCAUB
    {
    ULONG cElems;
    /* [size_is] */ UCHAR *pElems;
    } 	CAUB;

typedef struct tagCAI
    {
    ULONG cElems;
    /* [size_is] */ SHORT *pElems;
    } 	CAI;

typedef struct tagCAUI
    {
    ULONG cElems;
    /* [size_is] */ USHORT *pElems;
    } 	CAUI;

typedef struct tagCAL
    {
    ULONG cElems;
    /* [size_is] */ LONG *pElems;
    } 	CAL;

typedef struct tagCAUL
    {
    ULONG cElems;
    /* [size_is] */ ULONG *pElems;
    } 	CAUL;

typedef struct tagCAFLT
    {
    ULONG cElems;
    /* [size_is] */ FLOAT *pElems;
    } 	CAFLT;

typedef struct tagCADBL
    {
    ULONG cElems;
    /* [size_is] */ DOUBLE *pElems;
    } 	CADBL;

typedef struct tagCACY
    {
    ULONG cElems;
    /* [size_is] */ CY *pElems;
    } 	CACY;

typedef struct tagCADATE
    {
    ULONG cElems;
    /* [size_is] */ DATE *pElems;
    } 	CADATE;

typedef struct tagCABSTR
    {
    ULONG cElems;
    /* [size_is] */ BSTR *pElems;
    } 	CABSTR;

typedef struct tagCABSTRBLOB
    {
    ULONG cElems;
    /* [size_is] */ BSTRBLOB *pElems;
    } 	CABSTRBLOB;

typedef struct tagCABOOL
    {
    ULONG cElems;
    /* [size_is] */ VARIANT_BOOL *pElems;
    } 	CABOOL;

typedef struct tagCASCODE
    {
    ULONG cElems;
    /* [size_is] */ SCODE *pElems;
    } 	CASCODE;

typedef struct tagCAPROPVARIANT
    {
    ULONG cElems;
    /* [size_is] */ PROPVARIANT *pElems;
    } 	CAPROPVARIANT;

typedef struct tagCAH
    {
    ULONG cElems;
    /* [size_is] */ LARGE_INTEGER *pElems;
    } 	CAH;

typedef struct tagCAUH
    {
    ULONG cElems;
    /* [size_is] */ ULARGE_INTEGER *pElems;
    } 	CAUH;

typedef struct tagCALPSTR
    {
    ULONG cElems;
    /* [size_is] */ LPSTR *pElems;
    } 	CALPSTR;

typedef struct tagCALPWSTR
    {
    ULONG cElems;
    /* [size_is] */ LPWSTR *pElems;
    } 	CALPWSTR;

typedef struct tagCAFILETIME
    {
    ULONG cElems;
    /* [size_is] */ FILETIME *pElems;
    } 	CAFILETIME;

typedef struct tagCACLIPDATA
    {
    ULONG cElems;
    /* [size_is] */ CLIPDATA *pElems;
    } 	CACLIPDATA;

typedef struct tagCACLSID
    {
    ULONG cElems;
    /* [size_is] */ CLSID *pElems;
    } 	CACLSID;

#ifdef MIDL_PASS
// This is the PROPVARIANT padding layout for marshaling.
typedef BYTE PROPVAR_PAD1;

typedef BYTE PROPVAR_PAD2;

typedef ULONG PROPVAR_PAD3;

#else
// This is the standard C layout of the structure.
typedef WORD PROPVAR_PAD1;
typedef WORD PROPVAR_PAD2;
typedef WORD PROPVAR_PAD3;
#define tag_inner_PROPVARIANT
#endif

#if !defined(_MSC_EXTENSIONS)

struct tagPROPVARIANT;

#else
#ifndef MIDL_PASS
struct tagPROPVARIANT {
  union {
#endif
struct tag_inner_PROPVARIANT
    {
    VARTYPE vt;
    PROPVAR_PAD1 wReserved1;
    PROPVAR_PAD2 wReserved2;
    PROPVAR_PAD3 wReserved3;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */  /* Empty union arm */ 
        /* [case()] */ CHAR cVal;
        /* [case()] */ UCHAR bVal;
        /* [case()] */ SHORT iVal;
        /* [case()] */ USHORT uiVal;
        /* [case()] */ LONG lVal;
        /* [case()] */ ULONG ulVal;
        /* [case()] */ INT intVal;
        /* [case()] */ UINT uintVal;
        /* [case()] */ LARGE_INTEGER hVal;
        /* [case()] */ ULARGE_INTEGER uhVal;
        /* [case()] */ FLOAT fltVal;
        /* [case()] */ DOUBLE dblVal;
        /* [case()] */ VARIANT_BOOL boolVal;
        /* [case()] */ _VARIANT_BOOL bool;
        /* [case()] */ SCODE scode;
        /* [case()] */ CY cyVal;
        /* [case()] */ DATE date;
        /* [case()] */ FILETIME filetime;
        /* [case()] */ CLSID *puuid;
        /* [case()] */ CLIPDATA *pclipdata;
        /* [case()] */ BSTR bstrVal;
        /* [case()] */ BSTRBLOB bstrblobVal;
        /* [case()] */ BLOB blob;
        /* [case()] */ LPSTR pszVal;
        /* [case()] */ LPWSTR pwszVal;
        /* [case()] */ IUnknown *punkVal;
        /* [case()] */ IDispatch *pdispVal;
        /* [case()] */ IStream *pStream;
        /* [case()] */ IStorage *pStorage;
        /* [case()] */ LPVERSIONEDSTREAM pVersionedStream;
        /* [case()] */ LPSAFEARRAY parray;
        /* [case()] */ CAC cac;
        /* [case()] */ CAUB caub;
        /* [case()] */ CAI cai;
        /* [case()] */ CAUI caui;
        /* [case()] */ CAL cal;
        /* [case()] */ CAUL caul;
        /* [case()] */ CAH cah;
        /* [case()] */ CAUH cauh;
        /* [case()] */ CAFLT caflt;
        /* [case()] */ CADBL cadbl;
        /* [case()] */ CABOOL cabool;
        /* [case()] */ CASCODE cascode;
        /* [case()] */ CACY cacy;
        /* [case()] */ CADATE cadate;
        /* [case()] */ CAFILETIME cafiletime;
        /* [case()] */ CACLSID cauuid;
        /* [case()] */ CACLIPDATA caclipdata;
        /* [case()] */ CABSTR cabstr;
        /* [case()] */ CABSTRBLOB cabstrblob;
        /* [case()] */ CALPSTR calpstr;
        /* [case()] */ CALPWSTR calpwstr;
        /* [case()] */ CAPROPVARIANT capropvar;
        /* [case()] */ CHAR *pcVal;
        /* [case()] */ UCHAR *pbVal;
        /* [case()] */ SHORT *piVal;
        /* [case()] */ USHORT *puiVal;
        /* [case()] */ LONG *plVal;
        /* [case()] */ ULONG *pulVal;
        /* [case()] */ INT *pintVal;
        /* [case()] */ UINT *puintVal;
        /* [case()] */ FLOAT *pfltVal;
        /* [case()] */ DOUBLE *pdblVal;
        /* [case()] */ VARIANT_BOOL *pboolVal;
        /* [case()] */ DECIMAL *pdecVal;
        /* [case()] */ SCODE *pscode;
        /* [case()] */ CY *pcyVal;
        /* [case()] */ DATE *pdate;
        /* [case()] */ BSTR *pbstrVal;
        /* [case()] */ IUnknown **ppunkVal;
        /* [case()] */ IDispatch **ppdispVal;
        /* [case()] */ LPSAFEARRAY *pparray;
        /* [case()] */ PROPVARIANT *pvarVal;
        } 	;
    } ;
#ifndef MIDL_PASS
    DECIMAL decVal;
  };
};
#endif

#endif /* _MSC_EXTENSIONS */

#ifdef MIDL_PASS
// This is the LPPROPVARIANT definition for marshaling.
typedef struct tag_inner_PROPVARIANT *LPPROPVARIANT;

typedef const PROPVARIANT *REFPROPVARIANT;

#else

// This is the standard C layout of the PROPVARIANT.
typedef struct tagPROPVARIANT * LPPROPVARIANT;

#ifndef _REFPROPVARIANT_DEFINED
#define _REFPROPVARIANT_DEFINED
#ifdef __cplusplus
#define REFPROPVARIANT const PROPVARIANT &
#else
#define REFPROPVARIANT const PROPVARIANT * __MIDL_CONST
#endif
#endif

#endif // MIDL_PASS

// Reserved global Property IDs
#define	PID_DICTIONARY	( 0 )

#define	PID_CODEPAGE	( 0x1 )

#define	PID_FIRST_USABLE	( 0x2 )

#define	PID_FIRST_NAME_DEFAULT	( 0xfff )

#define	PID_LOCALE	( 0x80000000 )

#define	PID_MODIFY_TIME	( 0x80000001 )

#define	PID_SECURITY	( 0x80000002 )

#define	PID_BEHAVIOR	( 0x80000003 )

#define	PID_ILLEGAL	( 0xffffffff )

// Range which is read-only to downlevel implementations
#define	PID_MIN_READONLY	( 0x80000000 )

#define	PID_MAX_READONLY	( 0xbfffffff )

#define	PRSPEC_INVALID	( 0xffffffff )

#define	PRSPEC_LPWSTR	( 0 )

#define	PRSPEC_PROPID	( 1 )

typedef struct tagPROPSPEC
    {
    ULONG ulKind;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ PROPID propid;
        /* [case()] */ LPOLESTR lpwstr;
        /* [default] */  /* Empty union arm */ 
        } 	DUMMYUNIONNAME;
    } 	PROPSPEC;

typedef struct tagSTATPROPSTG
    {
    LPOLESTR lpwstrName;
    PROPID propid;
    VARTYPE vt;
    } 	STATPROPSTG;

// Macros for parsing the OS Version of the Property Set Header
#define PROPSETHDR_OSVER_KIND(dwOSVer)      HIWORD( (dwOSVer) )
#define PROPSETHDR_OSVER_MAJOR(dwOSVer)     LOBYTE(LOWORD( (dwOSVer) ))
#define PROPSETHDR_OSVER_MINOR(dwOSVer)     HIBYTE(LOWORD( (dwOSVer) ))
#define PROPSETHDR_OSVERSION_UNKNOWN        0xFFFFFFFF
typedef struct tagSTATPROPSETSTG
    {
    FMTID fmtid;
    CLSID clsid;
    DWORD grfFlags;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD dwOSVersion;
    } 	STATPROPSETSTG;



extern RPC_IF_HANDLE __MIDL_itf_propidl_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_propidl_0000_0000_v0_0_s_ifspec;

#ifndef __IPropertyStorage_INTERFACE_DEFINED__
#define __IPropertyStorage_INTERFACE_DEFINED__

/* interface IPropertyStorage */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IPropertyStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000138-0000-0000-C000-000000000046")
    IPropertyStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReadMultiple( 
            /* [in] */ ULONG cpspec,
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPSPEC rgpspec[  ],
            /* [size_is][out] */ __RPC__out_ecount_full(cpspec) PROPVARIANT rgpropvar[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteMultiple( 
            /* [in] */ ULONG cpspec,
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPSPEC rgpspec[  ],
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPVARIANT rgpropvar[  ],
            /* [in] */ PROPID propidNameFirst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMultiple( 
            /* [in] */ ULONG cpspec,
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPSPEC rgpspec[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadPropertyNames( 
            /* [in] */ ULONG cpropid,
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const PROPID rgpropid[  ],
            /* [size_is][out] */ __RPC__out_ecount_full(cpropid) LPOLESTR rglpwstrName[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WritePropertyNames( 
            /* [in] */ ULONG cpropid,
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const PROPID rgpropid[  ],
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const LPOLESTR rglpwstrName[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeletePropertyNames( 
            /* [in] */ ULONG cpropid,
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const PROPID rgpropid[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ DWORD grfCommitFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Revert( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enum( 
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSTG **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimes( 
            /* [in] */ __RPC__in const FILETIME *pctime,
            /* [in] */ __RPC__in const FILETIME *patime,
            /* [in] */ __RPC__in const FILETIME *pmtime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClass( 
            /* [in] */ __RPC__in REFCLSID clsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stat( 
            /* [out] */ __RPC__out STATPROPSETSTG *pstatpsstg) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPropertyStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IPropertyStorage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IPropertyStorage * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReadMultiple )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ ULONG cpspec,
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPSPEC rgpspec[  ],
            /* [size_is][out] */ __RPC__out_ecount_full(cpspec) PROPVARIANT rgpropvar[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *WriteMultiple )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ ULONG cpspec,
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPSPEC rgpspec[  ],
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPVARIANT rgpropvar[  ],
            /* [in] */ PROPID propidNameFirst);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMultiple )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ ULONG cpspec,
            /* [size_is][in] */ __RPC__in_ecount_full(cpspec) const PROPSPEC rgpspec[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *ReadPropertyNames )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ ULONG cpropid,
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const PROPID rgpropid[  ],
            /* [size_is][out] */ __RPC__out_ecount_full(cpropid) LPOLESTR rglpwstrName[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *WritePropertyNames )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ ULONG cpropid,
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const PROPID rgpropid[  ],
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const LPOLESTR rglpwstrName[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *DeletePropertyNames )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ ULONG cpropid,
            /* [size_is][in] */ __RPC__in_ecount_full(cpropid) const PROPID rgpropid[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Revert )( 
            __RPC__in IPropertyStorage * This);
        
        HRESULT ( STDMETHODCALLTYPE *Enum )( 
            __RPC__in IPropertyStorage * This,
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSTG **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *SetTimes )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ __RPC__in const FILETIME *pctime,
            /* [in] */ __RPC__in const FILETIME *patime,
            /* [in] */ __RPC__in const FILETIME *pmtime);
        
        HRESULT ( STDMETHODCALLTYPE *SetClass )( 
            __RPC__in IPropertyStorage * This,
            /* [in] */ __RPC__in REFCLSID clsid);
        
        HRESULT ( STDMETHODCALLTYPE *Stat )( 
            __RPC__in IPropertyStorage * This,
            /* [out] */ __RPC__out STATPROPSETSTG *pstatpsstg);
        
        END_INTERFACE
    } IPropertyStorageVtbl;

    interface IPropertyStorage
    {
        CONST_VTBL struct IPropertyStorageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertyStorage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPropertyStorage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPropertyStorage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPropertyStorage_ReadMultiple(This,cpspec,rgpspec,rgpropvar)	\
    ( (This)->lpVtbl -> ReadMultiple(This,cpspec,rgpspec,rgpropvar) ) 

#define IPropertyStorage_WriteMultiple(This,cpspec,rgpspec,rgpropvar,propidNameFirst)	\
    ( (This)->lpVtbl -> WriteMultiple(This,cpspec,rgpspec,rgpropvar,propidNameFirst) ) 

#define IPropertyStorage_DeleteMultiple(This,cpspec,rgpspec)	\
    ( (This)->lpVtbl -> DeleteMultiple(This,cpspec,rgpspec) ) 

#define IPropertyStorage_ReadPropertyNames(This,cpropid,rgpropid,rglpwstrName)	\
    ( (This)->lpVtbl -> ReadPropertyNames(This,cpropid,rgpropid,rglpwstrName) ) 

#define IPropertyStorage_WritePropertyNames(This,cpropid,rgpropid,rglpwstrName)	\
    ( (This)->lpVtbl -> WritePropertyNames(This,cpropid,rgpropid,rglpwstrName) ) 

#define IPropertyStorage_DeletePropertyNames(This,cpropid,rgpropid)	\
    ( (This)->lpVtbl -> DeletePropertyNames(This,cpropid,rgpropid) ) 

#define IPropertyStorage_Commit(This,grfCommitFlags)	\
    ( (This)->lpVtbl -> Commit(This,grfCommitFlags) ) 

#define IPropertyStorage_Revert(This)	\
    ( (This)->lpVtbl -> Revert(This) ) 

#define IPropertyStorage_Enum(This,ppenum)	\
    ( (This)->lpVtbl -> Enum(This,ppenum) ) 

#define IPropertyStorage_SetTimes(This,pctime,patime,pmtime)	\
    ( (This)->lpVtbl -> SetTimes(This,pctime,patime,pmtime) ) 

#define IPropertyStorage_SetClass(This,clsid)	\
    ( (This)->lpVtbl -> SetClass(This,clsid) ) 

#define IPropertyStorage_Stat(This,pstatpsstg)	\
    ( (This)->lpVtbl -> Stat(This,pstatpsstg) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPropertyStorage_INTERFACE_DEFINED__ */


#ifndef __IPropertySetStorage_INTERFACE_DEFINED__
#define __IPropertySetStorage_INTERFACE_DEFINED__

/* interface IPropertySetStorage */
/* [unique][uuid][object] */ 

typedef /* [unique] */  __RPC_unique_pointer IPropertySetStorage *LPPROPERTYSETSTORAGE;


EXTERN_C const IID IID_IPropertySetStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000013A-0000-0000-C000-000000000046")
    IPropertySetStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Create( 
            /* [in] */ __RPC__in REFFMTID rfmtid,
            /* [unique][in] */ __RPC__in_opt const CLSID *pclsid,
            /* [in] */ DWORD grfFlags,
            /* [in] */ DWORD grfMode,
            /* [out] */ __RPC__deref_out_opt IPropertyStorage **ppprstg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ __RPC__in REFFMTID rfmtid,
            /* [in] */ DWORD grfMode,
            /* [out] */ __RPC__deref_out_opt IPropertyStorage **ppprstg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ __RPC__in REFFMTID rfmtid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enum( 
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSETSTG **ppenum) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IPropertySetStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IPropertySetStorage * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IPropertySetStorage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IPropertySetStorage * This);
        
        HRESULT ( STDMETHODCALLTYPE *Create )( 
            __RPC__in IPropertySetStorage * This,
            /* [in] */ __RPC__in REFFMTID rfmtid,
            /* [unique][in] */ __RPC__in_opt const CLSID *pclsid,
            /* [in] */ DWORD grfFlags,
            /* [in] */ DWORD grfMode,
            /* [out] */ __RPC__deref_out_opt IPropertyStorage **ppprstg);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            __RPC__in IPropertySetStorage * This,
            /* [in] */ __RPC__in REFFMTID rfmtid,
            /* [in] */ DWORD grfMode,
            /* [out] */ __RPC__deref_out_opt IPropertyStorage **ppprstg);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            __RPC__in IPropertySetStorage * This,
            /* [in] */ __RPC__in REFFMTID rfmtid);
        
        HRESULT ( STDMETHODCALLTYPE *Enum )( 
            __RPC__in IPropertySetStorage * This,
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSETSTG **ppenum);
        
        END_INTERFACE
    } IPropertySetStorageVtbl;

    interface IPropertySetStorage
    {
        CONST_VTBL struct IPropertySetStorageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropertySetStorage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPropertySetStorage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPropertySetStorage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPropertySetStorage_Create(This,rfmtid,pclsid,grfFlags,grfMode,ppprstg)	\
    ( (This)->lpVtbl -> Create(This,rfmtid,pclsid,grfFlags,grfMode,ppprstg) ) 

#define IPropertySetStorage_Open(This,rfmtid,grfMode,ppprstg)	\
    ( (This)->lpVtbl -> Open(This,rfmtid,grfMode,ppprstg) ) 

#define IPropertySetStorage_Delete(This,rfmtid)	\
    ( (This)->lpVtbl -> Delete(This,rfmtid) ) 

#define IPropertySetStorage_Enum(This,ppenum)	\
    ( (This)->lpVtbl -> Enum(This,ppenum) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPropertySetStorage_INTERFACE_DEFINED__ */


#ifndef __IEnumSTATPROPSTG_INTERFACE_DEFINED__
#define __IEnumSTATPROPSTG_INTERFACE_DEFINED__

/* interface IEnumSTATPROPSTG */
/* [unique][uuid][object] */ 

typedef /* [unique] */  __RPC_unique_pointer IEnumSTATPROPSTG *LPENUMSTATPROPSTG;


EXTERN_C const IID IID_IEnumSTATPROPSTG;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00000139-0000-0000-C000-000000000046")
    IEnumSTATPROPSTG : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [annotation][length_is][size_is][out] */ 
            _Out_writes_to_(celt, *pceltFetched)  STATPROPSTG *rgelt,
            /* [annotation][out] */ 
            _Out_opt_ _Deref_out_range_(0, celt)  ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSTG **ppenum) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IEnumSTATPROPSTGVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IEnumSTATPROPSTG * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IEnumSTATPROPSTG * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IEnumSTATPROPSTG * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSTATPROPSTG * This,
            /* [in] */ ULONG celt,
            /* [annotation][length_is][size_is][out] */ 
            _Out_writes_to_(celt, *pceltFetched)  STATPROPSTG *rgelt,
            /* [annotation][out] */ 
            _Out_opt_ _Deref_out_range_(0, celt)  ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            __RPC__in IEnumSTATPROPSTG * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            __RPC__in IEnumSTATPROPSTG * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            __RPC__in IEnumSTATPROPSTG * This,
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSTG **ppenum);
        
        END_INTERFACE
    } IEnumSTATPROPSTGVtbl;

    interface IEnumSTATPROPSTG
    {
        CONST_VTBL struct IEnumSTATPROPSTGVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSTATPROPSTG_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IEnumSTATPROPSTG_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IEnumSTATPROPSTG_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IEnumSTATPROPSTG_Next(This,celt,rgelt,pceltFetched)	\
    ( (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched) ) 

#define IEnumSTATPROPSTG_Skip(This,celt)	\
    ( (This)->lpVtbl -> Skip(This,celt) ) 

#define IEnumSTATPROPSTG_Reset(This)	\
    ( (This)->lpVtbl -> Reset(This) ) 

#define IEnumSTATPROPSTG_Clone(This,ppenum)	\
    ( (This)->lpVtbl -> Clone(This,ppenum) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumSTATPROPSTG_RemoteNext_Proxy( 
    __RPC__in IEnumSTATPROPSTG * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ __RPC__out_ecount_part(celt, *pceltFetched) STATPROPSTG *rgelt,
    /* [out] */ __RPC__out ULONG *pceltFetched);


void __RPC_STUB IEnumSTATPROPSTG_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSTATPROPSTG_INTERFACE_DEFINED__ */


#ifndef __IEnumSTATPROPSETSTG_INTERFACE_DEFINED__
#define __IEnumSTATPROPSETSTG_INTERFACE_DEFINED__

/* interface IEnumSTATPROPSETSTG */
/* [unique][uuid][object] */ 

typedef /* [unique] */  __RPC_unique_pointer IEnumSTATPROPSETSTG *LPENUMSTATPROPSETSTG;


EXTERN_C const IID IID_IEnumSTATPROPSETSTG;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0000013B-0000-0000-C000-000000000046")
    IEnumSTATPROPSETSTG : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [annotation][length_is][size_is][out] */ 
            _Out_writes_to_(celt, *pceltFetched)  STATPROPSETSTG *rgelt,
            /* [annotation][out] */ 
            _Out_opt_ _Deref_out_range_(0, celt)  ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSETSTG **ppenum) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IEnumSTATPROPSETSTGVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IEnumSTATPROPSETSTG * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IEnumSTATPROPSETSTG * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IEnumSTATPROPSETSTG * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumSTATPROPSETSTG * This,
            /* [in] */ ULONG celt,
            /* [annotation][length_is][size_is][out] */ 
            _Out_writes_to_(celt, *pceltFetched)  STATPROPSETSTG *rgelt,
            /* [annotation][out] */ 
            _Out_opt_ _Deref_out_range_(0, celt)  ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            __RPC__in IEnumSTATPROPSETSTG * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            __RPC__in IEnumSTATPROPSETSTG * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            __RPC__in IEnumSTATPROPSETSTG * This,
            /* [out] */ __RPC__deref_out_opt IEnumSTATPROPSETSTG **ppenum);
        
        END_INTERFACE
    } IEnumSTATPROPSETSTGVtbl;

    interface IEnumSTATPROPSETSTG
    {
        CONST_VTBL struct IEnumSTATPROPSETSTGVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSTATPROPSETSTG_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IEnumSTATPROPSETSTG_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IEnumSTATPROPSETSTG_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IEnumSTATPROPSETSTG_Next(This,celt,rgelt,pceltFetched)	\
    ( (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched) ) 

#define IEnumSTATPROPSETSTG_Skip(This,celt)	\
    ( (This)->lpVtbl -> Skip(This,celt) ) 

#define IEnumSTATPROPSETSTG_Reset(This)	\
    ( (This)->lpVtbl -> Reset(This) ) 

#define IEnumSTATPROPSETSTG_Clone(This,ppenum)	\
    ( (This)->lpVtbl -> Clone(This,ppenum) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumSTATPROPSETSTG_RemoteNext_Proxy( 
    __RPC__in IEnumSTATPROPSETSTG * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ __RPC__out_ecount_part(celt, *pceltFetched) STATPROPSETSTG *rgelt,
    /* [out] */ __RPC__out ULONG *pceltFetched);


void __RPC_STUB IEnumSTATPROPSETSTG_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSTATPROPSETSTG_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_propidl_0000_0004 */
/* [local] */ 

typedef /* [unique] */  __RPC_unique_pointer IPropertyStorage *LPPROPERTYSTORAGE;

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) */
#pragma endregion

#define _PROPIDLBASE_
#endif
#include <coml2api.h>
#pragma region Desktop Family or OneCore Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
// Property IDs for the DiscardableInformation Property Set

#define PIDDI_THUMBNAIL          0x00000002L // VT_BLOB

// Property IDs for the SummaryInformation Property Set

#define PIDSI_TITLE               0x00000002L  // VT_LPSTR
#define PIDSI_SUBJECT             0x00000003L  // VT_LPSTR
#define PIDSI_AUTHOR              0x00000004L  // VT_LPSTR
#define PIDSI_KEYWORDS            0x00000005L  // VT_LPSTR
#define PIDSI_COMMENTS            0x00000006L  // VT_LPSTR
#define PIDSI_TEMPLATE            0x00000007L  // VT_LPSTR
#define PIDSI_LASTAUTHOR          0x00000008L  // VT_LPSTR
#define PIDSI_REVNUMBER           0x00000009L  // VT_LPSTR
#define PIDSI_EDITTIME            0x0000000aL  // VT_FILETIME (UTC)
#define PIDSI_LASTPRINTED         0x0000000bL  // VT_FILETIME (UTC)
#define PIDSI_CREATE_DTM          0x0000000cL  // VT_FILETIME (UTC)
#define PIDSI_LASTSAVE_DTM        0x0000000dL  // VT_FILETIME (UTC)
#define PIDSI_PAGECOUNT           0x0000000eL  // VT_I4
#define PIDSI_WORDCOUNT           0x0000000fL  // VT_I4
#define PIDSI_CHARCOUNT           0x00000010L  // VT_I4
#define PIDSI_THUMBNAIL           0x00000011L  // VT_CF
#define PIDSI_APPNAME             0x00000012L  // VT_LPSTR
#define PIDSI_DOC_SECURITY        0x00000013L  // VT_I4

// Property IDs for the DocSummaryInformation Property Set

#define PIDDSI_CATEGORY          0x00000002 // VT_LPSTR
#define PIDDSI_PRESFORMAT        0x00000003 // VT_LPSTR
#define PIDDSI_BYTECOUNT         0x00000004 // VT_I4
#define PIDDSI_LINECOUNT         0x00000005 // VT_I4
#define PIDDSI_PARCOUNT          0x00000006 // VT_I4
#define PIDDSI_SLIDECOUNT        0x00000007 // VT_I4
#define PIDDSI_NOTECOUNT         0x00000008 // VT_I4
#define PIDDSI_HIDDENCOUNT       0x00000009 // VT_I4
#define PIDDSI_MMCLIPCOUNT       0x0000000A // VT_I4
#define PIDDSI_SCALE             0x0000000B // VT_BOOL
#define PIDDSI_HEADINGPAIR       0x0000000C // VT_VARIANT | VT_VECTOR
#define PIDDSI_DOCPARTS          0x0000000D // VT_LPSTR | VT_VECTOR
#define PIDDSI_MANAGER           0x0000000E // VT_LPSTR
#define PIDDSI_COMPANY           0x0000000F // VT_LPSTR
#define PIDDSI_LINKSDIRTY        0x00000010 // VT_BOOL


//  FMTID_MediaFileSummaryInfo - Property IDs

#define PIDMSI_EDITOR                   0x00000002L  // VT_LPWSTR
#define PIDMSI_SUPPLIER                 0x00000003L  // VT_LPWSTR
#define PIDMSI_SOURCE                   0x00000004L  // VT_LPWSTR
#define PIDMSI_SEQUENCE_NO              0x00000005L  // VT_LPWSTR
#define PIDMSI_PROJECT                  0x00000006L  // VT_LPWSTR
#define PIDMSI_STATUS                   0x00000007L  // VT_UI4
#define PIDMSI_OWNER                    0x00000008L  // VT_LPWSTR
#define PIDMSI_RATING                   0x00000009L  // VT_LPWSTR
#define PIDMSI_PRODUCTION               0x0000000AL  // VT_FILETIME (UTC)
#define PIDMSI_COPYRIGHT                0x0000000BL  // VT_LPWSTR

//  PIDMSI_STATUS value definitions

enum PIDMSI_STATUS_VALUE
    {
        PIDMSI_STATUS_NORMAL	= 0,
        PIDMSI_STATUS_NEW	= ( PIDMSI_STATUS_NORMAL + 1 ) ,
        PIDMSI_STATUS_PRELIM	= ( PIDMSI_STATUS_NEW + 1 ) ,
        PIDMSI_STATUS_DRAFT	= ( PIDMSI_STATUS_PRELIM + 1 ) ,
        PIDMSI_STATUS_INPROGRESS	= ( PIDMSI_STATUS_DRAFT + 1 ) ,
        PIDMSI_STATUS_EDIT	= ( PIDMSI_STATUS_INPROGRESS + 1 ) ,
        PIDMSI_STATUS_REVIEW	= ( PIDMSI_STATUS_EDIT + 1 ) ,
        PIDMSI_STATUS_PROOF	= ( PIDMSI_STATUS_REVIEW + 1 ) ,
        PIDMSI_STATUS_FINAL	= ( PIDMSI_STATUS_PROOF + 1 ) ,
        PIDMSI_STATUS_OTHER	= 0x7fff
    } ;
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM) */
#pragma endregion
#pragma region Application Family or OneCore Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM)
_Check_return_ WINOLEAPI PropVariantCopy(
            _Out_ PROPVARIANT* pvarDest,
            _In_ const PROPVARIANT * pvarSrc);

WINOLEAPI PropVariantClear(_Inout_ PROPVARIANT* pvar);

WINOLEAPI FreePropVariantArray(
            _In_ ULONG cVariants,
            _Inout_updates_(cVariants) PROPVARIANT* rgvars);

#if defined(_MSC_EXTENSIONS)


#define _PROPVARIANTINIT_DEFINED_

#   ifdef __cplusplus

inline void PropVariantInit (_Out_ PROPVARIANT * pvar )
{
    memset ( pvar, 0, sizeof(PROPVARIANT) );
}

#   else

#   define PropVariantInit(pvar) memset ( (pvar), 0, sizeof(PROPVARIANT) )

#   endif


#endif /* _MSC_EXTENSIONS */


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) */
#pragma endregion

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#ifndef _SERIALIZEDPROPERTYVALUE_DEFINED_
#define _SERIALIZEDPROPERTYVALUE_DEFINED_
typedef struct tagSERIALIZEDPROPERTYVALUE
{
    DWORD	dwType;
    BYTE	rgb[1];
} SERIALIZEDPROPERTYVALUE;
#endif

EXTERN_C
_Success_(TRUE)  /* Raises status on failure */
SERIALIZEDPROPERTYVALUE* __stdcall
StgConvertVariantToProperty(
            _In_ const PROPVARIANT* pvar,
            _In_ USHORT CodePage,
            _Out_writes_bytes_opt_(*pcb) SERIALIZEDPROPERTYVALUE* pprop,
            _Inout_ ULONG* pcb,
            _In_ PROPID pid,
            _Reserved_ BOOLEAN fReserved,
            _Inout_opt_ ULONG* pcIndirect);

#ifdef __cplusplus
class PMemoryAllocator;

EXTERN_C
_Success_(TRUE)  /* Raises status on failure */
BOOLEAN __stdcall
StgConvertPropertyToVariant(
            _In_ const SERIALIZEDPROPERTYVALUE* pprop,
            _In_ USHORT CodePage,
            _Out_ PROPVARIANT* pvar,
            _In_ PMemoryAllocator* pma);
#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)    /* Nameless struct/union */
#pragma warning(default:4237)    /* keywords bool, true, false, etc.. */
#endif


extern RPC_IF_HANDLE __MIDL_itf_propidl_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_propidl_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     __RPC__in unsigned long *, __RPC__in BSTR * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     __RPC__in unsigned long *, __RPC__in LPSAFEARRAY * ); 

unsigned long             __RPC_USER  BSTR_UserSize64(     __RPC__in unsigned long *, unsigned long            , __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal64(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal64(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out BSTR * ); 
void                      __RPC_USER  BSTR_UserFree64(     __RPC__in unsigned long *, __RPC__in BSTR * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize64(     __RPC__in unsigned long *, unsigned long            , __RPC__in LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal64(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal64(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree64(     __RPC__in unsigned long *, __RPC__in LPSAFEARRAY * ); 

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumSTATPROPSTG_Next_Proxy( 
    IEnumSTATPROPSTG * This,
    /* [in] */ ULONG celt,
    /* [annotation][length_is][size_is][out] */ 
    _Out_writes_to_(celt, *pceltFetched)  STATPROPSTG *rgelt,
    /* [annotation][out] */ 
    _Out_opt_ _Deref_out_range_(0, celt)  ULONG *pceltFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumSTATPROPSTG_Next_Stub( 
    __RPC__in IEnumSTATPROPSTG * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ __RPC__out_ecount_part(celt, *pceltFetched) STATPROPSTG *rgelt,
    /* [out] */ __RPC__out ULONG *pceltFetched);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumSTATPROPSETSTG_Next_Proxy( 
    IEnumSTATPROPSETSTG * This,
    /* [in] */ ULONG celt,
    /* [annotation][length_is][size_is][out] */ 
    _Out_writes_to_(celt, *pceltFetched)  STATPROPSETSTG *rgelt,
    /* [annotation][out] */ 
    _Out_opt_ _Deref_out_range_(0, celt)  ULONG *pceltFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumSTATPROPSETSTG_Next_Stub( 
    __RPC__in IEnumSTATPROPSETSTG * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ __RPC__out_ecount_part(celt, *pceltFetched) STATPROPSETSTG *rgelt,
    /* [out] */ __RPC__out ULONG *pceltFetched);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


