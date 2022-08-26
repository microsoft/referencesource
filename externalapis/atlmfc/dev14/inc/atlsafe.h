// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLSAFE_H__
#define __ATLSAFE_H__

#pragma once

#include <atlbase.h>

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

// This struct is used with CComSafeArray to set the matching VARTYPE based on
// template argument type passed in to CComSafeArray.
template <typename T>
struct _ATL_AutomationType
{
};

#define DEFINE_AUTOMATION_TYPE_FUNCTION(ctype, typewrapper, oleautomationtype) \
	template <> \
	struct _ATL_AutomationType<ctype> \
	{ \
		typedef typewrapper _typewrapper;\
		enum { type = oleautomationtype }; \
		static void* GetT(const ctype& t) throw() \
		{ \
			return (void*)&t; \
		} \
	};
	// specialization for BSTR so GetT doesn't return &BSTR
	template <>
	struct _ATL_AutomationType<BSTR>
	{
		typedef CComBSTR _typewrapper ;
		enum { type = VT_BSTR };
		static void* GetT(_In_ const BSTR& t) throw()
		{
			return t;
		}
	};
	// specialization for LPUNKNOWN so GetT doesn't return &LPUNKNOWN
	template <>
	struct _ATL_AutomationType<LPUNKNOWN>
	{
		typedef CComPtr<IUnknown> _typewrapper;
		enum { type = VT_UNKNOWN};
		static void* GetT(_In_ const LPUNKNOWN& t) throw()
		{
			return t;
		}
	};
	// specialization for LPDISPATCH so GetT doesn't return &LPDISPATCH
	template <>
	struct _ATL_AutomationType<LPDISPATCH>
	{
		typedef CComPtr<IDispatch> _typewrapper;
		enum { type = VT_DISPATCH};
		static void* GetT(_In_ const LPDISPATCH& t) throw()
		{
			return t;
		}
	};

	DEFINE_AUTOMATION_TYPE_FUNCTION(CHAR		, CHAR			,VT_I1)
	DEFINE_AUTOMATION_TYPE_FUNCTION(SHORT		, SHORT			,VT_I2)
	DEFINE_AUTOMATION_TYPE_FUNCTION(INT			, INT			,VT_I4)
	DEFINE_AUTOMATION_TYPE_FUNCTION(LONG		, LONG			,VT_I4)
	DEFINE_AUTOMATION_TYPE_FUNCTION(LONGLONG	, LONGLONG		,VT_I8)
	DEFINE_AUTOMATION_TYPE_FUNCTION(BYTE		, BYTE			,VT_UI1)
	DEFINE_AUTOMATION_TYPE_FUNCTION(USHORT		, USHORT		,VT_UI2)
	DEFINE_AUTOMATION_TYPE_FUNCTION(UINT		, UINT			,VT_UI4)
	DEFINE_AUTOMATION_TYPE_FUNCTION(ULONG		, ULONG			,VT_UI4)
	DEFINE_AUTOMATION_TYPE_FUNCTION(ULONGLONG	, ULONGLONG		,VT_UI8)
	DEFINE_AUTOMATION_TYPE_FUNCTION(FLOAT		, FLOAT			,VT_R4)
	DEFINE_AUTOMATION_TYPE_FUNCTION(DOUBLE		, DOUBLE		,VT_R8)
	DEFINE_AUTOMATION_TYPE_FUNCTION(DECIMAL		, DECIMAL		,VT_DECIMAL)
	DEFINE_AUTOMATION_TYPE_FUNCTION(VARIANT		, CComVariant	,VT_VARIANT)
	DEFINE_AUTOMATION_TYPE_FUNCTION(CY			, CY			,VT_CY)

// wrapper for SAFEARRAYBOUND used with CComSafeArray
class CComSafeArrayBound :
	public SAFEARRAYBOUND
{
public:
	CComSafeArrayBound(
		_In_ ULONG ulCount = 0,
		_In_ LONG lLowerBound = 0) throw()
	{
		cElements = ulCount;
		lLbound = lLowerBound;
	}
	CComSafeArrayBound& operator=(_In_ const CComSafeArrayBound& bound) throw()
	{
		cElements = bound.cElements;
		lLbound = bound.lLbound;
		return *this;
	}
	CComSafeArrayBound& operator=(_In_ ULONG ulCount) throw()
	{
		cElements = ulCount;
		lLbound = 0;
		return *this;
	}
	ULONG GetCount() const throw()
	{
		return cElements;
	}
	ULONG SetCount(_In_ ULONG ulCount) throw()
	{
		cElements = ulCount;
		return cElements;
	}
	LONG GetLowerBound() const throw()
	{
		return lLbound;
	}
	LONG SetLowerBound(_In_ LONG lLowerBound) throw()
	{
		lLbound = lLowerBound;
		return lLbound;
	}
	LONG GetUpperBound() const throw()
	{
		return lLbound + cElements - 1;
	}
};

// wrapper for SAFEARRAY.  T is type stored (e.g. BSTR, VARIANT, etc.)
template <typename T, VARTYPE _vartype = _ATL_AutomationType<T>::type>
class CComSafeArray
{
public:
// Constructors
	CComSafeArray() throw() : m_psa(NULL)
	{
	}
	// create SAFEARRAY where number of elements = ulCount
	explicit CComSafeArray(
		_In_ ULONG ulCount,
		_In_ LONG lLBound = 0) : m_psa(NULL)
	{
		CComSafeArrayBound bound(ulCount, lLBound);
		HRESULT hRes = Create(&bound);
		if (FAILED(hRes))
			AtlThrow(hRes);
	}
	explicit CComSafeArray(_In_ const SAFEARRAYBOUND& bound) : m_psa(NULL)
	{
		HRESULT hRes = Create(&bound);
		if (FAILED(hRes))
			AtlThrow(hRes);
	}
	// pass an array of SAFEARRAYBOUNDs for multi-dimensional
	explicit CComSafeArray(
		_In_ const SAFEARRAYBOUND *pBound,
		_In_ UINT uDims = 1) : m_psa(NULL)
	{
		ATLASSERT(pBound != NULL);
		ATLASSERT(uDims > 0);
		HRESULT hRes = Create(pBound, uDims);
		if (FAILED(hRes))
			AtlThrow(hRes);
	}
	CComSafeArray(_In_ const SAFEARRAY *psaSrc) : m_psa(NULL)
	{
		ATLASSERT(psaSrc != NULL);
		HRESULT hRes = CopyFrom(psaSrc);
		if (FAILED(hRes))
			AtlThrow(hRes);
	}
	CComSafeArray(_In_ const SAFEARRAY& saSrc) : m_psa(NULL)
	{
		HRESULT hRes = CopyFrom(&saSrc);
		if (FAILED(hRes))
			AtlThrow(hRes);
	}
	CComSafeArray(_In_ const CComSafeArray& saSrc) : m_psa(NULL)
	{
		ATLASSUME(saSrc.m_psa != NULL);
		HRESULT hRes = CopyFrom(saSrc.m_psa);
		if (FAILED(hRes))
			AtlThrow(hRes);
	}

	~CComSafeArray() throw()
	{
		HRESULT hRes = Destroy();
		DBG_UNREFERENCED_LOCAL_VARIABLE(hRes);
		ATLASSERT(SUCCEEDED(hRes));
	}

	HRESULT Attach(_In_ const SAFEARRAY *psaSrc)
	{
		ATLENSURE_THROW(psaSrc != NULL, E_INVALIDARG);

		VARTYPE vt;
		HRESULT hRes = ::ATL::AtlSafeArrayGetActualVartype(const_cast<LPSAFEARRAY>(psaSrc), &vt);
		ATLENSURE_SUCCEEDED(hRes);
		ATLENSURE_THROW(vt == GetType(), E_INVALIDARG);

		hRes = Destroy();
		ATLENSURE_SUCCEEDED(hRes);

		m_psa = const_cast<LPSAFEARRAY>(psaSrc);
		hRes = Lock();

		return hRes;
	}
	LPSAFEARRAY Detach()
	{
		Unlock();
		LPSAFEARRAY pTemp = m_psa;
		m_psa = NULL;
		return pTemp;
	}

// overloaded operators
	CComSafeArray<T>& operator=(_In_ const CComSafeArray& saSrc)
	{
		*this = saSrc.m_psa;
		return *this;
	}
	CComSafeArray<T>& operator=(_In_ const SAFEARRAY *psaSrc)
	{
		ATLASSERT(psaSrc != NULL);
		HRESULT hRes = CopyFrom(psaSrc);
		if (FAILED(hRes))
			AtlThrow(hRes);
		return *this;
	}
	operator const SAFEARRAY *() const throw()
	{
		return m_psa;
	}
	operator LPSAFEARRAY() throw()
	{
		return m_psa;
	}
	LPSAFEARRAY* GetSafeArrayPtr() throw()
	{
		return &m_psa;
	}
	const typename _ATL_AutomationType<T>::_typewrapper& operator[](_In_ int nIndex) const
	{
		return GetAt(nIndex);
	}
	typename _ATL_AutomationType<T>::_typewrapper& operator[](_In_ int nIndex)
	{
		return GetAt(nIndex);
	}
	const typename _ATL_AutomationType<T>::_typewrapper& operator[](_In_ LONG nIndex) const
	{
		return GetAt(nIndex);
	}
	typename _ATL_AutomationType<T>::_typewrapper& operator[](_In_ LONG nIndex)
	{
		return GetAt(nIndex);
	}

// info functions
	LONG GetLowerBound(_In_ UINT uDim = 0) const
	{
		ATLASSUME(m_psa != NULL);
		LONG lLBound = 0;
		HRESULT hRes = SafeArrayGetLBound(m_psa, uDim+1, &lLBound);
		if (FAILED(hRes))
			AtlThrow(hRes);
		return lLBound;
	}
	LONG GetUpperBound(_In_ UINT uDim = 0) const
	{
		ATLASSUME(m_psa != NULL);
		LONG lUBound = 0;
		HRESULT hRes = SafeArrayGetUBound(m_psa, uDim+1, &lUBound);
		if (FAILED(hRes))
			AtlThrow(hRes);
		return lUBound;
	}
	ULONG GetCount(_In_ UINT uDim = 0) const
	{
		ATLASSUME(m_psa != NULL);
		ATLASSERT(uDim < GetDimensions());
		LONG lLBound, lUBound;
		HRESULT hRes = SafeArrayGetLBound(m_psa, uDim+1, &lLBound);
		ATLASSERT(SUCCEEDED(hRes));
		if(FAILED(hRes))
			AtlThrow(hRes);
		hRes = SafeArrayGetUBound(m_psa, uDim+1, &lUBound);
		ATLASSERT(SUCCEEDED(hRes));
		if(FAILED(hRes))
			AtlThrow(hRes);
		return (lUBound - lLBound + 1);
	}
	UINT GetDimensions() const
	{
		ATLASSUME(m_psa != NULL);
		return SafeArrayGetDim(m_psa);
	}
	VARTYPE GetType() const throw()
	{
		return _vartype;
	}
	bool IsSizable() const
	{
		ATLASSUME(m_psa != NULL);
		if(m_psa == NULL)
			AtlThrow(E_FAIL);

		return (m_psa->fFeatures & FADF_FIXEDSIZE) ? false : true;
	}

// miscellaneous functions
	const typename _ATL_AutomationType<T>::_typewrapper& GetAt(_In_ LONG lIndex) const
	{
		ATLASSUME(m_psa != NULL);
		if(m_psa == NULL)
			AtlThrow(E_FAIL);

		LONG lLBound = GetLowerBound();
		ATLASSERT(lIndex >= lLBound);
		ATLASSERT(lIndex <= GetUpperBound());
		if ((lIndex < lLBound) || (lIndex > GetUpperBound()))
			AtlThrow(E_INVALIDARG);

		return ((typename _ATL_AutomationType<T>::_typewrapper*)m_psa->pvData)[lIndex-lLBound];
	}

	typename _ATL_AutomationType<T>::_typewrapper& GetAt(_In_ LONG lIndex)
	{
		ATLASSUME(m_psa != NULL);
		if(m_psa == NULL)
			AtlThrow(E_FAIL);

		LONG lLBound = GetLowerBound();
		ATLASSERT(lIndex >= lLBound);
		ATLASSERT(lIndex <= GetUpperBound());
		if ((lIndex < lLBound) || (lIndex > GetUpperBound()))
			AtlThrow(E_INVALIDARG);

		return ((typename _ATL_AutomationType<T>::_typewrapper*)m_psa->pvData)[lIndex-lLBound];
	}
	HRESULT SetAt(
		_In_ LONG lIndex,
		_In_ const T& t,
		_In_ BOOL bCopy = TRUE)
	{
		UNREFERENCED_PARAMETER(bCopy);
		ATLASSUME(m_psa != NULL);
		if(m_psa == NULL)
			return E_FAIL;

		LONG lLBound = GetLowerBound();
		ATLASSERT(lIndex >= lLBound);
		ATLASSERT(lIndex <= GetUpperBound());
		if ((lIndex < lLBound) || (lIndex > GetUpperBound()))
			return E_INVALIDARG;

		((T*)m_psa->pvData)[lIndex-lLBound] = t;
		return S_OK;
	}
	// multi-dimensional version
	HRESULT MultiDimGetAt(_In_ const LONG *alIndex, _Out_ T& t)
	{
		ATLASSUME(m_psa != NULL);
		return SafeArrayGetElement(m_psa, const_cast<LONG*>(alIndex), &t);
	}
	// multi-dimensional version
	HRESULT MultiDimSetAt(_In_ const LONG *alIndex, _In_ const T& t)
	{
		ATLASSUME(m_psa != NULL);
		return SafeArrayPutElement(m_psa, const_cast<LONG*>(alIndex), _ATL_AutomationType<T>::GetT(t));
	}
	// append an item
	HRESULT Add(
		_In_ const T& t,
		_In_ BOOL bCopy = TRUE)
	{
		HRESULT hRes = S_OK;
		if (NULL == m_psa)
		{
			hRes = Create();
			if (FAILED(hRes))
				return hRes;
		}
		ATLASSERT(GetDimensions() == 1); // not for multi-dimensional
		LONG lLBound = GetLowerBound();
		hRes = Resize(GetCount() + 1, lLBound);
		if (SUCCEEDED(hRes))
			return SetAt(GetCount() - 1 + lLBound, t, bCopy);
		else
			return hRes;
	}
	// appends an array of type T items
	HRESULT Add(
		_In_ ULONG ulCount,
		_In_reads_(ulCount) const T *pT,
		_In_ BOOL bCopy = TRUE)
	{
		ATLASSERT(pT != NULL);
		ATLASSERT(ulCount > 0);
		if(pT == NULL)
			return E_INVALIDARG;

		HRESULT hRes = S_OK;
		if (NULL == m_psa)
		{
			hRes = Create();
			if (FAILED(hRes))
				return hRes;
		}
		ATLASSERT(GetDimensions() == 1); // not for multi-dimensional
		ULONG ulLastIndex = GetCount();
		ULONG ulTotalCount = ulLastIndex + ulCount;
		LONG lLBound = GetLowerBound();
		hRes = Resize(ulTotalCount, lLBound);
		if (SUCCEEDED(hRes))
		{
			for (ULONG ulCntr=0; ulCntr<ulCount; ulCntr++, ulLastIndex++)
			{
				hRes = SetAt(ulLastIndex + lLBound, pT[ulCntr], bCopy);
				if (FAILED(hRes))
					break;
			}
		}
		return hRes;
	}
	// appends items in the safearray
	HRESULT Add(_In_ const SAFEARRAY *psaSrc)
	{
		ATLASSUME(psaSrc != NULL);
		// safearrays must only have one dimension
		ATLASSERT(SafeArrayGetDim(const_cast<LPSAFEARRAY>(psaSrc)) == 1);

		HRESULT hRes = S_OK;
		if (NULL == m_psa)
		{
			hRes = Create();
			if (FAILED(hRes))
				return hRes;
		}
		ATLASSERT(GetDimensions() == 1);

		// types must match
		VARTYPE vt;
		hRes = ::ATL::AtlSafeArrayGetActualVartype(const_cast<LPSAFEARRAY>(psaSrc), &vt);
		if(FAILED(hRes))
			return hRes;

		if(GetType() != vt)
		{
			ATLASSERT(FALSE);
			return E_INVALIDARG;
		}

		// resize safearray
		ULONG ulCount = psaSrc->rgsabound[0].cElements;
		ULONG ulLastIndex = GetCount();
		ULONG ulTotalCount = ulLastIndex + ulCount;
		LONG lLBound = GetLowerBound();
		hRes = Resize(ulTotalCount, lLBound);
		if (SUCCEEDED(hRes))
		{
			CComSafeArray saTemp;
			hRes = saTemp.Attach(psaSrc);
			if (FAILED(hRes))
				return hRes;

			LONG lsrcLBound = saTemp.GetLowerBound();
			for(ULONG lCntr = 0; lCntr < ulCount; lCntr++, ulLastIndex++)
			{
				// copy data to end of our safearray
				hRes = SetAt(ulLastIndex + lLBound, saTemp.GetAt(lCntr + lsrcLBound));
				if (FAILED(hRes))
					break;
			}
			saTemp.Detach();
		}
		return hRes;
	}

	// Resize only resizes the right-most dimension
	HRESULT Resize(
		_In_ ULONG ulCount,
		_In_ LONG lLBound = 0)
	{
		ATLASSUME(m_psa != NULL);
		CComSafeArrayBound bound(ulCount, lLBound);
		return Resize(&bound);
	}
	// Resize only resizes the right-most dimension
	HRESULT Resize(_In_ const SAFEARRAYBOUND *pBound)
	{
		ATLASSUME(m_psa != NULL);
		ATLASSUME(pBound != NULL);
		if (!IsSizable())
        {
			return E_FAIL;
        }
		HRESULT hRes = Unlock();
		if (SUCCEEDED(hRes))
		{
			hRes = SafeArrayRedim(m_psa, const_cast<LPSAFEARRAYBOUND>(pBound));
			HRESULT hrLock = Lock();
			if (SUCCEEDED(hRes))
			{
				hRes = hrLock;
			}
		}
		return hRes;
	}
	HRESULT CopyFrom(_In_ const SAFEARRAY *psaSrc)
	{
		ATLENSURE_THROW(psaSrc != NULL, E_INVALIDARG);

		VARTYPE vt;
		HRESULT hRes = ::ATL::AtlSafeArrayGetActualVartype(const_cast<LPSAFEARRAY>(psaSrc), &vt);
		ATLENSURE_SUCCEEDED(hRes);
		ATLENSURE_THROW(vt == GetType(), E_INVALIDARG);

		hRes = Destroy();
		ATLENSURE_SUCCEEDED(hRes);

		hRes = SafeArrayCopy( const_cast<LPSAFEARRAY>(psaSrc), &m_psa );
		ATLENSURE_SUCCEEDED(hRes);

		if( m_psa )
		{
			hRes = Lock();
		}
		return hRes;
	}
	HRESULT CopyTo(_Out_ LPSAFEARRAY *ppArray)
	{
		ATLENSURE_THROW(ppArray != NULL, E_POINTER);
		ATLENSURE(m_psa != NULL);

		return SafeArrayCopy( m_psa, ppArray );
	}
	HRESULT Create(
		_In_ ULONG ulCount = 0,
		_In_ LONG lLBound = 0)
	{
		CComSafeArrayBound bound(ulCount, lLBound);
		return Create(&bound);
	}
	HRESULT Create(
		_In_ const SAFEARRAYBOUND *pBound,
		_In_ UINT uDims = 1)
	{
		ATLASSUME(m_psa == NULL);
		ATLASSERT(uDims > 0);
		if(m_psa != NULL)
			return E_FAIL;

		if(pBound == NULL || uDims == 0)
			return E_INVALIDARG;

		HRESULT hRes = S_OK;
		m_psa = SafeArrayCreate(_vartype, uDims, const_cast<LPSAFEARRAYBOUND>(pBound));
		if (NULL == m_psa)
			hRes = E_OUTOFMEMORY;
		else
			hRes = Lock();
		return hRes;
	}
	HRESULT Destroy()
	{
		HRESULT hRes = S_OK;
		if (m_psa != NULL)
		{
			hRes = Unlock();
			if (SUCCEEDED(hRes))
			{
				hRes = SafeArrayDestroy(m_psa);
				if (SUCCEEDED(hRes))
					m_psa = NULL;
			}
		}
		return hRes;
	}
protected:
	HRESULT Lock()
	{
		ATLASSUME(m_psa != NULL);
		return SafeArrayLock(m_psa);
	}
	HRESULT Unlock()
	{
		ATLASSUME(m_psa != NULL);
		return SafeArrayUnlock(m_psa);
	}
public:
	LPSAFEARRAY m_psa;
};

template<>
HRESULT CComSafeArray<BSTR>::SetAt(
	_In_ LONG lIndex,
	_In_ const BSTR& strData,
	_In_ BOOL bCopy)
{
	ATLASSERT(strData != NULL);
	if(strData == NULL)
		return E_INVALIDARG;

	ATLASSUME(m_psa != NULL);
	LONG lLBound = GetLowerBound();
	ATLASSERT(lIndex >= lLBound);
	ATLASSERT(lIndex <= GetUpperBound());

	if((lIndex < lLBound) || (lIndex > GetUpperBound()))
		return E_INVALIDARG;

	BSTR strOrg = ((BSTR*)m_psa->pvData)[lIndex-lLBound];
	if (strOrg)
		::SysFreeString(strOrg);

	if (bCopy)
	{
		BSTR strTemp = ::SysAllocString(strData);
		if (NULL == strTemp)
			return E_OUTOFMEMORY;
		((BSTR*)m_psa->pvData)[lIndex-lLBound] = strTemp;
	}
	else
		((BSTR*)m_psa->pvData)[lIndex-lLBound] = strData;

	return S_OK;
}
template<>
HRESULT CComSafeArray<VARIANT>::SetAt(
	_In_ LONG lIndex,
	_In_ const VARIANT& varData,
	_In_ BOOL bCopy)
{
	ATLASSUME(m_psa != NULL);
	LONG lLBound = GetLowerBound();
	ATLASSERT(lIndex >= lLBound);
	ATLASSERT(lIndex <= GetUpperBound());

	if((lIndex < lLBound) || (lIndex > GetUpperBound()))
		return E_INVALIDARG;

	if (bCopy)
		return VariantCopyInd(&((VARIANT*)m_psa->pvData)[lIndex-lLBound], const_cast<LPVARIANT>(&varData));
	else
	{
		VARIANT varOrg = ((VARIANT*)m_psa->pvData)[lIndex-lLBound];
		if (V_VT(&varOrg) != VT_EMPTY)
			::VariantClear(&varOrg);
		((VARIANT*)m_psa->pvData)[lIndex-lLBound] = varData;
		return S_OK;
	}
}
template<>
HRESULT CComSafeArray<LPUNKNOWN>::SetAt(
	_In_ LONG lIndex,
	_In_ const LPUNKNOWN& pUnk,
	_In_ BOOL bAddRef)
{
	ATLENSURE_RETURN(pUnk != NULL);
	ATLASSUME(m_psa != NULL);
	LONG lLBound = GetLowerBound();
	ATLASSERT(lIndex >= lLBound);
	ATLASSERT(lIndex <= GetUpperBound());

	if((lIndex < lLBound) || (lIndex > GetUpperBound()))
		return E_INVALIDARG;

	LPUNKNOWN pOrgUnk = ((LPUNKNOWN*)m_psa->pvData)[lIndex-lLBound];
	if (pOrgUnk)
		pOrgUnk->Release();
	if (bAddRef)
		pUnk->AddRef();
	((LPUNKNOWN*)m_psa->pvData)[lIndex-lLBound] = pUnk;
	return S_OK;
}
template<>
HRESULT CComSafeArray<LPDISPATCH>::SetAt(
	_In_ LONG lIndex,
	_In_ const LPDISPATCH& pDisp,
	_In_ BOOL bAddRef)
{
	ATLENSURE_RETURN(pDisp != NULL);
	ATLASSUME(m_psa != NULL);
	LONG lLBound = GetLowerBound();
	ATLASSERT(lIndex >= lLBound);
	ATLASSERT(lIndex <= GetUpperBound());

	if((lIndex < lLBound) || (lIndex > GetUpperBound()))
		return E_INVALIDARG;

	LPDISPATCH pOrgDisp = ((LPDISPATCH*)m_psa->pvData)[lIndex-lLBound];
	if (pOrgDisp)
		pOrgDisp->Release();
	if (bAddRef)
		pDisp->AddRef();
	((LPDISPATCH*)m_psa->pvData)[lIndex-lLBound] = pDisp;
	return S_OK;
}

}; //namespace ATL

#pragma pack(pop)

#endif //__ATLSAFE_H__
