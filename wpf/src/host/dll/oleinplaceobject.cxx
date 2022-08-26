//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the OleInPlaceObject class of PresentationHost.
//
//  History:
//     2002/06/27-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "OleInPlaceObject.hxx"
#include "OleDocument.hxx"
#include "OleDocumentView.hxx"

//******************************************************************************
//
// COleInPlaceObject::COleInPlaceObject()
//   
//******************************************************************************

COleInPlaceObject::COleInPlaceObject(__in COleDocument *pOleDoc)
{
    m_pOleDoc = pOleDoc;
}

//******************************************************************************
//
// COleInPlaceObject::~COleInPlaceObject()
//   
//******************************************************************************

COleInPlaceObject::~COleInPlaceObject() 
{
}

//******************************************************************************
//
// COleInPlaceObject::QueryInterface()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceObject::QueryInterface(REFIID riid, __out LPVOID *ppReturn)
{
    return m_pOleDoc->QueryInterface(riid, ppReturn);
}

//******************************************************************************
//
// COleInPlaceObject::AddRef()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) COleInPlaceObject::AddRef()
{
    return m_pOleDoc->AddRef();
}

//******************************************************************************
//
// COleInPlaceObject::Release()
//   
//******************************************************************************

STDMETHODIMP_(ULONG) COleInPlaceObject::Release()
{
    return m_pOleDoc->Release();
}

//******************************************************************************
//
// COleInPlaceObject::InPlaceDeactivate()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceObject::InPlaceDeactivate()
{
    return m_pOleDoc->DeactivateInPlace();
}

//******************************************************************************
//
// COleInPlaceObject::UIDeactivate()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceObject::UIDeactivate()
{
    return m_pOleDoc->DeactivateUI();
}

//******************************************************************************
//
// COleInPlaceObject::SetObjectRects()
//   
// This is called when hosted as a control.
//
//******************************************************************************

STDMETHODIMP COleInPlaceObject::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    ASSERT(memcmp(lprcPosRect, lprcClipRect, sizeof RECT) == 0); // The only tested case
    return m_pOleDoc->m_pOleDocView->SetRect(const_cast<LPRECT>(lprcPosRect));
}

//******************************************************************************
//
// COleInPlaceObject::GetWindow()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceObject::GetWindow(__out HWND *phwnd)
{
    return m_pOleDoc->GetWindow(phwnd);
}

//******************************************************************************
//
// COleInPlaceObject::ContextSensitiveHelp()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceObject::ContextSensitiveHelp(BOOL /* fEnterMode */)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// COleInPlaceObject::ReactivateAndUndo()
//   
//******************************************************************************

STDMETHODIMP COleInPlaceObject::ReactivateAndUndo()
{
    return INPLACE_E_NOTUNDOABLE;
}

