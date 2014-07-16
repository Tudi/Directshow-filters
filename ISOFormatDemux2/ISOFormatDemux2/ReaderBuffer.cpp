#include "StdAfx.h"
#include "ReaderBuffer.h"

CReaderBuffer::CReaderBuffer( ULONG ulSize)
: m_ulBufferSize(ulSize)
, m_dBufferFullness(0.0)
, m_ulWritePos(0)
, m_ulWriteRepeat(0)
, m_ulReadPos(0)
, m_ulReadRepeat(0)
, m_ulBufferUsed(0)

{
	::InitializeCriticalSection(&m_csBuffer);

	if(m_ulBufferSize > MAX_BUFFER_SIZE)
		m_ulBufferSize = MAX_BUFFER_SIZE; 

	for(ULONG No = 0 ; No < m_ulBufferSize ; ++No )
	{
		FILEDATABuffer *tbuff = new FILEDATABuffer;
		tbuff->Init();
		m_lFreePool.push_front( tbuff );
	}
}

CReaderBuffer::~CReaderBuffer(void)
{
	while( m_lFreePool.empty() == false )
	{
		FILEDATABuffer *t = *( m_lFreePool.begin() );
		m_lFreePool.pop_front();
		delete t;
	}
	while( m_lTakenPool.empty() == false )
	{
		FILEDATABuffer *t = *( m_lTakenPool.begin() );
		m_lTakenPool.pop_front();
		delete t;
	}
	::DeleteCriticalSection(&m_csBuffer);
}

int CReaderBuffer::Write(int nTimeScale, LONGLONG llTimeStamp, unsigned char *pData, ULONG ulDataLen )
{
	if(!pData || (ulDataLen <= 0))
		return 0;

	int nRet = 0;
	::EnterCriticalSection(&m_csBuffer);
	
	// check current position
	if( m_ulBufferUsed >= m_ulBufferSize || m_ulWritePos >= m_ulBufferSize 
		|| m_lFreePool.empty() == true ) // unexpected behavior
	{
		::LeaveCriticalSection(&m_csBuffer);
		return -1;
	}	

	// write data
	FILEDATABuffer *pFileData = *(m_lFreePool.begin());
	m_lFreePool.pop_front();
	pFileData->ulTimeScale = nTimeScale;
	pFileData->llTimeStamp = llTimeStamp;
	pFileData->ulLength = ulDataLen;

	if (pFileData->CopyData(pData, ulDataLen ) == TRUE )
	{
		++m_ulWritePos;
		if( m_ulWritePos >= m_ulBufferSize) //rotation. 
		{
			++m_ulWriteRepeat;
			m_ulWritePos = 0;
		}			
		++m_ulBufferUsed;
		nRet = 1;
	}
	else
	{
		pFileData->ulLength = 0;
		nRet = -1;
	}	
	m_lTakenPool.push_back( pFileData );

	::LeaveCriticalSection(&m_csBuffer);

	return nRet;
}

FILEDATABuffer * CReaderBuffer::Read()
{	
	FILEDATABuffer * pFileData = NULL;

	::EnterCriticalSection(&m_csBuffer);
	// check current position
	if( m_ulBufferUsed <= 0 || m_ulReadPos >= m_ulBufferSize 
		|| m_lTakenPool.empty() == true
		) // unexpected behavior
	{
		::LeaveCriticalSection(&m_csBuffer);
		return NULL;
	}
	
	// read data
	pFileData = *( m_lTakenPool.begin() );
	m_lTakenPool.pop_front();
	m_lFreePool.push_back( pFileData );
	++m_ulReadPos;
	if(m_ulReadPos >= m_ulBufferSize) //rotation. 
	{
		++m_ulReadRepeat;
		m_ulReadPos = 0;
	}		
	--m_ulBufferUsed;	

	::LeaveCriticalSection(&m_csBuffer);
	return pFileData;
}

FILEDATABuffer * CReaderBuffer::Show()
{	
	FILEDATABuffer *pFileData = NULL;

	::EnterCriticalSection(&m_csBuffer);
	// check current position
	if( m_ulBufferUsed <= 0 || m_ulReadPos >= m_ulBufferSize 
		|| m_lTakenPool.empty() == true
		) // unexpected behavior
	{
		::LeaveCriticalSection(&m_csBuffer);
		return NULL;
	}
	// read data
	pFileData = *( m_lTakenPool.begin() );

	::LeaveCriticalSection(&m_csBuffer);
	return pFileData;
}
/*
bool CReaderBuffer::Reset()
{
	::EnterCriticalSection(&m_csBuffer);
	
	m_ulWritePos = 0;
	m_ulWriteRepeat = 0;
	m_ulReadPos = 0;
	m_ulReadRepeat = 0;
	m_ulBufferUsed = 0;

	::LeaveCriticalSection(&m_csBuffer);

	return true;
} */

double CReaderBuffer::GetFullness()
{
	double Percent;
	::EnterCriticalSection(&m_csBuffer);
	Percent = (double)m_ulBufferUsed * 100 / (double)m_ulBufferSize;
	::LeaveCriticalSection(&m_csBuffer);
	return Percent;
}