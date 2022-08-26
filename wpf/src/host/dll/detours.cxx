//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//      [See .hxx]
//
//------------------------------------------------------------------------

#include "precompiled.hxx"
#include "Detours.hxx"
#include "..\Detours\Detours.h"

// The managed PresentationHostSecurityManager needs to convince the ClickOnce elevation prompt to use the
// browser's top-level window as the owner. The prompt dialog is created without an explicit owner, on its
// own thread. As a fallback, SWF.Form.ShowDialog() uses the active window as owner. But there is no other
// window on the dialog's thread. So, we fake it by detouring ::GetActiveWindow() and returning the browser's
// top-level window on the first call.
HWND g_fakeActiveWindow;

// DLL-exported. Called by PresentationHostSecurityManager.
void __stdcall SetFakeActiveWindow(HWND hwnd)
{
    g_fakeActiveWindow = hwnd;
}

HWND (WINAPI *Detours::s_pfGetActiveWindow)() = &GetActiveWindow;

HRESULT Detours::Init()
{
    HRESULT hr = S_OK;

    CHECK_ERROR_CODE(DetourTransactionBegin());
    CHECK_ERROR_CODE(DetourUpdateThread(GetCurrentThread()));
    CHECK_ERROR_CODE(DetourAttach((void**)&s_pfGetActiveWindow, GetActiveWindowDetour));
    CHECK_ERROR_CODE(DetourTransactionCommit());

Cleanup:
    return hr;
}

void Detours::Uninit()
{
    if(s_pfGetActiveWindow != GetActiveWindow) // detoured?
    {
        if(DetourTransactionBegin() == NOERROR &&
            DetourUpdateThread(GetCurrentThread()) == NOERROR &&
            DetourDetach((void**)&s_pfGetActiveWindow, GetActiveWindowDetour) == NOERROR)
        {
            DetourTransactionCommit();
        }
        else
        {
            ASSERT(false);
        }
    }
}

HWND Detours::GetActiveWindowDetour()
{
    if(g_fakeActiveWindow)
    {
        HWND hwnd = g_fakeActiveWindow;
        g_fakeActiveWindow = 0;
        return hwnd;
    }
    return (*s_pfGetActiveWindow)(); // call the real function
}

