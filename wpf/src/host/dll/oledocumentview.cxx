//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements the OleDocumentView class of PresentationHost.
//
//  History:
//     2002/06/27-murrayw
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "OleDocumentView.hxx"
#include "OleDocument.hxx"
#include "OleInPlaceActiveObject.hxx"
#include "ErrorPage.hxx"
#include "Utilities.hxx"


extern HINSTANCE  g_hInstance;


//******************************************************************************
//
// COleDocumentView::COleDocumentView()
//
//******************************************************************************

COleDocumentView::COleDocumentView(__in COleDocument *pOleDoc):
    m_pInPlaceSite(0), m_pInPlaceFrame(0), m_pInPlaceUIWindow(0), m_FrameInfo(),
    m_fShow(false), m_fUIActive(false), m_fInPlaceActive(false), m_Rect(),
    m_MenuGroupWidth(), 
    m_hDocObjMenu(0), m_hSharedMenu(0), m_hViewMenu(0), m_hOleMenu(0)
{
    //This is interface aggregation, don't AddRef the OleDoc
    m_pOleDoc          = pOleDoc;
}

//******************************************************************************
//
// COleDocumentView::~COleDocumentView()
//
//******************************************************************************

COleDocumentView::~COleDocumentView()
{
    //Should already have been destroyed when we remove menus. Calling it here 
    //to be on the safe side
    DestroyMenuResources();

    ReleaseInterface(m_pInPlaceSite);
    ReleaseInterface(m_pInPlaceFrame);
    ReleaseInterface(m_pInPlaceUIWindow);
}

//******************************************************************************
//
// IUnknown Implementation
//
//******************************************************************************

//******************************************************************************
//
// COleDocumentView::QueryInterface
//
//******************************************************************************

STDMETHODIMP COleDocumentView::QueryInterface(REFIID riid, 
                                              __out LPVOID *ppReturn)
{
    return m_pOleDoc->QueryInterface(riid, ppReturn);
}                                             

//******************************************************************************
//
// COleDocumentView::AddRef
//
//******************************************************************************

STDMETHODIMP_(DWORD) COleDocumentView::AddRef()
{
    return m_pOleDoc->AddRef();
}

//******************************************************************************
//
// COleDocumentView::Release
//
//******************************************************************************

STDMETHODIMP_(DWORD) COleDocumentView::Release()
{
    return m_pOleDoc->Release();
}

//******************************************************************************
//
// COleDocumentView::SetInPlaceSite()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::SetInPlaceSite(__in_opt IOleInPlaceSite *pNewSite)
{
    HRESULT hr = S_OK;

    if (m_pInPlaceSite)
    {
        if (m_fUIActive)
        {
            CKHR(DeactivateUI());
        }
           
        if (m_fInPlaceActive)
        {
            CKHR(DeactivateInPlace());
        }
           
        if (m_pInPlaceFrame)
        {
            ReleaseInterface(m_pInPlaceFrame);
        }
           
        if (m_pInPlaceUIWindow)
        {
            ReleaseInterface(m_pInPlaceUIWindow);
        }
           
        ReleaseInterface(m_pInPlaceSite);
    }

    m_pInPlaceSite = pNewSite;

    if (m_pInPlaceSite)
    {
        m_pInPlaceSite->AddRef();

        RECT  rcClip;

        m_FrameInfo.cb = sizeof(m_FrameInfo);
           
        CKHR(m_pInPlaceSite->GetWindowContext(&m_pInPlaceFrame,
                                         &m_pInPlaceUIWindow,
                                         &m_Rect,
                                         &rcClip,
                                         &m_FrameInfo));
    }

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleDocumentView::GetInPlaceSite()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::GetInPlaceSite(__out IOleInPlaceSite **ppInPlaceSite)
{
    HRESULT  hr = E_FAIL;

    CK_PARG(ppInPlaceSite)
    *ppInPlaceSite = m_pInPlaceSite;

    if (*ppInPlaceSite)
    {
        (*ppInPlaceSite)->AddRef();
        hr = S_OK;
    }

Cleanup:
    return hr;
}

//******************************************************************************
//
// COleDocumentView::GetDocument()
//  
//******************************************************************************

STDMETHODIMP COleDocumentView::GetDocument(__out IUnknown **ppUnk)
{
    HRESULT hr = S_OK;
    CK_PARG(ppUnk);
    *ppUnk = NULL;

    CKHR(m_pOleDoc->QueryInterface(IID_IUnknown, (void **)ppUnk));
Cleanup:
    return hr;
}

//******************************************************************************
//
// COleDocumentView::SetRect()
//   
//******************************************************************************

void COleDocumentView::MoveViewportHelper(int x, int y, int nWidth, int nHeight)
{
    // update the dimensions of the top dockobj hwnd using the dimensions provided

    // A 0x0 rect is passed to IOleDocumentView::SetRect() or IOleInPlaceObject::SetObjectRects()
    // (depending on the hosting mode) when the top-level window is minimized. This appears to be a bug
    // in the ATL host, which gets 0x0 in its WM_SIZE handler but ignores the SIZE_MINIMIZED flag.
    // IE probably does something similar. But normally child windows are not resized or moved when the
    // top-level window is minimized.
    if(nWidth > 0 || nHeight > 0)
    {
        // Drawing optimization (last param of MoveWindow): don't paint the background of the top window if the 
        // RBW is present. (In spite of the WS_CLIPCHILDREN style, this drawing does take place, at least 
        // during resizing.)
        MoveWindow(m_pOleDoc->m_hwndTop, x, y, nWidth, nHeight, !m_pOleDoc->ApplicationHelper());
        if(m_pOleDoc->ApplicationHelper())
        {
            // update dimensions of the RBW window.  Note, x,y of RBW should always be 
            // 0,0 relative to the top dockobj hwnd
            (m_pOleDoc->ApplicationHelper())->Move(0, 0, nWidth, nHeight);
        }

        CErrorPage::SetRect(x, y, nWidth, nHeight);
    }
}

STDMETHODIMP COleDocumentView::SetRect(__in LPRECT pRect)
{
    HRESULT hr = S_OK;

    CK_PARG(pRect);

    m_Rect = *pRect;

    // MoveViewportHelper expects x, y, width, height
    MoveViewportHelper(m_Rect.left, 
                       m_Rect.top, 
                       m_Rect.right - m_Rect.left, 
                       m_Rect.bottom - m_Rect.top);


Cleanup:

    return hr;
}

//******************************************************************************
//
// COleDocumentView::GetRect()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::GetRect(__out LPRECT pRect)
{
    HRESULT hr = S_OK;

    CK_PARG(pRect);

    (*pRect) = m_Rect;

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleDocumentView::SetRectComplex()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::SetRectComplex(LPRECT /* prcView */, 
                                              LPRECT /* prcHScroll */, 
                                              LPRECT /* prcVScroll */, 
                                              LPRECT /* prcSizeBox */)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// COleDocumentView::Show()
//   
//******************************************************************************
void COleDocumentView::ShowHelper(bool fShow)
{
    ShowWindow(m_pOleDoc->m_hwndTop,fShow);
    if(m_pOleDoc->ApplicationHelper())
    {
        (m_pOleDoc->ApplicationHelper())->Show(fShow);
    }
    else
    {
        //ASSERT(CErrorPage::IsActive());
        //SP2: Not true anymore. UI Activation now happens before loading any managed code.
    }

    // 
}

STDMETHODIMP COleDocumentView::Show(BOOL bShow)
{
    m_fShow = bShow;

    if (bShow)
    {
        //if the object is not in-place active, make it that way
        if (!m_fInPlaceActive)
        {
            ActivateInPlace();
        }

        ShowHelper(true);
    }
    else
    {
        //if the object is UI active, make remove that state
        if (m_fUIActive)
        {
            DeactivateUI();
        }

        //hide the window
        ShowHelper(false);
    }

    return S_OK;
}

//******************************************************************************
//
// COleDocumentView::UIActivate()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::UIActivate(BOOL bActivate)
{
    if (bActivate)
    {
        return ActivateUI();
    }

    return DeactivateUI();
}

//******************************************************************************
//
// COleDocumentView::ActivateUI()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::ActivateUI()
{
    HRESULT hr = S_OK;

    //set the active object either one of these could be good
    if (m_pInPlaceFrame)
    {
        m_pInPlaceFrame->SetActiveObject(m_pOleDoc->m_pOleInPlaceActiveObject, 
                                         NULL);
    }

    if (m_pInPlaceUIWindow)
    {
        m_pInPlaceUIWindow->SetActiveObject(m_pOleDoc->m_pOleInPlaceActiveObject, 
                                            NULL);
    }
          
    if (m_pInPlaceSite)
    {
        m_fUIActive = TRUE;

        ActivateInPlace();
              
        m_pInPlaceSite->OnUIActivate();

        CKHR(InsertMergedMenus());

        SetFocus(m_pOleDoc->m_hwndTop);
    }

Cleanup:

    return hr;
}

//******************************************************************************
//
// COleDocumentView::DeactivateUI()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::DeactivateUI()
{
    m_fUIActive = FALSE;

    RemoveMergedMenus();

    //remove the active object either one of these could be good
    if (m_pInPlaceFrame)
    {
        m_pInPlaceFrame->SetActiveObject(NULL, NULL);
    }

    if (m_pInPlaceUIWindow)
    {
        m_pInPlaceUIWindow->SetActiveObject(NULL, NULL);
    }

    if (m_pInPlaceSite)
    {
        m_pInPlaceSite->OnUIDeactivate(FALSE);
    }

    return S_OK;
}

//******************************************************************************
//
// COleDocumentView::Open()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::Open(void)
{
    return S_OK;
}

//******************************************************************************
//
// COleDocumentView::CloseView()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::CloseView(DWORD /* dwReserved */)
{
    SetInPlaceSite(NULL);
    return S_OK;
}

//******************************************************************************
//
// COleDocumentView::SaveViewState()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::SaveViewState(IStream * /* pStream */)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// COleDocumentView::ApplyViewState()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::ApplyViewState(IStream * /* pStream */)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// COleDocumentView::Clone()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::Clone(IOleInPlaceSite * /* pIPSite */, 
                                     IOleDocumentView ** /* ppView */)
{
    return E_NOTIMPL;
}

//******************************************************************************
//
// COleDocumentView::ActivateInPlace()
//   
//******************************************************************************
void COleDocumentView::SetParentHelper(HWND hwndParent)
{
    // if hwndParent == NULL We are going away
    if (hwndParent == NULL)
    {
        SetWindowLong(m_pOleDoc->m_hwndTop, GWL_STYLE, WS_OVERLAPPED | WS_CLIPSIBLINGS | WS_DISABLED);    
    }
    else
    {
        SetWindowLong(m_pOleDoc->m_hwndTop, GWL_STYLE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
    }

    //This will notify the managed window to set up the parent or cleanup during shutdown
    //If we don't detach the parent window during shutdown, we will end up processing WM_DESTROY
    //twice since we close windows when Application is shutdown which posts one msg and when the 
    //browser window is closed, the managed child window will get another one. 
    //Detach the parent window to avoid that.
    SetParent(m_pOleDoc->m_hwndTop, hwndParent);
    SetWindowPos(m_pOleDoc->m_hwndTop, 0,0,0,0,0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);

    // 
}

STDMETHODIMP COleDocumentView::ActivateInPlace()
{
    HWND hwndParent;

    m_fInPlaceActive = TRUE;

    if (m_pInPlaceSite) 
    {
        // tell the site we are in-place activating
        m_pInPlaceSite->OnInPlaceActivate();
        
        m_pInPlaceSite->GetWindow(&hwndParent);

        SetParentHelper(hwndParent);
    }

    return S_OK;
}

//******************************************************************************
//
// COleDocumentView::DeactivateInPlace()
//   
//******************************************************************************

STDMETHODIMP COleDocumentView::DeactivateInPlace()
{
    m_fInPlaceActive = FALSE;

    if (m_pInPlaceSite) 
    {
        // tell the site we are in-place deactivating
        m_pInPlaceSite->OnInPlaceDeactivate();
    }

    //set the parent to NULL
    SetParentHelper(NULL);

    return S_OK;
}


//******************************************************************************
//
// COleDocumentView::InsertMergedMenus()
//   
//******************************************************************************

#define MSGFLT_ADD 1
#define MSGFLT_REMOVE 2
typedef BOOL (WINAPI *PFChangeWindowMessageFilter)(__in UINT message, __in DWORD dwFlag);
typedef BOOL (WINAPI *PFChangeWindowMessageFilterEx)(HWND hwnd, UINT message, DWORD action, __inout_opt void* /*PCHANGEFILTERSTRUCT*/ pChangeFilterStruct); 

BOOL WINAPI CallOldChangeWindowMessageFilter(HWND hwnd, UINT message, DWORD action, __inout_opt void* /*PCHANGEFILTERSTRUCT*/ pChangeFilterStruct)
{
    static PFChangeWindowMessageFilter pfCWMF;
    if(!pfCWMF)
    {
        GetProcAddress(L"User32.dll", "ChangeWindowMessageFilter", &pfCWMF);
    }
    return (*pfCWMF)(message, action);
}

HRESULT COleDocumentView::InsertMergedMenus()
{
    EventWriteWpfHostUm_MergingMenusStart();

    HRESULT hr = S_OK;

    HWND    hwnd = NULL;

    IProtectedModeMenuServices *pFrameMenuServices = NULL;
    WCHAR*  szText  = NULL;
    HMENU   hMenuToAdd   = NULL;
    int menuItemCount = 0;
    
    CHECK_POINTER(m_pInPlaceFrame);

    // IE7's interface for creating and loading menus in Protected Mode (ONLY)
    m_pInPlaceFrame->QueryInterface(IID_IProtectedModeMenuServices, (void**)&pFrameMenuServices);

    //Create an empty menu
    Assert(m_hSharedMenu == NULL);
    if(!pFrameMenuServices)
    {
        m_hSharedMenu = CreateMenu();
        CK_ALLOC(m_hSharedMenu);
    }
    else
    {
        CKHR(pFrameMenuServices->CreateMenu(&m_hSharedMenu));

        // Let the Low Integrity IE process send us WM_MENUSELECT & WM_COMMAND.
        // We prefer ChangeWindowMessageFilterEx(), which is new to Windows 7.
        PFChangeWindowMessageFilterEx pfCWMF = 0;
        GetProcAddress(L"User32.dll", "ChangeWindowMessageFilterEx", &pfCWMF);
        if(!pfCWMF)
        {
            pfCWMF = CallOldChangeWindowMessageFilter;
        }
        HWND hwnd = m_pOleDoc->m_hwndTop;
        BOOL res = (*pfCWMF)(hwnd, WM_MENUSELECT, MSGFLT_ADD, 0);
        ASSERT(res); (void)res;
        (*pfCWMF)(hwnd, WM_INITMENUPOPUP, MSGFLT_ADD, 0);
        (*pfCWMF)(hwnd, WM_COMMAND, MSGFLT_ADD, 0);
    }

    //Zero out the merged menu array per OLE spec
    ZeroMemory(&m_MenuGroupWidth, sizeof(m_MenuGroupWidth));

    //Call container to insert its menus
    CKHR(m_pInPlaceFrame->InsertMenus(m_hSharedMenu, &m_MenuGroupWidth));

    // Remove the "Go To" menu
    // The items under "home" on this menu are added later by the browser frame.  If we 
    // remove this item from the menu they will not be added because the browser cannot
    // find the menu.
    // CKHR(RemoveMenu(m_hSharedMenu, 33104, MF_BYCOMMAND));
  
    // Load DocObj menus
    if(!pFrameMenuServices)
    {
        CKHR(LoadMenuFromMuiResource(&m_hDocObjMenu));
    }
    else
    {
        wchar_t dllPath[MAX_PATH];
        CHECK_NULL_FROM_WIN32(GetModuleFileName(g_hInstance, dllPath, MAX_PATH));
        CKHR(pFrameMenuServices->LoadMenuID(
                dllPath, GetMenuResourceId(m_pOleDoc->GetCurrentMime()), &m_hDocObjMenu));
    }
    
    CHECK_ZERO_FROM_WIN32(menuItemCount = GetMenuItemCount(m_hDocObjMenu));
    szText = new WCHAR[MAX_PATH+1];
    CK_ALLOC(szText);
    
    // Add each menu in our resources to the shared menu.  Adding them in reverse order ensures that they
    // appear in a logical order when displayed
    for (int menuItemIndex = (menuItemCount-1); menuItemIndex > -1; menuItemIndex--)
    {
        // get the menu from the loaded set of menus
        hMenuToAdd = GetSubMenu(m_hDocObjMenu, menuItemIndex);
        CHECK_POINTER(hMenuToAdd);

        // If this is our 'View' menu
        if (menuItemIndex == 1) 
        {
            // keep track of the view menu so we can check/uncheck the "Status Bar" and "Full Screen" items.
            m_hViewMenu = hMenuToAdd;
            
            // Determine the check state of the Status Bar and Full Screen menu options
            SetViewMenuItemsCheckState(hMenuToAdd);
        }

        // Add the menu to the shared menus using the name it was given in the resource
        CHECK_ZERO_FROM_WIN32(GetMenuString(m_hDocObjMenu, menuItemIndex, szText, MAX_PATH+1, MF_BYPOSITION));
        CHECK_BOOL_FROM_WIN32(InsertMenu(m_hSharedMenu, 
                                    m_MenuGroupWidth.width[0],
                                    MF_BYPOSITION | MF_POPUP,
                                    (UINT_PTR) hMenuToAdd,
                                    szText));
        
        // update the OLEMENUGROUPWIDTHS
        m_MenuGroupWidth.width[1] += 1;
    }

    //  Get the Ole menu descriptor
    CHECK_NULL_FROM_WIN32(m_hOleMenu = OleCreateMenuDescriptor(m_hSharedMenu, &m_MenuGroupWidth));

    //
    // Set the Menu on the container's frame
    //
    // The window passed here should be the one from our IOleInPlaceActiveObject->GetWindow
    // As long as we are a single-instance server, this is okay.
    // If not, we need to create a window per View/InplaceActiveObj to handle messages from this menu
    CKHR(m_pOleDoc->GetWindow(&hwnd));
    CKHR(m_pInPlaceFrame->SetMenu(m_hSharedMenu, m_hOleMenu, hwnd));

    /* //Testing - Adding a menu item to container owned menu group
    // Can Add successfully, but we don't get QueryStatus and Exec calls for it
    {
        hSubMenu = GetSubMenu(m_hSharedMenu,0);
        int nItems = GetMenuItemCount(hSubMenu);
        
        MENUITEMINFOW mii = { sizeof(mii) };

        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.wID = IDM_MENU_LAST+100;
        mii.fType = MFT_RADIOCHECK | MFT_STRING;
        mii.dwTypeData = L"Test";
        mii.cch = lstrlenW(L"Test");

        BOOL result;
        result = InsertMenuItemW(hSubMenu, nItems, TRUE, &mii);
    } */

    /* // TESTING Disable a menu item on a container owned menu
    //Can enable/disable container owned menus using Windows APIs incase we don't get QueryStatus
    //but we cannot handle the WM_COMMAND for these menu items
    {
    hSubMenu = GetSubMenu(m_hSharedMenu,0);

    EnableMenuItem(hSubMenu, 0, MF_BYPOSITION | MF_GRAYED);

    m_pInPlaceFrame->SetMenu(m_hSharedMenu, m_hOleMenu, m_pOleDoc->m_hwndTop); 
    } */

    EventWriteWpfHostUm_MergingMenusEnd();

Cleanup:

    ReleaseInterface(pFrameMenuServices);
    if (FAILED(hr))
    {
        DestroyMenuResources();
    }
    delete [] szText;

    return hr;
}

//******************************************************************************
//
// COleDocumentView::LoadMenuFromMuiResource()
//   
//******************************************************************************
STDMETHODIMP 
COleDocumentView::LoadMenuFromMuiResource(__out_ecount(1) HMENU * menu)
{
    HRESULT hr = S_OK;
    HINSTANCE hResourceInstance = LoadResourceDLL();
    CHECK_POINTER(hResourceInstance);

    //Load the docobj menus for our mime type. hr is set to S_OK or HRESULT_FROM_WIN32(GetLastError())
    Assert(m_hDocObjMenu == NULL);
    *menu = LoadMenu(hResourceInstance, MAKEINTRESOURCE(GetMenuResourceId(m_pOleDoc->GetCurrentMime())));    
    CHECK_POINTER(*menu);

Cleanup:
    FreeMUILibrary(hResourceInstance);
    return hr;
}

//******************************************************************************
//
// COleDocumentView::SetViewMenuItemsCheckState()
//   
//******************************************************************************
STDMETHODIMP 
COleDocumentView::SetViewMenuItemsCheckState(HMENU viewMenu)
{
    HRESULT hr = S_OK;
    IWebBrowser2        * pWebBrowserTop    = NULL;
    VARIANT_BOOL varBool;
    UINT uCheck;

    CKHR(GetTopLevelBrowser(m_pOleDoc->GetClientSite(), &pWebBrowserTop));

    // check Status Bar
    CKHR(pWebBrowserTop->get_StatusBar(&varBool));
    uCheck = (varBool == VARIANT_TRUE ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(viewMenu, IDM_VIEW_STATUSBAR, MF_BYCOMMAND | uCheck);

    // check View Full Screen
    CKHR(pWebBrowserTop->get_TheaterMode(&varBool));
    uCheck = (varBool == VARIANT_TRUE ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(viewMenu, IDM_VIEW_FULLSCREEN, MF_BYCOMMAND | uCheck);

Cleanup:
    ReleaseInterface(pWebBrowserTop);
    return hr;
}

//******************************************************************************
//
// COleDocumentView::RemoveMergedMenus()
//   
//******************************************************************************
void COleDocumentView::RemoveMergedMenus()
{
    int menuItemCount = 0;

    if (m_hSharedMenu == NULL)
    {
        Assert(m_hDocObjMenu == NULL);
        Assert(m_hOleMenu == NULL);
        return;
    }

    //Remove the DocObj menus first
    menuItemCount = GetMenuItemCount(m_hDocObjMenu);
    for (int index=0; index < menuItemCount; index++)
    {
        RemoveMenu(m_hSharedMenu, m_MenuGroupWidth.width[0], MF_BYPOSITION);
    }

    //Remove the container's menus next
    m_pInPlaceFrame->RemoveMenus(m_hSharedMenu);

    //Destroy resources since we are not going to reuse the menu handles
    DestroyMenuResources();
}

void COleDocumentView::DestroyMenuResources()
{
    //Destroy the shared menu handle
    if (m_hSharedMenu)
    {
        DestroyMenu(m_hSharedMenu);
        m_hSharedMenu = NULL;
    }

    //Destroy our local docobj menu handle
    if (m_hDocObjMenu)
    {
        DestroyMenu(m_hDocObjMenu);
        m_hDocObjMenu = NULL;
        // The 'view' menu is just part of m_hDocObjMenu and 
        // does not need to be destroyed separately.
        m_hViewMenu = NULL;
    }

    //Destroy the Ole menu descriptor
    if (m_hOleMenu)
    {
        OleDestroyMenuDescriptor(m_hOleMenu);
        m_hOleMenu = NULL;
    }
}
