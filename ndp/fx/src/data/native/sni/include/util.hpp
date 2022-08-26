//*********************************************************************
//		Copyright (c) Microsoft Corporation.
//
// @File: util.hpp
// @Owner: petergv, nantu
// @Test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose: 
//		Header for common types and structures 
//
// Notes:
//	
// @EndHeader@
//****************************************************************************	

#ifndef _UTIL_HPP
#define _UTIL_HPP


class DynamicQueue
{
	class QueueItem {
	public:
		QueueItem *pNext;
		HANDLE Key;		
	};

	QueueItem  *	m_pHead;
	QueueItem **	m_ppTail;
	QueueItem  *	m_pFree;

public:

	DynamicQueue();

	~DynamicQueue();
	
	DWORD EnQueue(HANDLE Key);

	HANDLE DeQueue();

	HANDLE Peek();
	
	inline bool IsEmpty()
	{
		return ( m_pHead==0 );
	}
};

//*************************** CAutoSNICritSec **************
//
//	Note: similar functionality to SOS' SpinlockHolder<> template 
//	(see spinlock.inl and spinlock.h).  
//
#define SNI_AUTOCS_ENTER		TRUE
#define SNI_AUTOCS_DO_NOT_ENTER	FALSE

class CAutoSNICritSec
{
private:
	SNICritSec& m_cs;
	BOOL		m_fCritSecHeld; 
	
public: 	// @access public
	inline CAutoSNICritSec(SNICritSec * cs, BOOL fEnter) ;
	inline ~CAutoSNICritSec();
	inline void Enter();
	inline bool TryEnter();
	inline void Leave(); 
};

inline CAutoSNICritSec::CAutoSNICritSec(SNICritSec * cs, BOOL fEnter) : 
	m_fCritSecHeld(FALSE), 
	m_cs(*cs)
{
	if( fEnter )
	{
		m_cs.Enter();
		m_fCritSecHeld = TRUE; 
	}
}

inline CAutoSNICritSec::~CAutoSNICritSec()
{
	if( m_fCritSecHeld )
	{
		m_cs.Leave();
	}
}

inline void CAutoSNICritSec::Enter()
{
	m_cs.Enter();
	m_fCritSecHeld = TRUE; 
}

inline bool CAutoSNICritSec::TryEnter()
{
	bool fEntered = m_cs.TryEnter();

	if( fEntered )
	{
		m_fCritSecHeld = TRUE; 
	}

	return fEntered; 
}

inline void CAutoSNICritSec::Leave()
{
	#ifndef SNI_BASED_CLIENT
	FAILPOINT_HDR(PROTOCOLS, SNICRITSEC_LEAVE);
	#endif

	Assert( m_fCritSecHeld ); 

	m_cs.Leave();
	m_fCritSecHeld = FALSE; 
}	
#endif
