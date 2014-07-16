#pragma once

#include <process.h>
//#include "C3GPFileReader.h"
#if defined( ENABLE_PLAYLIST_DEBUG_LOG )	//[jerry:2009-12-15]
#	define DMSG_3GP_CLOCK_ON
#endif 

class C3GPFileReaderClock
{
public:
	C3GPFileReaderClock(void);
	virtual ~C3GPFileReaderClock(void);

	bool	WaitingForTimeStamp(LONGLONG llTimeStamp, int nTimeScale);
	bool	WaitingForTimeStampAsync(LONGLONG llTimeStamp, int nTimeScale);
	void	StartClock();
	void	SetStartPosition(LONGLONG llStartPosition);

protected:
	//static unsigned int __stdcall TimerThread(void *pObj);
	static DWORD WINAPI TimerThread(LPVOID pObj); 
	void Timer();

private:
	LONGLONG			m_llElapsedTime;	
	LONGLONG			m_llStartPosition;
	LONGLONG			m_llOffsetTime;
	DWORD				m_dwStartTick;
	DWORD				m_dwPreviousTick;
	DWORD				m_dwCurrentTick;	
	HANDLE				m_hTimer;
	HANDLE				m_hTimerEvent;
	HANDLE				m_hThread; //[jerry:081118] thread handle
	CRITICAL_SECTION    m_csLock;
	//BOOL				m_bStarted;
	//BOOL				m_bFirst;	
	//LONGLONG			m_ulStartTime;
#if defined( DMSG_3GP_CLOCK_ON )
public:
	CViDebug * m_pDebug;
	void SetDebug( CViDebug * pDebug ){ m_pDebug = pDebug; }
#endif 
};

#if defined( DMSG_3GP_CLOCK_ON )
#	define D3GP_CLOCK_FNC(X)	if( m_pDebug ) m_pDebug->Message2 X
#else
#	define D3GP_CLOCK_FNC(X)
#endif 
