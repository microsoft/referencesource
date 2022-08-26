//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: util.cpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: 
//		Implementation of common classes 
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#include "snipch.hpp"

// Converts from Unicode or Hex to Decimal integer
int Wcstoi(const WCHAR * wszStr)
{
    UINT retVal = 0;
	bool fNegative=false;

	if( wszStr && *wszStr)
	{
		WCHAR ch;

		if(*wszStr==L'-')
		{
			fNegative=true;
			wszStr++;
		}
		else if(*wszStr == L'+')
			wszStr++;

		// Check if it is a hex number
		if( (L'0' == wszStr[0]) && ((L'X' == wszStr[1]) || (L'x' == wszStr[1])) )
		{
			wszStr+=2;
			while( ch = *wszStr++)
			{
				if(ch >= L'0' && ch <=L'9' )
					ch-=L'0';
				else if( ch >= L'A' && ch<=L'F' )
					ch=ch-L'A' + 10;
				else if( ch >= L'a' && ch<=L'f')
					ch=ch-L'a'+10;
				else
					return 0;	//invalid string

				if (FAILED (UIntMult(retVal, 16, &retVal)))
					return 0;
				if (FAILED (UIntAdd(retVal, (UINT)ch, &retVal)))
					return 0;
			}
		}
		// Otherwise its an int
		else
		{
			while( ch = *wszStr++)
			{
				if(ch >= L'0' && ch <=L'9' )
					ch-=L'0';
				else
					return 0;	//invalid string
					
				if (FAILED (UIntMult(retVal, 10, &retVal)))
					return 0;
				if (FAILED (UIntAdd(retVal, (UINT)ch, &retVal)))
					return 0;
			}
		}
	}

	Assert (INT_MAX > retVal);
    return ( fNegative ? -((int)retVal) : (int)retVal );
}

DynamicQueue::DynamicQueue()
{
	m_pHead = NULL;
	m_ppTail = &m_pHead;
	m_pFree = NULL;
}

DynamicQueue::~DynamicQueue()
{
	Assert( !m_pHead );

	QueueItem *pTmp;
	while( m_pFree )
	{
		pTmp = m_pFree;
		m_pFree = m_pFree->pNext;
		delete pTmp;
	}
}

DWORD DynamicQueue::EnQueue(HANDLE Key)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T("Key: %p{HANDLE}\n"), Key );
	
	QueueItem *pNew;
	if(m_pFree)
	{
		pNew = m_pFree;
		m_pFree = m_pFree->pNext;
	}
	else
	{
		pNew = NewNoX(gpmo) QueueItem;
		if( pNew == NULL )
		{
			SNI_SET_LAST_ERROR( INVALID_PROV, SNIE_4, ERROR_OUTOFMEMORY );

			BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_OUTOFMEMORY);
			
			return ERROR_OUTOFMEMORY;
		}
	}

	pNew->pNext = NULL;
	pNew->Key = Key;
	
	*m_ppTail = pNew;
	m_ppTail=&pNew->pNext;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d{WINERR}\n"), ERROR_SUCCESS);
			
	return ERROR_SUCCESS;
}

HANDLE DynamicQueue::DeQueue()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	HANDLE Key;

	Assert(m_pHead);

	Key = m_pHead->Key;

	QueueItem *tmp;
	tmp= m_pHead;
	
	m_pHead=m_pHead->pNext;

	if(!m_pHead)
	{
		m_ppTail=&m_pHead;
	}
	
	tmp->pNext = m_pFree;
	m_pFree = tmp;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Key: %p\n"), Key);
	
	return Key;
}

HANDLE DynamicQueue::Peek()
{
	BidxScopeAutoSNI0( SNIAPI_TAG _T("\n") );
	
	HANDLE Key;

	Assert(m_pHead);

	Key = m_pHead->Key;

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Key: %p\n"), Key);
	
	return Key;
}
	

