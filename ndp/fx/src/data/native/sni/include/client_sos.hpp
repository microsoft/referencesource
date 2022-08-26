//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: client_sos.hpp
// @Owner: petergv, nantu
// @test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose:
//     This file will define some of sos classes on the client side to enable us
// keep the sources in sync.
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

#ifdef _CLIENT_SOS_HPP_
#error this file should be included only by client side precompiled header
#else
#define _CLIENT_SOS_HPP_

// SOS Result type (unsigned, 32-bit)
//
typedef DWORD		SOS_RESULT;

// Enumerates SOS errors
//
enum SOS_ERROR_CODES
{
	SOS_OK   = 0
};

enum EVENT_STATE
{
	EVENT_NOTSIGNALED = 0,
	EVENT_SIGNALED
}; 


typedef UINT PWAITTYPE;

enum PWAIT_enum
{
	PWAIT_VIA_ACCEPT_DONE = 0x40012f
};

class SOS_WaitInfo
{
public:
	SOS_WaitInfo (PWAITTYPE WaitType) {;}
};

class SOS_EventAuto
{

	HANDLE m_Event;
	
public:

	inline SOS_EventAuto (__in const EVENT_STATE state = EVENT_NOTSIGNALED)
	{
		m_Event = CreateEvent(
						NULL,
						FALSE,
  						state == EVENT_SIGNALED ? TRUE : FALSE,
						NULL);

		Assert( m_Event );
	}
	
	~SOS_EventAuto()
	{
		if( NULL != m_Event )
		{
			CloseHandle( m_Event );
		}
	}

	// Wait on the event
	//
	SOS_RESULT Wait (
					__in const DWORD 			cmsecTimeout, 
					__in const SOS_WaitInfo* 	pwaitInfo)
	{

		return WaitForSingleObject( 
						m_Event,
  						cmsecTimeout);
	}


	void
	SOS_EventAuto::Signal ()
	{
		BOOL fReturn;
		
		fReturn = SetEvent( m_Event );

		Assert( fReturn );
	}
};

#define MAX_NODES 1

typedef DWORD_PTR 	Affinity;
typedef LONG_PTR 	Counter;


enum TaskEnqueueOptions
{
	PermanentTask		= 0x1,
	PreemptiveTask 	= 0x2
};

class SOS_Task
{
public:

	static SOS_Task s_ClientTask;
	
	typedef LPTHREAD_START_ROUTINE Func;

	void Release()
	{
	}

};

extern "C"	DWORD SNICreateWaitThread(LPTHREAD_START_ROUTINE, PVOID pParam);

class SOS_Node
{
	static SOS_Node s_ClientCurrentNode;
	
public:

	BOOL IsDACNode() const
	{
		return FALSE;
	}

	BOOL IsOffline() const
	{
		return FALSE;
	}
	Counter GetNodeId()
	{
		return 0;
	}

	SOS_RESULT 
	SOS_Node::EnqueueTaskDirect ( 	
		__in SOS_Task::Func	pfunc,				// I  Pointer to task entry point
		__in_opt LPVOID	param,				// I  Parameters to task entry point
		__in DWORD exec_flags,			// I  Execution flags
		__out SOS_Task**	ppTask)				// O  Returns created task
	{

		Assert( exec_flags == PermanentTask );

		return SNICreateWaitThread( pfunc, param);
	}
	
	static SOS_Node * GetCurrent ()
	{
		return &s_ClientCurrentNode;
	}

};

class SOS_NodeEnum
{

public:

	inline SOS_NodeEnum ()
	{
	}

	inline SOS_Node*	GetNext (SOS_Node* pNode)
	{
		if( pNode )
		{
			Assert( pNode == SOS_Node::GetCurrent() );
			return NULL;
		}
		else
		{
			return SOS_Node::GetCurrent();
		}
	}
};

#endif
