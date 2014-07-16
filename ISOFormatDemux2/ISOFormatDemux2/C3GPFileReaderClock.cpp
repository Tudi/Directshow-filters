#include "StdAfx.h"
#include "C3GPFileReaderClock.h"
#include <crtdbg.h>

LONGLONG ConvertMsecTime(unsigned int nTimeScale, DWORD dwTime)
{
	return (LONGLONG)((double)dwTime * 1000.0 / nTimeScale);
}

//unsigned int __stdcall C3GPFileReaderClock::TimerThread(void *pObj)
DWORD WINAPI C3GPFileReaderClock::TimerThread(LPVOID pObj)
{
	C3GPFileReaderClock *pClock = reinterpret_cast<C3GPFileReaderClock*>(pObj);
	if(!pClock)
		return 0;

	while(::WaitForSingleObject(pClock->m_hTimer, 10) == WAIT_TIMEOUT)
	{
		pClock->Timer();
	}

	return 0;
}

C3GPFileReaderClock::C3GPFileReaderClock()
//: m_ulStartTime(0)
//, m_ulElapsedTime(0)
//, m_llStartPosition(0)
:m_llElapsedTime(0)
, m_llStartPosition(0)
, m_hThread(NULL)
, m_llOffsetTime(0)
, m_dwStartTick(0)
, m_dwCurrentTick(0)
, m_dwPreviousTick(0)
{
#if defined( DMSG_3GP_CLOCK_ON )
	m_pDebug = NULL;
#endif 
	m_hTimer = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	InitializeCriticalSection( &m_csLock );
}

C3GPFileReaderClock::~C3GPFileReaderClock(void)
{
	// Finish timer thread
	::SetEvent(m_hTimer);
	//[jerry:081118]
	//Sleep(10);	
	if( m_hThread )
	{
		if( ::WaitForSingleObject( m_hThread, 10000 ) != WAIT_OBJECT_0 )
		{				
			::TerminateThread( m_hThread, 0 );
		}
		::CloseHandle( m_hThread );
		m_hThread = NULL;
	}
	//end [jerry]
	::CloseHandle(m_hTimer);

	DeleteCriticalSection( &m_csLock );
}

bool C3GPFileReaderClock::WaitingForTimeStampAsync(LONGLONG llTimeStamp, int nTimeScale)
{
	LONGLONG CurrentTime;

	EnterCriticalSection( &m_csLock );
	CurrentTime = m_llElapsedTime;
	LeaveCriticalSection( &m_csLock );

	//while(ConvertMsecTime(nTimeScale, llTimeStamp) > m_ulElapsedTime)
	if(ConvertMsecTime(nTimeScale, llTimeStamp) > CurrentTime)
	{
		return false;
	}
	return true;
}

bool C3GPFileReaderClock::WaitingForTimeStamp(LONGLONG llTimeStamp, int nTimeScale)
{
	LONGLONG CurrentTime = 0;
	while( 1 )
	{
		EnterCriticalSection( &m_csLock );
		CurrentTime = m_llElapsedTime;
		LeaveCriticalSection(&m_csLock);

		if( ConvertMsecTime(nTimeScale, llTimeStamp) > CurrentTime )
		{
		Sleep(5);
		}	
		else
		{
			break;
		}
	}
	//while(ConvertMsecTime(nTimeScale, llTimeStamp) > m_ulElapsedTime)
	//	Sleep(5);

	return true;
}


void C3GPFileReaderClock::StartClock()
{
	// Stop the timer if it is running
	::SetEvent(m_hTimer);
	//[jerry:081118] Terminate Thread	
	if( m_hThread )
	{		
		if( ::WaitForSingleObject( m_hThread, 10000 ) == WAIT_TIMEOUT )
		{	
			::TerminateThread( m_hThread, 0 );
		}
		::CloseHandle( m_hThread );
		m_hThread = NULL;
	}
	//::Sleep(10);
	//end [jerry]

	m_llOffsetTime = 0; //[jerry:2010-01-08]	

	//unsigned int threadid = ::_beginthreadex(NULL, 0, TimerThread, this, CREATE_SUSPENDED, &threadid);
	//unsigned int threadid;
	//m_hThread = (HANDLE)_beginthreadex(NULL, 0, TimerThread, this, CREATE_SUSPENDED, &threadid); //[jerry:081118]	
	DWORD threadid;
	m_hThread = ::CreateThread(NULL, 65536, TimerThread, this, CREATE_SUSPENDED|STACK_SIZE_PARAM_IS_A_RESERVATION, &threadid);
	if(m_hThread != NULL)
	{
		// Set starting time
		//m_ulStartTime = ::GetTickCount();
		m_dwStartTick = ::GetTickCount();
		m_dwPreviousTick = m_dwStartTick;

		if(m_llStartPosition > 0)
			m_llOffsetTime += m_llStartPosition;

		// Start timer
		::ResetEvent(m_hTimer);
		::ResumeThread((HANDLE)m_hThread);//[jerry:081118]
	}
}

void C3GPFileReaderClock::SetStartPosition(LONGLONG llStartPosition)
{
	_ASSERT(llStartPosition >= 0);
	if(llStartPosition < 0)
		return;

	m_llStartPosition = llStartPosition;
}

void C3GPFileReaderClock::Timer()
{
	// Measure elapsed time from the starting
	m_dwCurrentTick = ::GetTickCount();
	if( m_dwCurrentTick < m_dwPreviousTick ) // roll-over
	{
		DWORD Offset = ((DWORD)0xFFFFFFFF) - m_dwStartTick;

		D3GP_CLOCK_FNC( (TEXT("[3GPR:Timer] Timer - Rollover!! Prev(%u),Cur(%u), Start(%u), Offset(%u), CurOffsetTime(%I64d)"), 
			m_dwPreviousTick, m_dwCurrentTick, m_dwStartTick, Offset, m_llOffsetTime ) );

		m_llOffsetTime += Offset;
		m_llOffsetTime += m_dwCurrentTick;
		m_dwStartTick = m_dwCurrentTick;
	}
	/*else Log Test Code. [jerry:2010-01-13]
	{
		DWORD Offset = ((DWORD)0xFFFFFFFF) - m_dwStartTick;
		//[test for it works or not]
		D3GP_CLOCK_FNC( (TEXT("[3GPR:Timer] Timer - Rollover!! Prev(%u),Cur(%u), Start(%u), Offset(%u), CurOffsetTime(%I64d)"), 
			m_dwPreviousTick, m_dwCurrentTick, m_dwStartTick, Offset, m_llOffsetTime ) );
	}*/

	EnterCriticalSection( &m_csLock );
	m_llElapsedTime = m_llOffsetTime + (m_dwCurrentTick - m_dwStartTick);
	LeaveCriticalSection( &m_csLock );

	m_dwPreviousTick = m_dwCurrentTick;
	
// 	LONGLONG CurrentTick = ::GetTickCount();
// 	if( CurrentTick < m_ulStartTime )
// 	{
// 		LONGLONG Offset = MAX32U - m_ulStartTime;
// 		m_llOffsetTime += Offset;
// 		m_ulStartTime = 0;
// 	}
// 	EnterCriticalSection( &m_csLock );
// 	m_ulElapsedTime = m_llOffsetTime + (CurrentTick - m_ulStartTime);
// 	LeaveCriticalSection( &m_csLock );
}