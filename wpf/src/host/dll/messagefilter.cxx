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

#include "Precompiled.hxx"
#include "MessageFilter.hxx"

void CMessageFilter::Init(DWORD maxRetryMilliseconds)
{
    m_dwMaxRetryMilliseconds = maxRetryMilliseconds;
}

STDMETHODIMP_(DWORD) CMessageFilter::HandleInComingCall(__in DWORD dwCallType, __in HTASK threadIDCaller, __in DWORD dwTickCount, __in_opt LPINTERFACEINFO lpInterfaceInfo)
{
    return SERVERCALL_ISHANDLED;
}

STDMETHODIMP_(DWORD) CMessageFilter::RetryRejectedCall(__in HTASK threadIDCallee, __in DWORD dwTickCount, __in DWORD dwRejectType)
{
    // See Dev10 bug 754024 - Crash hosting an XBAP inside another XBAP via a WebBrowser.Navigate
    // During our activation sequence, the browser is making a QueryStatus call, which is input-synchronous. If at the same time,
    // we call into the browser, an RPC_E_CALL_REJECT failure results. Therefore we use a message filter to ask the COM runtime
    // to retry making this call (with an upper retry duration limit as specified in m_dwMaxRetryMilliseconds).
    // Another case can be found in Dev10 bug 794667 - WebBrowser: multiple fast navigations = COM exception
    if ((dwRejectType == SERVERCALL_RETRYLATER || dwRejectType == SERVERCALL_REJECTED) && dwTickCount < m_dwMaxRetryMilliseconds)
    {
        // 100 ms is lower boundary for timed retry
        return 100;
    }

    // Give up - will cause an RPC_E_CALL_REJECTED failure.
    return (DWORD)-1;
}

STDMETHODIMP_(DWORD) CMessageFilter::MessagePending(__in HTASK threadIDCallee, __in DWORD dwTickCount, __in DWORD dwPendingType)
{
    return PENDINGMSG_WAITDEFPROCESS;
}

HRESULT CMessageFilter::Register(DWORD maxRetryMilliseconds)
{
    CComObject<CMessageFilter> *pMessageFilter;

    HRESULT hr = CComObject<CMessageFilter>::CreateInstance(&pMessageFilter);
    if (SUCCEEDED(hr))
    {
        pMessageFilter->Init(maxRetryMilliseconds);

        CComPtr<IMessageFilter> spOldMessageFilter;
        hr = CoRegisterMessageFilter(pMessageFilter, &spOldMessageFilter);

        ASSERT(!spOldMessageFilter);
    }

    return hr;
}

void CMessageFilter::Unregister()
{
    CComPtr<IMessageFilter> spOldMessageFilter;
    CoRegisterMessageFilter(NULL, &spOldMessageFilter);
}


