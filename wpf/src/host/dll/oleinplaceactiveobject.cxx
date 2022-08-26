//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the OleInPlaceActiveObject class of PresentationHost.
//
//  History:
//     2002/06/27-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "OleInPlaceActiveObject.hxx"
#include "OleDocument.hxx"
#include "Utilities.hxx"

//******************************************************************************
//
// COleInPlaceActiveObject::COleInPlaceActiveObject()
//   
//******************************************************************************

COleInPlaceActiveObject::COleInPlaceActiveObject(__in COleDocument *pOleDoc)
{
    m_pOleDoc = pOleDoc;
    //m_fEnableModeless = FALSE;
}
//******************************************************************************
//
// COleInPlaceActiveObject::~COleInPlaceActiveObject()
//   
//******************************************************************************

COleInPlaceActiveObject::~COleInPlaceActiveObject()
{
}

//******************************************************************************
//
// COleInPlaceActiveObject::QueryInterface()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::QueryInterface(REFIID riid, 
                                                     __out LPVOID *ppReturn)
{
    // IE queries for IInputObject via CHostShim.

    return m_pOleDoc->QueryInterface(riid, ppReturn);
}

//******************************************************************************
//
// COleInPlaceActiveObject::AddRef()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) COleInPlaceActiveObject::AddRef()
{
    return m_pOleDoc->AddRef();
}

//******************************************************************************
//
// COleInPlaceActiveObject::Release()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) COleInPlaceActiveObject::Release()
{
    return m_pOleDoc->Release();
}

//******************************************************************************
//
// COleInPlaceActiveObject::OnDocWindowActivate()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::OnDocWindowActivate(BOOL fActivate)
{
    return S_OK;
}

//******************************************************************************
//
// COleInPlaceActiveObject::OnFrameWindowActivate()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::OnFrameWindowActivate(BOOL fActivate )
{
    IBrowserHostServices  *pBrowserHost;

    pBrowserHost = m_pOleDoc->ApplicationHelper();

    if ( pBrowserHost != NULL )
    {
        pBrowserHost->Activate(fActivate);
    }

    return S_OK;
}

//******************************************************************************
//
// COleInPlaceActiveObject::GetWindow()
//  
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::GetWindow(__out HWND *phwnd)
{
    return m_pOleDoc->GetWindow(phwnd);
}

//******************************************************************************
//
// COleInPlaceActiveObject::ContextSensitiveHelp()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::ContextSensitiveHelp(BOOL /* fEnterMode */)
{
    return E_NOTIMPL;
}

//******************************************************************************
// COleInPlaceActiveObject::TranslateAccelerator()
// We would look up our own accelerators here, but we don't have any.
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::TranslateAccelerator(LPMSG pMsg)
{
    // no accelerator table, return FALSE

    return S_FALSE;
}

//******************************************************************************
//
// COleInPlaceActiveObject::ResizeBorder()
//  
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::ResizeBorder(LPCRECT /* lprectBorder */,
                                                   LPOLEINPLACEUIWINDOW /* lpUIWindow */,
                                                   BOOL /* fFrameWindow */)
{
    return S_OK;
}

//******************************************************************************
//
// COleInPlaceActiveObject::EnableModeless()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceActiveObject::EnableModeless(BOOL fEnable)
{
    STD_ASSERT_UNUSED;
/*
    m_fEnableModeless = fEnable;

    return S_OK;
*/
}

//******************************************************************************
//
// IInputObject implementation. Used by IE.
//      Enables tabbing into the DocObject from the browser frame.
//      Tabbing into an HTML frame with our DocObject is handled by COleObject::DoVerb().
//      Tabbing out is via IBrowserCallbackServices::TabOut().
//
//  In Firefox, tabbing in is done via IOleCommandTarget and a custom command - PHCMDID_TABINTO.

STDMETHODIMP COleInPlaceActiveObject::TranslateAcceleratorIO(__in MSG *pMsg)
{
    HRESULT hr = E_FAIL;

    int nDir = IsTabCycler(*pMsg);
    if(nDir && HasFocusIO() == S_FALSE && m_pOleDoc->ApplicationHelper())
    {
        hr = m_pOleDoc->ApplicationHelper()->TabInto(nDir > 0);
    }

    return hr;
}

STDMETHODIMP COleInPlaceActiveObject::HasFocusIO()
{
    HRESULT hr = S_FALSE;
    HWND hwnd = NULL;
    if (SUCCEEDED(GetWindow(&hwnd)))
    {
        HWND hwndFocus = GetFocus();
        if (hwnd == hwndFocus || IsChild(hwnd, hwndFocus))
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP COleInPlaceActiveObject::UIActivateIO(BOOL fActivate, __in_opt MSG *pMsg)
{
    // This is currently not used, but in general it should UI activate/deactivate.
    return OnDocWindowActivate(fActivate);
}
