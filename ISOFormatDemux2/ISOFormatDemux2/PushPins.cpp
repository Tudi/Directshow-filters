#include "StdAfx.h"
#include "PushPins.h"

#if UPDATED_TO_BE_COMPATIBLE_WITH_SEEK_AND_REST
ISOFormatPushPin::ISOFormatPushPin(TCHAR *pObjectName, HRESULT *pHr, CVidiatorISOFormatDemuxFilter *pFilter, LPCWSTR pPinName):
CBaseOutputPin( pObjectName, pFilter, &m_Lock, pHr, pPinName )
{
	m_nBufferNum = 1;
	m_nBufferSize = 1;
	m_bDiscontinuity = TRUE;
	m_bEndOfStream = FALSE;
	m_pOwner =  pFilter;
}

HRESULT ISOFormatPushPin::DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{ 
	HRESULT hResult = NOERROR;

	CheckPointer(pMemAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	//1 second buffering
	if( pProperties->cBuffers < m_nBufferNum )
		pProperties->cBuffers = m_nBufferNum;

	if( pProperties->cbBuffer < m_nBufferSize )
		pProperties->cbBuffer = m_nBufferSize;

	ALLOCATOR_PROPERTIES Actual;
	if( FAILED(hResult = pMemAlloc->SetProperties(pProperties, &Actual)) )
	{
		return hResult;
	}

	if( Actual.cbBuffer < pProperties->cbBuffer ||
		Actual.cBuffers < pProperties->cBuffers ) 
		return E_FAIL;

	return NOERROR;
}

HRESULT ISOFormatPushPin::Deliver(IMediaSample *pSample)
{
	if( pSample && m_bDiscontinuity )
	{
		pSample->SetDiscontinuity( true );
		m_bDiscontinuity = FALSE;
	}
	return __super::Deliver( pSample );
}

HRESULT ISOFormatPushPin::SetMediaType(const CMediaType *pMediaType)
{
	CopyMediaType( &m_MediaType, pMediaType );
	return __super::SetMediaType( pMediaType );
}

HRESULT ISOFormatPushPin::GetMediaType(int iPosition, __inout CMediaType *pMediaType)
{
	if( iPosition == 0 && pMediaType != NULL )
	{
		CopyMediaType( pMediaType, &m_MediaType );
		return S_OK;
	}
	return E_UNEXPECTED;
}

HRESULT ISOFormatPushPin::CheckMediaType(const CMediaType *pMediaType)
{ 
	return S_OK; 
}
#endif

/////////////////////////////////////////////////////////////
// Stream Pin
/////////////////////////////////////////////////////////////


//disable debugging for now. Will implement later
#ifdef MPTSFILT_INF
	#undef MPTSFILT_INF
	#undef MPTSFILT_FNC
	#undef MPTSFILT_PACKET
	#define MPTSFILT_INF(x) 1
	#define MPTSFILT_FNC(x) 1
	#define MPTSFILT_PACKET(x) 1
#else
	#define MPTSFILT_INF(x) 1
	#define MPTSFILT_FNC(x) 1
	#define MPTSFILT_PACKET(x) 1
#endif

HRESULT CopySample(IMediaSample *pMSsrc, IMediaSample *pMSdst)
{
    BYTE* pDest;
    BYTE* pSrc;
    if( pMSdst->GetPointer(&pDest) != S_OK )
	{
		return S_FALSE;
	}
    if( pMSsrc->GetPointer(&pSrc) != S_OK )
	{
		return S_FALSE;
	}

    long cOut = pMSdst->GetSize();
    long cIn = pMSsrc->GetActualDataLength();
    if( pMSdst->SetActualDataLength( cIn ) != S_OK )
	{
		return VFW_E_BUFFER_OVERFLOW;
	}
    if( cOut < cIn )
	{
        return VFW_E_BUFFER_OVERFLOW;
	}
    
    CopyMemory( pDest, pSrc, cIn );

    REFERENCE_TIME tStart, tEnd;
	switch( pMSsrc->GetTime(&tStart,&tEnd) )
	{
		case S_OK: pMSdst->SetTime(&tStart,&tEnd); break;
		case VFW_S_NO_STOP_TIME: pMSdst->SetTime(&tStart,NULL); break;
		case VFW_E_SAMPLE_TIME_NOT_SET: pMSdst->SetTime(NULL,NULL);	break;
		default: 
			{
				return VFW_E_SAMPLE_TIME_NOT_SET;
			}
	}
	switch( pMSsrc->GetMediaTime(&tStart,&tEnd) )
	{
		case S_OK: pMSdst->SetMediaTime(&tStart,&tEnd); break;
		case VFW_E_MEDIA_TIME_NOT_SET: pMSdst->SetMediaTime(NULL,NULL); break;
		default: 
			{
				return VFW_E_SAMPLE_TIME_NOT_SET;
			}
	}

    if (pMSsrc->IsSyncPoint() == S_OK)
        pMSdst->SetSyncPoint(true);
	else
        pMSdst->SetSyncPoint(false);
    if (pMSsrc->IsDiscontinuity() == S_OK)
        pMSdst->SetDiscontinuity(true);
	else
        pMSdst->SetDiscontinuity(false);
    if (pMSsrc->IsPreroll() == S_OK)
        pMSdst->SetPreroll(true);
	else
        pMSdst->SetPreroll(false);

    AM_MEDIA_TYPE *pMediaType;
	HRESULT hr;
    hr = pMSsrc->GetMediaType( &pMediaType );
	if( hr == S_OK && pMediaType != NULL )
		hr = pMSdst->SetMediaType( pMediaType );
    DeleteMediaType(pMediaType);
	
	return S_OK;
}

ISOFormatPushStreamPin::ISOFormatPushStreamPin( TCHAR * pObjectName, HRESULT * pHr, CVidiatorISOFormatDemuxFilter * pFilter, LPCWSTR pPinName )
:CSourceStream( pObjectName, pHr, pFilter, pPinName )
,m_nBufferNum( DEFAULT_PACKET_QUEUE_SIZE )
,m_nBufferSize( DEFAULT_PACKET_BUFFER_SIZZE )
,m_bDiscontinuity( FALSE )
,m_pRefClock( NULL )
,m_bEndOfStream( FALSE )
,m_pPushAllocator(NULL)
{
	memset( &m_MediaType, 0, sizeof( m_MediaType ) );
	m_nQualityControlSleep = 0; //renderer will send us feedback if we are sending packets too fast
	m_pOwner =  pFilter;
#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
	m_nTrackID = 0;
	m_nPinType = 2; //invalid value
#endif
#ifdef SLEEP_PUSHER_PIN_UNTIL_SYSTEM_TIMESTAMP
	m_nSystemClockStart = UNDEFINED_CLOCK_VALUE;
	m_nStreamClockStart = UNDEFINED_CLOCK_VALUE;
#endif
}

ISOFormatPushStreamPin::~ISOFormatPushStreamPin( )
{
	FlushQueue();
    if (m_pPushAllocator)
	{
        m_pPushAllocator->Decommit();
        m_pPushAllocator->Release();
        m_pPushAllocator = NULL;
    }
}

STDMETHODIMP ISOFormatPushStreamPin::NonDelegatingQueryInterface(REFIID iid, void** ppv)
{
    if (iid == IID_IMediaSeeking)
    {
        return GetInterface((IMediaSeeking*)this, ppv);
    }
    else
    {
		return CSourceStream::NonDelegatingQueryInterface(iid, ppv);
    }
}

HRESULT	ISOFormatPushStreamPin::FlushQueue()
{
	m_csQueue.Lock();
	while( !m_lPacketQueue.empty() )
	{
		IMediaSample *LocalPacket = *(m_lPacketQueue.begin());
		LocalPacket->Release();
		m_lPacketQueue.pop_front();
	}
	m_csQueue.Unlock();
	return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::Notify(IBaseFilter *pSender, Quality q)
{
	//increase time between packets
	if( q.Proportion <= 0 )
		m_nQualityControlSleep = 1000; //1 packet per second
    else
    {
        m_nQualityControlSleep = m_nQualityControlSleep * 1000 / q.Proportion;
        if( m_nQualityControlSleep > 1000 )
            m_nQualityControlSleep = 1000;   
    }

    // skip forwards
//    if(q.Late > 0)
//        int t = q.Late;

	return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::SetSink(IQualityControl *piqc)
{
	return S_OK;
}

HRESULT ISOFormatPushStreamPin::DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	HRESULT hResult = NOERROR;

	CheckPointer(pMemAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	if( pProperties->cBuffers < m_nBufferNum )
		pProperties->cBuffers = m_nBufferNum;

	//did we load up file details yet ?
#if AUTO_GUESS_REQUIRED_BUFFER_SIZE
	if( m_MediaType.majortype != GUID_NULL && m_MediaType.pbFormat )
	{
		if( m_MediaType.formattype == FORMAT_MPEG2Video )
		{
			MPEG2VIDEOINFO *pVI = (MPEG2VIDEOINFO*)m_MediaType.pbFormat;
			m_nBufferSize = pVI->hdr.bmiHeader.biWidth * pVI->hdr.bmiHeader.biHeight * pVI->hdr.bmiHeader.biPlanes * pVI->hdr.bmiHeader.biBitCount / 8;
		}
		else if( m_MediaType.formattype == FORMAT_VideoInfo )
		{
			VIDEOINFOHEADER *pVI = (VIDEOINFOHEADER*)m_MediaType.pbFormat;
			m_nBufferSize = pVI->bmiHeader.biWidth * pVI->bmiHeader.biHeight * pVI->bmiHeader.biPlanes * pVI->bmiHeader.biBitCount / 8;
		}
		else if( m_MediaType.formattype == FORMAT_WaveFormatEx )
		{
			WAVEFORMATEX *pAI = (WAVEFORMATEX*)m_MediaType.pbFormat;
			m_nBufferSize = pAI->nChannels * pAI->nSamplesPerSec * pAI->wBitsPerSample / 8;
		}
	}
#endif

	if( pProperties->cbBuffer < m_nBufferSize )
		pProperties->cbBuffer = m_nBufferSize;

	ALLOCATOR_PROPERTIES Actual;
	if( FAILED(hResult = pMemAlloc->SetProperties(pProperties, &Actual)) )
	{
		return hResult;
	}

	if(Actual.cbBuffer < pProperties->cbBuffer || Actual.cBuffers < pProperties->cBuffers ) 
		return E_FAIL;

	// set exactly same push allocator
	if (m_pPushAllocator)
	{
		m_pPushAllocator->Decommit();
		m_pPushAllocator->Release();
		m_pPushAllocator = NULL;
	}

	if (FAILED(hResult = CreateMemoryAllocator(&m_pPushAllocator)))
	{
		return hResult;
	}

	if (FAILED(hResult = m_pPushAllocator->SetProperties(pProperties, &Actual)))
	{
		return hResult;
	}

	if (Actual.cbBuffer < pProperties->cbBuffer || Actual.cBuffers < pProperties->cBuffers)
	{
		return E_FAIL;
	}

	return NOERROR;
}

HRESULT ISOFormatPushStreamPin::DoBufferProcessingLoop(void) 
{
	Command com;
	OnThreadStartPlay();

	do 
	{
		while (!CheckRequest(&com)) 
		{
			HRESULT hr = S_OK;
			IMediaSample* pSample = NULL;
			if (SUCCEEDED(hr = m_pPushAllocator->GetBuffer(&pSample, 0, 0, 0)))
			{
				hr = QueueDataPop( &pSample );
			}

			if (hr == S_OK) 
			{
				LONGLONG start,end;
				pSample->GetTime( &start, &end );
#ifdef SLEEP_PUSHER_PIN_UNTIL_SYSTEM_TIMESTAMP
				//only required if we are missing reference clock
				LONGLONG DeliveryLatencyStart = 0;
				if( m_pRefClock == NULL )
				{
					DeliveryLatencyStart = GetTickCount();
//printf(" Missing reference clock, Will fetch system time\n");
				}
#endif

				hr = Deliver(pSample);

				pSample->Release();
				pSample = NULL;

#ifdef SLEEP_PUSHER_PIN_UNTIL_SYSTEM_TIMESTAMP
				if( m_nStreamClockStart == (LONGLONG)( _I64_MAX / 2 ) )
					m_nStreamClockStart = start;
				REFERENCE_TIME SystemTimePassed;
				REFERENCE_TIME StreamTimePassed = ( start - m_nStreamClockStart ) / 10000;
				if( m_pRefClock )
				{
					REFERENCE_TIME tSys;
					HRESULT thr = m_pRefClock->GetTime( &tSys );
					if( m_nSystemClockStart == UNDEFINED_CLOCK_VALUE )
						m_nSystemClockStart = tSys;
					SystemTimePassed = tSys - m_nSystemClockStart;
				}
				else
				{
					if( m_nSystemClockStart == UNDEFINED_CLOCK_VALUE )
						m_nSystemClockStart = GetTickCount();
					SystemTimePassed = GetTickCount() - m_nSystemClockStart;
				}
				//anti BUG protection. If packet duration is more then 10 seconds then there is a high chance something went wrong. That would mean 0.1 FPS rendering
				if( SystemTimePassed + 10000 < StreamTimePassed )
				{
//printf("Need to reinitialize %d %d\n", SystemTimePassed, StreamTimePassed );
					m_nSystemClockStart = UNDEFINED_CLOCK_VALUE;
					m_nStreamClockStart = UNDEFINED_CLOCK_VALUE;
					SystemTimePassed = StreamTimePassed;
				}
				//wait until we are in sync with system time
				if( SystemTimePassed + 2 * DEFAULT_THREAD_SLEEP_TIME < StreamTimePassed )
				{
//printf("Need to Sleep %d \n", StreamTimePassed - ( SystemTimePassed + 2 * DEFAULT_THREAD_SLEEP_TIME ) );
					Sleep( StreamTimePassed - ( SystemTimePassed + 2 * DEFAULT_THREAD_SLEEP_TIME ) );
				}
#endif
				if( m_nQualityControlSleep > 0 )
					Sleep( m_nQualityControlSleep );
				// downstream filter returns S_FALSE if it wants us to
				// stop or an error if it's reporting an error.
				if(hr != S_OK)
				{
					DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
					MPTSFILT_INF((_T("[SOURCE::DoBufferProcessingLoop] Name(%s), ODeliver() returned %08x; stopping"), m_pName, hr ));
					return S_OK;
				}

			} 
			else if (hr == S_FALSE) 
			{
				// derived class wants us to stop pushing data
				if( pSample )
					pSample->Release();
				if( m_bEndOfStream )
				{
					DeliverEndOfStream();
					MPTSFILT_INF((_T("[SOURCE::DoBufferProcessingLoop] Name(%s), Deliver End Of Stream"), m_pName));
					return S_OK;
				}				
				//Maybe we should wait until there is some buffer to be pushed ? Might lead to high CPU usage
				Sleep( DEFAULT_THREAD_WAIT_SLEEP_MS );
			} 
			else 
			{
				// derived class encountered an error
				if( pSample )
					pSample->Release();
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
				MPTSFILT_INF((_T("[SOURCE::DoBufferProcessingLoop] Name(%s), Error %08lX from FillBuffer!!!"), m_pName,hr));
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}
			Sleep( DEFAULT_THREAD_SLEEP_TIME );
			// all paths release the sample
		}
		// For all commands sent to us there must be a Reply call!
		if (com == CMD_RUN || com == CMD_PAUSE)
		{
			Reply(NOERROR);
		} 
		else if (com != CMD_STOP)
		{
			Reply((DWORD) E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
	} while (com != CMD_STOP);

	return S_FALSE;
}

HRESULT ISOFormatPushStreamPin::Active(void) 
{
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;
	MPTSFILT_FNC((_T("[SOURCE::Active]<+>")));

	if (m_pFilter->IsActive()) 
	{
		MPTSFILT_FNC((_T("[SOURCE::Active]<ERROR><->")));
		return S_FALSE;	// succeeded, but did not allocate resources (they already exist...)
	}

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) 
	{
		MPTSFILT_FNC((_T("[SOURCE::Active]<ERROR><->")));
		return NOERROR;
	}

	m_bDiscontinuity = TRUE;
//	m_PrevStartTime = TIME_Unknown;

	hr = CBaseOutputPin::Active();
	if (FAILED(hr)) 
	{
		MPTSFILT_FNC((_T("[SOURCE::Active]<ERROR><->")));
		return hr;
	}

	if (FAILED(hr = m_pPushAllocator->Commit()))
	{
		return hr;
	}

	ASSERT(!ThreadExists());
	
	//////////////////////
	// Get
	if( m_pRefClock )
	{ 
		m_pRefClock->Release(); 
		m_pRefClock = NULL;
	}	
	
	if( FAILED( m_pFilter->GetSyncSource( &m_pRefClock ) ) || m_pRefClock == NULL )
	{
		MPTSFILT_FNC((_T("[SOURCE::Active]<ERROR> Failed to get Reference clock. ")));		
	}
		
//	if( m_pRefClock )
//		m_pRefClock->GetTime( &m_nSystemClockStart );

	// start the thread
	if (!Create()) 
	{
		MPTSFILT_FNC((_T("[SOURCE::Active]<ERROR><->")));
		return E_FAIL;
	}

	// Tell thread to initialize. If OnThreadCreate Fails, so does this.
	hr = Init();
	if (FAILED(hr))
		return hr;

	MPTSFILT_FNC((_T("[SOURCE::Active]<->")));

	return Pause();
}

HRESULT ISOFormatPushStreamPin::Inactive(void) {

	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;
	MPTSFILT_FNC((_T("[SOURCE::InActive]<+>")));

	//flush our queue to avoid accidental resume and send of data
	FlushQueue();

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
/*	if( !IsConnected() )
	{
		MPTSFILT_FNC((_T("[SOURCE::InActive]<ERROR><->")));
		return NOERROR;
	}/**/

	// !!! need to do this before trying to stop the thread, because
	// we may be stuck waiting for our own allocator!!!

	hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	if (FAILED(hr) && hr != VFW_E_NO_ALLOCATOR )
	{
		MPTSFILT_FNC((_T("[SOURCE::InActive]<ERROR><->")));
		return hr;
	}

	if (ThreadExists()) 
	{
		hr = Stop();

		if (FAILED(hr))
		{
			MPTSFILT_FNC((_T("[SOURCE::InActive]<ERROR><->")));
			return hr;
		}

		hr = Exit();
		if (FAILED(hr))
		{
			MPTSFILT_FNC((_T("[SOURCE::InActive]<ERROR><->")));
			return hr;
		}

		Close();	// Wait for the thread to exit, then tidy up.
	}

	if (FAILED(hr = m_pPushAllocator->Decommit()))
	{
		return hr;
	}

	if( m_pRefClock )
	{ 
		m_pRefClock->Release(); 
		m_pRefClock = NULL; 
	}	

	// hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	//if (FAILED(hr)) {
	//	return hr;
	//}
	MPTSFILT_FNC((_T("[SOURCE::InActive]<->")));
	return NOERROR;
}

HRESULT ISOFormatPushStreamPin::FillBuffer(IMediaSample* pSample)
{
	m_csQueue.Lock();
	if( m_lPacketQueue.empty() )
	{
		m_csQueue.Unlock();
		return E_FAIL;
	}
	IMediaSample *LocalPacket = *(m_lPacketQueue.begin());
	*pSample = *LocalPacket;

	return S_OK; //not implemented
}

HRESULT ISOFormatPushStreamPin::GetMediaType( CMediaType *pMediaType)
{
	CAutoLock Lock(m_pFilter->pStateLock());

	*pMediaType = m_MediaType;
    if( m_MediaType.cbFormat != 0 )
	{
        pMediaType->pbFormat = (PBYTE)CoTaskMemAlloc(m_MediaType.cbFormat);
        if (pMediaType->pbFormat == NULL)
		{
            pMediaType->cbFormat = 0;
            return E_OUTOFMEMORY;
        } 
		else
		{
            CopyMemory((PVOID)pMediaType->pbFormat, (PVOID)m_MediaType.pbFormat, pMediaType->cbFormat);
        }
    }
    if( pMediaType->pUnk != NULL ) 
	{
        pMediaType->pUnk->AddRef();
    }

	return S_OK;
}

HRESULT ISOFormatPushStreamPin::SetMediaTypeInternal( CMediaType *pMediaType )
{
	CopyMediaType( &m_MediaType, pMediaType );
	return __super::SetMediaType( pMediaType );
}

HRESULT ISOFormatPushStreamPin::QueueDataToPush( unsigned char *pdata, int nDataLen, LONGLONG nTimeStamp, LONGLONG nTimeDisplay )
{
	if( IsConnected() == false )
		return S_OK;
	//try to allocate storage buffer
	IMediaSample *pSample;
	HRESULT hr = GetDeliveryBuffer( &pSample, NULL, NULL, 0 );
	//not initialized or PIN is inactive
	if (FAILED(hr)) 
		return S_FALSE;
	//not enough space allocated ? We are doomed then
	if( pSample->GetSize() < nDataLen )
	{
		pSample->Release();
		return S_FALSE;
	}

	BYTE *pSampleData = NULL;
	hr = pSample->GetPointer( &pSampleData );
	if( hr != S_OK || pSampleData == NULL )
	{
		pSample->Release();
		return E_NOT_SUFFICIENT_BUFFER;
	}

	memcpy( pSampleData, pdata, nDataLen );

	//Should not happen
	hr = pSample->SetActualDataLength( nDataLen );
	if( hr != S_OK )
	{
		pSample->Release();
		return hr;
	}

	LONGLONG TimeEnd = nTimeStamp + nTimeDisplay;
	if( nTimeDisplay == 0 )
		TimeEnd = 0;
	hr = pSample->SetTime( &nTimeStamp, &TimeEnd );
	hr = pSample->SetMediaTime( &nTimeStamp, &TimeEnd );

	pSample->SetDiscontinuity( FALSE );
	pSample->SetPreroll( FALSE );
	pSample->SetSyncPoint( FALSE );

	//most probably the size of the queue is ok now
	m_csQueue.Lock();
	m_lPacketQueue.push_back( pSample );
	m_csQueue.Unlock();

	return S_OK;
}

HRESULT ISOFormatPushStreamPin::QueueDataPop( IMediaSample **pSample )
{
#if defined( HACK_DELAY_SEND_TO_GUESS_TIME ) && defined( MEDIA_TIMESTAMP_FROM_NEXT_PACKET )
	int AntiDeadlockProtection = 1000;
	while( m_lPacketQueue.size() < 2 && AntiDeadlockProtection > 0 )
	{
		Sleep( DEFAULT_THREAD_WAIT_SLEEP_MS );
		AntiDeadlockProtection--;
	}
#endif

	HRESULT hr = S_OK;

	m_csQueue.Lock();

	if( m_lPacketQueue.empty() )
	{
		m_csQueue.Unlock();
		return S_FALSE;
	}

	//fetch a queued packet
	IMediaSample *LocalPacket = *(m_lPacketQueue.begin());
	m_lPacketQueue.pop_front();
#if defined( HACK_DELAY_SEND_TO_GUESS_TIME ) && defined( MEDIA_TIMESTAMP_FROM_NEXT_PACKET )
	if( m_lPacketQueue.empty() == false )
	{
		IMediaSample *pNextSample = *(m_lPacketQueue.begin());
		REFERENCE_TIME refStartCur,refEndCur;
		LocalPacket->GetTime( &refStartCur, &refEndCur );
		REFERENCE_TIME refStartNext,refEndNext;
		pNextSample->GetTime( &refStartNext, &refEndNext );
		LocalPacket->SetTime( &refStartCur, &refStartNext );
		hr = CopySample(LocalPacket, *pSample);
		SAFE_RELEASE(LocalPacket);
	}
#endif

	m_csQueue.Unlock();

	if (hr != S_OK) return E_FAIL;

#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
	REFERENCE_TIME refStart,refEnd;
	(*pSample)->GetTime( &refStart, &refEnd );
	m_pOwner->UpdateSentDuration( m_nTrackID, m_nPinType, refEnd - refStart );
#endif

	//special packet ?
	if( m_bDiscontinuity == TRUE )
	{
		(*pSample)->SetDiscontinuity( TRUE );
		m_bDiscontinuity = FALSE;
	}

	return S_OK; 
}



STDMETHODIMP ISOFormatPushStreamPin::GetCapabilities(DWORD * pCapabilities)
{
    *pCapabilities = AM_SEEKING_CanSeekAbsolute |
                     AM_SEEKING_CanSeekForwards |
                     AM_SEEKING_CanSeekBackwards |
                     AM_SEEKING_CanGetDuration |
                     AM_SEEKING_CanGetCurrentPos |
                     AM_SEEKING_CanGetStopPos;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::CheckCapabilities(DWORD * pCapabilities)
{
    DWORD dwActual;
    GetCapabilities(&dwActual);
    if (*pCapabilities & (~dwActual))
    {
        return S_FALSE;
    }
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::IsFormatSupported(const GUID * pFormat)
{
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
    {
        return S_OK;
    }
    return S_FALSE;

}

STDMETHODIMP ISOFormatPushStreamPin::QueryPreferredFormat(GUID * pFormat)
{
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::GetTimeFormat(GUID *pFormat)
{
    return QueryPreferredFormat(pFormat);
}

STDMETHODIMP ISOFormatPushStreamPin::IsUsingTimeFormat(const GUID * pFormat)
{
    GUID guidActual;
    HRESULT hr = GetTimeFormat(&guidActual);

    if (SUCCEEDED(hr) && (guidActual == *pFormat))
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

STDMETHODIMP ISOFormatPushStreamPin::ConvertTimeFormat( LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
    if (pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME)
    {
        if (pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME)
        {
            *pTarget = Source;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

STDMETHODIMP ISOFormatPushStreamPin::SetTimeFormat(const GUID * pFormat)
{
    // only one pin can control seeking for the whole filter.
    // This method is used to select the seeker.
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
    {
        // try to select this pin as seeker (if the first to do so)
        if (m_pOwner->SelectSeekingPin(this))
        {
            return S_OK;
        }
        else
        {
            return E_NOTIMPL;
        }
    }
    else if (*pFormat == TIME_FORMAT_NONE)
    {
        // deselect ourself, if we were the controlling pin
        m_pOwner->DeselectSeekingPin(this);
        return S_OK;
    }
    else
    {
        // no other formats supported
        return E_NOTIMPL;
    }
}

STDMETHODIMP ISOFormatPushStreamPin::GetDuration(LONGLONG *pDuration)
{
    *pDuration = m_pOwner->GetDuration() * 10000;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::GetStopPosition(LONGLONG *pStop)
{
    REFERENCE_TIME tStart, tStop;
    double dRate;
    m_pOwner->GetSeekingParams(&tStart, &tStop, &dRate);
    *pStop = tStop;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::GetCurrentPosition(LONGLONG *pCurrent)
{
    // this method is not supposed to report the previous start
    // position, but rather where we are now. This is normally
    // implemented by renderers, not parsers
    UNREFERENCED_PARAMETER(pCurrent);
    return E_NOTIMPL;
}

STDMETHODIMP ISOFormatPushStreamPin::SetPositions( LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    if (!m_pOwner->SelectSeekingPin(this))
    {
        return S_OK;
    }

    // fetch current properties
    REFERENCE_TIME tStart, tStop;
    double dRate;
    m_pOwner->GetSeekingParams(&tStart, &tStop, &dRate);
    if (dwCurrentFlags & AM_SEEKING_AbsolutePositioning)
        tStart = *pCurrent;
    else if (dwCurrentFlags & AM_SEEKING_RelativePositioning)
        tStart += *pCurrent;

    if (dwStopFlags & AM_SEEKING_AbsolutePositioning)
        tStop = *pStop;
    else if (dwStopFlags & AM_SEEKING_IncrementalPositioning)
        tStop = *pStop + tStart;
    else
    {
        if (dwStopFlags & AM_SEEKING_RelativePositioning)
            tStop += *pStop;
    }

    if (dwCurrentFlags & AM_SEEKING_PositioningBitsMask)
    {
        return m_pOwner->Seek(tStart, tStop, dRate);
    }
    else if (dwStopFlags & AM_SEEKING_PositioningBitsMask)
    {
        // stop change only
        return m_pOwner->SetStopTime(tStop);
    }
    else
    {
        // no operation required
        return S_FALSE;
    }

}

STDMETHODIMP ISOFormatPushStreamPin::GetPositions(LONGLONG * pCurrent, LONGLONG * pStop)
{
    REFERENCE_TIME tStart, tStop;
    double dRate;
    m_pOwner->GetSeekingParams(&tStart, &tStop, &dRate);
    *pCurrent = tStart;
    *pStop = tStop;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest)
{
    if (pEarliest != NULL)
        *pEarliest = 0;
    if (pLatest != NULL)
        *pLatest = m_pOwner->GetDuration() * 10000;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::SetRate(double dRate)
{
    HRESULT hr = S_OK;
	//we do not support AM_SEEKING_CanPlayBackwards
	if( dRate <= 0.0 )
		return E_INVALIDARG;
    if (m_pOwner->SelectSeekingPin(this))
        hr = m_pOwner->SetRate(dRate);
    return hr;
}

STDMETHODIMP ISOFormatPushStreamPin::GetRate(double * pdRate)
{
    REFERENCE_TIME tStart, tStop;
    double dRate;
    m_pOwner->GetSeekingParams(&tStart, &tStop, &dRate);
    *pdRate = dRate;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::GetPreroll(LONGLONG * pllPreroll)
{
    // don't need to allow any preroll time for us
    *pllPreroll = 0;
    return S_OK;
}

STDMETHODIMP ISOFormatPushStreamPin::QueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
	IMPLEMENT_QUERY_INTERFACE( IMediaSeeking );
	return __super::QueryInterface(riid, ppv);
} 

ULONG STDMETHODCALLTYPE ISOFormatPushStreamPin::AddRef()
{
	return __super::AddRef();
}

ULONG STDMETHODCALLTYPE ISOFormatPushStreamPin::Release()
{
	return __super::Release();
}

STDMETHODIMP ISOFormatPushStreamPin::Stop()
{
	m_bDiscontinuity = TRUE;
#ifdef SLEEP_PUSHER_PIN_UNTIL_SYSTEM_TIMESTAMP
	m_nSystemClockStart = UNDEFINED_CLOCK_VALUE;
	m_nStreamClockStart = UNDEFINED_CLOCK_VALUE;
#endif
	return __super::Stop();
}
