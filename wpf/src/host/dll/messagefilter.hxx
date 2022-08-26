// +-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements a COM message filter to allow retry for RPC calls.
//
//  History:
//     2009/08/24   Microsoft      Created
//
// ------------------------------------------------------------------------

// This filter implementation was motivated in the Dev10 timeframe when we encountered
// or uncovered issues related to rejected RPC calls when PresentationHost and browser
// processes are communicating. Two bugs are worth referencing here:
//
// - Dev10 bug 754024 - Crash hosting an XBAP inside another XBAP via a WebBrowser.Navigate
// - Dev10 bug 794667 - WebBrowser: multiple fast navigations = COM exception
//
// Other conceptually related filters are implemented and can be found in DllMain.cxx,
// in particular:
//
// - CMessageFilterMarshaler
// - CMessageFilterDelegate
// - CInPlaceFrameTranslateAcceleratorDelegate
// - CWpfHostSupportFilterInputMessageDelegate
//
// This code hasn't been relocated due to quite some entanglement with it surroundings.

#pragma once 

class ATL_NO_VTABLE CMessageFilter :
    public CComObjectRoot,
    public IMessageFilter
{
public:

    void Init(DWORD maxRetryMilliseconds);

    BEGIN_COM_MAP(CMessageFilter)
        COM_INTERFACE_ENTRY(IMessageFilter)
    END_COM_MAP()

    STDMETHOD_(DWORD, HandleInComingCall) (__in DWORD dwCallType, __in HTASK threadIDCaller, __in DWORD dwTickCount, __in_opt LPINTERFACEINFO lpInterfaceInfo);
    STDMETHOD_(DWORD, RetryRejectedCall)  (__in HTASK threadIDCallee, __in DWORD dwTickCount, __in DWORD dwRejectType);
    STDMETHOD_(DWORD, MessagePending)     (__in HTASK threadIDCallee, __in DWORD dwTickCount, __in DWORD dwPendingType);

    static HRESULT Register(DWORD maxRetryMilliseconds);
    static void Unregister();

private:
    DWORD m_dwMaxRetryMilliseconds;
};
