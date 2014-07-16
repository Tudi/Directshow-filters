#ifndef _VIDIATOR_ISO_FORMAT_DEMUX_PUSHER_PIN_H_
#define _VIDIATOR_ISO_FORMAT_DEMUX_PUSHER_PIN_H_

#define HACK_DELAY_SEND_TO_GUESS_TIME	1
#define UNDEFINED_CLOCK_VALUE ( _I64_MAX / 2 )
#define DEFAULT_THREAD_SLEEP_TIME	1	//avoid very high CPU usage on idle

class CVidiatorISOFormatDemuxFilter;

#if UPDATED_TO_BE_COMPATIBLE_WITH_SEEK_AND_REST
class ISOFormatPushPin: public CBaseOutputPin
{
private:
	friend class CVidiatorISOFormatDemuxFilter;
	CVidiatorISOFormatDemuxFilter	*m_pOwner;
public:
	ISOFormatPushPin( TCHAR *pObjectName, HRESULT *pHr, CVidiatorISOFormatDemuxFilter *pFilter, LPCWSTR pPinName );
	HRESULT DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
	//Does not refresh allocator. Only sets internal data that will be used on next connect allocator negociation
	void	SetAllocatorProps( int BuffCount, int BuffSize )
	{
		m_nBufferNum = BuffCount;
		if( m_nBufferNum < 1 )
			m_nBufferNum = 1;
		m_nBufferSize = BuffSize;
		if( m_nBufferSize < 1 )
			m_nBufferSize = 1;
	}
	HRESULT Deliver(IMediaSample *);
	HRESULT SetMediaType(const CMediaType *);
	HRESULT GetMediaType(int iPosition, __inout CMediaType *pMediaType);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
private:
	CMediaType	m_MediaType;
	CCritSec	m_Lock;
	int			m_nBufferNum;
	int			m_nBufferSize;
	BOOL		m_bDiscontinuity;
	BOOL		m_bEndOfStream;
};
#endif

#define QUEUE_POLL_UPDATE_SLEEP			1
#define DEFAULT_THREAD_WAIT_SLEEP_MS	4			//in some cases not sleeping threads might lead to very high CPU usage
#define DEFAULT_PACKET_QUEUE_SIZE		20			//8 packets max in the queue. Please let it be at least 2
#define DEFAULT_PACKET_BUFFER_SIZZE		(1*1024)	//very random number. Too large for audio and too small for video

class ISOFormatPushStreamPin: public IMediaSeeking, public CSourceStream
{
	friend class CVidiatorISOFormatDemuxFilter;
	CVidiatorISOFormatDemuxFilter	*m_pOwner;
public:
	ISOFormatPushStreamPin( TCHAR * pObjectName, HRESULT * pHr, CVidiatorISOFormatDemuxFilter *pFilter, LPCWSTR pPinName );
	virtual		~ISOFormatPushStreamPin();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void** ppv);
	HRESULT		QueueDataToPush( unsigned char *pdata, int nDataLen, LONGLONG nTimeStamp, LONGLONG nTimeDisplay );
	BOOL		IsQueueFull() 
	{ 
//		return ( (int)m_lPacketQueue.size() > m_nBufferNum - 1 );
		return ( (int)m_lPacketQueue.size() + 2 > m_nBufferNum );
	}
	void	SetAllocatorProps( int BuffCount, int BuffSize )
	{
		m_nBufferNum = BuffCount;
		if( m_nBufferNum < 1 )
			m_nBufferNum = 1;
		m_nBufferSize = BuffSize;
		if( m_nBufferSize < 1 )
			m_nBufferSize = 1;
	}
	STDMETHODIMP	Stop();
	HRESULT			FlushQueue();
	HRESULT			SetDiscontinuity(BOOL bNewVal) 
	{ 
		m_bDiscontinuity = bNewVal; 
		return S_OK;
	}
#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
	void			SetTrackID( int ID, int PinType )
	{
		m_nTrackID = ID;
		m_nPinType = PinType;
	}
#endif
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

	HRESULT			SetMediaTypeInternal( CMediaType *pMediaType );	
// IMediaSeeking
public:
    STDMETHODIMP GetCapabilities(DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities(DWORD * pCapabilities );
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                   LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP SetPositions(LONGLONG * pCurrent, DWORD dwCurrentFlags
                              , LONGLONG * pStop, DWORD dwStopFlags );
    STDMETHODIMP GetPositions(LONGLONG * pCurrent,
                              LONGLONG * pStop );
    STDMETHODIMP GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG * pllPreroll);
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

private:
	HRESULT			QueueDataPop( IMediaSample **pSample );
protected:	
	//queueing is not a must. But if our source does not have a constant processing speed then it is highly recommended
	std::list< IMediaSample* > m_lPacketQueue;
    IMemAllocator *m_pPushAllocator;
	CCritSec		m_csQueue;
	CMediaType		m_MediaType;

	int				m_nBufferNum;
	int				m_nBufferSize;	

	BOOL			m_bDiscontinuity;
	BOOL			m_bEndOfStream;
	IReferenceClock *m_pRefClock;
	
	int				m_nQualityControlSleep;
#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
	int				m_nTrackID;	//used to signal back to reader
	int				m_nPinType;	//audio or video
#endif
#ifdef SLEEP_PUSHER_PIN_UNTIL_SYSTEM_TIMESTAMP
	LONGLONG		m_nSystemClockStart;
	LONGLONG		m_nStreamClockStart;
#endif
};

#endif