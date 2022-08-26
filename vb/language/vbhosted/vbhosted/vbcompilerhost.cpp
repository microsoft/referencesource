#include "stdAfx.h"

STDMETHODIMP VbCompilerHost::Create(IVbCompilerHost **ppVbCompilerHost)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyOutPtr(ppVbCompilerHost, "[VbCompilerHost::Create] 'ppVbCompilerHost' parameter is null");

    HRESULT hr = S_OK;

    CComObject<VbCompilerHost> *pVbCompilerHost = NULL;

    *ppVbCompilerHost = NULL;
    
    IfFailGo(CComObject<VbCompilerHost>::CreateInstance(&pVbCompilerHost));
    
    pVbCompilerHost->AddRef();
    
    IfFailGo(pVbCompilerHost->QueryInterface(IID_IVbCompilerHost, reinterpret_cast<void**>(ppVbCompilerHost)));

Error:

    if(pVbCompilerHost)
    {
        pVbCompilerHost->Release();
    }

    return hr;
}

STDMETHODIMP VbCompilerHost::OutputString(LPCWSTR Output)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(Output, "[VbCompilerHost::OutputString] 'Output' parameter is null");

    ASSERT(false, "OutputString was called");

    return S_OK;
}


STDMETHODIMP VbCompilerHost::GetSdkPath(BSTR *pSdkPath)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyOutPtr(pSdkPath, "[VbCompilerHost::GetSdkPath] 'pSdkPath' parameter is null");

    *pSdkPath = ::GetNDPSystemPath();

    return (*pSdkPath == NULL) ? E_OUTOFMEMORY : S_OK;
}

STDMETHODIMP VbCompilerHost::GetTargetLibraryType(VBTargetLibraryType *pTargetLibraryType)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyOutPtr(pTargetLibraryType, "[VbCompilerHost::GetTargetLibraryType] 'pTargetLibraryType' parameter is null");

    *pTargetLibraryType = TLB_Desktop;

    return S_OK;
}

#pragma pop(new)
#pragma pop(delete)

