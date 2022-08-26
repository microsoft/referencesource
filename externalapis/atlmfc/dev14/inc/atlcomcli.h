// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCOMCLI_H__
#define __ATLCOMCLI_H__

#pragma once

#include <atlcore.h>
#include <ole2.h>
#ifndef _ATL_USE_WINAPI_FAMILY_PHONE_APP
#include <olectl.h>
#endif // _ATL_USE_WINAPI_FAMILY_PHONE_APP

#pragma warning (push)
#pragma warning (disable: 4127)  // conditional expression constant
#pragma warning (disable: 4510)  // compiler cannot generate default constructor
#pragma warning (disable: 4571)  //catch(...) blocks compiled with /EHs do NOT catch or re-throw Structured Exceptions
#pragma warning (disable: 4610)  // class has no user-defined or default constructors

#pragma pack(push,_ATL_PACKING)
namespace ATL
{
/////////////////////////////////////////////////////////////////////////////
// Error to HRESULT helpers

_Post_satisfies_(FAILED(return))
ATL_NOINLINE inline HRESULT AtlHresultFromLastError() throw()
{
    DWORD dwErr = ::GetLastError();
    return HRESULT_FROM_WIN32(dwErr);
}

ATL_NOINLINE inline
_When_(nError == 0, _Post_equal_to_(0)) _When_(nError != 0, _Post_satisfies_(return < 0))
HRESULT AtlHresultFromWin32(_In_ DWORD nError) throw()
{
    return( HRESULT_FROM_WIN32( nError ) );
}

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLAPI_(IUnknown*) AtlComPtrAssign(
    _Inout_opt_ _Deref_pre_maybenull_ _Deref_post_maybenull_ IUnknown** pp,
    _In_opt_ IUnknown* lp);

ATLAPI_(IUnknown*) AtlComQIPtrAssign(
    _Inout_opt_ _Deref_pre_maybenull_ _Deref_post_maybenull_ IUnknown** pp,
    _In_opt_ IUnknown* lp,
    _In_ REFIID riid);

/////////////////////////////////////////////////////////////////////////////
// Safe Ole Object Reading

union ClassesAllowedInStream
{
    const CLSID *rgclsidAllowed;
    HRESULT (*pfnClsidAllowed)(
        _In_ const CLSID& clsid,
        _In_ REFIID iidInterface,
        _Outptr_result_maybenull_ void** ppvObj);
};

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

_Check_return_ inline HRESULT AtlInternalOleLoadFromStream(
    _Inout_ IStream* pStm,
    _In_ REFIID iidInterface,
    _Outptr_ void** ppvObj,
    _In_ ClassesAllowedInStream rgclsidAllowed,
    _In_ DWORD cclsidAllowed);

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

ATLINLINE ATLAPI_(IUnknown*) AtlComPtrAssign(
    _Inout_opt_ _Deref_pre_maybenull_ _Deref_post_maybenull_ IUnknown** pp,
    _In_opt_ IUnknown* lp)
{
    if (pp == NULL)
        return NULL;

    if (lp != NULL)
        lp->AddRef();
    if (*pp)
        (*pp)->Release();
    *pp = lp;
    return lp;
}

ATLINLINE ATLAPI_(IUnknown*) AtlComQIPtrAssign(
    _Inout_opt_ _Deref_pre_maybenull_ _Deref_post_maybenull_ IUnknown** pp,
    _In_opt_ IUnknown* lp,
    _In_ REFIID riid)
{
    if (pp == NULL)
        return NULL;

    IUnknown* pTemp = *pp;

    if (lp == NULL || FAILED(lp->QueryInterface(riid, (void**)pp)))
        *pp = NULL;

    if (pTemp)
        pTemp->Release();
    return *pp;
}

/////////////////////////////////////////////////////////////////////////////
// COM Smart pointers

template <class T>
class _NoAddRefReleaseOnCComPtr :
    public T
{
    private:
        STDMETHOD_(ULONG, AddRef)()=0;
        STDMETHOD_(ULONG, Release)()=0;
};

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

_Check_return_ inline HRESULT AtlSetChildSite(
    _Inout_ IUnknown* punkChild,
    _Inout_opt_ IUnknown* punkParent)
{
    if (punkChild == NULL)
        return E_POINTER;

    HRESULT hr;
    IObjectWithSite* pChildSite = NULL;
    hr = punkChild->QueryInterface(__uuidof(IObjectWithSite), (void**)&pChildSite);
    if (SUCCEEDED(hr) && pChildSite != NULL)
    {
        hr = pChildSite->SetSite(punkParent);
        pChildSite->Release();
    }
    return hr;
}

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

//CComPtrBase provides the basis for all other smart pointers
//The other smartpointers add their own constructors and operators
template <class T>
class CComPtrBase
{
protected:
    CComPtrBase() throw()
    {
        p = NULL;
    }
    CComPtrBase(_Inout_opt_ T* lp) throw()
    {
        p = lp;
        if (p != NULL)
            p->AddRef();
    }
    void Swap(CComPtrBase& other)
    {
        T* pTemp = p;
        p = other.p;
        other.p = pTemp;
    }
public:
    typedef T _PtrClass;
    ~CComPtrBase() throw()
    {
        if (p)
            p->Release();
    }
    operator T*() const throw()
    {
        return p;
    }
    T& operator*() const
    {
        ATLENSURE(p!=NULL);
        return *p;
    }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the p member explicitly.
    T** operator&() throw()
    {
        ATLASSERT(p==NULL);
        return &p;
    }
    _NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
    {
        ATLASSERT(p!=NULL);
        return (_NoAddRefReleaseOnCComPtr<T>*)p;
    }
    bool operator!() const throw()
    {
        return (p == NULL);
    }
    bool operator<(_In_opt_ T* pT) const throw()
    {
        return p < pT;
    }
    bool operator!=(_In_opt_ T* pT) const
    {
        return !operator==(pT);
    }
    bool operator==(_In_opt_ T* pT) const throw()
    {
        return p == pT;
    }

    // Release the interface and set to NULL
    void Release() throw()
    {
        T* pTemp = p;
        if (pTemp)
        {
            p = NULL;
            pTemp->Release();
        }
    }
    // Compare two objects for equivalence
    inline bool IsEqualObject(_Inout_opt_ IUnknown* pOther) throw();

    // Attach to an existing interface (does not AddRef)
    void Attach(_In_opt_ T* p2) throw()
    {
        if (p)
        {
            ULONG ref = p->Release();
            (ref);
            // Attaching to the same object only works if duplicate references are being coalesced.  Otherwise
            // re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
            ATLASSERT(ref != 0 || p2 != p);
        }
        p = p2;
    }
    // Detach the interface (does not Release)
    T* Detach() throw()
    {
        T* pt = p;
        p = NULL;
        return pt;
    }
    _Check_return_ HRESULT CopyTo(_COM_Outptr_result_maybenull_ T** ppT) throw()
    {
        ATLASSERT(ppT != NULL);
        if (ppT == NULL)
            return E_POINTER;
        *ppT = p;
        if (p)
            p->AddRef();
        return S_OK;
    }
    _Check_return_ HRESULT SetSite(_Inout_opt_ IUnknown* punkParent) throw()
    {
        return AtlSetChildSite(p, punkParent);
    }
    _Check_return_ HRESULT Advise(
        _Inout_ IUnknown* pUnk,
        _In_ const IID& iid,
        _Out_ LPDWORD pdw) throw()
    {
        return AtlAdvise(p, pUnk, iid, pdw);
    }
    _Check_return_ HRESULT CoCreateInstance(
        _In_ REFCLSID rclsid,
        _Inout_opt_ LPUNKNOWN pUnkOuter = NULL,
        _In_ DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        ATLASSERT(p == NULL);
        return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
    }
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    _Check_return_ HRESULT CoCreateInstance(
        _In_z_ LPCOLESTR szProgID,
        _Inout_opt_ LPUNKNOWN pUnkOuter = NULL,
        _In_ DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        CLSID clsid;
        HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
        ATLASSERT(p == NULL);
        if (SUCCEEDED(hr))
            hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
        return hr;
    }
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    template <class Q>
    _Check_return_ HRESULT QueryInterface(_Outptr_ Q** pp) const throw()
    {
        ATLASSERT(pp != NULL);
        return p->QueryInterface(__uuidof(Q), (void**)pp);
    }
    T* p;
};

template <class T>
class CComPtr :
    public CComPtrBase<T>
{
public:
    CComPtr() throw()
    {
    }
    CComPtr(_Inout_opt_ T* lp) throw() :
        CComPtrBase<T>(lp)
    {
    }
    CComPtr(_Inout_ const CComPtr<T>& lp) throw() :
        CComPtrBase<T>(lp.p)
    {
    }
    T* operator=(_Inout_opt_ T* lp) throw()
    {
        if(*this!=lp)
        {
            CComPtr(lp).Swap(*this);
        }
        return *this;
    }
    template <typename Q>
    T* operator=(_Inout_ const CComPtr<Q>& lp) throw()
    {
        if( !this->IsEqualObject(lp) )
        {
            return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&this->p, lp, __uuidof(T)));
        }
        return *this;
    }
    T* operator=(_Inout_ const CComPtr<T>& lp) throw()
    {
        if(*this!=lp)
        {
            CComPtr(lp).Swap(*this);
        }
        return *this;
    }
    CComPtr(_Inout_ CComPtr<T>&& lp) throw() :
        CComPtrBase<T>()
    {
        lp.Swap(*this);
    }
    T* operator=(_Inout_ CComPtr<T>&& lp) throw()
    {
        if (*this != lp)
        {
            CComPtr(static_cast<CComPtr&&>(lp)).Swap(*this);
        }
        return *this;
    }
};

//specialization for IDispatch
template <>
class CComPtr<IDispatch> :
    public CComPtrBase<IDispatch>
{
public:
    CComPtr() throw()
    {
    }
    CComPtr(_Inout_opt_ IDispatch* lp) throw() :
        CComPtrBase<IDispatch>(lp)
    {
    }
    CComPtr(_Inout_ const CComPtr<IDispatch>& lp) throw() :
        CComPtrBase<IDispatch>(lp.p)
    {
    }
    IDispatch* operator=(_Inout_opt_ IDispatch* lp) throw()
    {
        if(*this!=lp)
        {
            CComPtr(lp).Swap(*this);
        }
        return *this;
    }
    IDispatch* operator=(_Inout_ const CComPtr<IDispatch>& lp) throw()
    {
        if(*this!=lp)
        {
            CComPtr(lp).Swap(*this);
        }
        return *this;
    }
    CComPtr(_Inout_ CComPtr<IDispatch>&& lp) throw() :
        CComPtrBase<IDispatch>()
    {
        this->p = lp.p;
        lp.p = NULL;
    }
    IDispatch* operator=(_Inout_ CComPtr<IDispatch>&& lp) throw()
    {
        CComPtr(static_cast<CComPtr&&>(lp)).Swap(*this);
        return *this;
    }
// IDispatch specific stuff
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    _Check_return_ HRESULT GetPropertyByName(
        _In_z_ LPCOLESTR lpsz,
        _Out_ VARIANT* pVar) throw()
    {
        ATLASSERT(this->p);
        ATLASSERT(pVar);
        DISPID dwDispID;
        HRESULT hr = GetIDOfName(lpsz, &dwDispID);
        if (SUCCEEDED(hr))
            hr = GetProperty(dwDispID, pVar);
        return hr;
    }
    _Check_return_ HRESULT GetProperty(
        _In_ DISPID dwDispID,
        _Out_ VARIANT* pVar) throw()
    {
        return GetProperty(this->p, dwDispID, pVar);
    }
    _Check_return_ HRESULT PutPropertyByName(
        _In_z_ LPCOLESTR lpsz,
        _In_ VARIANT* pVar) throw()
    {
        ATLASSERT(this->p);
        ATLASSERT(pVar);
        DISPID dwDispID;
        HRESULT hr = GetIDOfName(lpsz, &dwDispID);
        if (SUCCEEDED(hr))
            hr = PutProperty(dwDispID, pVar);
        return hr;
    }
    _Check_return_ HRESULT PutProperty(
        _In_ DISPID dwDispID,
        _In_ VARIANT* pVar) throw()
    {
        return PutProperty(this->p, dwDispID, pVar);
    }
    _Check_return_ HRESULT GetIDOfName(
        _In_z_ LPCOLESTR lpsz,
        _Out_ DISPID* pdispid) throw()
    {
        return this->p->GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&lpsz), 1, LOCALE_USER_DEFAULT, pdispid);
    }
    // Invoke a method by DISPID with no parameters
    _Check_return_ HRESULT Invoke0(
        _In_ DISPID dispid,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        DISPPARAMS dispparams = { NULL, NULL, 0, 0};
        return this->p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
    }
    // Invoke a method by name with no parameters
    _Check_return_ HRESULT Invoke0(
        _In_z_ LPCOLESTR lpszName,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        HRESULT hr;
        DISPID dispid;
        hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = Invoke0(dispid, pvarRet);
        return hr;
    }
    // Invoke a method by DISPID with a single parameter
    _Check_return_ HRESULT Invoke1(
        _In_ DISPID dispid,
        _In_ VARIANT* pvarParam1,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        DISPPARAMS dispparams = { pvarParam1, NULL, 1, 0};
        return this->p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
    }
    // Invoke a method by name with a single parameter
    _Check_return_ HRESULT Invoke1(
        _In_z_ LPCOLESTR lpszName,
        _In_ VARIANT* pvarParam1,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        DISPID dispid;
        HRESULT hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = Invoke1(dispid, pvarParam1, pvarRet);
        return hr;
    }
    // Invoke a method by DISPID with two parameters
    _Check_return_ HRESULT Invoke2(
        _In_ DISPID dispid,
        _In_ VARIANT* pvarParam1,
        _In_ VARIANT* pvarParam2,
        _Out_opt_ VARIANT* pvarRet = NULL) throw();
    // Invoke a method by name with two parameters
    _Check_return_ HRESULT Invoke2(
        _In_z_ LPCOLESTR lpszName,
        _In_ VARIANT* pvarParam1,
        _In_ VARIANT* pvarParam2,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        DISPID dispid;
        HRESULT hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = Invoke2(dispid, pvarParam1, pvarParam2, pvarRet);
        return hr;
    }
    // Invoke a method by DISPID with N parameters
    _Check_return_ HRESULT InvokeN(
        _In_ DISPID dispid,
        _In_ VARIANT* pvarParams,
        _In_ int nParams,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        DISPPARAMS dispparams = {pvarParams, NULL, (unsigned int)nParams, 0};
        return this->p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
    }
    // Invoke a method by name with Nparameters
    _Check_return_ HRESULT InvokeN(
        _In_z_ LPCOLESTR lpszName,
        _In_ VARIANT* pvarParams,
        _In_ int nParams,
        _Out_opt_ VARIANT* pvarRet = NULL) throw()
    {
        HRESULT hr;
        DISPID dispid;
        hr = GetIDOfName(lpszName, &dispid);
        if (SUCCEEDED(hr))
            hr = InvokeN(dispid, pvarParams, nParams, pvarRet);
        return hr;
    }

    _Check_return_ static HRESULT PutProperty(
        _In_ IDispatch* pDispatch,
        _In_ DISPID dwDispID,
        _In_ VARIANT* pVar) throw()
    {
        ATLASSERT(pDispatch);
        ATLASSERT(pVar != NULL);
        if (pVar == NULL)
            return E_POINTER;

        if (pDispatch == NULL)
            return E_INVALIDARG;

        ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::PutProperty\n"));
        DISPPARAMS dispparams = {NULL, NULL, 1, 1};
        dispparams.rgvarg = pVar;
        DISPID dispidPut = DISPID_PROPERTYPUT;
        dispparams.rgdispidNamedArgs = &dispidPut;

        if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH ||
            (pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
        {
            HRESULT hr = pDispatch->Invoke(dwDispID, IID_NULL,
                LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
                &dispparams, NULL, NULL, NULL);
            if (SUCCEEDED(hr))
                return hr;
        }
        return pDispatch->Invoke(dwDispID, IID_NULL,
                LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
                &dispparams, NULL, NULL, NULL);
    }

    _Check_return_ static HRESULT GetProperty(
        _In_ IDispatch* pDispatch,
        _In_ DISPID dwDispID,
        _Out_ VARIANT* pVar) throw()
    {
        ATLASSERT(pDispatch);
        ATLASSERT(pVar != NULL);
        if (pVar == NULL)
            return E_POINTER;

        if (pDispatch == NULL)
            return E_INVALIDARG;

        ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::GetProperty\n"));
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
        return pDispatch->Invoke(dwDispID, IID_NULL,
                LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                &dispparamsNoArgs, pVar, NULL, NULL);
    }
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
};

template <class T>
inline bool CComPtrBase<T>::IsEqualObject(_Inout_opt_ IUnknown* pOther) throw()
{
    if (p == NULL && pOther == NULL)
        return true;	// They are both NULL objects

    if (p == NULL || pOther == NULL)
        return false;	// One is NULL the other is not

    CComPtr<IUnknown> punk1;
    CComPtr<IUnknown> punk2;
    p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
    pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
    return punk1 == punk2;
}

template <class T, const IID* piid = &__uuidof(T)>
class CComQIPtr :
    public CComPtr<T>
{
public:
    CComQIPtr() throw()
    {
    }
    CComQIPtr(decltype(__nullptr)) throw()
    {
    }
    CComQIPtr(_Inout_opt_ T* lp) throw() :
        CComPtr<T>(lp)
    {
    }
    CComQIPtr(_Inout_ const CComQIPtr<T,piid>& lp) throw() :
        CComPtr<T>(lp.p)
    {
    }
    CComQIPtr(_Inout_opt_ IUnknown* lp) throw()
    {
        if (lp != NULL)
        {
            if (FAILED(lp->QueryInterface(*piid, (void **)&this->p)))
                this->p = NULL;
        }
    }
    T* operator=(decltype(__nullptr)) throw()
    {
        CComQIPtr(nullptr).Swap(*this);
        return nullptr;
    }
    T* operator=(_Inout_opt_ T* lp) throw()
    {
        if(*this!=lp)
        {
            CComQIPtr(lp).Swap(*this);
        }
        return *this;
    }
    T* operator=(_Inout_ const CComQIPtr<T,piid>& lp) throw()
    {
        if(*this!=lp)
        {
            CComQIPtr(lp).Swap(*this);
        }
        return *this;
    }
    T* operator=(_Inout_opt_ IUnknown* lp) throw()
    {
        if(*this!=lp)
        {
            return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&this->p, lp, *piid));
        }
        return *this;
    }
};

//Specialization to make it work
template<>
class CComQIPtr<IUnknown, &IID_IUnknown> :
    public CComPtr<IUnknown>
{
public:
    CComQIPtr() throw()
    {
    }
    CComQIPtr(_Inout_opt_ IUnknown* lp) throw()
    {
        //Actually do a QI to get identity
        if (lp != NULL)
        {
            if (FAILED(lp->QueryInterface(__uuidof(IUnknown), (void **)&this->p)))
                this->p = NULL;
        }
    }
    CComQIPtr(_Inout_ const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw() :
        CComPtr<IUnknown>(lp.p)
    {
    }
    IUnknown* operator=(_Inout_opt_ IUnknown* lp) throw()
    {
        if(*this!=lp)
        {
            //Actually do a QI to get identity
            return AtlComQIPtrAssign((IUnknown**)&this->p, lp, __uuidof(IUnknown));
        }
        return *this;
    }

    IUnknown* operator=(_Inout_ const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw()
    {
        if(*this!=lp)
        {
            CComQIPtr(lp).Swap(*this);
        }
        return *this;
    }
};

typedef CComQIPtr<IDispatch, &__uuidof(IDispatch)> CComDispatchDriver;

#define com_cast ATL::CComQIPtr
#ifndef _ATL_STREAM_MAX_SIZE
#define _ATL_STREAM_MAX_SIZE  0x100000
#endif

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

class CComBSTR
{
public:
    BSTR m_str;

    CComBSTR() throw()
    {
        m_str = NULL;
    }
    CComBSTR(decltype(__nullptr)) throw()
    {
        m_str = NULL;
    }
#ifdef _ATL_CCOMBSTR_EXPLICIT_CONSTRUCTORS
    explicit CComBSTR(_In_ int nSize)
#else
    CComBSTR(_In_ int nSize)
#endif
    {
        if (nSize < 0)
        {
            AtlThrow(E_INVALIDARG);
        }

        if (nSize == 0)
        {
            m_str = NULL;
        }
        else
        {
            m_str = ::SysAllocStringLen(NULL, nSize);
            if (!*this)
            {
                AtlThrow(E_OUTOFMEMORY);
            }
        }
    }

    CComBSTR(_In_ int nSize, _In_reads_opt_(nSize) LPCOLESTR sz)
    {
        if (nSize < 0)
        {
            AtlThrow(E_INVALIDARG);
        }

        if (nSize == 0)
        {
            m_str = NULL;
        }
        else
        {
            m_str = ::SysAllocStringLen(sz, nSize);
            if (!*this)
            {
                AtlThrow(E_OUTOFMEMORY);
            }
        }
    }

    CComBSTR(_In_opt_z_ LPCOLESTR pSrc)
    {
        if (pSrc == NULL)
        {
            m_str = NULL;
        }
        else
        {
            m_str = ::SysAllocString(pSrc);
            if (!*this)
            {
                AtlThrow(E_OUTOFMEMORY);
            }
        }
    }

    CComBSTR(_In_ const CComBSTR& src)
    {
        m_str = src.Copy();
        if (!!src && !*this)
        {
            AtlThrow(E_OUTOFMEMORY);
        }
    }

    CComBSTR(_In_ REFGUID guid)
    {
        OLECHAR szGUID[64];
        int result = ::StringFromGUID2(guid, szGUID, 64);
        ATLASSERT(result != 0);
        UNREFERENCED_PARAMETER(result);
        m_str = ::SysAllocString(szGUID);
        if (!*this)
        {
            AtlThrow(E_OUTOFMEMORY);
        }
    }

    CComBSTR& operator=(decltype(__nullptr)) throw()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
        return *this;
    }

    CComBSTR& operator=(_In_ const CComBSTR& src)
    {
        if (m_str != src.m_str)
        {
            ::SysFreeString(m_str);
            m_str = src.Copy();
            if (!!src && !*this)
            {
                AtlThrow(E_OUTOFMEMORY);
            }
        }
        return *this;
    }

    CComBSTR& operator=(_In_opt_z_ LPCOLESTR pSrc)
    {
        if (pSrc != m_str)
        {
            ::SysFreeString(m_str);
            if (pSrc != NULL)
            {
                m_str = ::SysAllocString(pSrc);
                if (!*this)
                {
                    AtlThrow(E_OUTOFMEMORY);
                }
            }
            else
            {
                m_str = NULL;
            }
        }
        return *this;
    }
    CComBSTR(_Inout_ CComBSTR&& src) throw()
    {
        m_str = src.m_str;
        src.m_str = NULL;
    }

    CComBSTR& operator=(_Inout_ CComBSTR&& src) throw()
    {
        if (m_str != src.m_str)
        {
            ::SysFreeString(m_str);
            m_str = src.m_str;
            src.m_str = NULL;
        }
        return *this;
    }

    ~CComBSTR() throw();

    unsigned int Length() const throw()
    {
        return ::SysStringLen(m_str);
    }

    unsigned int ByteLength() const throw()
    {
        return ::SysStringByteLen(m_str);
    }

    operator BSTR() const throw()
    {
        return m_str;
    }


#ifndef ATL_CCOMBSTR_ADDRESS_OF_ASSERT
// Temp disable CComBSTR::operator& Assert
#define ATL_NO_CCOMBSTR_ADDRESS_OF_ASSERT
#endif


    BSTR* operator&() throw()
    {
#ifndef ATL_NO_CCOMBSTR_ADDRESS_OF_ASSERT
        ATLASSERT(!*this);
#endif
        return &m_str;
    }

    _Ret_maybenull_z_ BSTR Copy() const throw()
    {
        if (!*this)
        {
            return NULL;
        }
        else if (m_str != NULL)
        {
            return ::SysAllocStringByteLen((char*)m_str, ::SysStringByteLen(m_str));
        }
        else
        {
            return ::SysAllocStringByteLen(NULL, 0);
        }
    }

    _Check_return_ HRESULT CopyTo(_Outptr_result_maybenull_ _Result_nullonfailure_ BSTR* pbstr) const throw()
    {
        ATLASSERT(pbstr != NULL);
        if (pbstr == NULL)
        {
            return E_POINTER;
        }
        *pbstr = Copy();

        if ((*pbstr == NULL) && (m_str != NULL))
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    // copy BSTR to VARIANT
    _Check_return_ HRESULT CopyTo(_Out_ VARIANT *pvarDest) const throw()
    {
        ATLASSERT(pvarDest != NULL);
        HRESULT hRes = E_POINTER;
        if (pvarDest != NULL)
        {
            pvarDest->vt = VT_BSTR;
            pvarDest->bstrVal = Copy();

            if (pvarDest->bstrVal == NULL && m_str != NULL)
            {
                hRes = E_OUTOFMEMORY;
            }
            else
            {
                hRes = S_OK;
            }
        }
        return hRes;
    }

    void Attach(_In_opt_z_ BSTR src) throw()
    {
        if (m_str != src)
        {
            ::SysFreeString(m_str);
            m_str = src;
        }
    }

    _Ret_maybenull_z_ BSTR Detach() throw()
    {
        BSTR s = m_str;
        m_str = NULL;
        return s;
    }

    void Empty() throw()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
    }

    bool operator!() const throw()
    {
        return (m_str == NULL);
    }

    _Check_return_ HRESULT Append(_In_ const CComBSTR& bstrSrc) throw()
    {
        return AppendBSTR(bstrSrc.m_str);
    }

    _Check_return_ HRESULT Append(_In_z_ LPCOLESTR lpsz) throw()
    {
        return Append(lpsz, UINT(ocslen(lpsz)));
    }

    // a BSTR is just a LPCOLESTR so we need a special version to signify
    // that we are appending a BSTR
    _Check_return_ HRESULT AppendBSTR(_In_opt_z_ BSTR p) throw()
    {
        if (::SysStringLen(p) == 0)
        {
            return S_OK;
        }
        BSTR bstrNew = NULL;
        HRESULT hr;
        _Analysis_assume_(p);
        hr = VarBstrCat(m_str, p, &bstrNew);
        if (SUCCEEDED(hr))
        {
            ::SysFreeString(m_str);
            m_str = bstrNew;
        }
        return hr;
    }

    _Check_return_ HRESULT Append(_In_reads_opt_(nLen) LPCOLESTR lpsz, _In_ int nLen) throw()
    {
        if (lpsz == NULL || (m_str != NULL && nLen == 0))
        {
            return S_OK;
        }
        else if (nLen < 0)
        {
            return E_INVALIDARG;
        }

        const unsigned int n1 = Length();
        unsigned int n1Bytes = 0;
        unsigned int nSize = 0;
        unsigned int nSizeBytes = 0;

        HRESULT hr = AtlAdd<unsigned int>(&nSize, n1, nLen);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = AtlMultiply<unsigned int>(&nSizeBytes, nSize, sizeof(OLECHAR));
        if (FAILED(hr))
        {
            return hr;
        }

        hr = AtlMultiply<unsigned int>(&n1Bytes, n1, sizeof(OLECHAR));
        if (FAILED(hr))
        {
            return hr;
        }

        BSTR b = ::SysAllocStringLen(NULL, nSize);
        if (b == NULL)
        {
            return E_OUTOFMEMORY;
        }

        if(::SysStringLen(m_str) > 0)
        {
            _Analysis_assume_(m_str); // ::SysStringLen(m_str) guarantees that m_str != NULL
            Checked::memcpy_s(b, nSizeBytes, m_str, n1Bytes);
        }

        Checked::memcpy_s(b+n1, nLen*sizeof(OLECHAR), lpsz, nLen*sizeof(OLECHAR));
        b[nSize] = '\0';
        SysFreeString(m_str);
        m_str = b;
        return S_OK;
    }

    _Check_return_ HRESULT Append(_In_ char ch) throw()
    {
        OLECHAR chO = ch;

        return( Append( &chO, 1 ) );
    }

    _Check_return_ HRESULT Append(_In_ wchar_t ch) throw()
    {
        return( Append( &ch, 1 ) );
    }

    _Check_return_ HRESULT AppendBytes(
        _In_reads_opt_(nLen) const char* lpsz,
        _In_ int nLen) throw()
    {
        if (lpsz == NULL || nLen == 0)
        {
            return S_OK;
        }
        else if (nLen < 0)
        {
            return E_INVALIDARG;
        }

        const unsigned int n1 = ByteLength();
        unsigned int nSize = 0;
        HRESULT hr = AtlAdd<unsigned int>(&nSize, n1, nLen);
        if (FAILED(hr))
        {
            return hr;
        }

        BSTR b = ::SysAllocStringByteLen(NULL, nSize);
        if (b == NULL)
        {
            return E_OUTOFMEMORY;
        }

        Checked::memcpy_s(b, nSize, m_str, n1);
        Checked::memcpy_s(((char*)b) + n1, nLen, lpsz, nLen);

        *((OLECHAR*)(((char*)b) + nSize)) = '\0';
        SysFreeString(m_str);
        m_str = b;
        return S_OK;
    }

    _Check_return_ HRESULT AssignBSTR(_In_opt_z_ const BSTR bstrSrc) throw()
    {
        HRESULT hr = S_OK;
        if (m_str != bstrSrc)
        {
            ::SysFreeString(m_str);
            if (bstrSrc != NULL)
            {
                m_str = ::SysAllocStringByteLen((char*)bstrSrc, ::SysStringByteLen(bstrSrc));
                if (!*this)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                m_str = NULL;
            }
        }

        return hr;
    }

    _Check_return_ HRESULT ToLower() throw()
    {
        if (::SysStringLen(m_str) > 0)
        {
#ifdef _UNICODE

#if !defined(_ATL_USE_WINAPI_FAMILY_DESKTOP_APP)
            int length = Length();
            if (_AtlLCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_LOWERCASE, m_str, length, m_str, length, NULL, NULL, 0) == 0)
            {
                return AtlHresultFromLastError();
            }
#else
            // Convert in place
            CharLowerBuff(m_str, Length());
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

#else
            // Cannot use conversion macros due to possible embedded NULLs
            UINT _acp = _AtlGetConversionACP();
            int _convert = WideCharToMultiByte(_acp, 0, m_str, Length(), NULL, 0, NULL, NULL);
            CTempBuffer<char> pszA;
            ATLTRY(pszA.Allocate(_convert));
            if (pszA == NULL)
                return E_OUTOFMEMORY;

            int nRet = WideCharToMultiByte(_acp, 0, m_str, Length(), pszA, _convert, NULL, NULL);
            if (nRet == 0)
            {
                ATLASSERT(0);
                return AtlHresultFromLastError();
            }

            CharLowerBuff(pszA, nRet);

            _convert = MultiByteToWideChar(_acp, 0, pszA, nRet, NULL, 0);

            CTempBuffer<WCHAR> pszW;
            ATLTRY(pszW.Allocate(_convert));
            if (pszW == NULL)
                return E_OUTOFMEMORY;

            nRet = MultiByteToWideChar(_acp, 0, pszA, nRet, pszW, _convert);
            if (nRet <= 0)
            {
                ATLASSERT(0);
                return AtlHresultFromLastError();
            }

            UINT nBytes=0;
            HRESULT hr=S_OK;
            if( FAILED(hr=::ATL::AtlMultiply(&nBytes, static_cast<UINT>(nRet), static_cast<UINT>(sizeof(OLECHAR)))))
            {
                return hr;
            }
            BSTR b = ::SysAllocStringByteLen((LPCSTR) (LPWSTR) pszW, nBytes);
            if (b == NULL)
                return E_OUTOFMEMORY;
            SysFreeString(m_str);
            m_str = b;
#endif
        }
        return S_OK;
    }
    _Check_return_ HRESULT ToUpper() throw()
    {
        if (::SysStringLen(m_str) > 0)
        {
#ifdef _UNICODE

#if !defined(_ATL_USE_WINAPI_FAMILY_DESKTOP_APP)
            int length = Length();
            if (_AtlLCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_UPPERCASE, m_str, length, m_str, length, NULL, NULL, 0) == 0)
            {
                return AtlHresultFromLastError();
            }
#else
            // Convert in place
            CharUpperBuff(m_str, Length());
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

#else
            // Cannot use conversion macros due to possible embedded NULLs
            UINT _acp = _AtlGetConversionACP();
            int _convert = WideCharToMultiByte(_acp, 0, m_str, Length(), NULL, 0, NULL, NULL);
            CTempBuffer<char> pszA;
            ATLTRY(pszA.Allocate(_convert));
            if (pszA == NULL)
                return E_OUTOFMEMORY;

            int nRet = WideCharToMultiByte(_acp, 0, m_str, Length(), pszA, _convert, NULL, NULL);
            if (nRet == 0)
            {
                ATLASSERT(0);
                return AtlHresultFromLastError();
            }

            CharUpperBuff(pszA, nRet);

            _convert = MultiByteToWideChar(_acp, 0, pszA, nRet, NULL, 0);

            CTempBuffer<WCHAR> pszW;
            ATLTRY(pszW.Allocate(_convert));
            if (pszW == NULL)
                return E_OUTOFMEMORY;

            nRet = MultiByteToWideChar(_acp, 0, pszA, nRet, pszW, _convert);
            if (nRet <= 0)
            {
                ATLASSERT(0);
                return AtlHresultFromLastError();
            }

            UINT nBytes=0;
            HRESULT hr=S_OK;
            if( FAILED(hr=::ATL::AtlMultiply(&nBytes, static_cast<UINT>(nRet), static_cast<UINT>(sizeof(OLECHAR)))))
            {
                return hr;
            }
            BSTR b = ::SysAllocStringByteLen((LPCSTR) (LPWSTR) pszW, nBytes);
            if (b == NULL)
                return E_OUTOFMEMORY;
            SysFreeString(m_str);
            m_str = b;
#endif
        }
        return S_OK;
    }

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    bool LoadString(
        _In_ HINSTANCE hInst,
        _In_ UINT nID) throw()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
        return LoadStringResource(hInst, nID, m_str);
    }

    bool LoadString(_In_ UINT nID) throw()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
        return LoadStringResource(nID, m_str);
    }
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

    CComBSTR& operator+=(_In_ const CComBSTR& bstrSrc)
    {
        HRESULT hr;
        hr = AppendBSTR(bstrSrc.m_str);
        if (FAILED(hr))
            AtlThrow(hr);
        return *this;
    }

    CComBSTR& operator+=(_In_z_ LPCOLESTR pszSrc)
    {
        HRESULT hr;
        hr = Append(pszSrc);
        if (FAILED(hr))
            AtlThrow(hr);
        return *this;
    }

    bool operator<(_In_ const CComBSTR& bstrSrc) const throw()
    {
        return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_LT);
    }
    bool operator<(_In_z_ LPCOLESTR pszSrc) const
    {
        CComBSTR bstr2(pszSrc);
        return operator<(bstr2);
    }
    bool operator<(_In_z_ LPOLESTR pszSrc) const
    {
        return operator<((LPCOLESTR)pszSrc);
    }

    bool operator>(_In_ const CComBSTR& bstrSrc) const throw()
    {
        return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_GT);
    }
    bool operator>(_In_z_ LPCOLESTR pszSrc) const
    {
        CComBSTR bstr2(pszSrc);
        return operator>(bstr2);
    }
    bool operator>(_In_z_ LPOLESTR pszSrc) const
    {
        return operator>((LPCOLESTR)pszSrc);
    }

    bool operator!=(_In_ const CComBSTR& bstrSrc) const throw()
    {
        return !operator==(bstrSrc);
    }
    bool operator!=(_In_z_ LPCOLESTR pszSrc) const
    {
        return !operator==(pszSrc);
    }
    bool operator!=(_In_ int nNull) const throw()
    {
        return !operator==(nNull);
    }
    bool operator!=(_In_z_ LPOLESTR pszSrc) const
    {
        return operator!=((LPCOLESTR)pszSrc);
    }
    bool operator==(_In_ const CComBSTR& bstrSrc) const throw()
    {
        return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_EQ);
    }
    bool operator==(LPCOLESTR pszSrc) const
    {
        CComBSTR bstr2(pszSrc);
        return operator==(bstr2);
    }
    bool operator==(_In_z_ LPOLESTR pszSrc) const
    {
        return operator==((LPCOLESTR)pszSrc);
    }

    bool operator==(_In_ int nNull) const throw()
    {
        ATLASSERT(nNull == 0);
        (void)nNull;
        return (!*this);
    }

    bool operator==(decltype(__nullptr)) const throw()
    {
        return *this == 0;
    }

    bool operator!=(decltype(__nullptr)) const throw()
    {
        return *this != 0;
    }

    CComBSTR(_In_opt_z_ LPCSTR pSrc)
    {
        if (pSrc != NULL)
        {
            m_str = A2WBSTR(pSrc);
            if (!*this)
            {
                AtlThrow(E_OUTOFMEMORY);
            }
        }
        else
        {
            m_str = NULL;
        }
    }

    CComBSTR(_In_ int nSize, _In_reads_opt_(nSize) LPCSTR sz)
    {
        if (nSize < 0)
        {
            AtlThrow(E_INVALIDARG);
        }

        if (nSize != 0 && sz == NULL)
        {
            m_str = ::SysAllocStringLen(NULL, nSize);
            if (!*this)
            {
                AtlThrow(E_OUTOFMEMORY);
            }
            return;
        }

        m_str = A2WBSTR(sz, nSize);
        if (!*this && nSize != 0)
        {
            AtlThrow(E_OUTOFMEMORY);
        }
    }

    _Check_return_ HRESULT Append(_In_opt_z_ LPCSTR lpsz) throw()
    {
        if (lpsz == NULL)
            return S_OK;

        CComBSTR bstrTemp;
        ATLTRY(bstrTemp = lpsz);
        if (!bstrTemp)
        {
            return E_OUTOFMEMORY;
        }
        return Append(bstrTemp);
    }

    CComBSTR& operator=(_In_opt_z_ LPCSTR pSrc)
    {
        ::SysFreeString(m_str);
        m_str = A2WBSTR(pSrc);
        if (!*this && pSrc != NULL)
        {
            AtlThrow(E_OUTOFMEMORY);
        }
        return *this;
    }

    bool operator<(_In_opt_z_ LPCSTR pszSrc) const
    {
        CComBSTR bstr2(pszSrc);
        return operator<(bstr2);
    }
    bool operator>(_In_opt_z_ LPCSTR pszSrc) const
    {
        CComBSTR bstr2(pszSrc);
        return operator>(bstr2);
    }
    bool operator!=(_In_opt_z_ LPCSTR pszSrc) const
    {
        return !operator==(pszSrc);
    }
    bool operator==(_In_opt_z_ LPCSTR pszSrc) const
    {
        CComBSTR bstr2(pszSrc);
        return operator==(bstr2);
    }

    _Check_return_ HRESULT WriteToStream(_Inout_ IStream* pStream) throw()
    {
        ATLASSERT(pStream != NULL);
        if(pStream == NULL)
        {
            return E_INVALIDARG;
        }

        ULONG cb;
        ULONG cbStrLen = CComBSTR::GetStreamSize(m_str);
        ATLASSERT(cbStrLen >= sizeof(ULONG));
        cbStrLen -= sizeof(ULONG);

        HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
        if (FAILED(hr))
        {
            return hr;
        }

        if (cbStrLen == 0)
        {
            return S_OK;
        }
        return pStream->Write((void*) m_str, cbStrLen, &cb);
    }

    _Check_return_ HRESULT ReadFromStream(_Inout_ IStream* pStream) throw()
    {
        ATLASSERT(pStream != NULL);
        if(pStream == NULL)
        {
            return E_INVALIDARG;
        }

        ATLASSERT(!*this); // should be empty
        Empty();

        HRESULT hrSeek;
        ULARGE_INTEGER nBegOffset;
        {
            LARGE_INTEGER nZeroOffset;
            nZeroOffset.QuadPart = 0L;
            hrSeek = pStream->Seek(nZeroOffset, STREAM_SEEK_CUR, &nBegOffset);
        }

        ULONG cbRead = 0;
        ULONG cbStrLen = 0;
        HRESULT hr = pStream->Read(reinterpret_cast<void*>(&cbStrLen), sizeof(cbStrLen), &cbRead);

        if (SUCCEEDED(hr))
        {
            // invalid data size
            if (sizeof(cbStrLen) != cbRead)
            {
                ATLTRACE(atlTraceCOM, 0, _T("Input stream is corrupted."));
                hr = E_FAIL;
            }
            // read NULL string
            else if (cbStrLen == 0)
            {
            }
            // invalid data length
            else if (cbStrLen < sizeof(OLECHAR))
            {
                ATLTRACE(atlTraceCOM, 0, _T("Input stream is corrupted."));
                hr = E_FAIL;
            }
            // security checks for huge stream of data
            else if (cbStrLen > _ATL_STREAM_MAX_SIZE)
            {
                ATLTRACE(atlTraceCOM, 0, _T("String exceeded the maximum allowed size see _ATL_STREAM_MAX_SIZE."));
                hr = E_ACCESSDENIED;
            }
            else
            {
                //subtract size for terminating NULL which we wrote out
                cbStrLen -= sizeof(OLECHAR);

                m_str = ::SysAllocStringByteLen(NULL, cbStrLen);
                if (!*this)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = pStream->Read(reinterpret_cast<void*>(m_str), cbStrLen, &cbRead);

                    if (SUCCEEDED(hr))
                    {
                        if (cbRead != cbStrLen)
                        {
                            ATLTRACE(atlTraceCOM, 0, _T("Length of string data is different than expected."));
                            hr = E_FAIL;
                        }
                        else
                        {
                            OLECHAR ch;
                            hr = pStream->Read(reinterpret_cast<void*>(&ch), sizeof(OLECHAR), &cbRead);

                            if (SUCCEEDED(hr))
                            {
#ifndef _ATL_CCOMBSTR_READFROMSTREAM_INSECURE
                                if (cbRead != sizeof(OLECHAR) || ch != L'\0')
#else
                                if (cbRead != sizeof(OLECHAR))
#endif
                                {
                                    ATLTRACE(atlTraceCOM, 0, _T("Cannot read NULL terminator from stream."));
                                    hr = E_FAIL;
                                }
                            }
                        }
                    }

                    if (FAILED(hr))
                    {
ATLPREFAST_SUPPRESS(6102)
                        ::SysFreeString(m_str);
ATLPREFAST_UNSUPPRESS()
                        m_str = NULL;
                    }
                }
            }
        }

        // If SysAllocStringByteLen or IStream::Read failed, reset seek
        // pointer to start of BSTR size.
        if (FAILED(hr) && SUCCEEDED(hrSeek))
        {
            LARGE_INTEGER nOffset;
            nOffset.QuadPart = static_cast<LONGLONG>(nBegOffset.QuadPart);
            pStream->Seek(nOffset, STREAM_SEEK_SET, NULL);
        }

        return hr;
    }

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    _Success_(return != false)
    static bool LoadStringResource(
        _In_ HINSTANCE hInstance,
        _In_ UINT uID,
        _Inout_ _Pre_null_ _Post_z_ BSTR& bstrText) throw()
    {
        ATLASSERT(bstrText == NULL);

        const ATLSTRINGRESOURCEIMAGE* pImage = AtlGetStringResourceImage(hInstance, uID);
        if (pImage != NULL)
        {
            bstrText = ::SysAllocStringLen(pImage->achString, pImage->nLength);
        }
        else
        {
            bstrText = NULL;
        }
        return (bstrText != NULL) ? true : false;
    }

    _Success_(return != false)
    static bool LoadStringResource(
        _In_ UINT uID,
        _Inout_ _Pre_null_ _Post_z_ BSTR& bstrText) throw()
    {
        ATLASSERT(bstrText == NULL);

        const ATLSTRINGRESOURCEIMAGE* pImage = AtlGetStringResourceImage(uID);
        if (pImage != NULL)
        {
            bstrText = ::SysAllocStringLen(pImage->achString, pImage->nLength);
        }
        else
        {
            bstrText = NULL;
        }

        return (bstrText != NULL) ? true : false;
    }

    // each character in BSTR is copied to each element in SAFEARRAY
    HRESULT BSTRToArray(_Outptr_ LPSAFEARRAY *ppArray) throw()
    {
        return VectorFromBstr(m_str, ppArray);
    }

    // first character of each element in SAFEARRAY is copied to BSTR
    _Check_return_ HRESULT ArrayToBSTR(_In_ const SAFEARRAY *pSrc) throw()
    {
        ::SysFreeString(m_str);
        return BstrFromVector((LPSAFEARRAY)pSrc, &m_str);
    }
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

    static ULONG GetStreamSize(_In_opt_z_ BSTR bstr)
    {
        ULONG ulSize = sizeof(ULONG);
        if (bstr != NULL)
        {
            ulSize += SysStringByteLen(bstr) + sizeof(OLECHAR);
        }

        return ulSize;
    }
};

inline CComBSTR::~CComBSTR() throw()
{
    ::SysFreeString(m_str);
}

inline void SysFreeStringHelper(_In_ CComBSTR& bstr)
{
    bstr.Empty();
}

inline void SysFreeStringHelper(_In_opt_z_ BSTR bstr)
{
    ::SysFreeString(bstr);
}

_Check_return_ inline HRESULT SysAllocStringHelper(
    _Out_ CComBSTR& bstrDest,
    _In_opt_z_ BSTR bstrSrc)
{
    bstrDest=bstrSrc;
    return !bstrDest ? E_OUTOFMEMORY : S_OK;
}

_Check_return_ inline HRESULT SysAllocStringHelper(
    _Out_ BSTR& bstrDest,
    _In_opt_z_ BSTR bstrSrc)
{
    bstrDest=::SysAllocString(bstrSrc);

    return bstrDest==NULL ? E_OUTOFMEMORY : S_OK;
}

/////////////////////////////////////////////////////////////
// Class to Adapt CComBSTR and CComPtr for use with STL containers
// the syntax to use it is
// std::vector< CAdapt <CComBSTR> > vect;

template <class T>
class CAdapt
{
public:
    CAdapt()
    {
    }

    CAdapt(_In_ const T& rSrc) :
        m_T( rSrc )
    {
    }

    CAdapt(_In_ const CAdapt<T>& rSrCA) :
        m_T( rSrCA.m_T )
    {
    }

    CAdapt<T>& operator=(_In_ const T& rSrc)
    {
        m_T = rSrc;

        return *this;
    }

    CAdapt<T>& operator=(_In_ const CAdapt<T>& rSrc)
    {
        if (this != &rSrc)
        {
            m_T = rSrc.m_T;
        }
        return *this;
    }

    CAdapt(_Inout_ T&& rSrc) :
        m_T( static_cast<T&&>(rSrc) )
    {
    }

    CAdapt(_Inout_ CAdapt<T>&& rSrCA) _ATL_NOEXCEPT(details::is_nothrow_move_constructible<T>::value) :
        m_T( static_cast<T&&>(rSrCA.m_T) )
    {
    }

    CAdapt<T>& operator=(_Inout_ T&& rSrc)
    {
        m_T = static_cast<T&&>(rSrc);

        return *this;
    }

    CAdapt<T>& operator=(_Inout_ CAdapt<T>&& rSrc) _ATL_NOEXCEPT(details::is_nothrow_move_assignable<T>::value)
    {
        if (this != &rSrc)
        {
            m_T = static_cast<T&&>( rSrc.m_T );
        }
        return *this;
    }

    bool operator<(_In_ const T& rSrc) const
    {
        return m_T < rSrc;
    }

    bool operator==(_In_ const T& rSrc) const
    {
        return m_T == rSrc;
    }

    operator T&()
    {
        return m_T;
    }

    operator const T&() const
    {
        return m_T;
    }

    T& operator->()
    {
        return m_T;
    }

    const T& operator->() const
    {
        return m_T;
    }

    T m_T;
};

/////////////////////////////////////////////////////////////////////////////
// CComVariant


#define ATL_VARIANT_TRUE VARIANT_BOOL( -1 )
#define ATL_VARIANT_FALSE VARIANT_BOOL( 0 )

template< typename T >
class CVarTypeInfo
{
//	static const VARTYPE VT;  // VARTYPE corresponding to type T
//	static T VARIANT::* const pmField;  // Pointer-to-member of corresponding field in VARIANT struct
};

template<>
class CVarTypeInfo< char >
{
public:
    static const VARTYPE VT = VT_I1;
    static char VARIANT::* const pmField;
};

__declspec( selectany ) char VARIANT::* const CVarTypeInfo< char >::pmField = &VARIANT::cVal;

template<>
class CVarTypeInfo< unsigned char >
{
public:
    static const VARTYPE VT = VT_UI1;
    static unsigned char VARIANT::* const pmField;
};

__declspec( selectany ) unsigned char VARIANT::* const CVarTypeInfo< unsigned char >::pmField = &VARIANT::bVal;

template<>
class CVarTypeInfo< char* >
{
public:
    static const VARTYPE VT = VT_I1|VT_BYREF;
    static char* VARIANT::* const pmField;
};

__declspec( selectany ) char* VARIANT::* const CVarTypeInfo< char* >::pmField = &VARIANT::pcVal;

template<>
class CVarTypeInfo< unsigned char* >
{
public:
    static const VARTYPE VT = VT_UI1|VT_BYREF;
    static unsigned char* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned char* VARIANT::* const CVarTypeInfo< unsigned char* >::pmField = &VARIANT::pbVal;

template<>
class CVarTypeInfo< short >
{
public:
    static const VARTYPE VT = VT_I2;
    static short VARIANT::* const pmField;
};

__declspec( selectany ) short VARIANT::* const CVarTypeInfo< short >::pmField = &VARIANT::iVal;

template<>
class CVarTypeInfo< short* >
{
public:
    static const VARTYPE VT = VT_I2|VT_BYREF;
    static short* VARIANT::* const pmField;
};

__declspec( selectany ) short* VARIANT::* const CVarTypeInfo< short* >::pmField = &VARIANT::piVal;

template<>
class CVarTypeInfo< unsigned short >
{
public:
    static const VARTYPE VT = VT_UI2;
    static unsigned short VARIANT::* const pmField;
};

__declspec( selectany ) unsigned short VARIANT::* const CVarTypeInfo< unsigned short >::pmField = &VARIANT::uiVal;

#ifdef _NATIVE_WCHAR_T_DEFINED  // Only treat unsigned short* as VT_UI2|VT_BYREF if BSTR isn't the same as unsigned short*
template<>
class CVarTypeInfo< unsigned short* >
{
public:
    static const VARTYPE VT = VT_UI2|VT_BYREF;
    static unsigned short* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned short* VARIANT::* const CVarTypeInfo< unsigned short* >::pmField = &VARIANT::puiVal;
#endif  // _NATIVE_WCHAR_T_DEFINED

template<>
class CVarTypeInfo< int >
{
public:
    static const VARTYPE VT = VT_I4;
    static int VARIANT::* const pmField;
};

__declspec( selectany ) int VARIANT::* const CVarTypeInfo< int >::pmField = &VARIANT::intVal;

template<>
class CVarTypeInfo< int* >
{
public:
    static const VARTYPE VT = VT_I4|VT_BYREF;
    static int* VARIANT::* const pmField;
};

__declspec( selectany ) int* VARIANT::* const CVarTypeInfo< int* >::pmField = &VARIANT::pintVal;

template<>
class CVarTypeInfo< unsigned int >
{
public:
    static const VARTYPE VT = VT_UI4;
    static unsigned int VARIANT::* const pmField;
};

__declspec( selectany ) unsigned int VARIANT::* const CVarTypeInfo< unsigned int >::pmField = &VARIANT::uintVal;

template<>
class CVarTypeInfo< unsigned int* >
{
public:
    static const VARTYPE VT = VT_UI4|VT_BYREF;
    static unsigned int* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned int* VARIANT::* const CVarTypeInfo< unsigned int* >::pmField = &VARIANT::puintVal;

template<>
class CVarTypeInfo< long >
{
public:
    static const VARTYPE VT = VT_I4;
    static long VARIANT::* const pmField;
};

__declspec( selectany ) long VARIANT::* const CVarTypeInfo< long >::pmField = &VARIANT::lVal;

template<>
class CVarTypeInfo< long* >
{
public:
    static const VARTYPE VT = VT_I4|VT_BYREF;
    static long* VARIANT::* const pmField;
};

__declspec( selectany ) long* VARIANT::* const CVarTypeInfo< long* >::pmField = &VARIANT::plVal;

template<>
class CVarTypeInfo< unsigned long >
{
public:
    static const VARTYPE VT = VT_UI4;
    static unsigned long VARIANT::* const pmField;
};

__declspec( selectany ) unsigned long VARIANT::* const CVarTypeInfo< unsigned long >::pmField = &VARIANT::ulVal;

template<>
class CVarTypeInfo< unsigned long* >
{
public:
    static const VARTYPE VT = VT_UI4|VT_BYREF;
    static unsigned long* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned long* VARIANT::* const CVarTypeInfo< unsigned long* >::pmField = &VARIANT::pulVal;

template<>
class CVarTypeInfo< __int64 >
{
public:
    static const VARTYPE VT = VT_I8;
    static __int64 VARIANT::* const pmField;
};

__declspec( selectany ) __int64 VARIANT::* const CVarTypeInfo< __int64 >::pmField = &VARIANT::llVal;

template<>
class CVarTypeInfo< __int64* >
{
public:
    static const VARTYPE VT = VT_I8|VT_BYREF;
    static __int64* VARIANT::* const pmField;
};

__declspec( selectany ) __int64* VARIANT::* const CVarTypeInfo< __int64* >::pmField = &VARIANT::pllVal;

template<>
class CVarTypeInfo< unsigned __int64 >
{
public:
    static const VARTYPE VT = VT_UI8;
    static unsigned __int64 VARIANT::* const pmField;
};

__declspec( selectany ) unsigned __int64 VARIANT::* const CVarTypeInfo< unsigned __int64 >::pmField = &VARIANT::ullVal;

template<>
class CVarTypeInfo< unsigned __int64* >
{
public:
    static const VARTYPE VT = VT_UI8|VT_BYREF;
    static unsigned __int64* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned __int64* VARIANT::* const CVarTypeInfo< unsigned __int64* >::pmField = &VARIANT::pullVal;

template<>
class CVarTypeInfo< float >
{
public:
    static const VARTYPE VT = VT_R4;
    static float VARIANT::* const pmField;
};

__declspec( selectany ) float VARIANT::* const CVarTypeInfo< float >::pmField = &VARIANT::fltVal;

template<>
class CVarTypeInfo< float* >
{
public:
    static const VARTYPE VT = VT_R4|VT_BYREF;
    static float* VARIANT::* const pmField;
};

__declspec( selectany ) float* VARIANT::* const CVarTypeInfo< float* >::pmField = &VARIANT::pfltVal;

template<>
class CVarTypeInfo< double >
{
public:
    static const VARTYPE VT = VT_R8;
    static double VARIANT::* const pmField;
};

__declspec( selectany ) double VARIANT::* const CVarTypeInfo< double >::pmField = &VARIANT::dblVal;

template<>
class CVarTypeInfo< double* >
{
public:
    static const VARTYPE VT = VT_R8|VT_BYREF;
    static double* VARIANT::* const pmField;
};

__declspec( selectany ) double* VARIANT::* const CVarTypeInfo< double* >::pmField = &VARIANT::pdblVal;

template<>

class CVarTypeInfo< VARIANT* >
{
public:
    static const VARTYPE VT = VT_VARIANT|VT_BYREF;
};

template<>
class CVarTypeInfo< BSTR >
{
public:
    static const VARTYPE VT = VT_BSTR;
    static BSTR VARIANT::* const pmField;
};

__declspec( selectany ) BSTR VARIANT::* const CVarTypeInfo< BSTR >::pmField = &VARIANT::bstrVal;

template<>
class CVarTypeInfo< BSTR* >
{
public:
    static const VARTYPE VT = VT_BSTR|VT_BYREF;
    static BSTR* VARIANT::* const pmField;
};

__declspec( selectany ) BSTR* VARIANT::* const CVarTypeInfo< BSTR* >::pmField = &VARIANT::pbstrVal;

template<>
class CVarTypeInfo< IUnknown* >
{
public:
    static const VARTYPE VT = VT_UNKNOWN;
    static IUnknown* VARIANT::* const pmField;
};

__declspec( selectany ) IUnknown* VARIANT::* const CVarTypeInfo< IUnknown* >::pmField = &VARIANT::punkVal;

template<>
class CVarTypeInfo< IUnknown** >
{
public:
    static const VARTYPE VT = VT_UNKNOWN|VT_BYREF;
    static IUnknown** VARIANT::* const pmField;
};

__declspec( selectany ) IUnknown** VARIANT::* const CVarTypeInfo< IUnknown** >::pmField = &VARIANT::ppunkVal;

template<>
class CVarTypeInfo< IDispatch* >
{
public:
    static const VARTYPE VT = VT_DISPATCH;
    static IDispatch* VARIANT::* const pmField;
};

__declspec( selectany ) IDispatch* VARIANT::* const CVarTypeInfo< IDispatch* >::pmField = &VARIANT::pdispVal;

template<>
class CVarTypeInfo< IDispatch** >
{
public:
    static const VARTYPE VT = VT_DISPATCH|VT_BYREF;
    static IDispatch** VARIANT::* const pmField;
};

__declspec( selectany ) IDispatch** VARIANT::* const CVarTypeInfo< IDispatch** >::pmField = &VARIANT::ppdispVal;

template<>
class CVarTypeInfo< CY >
{
public:
    static const VARTYPE VT = VT_CY;
    static CY VARIANT::* const pmField;
};

__declspec( selectany ) CY VARIANT::* const CVarTypeInfo< CY >::pmField = &VARIANT::cyVal;

template<>
class CVarTypeInfo< CY* >
{
public:
    static const VARTYPE VT = VT_CY|VT_BYREF;
    static CY* VARIANT::* const pmField;
};

__declspec( selectany ) CY* VARIANT::* const CVarTypeInfo< CY* >::pmField = &VARIANT::pcyVal;

#ifdef _ATL_NO_VARIANT_THROW
#define ATLVARIANT_THROW()		throw()
#else
#define ATLVARIANT_THROW()
#endif

class CComVariant :
    public tagVARIANT
{
// Constructors
public:
    CComVariant() throw()
    {
        // Make sure that variant data are initialized to 0
        memset(this, 0, sizeof(tagVARIANT));
        ::VariantInit(this);
    }
    ~CComVariant() throw()
    {
        HRESULT hr = Clear();
        ATLASSERT(SUCCEEDED(hr));
        (hr);
    }
    CComVariant(_In_ const VARIANT& varSrc) ATLVARIANT_THROW()
    {
        vt = VT_EMPTY;
        InternalCopy(&varSrc);
    }
    CComVariant(_In_ const CComVariant& varSrc) ATLVARIANT_THROW()
    {
        vt = VT_EMPTY;
        InternalCopy(&varSrc);
    }
    CComVariant(_In_z_ LPCOLESTR lpszSrc) ATLVARIANT_THROW()
    {
        vt = VT_EMPTY;
        *this = lpszSrc;
    }
    CComVariant(_In_z_ LPCSTR lpszSrc) ATLVARIANT_THROW()
    {
        vt = VT_EMPTY;
        *this = lpszSrc;
    }
    CComVariant(_In_ bool bSrc) throw()
    {
        vt = VT_BOOL;
        boolVal = bSrc ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
    }

    CComVariant(_In_ int nSrc, _In_ VARTYPE vtSrc = VT_I4) ATLVARIANT_THROW()
    {
        ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_INT);
        if (vtSrc == VT_I4 || vtSrc == VT_INT)
        {
            vt = vtSrc;
            intVal = nSrc;
        }
        else
        {
            vt = VT_ERROR;
            scode = E_INVALIDARG;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_INVALIDARG);
#endif
        }
    }

    CComVariant(_In_ BYTE nSrc) throw()
    {
        vt = VT_UI1;
        bVal = nSrc;
    }
    CComVariant(_In_ short nSrc) throw()
    {
        vt = VT_I2;
        iVal = nSrc;
    }
    CComVariant(_In_ long nSrc, _In_ VARTYPE vtSrc = VT_I4) ATLVARIANT_THROW()
    {
        ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR);
        if (vtSrc == VT_I4 || vtSrc == VT_ERROR)
        {
            vt = vtSrc;
            lVal = nSrc;
        }
        else
        {
            vt = VT_ERROR;
            scode = E_INVALIDARG;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_INVALIDARG);
#endif
        }
    }

    CComVariant(_In_ float fltSrc) throw()
    {
        vt = VT_R4;
        fltVal = fltSrc;
    }
    CComVariant(_In_ double dblSrc, _In_ VARTYPE vtSrc = VT_R8) ATLVARIANT_THROW()
    {
        ATLASSERT(vtSrc == VT_R8 || vtSrc == VT_DATE);
        if (vtSrc == VT_R8 || vtSrc == VT_DATE)
        {
            vt = vtSrc;
            dblVal = dblSrc;
        }
        else
        {
            vt = VT_ERROR;
            scode = E_INVALIDARG;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_INVALIDARG);
#endif
        }
    }

    CComVariant(_In_ LONGLONG nSrc) throw()
    {
        vt = VT_I8;
        llVal = nSrc;
    }
    CComVariant(_In_ ULONGLONG nSrc) throw()
    {
        vt = VT_UI8;
        ullVal = nSrc;
    }
    CComVariant(_In_ CY cySrc) throw()
    {
        vt = VT_CY;
        cyVal.Hi = cySrc.Hi;
        cyVal.Lo = cySrc.Lo;
    }
    CComVariant(_In_opt_ IDispatch* pSrc) throw()
    {
        vt = VT_DISPATCH;
        pdispVal = pSrc;
        // Need to AddRef as VariantClear will Release
        if (pdispVal != NULL)
            pdispVal->AddRef();
    }
    CComVariant(_In_opt_ IUnknown* pSrc) throw()
    {
        vt = VT_UNKNOWN;
        punkVal = pSrc;
        // Need to AddRef as VariantClear will Release
        if (punkVal != NULL)
            punkVal->AddRef();
    }
    CComVariant(_In_ char cSrc) throw()
    {
        vt = VT_I1;
        cVal = cSrc;
    }
    CComVariant(_In_ unsigned short nSrc) throw()
    {
        vt = VT_UI2;
        uiVal = nSrc;
    }
    CComVariant(_In_ unsigned long nSrc) throw()
    {
        vt = VT_UI4;
        ulVal = nSrc;
    }
    CComVariant(_In_ unsigned int nSrc, _In_ VARTYPE vtSrc = VT_UI4) ATLVARIANT_THROW()
    {
        ATLASSERT(vtSrc == VT_UI4 || vtSrc == VT_UINT);
        if (vtSrc == VT_UI4 || vtSrc == VT_UINT)
        {
            vt = vtSrc;
            uintVal= nSrc;
        }
        else
        {
            vt = VT_ERROR;
            scode = E_INVALIDARG;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_INVALIDARG);
#endif
        }
    }
    CComVariant(_In_ const CComBSTR& bstrSrc) ATLVARIANT_THROW()
    {
        vt = VT_EMPTY;
        *this = bstrSrc;
    }
    CComVariant(_In_ const SAFEARRAY *pSrc) ATLVARIANT_THROW()
    {
        ATLASSERT(pSrc != NULL);
        if (pSrc == NULL)
        {
            vt = VT_ERROR;
            scode = E_INVALIDARG;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_INVALIDARG);
#endif
        }
        else
        {
            LPSAFEARRAY pCopy;
            HRESULT hRes = ::SafeArrayCopy((LPSAFEARRAY)pSrc, &pCopy);
            if (SUCCEEDED(hRes))
            {
                hRes = ::ATL::AtlSafeArrayGetActualVartype((LPSAFEARRAY)pSrc, &vt);
                if (SUCCEEDED(hRes))
                {
                    vt |= VT_ARRAY;
                    parray = pCopy;
                }
                else
                {
                    vt = VT_ERROR;
                    scode = hRes;
                }
            }
            else
            {
                vt = VT_ERROR;
                scode = hRes;
            }

#ifndef _ATL_NO_VARIANT_THROW
            if (FAILED(hRes))
            {
                if(hRes == E_OUTOFMEMORY)
                {
                    AtlThrow(E_OUTOFMEMORY);
                }
                else
                {
                    ATLENSURE_THROW(FALSE, hRes);
                }
            }
#endif
        }
    }
// Assignment Operators
public:
    CComVariant& operator=(_In_ const CComVariant& varSrc) ATLVARIANT_THROW()
    {
        if(this!=&varSrc)
        {
            InternalCopy(&varSrc);
        }
        return *this;
    }

    CComVariant& operator=(_In_ const VARIANT& varSrc) ATLVARIANT_THROW()
    {
        if(static_cast<VARIANT *>(this)!=&varSrc)
        {
            InternalCopy(&varSrc);
        }
        return *this;
    }

    CComVariant& operator=(_In_ const CComBSTR& bstrSrc) ATLVARIANT_THROW()
    {
        ClearThrow();

        vt = VT_BSTR;
        bstrVal = bstrSrc.Copy();

        if (bstrVal == NULL && bstrSrc.m_str != NULL)
        {
            vt = VT_ERROR;
            scode = E_OUTOFMEMORY;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_OUTOFMEMORY);
#endif
        }

        return *this;
    }

    CComVariant& operator=(_In_z_ LPCOLESTR lpszSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_BSTR || bstrVal != lpszSrc)
        {
            ClearThrow();

            vt = VT_BSTR;
            bstrVal = ::SysAllocString(lpszSrc);

            if (bstrVal == NULL && lpszSrc != NULL)
            {
                vt = VT_ERROR;
                scode = E_OUTOFMEMORY;
#ifndef _ATL_NO_VARIANT_THROW
                AtlThrow(E_OUTOFMEMORY);
#endif
            }
        }
        return *this;
    }

    CComVariant& operator=(_In_z_ LPCSTR lpszSrc) ATLVARIANT_THROW()
    {
        USES_CONVERSION_EX;
        ClearThrow();

        vt = VT_BSTR;
        bstrVal = ::SysAllocString(A2COLE_EX(lpszSrc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD));

        if (bstrVal == NULL && lpszSrc != NULL)
        {
            vt = VT_ERROR;
            scode = E_OUTOFMEMORY;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_OUTOFMEMORY);
#endif
        }
        return *this;
    }

    CComVariant& operator=(_In_ bool bSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_BOOL)
        {
            ClearThrow();
            vt = VT_BOOL;
        }
        boolVal = bSrc ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
        return *this;
    }

    CComVariant& operator=(_In_ int nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_I4)
        {
            ClearThrow();
            vt = VT_I4;
        }
        intVal = nSrc;

        return *this;
    }

    CComVariant& operator=(_In_ BYTE nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_UI1)
        {
            ClearThrow();
            vt = VT_UI1;
        }
        bVal = nSrc;
        return *this;
    }

    CComVariant& operator=(_In_ short nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_I2)
        {
            ClearThrow();
            vt = VT_I2;
        }
        iVal = nSrc;
        return *this;
    }

    CComVariant& operator=(_In_ long nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_I4)
        {
            ClearThrow();
            vt = VT_I4;
        }
        lVal = nSrc;
        return *this;
    }

    CComVariant& operator=(_In_ float fltSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_R4)
        {
            ClearThrow();
            vt = VT_R4;
        }
        fltVal = fltSrc;
        return *this;
    }

    CComVariant& operator=(_In_ double dblSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_R8)
        {
            ClearThrow();
            vt = VT_R8;
        }
        dblVal = dblSrc;
        return *this;
    }

    CComVariant& operator=(_In_ CY cySrc) ATLVARIANT_THROW()
    {
        if (vt != VT_CY)
        {
            ClearThrow();
            vt = VT_CY;
        }
        cyVal.Hi = cySrc.Hi;
        cyVal.Lo = cySrc.Lo;
        return *this;
    }

    CComVariant& operator=(_Inout_opt_ IDispatch* pSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_DISPATCH || pSrc != pdispVal)
        {
            ClearThrow();

            vt = VT_DISPATCH;
            pdispVal = pSrc;
            // Need to AddRef as VariantClear will Release
            if (pdispVal != NULL)
                pdispVal->AddRef();
        }
        return *this;
    }

    CComVariant& operator=(_Inout_opt_ IUnknown* pSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_UNKNOWN || pSrc != punkVal)
        {
            ClearThrow();

            vt = VT_UNKNOWN;
            punkVal = pSrc;

            // Need to AddRef as VariantClear will Release
            if (punkVal != NULL)
                punkVal->AddRef();
        }
        return *this;
    }

    CComVariant& operator=(_In_ char cSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_I1)
        {
            ClearThrow();
            vt = VT_I1;
        }
        cVal = cSrc;
        return *this;
    }

    CComVariant& operator=(_In_ unsigned short nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_UI2)
        {
            ClearThrow();
            vt = VT_UI2;
        }
        uiVal = nSrc;
        return *this;
    }

    CComVariant& operator=(_In_ unsigned long nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_UI4)
        {
            ClearThrow();
            vt = VT_UI4;
        }
        ulVal = nSrc;
        return *this;
    }

    CComVariant& operator=(_In_ unsigned int nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_UI4)
        {
            ClearThrow();
            vt = VT_UI4;
        }
        uintVal= nSrc;
        return *this;
    }

    CComVariant& operator=(_In_ BYTE* pbSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_UI1|VT_BYREF))
        {
            ClearThrow();
            vt = VT_UI1|VT_BYREF;
        }
        pbVal = pbSrc;
        return *this;
    }

    CComVariant& operator=(_In_ short* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_I2|VT_BYREF))
        {
            ClearThrow();
            vt = VT_I2|VT_BYREF;
        }
        piVal = pnSrc;
        return *this;
    }

#ifdef _NATIVE_WCHAR_T_DEFINED
    CComVariant& operator=(_In_ USHORT* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_UI2|VT_BYREF))
        {
            ClearThrow();
            vt = VT_UI2|VT_BYREF;
        }
        puiVal = pnSrc;
        return *this;
    }
#endif

    CComVariant& operator=(_In_ int* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_I4|VT_BYREF))
        {
            ClearThrow();
            vt = VT_I4|VT_BYREF;
        }
        pintVal = pnSrc;
        return *this;
    }

    CComVariant& operator=(_In_ UINT* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_UI4|VT_BYREF))
        {
            ClearThrow();
            vt = VT_UI4|VT_BYREF;
        }
        puintVal = pnSrc;
        return *this;
    }

    CComVariant& operator=(_In_ long* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_I4|VT_BYREF))
        {
            ClearThrow();
            vt = VT_I4|VT_BYREF;
        }
        plVal = pnSrc;
        return *this;
    }

    CComVariant& operator=(_In_ ULONG* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_UI4|VT_BYREF))
        {
            ClearThrow();
            vt = VT_UI4|VT_BYREF;
        }
        pulVal = pnSrc;
        return *this;
    }

    CComVariant& operator=(_In_ LONGLONG nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_I8)
        {
            ClearThrow();
            vt = VT_I8;
        }
        llVal = nSrc;

        return *this;
    }

    CComVariant& operator=(_In_ LONGLONG* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_I8|VT_BYREF))
        {
            ClearThrow();
            vt = VT_I8|VT_BYREF;
        }
        pllVal = pnSrc;
        return *this;
    }

    CComVariant& operator=(_In_ ULONGLONG nSrc) ATLVARIANT_THROW()
    {
        if (vt != VT_UI8)
        {
            ClearThrow();
            vt = VT_UI8;
        }
        ullVal = nSrc;

        return *this;
    }

    CComVariant& operator=(_In_ ULONGLONG* pnSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_UI8|VT_BYREF))
        {
            ClearThrow();
            vt = VT_UI8|VT_BYREF;
        }
        pullVal = pnSrc;
        return *this;
    }

    CComVariant& operator=(_In_ float* pfSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_R4|VT_BYREF))
        {
            ClearThrow();
            vt = VT_R4|VT_BYREF;
        }
        pfltVal = pfSrc;
        return *this;
    }

    CComVariant& operator=(_In_ double* pfSrc) ATLVARIANT_THROW()
    {
        if (vt != (VT_R8|VT_BYREF))
        {
            ClearThrow();
            vt = VT_R8|VT_BYREF;
        }
        pdblVal = pfSrc;
        return *this;
    }

    CComVariant& operator=(_In_ const SAFEARRAY *pSrc) ATLVARIANT_THROW()
    {
        ATLASSERT(pSrc != NULL);

        if (pSrc == NULL)
        {
            ClearThrow();
            vt = VT_ERROR;
            scode = E_INVALIDARG;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(E_INVALIDARG);
#endif
        }
        else if ((vt & VT_ARRAY) == 0 || pSrc != parray)
        {
            ClearThrow();
            LPSAFEARRAY pCopy;
            HRESULT hr = ::SafeArrayCopy((LPSAFEARRAY)pSrc, &pCopy);
            if (SUCCEEDED(hr))
            {
                ::ATL::AtlSafeArrayGetActualVartype((LPSAFEARRAY)pSrc, &vt);
                vt |= VT_ARRAY;
                parray = pCopy;
            }
            else
            {
                vt = VT_ERROR;
                scode = hr;
#ifndef _ATL_NO_VARIANT_THROW
                if(hr == E_OUTOFMEMORY)
                {
                    AtlThrow(E_OUTOFMEMORY);
                }
                else
                {
                    ATLENSURE_THROW(FALSE, hr);
                }
#endif
            }
        }

        return *this;
    }

// Comparison Operators
public:
    bool operator==(_In_ const VARIANT& varSrc) const throw()
    {
        // For backwards compatibility
        if (vt == VT_NULL && varSrc.vt == VT_NULL)
        {
            return true;
        }
        // Variants not equal if types don't match
        if (vt != varSrc.vt)
        {
            return false;
        }
        return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_EQ);
    }

    bool operator!=(_In_ const VARIANT& varSrc) const throw()
    {
        return !operator==(varSrc);
    }

    bool operator<(_In_ const VARIANT& varSrc) const throw()
    {
        if (vt == VT_NULL && varSrc.vt == VT_NULL)
            return false;
        return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0)== static_cast<HRESULT>(VARCMP_LT);
    }

    bool operator>(_In_ const VARIANT& varSrc) const throw()
    {
        if (vt == VT_NULL && varSrc.vt == VT_NULL)
            return false;
        return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0)== static_cast<HRESULT>(VARCMP_GT);
    }

private:
    inline HRESULT VarCmp(
        _In_ LPVARIANT pvarLeft,
        _In_ LPVARIANT pvarRight,
        _In_ LCID lcid,
        _In_ ULONG dwFlags) const throw();

// Operations
public:
    HRESULT Clear()
    {
        return ::VariantClear(this);
    }
    HRESULT Copy(_In_ const VARIANT* pSrc)
    {
        return ::VariantCopy(this, const_cast<VARIANT*>(pSrc));
    }

    // copy VARIANT to BSTR
    HRESULT CopyTo(_Outptr_result_z_ BSTR *pstrDest) const
    {
        ATLASSERT(pstrDest != NULL && vt == VT_BSTR);
        HRESULT hRes = E_POINTER;
        if (pstrDest != NULL && vt == VT_BSTR)
        {
            *pstrDest = ::SysAllocStringByteLen((char*)bstrVal, ::SysStringByteLen(bstrVal));
            if (*pstrDest == NULL)
                hRes = E_OUTOFMEMORY;
            else
                hRes = S_OK;
        }
        else if (vt != VT_BSTR)
            hRes = DISP_E_TYPEMISMATCH;

        return hRes;
    }

    HRESULT Attach(_In_ VARIANT* pSrc)
    {
        if(pSrc == NULL)
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        if (this != pSrc)
        {
            // Clear out the variant
            hr = Clear();
            if (SUCCEEDED(hr))
            {
                // Copy the contents and give control to CComVariant
                Checked::memcpy_s(this, sizeof(CComVariant), pSrc, sizeof(VARIANT));
                pSrc->vt = VT_EMPTY;
                hr = S_OK;
            }
        }
        return hr;
    }

    HRESULT Detach(_Inout_ VARIANT* pDest)
    {
        ATLASSERT(pDest != NULL);
        if(pDest == NULL)
            return E_POINTER;

        // Clear out the variant
        HRESULT hr = ::VariantClear(pDest);
        if (SUCCEEDED(hr))
        {
            // Copy the contents and remove control from CComVariant
            Checked::memcpy_s(pDest, sizeof(VARIANT), this, sizeof(VARIANT));
            vt = VT_EMPTY;
            hr = S_OK;
        }
        return hr;
    }

    HRESULT ChangeType(_In_ VARTYPE vtNew, _In_opt_ const VARIANT* pSrc = NULL)
    {
        VARIANT* pVar = const_cast<VARIANT*>(pSrc);
        // Convert in place if pSrc is NULL
        if (pVar == NULL)
            pVar = this;
        // Do nothing if doing in place convert and vts not different
        return ::VariantChangeType(this, pVar, 0, vtNew);
    }

    template< typename T >
    void SetByRef(_In_ T* pT) ATLVARIANT_THROW()
    {
        ClearThrow();
        vt = CVarTypeInfo< T* >::VT;
        byref = pT;
    }

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
    _Check_return_ HRESULT WriteToStream(_Inout_ IStream* pStream);
    _Check_return_ HRESULT WriteToStream(
        _Inout_ IStream* pStream,
        _In_ VARTYPE vtWrite)
    {
        if (vtWrite != VT_EMPTY && vtWrite != vt)
        {
            CComVariant varConv;
            HRESULT hr = varConv.ChangeType(vtWrite, this);
            if (FAILED(hr))
            {
                return hr;
            }
            return varConv.WriteToStream(pStream);
        }
        return WriteToStream(pStream);
    }

    _Check_return_ HRESULT ReadFromStream(
        _Inout_ IStream* pStream,
        _In_ VARTYPE vtExpected = VT_EMPTY);

    _Check_return_ HRESULT ReadFromStream(
        _Inout_ IStream* pStream,
        _In_ VARTYPE vtExpected,
        _In_ ClassesAllowedInStream rgclsidAllowed,
        _In_ DWORD cclsidAllowed);

    // Return the size in bytes of the current contents
    ULONG GetSize() const;
    HRESULT GetSizeMax(_Out_ ULARGE_INTEGER* pcbSize) const;
#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

// Implementation
private:
    void ClearThrow() ATLVARIANT_THROW()
    {
        HRESULT hr = Clear();
        ATLASSERT(SUCCEEDED(hr));
        (hr);
#ifndef _ATL_NO_VARIANT_THROW
        if (FAILED(hr))
        {
            AtlThrow(hr);
        }
#endif
    }

public:
    _Check_return_ HRESULT InternalClear() ATLVARIANT_THROW()
    {
        HRESULT hr = Clear();
        ATLASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            vt = VT_ERROR;
            scode = hr;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(hr);
#endif
        }
        return hr;
    }

    void InternalCopy(_In_ const VARIANT* pSrc) ATLVARIANT_THROW()
    {
        HRESULT hr = Copy(pSrc);
        if (FAILED(hr))
        {
            vt = VT_ERROR;
            scode = hr;
#ifndef _ATL_NO_VARIANT_THROW
            AtlThrow(hr);
#endif
        }
    }
};

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

#pragma warning(push)
#pragma warning(disable: 4702)
_Check_return_ inline HRESULT CComVariant::WriteToStream(_Inout_ IStream* pStream)
{
    if(pStream == NULL)
        return E_INVALIDARG;

    HRESULT hr = pStream->Write(&vt, sizeof(VARTYPE), NULL);
    if (FAILED(hr))
        return hr;

    int cbWrite = 0;
    switch (vt)
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
        {
            CComPtr<IPersistStream> spStream;
            if (punkVal != NULL)
            {
                hr = punkVal->QueryInterface(__uuidof(IPersistStream), (void**)&spStream);
                if (FAILED(hr))
                {
                    hr = punkVal->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spStream);
                    if (FAILED(hr))
                    {
                        spStream.Detach();
                        return hr;
                    }
                }
            }
            if (spStream != NULL)
                return OleSaveToStream(spStream, pStream);
            return WriteClassStm(pStream, CLSID_NULL);
        }
    case VT_UI1:
    case VT_I1:
        cbWrite = sizeof(BYTE);
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        cbWrite = sizeof(short);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        cbWrite = sizeof(long);
        break;
    case VT_I8:
    case VT_UI8:
        cbWrite = sizeof(LONGLONG);
        break;
    case VT_R8:
    case VT_CY:
    case VT_DATE:
        cbWrite = sizeof(double);
        break;
    default:
        break;
    }
    if (cbWrite != 0)
        return pStream->Write((void*) &bVal, cbWrite, NULL);

    CComBSTR bstrWrite;
    CComVariant varBSTR;
    if (vt != VT_BSTR)
    {
        hr = VariantChangeType(&varBSTR, this, VARIANT_NOVALUEPROP, VT_BSTR);
        if (FAILED(hr))
            return hr;
        bstrWrite.Attach(varBSTR.bstrVal);
    }
    else
        bstrWrite.Attach(bstrVal);

    hr = bstrWrite.WriteToStream(pStream);
    bstrWrite.Detach();
    return hr;
}
#pragma warning(pop)	// C4702


_Check_return_ inline HRESULT CComVariant::ReadFromStream(
    _Inout_ IStream* pStream,
    _In_ VARTYPE vtExpected /* = VT_EMPTY */)
{
    ClassesAllowedInStream allowed;
    allowed.rgclsidAllowed = NULL;

    return ReadFromStream(pStream, vtExpected, allowed, 0);
}

_Check_return_ inline HRESULT CComVariant::ReadFromStream(
    _Inout_ IStream* pStream,
    _In_ VARTYPE vtExpected,
    _In_ ClassesAllowedInStream rgclsidAllowed,
    _In_ DWORD cclsidAllowed)
{
    ATLASSERT(pStream != NULL);
    if(pStream == NULL)
        return E_INVALIDARG;

    HRESULT hr;
    hr = VariantClear(this);
    if (FAILED(hr))
        return hr;
    VARTYPE vtRead = VT_EMPTY;
    ULONG cbRead = 0;

    hr = pStream->Read(&vtRead, sizeof(VARTYPE), &cbRead);
ATLPREFAST_SUPPRESS(6102)
    if (hr == S_FALSE || (cbRead != sizeof(VARTYPE) && hr == S_OK))
        hr = E_FAIL;
ATLPREFAST_UNSUPPRESS()
    if (FAILED(hr))
        return hr;
    if (vtExpected != VT_EMPTY && vtRead != vtExpected)
        return E_FAIL;

    vt = vtRead;
    cbRead = 0;
    switch (vtRead)
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
        {
            punkVal = NULL;
            hr = AtlInternalOleLoadFromStream(pStream,
                (vtRead == VT_UNKNOWN) ? __uuidof(IUnknown) : __uuidof(IDispatch),
                (void**)&punkVal, rgclsidAllowed, cclsidAllowed);
            // If IPictureDisp or IFontDisp property is not set,
            // OleLoadFromStream() will return REGDB_E_CLASSNOTREG.
            if (hr == REGDB_E_CLASSNOTREG)
                hr = S_OK;
            return hr;
        }
    case VT_UI1:
    case VT_I1:
        cbRead = sizeof(BYTE);
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        cbRead = sizeof(short);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        cbRead = sizeof(long);
        break;
    case VT_I8:
    case VT_UI8:
        cbRead = sizeof(LONGLONG);
        break;
    case VT_R8:
    case VT_CY:
    case VT_DATE:
        cbRead = sizeof(double);
        break;
    default:
        break;
    }
    if (cbRead != 0)
    {
        hr = pStream->Read((void*) &bVal, cbRead, NULL);
        if (hr == S_FALSE)
            hr = E_FAIL;
        return hr;
    }
    CComBSTR bstrRead;

    hr = bstrRead.ReadFromStream(pStream);
    if (FAILED(hr))
    {
        // If CComBSTR::ReadFromStream failed, reset seek pointer to start of
        // variant type.
        LARGE_INTEGER nOffset;
        nOffset.QuadPart = -(static_cast<LONGLONG>(sizeof(VARTYPE)));
        pStream->Seek(nOffset, STREAM_SEEK_CUR, NULL);
        vt = VT_EMPTY;
        return hr;
    }
    vt = VT_BSTR;
    bstrVal = bstrRead.Detach();
    if (vtRead != VT_BSTR)
        hr = ChangeType(vtRead);
    return hr;
}

inline HRESULT CComVariant::GetSizeMax(_Out_ ULARGE_INTEGER* pcbSize) const
{
    ATLASSERT(pcbSize != NULL);
    if (pcbSize == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    ULARGE_INTEGER nSize;
    nSize.QuadPart = sizeof(VARTYPE);

    switch (vt)
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
        {
            nSize.LowPart += sizeof(CLSID);

            if (punkVal != NULL)
            {
                CComPtr<IPersistStream> spStream;

                hr = punkVal->QueryInterface(__uuidof(IPersistStream), (void**)&spStream);
                if (FAILED(hr))
                {
                    hr = punkVal->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spStream);
                    if (FAILED(hr))
                    {
                        break;
                    }
                }

                ULARGE_INTEGER nPersistSize;
                nPersistSize.QuadPart = 0;

                ATLASSERT(spStream != NULL);
                hr = spStream->GetSizeMax(&nPersistSize);
                if (SUCCEEDED(hr))
                {
                    hr = AtlAdd(&nSize.QuadPart, nSize.QuadPart, nPersistSize.QuadPart);
                }
            }
        }
        break;
    case VT_UI1:
    case VT_I1:
        nSize.LowPart += sizeof(BYTE);
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        nSize.LowPart += sizeof(short);
        break;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        nSize.LowPart += sizeof(long);
        break;
    case VT_I8:
    case VT_UI8:
        nSize.LowPart += sizeof(LONGLONG);
        break;
    case VT_R8:
    case VT_CY:
    case VT_DATE:
        nSize.LowPart += sizeof(double);
        break;
    default:
        {
            VARTYPE vtTmp = vt;
            BSTR bstr = NULL;
            CComVariant varBSTR;
            if (vtTmp != VT_BSTR)
            {
                hr = VariantChangeType(&varBSTR, const_cast<VARIANT*>((const VARIANT*)this), VARIANT_NOVALUEPROP, VT_BSTR);
                if (SUCCEEDED(hr))
                {
                    bstr = varBSTR.bstrVal;
                    vtTmp = VT_BSTR;
                }
            }
            else
            {
                bstr = bstrVal;
            }

            if (vtTmp == VT_BSTR)
            {
                // Add the size of the length + string (in bytes) + NULL terminator.
                nSize.QuadPart += CComBSTR::GetStreamSize(bstr);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        pcbSize->QuadPart = nSize.QuadPart;
    }

    return hr;
}

inline ATL_DEPRECATED("GetSize has been replaced by GetSizeMax")
ULONG CComVariant::GetSize() const
{
    ULARGE_INTEGER nSize;
    HRESULT hr = GetSizeMax(&nSize);

    if (SUCCEEDED(hr) && nSize.QuadPart <= ULONG_MAX)
    {
        return nSize.LowPart;
    }

    return sizeof(VARTYPE);
}

_Check_return_ inline HRESULT CComPtr<IDispatch>::Invoke2(
    _In_ DISPID dispid,
    _In_ VARIANT* pvarParam1,
    _In_ VARIANT* pvarParam2,
    _Out_opt_ VARIANT* pvarRet) throw()
{
    if(pvarParam1 == NULL || pvarParam2 == NULL)
        return E_INVALIDARG;

    CComVariant varArgs[2] = { *pvarParam2, *pvarParam1 };
    DISPPARAMS dispparams = { &varArgs[0], NULL, 2, 0};
    return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
}

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

/*
    Workaround for VarCmp function which does not compare VT_I1, VT_UI2, VT_UI4, VT_UI8 values
*/
inline HRESULT CComVariant::VarCmp(
    _In_ LPVARIANT pvarLeft,
    _In_ LPVARIANT pvarRight,
    _In_ LCID lcid,
    _In_ ULONG dwFlags) const throw()
{
    switch(vt)
    {
        case VT_I1:
            if (pvarLeft->cVal == pvarRight->cVal)
            {
                return VARCMP_EQ;
            }
            return pvarLeft->cVal > pvarRight->cVal ? VARCMP_GT : VARCMP_LT;
        case VT_UI2:
            if (pvarLeft->uiVal == pvarRight->uiVal)
            {
                return VARCMP_EQ;
            }
            return pvarLeft->uiVal > pvarRight->uiVal ? VARCMP_GT : VARCMP_LT;

        case VT_UI4:
            if (pvarLeft->uintVal == pvarRight->uintVal)
            {
                return VARCMP_EQ;
            }
            return pvarLeft->uintVal > pvarRight->uintVal ? VARCMP_GT : VARCMP_LT;

        case VT_UI8:
            if (pvarLeft->ullVal == pvarRight->ullVal)
            {
                return VARCMP_EQ;
            }
            return pvarLeft->ullVal > pvarRight->ullVal ? VARCMP_GT : VARCMP_LT;

        default:
            return ::VarCmp(pvarLeft, pvarRight, lcid, dwFlags);
    }
}

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

ATLPREFAST_SUPPRESS(6387)
_Check_return_ inline HRESULT AtlInternalOleLoadFromStream(
    _Inout_ IStream* pStm,
    _In_ REFIID iidInterface,
    _Outptr_ void** ppvObj,
    _In_ ClassesAllowedInStream rgclsidAllowed,
    _In_ DWORD cclsidAllowed)
{
    ATLASSUME(pStm != NULL);
    *ppvObj = NULL;
    CLSID clsid;

    HRESULT hr = ReadClassStm(pStm, &clsid);

    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IUnknown> punkVal;

    if (cclsidAllowed != 0)
    {
        ATLASSUME(rgclsidAllowed.rgclsidAllowed != NULL);
        hr = E_ACCESSDENIED;

        for(DWORD i = 0; i < cclsidAllowed; i++)
        {
            if (IsEqualCLSID(clsid, rgclsidAllowed.rgclsidAllowed[i]))
            {
                hr = S_OK;
                break;
            }
        }
    }
    else if (rgclsidAllowed.pfnClsidAllowed != NULL)
    {
        hr = rgclsidAllowed.pfnClsidAllowed(clsid, iidInterface, reinterpret_cast<void**>(&punkVal));
    }

    if (FAILED(hr))
    {
        return hr;
    }

    if (punkVal == NULL)
    {
        hr = CoCreateInstance(clsid, NULL, CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD, iidInterface, reinterpret_cast<void**>(&punkVal));
        if (FAILED(hr))
        {
            return hr;
        }
    }

    CComPtr<IPersistStream> pPersistStm;
    hr = punkVal->QueryInterface(&pPersistStm);

    if (SUCCEEDED(hr))
    {
        hr = pPersistStm->Load(pStm);

        if (SUCCEEDED(hr))
        {
            *ppvObj = punkVal.Detach();
        }
    }

    return hr;
}
ATLPREFAST_UNSUPPRESS()

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

}	// namespace ATL
#pragma pack(pop)

#pragma warning (pop)

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

#endif	// __ATLCOMCLI_H__
