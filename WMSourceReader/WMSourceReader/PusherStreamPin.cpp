#include "stdafx.h"
#include "PusherStreamPin.h"
#include "WMSourceReaderFilter.h"
#include "utils.h"

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

PusherStreamPin::PusherStreamPin( TCHAR * pObjectName, HRESULT * pHr, CVidiatorWMSourceReaderFilter * pFilter, LPCWSTR pPinName, GUID MajorType )
:CSourceStream( pObjectName, pHr, pFilter, pPinName )
,m_nBufferNum( 2 )
,m_nBufferSize( 35536 )
,m_bFirstPacket( FALSE )
,m_PrevStartTime( TIME_Unknown )
,m_pRefClock( NULL )
,m_ReportTime( 0 )
,m_bEndOfStream( FALSE )
{
	memset( &m_MediaType, 0, sizeof( m_MediaType ) );
	m_MediaType.majortype = MajorType;
	//fake settings. We know our future output type. Not the details
	if( m_MediaType.majortype ==  MEDIATYPE_Video )
	{
		m_nBufferNum = DEFAULT_VIDEO_BUFFER_COUNT;
		m_MediaType.subtype = MEDIASUBTYPE_IYUV;
		m_MediaType.AllocFormatBuffer( sizeof( VIDEOINFOHEADER ) );
		memset( m_MediaType.pbFormat, 0 , sizeof( VIDEOINFOHEADER ) );
		m_MediaType.formattype = FORMAT_VideoInfo;
		VIDEOINFOHEADER *VIHour = (VIDEOINFOHEADER *)m_MediaType.pbFormat;
		VIHour->bmiHeader.biSize = sizeof( VIDEOINFOHEADER );
		VIHour->bmiHeader.biBitCount = 24;
		VIHour->bmiHeader.biPlanes = 1;
		VIHour->bmiHeader.biWidth = 1;
		VIHour->bmiHeader.biHeight = 1;
		VIHour->bmiHeader.biSizeImage = 0;
		m_MediaType.bFixedSizeSamples = 1;
		m_MediaType.lSampleSize = VIHour->bmiHeader.biWidth * VIHour->bmiHeader.biHeight * 3 / 2;
	}
	else if( m_MediaType.majortype ==  MEDIATYPE_Audio )
	{
		m_nBufferNum = DEFAULT_AUDIO_BUFFER_COUNT;
		m_MediaType.subtype = MEDIASUBTYPE_PCM;
		m_MediaType.AllocFormatBuffer( sizeof( WAVEFORMATEX ) );
		memset( m_MediaType.pbFormat, 0 , sizeof( WAVEFORMATEX ) );
		m_MediaType.formattype = FORMAT_WaveFormatEx;
		WAVEFORMATEX *AIHour = (WAVEFORMATEX *)m_MediaType.pbFormat;
		AIHour->nChannels = 1;
		AIHour->nSamplesPerSec = 8000;
		AIHour->wBitsPerSample = 16;
		m_MediaType.bFixedSizeSamples = 0; // samples do not come in same frequency and size
		m_MediaType.lSampleSize = AIHour->nChannels * AIHour->nSamplesPerSec * AIHour->wBitsPerSample / 8; //this is just a rough estimation as samples will vary
	}
#ifdef QUEUE_BLOCK_AFTER_SIZE
	m_hQueuePushBlock = ::CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
#ifdef ENABLE_PIN_DEBUG_LOGS
	m_fdLogs = NULL;
	char FileName[100];
	//it's all because of Danilo and he's special PC
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"./Log/WMReader_PIN_%d_%p_%d.txt", (char)(m_MediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"./Logs/WMReader_PIN_%d_%p_%d.txt", (char)(m_MediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"C:/temp/WMReader_PIN_%d_%p_%d.txt", (char)(m_MediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"C:/WMReader_PIN_%d_%p_%d.txt", (char)(m_MediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
#endif
}
PusherStreamPin::~PusherStreamPin( )
{
	int Remove = 0;
	m_csQueue.Lock();
	while( !m_lPacketQueue.empty() )
	{
		IMediaSample *LocalPacket = *(m_lPacketQueue.begin());
		LocalPacket->Release();
		m_lPacketQueue.pop_front();
	}
	m_csQueue.Unlock();
#ifdef QUEUE_BLOCK_AFTER_SIZE
	::CloseHandle( m_hQueuePushBlock );
#endif
#ifdef ENABLE_PIN_DEBUG_LOGS
	if( m_fdLogs )
	{
		fclose( m_fdLogs );
		m_fdLogs = NULL;
	}
#endif
}

STDMETHODIMP PusherStreamPin::Notify(IBaseFilter *pSender, Quality q)
{
	//return CSourceStream::Notify(pSender, q);
	return S_OK;
}

STDMETHODIMP PusherStreamPin::SetSink(IQualityControl *piqc)
{
	return S_OK;
}


HRESULT PusherStreamPin::DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	HRESULT hResult = NOERROR;

	CheckPointer(pMemAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	if( pProperties->cBuffers < m_nBufferNum )
		pProperties->cBuffers = m_nBufferNum;

	if( pProperties->cbBuffer < m_nBufferSize )
		pProperties->cbBuffer = m_nBufferSize;

	ALLOCATOR_PROPERTIES Actual;
	if( FAILED(hResult = pMemAlloc->SetProperties(pProperties, &Actual)) )
	{
		d1Log( "Allocator error : Could not set properties\n" );
		return hResult;
	}

	if( Actual.cbBuffer < pProperties->cbBuffer || Actual.cBuffers < pProperties->cBuffers ) 
	{
		d1Log2( "Allocator error : Could not negotiate properties %d %d\n", Actual.cbBuffer, Actual.cBuffers );
		return E_FAIL;
	}

	d1Log2( "Allocator status : Will use values %d %d\n", Actual.cbBuffer, Actual.cBuffers );

	return NOERROR;
}

//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT PusherStreamPin::DoBufferProcessingLoop(void) 
{
	Command com;
	OnThreadStartPlay();

	do 
	{
		while (!CheckRequest(&com)) 
		{
			IMediaSample *pSample;
			HRESULT hr = QueueDataPop( &pSample );

			if (hr == S_OK) 
			{
#ifdef ENABLE_PIN_DEBUG_LOGS
				LONGLONG start, end;
				pSample->GetTime( &start, &end );
				d4Log2( "Stream : Media times %d %d \n", (int)start, (int)end );
				d4Log4( "Stream : Preparing to send size at %d size %d, in queue %d, free pool %d \n", GetTickCount(), pSample->GetActualDataLength(), m_lPacketQueue.size(), m_nBufferNum - m_lPacketQueue.size() );
#endif
				hr = Deliver(pSample);
#ifdef ENABLE_PIN_DEBUG_LOGS
				d4Log1( "Stream : Finished sending at %d \n", GetTickCount() );
				if( ((CMediaSample*)pSample)->m_cRef != 1 )
					printf("Stream : !!! warning, something did not release reference to the buffer Ref counter : %d \n", ((CMediaSample*)pSample)->m_cRef );
#endif
				pSample->Release();
#ifdef QUEUE_BLOCK_AFTER_SIZE
				//we pushed out a packet ? that means we can allow a new packet to be queued
				::SetEvent( m_hQueuePushBlock );
#endif
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
					d4Log( "Stream : Signalling EOF, no more data to send\n" );
					DeliverEndOfStream();
					MPTSFILT_INF((_T("[SOURCE::DoBufferProcessingLoop] Name(%s), Deliver End Of Stream"), m_pName));
					return S_OK;
				}				
				//Maybe we should wait until there is some buffer to be pushed ? Might lead to high CPU usage
				Sleep( DEFAULT_THREAD_WAIT_SLEEP_MS );
//				d4Log( "Stream : Queue is empty, waiting for more data\n" );
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
//
// Active
//
// The pin is active - start up the worker thread
HRESULT PusherStreamPin::Active(void) 
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

	m_bFirstPacket = TRUE;
	m_PrevStartTime = TIME_Unknown;

	hr = CBaseOutputPin::Active();
	if (FAILED(hr)) 
	{
		MPTSFILT_FNC((_T("[SOURCE::Active]<ERROR><->")));
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
		
	m_TimerReportStream.Reset( );
	m_ReportTime = 0;
	m_RefClockStart = 0;
	if( m_pRefClock )
	{
		m_pRefClock->GetTime( &m_RefClockStart );
	}

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

HRESULT PusherStreamPin::Inactive(void) {

	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;
	MPTSFILT_FNC((_T("[SOURCE::InActive]<+>")));

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if( !IsConnected() )
	{
		MPTSFILT_FNC((_T("[SOURCE::InActive]<ERROR><->")));
		return NOERROR;
	}

	// !!! need to do this before trying to stop the thread, because
	// we may be stuck waiting for our own allocator!!!

	hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	if (FAILED(hr))
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

BOOL PusherStreamPin::FillSample( IMediaSample *pSample, MEDIAPacket *pPacket )
{
	if( pPacket == NULL || pSample == NULL )
	{
		return FALSE;
	}
	int Allocated = pSample->GetSize();

	BYTE * pData = NULL ; 
	HRESULT hResult =  pSample->GetPointer( &pData );

	if (FAILED(hResult))
	{
		//wcs(_T("{CViVideoOutStream::FillBuffer} S_FALSE.\n"));
		return FALSE;
	}

	if( pPacket->DataLen > Allocated )
	{
		d1Log( "Sample prepare : Error, received data does not fit into buffer. Should break connection\n" );
		return FALSE;
	}

	memcpy( pData, pPacket->Data, pPacket->DataLen );
	pSample->SetActualDataLength( pPacket->DataLen );

	if( pPacket->Start != TIME_Unknown )
	{
		REFERENCE_TIME refStart = MILLISECONDS_TO_100NS_UNITS( pPacket->Start );
		REFERENCE_TIME refEnd = MILLISECONDS_TO_100NS_UNITS( pPacket->End );
		pSample->SetTime( &refStart, &refEnd );
	}	

	MPTSFILT_PACKET((_T("[Packets] Name(%s){ DataLen(%d), Time(%I64d)}"),m_pName, pPacket->DataLen, StartTime) );

	return TRUE;
}

HRESULT PusherStreamPin::FillBuffer(IMediaSample* pSample)
{
	return S_FALSE; //not implemented
}

HRESULT PusherStreamPin::GetMediaType( CMediaType *pMediaType)
{
	CAutoLock Lock(m_pFilter->pStateLock());

	*pMediaType = m_MediaType;

	return S_OK;
}

HRESULT PusherStreamPin::SetMediaType( CMediaType *pMediaType )
{
	///////////////////
	// Default Buffer Size. 
	m_nBufferNum = 2;
	m_nBufferSize = 16384;

	//output is hardcoded. This pin is made for a specific filter
	if( m_MediaType.majortype ==  MEDIATYPE_Video )
	{
		m_nBufferNum = DEFAULT_VIDEO_BUFFER_COUNT;
#ifndef READER_DOES_NOT_CHANGE_INPUT_TYPE
		m_MediaType.subtype = MEDIASUBTYPE_IYUV;
#else
		m_MediaType.subtype = pMediaType->subtype;
#endif
		BITMAPINFOHEADER bih;
		ExtractBIH( pMediaType, &bih );
		if( m_MediaType.formattype == FORMAT_VideoInfo )
			((VIDEOINFOHEADER *)m_MediaType.pbFormat)->bmiHeader = bih;
		m_nBufferSize = bih.biWidth * bih.biHeight * 3 / 2;
		m_MediaType.bFixedSizeSamples = 1; //non compressed
		m_MediaType.lSampleSize = m_nBufferSize; 
	}
	else if( m_MediaType.majortype ==  MEDIATYPE_Audio )
	{
		m_nBufferNum = DEFAULT_AUDIO_BUFFER_COUNT;
#ifndef READER_DOES_NOT_CHANGE_INPUT_TYPE
		m_MediaType.subtype = MEDIASUBTYPE_PCM;
#else
		m_MediaType.subtype = pMediaType->subtype;
#endif
		if( pMediaType->formattype == FORMAT_WaveFormatEx )
		{
			memcpy( m_MediaType.pbFormat, pMediaType->pbFormat, sizeof( WAVEFORMATEX ) );
			WAVEFORMATEX *AIHour = (WAVEFORMATEX *)pMediaType->pbFormat;
			m_nBufferSize = AIHour->nSamplesPerSec * AIHour->nChannels * AIHour->wBitsPerSample / 8;
		}
		m_MediaType.bFixedSizeSamples = 1; //non compressed
		m_MediaType.lSampleSize = m_nBufferSize; 
	}
	else
	{
		d1Log( "SetMediaType : Unexpected media format\n" );
	}
//	m_MediaType = *pMediaType;
	return __super::SetMediaType( &m_MediaType );
}

HRESULT PusherStreamPin::SetMediaType( WM_MEDIA_TYPE *pMediaType )
{
	///////////////////
	// Default Buffer Size. 
	m_nBufferNum = 2;
	m_nBufferSize = 16384;

	//output is hardcoded. This pin is made for a specific filter
	if( m_MediaType.majortype ==  WMMEDIATYPE_Video )
	{
		m_nBufferNum = DEFAULT_VIDEO_BUFFER_COUNT;
#ifndef READER_DOES_NOT_CHANGE_INPUT_TYPE
		m_MediaType.subtype = MEDIASUBTYPE_IYUV;
#else
		m_MediaType.subtype = pMediaType->subtype;
#endif
		BITMAPINFOHEADER bih;
		ExtractBIH( pMediaType, &bih );
		if( m_MediaType.formattype == FORMAT_VideoInfo )
			((VIDEOINFOHEADER *)m_MediaType.pbFormat)->bmiHeader = bih;
		m_nBufferSize = bih.biWidth * bih.biHeight * 3 / 2;
		m_MediaType.bFixedSizeSamples = 1; //non compressed
		m_MediaType.lSampleSize = m_nBufferSize; 
	}
	else if( m_MediaType.majortype ==  WMMEDIATYPE_Audio )
	{
		m_nBufferNum = DEFAULT_AUDIO_BUFFER_COUNT;
#ifndef READER_DOES_NOT_CHANGE_INPUT_TYPE
		m_MediaType.subtype = MEDIASUBTYPE_PCM;
#else
		m_MediaType.subtype = pMediaType->subtype;
#endif
		if( pMediaType->formattype == FORMAT_WaveFormatEx )
		{
			memcpy( m_MediaType.pbFormat, pMediaType->pbFormat, sizeof( WAVEFORMATEX ) );
			WAVEFORMATEX *AIHour = (WAVEFORMATEX *)pMediaType->pbFormat;
			m_nBufferSize = AIHour->nSamplesPerSec * AIHour->nChannels * AIHour->wBitsPerSample / 8;
		}
		m_MediaType.bFixedSizeSamples = 1; //non compressed
		m_MediaType.lSampleSize = m_nBufferSize; 
	}
	else
	{
		d1Log( "SetMediaType : Unexpected media format\n" );
	}
	return S_OK;
}

HRESULT PusherStreamPin::QueueDataToPush( MEDIAPacket *pPacket )
{
	if( IsConnected() == false )
	{
		d1Log( "Queue push : PIN is not yet connected. Data is thrown away\n" );
		return S_OK;
	}
	//in case we are reading something that is already cached, let's not double allocate buffer for the sake of double/ tripple... caching
#if QUEUE_BLOCK_AFTER_SIZE > 0 
//	while( m_lPacketQueue.size() > QUEUE_BLOCK_AFTER_SIZE )
//		Sleep( QUEUE_POLL_UPDATE_SLEEP );
	if( m_lPacketQueue.size() > QUEUE_BLOCK_AFTER_SIZE )
	{
		::ResetEvent( m_hQueuePushBlock );
		//do not block forever. From time to time check if the pusher thread is halted or the whole filter is shutting down
		while( ::WaitForSingleObject( m_hQueuePushBlock, QUEUE_POLL_UPDATE_SLEEP ) == QUEUE_POLL_UPDATE_SLEEP )
		{
			if( IsStopped() )
				return S_OK;
		}
	}
#endif
	//try to allocate storage buffer
	IMediaSample *pSample;
	HRESULT hr = GetDeliveryBuffer( &pSample, NULL, NULL, 0 );
	if (FAILED(hr)) 
	{
		d1Log( "Queue push : Could not fetch buffer. Maybe we are not yet initialized ?\n" );
		return S_FALSE;
	}

	//make sure to signal no data in case of error. Should never happen
	pSample->SetActualDataLength( 0 );
	
	//fill packet with data 
	FillSample( pSample, pPacket );

	//most probably the size of the queue is ok now
	m_csQueue.Lock();
	m_lPacketQueue.push_back( pSample );
	m_csQueue.Unlock();

	return S_OK;
}

HRESULT PusherStreamPin::QueueDataPop( IMediaSample **pSample )
{
	*pSample = NULL; //nothing unless we successfully pop something

#ifdef HACK_DELAY_SEND_TO_GUESS_TIME
	int AntiDeadlockProtection = 1000;
	while( m_lPacketQueue.size() < 2 && AntiDeadlockProtection > 0 )
	{
		Sleep( DEFAULT_THREAD_WAIT_SLEEP_MS );
		AntiDeadlockProtection--;
	}
#endif

	m_csQueue.Lock();
	if( m_lPacketQueue.empty() )
	{
		m_csQueue.Unlock();
		return S_FALSE;
	}

	//fetch a queued packet
	IMediaSample *LocalPacket = *(m_lPacketQueue.begin());
	m_lPacketQueue.pop_front();
#ifdef HACK_DELAY_SEND_TO_GUESS_TIME
	if( m_lPacketQueue.empty() == false )
	{
		IMediaSample *pNextSample = *(m_lPacketQueue.begin());
		REFERENCE_TIME refStartCur,refEndCur;
		LocalPacket->GetTime( &refStartCur, &refEndCur );
		REFERENCE_TIME refStartNext,refEndNext;
		pNextSample->GetTime( &refStartNext, &refEndNext );
		LocalPacket->SetTime( &refStartCur, &refStartNext );
	}
#endif
	m_csQueue.Unlock();

	//special packet ?
	if( m_bFirstPacket == TRUE )
	{
		LocalPacket->SetDiscontinuity( TRUE );
		//pSample->SetPreroll( TRUE );
		//pSample->SetSyncPoint( TRUE );
		m_bFirstPacket = FALSE;
	}
	LocalPacket->SetSyncPoint( TRUE );	

	//note, at this point we most probably still have 1 ref count. We poped from one list and add return it to some other list
	*pSample = LocalPacket;

	return S_OK; 
}
