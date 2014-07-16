#pragma once

//#include "Heap/heap_debug.h"
//#include "MemAlloc.h"

#define ONE_FRAMESIZE		(100*1000)
#define	MAX_FRAMEBUFFER_SIZE (1920 * 1080 * 3 / ONE_FRAMESIZE + 1) * ONE_FRAMESIZE // uncompressed RGB24 1920x1080
#define	MAX_BUFFER_SIZE		12


typedef struct _tagFILEDATABuffer
{
	ULONG ulTimeScale;
	LONGLONG llTimeStamp;
	ULONG ulLength;
	BYTE * pData;
	ULONG ulBufSize;

	_tagFILEDATABuffer(): pData(NULL), ulBufSize(0), ulLength(0)
	{

	}
	~_tagFILEDATABuffer()
	{
		Clear();
	}
	void Init( )
	{
		Clear();
		ulTimeScale = 0;
		llTimeStamp = 0;
		ulLength = 0;
		ulBufSize = ONE_FRAMESIZE;
		pData = (unsigned char*)malloc( ulBufSize );
		memset( pData, 0 , ulBufSize );
	}
	void Clear()
	{
		if(pData)
		{
			free( pData );
			pData = NULL;
		}
		ulBufSize=0;
		ulLength = 0;
	}
	BOOL CopyData(BYTE* pSrcData, ULONG nSrcDataLen )
	{
		if(pSrcData == NULL || nSrcDataLen==0)
			return FALSE; // Invalid parameters. 

		if( pData == NULL ) //the file data buffer was not initialized before, 
		{
			return FALSE;
		}

		if( ulBufSize < nSrcDataLen)
		{
			Clear( );

			ulBufSize= (nSrcDataLen/ONE_FRAMESIZE +1 ) * ONE_FRAMESIZE;
			if( ulBufSize <= MAX_FRAMEBUFFER_SIZE )
			{
				pData = (unsigned char*)malloc( ulBufSize );
				memset( pData, 0, ulBufSize );
				if( pData == NULL )
				{
					ulBufSize = 0;
					return FALSE; 
				}
			}
			else
			{	
				ulBufSize = 0;
				return FALSE;
			}
		}
		ulLength = nSrcDataLen;
		memcpy( pData, pSrcData, nSrcDataLen );			
		return TRUE;
	}

	_tagFILEDATABuffer & operator =(_tagFILEDATABuffer &T)
	{
		this->ulTimeScale = T.ulTimeScale;
		this->llTimeStamp = T.llTimeStamp;
		this->CopyData(T.pData, T.ulLength);
		return *this;
	}
}FILEDATABuffer;

class CReaderBuffer
{
public:
	CReaderBuffer( ULONG ulSize = MAX_BUFFER_SIZE );
	virtual ~CReaderBuffer(void);

	int			Write(int nTimeScale, LONGLONG llTimeStamp, unsigned char *pData, ULONG ulDataLen );	
	FILEDATABuffer*	Read();
	FILEDATABuffer*	Show();
//	bool		Reset();
	double		GetFullness();

protected:
	std::list<FILEDATABuffer *> m_lFreePool;
	std::list<FILEDATABuffer *> m_lTakenPool;
	CRITICAL_SECTION	m_csBuffer;	
	double				m_dBufferFullness;
	ULONG				m_ulBufferSize;
	ULONG				m_ulWritePos;
	ULONG				m_ulWriteRepeat;
	ULONG				m_ulReadPos;
	ULONG				m_ulReadRepeat;
	ULONG				m_ulBufferUsed;
};
