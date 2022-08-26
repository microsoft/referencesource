/***
* comip.h - Native C++ compiler COM support - COM interface pointers header
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef _M_CEE_PURE
#error comip.h header cannot be included under /clr:safe or /clr:pure
#endif

#if !defined(_INC_COMIP)
#define _INC_COMIP

#include <ole2.h>
#include <malloc.h>

#include <comutil.h>

#pragma warning(push)
#pragma warning(disable: 4290)

#pragma push_macro("new")
#undef new

#include <new.h>

class _com_error;

void __stdcall _com_issue_error(HRESULT);
struct __declspec(uuid("00000000-0000-0000-c000-000000000046")) IUnknown;

// Provide Interface to IID association
//
template<typename _Interface, const IID* _IID /*= &__uuidof(_Interface)*/>
class _com_IIID {
public:
    typedef _Interface Interface;

    static _Interface* GetInterfacePtr() throw()
    {
        return NULL;
    }

    static _Interface& GetInterface() throw()
    {
        return *GetInterfacePtr();
    }

    static const IID& GetIID() throw()
    {
        return *_IID;
    }
};

template<typename _IIID> class _com_ptr_t {
public:
    // Declare interface type so that the type may be available outside
    // the scope of this template.
    //
    typedef _IIID ThisIIID;
    typedef typename _IIID::Interface Interface;

    // When the compiler supports references in template parameters,
    // _CLSID will be changed to a reference.  To avoid conversion
    // difficulties this function should be used to obtain the
    // CLSID.
    //
    static const IID& GetIID() throw()
    {
        return ThisIIID::GetIID();
    }

    // Constructs a smart-pointer from any other smart pointer.
    //
    template<typename _OtherIID> _com_ptr_t(const _com_ptr_t<_OtherIID>& p)
        : m_pInterface(NULL)
    {
        HRESULT hr = _QueryInterface(p);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }
    }

    // Constructs a smart-pointer from any IUnknown-based interface pointer.
    //
    template<typename _InterfaceType> _com_ptr_t(_InterfaceType* p)
        : m_pInterface(NULL)
    {
        HRESULT hr = _QueryInterface(p);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }
    }

    // Make sure correct ctor is called
    //
    template<> _com_ptr_t(_In_ LPSTR str)
    {
        new(this) _com_ptr_t(static_cast<LPCSTR> (str), NULL);
    }

    // Make sure correct ctor is called
    //
    template<> _com_ptr_t(_In_ LPWSTR str)
    {
        new(this) _com_ptr_t(static_cast<LPCWSTR> (str), NULL);
    }

    // Disable conversion using _com_ptr_t* specialization of
    // template<typename _InterfaceType> _com_ptr_t(_InterfaceType* p)
    //
    template<> explicit _com_ptr_t(_com_ptr_t* p)
        : m_pInterface(NULL)
    {
        if (p == NULL) {
            _com_issue_error(E_POINTER);
        }
        else {
            m_pInterface = p->m_pInterface;
            AddRef();
        }
    }

    // Default constructor.
    //
    _com_ptr_t() throw()
        : m_pInterface(NULL)
    {
    }

    // This constructor is provided to allow NULL assignment. It will issue
    // an error if any value other than null is assigned to the object.
    //
    _com_ptr_t(int null)
        : m_pInterface(NULL)
    {
        if (null != 0) {
            _com_issue_error(E_POINTER);
        }
    }

#if defined(_NATIVE_NULLPTR_SUPPORTED) && !defined(_DO_NOT_USE_NULLPTR_IN_COM_PTR_T)

    // This constructor is provided to allow nullptr assignment.
    _com_ptr_t(decltype(__nullptr)) : m_pInterface(NULL) { }

#endif // defined(_NATIVE_NULLPTR_SUPPORTED) && !defined(_DO_NOT_USE_NULLPTR_IN_COM_PTR_T)

    // Copy the pointer and AddRef().
    //
    _com_ptr_t(const _com_ptr_t& cp) throw()
        : m_pInterface(cp.m_pInterface)
    {
        _AddRef();
    }

    // Move the pointer.
    //
    _com_ptr_t(_com_ptr_t&& cp) throw()
        : m_pInterface(cp.m_pInterface)
    {
        cp.m_pInterface = nullptr;
    }

    // Saves the interface.
    //
    template<> _com_ptr_t(Interface* pInterface) throw()
        : m_pInterface(pInterface)
    {
        _AddRef();
    }

    // Copies the pointer. If fAddRef is TRUE, the interface will
    // be AddRef()ed.
    //
    _com_ptr_t(Interface* pInterface, bool fAddRef) throw()
        : m_pInterface(pInterface)
    {
        if (fAddRef) {
            _AddRef();
        }
    }

    // Construct a pointer for a _variant_t object.
    //
    _com_ptr_t(const _variant_t& varSrc)
        : m_pInterface(NULL)
    {
        HRESULT hr = QueryStdInterfaces(varSrc);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }
    }

    // Calls CoCreateClass with the provided CLSID.
    //
    explicit _com_ptr_t(const CLSID& clsid, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
        : m_pInterface(NULL)
    {
        HRESULT hr = CreateInstance(clsid, pOuter, dwClsContext);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }
    }

    // Calls CoCreateClass with the provided CLSID retrieved from
    // the string.
    //
    explicit _com_ptr_t(LPCWSTR str, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
        : m_pInterface(NULL)
    {
        HRESULT hr = CreateInstance(str, pOuter, dwClsContext);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }
    }

    // Calls CoCreateClass with the provided SBCS CLSID retrieved from
    // the string.
    //
    explicit _com_ptr_t(LPCSTR str, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
        : m_pInterface(NULL)
    {
        HRESULT hr = CreateInstance(str, pOuter, dwClsContext);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }
    }

    // Queries for interface.
    //
    template<typename _OtherIID> _com_ptr_t& operator=(const _com_ptr_t<_OtherIID>& p)
    {
        HRESULT hr = _QueryInterface(p);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }

        return *this;
    }

    // Queries for interface.
    //
    template<typename _InterfaceType> _com_ptr_t& operator=(_InterfaceType* p)
    {
        HRESULT hr = _QueryInterface(p);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }

        return *this;
    }

    // Saves the interface.
    //
    template<> _com_ptr_t& operator=(Interface* pInterface) throw()
    {
        if (m_pInterface != pInterface) {
            Interface* pOldInterface = m_pInterface;

            m_pInterface = pInterface;

            _AddRef();

            if (pOldInterface != nullptr) {
                pOldInterface->Release();
            }
        }

        return *this;
    }

    // Copies and AddRef()'s the interface.
    //
    _com_ptr_t& operator=(const _com_ptr_t& cp) throw()
    {
        return operator=(cp.m_pInterface);
    }

    // Moves the interface.
    //
    _com_ptr_t& operator=(_com_ptr_t&& cp) throw()
    {
        if (m_pInterface != cp.m_pInterface) {
            Interface* pOldInterface = m_pInterface;

            m_pInterface = cp.m_pInterface;
            cp.m_pInterface = nullptr;

            if (pOldInterface != nullptr) {
                pOldInterface->Release();
            }
        }

        return *this;
    }

    // This operator is provided to permit the assignment of NULL to the class.
    // It will issue an error if any value other than NULL is assigned to it.
    //
    _com_ptr_t& operator=(int null)
    {
        if (null != 0) {
            _com_issue_error(E_POINTER);
        }

        return operator=(reinterpret_cast<Interface*>(NULL));
    }

    // Construct a pointer for a _variant_t object.
    //
    _com_ptr_t& operator=(const _variant_t& varSrc)
    {
        HRESULT hr = QueryStdInterfaces(varSrc);

        if (FAILED(hr) && (hr != E_NOINTERFACE)) {
            _com_issue_error(hr);
        }

        return *this;
    }

    // If we still have an interface then Release() it. The interface
    // may be NULL if Detach() has previously been called, or if it was
    // never set.
    //
    ~_com_ptr_t() throw()
    {
        _Release();
    }

    // Saves/sets the interface without AddRef()ing. This call
    // will release any previously acquired interface.
    //
    void Attach(Interface* pInterface) throw()
    {
        _Release();
        m_pInterface = pInterface;
    }

    // Saves/sets the interface only AddRef()ing if fAddRef is TRUE.
    // This call will release any previously acquired interface.
    //
    void Attach(Interface* pInterface, bool fAddRef) throw()
    {
        _Release();
        m_pInterface = pInterface;

        if (fAddRef) {
            if (pInterface == NULL) {
                _com_issue_error(E_POINTER);
            }
            else {
                pInterface->AddRef();
            }
        }
    }

    // Simply NULL the interface pointer so that it isn't Released()'ed.
    //
    Interface* Detach() throw()
    {
        Interface* const old = m_pInterface;
        m_pInterface = NULL;
        return old;
    }

    // Return the interface. This value may be NULL.
    //
    operator Interface*() const throw()
    {
        return m_pInterface;
    }

    // Queries for the unknown and return it
    // Provides minimal level error checking before use.
    //
    operator Interface&() const
    {
        if (m_pInterface == NULL) {
            _com_issue_error(E_POINTER);
        }

        return *m_pInterface;
    }

    // Allows an instance of this class to act as though it were the
    // actual interface. Also provides minimal error checking.
    //
    Interface& operator*() const
    {
        if (m_pInterface == NULL) {
            _com_issue_error(E_POINTER);
        }

        return *m_pInterface;
    }

    // Returns the address of the interface pointer contained in this
    // class. This is useful when using the COM/OLE interfaces to create
    // this interface.
    //
    Interface** operator&() throw()
    {
        _Release();
        m_pInterface = NULL;
        return &m_pInterface;
    }

    // Allows this class to be used as the interface itself.
    // Also provides simple error checking.
    //
    Interface* operator->() const
    {
        if (m_pInterface == NULL) {
            _com_issue_error(E_POINTER);
        }

        return m_pInterface;
    }

    // This operator is provided so that simple boolean expressions will
    // work.  For example: "if (p) ...".
    // Returns TRUE if the pointer is not NULL.
    //
    operator bool() const throw()
    {
        return m_pInterface != NULL;
    }

    // Compare two smart pointers
    //
    template<typename _OtherIID> bool operator==(const _com_ptr_t<_OtherIID>& p) const
    {
        return _CompareUnknown(p) == 0;
    }

    // Compare two pointers
    //
    template<typename _InterfaceType> bool operator==(_InterfaceType* p) const
    {
        return _CompareUnknown(p) == 0;
    }

    // Compare with other interface
    //
    template<> bool operator==(Interface* p) const
    {
        return (m_pInterface == p)
                    ? true
                    : _CompareUnknown(p) == 0;
    }

    // Compare two smart pointers
    //
    template<> bool operator==(const _com_ptr_t& p) const throw()
    {
        return operator==(p.m_pInterface);
    }

    // For comparison to NULL
    //
    bool operator==(int null) const
    {
        if (null != 0) {
            _com_issue_error(E_POINTER);
        }

        return m_pInterface == NULL;
    }

    // Compare two smart pointers
    //
    template<typename _OtherIID> bool operator!=(const _com_ptr_t<_OtherIID>& p) const
    {
        return !(operator==(p));
    }

    // Compare two pointers
    //
    template<typename _InterfaceType> bool operator!=(_InterfaceType* p) const
    {
        return !(operator==(p));
    }

    // For comparison to NULL
    //
    bool operator!=(int null) const
    {
        return !(operator==(null));
    }

    // Compare two smart pointers
    //
    template<typename _OtherIID> bool operator<(const _com_ptr_t<_OtherIID>& p) const
    {
        return _CompareUnknown(p) < 0;
    }

    // Compare two pointers
    //
    template<typename _InterfaceType> bool operator<(_InterfaceType* p) const
    {
        return _CompareUnknown(p) < 0;
    }

    // Compare two smart pointers
    //
    template<typename _OtherIID> bool operator>(const _com_ptr_t<_OtherIID>& p) const
    {
        return _CompareUnknown(p) > 0;
    }

    // Compare two pointers
    //
    template<typename _InterfaceType> bool operator>(_InterfaceType* p) const
    {
        return _CompareUnknown(p) > 0;
    }

    // Compare two smart pointers
    //
    template<typename _OtherIID> bool operator<=(const _com_ptr_t<_OtherIID>& p) const
    {
        return _CompareUnknown(p) <= 0;
    }

    // Compare two pointers
    //
    template<typename _InterfaceType> bool operator<=(_InterfaceType* p) const
    {
        return _CompareUnknown(p) <= 0;
    }

    // Compare two smart pointers
    //
    template<typename _OtherIID> bool operator>=(const _com_ptr_t<_OtherIID>& p) const
    {
        return _CompareUnknown(p) >= 0;
    }

    // Compare two pointers
    //
    template<typename _InterfaceType> bool operator>=(_InterfaceType* p) const
    {
        return _CompareUnknown(p) >= 0;
    }

    // Provides error-checking Release()ing of this interface.
    //
    void Release()
    {
        if (m_pInterface == NULL) {
            _com_issue_error(E_POINTER);
        }
        else {
            m_pInterface->Release();
            m_pInterface = NULL;
        }
    }

    // Provides error-checking AddRef()ing of this interface.
    //
    void AddRef()
    {
        if (m_pInterface == NULL) {
            _com_issue_error(E_POINTER);
        }
        else {
            m_pInterface->AddRef();
        }
    }

    // Another way to get the interface pointer without casting.
    //
    Interface* GetInterfacePtr() const throw()
    {
        return m_pInterface;
    }

    // Another way to get the interface pointer without casting.
    // Use for [in, out] parameter passing
    Interface*& GetInterfacePtr() throw()
    {
        return m_pInterface;
    }

    // Loads an interface for the provided CLSID.
    // Returns an HRESULT.  Any previous interface is unconditionally released.
    //
    HRESULT CreateInstance(const CLSID& rclsid, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        HRESULT hr;

        _Release();

        if (dwClsContext & (CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)) {
            IUnknown* pIUnknown;
            hr = CoCreateInstance(rclsid, pOuter, dwClsContext, __uuidof(IUnknown), reinterpret_cast<void**>(&pIUnknown));

            if (SUCCEEDED(hr)) {
                hr = OleRun(pIUnknown);

                if (SUCCEEDED(hr)) {
                    hr = pIUnknown->QueryInterface(GetIID(), reinterpret_cast<void**>(&m_pInterface));
                }

                pIUnknown->Release();
            }
        }
        else {
            hr = CoCreateInstance(rclsid, pOuter, dwClsContext, GetIID(), reinterpret_cast<void**>(&m_pInterface));
        }

        if (FAILED(hr)) {
            // just in case refcount = 0 and dtor gets called
            m_pInterface = NULL;
        }

        return hr;
    }

    // Creates the class specified by clsidString.  clsidString may
    // contain a class id, or a prog id string.
    //
    HRESULT CreateInstance(LPCWSTR clsidString, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        if (clsidString == NULL) {
            return E_INVALIDARG;
        }

        CLSID clsid;
        HRESULT hr;

        if (clsidString[0] == L'{') {
            hr = CLSIDFromString(const_cast<LPWSTR> (clsidString), &clsid);
        }
        else {
            hr = CLSIDFromProgID(const_cast<LPWSTR> (clsidString), &clsid);
        }

        if (FAILED(hr)) {
            return hr;
        }

        return CreateInstance(clsid, pOuter, dwClsContext);
    }

    // Creates the class specified by SBCS clsidString.  clsidString may
    // contain a class id, or a prog id string.
    //
    HRESULT CreateInstance(LPCSTR clsidStringA, IUnknown* pOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
    {
        if (clsidStringA == NULL) {
            return E_INVALIDARG;
        }

        size_t const size = strlen(clsidStringA) + 1;

        if (size > INT_MAX) {
            return E_INVALIDARG;
        }

        int const destSize = MultiByteToWideChar(CP_ACP, 0, clsidStringA, static_cast<int>(size), NULL, 0);

        if (destSize == 0) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LPWSTR clsidStringW;
        clsidStringW = static_cast<LPWSTR>(_malloca(destSize * sizeof(WCHAR)));

        if (clsidStringW == NULL) {
            return E_OUTOFMEMORY;
        }

        if (MultiByteToWideChar(CP_ACP, 0, clsidStringA, static_cast<int>(size), clsidStringW, destSize) == 0) {
           _freea(clsidStringW);
           return HRESULT_FROM_WIN32(GetLastError());
        }

        HRESULT hr=CreateInstance(clsidStringW, pOuter, dwClsContext);
        _freea(clsidStringW);
        return hr;
    }

    // Attach to the active object specified by rclsid.
    // Any previous interface is released.
    //
    HRESULT GetActiveObject(const CLSID& rclsid) throw()
    {
        _Release();

        IUnknown* pIUnknown;

        HRESULT hr = ::GetActiveObject(rclsid, NULL, &pIUnknown);

        if (SUCCEEDED(hr)) {
            hr = pIUnknown->QueryInterface(GetIID(), reinterpret_cast<void**>(&m_pInterface));

            pIUnknown->Release();
        }

        if (FAILED(hr)) {
            // just in case refcount = 0 and dtor gets called
            m_pInterface = NULL;
        }

        return hr;
    }

    // Attach to the active object specified by clsidString.
    // First convert the LPCWSTR to a CLSID.
    //
    HRESULT GetActiveObject(LPCWSTR clsidString) throw()
    {
        if (clsidString == NULL) {
            return E_INVALIDARG;
        }

        CLSID clsid;
        HRESULT hr;

        if (clsidString[0] == '{') {
            hr = CLSIDFromString(const_cast<LPWSTR> (clsidString), &clsid);
        }
        else {
            hr = CLSIDFromProgID(const_cast<LPWSTR> (clsidString), &clsid);
        }

        if (FAILED(hr)) {
            return hr;
        }

        return GetActiveObject(clsid);
    }

    // Attach to the active object specified by clsidStringA.
    // First convert the LPCSTR to a LPCWSTR.
    //
    HRESULT GetActiveObject(LPCSTR clsidStringA) throw()
    {
        if (clsidStringA == NULL) {
            return E_INVALIDARG;
        }

        size_t const size = strlen(clsidStringA) + 1;

        if (size > INT_MAX) {
            return E_INVALIDARG;
        }

        int const destSize = MultiByteToWideChar(CP_ACP, 0, clsidStringA, static_cast<int>(size), NULL, 0);

        LPWSTR clsidStringW;
        __try {
            clsidStringW = static_cast<LPWSTR>(_alloca(destSize * sizeof(WCHAR)));
        }
        __except (GetExceptionCode() == STATUS_STACK_OVERFLOW) {
            clsidStringW = NULL;
        }

        if (clsidStringW == NULL) {
            return E_OUTOFMEMORY;
        }

        if (MultiByteToWideChar(CP_ACP, 0, clsidStringA, static_cast<int>(size), clsidStringW, destSize) == 0) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return GetActiveObject(clsidStringW);
    }

    // Performs the QI for the specified IID and returns it in p.
    // As with all QIs, the interface will be AddRef'd.
    //
    template<typename _InterfaceType> HRESULT QueryInterface(const IID& iid, _InterfaceType*& p) throw ()
    {
        if (m_pInterface != NULL) {
            return m_pInterface->QueryInterface(iid, reinterpret_cast<void**>(&p));
        }

        return E_POINTER;
    }

    // Performs the QI for the specified IID and returns it in p.
    // As with all QIs, the interface will be AddRef'd.
    //
    template<typename _InterfaceType> HRESULT QueryInterface(const IID& iid, _InterfaceType** p) throw()
    {
        return QueryInterface(iid, *p);
    }

private:
    // The Interface.
    //
    Interface* m_pInterface;

    // Releases only if the interface is not null.
    // The interface is not set to NULL.
    //
    void _Release() throw()
    {
        if (m_pInterface != NULL) {
            m_pInterface->Release();
        }
    }

    // AddRefs only if the interface is not NULL
    //
    void _AddRef() throw()
    {
        if (m_pInterface != NULL) {
            m_pInterface->AddRef();
        }
    }

    // Performs a QI on pUnknown for the interface type returned
    // for this class.  The interface is stored.  If pUnknown is
    // NULL, or the QI fails, E_NOINTERFACE is returned and
    // _pInterface is set to NULL.
    //
    template<typename _InterfacePtr> HRESULT _QueryInterface(_InterfacePtr p) throw()
    {
        HRESULT hr;

        // Can't QI NULL
        //
        if (p != NULL) {
            // Query for this interface
            //
            Interface* pInterface = NULL;
            hr = p->QueryInterface(GetIID(), reinterpret_cast<void**>(&pInterface));

            // Save the interface without AddRef()ing.
            //
            Attach(SUCCEEDED(hr)? pInterface: NULL);
        }
        else {
            operator=(static_cast<Interface*>(NULL));
            hr = E_NOINTERFACE;
        }

        return hr;
    }

    // Compares the provided pointer with this by obtaining IUnknown interfaces
    // for each pointer and then returning the difference.
    //
    template<typename _InterfacePtr> int _CompareUnknown(_InterfacePtr p) const
    {
        IUnknown* pu1 = NULL;
        IUnknown* pu2 = NULL;

        if (m_pInterface != NULL) {
            HRESULT hr = m_pInterface->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pu1));

            if (FAILED(hr)) {
                pu1 = NULL;
                _com_issue_error(hr);
            }
            else {
                pu1->Release();
            }
        }

        if (p != NULL) {
            HRESULT hr = p->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pu2));

            if (FAILED(hr)) {
                pu2 = NULL;
                _com_issue_error(hr);
            }
            else {
                pu2->Release();
            }
        }

        if (pu1 == pu2)
        {
            return 0;
        }

        return (pu1 > pu2) ? 1 : -1;
    }

    // Try to extract either IDispatch* or an IUnknown* from
    // the VARIANT
    //
    HRESULT QueryStdInterfaces(const _variant_t& varSrc) throw()
    {
        if (V_VT(&varSrc) == VT_DISPATCH) {
            return _QueryInterface(V_DISPATCH(&varSrc));
        }

        if (V_VT(&varSrc) == VT_UNKNOWN) {
            return _QueryInterface(V_UNKNOWN(&varSrc));
        }

        // We have something other than an IUnknown or an IDispatch.
        // Can we convert it to either one of these?
        // Try IDispatch first
        //
        VARIANT varDest;
        VariantInit(&varDest);

        HRESULT hr = VariantChangeType(&varDest, const_cast<VARIANT*>(static_cast<const VARIANT*>(&varSrc)), 0, VT_DISPATCH);
        if (SUCCEEDED(hr)) {
            hr = _QueryInterface(V_DISPATCH(&varDest));
        }

        if (hr == E_NOINTERFACE) {
            // That failed ... so try IUnknown
            //
            VariantInit(&varDest);
            hr = VariantChangeType(&varDest, const_cast<VARIANT*>(static_cast<const VARIANT*>(&varSrc)), 0, VT_UNKNOWN);
            if (SUCCEEDED(hr)) {
                hr = _QueryInterface(V_UNKNOWN(&varDest));
            }
        }

        VariantClear(&varDest);
        return hr;
    }
};

// Reverse comparison operators for _com_ptr_t
//
template<typename _InterfaceType> bool operator==(int null, const _com_ptr_t<_InterfaceType>& p)
{
    if (null != 0) {
        _com_issue_error(E_POINTER);
    }

    return p == NULL;
}

template<typename _Interface, typename _InterfacePtr> bool operator==(_Interface* i, const _com_ptr_t<_InterfacePtr>& p)
{
    return p == i;
}

template<typename _Interface> bool operator!=(int null, const _com_ptr_t<_Interface>& p)
{
    if (null != 0) {
        _com_issue_error(E_POINTER);
    }

    return p != NULL;
}

template<typename _Interface, typename _InterfacePtr> bool operator!=(_Interface* i, const _com_ptr_t<_InterfacePtr>& p)
{
    return p != i;
}

template<typename _Interface> bool operator<(int null, const _com_ptr_t<_Interface>& p)
{
    if (null != 0) {
        _com_issue_error(E_POINTER);
    }

    return p > NULL;
}

template<typename _Interface, typename _InterfacePtr> bool operator<(_Interface* i, const _com_ptr_t<_InterfacePtr>& p)
{
    return p > i;
}

template<typename _Interface> bool operator>(int null, const _com_ptr_t<_Interface>& p)
{
    if (null != 0) {
        _com_issue_error(E_POINTER);
    }

    return p < NULL;
}

template<typename _Interface, typename _InterfacePtr> bool operator>(_Interface* i, const _com_ptr_t<_InterfacePtr>& p)
{
    return p < i;
}

template<typename _Interface> bool operator<=(int null, const _com_ptr_t<_Interface>& p)
{
    if (null != 0) {
        _com_issue_error(E_POINTER);
    }

    return p >= NULL;
}

template<typename _Interface, typename _InterfacePtr> bool operator<=(_Interface* i, const _com_ptr_t<_InterfacePtr>& p)
{
    return p >= i;
}

template<typename _Interface> bool operator>=(int null, const _com_ptr_t<_Interface>& p)
{
    if (null != 0) {
        _com_issue_error(E_POINTER);
    }

    return p <= NULL;
}

template<typename _Interface, typename _InterfacePtr> bool operator>=(_Interface* i, const _com_ptr_t<_InterfacePtr>& p)
{
    return p <= i;
}

#pragma pop_macro("new")
#pragma warning(pop)

#endif // _INC_COMIP
