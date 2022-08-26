//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Template Utilities 
//
//-------------------------------------------------------------------------------------------------

#pragma once

class ComUtil 
{
public:
    template <class T>
    static 
    HRESULT CreateWithRef(T** ppObject)
    {
        CComObject<T> *pObject;
        HRESULT hr = CComObject<T>::CreateInstance(&pObject);
        if ( SUCCEEDED(hr) )
        {
            pObject->AddRef();
            *ppObject = pObject;
        }

        return hr; 
    }

    template <class T>
    static
    HRESULT CreateWithRef( CComPtrBase<T>& spObject )
    {
        CComObject<T> *pObject;
        HRESULT hr = CComObject<T>::CreateInstance(&pObject);
        if ( SUCCEEDED(hr) )
        {
            pObject->AddRef();
            spObject.Attach(pObject);
        }

        return hr; 
    }

    template <class T>
    static
    CComPtr<T> CreateWithRef()
    {
        CComPtr<T> spT;
        IfFailThrow( CreateWithRef( spT ) );
        return spT;
    }

    template <class T>
    static
    CComPtrEx<T> CreateWithRefEx()
    {
        CComPtrEx<T> spT;
        IfFailThrow( CreateWithRef( spT) );
        return spT;
    }

    template <class T, class TInterface>
    static
    HRESULT CreateWithRefQI( _Out_ TInterface **ppInterface )
    {
        CComPtr<T> spSpecific;
        HRESULT hr = CreateWithRef<T>( spSpecific );
        if ( SUCCEEDED(hr) )
        {
            hr = QI(spSpecific, ppInterface);
        }

        return hr;
    }

    template <class T, class TInterface, class TParam1>
    static
    HRESULT CreateWithInitRefQI( TParam1 tp1, _Out_ TInterface **ppInterface )
    {
        CComPtr<T> spSpecific;
        HRESULT hr = CreateWithRef<T>( spSpecific );
        if ( SUCCEEDED(hr) )
        {
            spSpecific->Init(tp1);
            hr = QI(spSpecific, ppInterface);
        }

        return hr;
    }

    template <class T>
    static
    HRESULT QI(
        _In_ IUnknown *pUnknown,
        _Out_ T** ppType )
    {
        return pUnknown->QueryInterface(__uuidof(T), (void**)ppType);
    }

    template <class T>
    static 
    HRESULT QI(
        _In_ IUnknown *pUnknown,
        _In_ GUID iid,
        _Out_ T** ppType)
    {
        return pUnknown->QueryInterface(iid, (void**)ppType);
    }

    template <class T>
    static
    CComPtr<T> QI( _In_ IUnknown* pUnk )
    {
        CComPtr<T> spT;
        IfFailThrow(QI(pUnk, &spT));
        return spT;
    }

    //-------------------------------------------------------------------------------------------------
    //
    // The IUnknown implementation is templated to fix the same problem CComPtrEx<> fixes.  If you have
    // a class which inherits from more than one interface, there are multiple paths to IUnknown and 
    // hence you cannot implicitly cast to IUnknown.  Using a template avoids this problem
    //
    //-------------------------------------------------------------------------------------------------
    template <class TSource, class TDest>
    static
    HRESULT QI(
        _In_ const CComPtrBase<TSource>& spUnknown,
        _Out_ TDest** ppType)
    {
        return spUnknown->QueryInterface(__uuidof(TDest), (void**)(ppType));
    }

    template <class TSource, class TDest>
    static 
    HRESULT QI(
        _In_ const CComPtrBase<TSource>& spUnknown,
        _In_ GUID iid,
        _Out_ TDest** ppType)
    {
        return spUnknown->QueryInterface(iid, (void**)ppType);
    }

    template <class TSource, class TDest>
    static
    void CopyTo(
        _In_ const CComPtrBase<TSource>& spSource,
        _Out_ TDest** ppType)
    {
        ThrowIfNull(ppType);
        *ppType = spSource;
        (*ppType)->AddRef();
    }

    template <class T>
    static 
    HRESULT ReleaseAll(
        _In_count_(count) T *pUnknownArray,
        _In_ size_t count)
    {
        for ( size_t cur = 0; cur < count; ++i )
        {
            pUnknownArray[cur]->Release();
        }
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Safer version of SysAllocString.  This will throw if sufficient memory does not exist
    //
    //-------------------------------------------------------------------------------------------------
    static
    BSTR SafeSysAllocString(_In_opt_z_ const OLECHAR *pSource)
    {
        BSTR bstr = ::SysAllocString(pSource);
        if ( !bstr && pSource)
        {
            // Onlything valid here is a "".  Otherwise it is a OOM scenario
            if ( L'\0' != *pSource )
            {
                VbThrow(E_OUTOFMEMORY);
            }
        }

        return bstr;
    }

    //-------------------------------------------------------------------------------------------------
    //
    // Useful when you have a type which must be used with CComPtrEx<> but you need to actually 
    // convert to a CComPtr<>.  
    //
    //-------------------------------------------------------------------------------------------------
    template <typename T>
    static void WrapInSmartPtr( _In_opt_ T* pSource, _Out_ CComPtr<T>& spDest)
    {
        CComPtrEx<T> sp(pSource);
        TemplateUtil::Swap(&sp.p, &spDest.p);
    }

};
