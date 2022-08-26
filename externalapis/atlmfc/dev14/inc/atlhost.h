// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLHOST_H__
#define __ATLHOST_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4571) //catch(...) blocks compiled with /EHs do NOT catch or re-throw Structured Exceptions
#endif //!_ATL_NO_PRAGMA_WARNINGS

#include <atldef.h>

#if !defined(_ATL_USE_WINAPI_FAMILY_DESKTOP_APP)
#error This file is not compatible with the current WINAPI_FAMILY
#endif

#include <urlmon.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <exdisp.h>

#ifndef _ATL_AXHOST
#define _ATL_AXHOST
#endif //_ATL_AXHOST

#include <atlwin.h>
#include <atlcom.h>

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef _ATL_NO_HOSTING
	#error atlhost.h requires Hosting support (_ATL_NO_HOSTING is defined)
#endif //_ATL_NO_HOSTING


#pragma pack(push,_ATL_PACKING)
namespace ATL
{
//AtlAxWinTerm is not exported
inline BOOL AtlAxWinTerm()
{
	UnregisterClass(CAxWindow::GetWndClassName(), _AtlBaseModule.GetModuleInstance());
	UnregisterClass(CAxWindow2::GetWndClassName(), _AtlBaseModule.GetModuleInstance());

	return TRUE;
}


// Define this to host SHDOCVW rather than MSHTML
#define SHDOCVW

UINT __declspec(selectany) WM_ATLGETHOST = 0;
UINT __declspec(selectany) WM_ATLGETCONTROL = 0;

typedef HRESULT (__stdcall *typeMkParseDisplayName)(
	_In_ IBindCtx*,
	_In_z_ LPCWSTR ,
	_Out_ ULONG*,
	_Out_ LPMONIKER*);
ATLPREFAST_SUPPRESS(6387)
static HRESULT CreateNormalizedObject(
	_In_opt_z_ LPCOLESTR lpszTricsData,
	_In_ REFIID riid,
	_Outptr_ void** ppvObj,
	_Out_ bool& bWasHTML,
	_In_opt_z_ BSTR bstrLicKey)
{
	ATLASSERT(ppvObj);
	if (ppvObj == NULL)
	{
		return E_POINTER;
	}
	*ppvObj = NULL;

	CLSID clsid;
	HRESULT hr = E_FAIL;

	bWasHTML = false;

	if (lpszTricsData == NULL || lpszTricsData[0] == 0)
	{
		return S_OK;
	}

	// Is it HTML ?
	if ((lpszTricsData[0] == OLECHAR('M') || lpszTricsData[0] == OLECHAR('m')) &&
		(lpszTricsData[1] == OLECHAR('S') || lpszTricsData[1] == OLECHAR('s')) &&
		(lpszTricsData[2] == OLECHAR('H') || lpszTricsData[2] == OLECHAR('h')) &&
		(lpszTricsData[3] == OLECHAR('T') || lpszTricsData[3] == OLECHAR('t')) &&
		(lpszTricsData[4] == OLECHAR('M') || lpszTricsData[4] == OLECHAR('m')) &&
		(lpszTricsData[5] == OLECHAR('L') || lpszTricsData[5] == OLECHAR('l')) &&
		(lpszTricsData[6] == OLECHAR(':')))
	{
		// It's HTML, so let's create mshtml
		hr = CoCreateInstance(__uuidof(HTMLDocument), NULL, CLSCTX_INPROC_SERVER, riid, ppvObj);
		bWasHTML = true;
	}
	// Is it a URL?
	else if (CAtlModule::FindOneOf(COLE2CT(lpszTricsData), _T(":")))
	{
		// URL so let's create shdocvw
		hr = CoCreateInstance(__uuidof(WebBrowser), NULL, CLSCTX_INPROC_SERVER, riid, ppvObj);
		bWasHTML = true;
	}
	// assume ProgID or CLSID
	else
	{
		// Can't be clsid, or progid if length is greater than 255
		if (ocslen(lpszTricsData) < 255)
		{
			if (lpszTricsData[0] == '{') // Is it a CLSID?
			{
				hr = CLSIDFromString((LPOLESTR)lpszTricsData, &clsid);
			}
			else
			{
				hr = CLSIDFromProgID((LPOLESTR)lpszTricsData, &clsid); // How about a ProgID?
			}
		}
		if (SUCCEEDED(hr))
		{
			// If the license key is present
			// Create using IClassFactory2.
			if (::SysStringLen(bstrLicKey) != 0)
			{
				CComPtr<IClassFactory2> spClassFactory;
				hr = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, __uuidof(IClassFactory2), (void**)&spClassFactory);
				if (SUCCEEDED(hr))
				{
					hr = spClassFactory->CreateInstanceLic(NULL, NULL, riid, bstrLicKey, ppvObj);
				}
			}
			else
			{
				hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, riid, ppvObj);
			}
		}
	}
	return hr;
}
ATLPREFAST_UNSUPPRESS()

class ATL_NO_VTABLE CAxFrameWindow :
	public CComObjectRootEx<CComObjectThreadModel>,
	public CWindowImpl<CAxFrameWindow>,
	public IOleInPlaceFrame
{
public:
	CAxFrameWindow()
	{
	}
	void FinalRelease()
	{
		m_spActiveObject.Release();
		if (m_hWnd)
			DestroyWindow();
	}

	DECLARE_POLY_AGGREGATABLE(CAxFrameWindow)

	BEGIN_COM_MAP(CAxFrameWindow)
		COM_INTERFACE_ENTRY(IOleInPlaceFrame)
		COM_INTERFACE_ENTRY(IOleInPlaceUIWindow)
		COM_INTERFACE_ENTRY(IOleWindow)
	END_COM_MAP()

	DECLARE_EMPTY_MSG_MAP()

// IOleWindow
	STDMETHOD(GetWindow)(_Out_ HWND* phwnd)
	{
		ATLASSERT(phwnd != NULL);
		if (phwnd == NULL)
			return E_POINTER;

		if (m_hWnd == NULL)
			Create(NULL, NULL, _T("AXWIN Frame Window"), WS_OVERLAPPEDWINDOW, 0, (UINT)NULL);
		*phwnd = m_hWnd;
		return S_OK;
	}
	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		return S_OK;
	}

// IOleInPlaceUIWindow
	STDMETHOD(GetBorder)(_In_ LPRECT /*lprectBorder*/)
	{
		return S_OK;
	}

	STDMETHOD(RequestBorderSpace)(_In_ LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return INPLACE_E_NOTOOLSPACE;
	}

	STDMETHOD(SetBorderSpace)(_In_ LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return S_OK;
	}

	STDMETHOD(SetActiveObject)(
		_In_ IOleInPlaceActiveObject* pActiveObject,
		_In_opt_z_ LPCOLESTR /*pszObjName*/)
	{
		m_spActiveObject = pActiveObject;
		return S_OK;
	}

// IOleInPlaceFrameWindow
	STDMETHOD(InsertMenus)(
		_In_ HMENU /*hmenuShared*/,
		_Inout_ LPOLEMENUGROUPWIDTHS /*lpMenuWidths*/)
	{
		return S_OK;
	}

	STDMETHOD(SetMenu)(
		_In_ HMENU /*hmenuShared*/,
		_In_ HOLEMENU /*holemenu*/,
		_In_ HWND /*hwndActiveObject*/)
	{
		return S_OK;
	}

	STDMETHOD(RemoveMenus)(_In_ HMENU /*hmenuShared*/)
	{
		return S_OK;
	}

	STDMETHOD(SetStatusText)(_In_opt_z_ LPCOLESTR /*pszStatusText*/)
	{
		return S_OK;
	}

	STDMETHOD(EnableModeless)(BOOL /*fEnable*/)
	{
		return S_OK;
	}

	STDMETHOD(TranslateAccelerator)(
		_In_ LPMSG /*lpMsg*/,
		WORD /*wID*/)
	{
		return S_FALSE;
	}

	CComPtr<IOleInPlaceActiveObject> m_spActiveObject;
};


class ATL_NO_VTABLE CAxUIWindow :
	public CComObjectRootEx<CComObjectThreadModel>,
	public CWindowImpl<CAxUIWindow>,
	public IOleInPlaceUIWindow
{
public:
	CAxUIWindow()
	{
	}

	void FinalRelease()
	{
		m_spActiveObject.Release();
		if (m_hWnd)
			DestroyWindow();
	}

	DECLARE_POLY_AGGREGATABLE(CAxUIWindow)

	BEGIN_COM_MAP(CAxUIWindow)
		COM_INTERFACE_ENTRY(IOleInPlaceUIWindow)
		COM_INTERFACE_ENTRY(IOleWindow)
	END_COM_MAP()

	DECLARE_EMPTY_MSG_MAP()

// IOleWindow
	STDMETHOD(GetWindow)(_Out_ HWND* phwnd)
	{
		ATLENSURE_RETURN(phwnd);
		if (m_hWnd == NULL)
			Create(NULL, NULL, _T("AXWIN UI Window"), WS_OVERLAPPEDWINDOW, 0, (UINT)NULL);
		*phwnd = m_hWnd;
		return S_OK;
	}

	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		return S_OK;
	}

// IOleInPlaceUIWindow
	STDMETHOD(GetBorder)(_In_ LPRECT /*lprectBorder*/)
	{
		return S_OK;
	}

	STDMETHOD(RequestBorderSpace)(_In_ LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return INPLACE_E_NOTOOLSPACE;
	}

	STDMETHOD(SetBorderSpace)(_In_ LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return S_OK;
	}

	STDMETHOD(SetActiveObject)(
		_In_ IOleInPlaceActiveObject* pActiveObject,
		_In_opt_z_ LPCOLESTR /*pszObjName*/)
	{
		m_spActiveObject = pActiveObject;
		return S_OK;
	}

	CComPtr<IOleInPlaceActiveObject> m_spActiveObject;
};


/////////////////////////////////////////////////////////////////////////////
// CAxHostWindow
// This class is not cocreateable

class ATL_NO_VTABLE CAxHostWindow :
		public CComCoClass<CAxHostWindow , &CLSID_NULL>,
		public CComObjectRootEx<CComSingleThreadModel>,
		public CWindowImpl<CAxHostWindow>,
		public IAxWinHostWindowLic,
		public IOleClientSite,
		public IOleInPlaceSiteWindowless,
		public IOleControlSite,
		public IOleContainer,
		public IObjectWithSiteImpl<CAxHostWindow>,
		public IServiceProvider,
		public IAdviseSink,
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		public IDocHostUIHandler,
#endif
		public IDispatchImpl<IAxWinAmbientDispatchEx, &__uuidof(IAxWinAmbientDispatchEx), &CAtlModule::m_libid, 0xFFFF, 0xFFFF>
{
public:
// ctor/dtor
	CAxHostWindow()
	{
		m_bInPlaceActive = FALSE;
		m_bUIActive = FALSE;
		m_bMDIApp = FALSE;
		m_bWindowless = FALSE;
		m_bCapture = FALSE;
		m_bHaveFocus = FALSE;
		m_bLocked = FALSE;

		// Initialize ambient properties
		m_bCanWindowlessActivate = TRUE;
		m_bUserMode = TRUE;
		m_bDisplayAsDefault = FALSE;
		m_clrBackground = 0;
		m_clrForeground = GetSysColor(COLOR_WINDOWTEXT);
		m_lcidLocaleID = LOCALE_USER_DEFAULT;
		m_bMessageReflect = true;

		m_bReleaseAll = FALSE;

		m_bSubclassed = FALSE;

		m_iidSink = {0};
		m_dwViewObjectType = 0;
		m_dwAdviseSink = 0xCDCDCDCD;
		m_dwDocHostFlags = DOCHOSTUIFLAG_NO3DBORDER;
		m_dwDocHostDoubleClickFlags = DOCHOSTUIDBLCLK_DEFAULT;
		m_bAllowContextMenu = TRUE;
		m_bAllowShowUI = FALSE;
		m_hDCScreen = NULL;
		m_bDCReleased = true;

		m_hAccel = NULL;

		m_dwOleObject = 0;
		m_dwMiscStatus = 0;
		m_hmSize = {0};
		m_pxSize = {0};
		m_rcPos = {0};
	}

	virtual ~CAxHostWindow()
	{
	}
	void FinalRelease()
	{
		ReleaseAll();
	}

	virtual void OnFinalMessage(_In_ HWND /*hWnd*/)
	{
		GetControllingUnknown()->Release();
	}

	DECLARE_NO_REGISTRY()
	DECLARE_POLY_AGGREGATABLE(CAxHostWindow)
	DECLARE_GET_CONTROLLING_UNKNOWN()

	BEGIN_COM_MAP(CAxHostWindow)
		COM_INTERFACE_ENTRY2(IDispatch, IAxWinAmbientDispatchEx)
		COM_INTERFACE_ENTRY(IAxWinHostWindow)
		COM_INTERFACE_ENTRY(IAxWinHostWindowLic)
		COM_INTERFACE_ENTRY(IOleClientSite)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteWindowless)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteEx)
		COM_INTERFACE_ENTRY(IOleInPlaceSite)
		COM_INTERFACE_ENTRY(IOleWindow)
		COM_INTERFACE_ENTRY(IOleControlSite)
		COM_INTERFACE_ENTRY(IOleContainer)
		COM_INTERFACE_ENTRY(IObjectWithSite)
		COM_INTERFACE_ENTRY(IServiceProvider)
		COM_INTERFACE_ENTRY(IAxWinAmbientDispatchEx)
		COM_INTERFACE_ENTRY(IAxWinAmbientDispatch)
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		COM_INTERFACE_ENTRY(IDocHostUIHandler)
#endif
		COM_INTERFACE_ENTRY(IAdviseSink)
	END_COM_MAP()

	static CWndClassInfo& GetWndClassInfo()
	{
		static CWndClassInfo wc =
		{
			{ sizeof(WNDCLASSEX), 0, StartWindowProc,
			  0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW + 1), 0, _T(ATLAXWIN_CLASS), 0 },
			NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
		};
		return wc;
	}

	BEGIN_MSG_MAP(CAxHostWindow)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		if (m_bWindowless && uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
		{
			// Mouse messages handled when a windowless control has captured the cursor
			// or if the cursor is over the control
			DWORD dwHitResult = m_bCapture ? HITRESULT_HIT : HITRESULT_OUTSIDE;
			if (dwHitResult == HITRESULT_OUTSIDE && m_spViewObject != NULL)
			{
				POINT ptMouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				m_spViewObject->QueryHitPoint(DVASPECT_CONTENT, &m_rcPos, ptMouse, 0, &dwHitResult);
			}
			if (dwHitResult == HITRESULT_HIT)
			{
				MESSAGE_HANDLER(WM_MOUSEMOVE, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_LBUTTONUP, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_RBUTTONUP, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_MBUTTONUP, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_LBUTTONDOWN, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_RBUTTONDOWN, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_MBUTTONDOWN, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnWindowlessMouseMessage)
			}
		}
		if (m_bWindowless & m_bHaveFocus)
		{
			// Keyboard messages handled only when a windowless control has the focus
			MESSAGE_HANDLER(WM_KEYDOWN, OnWindowMessage)
			MESSAGE_HANDLER(WM_KEYUP, OnWindowMessage)
			MESSAGE_HANDLER(WM_CHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_DEADCHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_SYSKEYDOWN, OnWindowMessage)
			MESSAGE_HANDLER(WM_SYSKEYUP, OnWindowMessage)
			MESSAGE_HANDLER(WM_SYSDEADCHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_HELP, OnWindowMessage)
			MESSAGE_HANDLER(WM_CANCELMODE, OnWindowMessage)
			MESSAGE_HANDLER(WM_IME_CHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnWindowMessage)
			MESSAGE_RANGE_HANDLER(WM_IME_SETCONTEXT, WM_IME_KEYUP, OnWindowMessage)
		}
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		if (m_bMessageReflect)
		{
			bHandled = TRUE;
			lResult = ReflectNotifications(uMsg, wParam, lParam, bHandled);
			if(bHandled)
				return TRUE;
		}
		MESSAGE_HANDLER(WM_ATLGETHOST, OnGetUnknown)
		MESSAGE_HANDLER(WM_ATLGETCONTROL, OnGetControl)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
	END_MSG_MAP()

	LRESULT OnForwardMsg(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM lParam,
		_Inout_ BOOL& /*bHandled*/)
	{
		ATLASSUME(lParam != 0);
		LPMSG lpMsg = (LPMSG)lParam;
		CComQIPtr<IOleInPlaceActiveObject, &__uuidof(IOleInPlaceActiveObject)> spInPlaceActiveObject(m_spUnknown);
		if(spInPlaceActiveObject)
		{
			if(spInPlaceActiveObject->TranslateAccelerator(lpMsg) == S_OK)
				return 1;
		}
		return 0;
	}

	LRESULT OnGetUnknown(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& /*bHandled*/)
	{
		IUnknown* pUnk = GetControllingUnknown();
		pUnk->AddRef();
		return (LRESULT)pUnk;
	}
	LRESULT OnGetControl(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& /*bHandled*/)
	{
		IUnknown* pUnk = m_spUnknown;
		if (pUnk)
			pUnk->AddRef();
		return (LRESULT)pUnk;
	}

	void ReleaseAll()
	{
		if (m_bReleaseAll)
			return;
		m_bReleaseAll = TRUE;

		if (m_spViewObject != NULL)
			m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, NULL);

		if(m_dwAdviseSink != 0xCDCDCDCD)
		{
			AtlUnadvise(m_spUnknown, m_iidSink, m_dwAdviseSink);
			m_dwAdviseSink = 0xCDCDCDCD;
		}

		if (m_spOleObject)
		{
			m_spOleObject->Unadvise(m_dwOleObject);
			m_spOleObject->Close(OLECLOSE_NOSAVE);
			m_spOleObject->SetClientSite(NULL);
		}

		if (m_spUnknown != NULL)
		{
			CComPtr<IObjectWithSite> spSite;
			m_spUnknown->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
			if (spSite != NULL)
				spSite->SetSite(NULL);
		}

		m_spViewObject.Release();
		m_dwViewObjectType = 0;

		m_spInPlaceObjectWindowless.Release();
		m_spOleObject.Release();
		m_spUnknown.Release();

		m_spInPlaceUIWindow.Release();
		m_spInPlaceFrame.Release();

		m_bInPlaceActive = FALSE;
		m_bWindowless = FALSE;
		m_bInPlaceActive = FALSE;
		m_bUIActive = FALSE;
		m_bCapture = FALSE;
		m_bReleaseAll = FALSE;

		if (m_hAccel != NULL)
		{
			DestroyAcceleratorTable(m_hAccel);
			m_hAccel = NULL;
		}
	}

// window message handlers
	LRESULT OnEraseBackground(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& bHandled)
	{
		if (m_spViewObject == NULL)
			bHandled = false;

		return 1;
	}

	LRESULT OnMouseActivate(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& bHandled)
	{
		bHandled = FALSE;
		if (m_dwMiscStatus & OLEMISC_NOUIACTIVATE)
		{
			if (m_spOleObject != NULL && !m_bInPlaceActive)
			{
				CComPtr<IOleClientSite> spClientSite;
				GetControllingUnknown()->QueryInterface(__uuidof(IOleClientSite), (void**)&spClientSite);
				if (spClientSite != NULL)
					m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
			}
		}
		else
		{
			BOOL b = false;
			OnSetFocus(0, 0, 0, b);
		}
		return 0;
	}
	LRESULT OnSetFocus(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& bHandled)
	{
		m_bHaveFocus = TRUE;
		if (!m_bReleaseAll)
		{
			if (m_spOleObject != NULL && !m_bUIActive)
			{
				CComPtr<IOleClientSite> spClientSite;
				GetControllingUnknown()->QueryInterface(__uuidof(IOleClientSite), (void**)&spClientSite);
				if (spClientSite != NULL)
					m_spOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
			}
			if (m_bWindowless)
				::SetFocus(m_hWnd);
			else if(!IsChild(::GetFocus()))
				::SetFocus(::GetWindow(m_hWnd, GW_CHILD));
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnKillFocus(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& bHandled)
	{
		m_bHaveFocus = FALSE;
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnSize(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM lParam,
		_Inout_ BOOL& bHandled)
	{
		int nWidth = GET_X_LPARAM(lParam);  // width of client area
		int nHeight = GET_Y_LPARAM(lParam); // height of client area

		m_rcPos.right = m_rcPos.left + nWidth;
		m_rcPos.bottom = m_rcPos.top + nHeight;
		m_pxSize.cx = m_rcPos.right - m_rcPos.left;
		m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
		AtlPixelToHiMetric(&m_pxSize, &m_hmSize);

		if (m_spOleObject)
			m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
		if (m_spInPlaceObjectWindowless)
			m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
		if (m_bWindowless)
			InvalidateRect(NULL, TRUE);
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnDestroy(
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam,
		_Inout_ BOOL& bHandled)
	{
		GetControllingUnknown()->AddRef();
		DefWindowProc(uMsg, wParam, lParam);
		ReleaseAll();
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnWindowMessage(
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam,
		_Inout_ BOOL& bHandled)
	{
		LRESULT lRes = 0;
		HRESULT hr = S_FALSE;
		if (m_bInPlaceActive && m_bWindowless && m_spInPlaceObjectWindowless)
			hr = m_spInPlaceObjectWindowless->OnWindowMessage(uMsg, wParam, lParam, &lRes);
        if (FAILED(hr))
            return 0;
		if (hr == S_FALSE)
			bHandled = FALSE;
		return lRes;
	}
	LRESULT OnWindowlessMouseMessage(
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam,
		_Inout_ BOOL& bHandled)
	{
        HRESULT hr;
		LRESULT lRes = 0;
		if (m_bInPlaceActive && m_bWindowless && m_spInPlaceObjectWindowless)
        {
			hr = m_spInPlaceObjectWindowless->OnWindowMessage(uMsg, wParam, lParam, &lRes);
            if (FAILED(hr))
                return 0;
        }
		bHandled = FALSE;
		return lRes;
	}
	LRESULT OnPaint(
		_In_ UINT /*uMsg*/,
		_In_ WPARAM /*wParam*/,
		_In_ LPARAM /*lParam*/,
		_Inout_ BOOL& bHandled)
	{
		if (m_spViewObject == NULL)
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(m_hWnd, &ps);
			if (hdc == NULL)
				return 0;
			RECT rcClient;
			GetClientRect(&rcClient);
			HBRUSH hbrBack = CreateSolidBrush(m_clrBackground);
			if (hbrBack != NULL)
			{
				FillRect(hdc, &rcClient, hbrBack);
				DeleteObject(hbrBack);
			}
			::EndPaint(m_hWnd, &ps);
			return 1;
		}
		if (m_spViewObject && m_bWindowless)
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(m_hWnd, &ps);

			if (hdc == NULL)
				return 0;

			RECT rcClient;
			GetClientRect(&rcClient);

			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
			if (hBitmap != NULL)
			{
				HDC hdcCompatible = ::CreateCompatibleDC(hdc);
				if (hdcCompatible != NULL)
				{
					HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcCompatible, hBitmap);
					if (hBitmapOld != NULL)
					{
						HBRUSH hbrBack = CreateSolidBrush(m_clrBackground);
						if (hbrBack != NULL)
						{
							FillRect(hdcCompatible, &rcClient, hbrBack);
							DeleteObject(hbrBack);

							m_spViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcCompatible, (RECTL*)&m_rcPos, (RECTL*)&m_rcPos, NULL, 0);

							::BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom,  hdcCompatible, 0, 0, SRCCOPY);
						}
						::SelectObject(hdcCompatible, hBitmapOld);
					}
					::DeleteDC(hdcCompatible);
				}
				::DeleteObject(hBitmap);
			}
			::EndPaint(m_hWnd, &ps);
		}
		else
		{
			bHandled = FALSE;
			return 0;
		}
		return 1;
	}

// IAxWinHostWindow
	STDMETHOD(CreateControl)(
		_In_z_ LPCOLESTR lpTricsData,
		_In_ HWND hWnd,
		_Inout_opt_ IStream* pStream)
	{
		CComPtr<IUnknown> p;
		return CreateControlLicEx(lpTricsData, hWnd, pStream, &p, IID_NULL, NULL, NULL);
	}
ATLPREFAST_SUPPRESS(6103)
	STDMETHOD(CreateControlEx)(
		_In_z_ LPCOLESTR lpszTricsData,
		_In_ HWND hWnd,
		_Inout_opt_ IStream* pStream,
		_Outptr_ IUnknown** ppUnk,
		_In_ REFIID iidAdvise,
		_Inout_ IUnknown* punkSink)
	{
		return CreateControlLicEx(lpszTricsData, hWnd, pStream, ppUnk, iidAdvise, punkSink, NULL);
	}
ATLPREFAST_UNSUPPRESS()
	STDMETHOD(AttachControl)(
		_Inout_ IUnknown* pUnkControl,
		_In_ HWND hWnd)
	{
		HRESULT hr = S_FALSE;

		ReleaseAll();

		bool bReleaseWindowOnFailure = false; // Used to keep track of whether we subclass the window

		if ((m_hWnd != NULL) && (m_hWnd != hWnd)) // Don't release the window if it's the same as the one we already subclass/own
		{
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
			ReleaseWindow();
		}

		if (::IsWindow(hWnd))
		{
			if (m_hWnd != hWnd) // Don't need to subclass the window if we already own it
			{
				SubclassWindow(hWnd);
				bReleaseWindowOnFailure = true;
			}

			hr = ActivateAx(pUnkControl, true, NULL);

			if (FAILED(hr))
			{
				ReleaseAll();

				if (m_hWnd != NULL)
				{
					RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
					if (bReleaseWindowOnFailure) // We subclassed the window in an attempt to create this control, so we unsubclass on failure
						ReleaseWindow();
				}
			}
		}
		return hr;
	}

ATLPREFAST_SUPPRESS(6387)
	STDMETHOD(QueryControl)(
		_In_ REFIID riid,
		_Outptr_ void** ppvObject)
	{
		HRESULT hr = E_POINTER;
		if (ppvObject)
		{
			if (m_spUnknown)
			{
				hr = m_spUnknown->QueryInterface(riid, ppvObject);
			}
			else
			{
				*ppvObject = NULL;
				hr = OLE_E_NOCONNECTION;
			}
		}
		return hr;
	}
ATLPREFAST_UNSUPPRESS()

	STDMETHOD(SetExternalDispatch)(_In_opt_ IDispatch* pDisp)
	{
		m_spExternalDispatch = pDisp;
		return S_OK;
	}
	STDMETHOD(SetExternalUIHandler)(_In_opt_ IDocHostUIHandlerDispatch* pUIHandler)
	{
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		m_spIDocHostUIHandlerDispatch = pUIHandler;
#endif
		return S_OK;
	}

ATLPREFAST_SUPPRESS(6387)

	STDMETHOD(CreateControlLic)(
		_In_z_ LPCOLESTR lpTricsData,
		_In_ HWND hWnd,
		_Inout_opt_ IStream* pStream,
		_In_opt_z_ BSTR bstrLic)
	{
		CComPtr<IUnknown> p;
		return CreateControlLicEx(lpTricsData, hWnd, pStream, &p, IID_NULL, NULL, bstrLic);
	}

	_Success_(return == S_OK) STDMETHOD(CreateControlLicEx)(
		_In_z_ LPCOLESTR lpszTricsData,
		_In_ HWND hWnd,
		_Inout_opt_ IStream* pStream,
		_Outptr_ IUnknown** ppUnk,
		_In_ REFIID iidAdvise,
		_Inout_opt_ IUnknown* punkSink,
		_In_opt_z_ BSTR bstrLic)
	{
		ATLASSERT(ppUnk != NULL);
		if (ppUnk == NULL)
			return E_POINTER;
		*ppUnk = NULL;
		HRESULT hr = S_FALSE;
		bool bReleaseWindowOnFailure = false; // Used to keep track of whether we subclass the window

		ReleaseAll();

		if ((m_hWnd != NULL) && (m_hWnd != hWnd)) // Don't release the window if it's the same as the one we already subclass/own
		{
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
			ReleaseWindow();
		}

		if (::IsWindow(hWnd))
		{
			if (m_hWnd != hWnd) // Don't need to subclass the window if we already own it
			{
				SubclassWindow(hWnd);
				bReleaseWindowOnFailure = true;
			}
			if (m_clrBackground == 0)
			{
				if (IsParentDialog())
				{
					m_clrBackground = GetSysColor(COLOR_BTNFACE);
				}
				else
				{
					m_clrBackground = GetSysColor(COLOR_WINDOW);
				}
			}

			bool bWasHTML = false;

			hr = CreateNormalizedObject(lpszTricsData, __uuidof(IUnknown), (void**)ppUnk, bWasHTML, bstrLic);

			if (SUCCEEDED(hr))
			{
				hr = ActivateAx(*ppUnk, false, pStream);
			}

			// Try to hook up any sink the user might have given us.
			m_iidSink = iidAdvise;
			if(SUCCEEDED(hr) && *ppUnk && punkSink)
			{
				AtlAdvise(*ppUnk, punkSink, m_iidSink, &m_dwAdviseSink);
			}

			if (SUCCEEDED(hr) && bWasHTML && *ppUnk != NULL)
			{
				if ((GetStyle() & (WS_VSCROLL | WS_HSCROLL)) == 0)
				{
					m_dwDocHostFlags |= DOCHOSTUIFLAG_SCROLL_NO;
				}
				else
				{
					DWORD dwStyle = GetStyle();
					SetWindowLong(GWL_STYLE, dwStyle & ~(WS_VSCROLL | WS_HSCROLL));
					SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_DRAWFRAME);
				}

				CComPtr<IUnknown> spUnk(*ppUnk);
				// Is it just plain HTML?
				if ((lpszTricsData[0] == OLECHAR('M') || lpszTricsData[0] == OLECHAR('m')) &&
					(lpszTricsData[1] == OLECHAR('S') || lpszTricsData[1] == OLECHAR('s')) &&
					(lpszTricsData[2] == OLECHAR('H') || lpszTricsData[2] == OLECHAR('h')) &&
					(lpszTricsData[3] == OLECHAR('T') || lpszTricsData[3] == OLECHAR('t')) &&
					(lpszTricsData[4] == OLECHAR('M') || lpszTricsData[4] == OLECHAR('m')) &&
					(lpszTricsData[5] == OLECHAR('L') || lpszTricsData[5] == OLECHAR('l')) &&
					(lpszTricsData[6] == OLECHAR(':')))
				{
					// Just HTML: load the HTML data into the document
					UINT nCreateSize = (static_cast<UINT>(ocslen(lpszTricsData)) - 7) * sizeof(OLECHAR);
					HGLOBAL hGlobal = GlobalAlloc(GHND, nCreateSize);
					if (hGlobal)
					{
						CComPtr<IStream> spStream;
						BYTE* pBytes = (BYTE*) GlobalLock(hGlobal);
						Checked::memcpy_s(pBytes, nCreateSize, lpszTricsData + 7, nCreateSize);
						GlobalUnlock(hGlobal);
						hr = CreateStreamOnHGlobal(hGlobal, TRUE, &spStream);
						if (SUCCEEDED(hr))
						{
							CComPtr<IPersistStreamInit> spPSI;
							hr = spUnk->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spPSI);
							if (SUCCEEDED(hr))
							{
								hr = spPSI->Load(spStream);
							}
						}
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				else
				{
					CComPtr<IWebBrowser2> spBrowser;
					spUnk->QueryInterface(__uuidof(IWebBrowser2), (void**)&spBrowser);
					if (spBrowser)
					{
						CComVariant ve;
						CComVariant vurl(lpszTricsData);
						spBrowser->put_Visible(ATL_VARIANT_TRUE);
						spBrowser->Navigate2(&vurl, &ve, &ve, &ve, &ve);
					}
				}

			}
			if (FAILED(hr) || m_spUnknown == NULL)
			{
				// We don't have a control or something failed so release
				ReleaseAll();

				if (m_hWnd != NULL)
				{
					RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
					if (FAILED(hr) && bReleaseWindowOnFailure) // We subclassed the window in an attempt to create this control, so we unsubclass on failure
					{
						ReleaseWindow();
					}
				}
			}
		}
		return hr;
	}
ATLPREFAST_UNSUPPRESS()

#ifndef _ATL_NO_DOCHOSTUIHANDLER
	// IDocHostUIHandler
	// MSHTML requests to display its context menu
	STDMETHOD(ShowContextMenu)(
		_In_ DWORD dwID,
		_In_ POINT* pptPosition,
		_In_ IUnknown* pCommandTarget,
		_In_ IDispatch* pDispatchObjectHit)
	{
		HRESULT hr = m_bAllowContextMenu ? S_FALSE : S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			m_spIDocHostUIHandlerDispatch->ShowContextMenu(
				dwID,
				pptPosition->x,
				pptPosition->y,
				pCommandTarget,
				pDispatchObjectHit,
				&hr);
		return hr;
	}
	// Called at initialization to find UI styles from container
	STDMETHOD(GetHostInfo)(_Inout_ DOCHOSTUIINFO* pInfo)
	{
		if (pInfo == NULL)
			return E_POINTER;

		if (m_spIDocHostUIHandlerDispatch != NULL)
			return m_spIDocHostUIHandlerDispatch->GetHostInfo(&pInfo->dwFlags, &pInfo->dwDoubleClick);

		pInfo->dwFlags = m_dwDocHostFlags;
		pInfo->dwDoubleClick = m_dwDocHostDoubleClickFlags;

		return S_OK;
	}
	// Allows the host to replace the IE4/MSHTML menus and toolbars.
	STDMETHOD(ShowUI)(
		_In_ DWORD dwID,
		_In_ IOleInPlaceActiveObject* pActiveObject,
		_In_ IOleCommandTarget* pCommandTarget,
		_In_ IOleInPlaceFrame* pFrame,
		_In_ IOleInPlaceUIWindow* pDoc)
	{
		HRESULT hr = m_bAllowShowUI ? S_FALSE : S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			m_spIDocHostUIHandlerDispatch->ShowUI(
				dwID,
				pActiveObject,
				pCommandTarget,
				pFrame,
				pDoc,
				&hr);
		return hr;
	}
	// Called when IE4/MSHTML removes its menus and toolbars.
	STDMETHOD(HideUI)()
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->HideUI();
		return hr;
	}
	// Notifies the host that the command state has changed.
	STDMETHOD(UpdateUI)()
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->UpdateUI();
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::EnableModeless
	STDMETHOD(EnableModeless)(BOOL fEnable)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->EnableModeless(fEnable ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE);
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::OnDocWindowActivate
	STDMETHOD(OnDocWindowActivate)(BOOL fActivate)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->OnDocWindowActivate(fActivate ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE);
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::OnFrameWindowActivate.
	STDMETHOD(OnFrameWindowActivate)(BOOL fActivate)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->OnFrameWindowActivate(fActivate ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE);
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::ResizeBorder.
	STDMETHOD(ResizeBorder)(
		_In_ LPCRECT prcBorder,
		_In_ IOleInPlaceUIWindow* pUIWindow,
		_In_ BOOL fFrameWindow)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->ResizeBorder(
				prcBorder->left,
				prcBorder->top,
				prcBorder->right,
				prcBorder->bottom,
				pUIWindow,
				fFrameWindow ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE);
		return hr;
	}
	// Called by IE4/MSHTML when IOleInPlaceActiveObject::TranslateAccelerator or IOleControlSite::TranslateAccelerator is called.
	STDMETHOD(TranslateAccelerator)(
		_In_ LPMSG lpMsg,
		_In_ const GUID* pguidCmdGroup,
		_In_ DWORD nCmdID)
	{
		HRESULT hr = S_FALSE;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			m_spIDocHostUIHandlerDispatch->TranslateAccelerator(
				(DWORD_PTR) lpMsg->hwnd,
				lpMsg->message,
				lpMsg->wParam,
				lpMsg->lParam,
				CComBSTR(*pguidCmdGroup),
				nCmdID,
				&hr);
		return hr;
	}
	// Returns the registry key under which IE4/MSHTML stores user preferences.
	// Returns S_OK if successful, or S_FALSE otherwise. If S_FALSE, IE4/MSHTML will default to its own user options.
	STDMETHOD(GetOptionKeyPath)(
		_Outptr_result_maybenull_z_ LPOLESTR* pchKey,
		DWORD dwReserved)
	{
		HRESULT hr = S_FALSE;
		ATLENSURE_RETURN_VAL(pchKey, E_POINTER);
		*pchKey = NULL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			hr = m_spIDocHostUIHandlerDispatch->GetOptionKeyPath(pchKey, dwReserved);
			if (FAILED(hr) || *pchKey == NULL)
				hr = S_FALSE;
		}
		else
		{
			if (m_bstrOptionKeyPath.Length() != 0)
			{
				int cchLength = ocslen(m_bstrOptionKeyPath) + 1;
				LPOLESTR pStr = (LPOLESTR)CoTaskMemAlloc(cchLength * sizeof(OLECHAR));
				if (pStr == NULL)
				{
					return E_OUTOFMEMORY;
				}
				if(!ocscpy_s(pStr, cchLength, m_bstrOptionKeyPath.m_str))
				{
					CoTaskMemFree(pStr);
					return E_FAIL;
				}
				*pchKey = pStr;
				hr = S_OK;
			}
		}
		return hr;
	}

ATLPREFAST_SUPPRESS(6387)
	// Called by IE4/MSHTML when it is being used as a drop target to allow the host to supply an alternative IDropTarget
	STDMETHOD(GetDropTarget)(
		_In_ IDropTarget* pDropTarget,
		_Outptr_ IDropTarget** ppDropTarget)
	{
		ATLASSERT(ppDropTarget != NULL);
		if (ppDropTarget == NULL)
			return E_POINTER;
		*ppDropTarget = NULL;

		HRESULT hr = E_NOTIMPL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			CComPtr<IUnknown> spUnk;
			hr = m_spIDocHostUIHandlerDispatch->GetDropTarget(pDropTarget, &spUnk);
			if (SUCCEEDED(hr) && spUnk)
			{
				hr = spUnk->QueryInterface(__uuidof(IDropTarget), (void**)ppDropTarget);
				if (FAILED(hr) || *ppDropTarget == NULL)
					hr = E_NOTIMPL;
			}
			else
			{
				hr = E_NOTIMPL;
			}
		}
		return hr;
	}
ATLPREFAST_UNSUPPRESS()

ATLPREFAST_SUPPRESS(6387)
	// Called by IE4/MSHTML to obtain the host's IDispatch interface
	STDMETHOD(GetExternal)(_Outptr_result_maybenull_ IDispatch** ppDispatch)
	{
		ATLASSERT(ppDispatch != NULL);
		if (ppDispatch == NULL)
			return E_POINTER;
		*ppDispatch = NULL;

		HRESULT hr = E_NOINTERFACE;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			hr = m_spIDocHostUIHandlerDispatch->GetExternal(ppDispatch);
			if (FAILED(hr) || *ppDispatch == NULL)
				hr = E_NOINTERFACE;
		}
		else
		{
			// return the IDispatch we have for extending the object Model
			if (ppDispatch != NULL)
			{
				hr=m_spExternalDispatch.CopyTo(ppDispatch);
			}
			else
				hr = E_POINTER;
		}
		return hr;
	}
ATLPREFAST_UNSUPPRESS()

	// Called by IE4/MSHTML to allow the host an opportunity to modify the URL to be loaded
	STDMETHOD(TranslateUrl)(
		DWORD dwTranslate,
		_In_z_ OLECHAR* pchURLIn,
		_Outptr_result_maybenull_z_ OLECHAR** ppchURLOut)
	{
		ATLENSURE_RETURN_VAL(ppchURLOut, E_POINTER);
		*ppchURLOut = NULL;

		HRESULT hr = S_FALSE;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			CComBSTR bstrURLOut;
			hr = m_spIDocHostUIHandlerDispatch->TranslateUrl(dwTranslate, CComBSTR(pchURLIn), &bstrURLOut);
			if (SUCCEEDED(hr) && bstrURLOut.Length() != 0)
			{
				UINT nLen = (bstrURLOut.Length() + 1) * 2;
				*ppchURLOut = (OLECHAR*) CoTaskMemAlloc(nLen);
				if (*ppchURLOut == NULL)
                {
					return E_OUTOFMEMORY;
                }
				Checked::memcpy_s(*ppchURLOut, nLen, bstrURLOut.m_str, nLen);
			}
			else
            {
				hr = S_FALSE;
            }
		}
		return hr;
	}
	// Called on the host by IE4/MSHTML to allow the host to replace IE4/MSHTML's data object.
	// This allows the host to block certain clipboard formats or support additional clipboard formats.
	STDMETHOD(FilterDataObject)(
		_Inout_ IDataObject* pDO,
		_Outptr_result_maybenull_ IDataObject** ppDORet)
	{
		ATLASSERT(ppDORet != NULL);
		if (ppDORet == NULL)
			return E_POINTER;
		*ppDORet = NULL;

		HRESULT hr = S_FALSE;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			CComPtr<IUnknown> spUnk;
			hr = m_spIDocHostUIHandlerDispatch->FilterDataObject(pDO, &spUnk);
			if (spUnk)
				hr = QueryInterface(__uuidof(IDataObject), (void**)ppDORet);
			if (FAILED(hr) || *ppDORet == NULL)
				hr = S_FALSE;
		}
		return hr;
	}
#endif

	HRESULT FireAmbientPropertyChange(_In_ DISPID dispChanged)
	{
		HRESULT hr = S_OK;
		CComQIPtr<IOleControl, &__uuidof(IOleControl)> spOleControl(m_spUnknown);
		if (spOleControl != NULL)
			hr = spOleControl->OnAmbientPropertyChange(dispChanged);
		return hr;
	}

	// IAxWinAmbientDispatch
	CComPtr<IDispatch> m_spAmbientDispatch;

	STDMETHOD(Invoke)(
		_In_ DISPID dispIdMember,
		_In_ REFIID riid,
		_In_ LCID lcid,
		_In_ WORD wFlags,
		_In_ DISPPARAMS *pDispParams,
		_Out_opt_ VARIANT *pVarResult,
		_Out_opt_ EXCEPINFO *pExcepInfo,
		_Out_opt_ UINT *puArgErr)
	{
		HRESULT hr = IDispatchImpl<IAxWinAmbientDispatchEx, &__uuidof(IAxWinAmbientDispatchEx), &CAtlModule::m_libid, 0xFFFF, 0xFFFF>::Invoke
			(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
		if ((hr == DISP_E_MEMBERNOTFOUND || hr == TYPE_E_ELEMENTNOTFOUND) && m_spAmbientDispatch != NULL)
		{
			hr = m_spAmbientDispatch->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
			if (SUCCEEDED(hr) && (wFlags & DISPATCH_PROPERTYPUT) != 0)
			{
				hr = FireAmbientPropertyChange(dispIdMember);
			}
		}
		return hr;
	}

	STDMETHOD(put_AllowWindowlessActivation)(_In_ VARIANT_BOOL bAllowWindowless)
	{
		m_bCanWindowlessActivate = bAllowWindowless;
		return S_OK;
	}
	STDMETHOD(get_AllowWindowlessActivation)(_Out_ VARIANT_BOOL* pbAllowWindowless)
	{
		ATLASSERT(pbAllowWindowless != NULL);
		if (pbAllowWindowless == NULL)
			return E_POINTER;

		*pbAllowWindowless = m_bCanWindowlessActivate ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_BackColor)(_In_ OLE_COLOR clrBackground)
	{
		m_clrBackground = clrBackground;
		FireAmbientPropertyChange(DISPID_AMBIENT_BACKCOLOR);
		InvalidateRect(0, FALSE);
		return S_OK;
	}
	STDMETHOD(get_BackColor)(_Out_ OLE_COLOR* pclrBackground)
	{
		ATLASSERT(pclrBackground != NULL);
		if (pclrBackground == NULL)
			return E_POINTER;

		*pclrBackground = m_clrBackground;
		return S_OK;
	}
	STDMETHOD(put_ForeColor)(_In_ OLE_COLOR clrForeground)
	{
		m_clrForeground = clrForeground;
		FireAmbientPropertyChange(DISPID_AMBIENT_FORECOLOR);
		return S_OK;
	}
	STDMETHOD(get_ForeColor)(_Out_ OLE_COLOR* pclrForeground)
	{
		ATLASSERT(pclrForeground != NULL);
		if (pclrForeground == NULL)
			return E_POINTER;

		*pclrForeground = m_clrForeground;
		return S_OK;
	}
	STDMETHOD(put_LocaleID)(_In_ LCID lcidLocaleID)
	{
		m_lcidLocaleID = lcidLocaleID;
		FireAmbientPropertyChange(DISPID_AMBIENT_LOCALEID);
		return S_OK;
	}
	STDMETHOD(get_LocaleID)(_Out_ LCID* plcidLocaleID)
	{
		ATLASSERT(plcidLocaleID != NULL);
		if (plcidLocaleID == NULL)
			return E_POINTER;

		*plcidLocaleID = m_lcidLocaleID;
		return S_OK;
	}
	STDMETHOD(put_UserMode)(_In_ VARIANT_BOOL bUserMode)
	{
		m_bUserMode = bUserMode;
		FireAmbientPropertyChange(DISPID_AMBIENT_USERMODE);
		return S_OK;
	}
	STDMETHOD(get_UserMode)(_Out_ VARIANT_BOOL* pbUserMode)
	{
		ATLASSERT(pbUserMode != NULL);
		if (pbUserMode == NULL)
			return E_POINTER;

		*pbUserMode = m_bUserMode ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_DisplayAsDefault)(_In_ VARIANT_BOOL bDisplayAsDefault)
	{
		m_bDisplayAsDefault = bDisplayAsDefault;
		FireAmbientPropertyChange(DISPID_AMBIENT_DISPLAYASDEFAULT);
		return S_OK;
	}
	STDMETHOD(get_DisplayAsDefault)(_Out_ VARIANT_BOOL* pbDisplayAsDefault)
	{
		ATLASSERT(pbDisplayAsDefault != NULL);
		if (pbDisplayAsDefault == NULL)
			return E_POINTER;

		*pbDisplayAsDefault = m_bDisplayAsDefault ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_Font)(_In_ IFontDisp* pFont)
	{
		m_spFont = pFont;
		FireAmbientPropertyChange(DISPID_AMBIENT_FONT);
		return S_OK;
	}
	STDMETHOD(get_Font)(_Outptr_result_maybenull_ IFontDisp** pFont)
	{
		ATLASSERT(pFont != NULL);
		if (pFont == NULL)
			return E_POINTER;
		*pFont = NULL;

		if (m_spFont == NULL)
		{
			USES_CONVERSION_EX;
			HFONT hSystemFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
			if (hSystemFont == NULL)
				hSystemFont = (HFONT) GetStockObject(SYSTEM_FONT);
			if (hSystemFont == NULL)
				return AtlHresultFromLastError();
			LOGFONT logfont;
			GetObject(hSystemFont, sizeof(logfont), &logfont);
			FONTDESC fd;
			fd.cbSizeofstruct = sizeof(FONTDESC);
			fd.lpstrName = T2OLE_EX_DEF(logfont.lfFaceName);
			fd.sWeight = (short)logfont.lfWeight;
			fd.sCharset = logfont.lfCharSet;
			fd.fItalic = logfont.lfItalic;
			fd.fUnderline = logfont.lfUnderline;
			fd.fStrikethrough = logfont.lfStrikeOut;

			long lfHeight = logfont.lfHeight;
			if (lfHeight < 0)
				lfHeight = -lfHeight;

			int ppi;
			HDC hdc;
			if (m_hWnd)
			{
				hdc = ::GetDC(m_hWnd);
				if (hdc == NULL)
					return AtlHresultFromLastError();
				ppi = GetDeviceCaps(hdc, LOGPIXELSY);
				::ReleaseDC(m_hWnd, hdc);
			}
			else
			{
				hdc = ::GetDC(GetDesktopWindow());
				if (hdc == NULL)
					return AtlHresultFromLastError();
				ppi = GetDeviceCaps(hdc, LOGPIXELSY);
				::ReleaseDC(GetDesktopWindow(), hdc);
			}
			fd.cySize.Lo = lfHeight * 720000 / ppi;
			fd.cySize.Hi = 0;

			OleCreateFontIndirect(&fd, __uuidof(IFontDisp), (void**) &m_spFont);
		}

		return m_spFont.CopyTo(pFont);
	}
	STDMETHOD(put_MessageReflect)(_In_ VARIANT_BOOL bMessageReflect)
	{
		m_bMessageReflect = bMessageReflect;
		FireAmbientPropertyChange(DISPID_AMBIENT_MESSAGEREFLECT);
		return S_OK;
	}
	STDMETHOD(get_MessageReflect)(_Out_ VARIANT_BOOL* pbMessageReflect)
	{
		ATLASSERT(pbMessageReflect != NULL);
		if (pbMessageReflect == NULL)
			return E_POINTER;

		*pbMessageReflect = m_bMessageReflect ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(get_ShowGrabHandles)(_Out_ VARIANT_BOOL* pbShowGrabHandles)
	{
		*pbShowGrabHandles = ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(get_ShowHatching)(_Out_ VARIANT_BOOL* pbShowHatching)
	{
		ATLASSERT(pbShowHatching != NULL);
		if (pbShowHatching == NULL)
			return E_POINTER;

		*pbShowHatching = ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_DocHostFlags)(_In_ DWORD dwDocHostFlags)
	{
		m_dwDocHostFlags = dwDocHostFlags;
		FireAmbientPropertyChange(DISPID_UNKNOWN);
		return S_OK;
	}
	STDMETHOD(get_DocHostFlags)(_Out_ DWORD* pdwDocHostFlags)
	{
		ATLASSERT(pdwDocHostFlags != NULL);
		if (pdwDocHostFlags == NULL)
			return E_POINTER;

		*pdwDocHostFlags = m_dwDocHostFlags;
		return S_OK;
	}
	STDMETHOD(put_DocHostDoubleClickFlags)(_In_ DWORD dwDocHostDoubleClickFlags)
	{
		m_dwDocHostDoubleClickFlags = dwDocHostDoubleClickFlags;
		return S_OK;
	}
	STDMETHOD(get_DocHostDoubleClickFlags)(_Out_ DWORD* pdwDocHostDoubleClickFlags)
	{
		ATLASSERT(pdwDocHostDoubleClickFlags != NULL);
		if (pdwDocHostDoubleClickFlags == NULL)
			return E_POINTER;

		*pdwDocHostDoubleClickFlags = m_dwDocHostDoubleClickFlags;
		return S_OK;
	}
	STDMETHOD(put_AllowContextMenu)(_In_ VARIANT_BOOL bAllowContextMenu)
	{
		m_bAllowContextMenu = bAllowContextMenu;
		return S_OK;
	}
	STDMETHOD(get_AllowContextMenu)(_Out_ VARIANT_BOOL* pbAllowContextMenu)
	{
		ATLASSERT(pbAllowContextMenu != NULL);
		if (pbAllowContextMenu == NULL)
			return E_POINTER;

		*pbAllowContextMenu = m_bAllowContextMenu ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_AllowShowUI)(_In_ VARIANT_BOOL bAllowShowUI)
	{
		m_bAllowShowUI = bAllowShowUI;
		return S_OK;
	}
	STDMETHOD(get_AllowShowUI)(_Out_ VARIANT_BOOL* pbAllowShowUI)
	{
		ATLASSERT(pbAllowShowUI != NULL);
		if (pbAllowShowUI == NULL)
			return E_POINTER;

		*pbAllowShowUI = m_bAllowShowUI ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_OptionKeyPath)(_In_z_ BSTR bstrOptionKeyPath) throw()
	{
		m_bstrOptionKeyPath = bstrOptionKeyPath;
		return S_OK;
	}
	STDMETHOD(get_OptionKeyPath)(_Outptr_result_maybenull_z_ BSTR* pbstrOptionKeyPath)
	{
		ATLASSERT(pbstrOptionKeyPath != NULL);
		if (pbstrOptionKeyPath == NULL)
			return E_POINTER;

		*pbstrOptionKeyPath = SysAllocString(m_bstrOptionKeyPath);
		return S_OK;
	}

	STDMETHOD(SetAmbientDispatch)(_In_ IDispatch* pDispatch)
	{
		m_spAmbientDispatch = pDispatch;
		return S_OK;
	}

	// IObjectWithSite
	STDMETHOD(SetSite)(_Inout_opt_ IUnknown* pUnkSite)
	{
		HRESULT hr = IObjectWithSiteImpl<CAxHostWindow>::SetSite(pUnkSite);

		if (SUCCEEDED(hr) && m_spUnkSite)
		{
			// Look for "outer" IServiceProvider
			hr = m_spUnkSite->QueryInterface(__uuidof(IServiceProvider), (void**)&m_spServices);
			ATLASSERT( SUCCEEDED(hr) && "No ServiceProvider!" );
		}

		if (pUnkSite == NULL)
			m_spServices.Release();

		return hr;
	}

	// IOleClientSite
	STDMETHOD(SaveObject)()
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::SaveObject"));
	}
	STDMETHOD(GetMoniker)(
		DWORD /*dwAssign*/,
		DWORD /*dwWhichMoniker*/,
		_In_opt_ IMoniker** /*ppmk*/)
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::GetMoniker"));
	}
	STDMETHOD(GetContainer)(_Outptr_ IOleContainer** ppContainer)
	{
		ATLTRACE(atlTraceHosting, 2, _T("IOleClientSite::GetContainer\n"));
		ATLASSERT(ppContainer != NULL);

		HRESULT hr = E_POINTER;
		if (ppContainer)
		{
			hr = E_NOTIMPL;
			(*ppContainer) = NULL;
			if (m_spUnkSite)
				hr = m_spUnkSite->QueryInterface(__uuidof(IOleContainer), (void**)ppContainer);
			if (FAILED(hr))
				hr = QueryInterface(__uuidof(IOleContainer), (void**)ppContainer);
		}
		return hr;
	}
	STDMETHOD(ShowObject)()
	{
		ATLTRACE(atlTraceHosting, 2, _T("IOleClientSite::ShowObject\r\n"));

		HDC hdc = CWindowImpl<CAxHostWindow>::GetDC();
		if (hdc == NULL)
			return E_FAIL;
		if (m_spViewObject)
			m_spViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, (RECTL*)&m_rcPos, (RECTL*)&m_rcPos, NULL, 0);
		CWindowImpl<CAxHostWindow>::ReleaseDC(hdc);
		return S_OK;
	}
	STDMETHOD(OnShowWindow)(BOOL /*fShow*/)
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::OnShowWindow"));
	}
	STDMETHOD(RequestNewObjectLayout)()
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::RequestNewObjectLayout"));
	}

	// IOleInPlaceSite
	STDMETHOD(GetWindow)(_Out_ HWND* phwnd)
	{
		ATLENSURE_RETURN(phwnd);
		*phwnd = m_hWnd;
		return S_OK;
	}
	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::ContextSensitiveHelp"));
	}
	STDMETHOD(CanInPlaceActivate)()
	{
		return S_OK;
	}
	STDMETHOD(OnInPlaceActivate)()
	{
		// should only be called once the first time control is inplace-activated
		ATLASSUME(m_bInPlaceActive == FALSE);
		ATLASSUME(m_spInPlaceObjectWindowless == NULL);

		m_bInPlaceActive = TRUE;
		OleLockRunning(m_spOleObject, TRUE, FALSE);
		m_bWindowless = FALSE;
		m_spOleObject->QueryInterface(__uuidof(IOleInPlaceObject), (void**) &m_spInPlaceObjectWindowless);
		return S_OK;
	}
	STDMETHOD(OnUIActivate)()
	{
		ATLTRACE(atlTraceHosting, 2, _T("IOleInPlaceSite::OnUIActivate\n"));
		m_bUIActive = TRUE;
		return S_OK;
	}
ATLPREFAST_SUPPRESS(6387)
	STDMETHOD(GetWindowContext)(
		_Outptr_opt_result_maybenull_ IOleInPlaceFrame** ppFrame,
		_Outptr_opt_result_maybenull_ IOleInPlaceUIWindow** ppDoc,
		_Out_ LPRECT lprcPosRect,
		_Out_ LPRECT lprcClipRect,
		_Inout_ LPOLEINPLACEFRAMEINFO pFrameInfo)
	{
		if (ppFrame != NULL)
			*ppFrame = NULL;
		if (ppDoc != NULL)
			*ppDoc = NULL;
		if (ppFrame == NULL || ppDoc == NULL || lprcPosRect == NULL || lprcClipRect == NULL)
		{
			ATLASSERT(false);
			return E_POINTER;
		}

		if (!m_spInPlaceFrame)
		{
			CComObject<CAxFrameWindow>* pFrameWindow;
			HRESULT hRet = CComObject<CAxFrameWindow>::CreateInstance(&pFrameWindow);

			if(FAILED(hRet))
			{
				return hRet;
			}

			pFrameWindow->QueryInterface(__uuidof(IOleInPlaceFrame), (void**) &m_spInPlaceFrame);
			ATLASSUME(m_spInPlaceFrame);
		}
		if (!m_spInPlaceUIWindow)
		{
			CComObject<CAxUIWindow>* pUIWindow;
			HRESULT hRet = CComObject<CAxUIWindow>::CreateInstance(&pUIWindow);

			if(FAILED(hRet))
			{
				return hRet;
			}

			pUIWindow->QueryInterface(__uuidof(IOleInPlaceUIWindow), (void**) &m_spInPlaceUIWindow);
			ATLASSUME(m_spInPlaceUIWindow);
		}
		HRESULT hr=S_OK;
		hr=m_spInPlaceFrame.CopyTo(ppFrame);
		if(FAILED(hr))
		{
			return hr;
		}
		hr=m_spInPlaceUIWindow.CopyTo(ppDoc);
		if(FAILED(hr))
		{
			return hr;
		}
		GetClientRect(lprcPosRect);
		GetClientRect(lprcClipRect);

		if (m_hAccel == NULL)
		{
			ACCEL ac = { 0,0,0 };
			m_hAccel = CreateAcceleratorTable(&ac, 1);
		}
		pFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
		pFrameInfo->fMDIApp = m_bMDIApp;
		pFrameInfo->hwndFrame = GetParent();
		pFrameInfo->haccel = m_hAccel;
		pFrameInfo->cAccelEntries = (m_hAccel != NULL) ? 1 : 0;

		return hr;
	}
ATLPREFAST_UNSUPPRESS()

	STDMETHOD(Scroll)(SIZE /*scrollExtant*/)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::Scroll"));
	}
	STDMETHOD(OnUIDeactivate)(BOOL /*fUndoable*/)
	{
		ATLTRACE(atlTraceHosting, 2, _T("IOleInPlaceSite::OnUIDeactivate\n"));
		m_bUIActive = FALSE;
		return S_OK;
	}
	STDMETHOD(OnInPlaceDeactivate)()
	{
		m_bInPlaceActive = FALSE;
		m_spInPlaceObjectWindowless.Release();
		return S_OK;
	}
	STDMETHOD(DiscardUndoState)()
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::DiscardUndoState"));
	}
	STDMETHOD(DeactivateAndUndo)()
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::DeactivateAndUndo"));
	}
	STDMETHOD(OnPosRectChange)(_In_ LPCRECT lprcPosRect)
	{
		ATLTRACE2(atlTraceHosting, 0, 	_T("IOleInPlaceSite::OnPosRectChange"));
		if (lprcPosRect==NULL) { return E_POINTER; }

		// Use MoveWindow() to resize the CAxHostWindow.
		// The CAxHostWindow handler for OnSize() will
		// take care of calling IOleInPlaceObject::SetObjectRects().

		// Convert to parent window coordinates for MoveWindow().
		RECT rect = *lprcPosRect;
		ClientToScreen( &rect );
		HWND hWnd = GetParent();

		// Check to make sure it's a non-top-level window.
		if(hWnd != NULL)
		{
			CWindow wndParent(hWnd);
			wndParent.ScreenToClient(&rect);
			wndParent.Detach ();
		}
		// Do the actual move.
		MoveWindow( &rect);

		return S_OK;
	}

// IOleInPlaceSiteEx
	STDMETHOD(OnInPlaceActivateEx)(
		_In_opt_ BOOL* /*pfNoRedraw*/,
		DWORD dwFlags)
	{
		// should only be called once the first time control is inplace-activated
		ATLASSUME(m_bInPlaceActive == FALSE);
		ATLASSUME(m_spInPlaceObjectWindowless == NULL);

		m_bInPlaceActive = TRUE;
		OleLockRunning(m_spOleObject, TRUE, FALSE);
		HRESULT hr = E_FAIL;
		if (dwFlags & ACTIVATE_WINDOWLESS)
		{
			m_bWindowless = TRUE;
			hr = m_spOleObject->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (void**) &m_spInPlaceObjectWindowless);
		}
		if (FAILED(hr))
		{
			m_bWindowless = FALSE;
			hr = m_spOleObject->QueryInterface(__uuidof(IOleInPlaceObject), (void**) &m_spInPlaceObjectWindowless);
		}
		if (m_spInPlaceObjectWindowless)
			m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
		return S_OK;
	}
	STDMETHOD(OnInPlaceDeactivateEx)(BOOL /*fNoRedraw*/)
	{
		m_bInPlaceActive = FALSE;
		m_spInPlaceObjectWindowless.Release();
		return S_OK;
	}
	STDMETHOD(RequestUIActivate)()
	{
		return S_OK;
	}

// IOleInPlaceSiteWindowless
	HDC m_hDCScreen;
	bool m_bDCReleased;

	STDMETHOD(CanWindowlessActivate)()
	{
		return m_bCanWindowlessActivate ? S_OK : S_FALSE;
	}
	STDMETHOD(GetCapture)()
	{
		return m_bCapture ? S_OK : S_FALSE;
	}
	STDMETHOD(SetCapture)(_In_ BOOL fCapture)
	{
		if (fCapture)
		{
			CWindow::SetCapture();
			m_bCapture = TRUE;
		}
		else
		{
			ReleaseCapture();
			m_bCapture = FALSE;
		}
		return S_OK;
	}
	STDMETHOD(GetFocus)()
	{
		return m_bHaveFocus ? S_OK : S_FALSE;
	}
	STDMETHOD(SetFocus)(_In_ BOOL fGotFocus)
	{
		m_bHaveFocus = fGotFocus;
		return S_OK;
	}
	STDMETHOD(GetDC)(
		_In_opt_ LPCRECT /*pRect*/,
		_In_ DWORD grfFlags,
		_Out_ HDC* phDC)
	{
		if (phDC == NULL)
			return E_POINTER;
		if (!m_bDCReleased)
			return E_FAIL;

		*phDC = CWindowImpl<CAxHostWindow>::GetDC();
		if (*phDC == NULL)
			return E_FAIL;

		m_bDCReleased = false;

		if (grfFlags & OLEDC_NODRAW)
			return S_OK;

		RECT rect;
		GetClientRect(&rect);
		if (grfFlags & OLEDC_OFFSCREEN)
		{
			HDC hDCOffscreen = CreateCompatibleDC(*phDC);
			if (hDCOffscreen != NULL)
			{
				HBITMAP hBitmap = CreateCompatibleBitmap(*phDC, rect.right - rect.left, rect.bottom - rect.top);
				if (hBitmap == NULL)
					DeleteDC(hDCOffscreen);
				else
				{
					HGDIOBJ hOldBitmap = SelectObject(hDCOffscreen, hBitmap);
					if (hOldBitmap == NULL)
					{
						DeleteObject(hBitmap);
						DeleteDC(hDCOffscreen);
					}
					else
					{
						DeleteObject(hOldBitmap);
						m_hDCScreen = *phDC;
						*phDC = hDCOffscreen;
					}
				}
			}
		}

		if (grfFlags & OLEDC_PAINTBKGND)
			::FillRect(*phDC, &rect, (HBRUSH) (COLOR_WINDOW+1));
		return S_OK;
	}
	STDMETHOD(ReleaseDC)(_Inout_ HDC hDC)
	{
		m_bDCReleased = true;
		if (m_hDCScreen != NULL)
		{
			RECT rect;
			GetClientRect(&rect);
			// Offscreen DC has to be copied to screen DC before releasing the screen dc;
			BitBlt(m_hDCScreen, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hDC, 0, 0, SRCCOPY);
			DeleteDC(hDC);
			hDC = m_hDCScreen;
		}

		CWindowImpl<CAxHostWindow>::ReleaseDC(hDC);
		return S_OK;
	}
	STDMETHOD(InvalidateRect)(
		_In_opt_ LPCRECT pRect,
		_In_ BOOL fErase)
	{
		CWindowImpl<CAxHostWindow>::InvalidateRect(pRect, fErase);
		return S_OK;
	}
	STDMETHOD(InvalidateRgn)(
		_In_ HRGN hRGN,
		_In_ BOOL fErase)
	{
		CWindowImpl<CAxHostWindow>::InvalidateRgn(hRGN, fErase);
		return S_OK;
	}
	STDMETHOD(ScrollRect)(
		INT /*dx*/,
		INT /*dy*/,
		_In_ LPCRECT /*pRectScroll*/,
		_In_ LPCRECT /*pRectClip*/)
	{
		return S_OK;
	}
	STDMETHOD(AdjustRect)(_In_ LPRECT /*prc*/)
	{
		return S_OK;
	}
	STDMETHOD(OnDefWindowMessage)(
		_In_ UINT msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam,
		_Out_ LRESULT* plResult)
	{
		*plResult = DefWindowProc(msg, wParam, lParam);
		return S_OK;
	}

	// IOleControlSite
	STDMETHOD(OnControlInfoChanged)()
	{
		return S_OK;
	}
	STDMETHOD(LockInPlaceActive)(BOOL /*fLock*/)
	{
		return S_OK;
	}
	STDMETHOD(GetExtendedControl)(_Outptr_ IDispatch** ppDisp)
	{
		if (ppDisp == NULL)
			return E_POINTER;
		return m_spOleObject.QueryInterface(ppDisp);
	}
	STDMETHOD(TransformCoords)(
		_Inout_ POINTL* /*pPtlHimetric*/,
		_Inout_ POINTF* /*pPtfContainer*/,
		DWORD /*dwFlags*/)
	{
		ATLTRACENOTIMPL(_T("CAxHostWindow::TransformCoords"));
	}
	STDMETHOD(TranslateAccelerator)(
		_In_ LPMSG /*lpMsg*/,
		DWORD /*grfModifiers*/)
	{
		return S_FALSE;
	}
	STDMETHOD(OnFocus)(BOOL fGotFocus)
	{
		m_bHaveFocus = fGotFocus;
		return S_OK;
	}
	STDMETHOD(ShowPropertyFrame)()
	{
		ATLTRACENOTIMPL(_T("CAxHostWindow::ShowPropertyFrame"));
	}

	// IAdviseSink
	STDMETHOD_(void, OnDataChange)(
		_In_ FORMATETC* /* pFormatetc */,
		_In_ STGMEDIUM* /* pStgmed */)
	{
	}
	STDMETHOD_(void, OnViewChange)(
		DWORD /*dwAspect*/,
		LONG /*lindex*/)
	{
	}
	STDMETHOD_(void, OnRename)(_In_opt_ IMoniker* /* pmk */)
	{
	}
	STDMETHOD_(void, OnSave)()
	{
	}
	STDMETHOD_(void, OnClose)()
	{
	}

	// IOleContainer
	STDMETHOD(ParseDisplayName)(
		_In_opt_ IBindCtx* /*pbc*/,
		_In_opt_z_ LPOLESTR /*pszDisplayName*/,
		_In_opt_ ULONG* /*pchEaten*/,
		_In_opt_ IMoniker** /*ppmkOut*/)
	{
		ATLTRACENOTIMPL(_T("CAxHostWindow::ParseDisplayName"));
	}

ATLPREFAST_SUPPRESS(6387)
	STDMETHOD(EnumObjects)(
		DWORD /*grfFlags*/,
		_Outptr_ IEnumUnknown** ppenum)
	{
		if (ppenum == NULL)
			return E_POINTER;
		*ppenum = NULL;
		typedef CComObject<CComEnum<IEnumUnknown, &__uuidof(IEnumUnknown), IUnknown*, _CopyInterface<IUnknown> > > enumunk;
		enumunk* p = NULL;

ATLPREFAST_SUPPRESS(6014)
		/* prefast noise VSW 489981 */
		p = _ATL_NEW enumunk;
ATLPREFAST_UNSUPPRESS()

		if(p == NULL)
			return E_OUTOFMEMORY;
		IUnknown* pTemp = m_spUnknown;
		// There is always only one object.
		HRESULT hRes = p->Init(reinterpret_cast<IUnknown**>(&pTemp), reinterpret_cast<IUnknown**>(&pTemp + 1), GetControllingUnknown(), AtlFlagCopy);
		if (SUCCEEDED(hRes))
			hRes = p->QueryInterface(__uuidof(IEnumUnknown), (void**)ppenum);
		if (FAILED(hRes))
			delete p;
		return hRes;
	}
ATLPREFAST_UNSUPPRESS()

	STDMETHOD(LockContainer)(BOOL fLock)
	{
		m_bLocked = fLock;
		return S_OK;
	}

	HRESULT ActivateAx(
		_Inout_opt_ IUnknown* pUnkControl,
		_In_ bool bInited,
		_Inout_opt_ IStream* pStream)
	{
		if (pUnkControl == NULL)
			return S_OK;

		m_spUnknown = pUnkControl;

		HRESULT hr = S_OK;
		pUnkControl->QueryInterface(__uuidof(IOleObject), (void**)&m_spOleObject);
		if (m_spOleObject)
		{
			m_spOleObject->GetMiscStatus(DVASPECT_CONTENT, &m_dwMiscStatus);
			if (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
			{
				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				m_spOleObject->SetClientSite(spClientSite);
			}

			if (!bInited) // If user hasn't initialized the control, initialize/load using IPersistStreamInit or IPersistStream
			{
				CComQIPtr<IPersistStreamInit> spPSI(m_spOleObject);
				if (spPSI)
				{
					if (pStream)
						hr = spPSI->Load(pStream);
					else
						hr = spPSI->InitNew();
				}
				else if (pStream)
				{
					CComQIPtr<IPersistStream> spPS(m_spOleObject);
					if (spPS)
						hr = spPS->Load(pStream);
				}

				if (FAILED(hr)) // If the initialization of the control failed...
				{
					// Clean up and return
					if (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
						m_spOleObject->SetClientSite(NULL);

					m_dwMiscStatus = 0;
					m_spOleObject.Release();
					m_spUnknown.Release();

					return hr;
				}
			}

			if (0 == (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
			{
				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				m_spOleObject->SetClientSite(spClientSite);
			}

			m_dwViewObjectType = 0;
			hr = m_spOleObject->QueryInterface(__uuidof(IViewObjectEx), (void**) &m_spViewObject);
			if (FAILED(hr))
			{
				hr = m_spOleObject->QueryInterface(__uuidof(IViewObject2), (void**) &m_spViewObject);
				if (SUCCEEDED(hr))
					m_dwViewObjectType = 3;
			} else
				m_dwViewObjectType = 7;

			if (FAILED(hr))
			{
				hr = m_spOleObject->QueryInterface(__uuidof(IViewObject), (void**) &m_spViewObject);
				if (SUCCEEDED(hr))
					m_dwViewObjectType = 1;
			}
			CComQIPtr<IAdviseSink> spAdviseSink(GetControllingUnknown());
			m_spOleObject->Advise(spAdviseSink, &m_dwOleObject);
			if (m_spViewObject)
				m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, spAdviseSink);
			m_spOleObject->SetHostNames(OLESTR("AXWIN"), NULL);

			if ((m_dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME) == 0)
			{
				GetClientRect(&m_rcPos);
				m_pxSize.cx = m_rcPos.right - m_rcPos.left;
				m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
				AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
				hr = m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
                if (FAILED(hr))
                    return hr;
				hr = m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
                if (FAILED(hr))
                    return hr;
				AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
				m_rcPos.right = m_rcPos.left + m_pxSize.cx;
				m_rcPos.bottom = m_rcPos.top + m_pxSize.cy;

				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				hr = m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
				RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
			}
		}
		CComPtr<IObjectWithSite> spSite;
		pUnkControl->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
		if (spSite != NULL)
			spSite->SetSite(GetControllingUnknown());

		return hr;
	}


// pointers
	CComPtr<IUnknown> m_spUnknown;
	CComPtr<IOleObject> m_spOleObject;
	CComPtr<IOleInPlaceFrame> m_spInPlaceFrame;
	CComPtr<IOleInPlaceUIWindow> m_spInPlaceUIWindow;
	CComPtr<IViewObjectEx> m_spViewObject;
	CComPtr<IOleInPlaceObjectWindowless> m_spInPlaceObjectWindowless;
	CComPtr<IDispatch> m_spExternalDispatch;
#ifndef _ATL_NO_DOCHOSTUIHANDLER
	CComPtr<IDocHostUIHandlerDispatch> m_spIDocHostUIHandlerDispatch;
#endif
	IID m_iidSink;
	DWORD m_dwViewObjectType;
	DWORD m_dwAdviseSink;

	// state
	unsigned long m_bInPlaceActive:1;
	unsigned long m_bUIActive:1;
	unsigned long m_bMDIApp:1;
	unsigned long m_bWindowless:1;
	unsigned long m_bCapture:1;
	unsigned long m_bHaveFocus:1;
	unsigned long m_bReleaseAll:1;
	unsigned long m_bLocked:1;

	DWORD m_dwOleObject;
	DWORD m_dwMiscStatus;
	SIZEL m_hmSize;
	SIZEL m_pxSize;
	RECT m_rcPos;

	// Accelerator table
	HACCEL m_hAccel;

	// Ambient property storage
	unsigned long m_bCanWindowlessActivate:1;
	unsigned long m_bUserMode:1;
	unsigned long m_bDisplayAsDefault:1;
	unsigned long m_bMessageReflect:1;
	unsigned long m_bSubclassed:1;
	unsigned long m_bAllowContextMenu:1;
	unsigned long m_bAllowShowUI:1;
	OLE_COLOR m_clrBackground;
	OLE_COLOR m_clrForeground;
	LCID m_lcidLocaleID;
	CComPtr<IFontDisp> m_spFont;
	CComPtr<IServiceProvider>  m_spServices;
	DWORD m_dwDocHostFlags;
	DWORD m_dwDocHostDoubleClickFlags;
	CComBSTR m_bstrOptionKeyPath;

	void SubclassWindow(_In_ HWND hWnd)
	{
		m_bSubclassed = CWindowImpl<CAxHostWindow>::SubclassWindow(hWnd);
	}

	void ReleaseWindow()
	{
		if (m_bSubclassed)
		{
			if(UnsubclassWindow(TRUE) != NULL)
				m_bSubclassed = FALSE;
		}
		else
			DestroyWindow();
	}

	// Reflection
	LRESULT ReflectNotifications(
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam,
		_Inout_ BOOL& bHandled)
	{
		HWND hWndChild = NULL;

		switch(uMsg)
		{
		case WM_COMMAND:
			if(lParam != 0)	// not from a menu
				hWndChild = (HWND)lParam;
			break;
		case WM_NOTIFY:
			hWndChild = ((LPNMHDR)lParam)->hwndFrom;
			break;
		case WM_PARENTNOTIFY:
			DefWindowProc();
			switch(LOWORD(wParam))
			{
			case WM_CREATE:
			case WM_DESTROY:
				hWndChild = (HWND)lParam;
				break;
			default:
				hWndChild = GetDlgItem(HIWORD(wParam));
				break;
			}
			break;
		case WM_DRAWITEM:
			{
				DRAWITEMSTRUCT* pdis = ((LPDRAWITEMSTRUCT)lParam);
				if (pdis->CtlType != ODT_MENU)	// not from a menu
					hWndChild = pdis->hwndItem;
				else							// Status bar control sends this message with type set to ODT_MENU
					if (::IsWindow(pdis->hwndItem))
						hWndChild = pdis->hwndItem;
			}
			break;
		case WM_MEASUREITEM:
			{
				MEASUREITEMSTRUCT* pmis = ((LPMEASUREITEMSTRUCT)lParam);
				if(pmis->CtlType != ODT_MENU)	// not from a menu
					hWndChild = GetDlgItem(pmis->CtlID);
			}
			break;
		case WM_COMPAREITEM:
				// Sent only by combo or list box
					hWndChild =((LPCOMPAREITEMSTRUCT)lParam)->hwndItem;
			break;
		case WM_DELETEITEM:
				// Sent only by combo or list box
				hWndChild = ((LPDELETEITEMSTRUCT)lParam)->hwndItem;
			break;
		case WM_VKEYTOITEM:
		case WM_CHARTOITEM:
		case WM_HSCROLL:
		case WM_VSCROLL:
			hWndChild = (HWND)lParam;
			break;
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			hWndChild = (HWND)lParam;
			break;
		default:
			break;
		}

		if(hWndChild == NULL)
		{
			bHandled = FALSE;
			return 1;
		}

		if (m_bWindowless)
		{
			LRESULT lRes = 0;
			if (m_bInPlaceActive && m_spInPlaceObjectWindowless)
                if (FAILED(m_spInPlaceObjectWindowless->OnWindowMessage(OCM__BASE + uMsg, wParam, lParam, &lRes)))
                    return 0;

			return lRes;
		}

		ATLASSERT(::IsWindow(hWndChild));
		return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
	}

ATLPREFAST_SUPPRESS(6387)
	STDMETHOD(QueryService)(
		_In_ REFGUID rsid,
		_In_ REFIID riid,
		_Outptr_ void** ppvObj)
	{
		ATLASSERT(ppvObj != NULL);
		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;

		HRESULT hr = E_NOINTERFACE;
		// Try for service on this object

		// No services currently

		// If that failed try to find the service on the outer object
		if (m_spServices)
		{
			hr = m_spServices->QueryService(rsid, riid, ppvObj);
		}

		return hr;
	}
ATLPREFAST_UNSUPPRESS()

};

static LRESULT CALLBACK AtlAxWindowProc(
	_In_ HWND hWnd,
	_In_ UINT uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		{
			// create control from a PROGID in the title
			// This is to make sure drag drop works
ATLPREFAST_SUPPRESS(6031)
			::OleInitialize(NULL);
ATLPREFAST_UNSUPPRESS()

			CREATESTRUCT* lpCreate = (CREATESTRUCT*)lParam;
			int nLen = ::GetWindowTextLength(hWnd);
			CAutoStackPtr<TCHAR> spName((TCHAR *)_malloca((nLen + 1) * sizeof(TCHAR)));
			if(!spName)
			{
				return -1;
			}
			::GetWindowText(hWnd, spName, nLen + 1);
			::SetWindowText(hWnd, _T(""));
			IAxWinHostWindow* pAxWindow = NULL;
			int nCreateSize = 0;
			if (lpCreate && lpCreate->lpCreateParams)
				nCreateSize = *((WORD*)lpCreate->lpCreateParams);

			CComPtr<IStream> spStream;
			if (nCreateSize)
			{
				HGLOBAL h = GlobalAlloc(GHND, nCreateSize);
ATLPREFAST_SUPPRESS(6387)
				if (h)
				{
					BYTE* pBytes = (BYTE*) GlobalLock(h);
					BYTE* pSource = ((BYTE*)(lpCreate->lpCreateParams)) + sizeof(WORD);
					//Align to DWORD
					//pSource += (((~((DWORD)pSource)) + 1) & 3);
					Checked::memcpy_s(pBytes, nCreateSize, pSource, nCreateSize);
					GlobalUnlock(h);
					ATLENSURE_RETURN_VAL(SUCCEEDED(CreateStreamOnHGlobal(h, TRUE, &spStream)), -1);
				}
ATLPREFAST_UNSUPPRESS()
			}

			USES_CONVERSION_EX;
			CComPtr<IUnknown> spUnk;
			LPCOLESTR lpszName = T2COLE_EX_DEF(spName);
			ATLASSUME(lpszName);
			HRESULT hRet = AtlAxCreateControlLic(lpszName, hWnd, spStream, &spUnk, NULL);
			if(FAILED(hRet))
			{
#ifdef _DEBUG
				LPTSTR pszMsg = NULL;
				::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, hRet, 0, (LPTSTR)&pszMsg, 0, NULL);
				ATLTRACE(atlTraceControls, 0, _T("Control creation failed for '%Ts'\n"), static_cast<TCHAR *>(spName));
				ATLTRACE(atlTraceControls, 0, _T("Error code: 0x%x - %Ts"), hRet, pszMsg);
				::LocalFree(pszMsg);
#endif
				return -1;	// abort window creation
			}
			hRet = spUnk->QueryInterface(__uuidof(IAxWinHostWindow), (void**)&pAxWindow);
			if(FAILED(hRet))
				return -1;	// abort window creation

			::SetWindowLongPtr(hWnd, GWLP_USERDATA, (DWORD_PTR)pAxWindow);
		// continue with DefWindowProc
		}
		break;
	case WM_NCDESTROY:
		{
			IAxWinHostWindow* pAxWindow = (IAxWinHostWindow*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if(pAxWindow != NULL)
				pAxWindow->Release();
			OleUninitialize();
		}
		break;

	case WM_PARENTNOTIFY:
		{
			if(LOWORD( wParam ) == WM_CREATE)
			{
				ATLASSERT(lParam);
				// Set the control parent style for the AxWindow
				if(lParam != 0) {
					DWORD dwExStyle = ::GetWindowLong((HWND)lParam, GWL_EXSTYLE);
					if(dwExStyle & WS_EX_CONTROLPARENT)
					{
						dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
						dwExStyle |= WS_EX_CONTROLPARENT;
						::SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
					}
				}
			}
		}
		break;

	default:
		break;
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK AtlAxWindowProc2(
	_In_ HWND hWnd,
	_In_ UINT uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		{
			// create control from a PROGID in the title
			// This is to make sure drag drop works
ATLPREFAST_SUPPRESS(6031)
			::OleInitialize(NULL);
ATLPREFAST_UNSUPPRESS()

			CREATESTRUCT* lpCreate = (CREATESTRUCT*)lParam;
			int nLen = ::GetWindowTextLength(hWnd);
			CAutoStackPtr<TCHAR> spName((TCHAR *)_malloca((nLen + 1) * sizeof(TCHAR)));
			if(!spName)
			{
				return -1;
			}
			::GetWindowText(hWnd, spName, nLen + 1);
			::SetWindowText(hWnd, _T(""));
			IAxWinHostWindowLic* pAxWindow = NULL;
			int nCreateSize = 0;
			if (lpCreate && lpCreate->lpCreateParams)
				nCreateSize = *((WORD*)lpCreate->lpCreateParams);

			// Format of data in lpCreateParams
			//	int nCreateSize;	// size of Create data in bytes
			//	WORD nMsg;			//constant used to indicate type of DLGINIT data. See _DialogSplitHelper for values.
			//	DWORD dwLen;		// Length of data strred for control in DLGINIT format in bytes.
			//	DWORD cchLicKey;	// Length of license key in OLECHAR's
			//  OLECHAR *szLicKey;	// This will be present only if cchLicKey is greater than 0. This is of variable length and will contain cchLicKey OLECHAR's that represent the licence key.
			//	The following two fields will be present only if nMsg is WM_OCC_LOADFROMSTREAM_EX or WM_OCC_LOADFROMSTORAGE_EX. If present this information will be ignored since databinding is not supported.
			//		ULONG cbDataBinding;	// Length of databinding information in bytes.
			//		BYTE *pbDataBindingInfo	// cbDataBinding bytes that contain databinding information
			//	BYTE *pbControlData;	// Actual control data persisted by the control.


			CComPtr<IStream> spStream;
			if (nCreateSize)
			{
				HGLOBAL h = GlobalAlloc(GHND, nCreateSize);
ATLPREFAST_SUPPRESS(6387)
				if (h)
				{
					BYTE* pBytes = (BYTE*) GlobalLock(h);
					BYTE* pSource = ((BYTE*)(lpCreate->lpCreateParams)) + sizeof(WORD);
					//Align to DWORD
					//pSource += (((~((DWORD)pSource)) + 1) & 3);
					Checked::memcpy_s(pBytes, nCreateSize, pSource, nCreateSize);
					GlobalUnlock(h);
					ATLENSURE_RETURN_VAL(SUCCEEDED(CreateStreamOnHGlobal(h, TRUE, &spStream)), -1);
				}
ATLPREFAST_UNSUPPRESS()
			}

			CComBSTR bstrLicKey;
			HRESULT hRet = _DialogSplitHelper::ParseInitData(spStream, &bstrLicKey.m_str);
			if (FAILED(hRet))
				return -1;

			USES_CONVERSION_EX;
			CComPtr<IUnknown> spUnk;
			LPCOLESTR lpszName = T2COLE_EX_DEF(spName);
			ATLASSUME(lpszName);
			hRet = AtlAxCreateControlLic(lpszName, hWnd, spStream, &spUnk, bstrLicKey);
			if(FAILED(hRet))
			{
#ifdef _DEBUG
				LPTSTR pszMsg = NULL;
				::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, hRet, 0, (LPTSTR)&pszMsg, 0, NULL);
				ATLTRACE(atlTraceControls, 0, _T("Control creation failed for '%Ts'\n"), static_cast<TCHAR *>(spName));
				ATLTRACE(atlTraceControls, 0, _T("Error code: 0x%x - %Ts"), hRet, pszMsg);
				::LocalFree(pszMsg);
#endif
				return -1;	// abort window creation
			}
			hRet = spUnk->QueryInterface(__uuidof(IAxWinHostWindowLic), (void**)&pAxWindow);
			if(FAILED(hRet))
				return -1;	// abort window creation
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, (DWORD_PTR)pAxWindow);
		// continue with DefWindowProc
		}
		break;
	case WM_NCDESTROY:
		{
			IAxWinHostWindowLic* pAxWindow = (IAxWinHostWindowLic*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if(pAxWindow != NULL)
				pAxWindow->Release();
			OleUninitialize();
		}
		break;

	case WM_PARENTNOTIFY:
		{
			if(LOWORD( wParam ) == WM_CREATE)
			{
				ATLASSERT(lParam);
				// Set the control parent style for the AxWindow
				if(lParam != 0) {
					DWORD dwExStyle = ::GetWindowLong((HWND)lParam, GWL_EXSTYLE);
					if(dwExStyle & WS_EX_CONTROLPARENT)
					{
						dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
						dwExStyle |= WS_EX_CONTROLPARENT;
						::SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
					}
				}
			}
		}
		break;

	default:
		break;
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

inline HRSRC AtlFindResource(
	_In_opt_ HMODULE hModule,
	_In_z_ LPCSTR lpName,
	_In_z_ LPCSTR lpType)
{
	return ::FindResourceA(hModule, lpName, lpType);
}

inline HRSRC AtlFindResource(
	_In_opt_ HMODULE hModule,
	_In_z_ LPCWSTR lpName,
	_In_z_ LPCWSTR lpType)
{
	return ::FindResourceW(hModule, lpName, lpType);
}

class _AtlDialogBoxIndirectParamHelper
{
public :
	typedef INT_PTR ReturnType;
	static ReturnType GetInvalidValue()
	{
		return -1;
	}
	static bool IsValidReturnVal(_In_ ReturnType nRet)
	{
		return (nRet != 0 && nRet != -1);
	}
};

class _AtlCreateDialogIndirectParamHelper
{
public :
	typedef HWND ReturnType;
	static ReturnType GetInvalidValue()
	{
		return NULL;
	}
	static bool IsValidReturnVal(_In_ ReturnType nRet)
	{
		return (nRet != 0);
	}
};

template <class StringType, class Helper, typename Helper::ReturnType (WINAPI *pFunc)(HINSTANCE, LPCDLGTEMPLATE, HWND,DLGPROC, LPARAM)>
typename Helper::ReturnType AtlAxDialogCreateT(
	_In_ HINSTANCE hInstance,
	_In_z_ StringType lpTemplateName,
	_In_ HWND hWndParent,
	_In_ DLGPROC lpDialogProc,
	_In_ LPARAM dwInitParam)
{
	AtlAxWinInit();
	typename Helper::ReturnType nRet = Helper::GetInvalidValue();

	HRSRC hDlg = AtlFindResource(hInstance, lpTemplateName, (StringType)RT_DIALOG);
	if (hDlg != NULL)
	{
		HRSRC hDlgInit = AtlFindResource(hInstance, lpTemplateName, (StringType)_ATL_RT_DLGINIT);
		HGLOBAL hData = NULL;
		BYTE* pInitData = NULL;

		if (hDlgInit)
		{
			hData = ::LoadResource(hInstance, hDlgInit);
			ATLASSUME(hData != NULL);
			pInitData = (BYTE*) ::LockResource(hData);
		}

		DWORD dwLastError = 0;
		HGLOBAL hResource = LoadResource(hInstance, hDlg);
		if (hResource != NULL)
		{
			DLGTEMPLATE* pDlg = (DLGTEMPLATE*) LockResource(hResource);
			if (pDlg != NULL)
			{
				LPCDLGTEMPLATE lpDialogTemplate;

				lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);

				nRet = (*pFunc)(hInstance, lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
				if (!Helper::IsValidReturnVal(nRet))
				{
					dwLastError = ::GetLastError();
				}
				if ((lpDialogTemplate != pDlg) && (lpDialogTemplate != NULL))
					GlobalFree(GlobalHandle(lpDialogTemplate));
			}
			else
				dwLastError = ::GetLastError();
		}
		else
			dwLastError = ::GetLastError();

		if (dwLastError != 0)
			SetLastError(dwLastError);
	}
	return nRet;
}

ATLINLINE ATLAPI_(INT_PTR) AtlAxDialogBoxW(
	_In_ HINSTANCE hInstance,
	_In_z_ LPCWSTR lpTemplateName,
	_In_ HWND hWndParent,
	_In_ DLGPROC lpDialogProc,
	_In_ LPARAM dwInitParam)
{
	return AtlAxDialogCreateT<LPCWSTR, _AtlDialogBoxIndirectParamHelper, ::DialogBoxIndirectParamW>(
		hInstance, lpTemplateName, hWndParent, lpDialogProc, dwInitParam);
}

ATLINLINE ATLAPI_(INT_PTR) AtlAxDialogBoxA(
	_In_ HINSTANCE hInstance,
	_In_z_ LPCSTR lpTemplateName,
	_In_ HWND hWndParent,
	_In_ DLGPROC lpDialogProc,
	_In_ LPARAM dwInitParam)
{
	return AtlAxDialogCreateT<LPCSTR, _AtlDialogBoxIndirectParamHelper, ::DialogBoxIndirectParamA>(
		hInstance, lpTemplateName, hWndParent, lpDialogProc, dwInitParam);
}

ATLINLINE ATLAPI_(HWND) AtlAxCreateDialogW(
	_In_ HINSTANCE hInstance,
	_In_z_ LPCWSTR lpTemplateName,
	_In_ HWND hWndParent,
	_In_ DLGPROC lpDialogProc,
	_In_ LPARAM dwInitParam)
{
	return AtlAxDialogCreateT<LPCWSTR, _AtlCreateDialogIndirectParamHelper, CreateDialogIndirectParamW>(
		hInstance, lpTemplateName, hWndParent, lpDialogProc, dwInitParam);
}

ATLINLINE ATLAPI_(HWND) AtlAxCreateDialogA(
	_In_ HINSTANCE hInstance,
	_In_z_ LPCSTR lpTemplateName,
	_In_ HWND hWndParent,
	_In_ DLGPROC lpDialogProc,
	_In_ LPARAM dwInitParam)
{
	return AtlAxDialogCreateT<LPCSTR, _AtlCreateDialogIndirectParamHelper, CreateDialogIndirectParamA>(hInstance, lpTemplateName, hWndParent, lpDialogProc, dwInitParam);
}

ATLINLINE ATLAPI AtlAxCreateControl(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd,
	_Inout_opt_ IStream* pStream,
	_Outptr_ IUnknown** ppUnkContainer)
{
	return AtlAxCreateControlEx(lpszName, hWnd, pStream, ppUnkContainer, NULL, IID_NULL, NULL);
}

ATLINLINE ATLAPI AtlAxCreateControlEx(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd,
	_Inout_opt_ IStream* pStream,
	_Outptr_opt_ IUnknown** ppUnkContainer,
	_Outptr_opt_ IUnknown** ppUnkControl,
	_In_ REFIID iidSink,
	_Inout_opt_ IUnknown* punkSink)
{
	return AtlAxCreateControlLicEx(lpszName, hWnd, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink, NULL);
}

ATLINLINE ATLAPI AtlAxCreateControlLic(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd,
	_Inout_opt_ IStream* pStream,
	_Outptr_opt_ IUnknown** ppUnkContainer,
	_In_opt_z_ BSTR bstrLic)
{
	return AtlAxCreateControlLicEx(lpszName, hWnd, pStream, ppUnkContainer, NULL, IID_NULL, NULL, bstrLic);
}

ATLPREFAST_SUPPRESS(6387)
ATLINLINE ATLAPI AtlAxCreateControlLicEx(
	_In_z_ LPCOLESTR lpszName,
	_In_ HWND hWnd,
	_Inout_opt_ IStream* pStream,
	_Outptr_opt_ IUnknown** ppUnkContainer,
	_Outptr_opt_ IUnknown** ppUnkControl,
	_In_ REFIID iidSink,
	_Inout_opt_ IUnknown* punkSink,
	_In_opt_z_ BSTR bstrLic)
{
	AtlAxWinInit();
	HRESULT hr;
	CComPtr<IUnknown> spUnkContainer;
	CComPtr<IUnknown> spUnkControl;

	hr = CAxHostWindow::_CreatorClass::CreateInstance(NULL, __uuidof(IUnknown), (void**)&spUnkContainer);
	if (SUCCEEDED(hr))
	{
		CComPtr<IAxWinHostWindowLic> pAxWindow;
		spUnkContainer->QueryInterface(__uuidof(IAxWinHostWindow), (void**)&pAxWindow);
		CComBSTR bstrName(lpszName);
		hr = pAxWindow->CreateControlLicEx(bstrName, hWnd, pStream, &spUnkControl, iidSink, punkSink, bstrLic);
	}
	if (ppUnkContainer != NULL)
	{
		if (SUCCEEDED(hr))
		{
			*ppUnkContainer = spUnkContainer.p;
			spUnkContainer.p = NULL;
		}
		else
			*ppUnkContainer = NULL;
	}
	if (ppUnkControl != NULL)
	{
		if (SUCCEEDED(hr))
		{
			*ppUnkControl = SUCCEEDED(hr) ? spUnkControl.p : NULL;
			spUnkControl.p = NULL;
		}
		else
			*ppUnkControl = NULL;
	}
	return hr;
}
ATLPREFAST_UNSUPPRESS()

ATLPREFAST_SUPPRESS(6387)
ATLINLINE ATLAPI AtlAxAttachControl(
	_Inout_ IUnknown* pControl,
	_In_ HWND hWnd,
	_Outptr_opt_ IUnknown** ppUnkContainer)
{
	AtlAxWinInit();
	if (pControl == NULL)
		return E_INVALIDARG;
	CComPtr<IUnknown> spUnkContainer;

	HRESULT hr = CAxHostWindow::_CreatorClass::CreateInstance(NULL, __uuidof(IUnknown), (void**)&spUnkContainer);
	if (SUCCEEDED(hr))
	{
		CComPtr<IAxWinHostWindow> spAxWindow;
		hr = spUnkContainer->QueryInterface(&spAxWindow);
		if (SUCCEEDED(hr))
			hr = spAxWindow->AttachControl(pControl, hWnd);
	}
	if (ppUnkContainer != NULL)
	{
		*ppUnkContainer = SUCCEEDED(hr) ? spUnkContainer.Detach() : NULL;
	}
	return hr;
}
ATLPREFAST_UNSUPPRESS()

//This either registers a global class (if AtlAxWinInit is in ATL.DLL)
// or it registers a local class
ATLINLINE ATLAPI_(BOOL) AtlAxWinInit()
{
	CComCritSecLock<CComCriticalSection> lock(_AtlWinModule.m_csWindowCreate, false);
	if (FAILED(lock.Lock()))
	{
		ATLTRACE(atlTraceHosting, 0, _T("ERROR : Unable to lock critical section in AtlAxWinInit\n"));
		ATLASSERT(0);
		return FALSE;
	}
	WM_ATLGETHOST = RegisterWindowMessage(_T("WM_ATLGETHOST"));
	WM_ATLGETCONTROL = RegisterWindowMessage(_T("WM_ATLGETCONTROL"));
	WNDCLASSEX wc;
// first check if the class is already registered
	wc.cbSize = sizeof(WNDCLASSEX);
	BOOL bRet = ::GetClassInfoEx(_AtlBaseModule.GetModuleInstance(), CAxWindow::GetWndClassName(), &wc);

// register class if not

	if(!bRet)
	{
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = AtlAxWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _AtlBaseModule.GetModuleInstance();
		wc.hIcon = NULL;
		wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CAxWindow::GetWndClassName();
		wc.hIconSm = NULL;

		ATOM atom= ::RegisterClassEx(&wc);
		if (atom)
		{
		   _AtlWinModule.m_rgWindowClassAtoms.Add(atom);
		   bRet=TRUE;
		}else
		{
		  bRet=FALSE;
		}
	}

	if (bRet)
	{
		// first check if the class is already registered
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize = sizeof(WNDCLASSEX);
		bRet = ::GetClassInfoEx(_AtlBaseModule.GetModuleInstance(), CAxWindow2::GetWndClassName(), &wc);

		// register class if not

		if(!bRet)
		{
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.style = CS_DBLCLKS;
			wc.lpfnWndProc = AtlAxWindowProc2;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = _AtlBaseModule.GetModuleInstance();
			wc.hIcon = NULL;
			wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			wc.lpszMenuName = NULL;
			wc.lpszClassName = CAxWindow2::GetWndClassName();
			wc.hIconSm = NULL;
			ATOM atom= RegisterClassEx(&wc);

			if (atom)
			{
				_AtlWinModule.m_rgWindowClassAtoms.Add(atom);
				bRet=TRUE;
			}else
			{
				bRet=FALSE;
			}

		}
	}
	return bRet;
}

ATLPREFAST_SUPPRESS(6387)
ATLINLINE ATLAPI AtlAxGetControl(
	_In_ HWND h,
	_Outptr_ IUnknown** pp)
{
	ATLASSERT(WM_ATLGETCONTROL != 0);
	if (pp == NULL)
		return E_POINTER;
	*pp = (IUnknown*)SendMessage(h, WM_ATLGETCONTROL, 0, 0);
	return (*pp) ? S_OK : E_FAIL;
}
ATLPREFAST_UNSUPPRESS()

ATLPREFAST_SUPPRESS(6387)
ATLINLINE ATLAPI AtlAxGetHost(
	_In_ HWND h,
	_Outptr_ IUnknown** pp)
{
	ATLASSERT(WM_ATLGETHOST != 0);
	if (pp == NULL)
		return E_POINTER;
	*pp = (IUnknown*)SendMessage(h, WM_ATLGETHOST, 0, 0);
	return (*pp) ? S_OK : E_FAIL;
}
ATLPREFAST_UNSUPPRESS()

}; //namespace ATL
#pragma pack(pop)

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif //!_ATL_NO_PRAGMA_WARNINGS

#endif  // __ATLHOST_H__
