//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the OleInPlaceActiveObject class of PresentationHost.
//
//  History:
//     2002/06/27-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

//******************************************************************************
//
//   Forward declarations
//
//******************************************************************************

class COleDocument;

class COleInPlaceActiveObject : public IOleInPlaceActiveObject, public IInputObject
{
private:

    COleDocument   *m_pOleDoc;

public:

    COleInPlaceActiveObject::COleInPlaceActiveObject(__in COleDocument*);
    COleInPlaceActiveObject::~COleInPlaceActiveObject();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IOleInPlaceActiveObject methods
    STDMETHODIMP OnDocWindowActivate(BOOL);
    STDMETHODIMP OnFrameWindowActivate(BOOL);
    STDMETHODIMP GetWindow(__out HWND*);
    STDMETHODIMP ContextSensitiveHelp(BOOL);
    STDMETHODIMP TranslateAccelerator(LPMSG);
    STDMETHODIMP ResizeBorder(LPCRECT, LPOLEINPLACEUIWINDOW, BOOL);
    STDMETHODIMP EnableModeless(BOOL);

    // IInputObject methods
    STDMETHODIMP TranslateAcceleratorIO(__in MSG *pMsg);
    STDMETHODIMP HasFocusIO();
    STDMETHODIMP UIActivateIO(BOOL fActivate, __in_opt MSG *pMsg);
};
