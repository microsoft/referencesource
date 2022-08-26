//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the OleDocumentView class of PresentationHost.
//
//  History:
//     2002/06/27-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

class COleDocumentView : public IOleDocumentView
{
friend class COleDocument;
friend class COleInPlaceActiveObject;

private:

    LPOLEINPLACESITE      m_pInPlaceSite;
    LPOLEINPLACEFRAME     m_pInPlaceFrame;
    LPOLEINPLACEUIWINDOW  m_pInPlaceUIWindow;
    OLEINPLACEFRAMEINFO   m_FrameInfo;

    BOOL                  m_fShow;
    BOOL                  m_fUIActive;
    BOOL                  m_fInPlaceActive;
    RECT                  m_Rect;

    OLEMENUGROUPWIDTHS    m_MenuGroupWidth;
    HMENU                 m_hDocObjMenu;
    HMENU                 m_hSharedMenu;
    HMENU                 m_hViewMenu;
    HOLEMENU              m_hOleMenu;

    COleDocument*         m_pOleDoc;
   
public:

    COleDocumentView(__in COleDocument*);
    ~COleDocumentView();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IOleDocumentView methods
    STDMETHODIMP SetInPlaceSite(__in_opt IOleInPlaceSite*);
    STDMETHODIMP GetInPlaceSite(__out IOleInPlaceSite**);
    STDMETHODIMP GetDocument(__out IUnknown**);
    STDMETHODIMP SetRect(__in LPRECT);
    STDMETHODIMP GetRect(__out LPRECT);
    STDMETHODIMP SetRectComplex(LPRECT, LPRECT, LPRECT, LPRECT);
    STDMETHODIMP Show(BOOL);
    STDMETHODIMP UIActivate(BOOL);
    STDMETHODIMP Open(void);
    STDMETHODIMP CloseView(DWORD);
    STDMETHODIMP SaveViewState(IStream*);
    STDMETHODIMP ApplyViewState(IStream*);
    STDMETHODIMP Clone(IOleInPlaceSite*, IOleDocumentView**);

    COleDocument* GetDocument() { return m_pOleDoc; }
    LPOLEINPLACEFRAME GetInPlaceFrame() { return m_pInPlaceFrame; }
    LPOLEINPLACEFRAMEINFO GetInPlaceFrameInfo() { return &m_FrameInfo; }

private:
    STDMETHODIMP ActivateInPlace();
    STDMETHODIMP DeactivateInPlace();
    STDMETHODIMP ActivateUI();
    STDMETHODIMP DeactivateUI();
    STDMETHODIMP LoadMenuFromMuiResource(__out_ecount(1) HMENU * menu);
    STDMETHODIMP SetViewMenuItemsCheckState(HMENU viewMenu);

    STDMETHODIMP InsertMergedMenus();
    void         RemoveMergedMenus();
    void         DestroyMenuResources();

    //
    // Helpers to communicate to Managed methods
    //
    void MoveViewportHelper(int x, int y, int nWidth, int nHeight);
    void ShowHelper(bool fShow);
    void SetParentHelper(HWND hwndParent);
};
