#pragma once

//need to swap this to events later, or remove it completly
// ! Needs to be large enough so sound packets will not block reading video packets. Need to be able to store N sound packets while we get to read 1 video packet
//#define QUEUE_BLOCK_AFTER_SIZE			0
#define QUEUE_POLL_UPDATE_SLEEP			1
#define DEFAULT_THREAD_WAIT_SLEEP_MS	4	//in some cases not sleeping threads might lead to very high CPU usage
#define DEFAULT_VIDEO_BUFFER_COUNT		8
#define DEFAULT_AUDIO_BUFFER_COUNT		16
#define HACK_DELAY_SEND_TO_GUESS_TIME	1

//#define ENABLE_PIN_DEBUG_LOGS
#ifdef ENABLE_PIN_DEBUG_LOGS
	//called rarely to report non expected states
	#define d1Log(a) if( m_fdLogs ) fprintf( m_fdLogs, a );
	#define d1Log1(a,b) if( m_fdLogs ) fprintf( m_fdLogs, a, b );
	#define d1Log2(a,b,c) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c );
	#define d1Log3(a,b,c,d) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d );
	#define d1Log4(a,b,c,d,e) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e );
	//these might create lag
	#define d4Log(a) if( m_fdLogs ) fprintf( m_fdLogs, a );
	#define d4Log1(a,b) if( m_fdLogs ) fprintf( m_fdLogs, a, b );
	#define d4Log2(a,b,c) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c );
	#define d4Log3(a,b,c,d) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d );
	#define d4Log4(a,b,c,d,e) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e );
#else
	//called rarely to report non expected states
	#define d1Log(a) 
	#define d1Log1(a,b)
	#define d1Log2(a,b,c)
	#define d1Log3(a,b,c,d)
	#define d1Log4(a,b,c,d,e)
	//these might create lag
	#define d4Log(a)
	#define d4Log1(a,b)
	#define d4Log2(a,b,c)
	#define d4Log3(a,b,c,d)
	#define d4Log4(a,b,c,d,e)
#endif

//This is not finished
//when it will be finished it should be a full implemented async data pushed with data queueing
class CVidiatorWMSourceReaderFilter;

class PusherStreamPin: public CSourceStream
{
	friend class CVidiatorWMSourceReaderFilter;
public:
	PusherStreamPin( TCHAR * pObjectName, HRESULT * pHr, CVidiatorWMSourceReaderFilter * pFilter, LPCWSTR pPinName, GUID MajorType );
	virtual ~PusherStreamPin();
protected:

	///////////////////////////////////////////////////
	// CBasePin
	STDMETHODIMP	SetSink(IQualityControl *piqc);
	STDMETHODIMP	Notify(IBaseFilter *pSender, Quality q);

	///////////////////////
	// Derived from CBaseOuputPIn
	virtual HRESULT DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties);

	///////////////////////
	// Derived from CSourceStream
	virtual HRESULT GetMediaType( CMediaType *pMediaType );	
	virtual HRESULT DoBufferProcessingLoop(void);
	//polling queue until data is available to be pushed
	virtual HRESULT FillBuffer(IMediaSample* pSample);

	///////////////////////
	// Active & InActive
	HRESULT			Active(void);    // Starts up the worker thread
	HRESULT			Inactive(void);  // Exits the worker thread.

	//our WM reader specific buffer filler. It will block until the buffer is fetched from us
	HRESULT			QueueDataToPush( MEDIAPacket *pPacket );	

	HRESULT			SetMediaType( CMediaType *pMediaType );	
	HRESULT			SetMediaType( WM_MEDIA_TYPE *pMediaType );	
private:
	BOOL			FillSample( IMediaSample* pSample, MEDIAPacket* pPacket );
	HRESULT			QueueDataPop( IMediaSample **pSample );
protected:	
	//queueing is not a must. But if our source does not have a constant processing speed then it is highly recommended
	std::list< IMediaSample* > m_lPacketQueue;
	CCritSec		m_csQueue;
#ifdef QUEUE_BLOCK_AFTER_SIZE
	HANDLE			m_hQueuePushBlock;
#endif
	CMediaType		m_MediaType;
	int				m_nBufferNum;
	int				m_nBufferSize;	
	BOOL			m_bFirstPacket;
	BOOL			m_bEndOfStream;
	MTime			m_PrevStartTime;
	IReferenceClock *m_pRefClock;
	REFERENCE_TIME	m_RefClockStart;
	CTickTimer		m_TimerReportStream;
	
	MTime			m_ReportTime;
#ifdef ENABLE_PIN_DEBUG_LOGS
	FILE			*m_fdLogs;
#endif
};
