// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0342 */
/* at Mon Feb 12 21:31:09 2001
 */
/* Compiler settings for atliface.idl:
	Oicf, W1, Zp8, env=Win32 (32b run)
	protocol : dce , ms_ext, c_ext
	error checks: allocation ref bounds_check enum stub_data 
	VC __declspec() decoration level: 
		 __declspec(uuid()), __declspec(selectany), __declspec(novtable)
		 DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __atliface_h__
#define __atliface_h__

#pragma once

/* Forward Declarations */ 

#ifndef __IRegistrarBase_FWD_DEFINED__
#define __IRegistrarBase_FWD_DEFINED__
typedef interface IRegistrarBase IRegistrarBase;
#endif 	/* __IRegistrarBase_FWD_DEFINED__ */


#ifndef __IRegistrar_FWD_DEFINED__
#define __IRegistrar_FWD_DEFINED__
typedef interface IRegistrar IRegistrar;
#endif 	/* __IRegistrar_FWD_DEFINED__ */


#ifndef __IDocHostUIHandlerDispatch_FWD_DEFINED__
#define __IDocHostUIHandlerDispatch_FWD_DEFINED__
typedef interface IDocHostUIHandlerDispatch IDocHostUIHandlerDispatch;
#endif 	/* __IDocHostUIHandlerDispatch_FWD_DEFINED__ */


#ifndef __IAxWinHostWindow_FWD_DEFINED__
#define __IAxWinHostWindow_FWD_DEFINED__
typedef interface IAxWinHostWindow IAxWinHostWindow;
#endif 	/* __IAxWinHostWindow_FWD_DEFINED__ */


#ifndef __IAxWinHostWindowLic_FWD_DEFINED__
#define __IAxWinHostWindowLic_FWD_DEFINED__
typedef interface IAxWinHostWindowLic IAxWinHostWindowLic;
#endif 	/* __IAxWinHostWindowLic_FWD_DEFINED__ */


#ifndef __IAxWinAmbientDispatch_FWD_DEFINED__
#define __IAxWinAmbientDispatch_FWD_DEFINED__
typedef interface IAxWinAmbientDispatch IAxWinAmbientDispatch;
#endif 	/* __IAxWinAmbientDispatch_FWD_DEFINED__ */


#ifndef __IAxWinAmbientDispatchEx_FWD_DEFINED__
#define __IAxWinAmbientDispatchEx_FWD_DEFINED__
typedef interface IAxWinAmbientDispatchEx IAxWinAmbientDispatchEx;
#endif 	/* __IAxWinAmbientDispatchEx_FWD_DEFINED__ */


#ifndef __IInternalConnection_FWD_DEFINED__
#define __IInternalConnection_FWD_DEFINED__
typedef interface IInternalConnection IInternalConnection;
#endif 	/* __IInternalConnection_FWD_DEFINED__ */


#ifndef __IAccessibleProxy_FWD_DEFINED__
#define __IAccessibleProxy_FWD_DEFINED__
typedef interface IAccessibleProxy IAccessibleProxy;
#endif 	/* __IAccessibleProxy_FWD_DEFINED__ */


#ifndef __IAccessibleServer_FWD_DEFINED__
#define __IAccessibleServer_FWD_DEFINED__
typedef interface IAccessibleServer IAccessibleServer;
#endif 	/* __IAccessibleServer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "oleacc.h"

#ifdef __cplusplus
extern "C"{
#endif 

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void * __RPC_USER MIDL_user_allocate(size_t size);
void __RPC_USER MIDL_user_free(_Pre_maybenull_ _Post_invalid_ void *);

/* interface __MIDL_itf_atliface_0000 */
/* [local] */ 

EXTERN_C const CLSID CLSID_Registrar;


extern RPC_IF_HANDLE __MIDL_itf_atliface_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_atliface_0000_v0_0_s_ifspec;

#ifndef __IRegistrarBase_INTERFACE_DEFINED__
#define __IRegistrarBase_INTERFACE_DEFINED__

/* interface IRegistrarBase */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRegistrarBase;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("e21f8a85-b05d-4243-8183-c7cb405588f7")
	IRegistrarBase : public IUnknown
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE AddReplacement( 
			/* [in] */ LPCOLESTR key,
			/* [in] */ LPCOLESTR item) = 0;

		virtual HRESULT STDMETHODCALLTYPE ClearReplacements( void) = 0;

	};

#else 	/* C style interface */

	typedef struct IRegistrarBaseVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IRegistrarBase * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IRegistrarBase * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IRegistrarBase * This);

		HRESULT ( STDMETHODCALLTYPE *AddReplacement )( 
			IRegistrarBase * This,
			/* [in] */ LPCOLESTR key,
			/* [in] */ LPCOLESTR item);

		HRESULT ( STDMETHODCALLTYPE *ClearReplacements )( 
			IRegistrarBase * This);

		END_INTERFACE
	} IRegistrarBaseVtbl;

	interface IRegistrarBase
	{
		CONST_VTBL struct IRegistrarBaseVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IRegistrarBase_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistrarBase_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IRegistrarBase_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IRegistrarBase_AddReplacement(This,key,item)	\
	(This)->lpVtbl -> AddReplacement(This,key,item)

#define IRegistrarBase_ClearReplacements(This)	\
	(This)->lpVtbl -> ClearReplacements(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRegistrarBase_AddReplacement_Proxy( 
	IRegistrarBase * This,
	/* [in] */ LPCOLESTR key,
	/* [in] */ LPCOLESTR item);


void __RPC_STUB IRegistrarBase_AddReplacement_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrarBase_ClearReplacements_Proxy( 
	IRegistrarBase * This);


void __RPC_STUB IRegistrarBase_ClearReplacements_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IRegistrarBase_INTERFACE_DEFINED__ */


#ifndef __IRegistrar_INTERFACE_DEFINED__
#define __IRegistrar_INTERFACE_DEFINED__

/* interface IRegistrar */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRegistrar;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("44EC053B-400F-11D0-9DCD-00A0C90391D3")
	IRegistrar : public IRegistrarBase
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE ResourceRegisterSz( 
			/* [in] */ _In_z_ LPCOLESTR resFileName,
			/* [in] */ _In_z_ LPCOLESTR szID,
			/* [in] */ _In_z_ LPCOLESTR szType) = 0;

		virtual HRESULT STDMETHODCALLTYPE ResourceUnregisterSz( 
			/* [in] */ _In_z_ LPCOLESTR resFileName,
			/* [in] */ _In_z_ LPCOLESTR szID,
			/* [in] */ _In_z_ LPCOLESTR szType) = 0;

		virtual HRESULT STDMETHODCALLTYPE FileRegister( 
			/* [in] */ _In_z_ LPCOLESTR fileName) = 0;

		virtual HRESULT STDMETHODCALLTYPE FileUnregister( 
			/* [in] */ _In_z_ LPCOLESTR fileName) = 0;

		virtual HRESULT STDMETHODCALLTYPE StringRegister( 
			/* [in] */ _In_z_ LPCOLESTR data) = 0;

		virtual HRESULT STDMETHODCALLTYPE StringUnregister( 
			/* [in] */ _In_z_ LPCOLESTR data) = 0;

		virtual HRESULT STDMETHODCALLTYPE ResourceRegister( 
			/* [in] */ _In_z_ LPCOLESTR resFileName,
			/* [in] */ _In_ UINT nID,
			/* [in] */ _In_z_ LPCOLESTR szType) = 0;

		virtual HRESULT STDMETHODCALLTYPE ResourceUnregister( 
			/* [in] */ _In_z_ LPCOLESTR resFileName,
			/* [in] */ _In_ UINT nID,
			/* [in] */ _In_z_ LPCOLESTR szType) = 0;

	};

#else 	/* C style interface */

	typedef struct IRegistrarVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IRegistrar * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IRegistrar * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IRegistrar * This);

		HRESULT ( STDMETHODCALLTYPE *AddReplacement )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR key,
			/* [in] */ LPCOLESTR item);

		HRESULT ( STDMETHODCALLTYPE *ClearReplacements )( 
			IRegistrar * This);

		HRESULT ( STDMETHODCALLTYPE *ResourceRegisterSz )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR resFileName,
			/* [in] */ LPCOLESTR szID,
			/* [in] */ LPCOLESTR szType);

		HRESULT ( STDMETHODCALLTYPE *ResourceUnregisterSz )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR resFileName,
			/* [in] */ LPCOLESTR szID,
			/* [in] */ LPCOLESTR szType);

		HRESULT ( STDMETHODCALLTYPE *FileRegister )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR fileName);

		HRESULT ( STDMETHODCALLTYPE *FileUnregister )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR fileName);

		HRESULT ( STDMETHODCALLTYPE *StringRegister )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR data);

		HRESULT ( STDMETHODCALLTYPE *StringUnregister )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR data);

		HRESULT ( STDMETHODCALLTYPE *ResourceRegister )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR resFileName,
			/* [in] */ UINT nID,
			/* [in] */ LPCOLESTR szType);

		HRESULT ( STDMETHODCALLTYPE *ResourceUnregister )( 
			IRegistrar * This,
			/* [in] */ LPCOLESTR resFileName,
			/* [in] */ UINT nID,
			/* [in] */ LPCOLESTR szType);

		END_INTERFACE
	} IRegistrarVtbl;

	interface IRegistrar
	{
		CONST_VTBL struct IRegistrarVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IRegistrar_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistrar_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IRegistrar_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IRegistrar_AddReplacement(This,key,item)	\
	(This)->lpVtbl -> AddReplacement(This,key,item)

#define IRegistrar_ClearReplacements(This)	\
	(This)->lpVtbl -> ClearReplacements(This)


#define IRegistrar_ResourceRegisterSz(This,resFileName,szID,szType)	\
	(This)->lpVtbl -> ResourceRegisterSz(This,resFileName,szID,szType)

#define IRegistrar_ResourceUnregisterSz(This,resFileName,szID,szType)	\
	(This)->lpVtbl -> ResourceUnregisterSz(This,resFileName,szID,szType)

#define IRegistrar_FileRegister(This,fileName)	\
	(This)->lpVtbl -> FileRegister(This,fileName)

#define IRegistrar_FileUnregister(This,fileName)	\
	(This)->lpVtbl -> FileUnregister(This,fileName)

#define IRegistrar_StringRegister(This,data)	\
	(This)->lpVtbl -> StringRegister(This,data)

#define IRegistrar_StringUnregister(This,data)	\
	(This)->lpVtbl -> StringUnregister(This,data)

#define IRegistrar_ResourceRegister(This,resFileName,nID,szType)	\
	(This)->lpVtbl -> ResourceRegister(This,resFileName,nID,szType)

#define IRegistrar_ResourceUnregister(This,resFileName,nID,szType)	\
	(This)->lpVtbl -> ResourceUnregister(This,resFileName,nID,szType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegisterSz_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR resFileName,
	/* [in] */ LPCOLESTR szID,
	/* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegisterSz_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregisterSz_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR resFileName,
	/* [in] */ LPCOLESTR szID,
	/* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregisterSz_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_FileRegister_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileRegister_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_FileUnregister_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileUnregister_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_StringRegister_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringRegister_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_StringUnregister_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringUnregister_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegister_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR resFileName,
	/* [in] */ UINT nID,
	/* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegister_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregister_Proxy( 
	IRegistrar * This,
	/* [in] */ LPCOLESTR resFileName,
	/* [in] */ UINT nID,
	/* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregister_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IRegistrar_INTERFACE_DEFINED__ */


#ifndef __IDocHostUIHandlerDispatch_INTERFACE_DEFINED__
#define __IDocHostUIHandlerDispatch_INTERFACE_DEFINED__

/* interface IDocHostUIHandlerDispatch */
/* [object][unique][helpstring][uuid][local] */ 

typedef 
enum tagDOCHOSTUIDBLCLKDispatch
	{	docHostUIDblClkDEFAULT	= 0,
	docHostUIDblClkSHOWPROPERTIES	= 1,
	docHostUIDblClkSHOWCODE	= 2
	} 	DOCHOSTUIDBLCLKDispatch;

typedef 
enum tagDocHostUIFlagDispatch
	{	docHostUIFlagDIALOG	= 1,
	docHostUIFlagDISABLE_HELP_MENU	= 2,
	docHostUIFlagNO3DBORDER	= 4,
	docHostUIFlagSCROLL_NO	= 8,
	docHostUIFlagDISABLE_SCRIPT_INACTIVE	= 16,
	docHostUIFlagOPENNEWWIN	= 32,
	docHostUIFlagDISABLE_OFFSCREEN	= 64,
	docHostUIFlagFLAT_SCROLLBAR	= 128,
	docHostUIFlagDIV_BLOCKDEFAULT	= 256,
	docHostUIFlagACTIVATE_CLIENTHIT_ONLY	= 512
	} 	DocHostUIFlagDispatch;


EXTERN_C const IID IID_IDocHostUIHandlerDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("425B5AF0-65F1-11d1-9611-0000F81E0D0D")
	IDocHostUIHandlerDispatch : public IDispatch
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE ShowContextMenu( 
			/* [in] */ DWORD dwID,
			/* [in] */ DWORD x,
			/* [in] */ DWORD y,
			/* [in] */ IUnknown *pcmdtReserved,
			/* [in] */ IDispatch *pdispReserved,
			/* [retval][out] */ HRESULT *dwRetVal) = 0;

		virtual HRESULT STDMETHODCALLTYPE GetHostInfo( 
			/* [out][in] */ DWORD *pdwFlags,
			/* [out][in] */ DWORD *pdwDoubleClick) = 0;

		virtual HRESULT STDMETHODCALLTYPE ShowUI( 
			/* [in] */ DWORD dwID,
			/* [in] */ IUnknown *pActiveObject,
			/* [in] */ IUnknown *pCommandTarget,
			/* [in] */ IUnknown *pFrame,
			/* [in] */ IUnknown *pDoc,
			/* [retval][out] */ HRESULT *dwRetVal) = 0;

		virtual HRESULT STDMETHODCALLTYPE HideUI( void) = 0;

		virtual HRESULT STDMETHODCALLTYPE UpdateUI( void) = 0;

		virtual HRESULT STDMETHODCALLTYPE EnableModeless( 
			/* [in] */ VARIANT_BOOL fEnable) = 0;

		virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate( 
			/* [in] */ VARIANT_BOOL fActivate) = 0;

		virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate( 
			/* [in] */ VARIANT_BOOL fActivate) = 0;

		virtual HRESULT STDMETHODCALLTYPE ResizeBorder( 
			/* [in] */ long left,
			/* [in] */ long top,
			/* [in] */ long right,
			/* [in] */ long bottom,
			/* [in] */ IUnknown *pUIWindow,
			/* [in] */ VARIANT_BOOL fFrameWindow) = 0;

		virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
			/* [in] */ DWORD_PTR hWnd,
			/* [in] */ DWORD nMessage,
			/* [in] */ DWORD_PTR wParam,
			/* [in] */ DWORD_PTR lParam,
			/* [in] */ BSTR bstrGuidCmdGroup,
			/* [in] */ DWORD nCmdID,
			/* [retval][out] */ HRESULT *dwRetVal) = 0;

		virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath( 
			/* [out] */ BSTR *pbstrKey,
			/* [in] */ DWORD dw) = 0;

		virtual HRESULT STDMETHODCALLTYPE GetDropTarget( 
			/* [in] */ IUnknown *pDropTarget,
			/* [out] */ IUnknown **ppDropTarget) = 0;

		virtual HRESULT STDMETHODCALLTYPE GetExternal( 
			/* [out] */ IDispatch **ppDispatch) = 0;

		virtual HRESULT STDMETHODCALLTYPE TranslateUrl( 
			/* [in] */ DWORD dwTranslate,
			/* [in] */ BSTR bstrURLIn,
			/* [out] */ BSTR *pbstrURLOut) = 0;

		virtual HRESULT STDMETHODCALLTYPE FilterDataObject( 
			/* [in] */ IUnknown *pDO,
			/* [out] */ IUnknown **ppDORet) = 0;

	};

#else 	/* C style interface */

	typedef struct IDocHostUIHandlerDispatchVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IDocHostUIHandlerDispatch * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IDocHostUIHandlerDispatch * This);

		HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
			IDocHostUIHandlerDispatch * This,
			/* [out] */ UINT *pctinfo);

		HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ UINT iTInfo,
			/* [in] */ LCID lcid,
			/* [out] */ ITypeInfo **ppTInfo);

		HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ REFIID riid,
			/* [size_is][in] */ _In_reads_(cNames) LPOLESTR *rgszNames,
			/* [in] */ UINT cNames,
			/* [in] */ LCID lcid,
			/* [size_is][out] */ DISPID *rgDispId);

		/* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ DISPID dispIdMember,
			/* [in] */ REFIID riid,
			/* [in] */ LCID lcid,
			/* [in] */ WORD wFlags,
			/* [out][in] */ DISPPARAMS *pDispParams,
			/* [out] */ VARIANT *pVarResult,
			/* [out] */ EXCEPINFO *pExcepInfo,
			/* [out] */ UINT *puArgErr);

		HRESULT ( STDMETHODCALLTYPE *ShowContextMenu )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ DWORD dwID,
			/* [in] */ DWORD x,
			/* [in] */ DWORD y,
			/* [in] */ IUnknown *pcmdtReserved,
			/* [in] */ IDispatch *pdispReserved,
			/* [retval][out] */ HRESULT *dwRetVal);

		HRESULT ( STDMETHODCALLTYPE *GetHostInfo )( 
			IDocHostUIHandlerDispatch * This,
			/* [out][in] */ DWORD *pdwFlags,
			/* [out][in] */ DWORD *pdwDoubleClick);

		HRESULT ( STDMETHODCALLTYPE *ShowUI )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ DWORD dwID,
			/* [in] */ IUnknown *pActiveObject,
			/* [in] */ IUnknown *pCommandTarget,
			/* [in] */ IUnknown *pFrame,
			/* [in] */ IUnknown *pDoc,
			/* [retval][out] */ HRESULT *dwRetVal);

		HRESULT ( STDMETHODCALLTYPE *HideUI )( 
			IDocHostUIHandlerDispatch * This);

		HRESULT ( STDMETHODCALLTYPE *UpdateUI )( 
			IDocHostUIHandlerDispatch * This);

		HRESULT ( STDMETHODCALLTYPE *EnableModeless )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ VARIANT_BOOL fEnable);

		HRESULT ( STDMETHODCALLTYPE *OnDocWindowActivate )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ VARIANT_BOOL fActivate);

		HRESULT ( STDMETHODCALLTYPE *OnFrameWindowActivate )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ VARIANT_BOOL fActivate);

		HRESULT ( STDMETHODCALLTYPE *ResizeBorder )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ long left,
			/* [in] */ long top,
			/* [in] */ long right,
			/* [in] */ long bottom,
			/* [in] */ IUnknown *pUIWindow,
			/* [in] */ VARIANT_BOOL fFrameWindow);

		HRESULT ( STDMETHODCALLTYPE *TranslateAccelerator )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ DWORD_PTR hWnd,
			/* [in] */ DWORD nMessage,
			/* [in] */ DWORD_PTR wParam,
			/* [in] */ DWORD_PTR lParam,
			/* [in] */ BSTR bstrGuidCmdGroup,
			/* [in] */ DWORD nCmdID,
			/* [retval][out] */ HRESULT *dwRetVal);

		HRESULT ( STDMETHODCALLTYPE *GetOptionKeyPath )( 
			IDocHostUIHandlerDispatch * This,
			/* [out] */ BSTR *pbstrKey,
			/* [in] */ DWORD dw);

		HRESULT ( STDMETHODCALLTYPE *GetDropTarget )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ IUnknown *pDropTarget,
			/* [out] */ IUnknown **ppDropTarget);

		HRESULT ( STDMETHODCALLTYPE *GetExternal )( 
			IDocHostUIHandlerDispatch * This,
			/* [out] */ IDispatch **ppDispatch);

		HRESULT ( STDMETHODCALLTYPE *TranslateUrl )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ DWORD dwTranslate,
			/* [in] */ BSTR bstrURLIn,
			/* [out] */ BSTR *pbstrURLOut);

		HRESULT ( STDMETHODCALLTYPE *FilterDataObject )( 
			IDocHostUIHandlerDispatch * This,
			/* [in] */ IUnknown *pDO,
			/* [out] */ IUnknown **ppDORet);

		END_INTERFACE
	} IDocHostUIHandlerDispatchVtbl;

	interface IDocHostUIHandlerDispatch
	{
		CONST_VTBL struct IDocHostUIHandlerDispatchVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IDocHostUIHandlerDispatch_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocHostUIHandlerDispatch_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IDocHostUIHandlerDispatch_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IDocHostUIHandlerDispatch_GetTypeInfoCount(This,pctinfo)	\
	(This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDocHostUIHandlerDispatch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
	(This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDocHostUIHandlerDispatch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
	(This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDocHostUIHandlerDispatch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
	(This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDocHostUIHandlerDispatch_ShowContextMenu(This,dwID,x,y,pcmdtReserved,pdispReserved,dwRetVal)	\
	(This)->lpVtbl -> ShowContextMenu(This,dwID,x,y,pcmdtReserved,pdispReserved,dwRetVal)

#define IDocHostUIHandlerDispatch_GetHostInfo(This,pdwFlags,pdwDoubleClick)	\
	(This)->lpVtbl -> GetHostInfo(This,pdwFlags,pdwDoubleClick)

#define IDocHostUIHandlerDispatch_ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc,dwRetVal)	\
	(This)->lpVtbl -> ShowUI(This,dwID,pActiveObject,pCommandTarget,pFrame,pDoc,dwRetVal)

#define IDocHostUIHandlerDispatch_HideUI(This)	\
	(This)->lpVtbl -> HideUI(This)

#define IDocHostUIHandlerDispatch_UpdateUI(This)	\
	(This)->lpVtbl -> UpdateUI(This)

#define IDocHostUIHandlerDispatch_EnableModeless(This,fEnable)	\
	(This)->lpVtbl -> EnableModeless(This,fEnable)

#define IDocHostUIHandlerDispatch_OnDocWindowActivate(This,fActivate)	\
	(This)->lpVtbl -> OnDocWindowActivate(This,fActivate)

#define IDocHostUIHandlerDispatch_OnFrameWindowActivate(This,fActivate)	\
	(This)->lpVtbl -> OnFrameWindowActivate(This,fActivate)

#define IDocHostUIHandlerDispatch_ResizeBorder(This,left,top,right,bottom,pUIWindow,fFrameWindow)	\
	(This)->lpVtbl -> ResizeBorder(This,left,top,right,bottom,pUIWindow,fFrameWindow)

#define IDocHostUIHandlerDispatch_TranslateAccelerator(This,hWnd,nMessage,wParam,lParam,bstrGuidCmdGroup,nCmdID,dwRetVal)	\
	(This)->lpVtbl -> TranslateAccelerator(This,hWnd,nMessage,wParam,lParam,bstrGuidCmdGroup,nCmdID,dwRetVal)

#define IDocHostUIHandlerDispatch_GetOptionKeyPath(This,pbstrKey,dw)	\
	(This)->lpVtbl -> GetOptionKeyPath(This,pbstrKey,dw)

#define IDocHostUIHandlerDispatch_GetDropTarget(This,pDropTarget,ppDropTarget)	\
	(This)->lpVtbl -> GetDropTarget(This,pDropTarget,ppDropTarget)

#define IDocHostUIHandlerDispatch_GetExternal(This,ppDispatch)	\
	(This)->lpVtbl -> GetExternal(This,ppDispatch)

#define IDocHostUIHandlerDispatch_TranslateUrl(This,dwTranslate,bstrURLIn,pbstrURLOut)	\
	(This)->lpVtbl -> TranslateUrl(This,dwTranslate,bstrURLIn,pbstrURLOut)

#define IDocHostUIHandlerDispatch_FilterDataObject(This,pDO,ppDORet)	\
	(This)->lpVtbl -> FilterDataObject(This,pDO,ppDORet)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_ShowContextMenu_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ DWORD dwID,
	/* [in] */ DWORD x,
	/* [in] */ DWORD y,
	/* [in] */ IUnknown *pcmdtReserved,
	/* [in] */ IDispatch *pdispReserved,
	/* [retval][out] */ HRESULT *dwRetVal);


void __RPC_STUB IDocHostUIHandlerDispatch_ShowContextMenu_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetHostInfo_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [out][in] */ DWORD *pdwFlags,
	/* [out][in] */ DWORD *pdwDoubleClick);


void __RPC_STUB IDocHostUIHandlerDispatch_GetHostInfo_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_ShowUI_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ DWORD dwID,
	/* [in] */ IUnknown *pActiveObject,
	/* [in] */ IUnknown *pCommandTarget,
	/* [in] */ IUnknown *pFrame,
	/* [in] */ IUnknown *pDoc,
	/* [retval][out] */ HRESULT *dwRetVal);


void __RPC_STUB IDocHostUIHandlerDispatch_ShowUI_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_HideUI_Proxy( 
	IDocHostUIHandlerDispatch * This);


void __RPC_STUB IDocHostUIHandlerDispatch_HideUI_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_UpdateUI_Proxy( 
	IDocHostUIHandlerDispatch * This);


void __RPC_STUB IDocHostUIHandlerDispatch_UpdateUI_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_EnableModeless_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB IDocHostUIHandlerDispatch_EnableModeless_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_OnDocWindowActivate_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ VARIANT_BOOL fActivate);


void __RPC_STUB IDocHostUIHandlerDispatch_OnDocWindowActivate_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_OnFrameWindowActivate_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ VARIANT_BOOL fActivate);


void __RPC_STUB IDocHostUIHandlerDispatch_OnFrameWindowActivate_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_ResizeBorder_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ long left,
	/* [in] */ long top,
	/* [in] */ long right,
	/* [in] */ long bottom,
	/* [in] */ IUnknown *pUIWindow,
	/* [in] */ VARIANT_BOOL fFrameWindow);


void __RPC_STUB IDocHostUIHandlerDispatch_ResizeBorder_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_TranslateAccelerator_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ DWORD_PTR hWnd,
	/* [in] */ DWORD nMessage,
	/* [in] */ DWORD_PTR wParam,
	/* [in] */ DWORD_PTR lParam,
	/* [in] */ BSTR bstrGuidCmdGroup,
	/* [in] */ DWORD nCmdID,
	/* [retval][out] */ HRESULT *dwRetVal);


void __RPC_STUB IDocHostUIHandlerDispatch_TranslateAccelerator_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetOptionKeyPath_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [out] */ BSTR *pbstrKey,
	/* [in] */ DWORD dw);


void __RPC_STUB IDocHostUIHandlerDispatch_GetOptionKeyPath_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetDropTarget_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ IUnknown *pDropTarget,
	/* [out] */ IUnknown **ppDropTarget);


void __RPC_STUB IDocHostUIHandlerDispatch_GetDropTarget_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_GetExternal_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [out] */ IDispatch **ppDispatch);


void __RPC_STUB IDocHostUIHandlerDispatch_GetExternal_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_TranslateUrl_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ DWORD dwTranslate,
	/* [in] */ BSTR bstrURLIn,
	/* [out] */ BSTR *pbstrURLOut);


void __RPC_STUB IDocHostUIHandlerDispatch_TranslateUrl_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocHostUIHandlerDispatch_FilterDataObject_Proxy( 
	IDocHostUIHandlerDispatch * This,
	/* [in] */ IUnknown *pDO,
	/* [out] */ IUnknown **ppDORet);


void __RPC_STUB IDocHostUIHandlerDispatch_FilterDataObject_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IDocHostUIHandlerDispatch_INTERFACE_DEFINED__ */


#ifndef __IAxWinHostWindow_INTERFACE_DEFINED__
#define __IAxWinHostWindow_INTERFACE_DEFINED__

/* interface IAxWinHostWindow */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IAxWinHostWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("B6EA2050-048A-11d1-82B9-00C04FB9942E")
	IAxWinHostWindow : public IUnknown
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE CreateControl( 
			_In_z_ LPCOLESTR lpTricsData,
			_In_ HWND hWnd,
			_Inout_opt_ IStream *pStream) = 0;

		virtual HRESULT STDMETHODCALLTYPE CreateControlEx( 
			_In_z_ LPCOLESTR lpszTricsData,
			_In_ HWND hWnd,
			_Inout_opt_ IStream* pStream,
			_Outptr_ IUnknown** ppUnk,
			_In_ REFIID iidAdvise,
			_Inout_ IUnknown* punkSink) = 0;

		virtual HRESULT STDMETHODCALLTYPE AttachControl( 
			_Inout_ IUnknown* pUnkControl,
			_In_ HWND hWnd) = 0;

		virtual HRESULT STDMETHODCALLTYPE QueryControl( 
			_In_ REFIID riid,
			_Outptr_ void** ppvObject) = 0;

		virtual HRESULT STDMETHODCALLTYPE SetExternalDispatch( 
			_In_opt_ IDispatch *pDisp) = 0;

		virtual HRESULT STDMETHODCALLTYPE SetExternalUIHandler( 
			_In_opt_ IDocHostUIHandlerDispatch *pDisp) = 0;

	};

#else 	/* C style interface */

	typedef struct IAxWinHostWindowVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IAxWinHostWindow * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IAxWinHostWindow * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IAxWinHostWindow * This);

		HRESULT ( STDMETHODCALLTYPE *CreateControl )( 
			IAxWinHostWindow * This,
			/* [in] */ LPCOLESTR lpTricsData,
			/* [in] */ HWND hWnd,
			/* [in] */ IStream *pStream);

		HRESULT ( STDMETHODCALLTYPE *CreateControlEx )( 
			IAxWinHostWindow * This,
			/* [in] */ LPCOLESTR lpTricsData,
			/* [in] */ HWND hWnd,
			/* [in] */ IStream *pStream,
			/* [out] */ IUnknown **ppUnk,
			/* [in] */ REFIID riidAdvise,
			/* [in] */ IUnknown *punkAdvise);

		HRESULT ( STDMETHODCALLTYPE *AttachControl )( 
			IAxWinHostWindow * This,
			/* [in] */ IUnknown *pUnkControl,
			/* [in] */ HWND hWnd);

		HRESULT ( STDMETHODCALLTYPE *QueryControl )( 
			IAxWinHostWindow * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		HRESULT ( STDMETHODCALLTYPE *SetExternalDispatch )( 
			IAxWinHostWindow * This,
			/* [in] */ IDispatch *pDisp);

		HRESULT ( STDMETHODCALLTYPE *SetExternalUIHandler )( 
			IAxWinHostWindow * This,
			/* [in] */ IDocHostUIHandlerDispatch *pDisp);

		END_INTERFACE
	} IAxWinHostWindowVtbl;

	interface IAxWinHostWindow
	{
		CONST_VTBL struct IAxWinHostWindowVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IAxWinHostWindow_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAxWinHostWindow_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IAxWinHostWindow_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IAxWinHostWindow_CreateControl(This,lpTricsData,hWnd,pStream)	\
	(This)->lpVtbl -> CreateControl(This,lpTricsData,hWnd,pStream)

#define IAxWinHostWindow_CreateControlEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise)	\
	(This)->lpVtbl -> CreateControlEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise)

#define IAxWinHostWindow_AttachControl(This,pUnkControl,hWnd)	\
	(This)->lpVtbl -> AttachControl(This,pUnkControl,hWnd)

#define IAxWinHostWindow_QueryControl(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryControl(This,riid,ppvObject)

#define IAxWinHostWindow_SetExternalDispatch(This,pDisp)	\
	(This)->lpVtbl -> SetExternalDispatch(This,pDisp)

#define IAxWinHostWindow_SetExternalUIHandler(This,pDisp)	\
	(This)->lpVtbl -> SetExternalUIHandler(This,pDisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAxWinHostWindow_CreateControl_Proxy( 
	IAxWinHostWindow * This,
	/* [in] */ LPCOLESTR lpTricsData,
	/* [in] */ HWND hWnd,
	/* [in] */ IStream *pStream);


void __RPC_STUB IAxWinHostWindow_CreateControl_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_CreateControlEx_Proxy( 
	IAxWinHostWindow * This,
	/* [in] */ LPCOLESTR lpTricsData,
	/* [in] */ HWND hWnd,
	/* [in] */ IStream *pStream,
	/* [out] */ IUnknown **ppUnk,
	/* [in] */ REFIID riidAdvise,
	/* [in] */ IUnknown *punkAdvise);


void __RPC_STUB IAxWinHostWindow_CreateControlEx_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_AttachControl_Proxy( 
	IAxWinHostWindow * This,
	/* [in] */ IUnknown *pUnkControl,
	/* [in] */ HWND hWnd);


void __RPC_STUB IAxWinHostWindow_AttachControl_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_QueryControl_Proxy( 
	IAxWinHostWindow * This,
	/* [in] */ REFIID riid,
	/* [iid_is][out] */ void **ppvObject);


void __RPC_STUB IAxWinHostWindow_QueryControl_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_SetExternalDispatch_Proxy( 
	IAxWinHostWindow * This,
	/* [in] */ IDispatch *pDisp);


void __RPC_STUB IAxWinHostWindow_SetExternalDispatch_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindow_SetExternalUIHandler_Proxy( 
	IAxWinHostWindow * This,
	/* [in] */ IDocHostUIHandlerDispatch *pDisp);


void __RPC_STUB IAxWinHostWindow_SetExternalUIHandler_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IAxWinHostWindow_INTERFACE_DEFINED__ */


#ifndef __IAxWinHostWindowLic_INTERFACE_DEFINED__
#define __IAxWinHostWindowLic_INTERFACE_DEFINED__

/* interface IAxWinHostWindowLic */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IAxWinHostWindowLic;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("3935BDA8-4ED9-495c-8650-E01FC1E38A4B")
	IAxWinHostWindowLic : public IAxWinHostWindow
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE CreateControlLic( 
			_In_z_ LPCOLESTR lpTricsData,
			_In_ HWND hWnd,
			_Inout_opt_ IStream* pStream,
			_In_opt_z_ BSTR bstrLic) = 0;

		virtual HRESULT STDMETHODCALLTYPE CreateControlLicEx( 
			_In_z_ LPCOLESTR lpszTricsData,
			_In_ HWND hWnd,
			_Inout_opt_ IStream* pStream,
			_Outptr_ IUnknown** ppUnk,
			_In_ REFIID iidAdvise,
			_Inout_opt_ IUnknown* punkSink,
			_In_opt_z_ BSTR bstrLic) = 0;

	};

#else 	/* C style interface */

	typedef struct IAxWinHostWindowLicVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IAxWinHostWindowLic * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IAxWinHostWindowLic * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IAxWinHostWindowLic * This);

		HRESULT ( STDMETHODCALLTYPE *CreateControl )( 
			IAxWinHostWindowLic * This,
			/* [in] */ LPCOLESTR lpTricsData,
			/* [in] */ HWND hWnd,
			/* [in] */ IStream *pStream);

		HRESULT ( STDMETHODCALLTYPE *CreateControlEx )( 
			IAxWinHostWindowLic * This,
			/* [in] */ LPCOLESTR lpTricsData,
			/* [in] */ HWND hWnd,
			/* [in] */ IStream *pStream,
			/* [out] */ IUnknown **ppUnk,
			/* [in] */ REFIID riidAdvise,
			/* [in] */ IUnknown *punkAdvise);

		HRESULT ( STDMETHODCALLTYPE *AttachControl )( 
			IAxWinHostWindowLic * This,
			/* [in] */ IUnknown *pUnkControl,
			/* [in] */ HWND hWnd);

		HRESULT ( STDMETHODCALLTYPE *QueryControl )( 
			IAxWinHostWindowLic * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		HRESULT ( STDMETHODCALLTYPE *SetExternalDispatch )( 
			IAxWinHostWindowLic * This,
			/* [in] */ IDispatch *pDisp);

		HRESULT ( STDMETHODCALLTYPE *SetExternalUIHandler )( 
			IAxWinHostWindowLic * This,
			/* [in] */ IDocHostUIHandlerDispatch *pDisp);

		HRESULT ( STDMETHODCALLTYPE *CreateControlLic )( 
			IAxWinHostWindowLic * This,
			/* [in] */ LPCOLESTR lpTricsData,
			/* [in] */ HWND hWnd,
			/* [in] */ IStream *pStream,
			/* [in] */ BSTR bstrLic);

		HRESULT ( STDMETHODCALLTYPE *CreateControlLicEx )( 
			IAxWinHostWindowLic * This,
			/* [in] */ LPCOLESTR lpTricsData,
			/* [in] */ HWND hWnd,
			/* [in] */ IStream *pStream,
			/* [out] */ IUnknown **ppUnk,
			/* [in] */ REFIID riidAdvise,
			/* [in] */ IUnknown *punkAdvise,
			/* [in] */ BSTR bstrLic);

		END_INTERFACE
	} IAxWinHostWindowLicVtbl;

	interface IAxWinHostWindowLic
	{
		CONST_VTBL struct IAxWinHostWindowLicVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IAxWinHostWindowLic_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAxWinHostWindowLic_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IAxWinHostWindowLic_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IAxWinHostWindowLic_CreateControl(This,lpTricsData,hWnd,pStream)	\
	(This)->lpVtbl -> CreateControl(This,lpTricsData,hWnd,pStream)

#define IAxWinHostWindowLic_CreateControlEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise)	\
	(This)->lpVtbl -> CreateControlEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise)

#define IAxWinHostWindowLic_AttachControl(This,pUnkControl,hWnd)	\
	(This)->lpVtbl -> AttachControl(This,pUnkControl,hWnd)

#define IAxWinHostWindowLic_QueryControl(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryControl(This,riid,ppvObject)

#define IAxWinHostWindowLic_SetExternalDispatch(This,pDisp)	\
	(This)->lpVtbl -> SetExternalDispatch(This,pDisp)

#define IAxWinHostWindowLic_SetExternalUIHandler(This,pDisp)	\
	(This)->lpVtbl -> SetExternalUIHandler(This,pDisp)


#define IAxWinHostWindowLic_CreateControlLic(This,lpTricsData,hWnd,pStream,bstrLic)	\
	(This)->lpVtbl -> CreateControlLic(This,lpTricsData,hWnd,pStream,bstrLic)

#define IAxWinHostWindowLic_CreateControlLicEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise,bstrLic)	\
	(This)->lpVtbl -> CreateControlLicEx(This,lpTricsData,hWnd,pStream,ppUnk,riidAdvise,punkAdvise,bstrLic)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAxWinHostWindowLic_CreateControlLic_Proxy( 
	IAxWinHostWindowLic * This,
	/* [in] */ LPCOLESTR lpTricsData,
	/* [in] */ HWND hWnd,
	/* [in] */ IStream *pStream,
	/* [in] */ BSTR bstrLic);


void __RPC_STUB IAxWinHostWindowLic_CreateControlLic_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAxWinHostWindowLic_CreateControlLicEx_Proxy( 
	IAxWinHostWindowLic * This,
	/* [in] */ LPCOLESTR lpTricsData,
	/* [in] */ HWND hWnd,
	/* [in] */ IStream *pStream,
	/* [out] */ IUnknown **ppUnk,
	/* [in] */ REFIID riidAdvise,
	/* [in] */ IUnknown *punkAdvise,
	/* [in] */ BSTR bstrLic);


void __RPC_STUB IAxWinHostWindowLic_CreateControlLicEx_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IAxWinHostWindowLic_INTERFACE_DEFINED__ */


#ifndef __IAxWinAmbientDispatch_INTERFACE_DEFINED__
#define __IAxWinAmbientDispatch_INTERFACE_DEFINED__

/* interface IAxWinAmbientDispatch */
/* [unique][helpstring][uuid][dual][object] */ 


EXTERN_C const IID IID_IAxWinAmbientDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("B6EA2051-048A-11d1-82B9-00C04FB9942E")
	IAxWinAmbientDispatch : public IDispatch
	{
	public:
		virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowWindowlessActivation( 
			_In_ VARIANT_BOOL bCanWindowlessActivate) = 0;

		virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowWindowlessActivation( 
			_Out_ VARIANT_BOOL *pbCanWindowlessActivate) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
			_In_ OLE_COLOR clrBackground) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
			_Out_ OLE_COLOR *pclrBackground) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ForeColor( 
			_In_ OLE_COLOR clrForeground) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ForeColor( 
			_Out_ OLE_COLOR *pclrForeground) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LocaleID( 
			_In_ LCID lcidLocaleID) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LocaleID( 
			_Out_ LCID *plcidLocaleID) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_UserMode( 
			_In_ VARIANT_BOOL bUserMode) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_UserMode( 
			_Out_ VARIANT_BOOL *pbUserMode) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DisplayAsDefault( 
			_In_ VARIANT_BOOL bDisplayAsDefault) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayAsDefault( 
			_Out_ VARIANT_BOOL *pbDisplayAsDefault) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Font( 
			_In_ IFontDisp *pFont) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Font( 
			_Out_ IFontDisp **pFont) = 0;

		virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MessageReflect( 
			_In_ VARIANT_BOOL bMsgReflect) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MessageReflect( 
			_Out_ VARIANT_BOOL *pbMsgReflect) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ShowGrabHandles( 
			_Out_ VARIANT_BOOL *pbShowGrabHandles) = 0;

		virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ShowHatching( 
			_Out_ VARIANT_BOOL *pbShowHatching) = 0;

		virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DocHostFlags( 
			_In_ DWORD dwDocHostFlags) = 0;

		virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DocHostFlags( 
			_Out_ DWORD *pdwDocHostFlags) = 0;

		virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DocHostDoubleClickFlags( 
			_In_ DWORD dwDocHostDoubleClickFlags) = 0;

		virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DocHostDoubleClickFlags( 
			_Out_ DWORD *pdwDocHostDoubleClickFlags) = 0;

		virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowContextMenu( 
			_In_ VARIANT_BOOL bAllowContextMenu) = 0;

		virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowContextMenu( 
			_Out_ VARIANT_BOOL *pbAllowContextMenu) = 0;

		virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AllowShowUI( 
			_In_ VARIANT_BOOL bAllowShowUI) = 0;

		virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AllowShowUI( 
			_Out_ VARIANT_BOOL *pbAllowShowUI) = 0;

		virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_OptionKeyPath( 
			_In_ BSTR bstrOptionKeyPath) = 0;

		virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_OptionKeyPath( 
			_Out_ BSTR *pbstrOptionKeyPath) = 0;

	};

#else 	/* C style interface */

	typedef struct IAxWinAmbientDispatchVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IAxWinAmbientDispatch * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IAxWinAmbientDispatch * This);

		HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
			IAxWinAmbientDispatch * This,
			/* [out] */ UINT *pctinfo);

		HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ UINT iTInfo,
			/* [in] */ LCID lcid,
			/* [out] */ ITypeInfo **ppTInfo);

		HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ REFIID riid,
			/* [size_is][in] */ _In_reads_(cNames) LPOLESTR *rgszNames,
			/* [in] */ UINT cNames,
			/* [in] */ LCID lcid,
			/* [size_is][out] */ DISPID *rgDispId);

		/* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ DISPID dispIdMember,
			/* [in] */ REFIID riid,
			/* [in] */ LCID lcid,
			/* [in] */ WORD wFlags,
			/* [out][in] */ DISPPARAMS *pDispParams,
			/* [out] */ VARIANT *pVarResult,
			/* [out] */ EXCEPINFO *pExcepInfo,
			/* [out] */ UINT *puArgErr);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowWindowlessActivation )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ VARIANT_BOOL bCanWindowlessActivate);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowWindowlessActivation )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbCanWindowlessActivate);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BackColor )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ OLE_COLOR clrBackground);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BackColor )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ OLE_COLOR *pclrBackground);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ForeColor )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ OLE_COLOR clrForeground);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ForeColor )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ OLE_COLOR *pclrForeground);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocaleID )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ LCID lcidLocaleID);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocaleID )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ LCID *plcidLocaleID);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UserMode )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ VARIANT_BOOL bUserMode);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserMode )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbUserMode);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayAsDefault )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ VARIANT_BOOL bDisplayAsDefault);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayAsDefault )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbDisplayAsDefault);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Font )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ IFontDisp *pFont);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Font )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ IFontDisp **pFont);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MessageReflect )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ VARIANT_BOOL bMsgReflect);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MessageReflect )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbMsgReflect);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShowGrabHandles )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbShowGrabHandles);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShowHatching )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbShowHatching);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DocHostFlags )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ DWORD dwDocHostFlags);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocHostFlags )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ DWORD *pdwDocHostFlags);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DocHostDoubleClickFlags )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ DWORD dwDocHostDoubleClickFlags);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocHostDoubleClickFlags )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ DWORD *pdwDocHostDoubleClickFlags);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowContextMenu )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ VARIANT_BOOL bAllowContextMenu);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowContextMenu )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbAllowContextMenu);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowShowUI )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ VARIANT_BOOL bAllowShowUI);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowShowUI )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ VARIANT_BOOL *pbAllowShowUI);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OptionKeyPath )( 
			IAxWinAmbientDispatch * This,
			/* [in] */ BSTR bstrOptionKeyPath);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OptionKeyPath )( 
			IAxWinAmbientDispatch * This,
			/* [retval][out] */ BSTR *pbstrOptionKeyPath);

		END_INTERFACE
	} IAxWinAmbientDispatchVtbl;

	interface IAxWinAmbientDispatch
	{
		CONST_VTBL struct IAxWinAmbientDispatchVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IAxWinAmbientDispatch_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAxWinAmbientDispatch_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IAxWinAmbientDispatch_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IAxWinAmbientDispatch_GetTypeInfoCount(This,pctinfo)	\
	(This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAxWinAmbientDispatch_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
	(This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAxWinAmbientDispatch_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
	(This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAxWinAmbientDispatch_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
	(This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAxWinAmbientDispatch_put_AllowWindowlessActivation(This,bCanWindowlessActivate)	\
	(This)->lpVtbl -> put_AllowWindowlessActivation(This,bCanWindowlessActivate)

#define IAxWinAmbientDispatch_get_AllowWindowlessActivation(This,pbCanWindowlessActivate)	\
	(This)->lpVtbl -> get_AllowWindowlessActivation(This,pbCanWindowlessActivate)

#define IAxWinAmbientDispatch_put_BackColor(This,clrBackground)	\
	(This)->lpVtbl -> put_BackColor(This,clrBackground)

#define IAxWinAmbientDispatch_get_BackColor(This,pclrBackground)	\
	(This)->lpVtbl -> get_BackColor(This,pclrBackground)

#define IAxWinAmbientDispatch_put_ForeColor(This,clrForeground)	\
	(This)->lpVtbl -> put_ForeColor(This,clrForeground)

#define IAxWinAmbientDispatch_get_ForeColor(This,pclrForeground)	\
	(This)->lpVtbl -> get_ForeColor(This,pclrForeground)

#define IAxWinAmbientDispatch_put_LocaleID(This,lcidLocaleID)	\
	(This)->lpVtbl -> put_LocaleID(This,lcidLocaleID)

#define IAxWinAmbientDispatch_get_LocaleID(This,plcidLocaleID)	\
	(This)->lpVtbl -> get_LocaleID(This,plcidLocaleID)

#define IAxWinAmbientDispatch_put_UserMode(This,bUserMode)	\
	(This)->lpVtbl -> put_UserMode(This,bUserMode)

#define IAxWinAmbientDispatch_get_UserMode(This,pbUserMode)	\
	(This)->lpVtbl -> get_UserMode(This,pbUserMode)

#define IAxWinAmbientDispatch_put_DisplayAsDefault(This,bDisplayAsDefault)	\
	(This)->lpVtbl -> put_DisplayAsDefault(This,bDisplayAsDefault)

#define IAxWinAmbientDispatch_get_DisplayAsDefault(This,pbDisplayAsDefault)	\
	(This)->lpVtbl -> get_DisplayAsDefault(This,pbDisplayAsDefault)

#define IAxWinAmbientDispatch_put_Font(This,pFont)	\
	(This)->lpVtbl -> put_Font(This,pFont)

#define IAxWinAmbientDispatch_get_Font(This,pFont)	\
	(This)->lpVtbl -> get_Font(This,pFont)

#define IAxWinAmbientDispatch_put_MessageReflect(This,bMsgReflect)	\
	(This)->lpVtbl -> put_MessageReflect(This,bMsgReflect)

#define IAxWinAmbientDispatch_get_MessageReflect(This,pbMsgReflect)	\
	(This)->lpVtbl -> get_MessageReflect(This,pbMsgReflect)

#define IAxWinAmbientDispatch_get_ShowGrabHandles(This,pbShowGrabHandles)	\
	(This)->lpVtbl -> get_ShowGrabHandles(This,pbShowGrabHandles)

#define IAxWinAmbientDispatch_get_ShowHatching(This,pbShowHatching)	\
	(This)->lpVtbl -> get_ShowHatching(This,pbShowHatching)

#define IAxWinAmbientDispatch_put_DocHostFlags(This,dwDocHostFlags)	\
	(This)->lpVtbl -> put_DocHostFlags(This,dwDocHostFlags)

#define IAxWinAmbientDispatch_get_DocHostFlags(This,pdwDocHostFlags)	\
	(This)->lpVtbl -> get_DocHostFlags(This,pdwDocHostFlags)

#define IAxWinAmbientDispatch_put_DocHostDoubleClickFlags(This,dwDocHostDoubleClickFlags)	\
	(This)->lpVtbl -> put_DocHostDoubleClickFlags(This,dwDocHostDoubleClickFlags)

#define IAxWinAmbientDispatch_get_DocHostDoubleClickFlags(This,pdwDocHostDoubleClickFlags)	\
	(This)->lpVtbl -> get_DocHostDoubleClickFlags(This,pdwDocHostDoubleClickFlags)

#define IAxWinAmbientDispatch_put_AllowContextMenu(This,bAllowContextMenu)	\
	(This)->lpVtbl -> put_AllowContextMenu(This,bAllowContextMenu)

#define IAxWinAmbientDispatch_get_AllowContextMenu(This,pbAllowContextMenu)	\
	(This)->lpVtbl -> get_AllowContextMenu(This,pbAllowContextMenu)

#define IAxWinAmbientDispatch_put_AllowShowUI(This,bAllowShowUI)	\
	(This)->lpVtbl -> put_AllowShowUI(This,bAllowShowUI)

#define IAxWinAmbientDispatch_get_AllowShowUI(This,pbAllowShowUI)	\
	(This)->lpVtbl -> get_AllowShowUI(This,pbAllowShowUI)

#define IAxWinAmbientDispatch_put_OptionKeyPath(This,bstrOptionKeyPath)	\
	(This)->lpVtbl -> put_OptionKeyPath(This,bstrOptionKeyPath)

#define IAxWinAmbientDispatch_get_OptionKeyPath(This,pbstrOptionKeyPath)	\
	(This)->lpVtbl -> get_OptionKeyPath(This,pbstrOptionKeyPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_AllowWindowlessActivation_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ VARIANT_BOOL bCanWindowlessActivate);


void __RPC_STUB IAxWinAmbientDispatch_put_AllowWindowlessActivation_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_AllowWindowlessActivation_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbCanWindowlessActivate);


void __RPC_STUB IAxWinAmbientDispatch_get_AllowWindowlessActivation_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_BackColor_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ OLE_COLOR clrBackground);


void __RPC_STUB IAxWinAmbientDispatch_put_BackColor_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_BackColor_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ OLE_COLOR *pclrBackground);


void __RPC_STUB IAxWinAmbientDispatch_get_BackColor_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_ForeColor_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ OLE_COLOR clrForeground);


void __RPC_STUB IAxWinAmbientDispatch_put_ForeColor_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_ForeColor_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ OLE_COLOR *pclrForeground);


void __RPC_STUB IAxWinAmbientDispatch_get_ForeColor_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_LocaleID_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ LCID lcidLocaleID);


void __RPC_STUB IAxWinAmbientDispatch_put_LocaleID_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_LocaleID_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ LCID *plcidLocaleID);


void __RPC_STUB IAxWinAmbientDispatch_get_LocaleID_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_UserMode_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ VARIANT_BOOL bUserMode);


void __RPC_STUB IAxWinAmbientDispatch_put_UserMode_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_UserMode_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbUserMode);


void __RPC_STUB IAxWinAmbientDispatch_get_UserMode_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_DisplayAsDefault_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ VARIANT_BOOL bDisplayAsDefault);


void __RPC_STUB IAxWinAmbientDispatch_put_DisplayAsDefault_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_DisplayAsDefault_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbDisplayAsDefault);


void __RPC_STUB IAxWinAmbientDispatch_get_DisplayAsDefault_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_Font_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ IFontDisp *pFont);


void __RPC_STUB IAxWinAmbientDispatch_put_Font_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_Font_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ IFontDisp **pFont);


void __RPC_STUB IAxWinAmbientDispatch_get_Font_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_MessageReflect_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ VARIANT_BOOL bMsgReflect);


void __RPC_STUB IAxWinAmbientDispatch_put_MessageReflect_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_MessageReflect_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbMsgReflect);


void __RPC_STUB IAxWinAmbientDispatch_get_MessageReflect_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_ShowGrabHandles_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbShowGrabHandles);


void __RPC_STUB IAxWinAmbientDispatch_get_ShowGrabHandles_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_ShowHatching_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbShowHatching);


void __RPC_STUB IAxWinAmbientDispatch_get_ShowHatching_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_DocHostFlags_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ DWORD dwDocHostFlags);


void __RPC_STUB IAxWinAmbientDispatch_put_DocHostFlags_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_DocHostFlags_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ DWORD *pdwDocHostFlags);


void __RPC_STUB IAxWinAmbientDispatch_get_DocHostFlags_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_DocHostDoubleClickFlags_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ DWORD dwDocHostDoubleClickFlags);


void __RPC_STUB IAxWinAmbientDispatch_put_DocHostDoubleClickFlags_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_DocHostDoubleClickFlags_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ DWORD *pdwDocHostDoubleClickFlags);


void __RPC_STUB IAxWinAmbientDispatch_get_DocHostDoubleClickFlags_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_AllowContextMenu_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ VARIANT_BOOL bAllowContextMenu);


void __RPC_STUB IAxWinAmbientDispatch_put_AllowContextMenu_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_AllowContextMenu_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbAllowContextMenu);


void __RPC_STUB IAxWinAmbientDispatch_get_AllowContextMenu_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_AllowShowUI_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ VARIANT_BOOL bAllowShowUI);


void __RPC_STUB IAxWinAmbientDispatch_put_AllowShowUI_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_AllowShowUI_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ VARIANT_BOOL *pbAllowShowUI);


void __RPC_STUB IAxWinAmbientDispatch_get_AllowShowUI_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_put_OptionKeyPath_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [in] */ BSTR bstrOptionKeyPath);


void __RPC_STUB IAxWinAmbientDispatch_put_OptionKeyPath_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatch_get_OptionKeyPath_Proxy( 
	IAxWinAmbientDispatch * This,
	/* [retval][out] */ BSTR *pbstrOptionKeyPath);


void __RPC_STUB IAxWinAmbientDispatch_get_OptionKeyPath_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IAxWinAmbientDispatch_INTERFACE_DEFINED__ */


#ifndef __IAxWinAmbientDispatchEx_INTERFACE_DEFINED__
#define __IAxWinAmbientDispatchEx_INTERFACE_DEFINED__

/* interface IAxWinAmbientDispatchEx */
/* [unique][helpstring][uuid][dual][object] */ 


EXTERN_C const IID IID_IAxWinAmbientDispatchEx;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("B2D0778B-AC99-4c58-A5C8-E7724E5316B5")
	IAxWinAmbientDispatchEx : public IAxWinAmbientDispatch
	{
	public:
		virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetAmbientDispatch( 
			_In_ IDispatch *pDispatch) = 0;

	};

#else 	/* C style interface */

	typedef struct IAxWinAmbientDispatchExVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IAxWinAmbientDispatchEx * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IAxWinAmbientDispatchEx * This);

		HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
			IAxWinAmbientDispatchEx * This,
			/* [out] */ UINT *pctinfo);

		HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ UINT iTInfo,
			/* [in] */ LCID lcid,
			/* [out] */ ITypeInfo **ppTInfo);

		HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ REFIID riid,
			/* [size_is][in] */ _In_reads_(cNames) LPOLESTR *rgszNames,
			/* [in] */ UINT cNames,
			/* [in] */ LCID lcid,
			/* [size_is][out] */ DISPID *rgDispId);

		/* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ DISPID dispIdMember,
			/* [in] */ REFIID riid,
			/* [in] */ LCID lcid,
			/* [in] */ WORD wFlags,
			/* [out][in] */ DISPPARAMS *pDispParams,
			/* [out] */ VARIANT *pVarResult,
			/* [out] */ EXCEPINFO *pExcepInfo,
			/* [out] */ UINT *puArgErr);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowWindowlessActivation )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ VARIANT_BOOL bCanWindowlessActivate);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowWindowlessActivation )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbCanWindowlessActivate);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BackColor )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ OLE_COLOR clrBackground);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BackColor )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ OLE_COLOR *pclrBackground);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ForeColor )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ OLE_COLOR clrForeground);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ForeColor )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ OLE_COLOR *pclrForeground);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocaleID )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ LCID lcidLocaleID);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocaleID )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ LCID *plcidLocaleID);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UserMode )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ VARIANT_BOOL bUserMode);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserMode )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbUserMode);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayAsDefault )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ VARIANT_BOOL bDisplayAsDefault);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayAsDefault )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbDisplayAsDefault);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Font )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ IFontDisp *pFont);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Font )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ IFontDisp **pFont);

		/* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MessageReflect )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ VARIANT_BOOL bMsgReflect);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MessageReflect )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbMsgReflect);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShowGrabHandles )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbShowGrabHandles);

		/* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShowHatching )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbShowHatching);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DocHostFlags )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ DWORD dwDocHostFlags);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocHostFlags )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ DWORD *pdwDocHostFlags);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DocHostDoubleClickFlags )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ DWORD dwDocHostDoubleClickFlags);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocHostDoubleClickFlags )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ DWORD *pdwDocHostDoubleClickFlags);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowContextMenu )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ VARIANT_BOOL bAllowContextMenu);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowContextMenu )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbAllowContextMenu);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowShowUI )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ VARIANT_BOOL bAllowShowUI);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowShowUI )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ VARIANT_BOOL *pbAllowShowUI);

		/* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OptionKeyPath )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ BSTR bstrOptionKeyPath);

		/* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OptionKeyPath )( 
			IAxWinAmbientDispatchEx * This,
			/* [retval][out] */ BSTR *pbstrOptionKeyPath);

		/* [id] */ HRESULT ( STDMETHODCALLTYPE *SetAmbientDispatch )( 
			IAxWinAmbientDispatchEx * This,
			/* [in] */ IDispatch *pDispatch);

		END_INTERFACE
	} IAxWinAmbientDispatchExVtbl;

	interface IAxWinAmbientDispatchEx
	{
		CONST_VTBL struct IAxWinAmbientDispatchExVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IAxWinAmbientDispatchEx_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAxWinAmbientDispatchEx_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IAxWinAmbientDispatchEx_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IAxWinAmbientDispatchEx_GetTypeInfoCount(This,pctinfo)	\
	(This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAxWinAmbientDispatchEx_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
	(This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAxWinAmbientDispatchEx_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
	(This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAxWinAmbientDispatchEx_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
	(This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAxWinAmbientDispatchEx_put_AllowWindowlessActivation(This,bCanWindowlessActivate)	\
	(This)->lpVtbl -> put_AllowWindowlessActivation(This,bCanWindowlessActivate)

#define IAxWinAmbientDispatchEx_get_AllowWindowlessActivation(This,pbCanWindowlessActivate)	\
	(This)->lpVtbl -> get_AllowWindowlessActivation(This,pbCanWindowlessActivate)

#define IAxWinAmbientDispatchEx_put_BackColor(This,clrBackground)	\
	(This)->lpVtbl -> put_BackColor(This,clrBackground)

#define IAxWinAmbientDispatchEx_get_BackColor(This,pclrBackground)	\
	(This)->lpVtbl -> get_BackColor(This,pclrBackground)

#define IAxWinAmbientDispatchEx_put_ForeColor(This,clrForeground)	\
	(This)->lpVtbl -> put_ForeColor(This,clrForeground)

#define IAxWinAmbientDispatchEx_get_ForeColor(This,pclrForeground)	\
	(This)->lpVtbl -> get_ForeColor(This,pclrForeground)

#define IAxWinAmbientDispatchEx_put_LocaleID(This,lcidLocaleID)	\
	(This)->lpVtbl -> put_LocaleID(This,lcidLocaleID)

#define IAxWinAmbientDispatchEx_get_LocaleID(This,plcidLocaleID)	\
	(This)->lpVtbl -> get_LocaleID(This,plcidLocaleID)

#define IAxWinAmbientDispatchEx_put_UserMode(This,bUserMode)	\
	(This)->lpVtbl -> put_UserMode(This,bUserMode)

#define IAxWinAmbientDispatchEx_get_UserMode(This,pbUserMode)	\
	(This)->lpVtbl -> get_UserMode(This,pbUserMode)

#define IAxWinAmbientDispatchEx_put_DisplayAsDefault(This,bDisplayAsDefault)	\
	(This)->lpVtbl -> put_DisplayAsDefault(This,bDisplayAsDefault)

#define IAxWinAmbientDispatchEx_get_DisplayAsDefault(This,pbDisplayAsDefault)	\
	(This)->lpVtbl -> get_DisplayAsDefault(This,pbDisplayAsDefault)

#define IAxWinAmbientDispatchEx_put_Font(This,pFont)	\
	(This)->lpVtbl -> put_Font(This,pFont)

#define IAxWinAmbientDispatchEx_get_Font(This,pFont)	\
	(This)->lpVtbl -> get_Font(This,pFont)

#define IAxWinAmbientDispatchEx_put_MessageReflect(This,bMsgReflect)	\
	(This)->lpVtbl -> put_MessageReflect(This,bMsgReflect)

#define IAxWinAmbientDispatchEx_get_MessageReflect(This,pbMsgReflect)	\
	(This)->lpVtbl -> get_MessageReflect(This,pbMsgReflect)

#define IAxWinAmbientDispatchEx_get_ShowGrabHandles(This,pbShowGrabHandles)	\
	(This)->lpVtbl -> get_ShowGrabHandles(This,pbShowGrabHandles)

#define IAxWinAmbientDispatchEx_get_ShowHatching(This,pbShowHatching)	\
	(This)->lpVtbl -> get_ShowHatching(This,pbShowHatching)

#define IAxWinAmbientDispatchEx_put_DocHostFlags(This,dwDocHostFlags)	\
	(This)->lpVtbl -> put_DocHostFlags(This,dwDocHostFlags)

#define IAxWinAmbientDispatchEx_get_DocHostFlags(This,pdwDocHostFlags)	\
	(This)->lpVtbl -> get_DocHostFlags(This,pdwDocHostFlags)

#define IAxWinAmbientDispatchEx_put_DocHostDoubleClickFlags(This,dwDocHostDoubleClickFlags)	\
	(This)->lpVtbl -> put_DocHostDoubleClickFlags(This,dwDocHostDoubleClickFlags)

#define IAxWinAmbientDispatchEx_get_DocHostDoubleClickFlags(This,pdwDocHostDoubleClickFlags)	\
	(This)->lpVtbl -> get_DocHostDoubleClickFlags(This,pdwDocHostDoubleClickFlags)

#define IAxWinAmbientDispatchEx_put_AllowContextMenu(This,bAllowContextMenu)	\
	(This)->lpVtbl -> put_AllowContextMenu(This,bAllowContextMenu)

#define IAxWinAmbientDispatchEx_get_AllowContextMenu(This,pbAllowContextMenu)	\
	(This)->lpVtbl -> get_AllowContextMenu(This,pbAllowContextMenu)

#define IAxWinAmbientDispatchEx_put_AllowShowUI(This,bAllowShowUI)	\
	(This)->lpVtbl -> put_AllowShowUI(This,bAllowShowUI)

#define IAxWinAmbientDispatchEx_get_AllowShowUI(This,pbAllowShowUI)	\
	(This)->lpVtbl -> get_AllowShowUI(This,pbAllowShowUI)

#define IAxWinAmbientDispatchEx_put_OptionKeyPath(This,bstrOptionKeyPath)	\
	(This)->lpVtbl -> put_OptionKeyPath(This,bstrOptionKeyPath)

#define IAxWinAmbientDispatchEx_get_OptionKeyPath(This,pbstrOptionKeyPath)	\
	(This)->lpVtbl -> get_OptionKeyPath(This,pbstrOptionKeyPath)


#define IAxWinAmbientDispatchEx_SetAmbientDispatch(This,pDispatch)	\
	(This)->lpVtbl -> SetAmbientDispatch(This,pDispatch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IAxWinAmbientDispatchEx_SetAmbientDispatch_Proxy( 
	IAxWinAmbientDispatchEx * This,
	/* [in] */ IDispatch *pDispatch);


void __RPC_STUB IAxWinAmbientDispatchEx_SetAmbientDispatch_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IAxWinAmbientDispatchEx_INTERFACE_DEFINED__ */


#ifndef __IInternalConnection_INTERFACE_DEFINED__
#define __IInternalConnection_INTERFACE_DEFINED__

/* interface IInternalConnection */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IInternalConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("72AD0770-6A9F-11d1-BCEC-0060088F444E")
	IInternalConnection : public IUnknown
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE AddConnection( void) = 0;

		virtual HRESULT STDMETHODCALLTYPE ReleaseConnection( void) = 0;

	};

#else 	/* C style interface */

	typedef struct IInternalConnectionVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IInternalConnection * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IInternalConnection * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IInternalConnection * This);

		HRESULT ( STDMETHODCALLTYPE *AddConnection )( 
			IInternalConnection * This);

		HRESULT ( STDMETHODCALLTYPE *ReleaseConnection )( 
			IInternalConnection * This);

		END_INTERFACE
	} IInternalConnectionVtbl;

	interface IInternalConnection
	{
		CONST_VTBL struct IInternalConnectionVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IInternalConnection_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInternalConnection_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IInternalConnection_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IInternalConnection_AddConnection(This)	\
	(This)->lpVtbl -> AddConnection(This)

#define IInternalConnection_ReleaseConnection(This)	\
	(This)->lpVtbl -> ReleaseConnection(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */

HRESULT STDMETHODCALLTYPE IInternalConnection_AddConnection_Proxy( 
	IInternalConnection * This);

void __RPC_STUB IInternalConnection_AddConnection_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);

HRESULT STDMETHODCALLTYPE IInternalConnection_ReleaseConnection_Proxy( 
	IInternalConnection * This);


void __RPC_STUB IInternalConnection_ReleaseConnection_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);

#endif 	/* __IInternalConnection_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_atliface_0257 */
/* [local] */ 


#pragma pack(push,_ATL_PACKING)
namespace ATL
{
#ifdef __cplusplus
#include <atldef.h>
#else
#define ATLAPI EXTERN_C HRESULT __declspec(dllimport) __stdcall
#define ATLAPI_(x) EXTERN_C __declspec(dllimport) x __stdcall
#define ATLINLINE
#endif	// __cplusplus

ATLAPI_(INT_PTR) AtlAxDialogBoxW(
	_In_ HINSTANCE hInstance, 
	_In_z_ LPCWSTR lpTemplateName,
	_In_ HWND hWndParent, 
	_In_ DLGPROC lpDialogProc, 
	_In_ LPARAM dwInitParam);

ATLAPI_(INT_PTR) AtlAxDialogBoxA(
	_In_ HINSTANCE hInstance, 
	_In_z_ LPCSTR lpTemplateName, 
	_In_ HWND hWndParent, 
	_In_ DLGPROC lpDialogProc, 
	_In_ LPARAM dwInitParam);

#ifdef UNICODE
#define AtlAxDialogBox AtlAxDialogBoxW
#else
#define AtlAxDialogBox AtlAxDialogBoxA
#endif

ATLAPI_(HWND) AtlAxCreateDialogW(
	_In_ HINSTANCE hInstance,
	_In_z_ LPCWSTR lpTemplateName, 
	_In_ HWND hWndParent, 
	_In_ DLGPROC lpDialogProc, 
	_In_ LPARAM dwInitParam);

ATLAPI_(HWND) AtlAxCreateDialogA(
	_In_ HINSTANCE hInstance, 
	_In_z_ LPCSTR lpTemplateName,
	_In_ HWND hWndParent, 
	_In_ DLGPROC lpDialogProc, 
	_In_ LPARAM dwInitParam);

#ifdef UNICODE
#define AtlAxCreateDialog AtlAxCreateDialogW
#else
#define AtlAxCreateDialog AtlAxCreateDialogA
#endif

ATLAPI AtlAxCreateControl(
	_In_z_ LPCOLESTR lpszName, 
	_In_ HWND hWnd, 
	_Inout_opt_ IStream* pStream, 
	_Outptr_ IUnknown** ppUnkContainer);

#ifdef __cplusplus

ATLAPI AtlAxCreateControlEx(
	_In_z_ LPCOLESTR lpszName, 
	_In_ HWND hWnd, 
	_Inout_opt_ IStream* pStream,
	_Outptr_opt_ IUnknown** ppUnkContainer,
	_Outptr_opt_ IUnknown** ppUnkControl,
	_In_ REFIID iidSink=IID_NULL, 
	_Inout_opt_ IUnknown* punkSink=NULL);

ATLAPI AtlAxCreateControlLic(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd,
	_Inout_opt_ IStream* pStream, 
	_Outptr_opt_ IUnknown** ppUnkContainer, 
	_In_opt_z_ BSTR bstrLic = NULL);

ATLAPI AtlAxCreateControlLicEx(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd, 
	_Inout_opt_ IStream* pStream, 
	_Outptr_opt_ IUnknown** ppUnkContainer, 
	_Outptr_opt_ IUnknown** ppUnkControl, 
	_In_ REFIID iidSink=IID_NULL, 
	_Inout_opt_ IUnknown* punkSink=NULL, 
	_In_opt_z_ BSTR bstrLic = NULL);

#else

ATLAPI AtlAxCreateControlEx(
	_In_z_ LPCOLESTR lpszName, 
	_In_ HWND hWnd, 
	_Inout_opt_ IStream* pStream, 
	_Outptr_opt_ IUnknown** ppUnkContainer,
	_Outptr_opt_ IUnknown** ppUnkControl,
	_In_ REFIID iidSink, 
	_Inout_opt_ IUnknown* punkSink);

ATLAPI AtlAxCreateControlLic(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd, 
	_Inout_opt_ IStream* pStream, 
	_Outptr_opt_ IUnknown** ppUnkContainer,
	_In_opt_z_ BSTR bstrLic);

ATLAPI AtlAxCreateControlLicEx(
	_In_z_ LPCOLESTR lpszName, 
	_Inout_ HWND hWnd, 
	_In_opt_ IStream* pStream, 
	_Outptr_opt_ IUnknown** ppUnkContainer, 
	_Outptr_opt_ IUnknown** ppUnkControl, 
	_In_ REFIID iidSink,
	_Inout_opt_ IUnknown* punkSink,
	_In_opt_z_ BSTR bstrLic);
	
#endif	// __cplusplus
	
ATLAPI AtlAxAttachControl(
	_Inout_ IUnknown* pControl,
	_In_ HWND hWnd, 
	_Outptr_opt_ IUnknown** ppUnkContainer);

ATLAPI_(BOOL) AtlAxWinInit();

ATLAPI AtlAxGetHost(
	_In_ HWND h, 
	_Outptr_ IUnknown** pp);

ATLAPI AtlAxGetControl(
	_In_ HWND h, 
	_Outptr_ IUnknown** pp);

}; //namespace ATL
#pragma pack(pop)

extern RPC_IF_HANDLE __MIDL_itf_atliface_0257_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_atliface_0257_v0_0_s_ifspec;

#ifndef __IAccessibleProxy_INTERFACE_DEFINED__
#define __IAccessibleProxy_INTERFACE_DEFINED__

/* interface IAccessibleProxy */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAccessibleProxy;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("7A7D9DCF-B7A1-4019-9031-258268846980")
	IAccessibleProxy : public IUnknown
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetServer( 
			/* [in] */ _In_ IAccessible *pAccessible,
			/* [in] */ _In_ IAccessibleServer *pServer) = 0;

	};

#else 	/* C style interface */

	typedef struct IAccessibleProxyVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IAccessibleProxy * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IAccessibleProxy * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IAccessibleProxy * This);

		HRESULT ( STDMETHODCALLTYPE *SetServer )( 
			IAccessibleProxy * This,
			/* [in] */ IAccessible *pAccessible,
			/* [in] */ IAccessibleServer *pServer);

		END_INTERFACE
	} IAccessibleProxyVtbl;

	interface IAccessibleProxy
	{
		CONST_VTBL struct IAccessibleProxyVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IAccessibleProxy_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccessibleProxy_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IAccessibleProxy_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IAccessibleProxy_SetServer(This,pAccessible,pServer)	\
	(This)->lpVtbl -> SetServer(This,pAccessible,pServer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAccessibleProxy_SetServer_Proxy( 
	IAccessibleProxy * This,
	/* [in] */ IAccessible *pAccessible,
	/* [in] */ IAccessibleServer *pServer);


void __RPC_STUB IAccessibleProxy_SetServer_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IAccessibleProxy_INTERFACE_DEFINED__ */


#ifndef __IAccessibleServer_INTERFACE_DEFINED__
#define __IAccessibleServer_INTERFACE_DEFINED__

/* interface IAccessibleServer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAccessibleServer;

#if defined(__cplusplus) && !defined(CINTERFACE)

	MIDL_INTERFACE("52C8FB5E-D779-4e77-AE9F-F611FA7E9D7A")
	IAccessibleServer : public IUnknown
	{
	public:
		virtual HRESULT STDMETHODCALLTYPE SetProxy( 
			/* [in] */ IAccessibleProxy *pUnknown) = 0;

		virtual HRESULT STDMETHODCALLTYPE GetHWND( 
			/* [out] */ HWND *phWnd) = 0;

		virtual HRESULT STDMETHODCALLTYPE GetEnumVariant( 
			/* [out] */ IEnumVARIANT **ppEnumVariant) = 0;

	};

#else 	/* C style interface */

	typedef struct IAccessibleServerVtbl
	{
		BEGIN_INTERFACE

		HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
			IAccessibleServer * This,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void **ppvObject);

		ULONG ( STDMETHODCALLTYPE *AddRef )( 
			IAccessibleServer * This);

		ULONG ( STDMETHODCALLTYPE *Release )( 
			IAccessibleServer * This);

		HRESULT ( STDMETHODCALLTYPE *SetProxy )( 
			IAccessibleServer * This,
			/* [in] */ IAccessibleProxy *pUnknown);

		HRESULT ( STDMETHODCALLTYPE *GetHWND )( 
			IAccessibleServer * This,
			/* [out] */ HWND *phWnd);

		HRESULT ( STDMETHODCALLTYPE *GetEnumVariant )( 
			IAccessibleServer * This,
			/* [out] */ IEnumVARIANT **ppEnumVariant);

		END_INTERFACE
	} IAccessibleServerVtbl;

	interface IAccessibleServer
	{
		CONST_VTBL struct IAccessibleServerVtbl *lpVtbl;
	};



#ifdef COBJMACROS


#define IAccessibleServer_QueryInterface(This,riid,ppvObject)	\
	(This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccessibleServer_AddRef(This)	\
	(This)->lpVtbl -> AddRef(This)

#define IAccessibleServer_Release(This)	\
	(This)->lpVtbl -> Release(This)


#define IAccessibleServer_SetProxy(This,pUnknown)	\
	(This)->lpVtbl -> SetProxy(This,pUnknown)

#define IAccessibleServer_GetHWND(This,phWnd)	\
	(This)->lpVtbl -> GetHWND(This,phWnd)

#define IAccessibleServer_GetEnumVariant(This,ppEnumVariant)	\
	(This)->lpVtbl -> GetEnumVariant(This,ppEnumVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAccessibleServer_SetProxy_Proxy( 
	IAccessibleServer * This,
	/* [in] */ IAccessibleProxy *pUnknown);


void __RPC_STUB IAccessibleServer_SetProxy_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessibleServer_GetHWND_Proxy( 
	IAccessibleServer * This,
	/* [out] */ HWND *phWnd);


void __RPC_STUB IAccessibleServer_GetHWND_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessibleServer_GetEnumVariant_Proxy( 
	IAccessibleServer * This,
	/* [out] */ IEnumVARIANT **ppEnumVariant);


void __RPC_STUB IAccessibleServer_GetEnumVariant_Stub(
	IRpcStubBuffer *This,
	IRpcChannelBuffer *_pRpcChannelBuffer,
	PRPC_MESSAGE _pRpcMessage,
	DWORD *_pdwStubPhase);



#endif 	/* __IAccessibleServer_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     __RPC__in unsigned long *, __RPC__in BSTR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out HWND * ); 
void                      __RPC_USER  HWND_UserFree(     __RPC__in unsigned long *, __RPC__in HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


