#pragma once

class ATL_NO_VTABLE VbCompilerHost :
    public IVbCompilerHost, //inherits from IVbCompilerHost
    public CComObjectRootEx<CComSingleThreadModel>
#if FV_DEADBEEF
    , public Deadbeef<VbCompilerHost> // Must be last base class!
#endif
{
    BEGIN_COM_MAP(VbCompilerHost)
        COM_INTERFACE_ENTRY(IVbCompilerHost)
    END_COM_MAP()

public:
    static STDMETHODIMP Create(IVbCompilerHost **ppVbCompilerHost);

    //IVbCompilerHost
    STDMETHODIMP OutputString(LPCWSTR Output);
    STDMETHODIMP GetSdkPath(BSTR *pSdkPath);
    STDMETHODIMP GetTargetLibraryType(VBTargetLibraryType *pTargetLibraryType);

};
