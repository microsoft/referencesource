//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the ErrorPage class of PresentationHost.
//
//  History:
//     2005/10/10 - Microsoft
//          Created
//     2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once 

class CErrorPage
{
public:
    // Display() returns only if called on the main thread and the browser is currently blocked.
    static void Display(__in_ecount(1) BSTR pErrorMsg);
    static bool IsActive() { return s_fIsActive != 0; }

    static void SetRect(int x, int y, int width, int height);
    static bool ExecOleCommand(__in_opt const GUID * pguidCmdGroup, DWORD dwCmdId, DWORD /*nCmdExecOpt*/,
                   __in_opt VARIANT * /*pvarArgIn*/, __out_opt VARIANT * /*pvarArgOut*/);

    static HRESULT RestartApplication();

private:
    __declspec(align(4)) static volatile LONG s_fIsActive;
    static DWORD s_threadId;
    static HWND s_hWnd, s_hRBW;

    static void DisplayOnAnyThread(__in_ecount(1) BSTR pErrorMsg);
    static void DisplayOnAnyThreadImpl(__in_ecount(1) BSTR pErrorMsg);

    static HRESULT LoadAndFixUpHtmlTemplate(__out _bstr_t &htmlTemplate);
    static bool GetShowDetailsRegistryFlag();
    static void DisableTextAreaCloseTag(__inout wchar_t *str);
    static _bstr_t ListLoadedModules();

    static HRESULT SetUpHostWindow();
    static LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
