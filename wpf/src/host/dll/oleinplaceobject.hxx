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

class COleInPlaceObject : public IOleInPlaceObject
{
private:
    COleDocument   *m_pOleDoc;

public:
    COleInPlaceObject::COleInPlaceObject(__in COleDocument*);
    COleInPlaceObject::~COleInPlaceObject();

   //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IOleInPlaceObject methods
    STDMETHODIMP InPlaceDeactivate();
    STDMETHODIMP UIDeactivate() ;
    STDMETHODIMP SetObjectRects(LPCRECT, LPCRECT);
    STDMETHODIMP GetWindow(__out HWND*) ;
    STDMETHODIMP ContextSensitiveHelp(BOOL);
    STDMETHODIMP ReactivateAndUndo();
};
