// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLMEM_H__
#define __ATLMEM_H__

#pragma once

#include <atlcore.h>
#include <limits.h>


#pragma pack(push,_ATL_PACKING)
namespace ATL
{

template< typename N >
inline N WINAPI AtlAlignUp(
	_In_ N n,
	_In_ ULONG nAlign) throw()
{
	return( N( (n+(nAlign-1))&~(N( nAlign )-1) ) );
}

template< typename N >
inline N WINAPI AtlAlignDown(
	_In_ N n,
	_In_ ULONG nAlign) throw()
{
	return( N( n&~(N( nAlign )-1) ) );
}

__interface __declspec(uuid("654F7EF5-CFDF-4df9-A450-6C6A13C622C0")) IAtlMemMgr
{
public:
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) void* Allocate(_In_ size_t nBytes) throw();
	void Free(_Inout_opt_ void* p) throw();
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) void* Reallocate(
		_Inout_updates_bytes_opt_(nBytes) void* p,
		_In_ size_t nBytes) throw();
	size_t GetSize(_In_ void* p) throw();
};

class CCRTHeap :
	public IAtlMemMgr
{
public:
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Allocate(_In_ size_t nBytes) throw()
	{
		return( malloc( nBytes ) );
	}
	virtual void Free(_In_opt_ void* p) throw()
	{
		free( p );
	}
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Reallocate(
		_In_opt_ void* p,
		_In_ size_t nBytes) throw()
	{
		return( realloc( p, nBytes ) );
	}
	virtual size_t GetSize(_In_ void* p) throw()
	{
		return( _msize( p ) );
	}

public:
};

class CWin32Heap :
	public IAtlMemMgr
{
public:
	CWin32Heap() throw() :
		m_hHeap( NULL ),
		m_bOwnHeap( false )
	{
	}
	CWin32Heap(_In_ HANDLE hHeap) throw() :
		m_hHeap( hHeap ),
		m_bOwnHeap( false )
	{
		ATLASSERT( hHeap != NULL );
	}
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
	CWin32Heap(
			_In_ DWORD dwFlags,
			_In_ size_t nInitialSize,
			_In_ size_t nMaxSize = 0 ) :
		m_hHeap( NULL ),
		m_bOwnHeap( true )
	{
		ATLASSERT( !(dwFlags&HEAP_GENERATE_EXCEPTIONS) );
		m_hHeap = ::HeapCreate( dwFlags, nInitialSize, nMaxSize );
		if( m_hHeap == NULL )
		{
			AtlThrowLastWin32();
		}
	}
#endif
	virtual ~CWin32Heap() throw()
	{
#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP
		if( m_bOwnHeap && (m_hHeap != NULL) )
		{
			BOOL bSuccess;

			bSuccess = ::HeapDestroy( m_hHeap );
			ATLASSERT( bSuccess );
		}
#endif
	}

	void Attach(
		_In_ HANDLE hHeap,
		_In_ bool bTakeOwnership) throw()
	{
		ATLASSERT( hHeap != NULL );
		ATLASSUME( m_hHeap == NULL );

		m_hHeap = hHeap;
		m_bOwnHeap = bTakeOwnership;
	}
	HANDLE Detach() throw()
	{
		HANDLE hHeap;

		hHeap = m_hHeap;
		m_hHeap = NULL;
		m_bOwnHeap = false;

		return( hHeap );
	}

// IAtlMemMgr
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Allocate(_In_ size_t nBytes) throw()
	{
		return( ::HeapAlloc( m_hHeap, 0, nBytes ) );
	}
	virtual void Free(_In_opt_ void* p) throw()
	{
		if( p != NULL )
		{
			BOOL bSuccess;

			bSuccess = ::HeapFree( m_hHeap, 0, p );
			ATLASSERT( bSuccess );
		}
	}
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Reallocate(
		_In_opt_ void* p,
		_In_ size_t nBytes) throw()
	{
		if( p == NULL )
		{
			return( Allocate( nBytes ) );
		}

		if (nBytes==0)
		{
			  Free(p);
			  return NULL;
		}

		return( ::HeapReAlloc( m_hHeap, 0, p, nBytes ) );
	}
	virtual size_t GetSize(_Inout_ void* p) throw()
	{
		return( ::HeapSize( m_hHeap, 0, p ) );
	}

public:
	HANDLE m_hHeap;
	bool m_bOwnHeap;
};

#ifdef _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

class CLocalHeap :
	public IAtlMemMgr
{
// IAtlMemMgr
public:
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Allocate(_In_ size_t nBytes) throw()
	{
		return( ::LocalAlloc( LMEM_FIXED, nBytes ) );
	}
	virtual void Free(_In_opt_ void* p) throw()
	{
		::LocalFree( p );
	}
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Reallocate(
		_In_opt_ void* p,
		_In_ size_t nBytes) throw()
	{
		if (p==NULL)
		{
			return ( Allocate(nBytes) );
		}
		if (nBytes==0)
		{
			Free(p);
			return NULL;
		}

		return SAL_Assume_bytecap_for_opt_(::LocalReAlloc(p, nBytes, 0), nBytes);
	}
	virtual size_t GetSize(_In_ void* p) throw()
	{
		return( ::LocalSize( p ) );
	}
};

class CGlobalHeap :
	public IAtlMemMgr
{
// IAtlMemMgr
public:
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Allocate(_In_ size_t nBytes) throw()
	{
		return( ::GlobalAlloc( LMEM_FIXED, nBytes ) );
	}
	virtual void Free(_In_opt_ void* p) throw()
	{
		::GlobalFree( p );
	}
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR virtual void* Reallocate(
		_In_opt_ void* p,
		_In_ size_t nBytes) throw()
	{
		if (p==NULL)
		{
			return ( Allocate(nBytes) );
		}
		if (nBytes==0){
			Free(p);
			return NULL;
		}
		return SAL_Assume_bytecap_for_opt_(::GlobalReAlloc( p, nBytes, 0 ), nBytes);
	}
	virtual size_t GetSize(_In_ void* p) throw()
	{
		return( ::GlobalSize( p ) );
	}
};

#endif // _ATL_USE_WINAPI_FAMILY_DESKTOP_APP

};  // namespace ATL
#pragma pack(pop)

#ifdef _OBJBASE_H_
#include <atlcommem.h>
#endif	// _OBJBASE_H_

#endif  //__ATLMEM_H__
