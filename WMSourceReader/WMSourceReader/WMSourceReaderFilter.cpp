#include "StdAfx.h"
#include "WMSourceReaderFilter.h"
#include "utils.h"
#include "ExtraGuids.h"
#include "nxColorConversionLib/nxColorConversion.h"

#define AUDIO_PIN					((CTransformInputPin*)m_pInput)
#define OUTPUT_PIN					((CTransformOutputPin*)m_pOutput)

#define WMRD_INF0(a)	1//
//disable debugging for now. Will implement later
#ifdef GLOG
	#undef GLOG
	#define GLOG(a)			1//
#else
	#define GLOG(a)			1//
#endif
#define ISMLOG			1//

#define USE_NEW_SAVE_FORMAT

#define LOCAL_STRING_BUFFER_SIZE	256

CVidiatorWMSourceReaderFilter::CVidiatorWMSourceReaderFilter(LPUNKNOWN pUnk,
                           HRESULT *phr) : CSource( NAME("CVidiatorWMSourceReader"), pUnk, CLSID_WMSourceReader )
{
#ifdef SUPPORT_CALLBACKS
	m_pSampleCallback = NULL;
	m_pEventCallback = NULL;
#endif
	m_wmStatus = WMT_ERROR;
	m_hWMResult = E_FAIL;
//	m_bLoop = true;
	m_StartTimeOffset = 0 ;
	m_VideoLastTS = TIME_Unknown;
	m_AudioLastTS = TIME_Unknown;
	m_VideoAvgDur = 0;
	m_AudioLastDur = 0;
	m_bReaderInit = FALSE;
	m_bReaderOpened = FALSE;
	m_bReaderStarted = FALSE;
	m_eInputSubtype = RGB24;
	m_dwSrcWidth = 0;
	m_dwSrcHeight = 0;
	m_pIYUVTempBuff = NULL;
	m_nIYUVTempBuff = 0;
#ifndef USE_POLLING_EVENTS
	for(int i = 0 ; i < SYNC_METHOD_MAX; ++i)
		m_pMethodHandleArray[i] = ::CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
	InitializeCriticalSection(&m_CSIYUVBuff);
#ifdef SUPPORT_OUTPUT_PINS
	m_pVideoOut = new PusherStreamPin( L"CVidiatorWMSourceReader Video Out ", phr, this, L"Video", MEDIATYPE_Video );
	m_pAudioOut = new PusherStreamPin( L"CVidiatorWMSourceReader Audio Out ", phr, this, L"Audio", MEDIATYPE_Audio );
#endif
}

CVidiatorWMSourceReaderFilter::~CVidiatorWMSourceReaderFilter(void)
{
	Close( );
	std::vector<WM_MEDIA_TYPE*>::iterator pIter;
	for( pIter = m_VecOutFormat.begin() ; pIter != m_VecOutFormat.end() ; ++pIter )
	{
		WM_MEDIA_TYPE *pMediaType = *pIter;
		if(pMediaType)
		{
			delete [] pMediaType;
			pMediaType = NULL;
		}		
	}
	m_VecOutFormat.clear();
#ifndef USE_POLLING_EVENTS
	for(int i = 0 ; i < SYNC_METHOD_MAX; ++i) 
		::CloseHandle(m_pMethodHandleArray[i]);
#endif
	EnterCriticalSection(&m_CSIYUVBuff);
 	SAFE_DELETE_ARRAY(m_pIYUVTempBuff);
	m_nIYUVTempBuff = 0;
	LeaveCriticalSection(&m_CSIYUVBuff);
	DeleteCriticalSection(&m_CSIYUVBuff);
#ifdef SUPPORT_OUTPUT_PINS
	if( m_pVideoOut )
	{
		delete m_pVideoOut;
		m_pVideoOut = NULL;
	}
	if( m_pAudioOut )
	{
		delete m_pAudioOut;
		m_pAudioOut = NULL;
	}
#endif

}
//

// NonDelegatingQueryInterface
//
//
STDMETHODIMP CVidiatorWMSourceReaderFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
//	IMPLEMENT_QUERY_INTERFACE( ISpecifyPropertyPages );
	IMPLEMENT_QUERY_INTERFACE( IWMReaderCallback );
	if (riid == __uuidof(IVidiatorWMSourceReader))
	{		
        return GetInterface((IVidiatorWMSourceReader*)this, ppv);
	}
	IMPLEMENT_QUERY_INTERFACE( IFileSourceFilter );
//	return __super::NonDelegatingQueryInterface(riid, ppv);
//	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	return CSource::NonDelegatingQueryInterface(riid, ppv);
} // NonDelegatingQueryInterface

// Send EndOfStream if no input connection
STDMETHODIMP CVidiatorWMSourceReaderFilter::Run(REFERENCE_TIME tStart)
{
	HRESULT hr = S_OK;
	hr = __super::Run(tStart);
	return hr;
}

STDMETHODIMP CVidiatorWMSourceReaderFilter::Pause()
{
//	CAutoLock lock( &m_csFilter );
	HRESULT hr = S_OK;
	if( IsStopped() )
	{
	}
	hr = __super::Pause();
	return hr;
}

void CVidiatorWMSourceReaderFilter::ResetTimeStamp( )
{
	MTime LastTime = TIME_Unknown;

	if( m_VideoLastTS != TIME_Unknown )
		LastTime = m_VideoLastTS + m_VideoAvgDur;	

	if( m_AudioLastTS != TIME_Unknown )
	{
		MTime AudioTime = m_AudioLastTS + m_AudioLastDur;
		if( AudioTime > m_VideoLastTS )
			LastTime = AudioTime;
	}

	WMRD_INF0( (TEXT("[WMRD:ResetTimeStamp] Offset(%I64d), VLastTS(%I64d), VAvgDur(%I64d), ALastTS(%I64d), ALastDur(%I64d), LastTime(%I64d)"), 
		m_StartTimeOffset, m_VideoLastTS, m_VideoAvgDur, m_AudioLastTS, m_AudioLastDur, LastTime ) );

	if( LastTime != TIME_Unknown )
		m_StartTimeOffset += LastTime; 

	m_VideoLastTS = TIME_Unknown;
	m_AudioLastTS = TIME_Unknown;
	m_VideoAvgDur = 0;
	m_AudioLastDur = 0;

#ifdef SUPPORT_GETAUDIOFORMAT_API
	m_nAudioByteRate = 0;
	WAVEFORMATEX AudioFormat;
	if( GetAudioFormat(&AudioFormat) )
	{
		m_nAudioByteRate = AudioFormat.nSamplesPerSec * AudioFormat.nChannels * (AudioFormat.wBitsPerSample/8);
	}
#endif
	WMRD_INF0( (TEXT("[WMRD:ResetTimeStamp] Offset(%I64d), m_nAudioByteRate (%d)"),m_StartTimeOffset, m_nAudioByteRate) );

}

bool CVidiatorWMSourceReaderFilter::InitReader( )
{
	WMRD_INF0( (TEXT("[WMRD:InitReader]<+>")) );
	if( m_bReaderInit )
	{
		WMRD_INF0( (TEXT("[WMRD:InitReader]<-> Already inited. ")) );
		return true;
	}

	HRESULT hResult;
	
	hResult = WMCreateReader(NULL, WMT_RIGHT_PLAYBACK, &m_pWMReader);
	//_ASSERT(SUCCEEDED(hr));
	if( FAILED(hResult) || !m_pWMReader )
	{
		WMRD_INF0( (TEXT("[WMRD:InitReader]<-><ERROR> Fail to create Reader, hResult(0x%x), Reader(0x%x)"), hResult, m_pWMReader ) );
		if( m_pWMReader )
		{
			m_pWMReader->Release();
			m_pWMReader = NULL;
		}
		return false;
	}
	
	m_bReaderInit = TRUE;

	WMRD_INF0( (TEXT("[WMRD:InitReader]<->")) );
	return true;
}

void CVidiatorWMSourceReaderFilter::DoneReader( )
{
	WMRD_INF0( (TEXT("[WMRD:DoneReader]<+> Init(%d), Open(%d), Start(%d)"), m_bReaderInit, m_bReaderOpened, m_bReaderStarted ) );
	if( m_bReaderInit )
	{
		if( m_bReaderStarted )
		{
			m_pWMReader->Stop( );
			m_bReaderStarted = FALSE;
		}

		if( m_bReaderOpened )
		{
			m_pWMReader->Close( );
			m_bReaderOpened = FALSE;
		}
		m_pWMReader->Release();
		m_pWMReader = NULL;

		m_bReaderInit = FALSE;
	}
	WMRD_INF0( (TEXT("[WMRD:DoneReader]<->") ) );
}

HRESULT CVidiatorWMSourceReaderFilter::Open(TCHAR *lpszURL )
{	
	//_ASSERT(lpszURL);

	WMRD_INF0( (TEXT("[WMRD:Open]<+>")) );

	if(!lpszURL || (_tcslen(lpszURL) <= 0))
	{
		WMRD_INF0( (TEXT("[WMRD:Open]<-><ERROR> Invalid argument.")) );
		return S_FALSE;
	}
	//_ASSERT(m_pWMReader);
	//if(!m_pWMReader)
	//{
	//	return false;
	//}	
	if( m_bReaderOpened )
	{
		WMRD_INF0( (TEXT("[WMRD:Open]<-><WARN> Already Opened")) );
		return S_OK;
	}

	WMRD_INF0( (TEXT("[WMRD:Open] Open URL(%s), ReOpen(%d), Inited(%d)"), lpszURL, bReOpen, m_bReaderInit ) );

	if( !InitReader( ) )
	{
		WMRD_INF0( (TEXT("[WMRD:Open]<-><ERROR> Reader-Init Failure! ")) );
		return S_FALSE;
	}
	//2011/02/22 mark yh kim -  fixed crash
//	m_strURL4Log.clear();
	
	if( lpszURL )
	{
//		m_strURL4Log=lpszURL;
	}
	
	m_VideoLastTS = TIME_Unknown;
	m_AudioLastTS = TIME_Unknown;
	m_StartTimeOffset = 0;
	m_VideoAvgDur = 0;
	m_AudioLastDur = 0;

#ifndef USE_POLLING_EVENTS
	::ResetEvent(m_pMethodHandleArray[OPEN_METHOD]);
#else
	Events[OPEN_METHOD]=0;
#endif
	HRESULT hResult;
	hResult = m_pWMReader->Open(lpszURL, this, (void*)this);
	if(FAILED(hResult))
	{
		WMRD_INF0( (TEXT("[WMRD:Open]<ERROR>  Fail to open. URL(%s)"), lpszURL) );
		GLOG( (ISMLOG, TEXT("[WMReader:Open]<ERROR> File to open source, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), hResult) );
		return S_FALSE;
	} 

	if(!WaitforWMStatus(OPEN_METHOD, 10000))
	{
		//_ASSERT(!_T("Open Timed out!!!"));
		WMRD_INF0( (TEXT("[WMRD:Open]<ERROR> Open Timeout, Close Reader")) );
		hResult = m_pWMReader->Close( );	
		DoneReader( ); // fix memory leak, vos 3998, 2010-09-29, tue, jerry
		WMRD_INF0( (TEXT("[WMRD:Open]<ERROR> Open Time Out. URL(%s)"), lpszURL) );
		GLOG( (ISMLOG, TEXT("[WMReader:Open]<ERROR> Open Time Out,URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
		return S_FALSE;
	}	
	
	if(FAILED(m_hWMResult))
	{
		WMRD_INF0( (TEXT("[WMRD:Open]<ERROR> Receive Error, Close Reader")) );
		hResult = m_pWMReader->Close( );	
		DoneReader( ); // fix memory leak, vos 3998, 2010-09-29, tue, jerry
		WMRD_INF0( (TEXT("[WMRD:Open]<ERROR> Fail to open, error occured. URL(%s)"), lpszURL) );
		GLOG( (ISMLOG, TEXT("[WMReader:Open]<ERROR> Open Error, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
		return S_FALSE;
	}

	SetYUVOutputProp();
	SetPCMOutputProp();

	m_bReaderOpened = TRUE;

	ResetTimeStamp( );

	WMRD_INF0( (TEXT("[WMRD:Open]<->")) );

	return S_OK;
}

void CVidiatorWMSourceReaderFilter::SetYUVOutputProp()
{
	DWORD dwOutputCount = 0;
	HRESULT hResult     = S_OK;

	if(FAILED(hResult = m_pWMReader->GetOutputCount(&dwOutputCount)))
	{
		WMRD_INF0( (TEXT("[WMRD:GetOutputCount]<ERROR>  Fail to get output count")) );
		GLOG( (ISMLOG, TEXT("[WMRD:GetOutputCount]<ERROR>  Fail to get output count"), m_strURL4Log.c_str(), hResult) );
		return;
	}
	
	E_INPUT_SUBTYPE eSubType = NON_SPECIFIED;

	for(DWORD i = 0;i<dwOutputCount;i++)
	{
		IWMOutputMediaProps * pMainOutputMediaProps = NULL;

		if(SUCCEEDED(hResult = m_pWMReader->GetOutputProps(i,&pMainOutputMediaProps)))
		{
			GUID guid;

			if(SUCCEEDED(hResult = pMainOutputMediaProps->GetType(&guid)))
			{
				if(WMMEDIATYPE_Video == guid)
				{
					DWORD	dwType = 0;
					hResult = pMainOutputMediaProps->GetMediaType(NULL,&dwType);
					//useless safety measure. Fear the buffer alignment horror
					if( dwType < sizeof( WM_MEDIA_TYPE ) )
						dwType = sizeof( WM_MEDIA_TYPE );
					BYTE * pMediaType = new BYTE[dwType];
					hResult = pMainOutputMediaProps->GetMediaType((WM_MEDIA_TYPE *)pMediaType,&dwType);

					WM_MEDIA_TYPE * pImMediaType = (WM_MEDIA_TYPE *)pMediaType;

					if(pImMediaType->formattype == WMFORMAT_VideoInfo || 
					   pImMediaType->formattype == WMFORMAT_MPEG2Video)
					{
						if(pImMediaType->formattype == WMFORMAT_VideoInfo)
						{
							WMVIDEOINFOHEADER * pVideoInfoHeader = (WMVIDEOINFOHEADER *)pImMediaType->pbFormat;

							m_dwSrcWidth  = pVideoInfoHeader->bmiHeader.biWidth;
							m_dwSrcHeight = pVideoInfoHeader->bmiHeader.biHeight; 
						}
						else if(pImMediaType->formattype == WMFORMAT_MPEG2Video)
						{
							WMMPEG2VIDEOINFO * pVideoInfoHeader = (WMMPEG2VIDEOINFO *)pImMediaType->pbFormat;
							
							m_dwSrcWidth  = pVideoInfoHeader->hdr.bmiHeader.biWidth;
							m_dwSrcHeight = pVideoInfoHeader->hdr.bmiHeader.biHeight; 
						}
					
						if(pImMediaType->subtype == WMMEDIASUBTYPE_IYUV)
						{
							eSubType = IYUV;
							pImMediaType->subtype = MEDIASUBTYPE_IYUV;
						}
						else
						{
							if(TRUE == SetMatchedOutputProps(i,WMMEDIASUBTYPE_IYUV))
							{
								eSubType = IYUV;
								pImMediaType->subtype = MEDIASUBTYPE_IYUV;
							}
							else
							{
								if(TRUE == SetMatchedOutputProps(i,WMMEDIASUBTYPE_YV12))
								{
									eSubType = YV12;
									pImMediaType->subtype = MEDIASUBTYPE_YV12;
								}
								else
								{
									eSubType = RGB24;
								}
							}
#ifdef SUPPORT_OUTPUT_PINS
							m_pVideoOut->SetMediaType( pImMediaType );
#endif
						}
					}

					SAFE_DELETE_ARRAY(pMediaType);
				}
			}

			SAFE_RELEASE(pMainOutputMediaProps);
		}
		//we support only 1 input
		if( eSubType != NON_SPECIFIED )
		{
			break;
		}
	} 

	m_eInputSubtype = eSubType;

	//only handle supported formats
	EnterCriticalSection(&m_CSIYUVBuff);
	if( m_eInputSubtype == RGB24 || m_eInputSubtype == YV12 || m_eInputSubtype == IYUV )
	{
		DWORD	dwTempBuff;
		if( m_eInputSubtype == RGB24 )
			dwTempBuff = m_dwSrcWidth*m_dwSrcHeight*3;
		else if( m_eInputSubtype == YV12 || m_eInputSubtype == IYUV )
			dwTempBuff = m_dwSrcWidth*m_dwSrcHeight*3/2;
		
		if( m_pIYUVTempBuff && m_nIYUVTempBuff < dwTempBuff )
		{	
			SAFE_DELETE_ARRAY(m_pIYUVTempBuff);
			m_pIYUVTempBuff = NULL;
			if(m_eInputSubtype == RGB24)
				GLOG( (EMCLOG,TEXT("[IYUV] alloc m_pIYUVTempBuff %d because SOURCE is RGB24"), m_nIYUVTempBuff));
			else if(m_eInputSubtype == YV12)
				GLOG( (EMCLOG,TEXT("[IYUV] alloc m_pIYUVTempBuff %d because SOURCE is YV12"), m_nIYUVTempBuff));
		}
		if( m_pIYUVTempBuff == NULL )
		{
			m_nIYUVTempBuff = dwTempBuff;
			m_pIYUVTempBuff = new BYTE[m_nIYUVTempBuff];

			if(m_eInputSubtype == RGB24)
				GLOG( (EMCLOG,TEXT("[IYUV] alloc m_pIYUVTempBuff %d because SOURCE is %s"), m_nIYUVTempBuff,#RGB24));
			else if(m_eInputSubtype == YV12)
				GLOG( (EMCLOG,TEXT("[IYUV] alloc m_pIYUVTempBuff %d because SOURCE is YV12"), m_nIYUVTempBuff));
		}
	} 
	else
	{
		if( m_pIYUVTempBuff != NULL )
		{
			SAFE_DELETE_ARRAY(m_pIYUVTempBuff);
			m_pIYUVTempBuff = NULL;
		}
		m_nIYUVTempBuff = 0;
	}
	LeaveCriticalSection(&m_CSIYUVBuff);
}


void CVidiatorWMSourceReaderFilter::SetPCMOutputProp()
{
	DWORD dwOutputCount = 0;
	HRESULT hResult     = S_OK;

	if(FAILED(hResult = m_pWMReader->GetOutputCount(&dwOutputCount)))
	{
		WMRD_INF0( (TEXT("[WMRD:GetOutputCount]<ERROR>  Fail to get output count")) );
		GLOG( (ISMLOG, TEXT("[WMRD:GetOutputCount]<ERROR>  Fail to get output count"), m_strURL4Log.c_str(), hResult) );
		return;
	}
	
	E_INPUT_SUBTYPE eSubType = NON_SPECIFIED;

	for(DWORD i = 0;i<dwOutputCount;i++)
	{
		IWMOutputMediaProps * pMainOutputMediaProps = NULL;

		if(SUCCEEDED(hResult = m_pWMReader->GetOutputProps(i,&pMainOutputMediaProps)))
		{
			GUID guid;

			if(SUCCEEDED(hResult = pMainOutputMediaProps->GetType(&guid)))
			{
				if(WMMEDIATYPE_Audio == guid)
				{
					DWORD	dwType = 0;
					hResult = pMainOutputMediaProps->GetMediaType(NULL,&dwType);
					//useless safety measure. Fear the buffer alignment horror
					if( dwType < sizeof( WM_MEDIA_TYPE ) )
						dwType = sizeof( WM_MEDIA_TYPE );
					BYTE * pMediaType = new BYTE[dwType];
					hResult = pMainOutputMediaProps->GetMediaType((WM_MEDIA_TYPE *)pMediaType,&dwType);

					WM_MEDIA_TYPE * pImMediaType = (WM_MEDIA_TYPE *)pMediaType;

					if(pImMediaType->formattype == WMFORMAT_WaveFormatEx )
					{
#ifdef SUPPORT_OUTPUT_PINS
						m_pAudioOut->SetMediaType( pImMediaType );
#endif
					}

					SAFE_DELETE_ARRAY(pMediaType);
				}
			}

			SAFE_RELEASE(pMainOutputMediaProps);
		}
	} 
}

BOOL CVidiatorWMSourceReaderFilter::SetMatchedOutputProps(DWORD dwStreamIdx,const IID & guid)
{
	DWORD				  dwFormatCount = 0;
	IWMOutputMediaProps * pOutputMediaProps = NULL;
	BOOL				  bFoundYV12Prop = FALSE;
	HRESULT				  hResult        = S_OK;

	if(SUCCEEDED(hResult = m_pWMReader->GetOutputFormatCount(dwStreamIdx,&dwFormatCount)))
	{
		for(DWORD j = 0; j<dwFormatCount;j++)
		{
			hResult = m_pWMReader->GetOutputFormat(dwStreamIdx,j,&pOutputMediaProps);

			DWORD	dwType = 0;
			hResult = pOutputMediaProps->GetMediaType(NULL,&dwType);
			BYTE * pMediaType = new BYTE[dwType];
			hResult = pOutputMediaProps->GetMediaType((WM_MEDIA_TYPE *)pMediaType,&dwType);

			WM_MEDIA_TYPE * pPtrMediaType = (WM_MEDIA_TYPE *)pMediaType;

			if(pPtrMediaType->formattype == WMFORMAT_VideoInfo ||
			   pPtrMediaType->formattype == WMFORMAT_MPEG2Video)
			{
				if(pPtrMediaType->subtype == guid)
				{
					m_pWMReader->SetOutputProps(dwStreamIdx,pOutputMediaProps);
					bFoundYV12Prop = TRUE;
				}
			}

			SAFE_DELETE_ARRAY(pMediaType); 
			SAFE_RELEASE(pOutputMediaProps);

			if(bFoundYV12Prop == TRUE)
				break;
		}
	}

	return bFoundYV12Prop;
}

HRESULT CVidiatorWMSourceReaderFilter::Start(void)
{
	//_ASSERT(m_pWMReader);
	HRESULT	hResult;
	//if(!m_pWMReader || !m_bReaderOpened )
	WMRD_INF0( (TEXT("[WMRD:Start]<+> Start SourceReader.")) );
	if( !m_bReaderInit || !m_bReaderOpened )
	{
		WMRD_INF0( (TEXT("[WMRD:Start]<-><ERROR> Init(0x%x), Opened(%d)"), m_bReaderInit, m_bReaderOpened ) );
		return S_FALSE;
	}

#ifndef USE_POLLING_EVENTS
	::ResetEvent(m_pMethodHandleArray[START_METHOD]);
#else
	Events[START_METHOD]=0;
#endif
	hResult = m_pWMReader->Start(0, 0, 1.0, (void*)this);
	if( FAILED(hResult) )
	{
		WMRD_INF0( (TEXT("[WMRD:Start]<-><ERROR> Failure Reader->Start, hResult(0x%x)"), hResult) );
		GLOG( (ISMLOG, TEXT("[WMReader:Start]<ERROR> Fail to start WMReader, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), hResult ) );		
		return S_FALSE;
	}

	if(!WaitforWMStatus(START_METHOD, 30000))
	{
		WMRD_INF0( (TEXT("[WMRD:Start]<ERROR> Timeout, Stop Reader")) );
		hResult = m_pWMReader->Stop( ); //[jerry:2010-01-11] remove stop
		WMRD_INF0( (TEXT("[WMRD:Start]<-><ERROR> Timeout to start , hResult(0x%x), WMResult(0x%x)"), hResult, m_hWMResult ) );
		GLOG( (ISMLOG, TEXT("[WMReader:Start]<ERROR> Start Time out, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
		return S_FALSE;
	}

	if(FAILED(m_hWMResult))
	{
		WMRD_INF0( (TEXT("[WMRD:Start]<ERROR> Receive Error, Stop Reader")) );
		hResult = m_pWMReader->Stop( );
		WMRD_INF0( (TEXT("[WMRD:Start]<-><ERROR> Receive Start Errror! hResult(0x%x), WMResult(0x%x)"), hResult, m_hWMResult ) );
		GLOG( (ISMLOG, TEXT("[WMReader:Start]<ERROR> Start Error, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
		return S_FALSE;
	}

	m_bReaderStarted = TRUE;

	//all pins get activated when the filter is started with (run->pause)
#if defined( SUPPORT_OUTPUT_PINS ) && !defined( USE_FILE_SOURCE_INTERFACE_ONLY )
	m_pVideoOut->Run();
	m_pAudioOut->Run();
	//start pusher thread. We could delay this until we actually have some data to push
	m_pVideoOut->Active();
	m_pAudioOut->Active();
#endif

	WMRD_INF0( (TEXT("[WMRD:Start]<->")) );
	return S_OK;
}

STDMETHODIMP CVidiatorWMSourceReaderFilter::Stop()
{
	//_ASSERT(m_pWMReader);
	HRESULT hResult;

	WMRD_INF0( (TEXT("[WMRD:Stop]<+>")) );

	//if(!m_pWMReader)
	if( !m_bReaderInit )
	{
		WMRD_INF0( (TEXT("[WMRD:Stop]<-><ERROR> Reader was not initialized.")) );
		return S_FALSE;
	}

	if( !m_bReaderStarted )
	{
		WMRD_INF0( (TEXT("[WMRD:Stop]<-><WARN> Reader was not started yet. ")) );
		return S_FALSE;
	}

#ifndef USE_POLLING_EVENTS
	::ResetEvent(m_pMethodHandleArray[STOP_METHOD]);
#else
	Events[STOP_METHOD]=0;
#endif
	m_bReaderStarted = FALSE;
	hResult = m_pWMReader->Stop();
	if( FAILED(hResult) )
	{
		WMRD_INF0( (TEXT("[WMRD:Stop]<-> Reader->Stop, Failure! hResult(0x%x)"), hResult) );
		GLOG( (ISMLOG, TEXT("[WMReader:Stop]<ERROR> Failt to stop WMReader, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), hResult) );
		return S_FALSE;
	}

	if(!WaitforWMStatus(STOP_METHOD, 10000))
	{
		//_ASSERT(!_T("Stop Timed out!!!"));		
		WMRD_INF0( (TEXT("[WMRD:Stop]<-><ERROR>Reader->Stop, TimeOut WMResult(0x%x)"), m_hWMResult) );
		GLOG( (ISMLOG, TEXT("[WMReader:Stop]<ERROR> Stop Time out, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
		return S_FALSE;
	}

	if(FAILED(m_hWMResult))
	{
		WMRD_INF0( (TEXT("[WMRD:Stop]<-><ERROR> Reader->Stop, Receive Error, WMResult(0x%x)"), m_hWMResult) );
		GLOG( (ISMLOG, TEXT("[WMReader:Stop]<ERROR> Stop Error, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
		return S_FALSE;
	}

#ifdef USE_FILE_SOURCE_INTERFACE_ONLY
	//also close the file
	Close();
#endif

#ifdef SUPPORT_OUTPUT_PINS
	// these are not required. _super will iterate through the PINs and stop + inactivate them
	m_pVideoOut->Stop();
	m_pAudioOut->Stop();
	m_pVideoOut->Inactive(); //shutdown pusher thread. Could do this on stop filter also
	m_pAudioOut->Inactive(); //shutdown pusher thread. Could do this on stop filter also
#endif

	WMRD_INF0( (TEXT("[WMRD:Stop]<->")) );

	hResult = __super::Stop();
	return hResult;
}

HRESULT CVidiatorWMSourceReaderFilter::Close()
{
	//_ASSERT(m_pWMReader);
	HRESULT hResult = S_OK;

	WMRD_INF0( (TEXT("[WMRD:Close]<+>")) );
	if( !m_bReaderInit )
	{
		WMRD_INF0( (TEXT("[WMRD:Close]<-><ERROR> Reader was not initialized.")) );
		return S_FALSE;
	}
	if( !m_bReaderOpened )
	{
		WMRD_INF0( (TEXT("[WMRD:Close]<-><WARN> Reader was not opened yet.")) );
		return S_OK;
	}

	if( m_bReaderStarted )
	{		
		Stop( );
	}

	WMRD_INF0( (TEXT("[WMRD:Close]<INFO> Close Source Reader.")) );
#ifndef USE_POLLING_EVENTS
	::ResetEvent(m_pMethodHandleArray[CLOSE_METHOD]);
#else
	Events[CLOSE_METHOD]=0;
#endif
	m_bReaderOpened = FALSE;
	if( m_pWMReader )
		hResult = m_pWMReader->Close();

	/////////////////////////
	//{ Fix memory leak, 2010-06-30, jerry 
	do 
	{
		if(FAILED(hResult))
		{
			WMRD_INF0( (TEXT("[WMRD:Close]<-><ERROR> Reader->Close, Failure. hResult(0x%x)"), hResult ) );
			GLOG( (ISMLOG, TEXT("[WMReader:Close]<ERROR> Fail to close WMReader, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), hResult) );			
			break;
		}

		if(!WaitforWMStatus(CLOSE_METHOD, 10000))
		{
			//_ASSERT(!_T("Close Timed out!!!"));
			WMRD_INF0( (TEXT("[WMRD:Close]<-><ERROR> Reader->Close, TimeOut. hResult(0x%x) WMResult(0x%x)"), hResult, m_hWMResult ) );
			GLOG( (ISMLOG, TEXT("[WMReader:Close]<ERROR> Close Time Out, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
			break;
		}

		if(FAILED(m_hWMResult))
		{
			WMRD_INF0( (TEXT("[WMRD:Close]<-><ERROR> Reader->Close, Receive Error. hResult(0x%x) WMResult(0x%x)"), hResult, m_hWMResult ) );
			GLOG( (ISMLOG, TEXT("[WMReader:Close]<ERROR> Close Error, Receive Error, URL(%s), Result(0x%x)"), m_strURL4Log.c_str(), m_hWMResult) );
			break;
		}
		hResult = S_OK;
	} while( FALSE );
	//} end of 2010-06-30 
	//////////////////////

	std::vector<WM_MEDIA_TYPE*>::iterator pIter;
	for( pIter = m_VecOutFormat.begin() ; pIter != m_VecOutFormat.end() ; ++pIter )
	{
		WM_MEDIA_TYPE *pMediaType = *pIter;
		if(pMediaType)
		{
			delete [] pMediaType;
			pMediaType = NULL;
		}		
	}
	m_VecOutFormat.clear();
	DoneReader( );

	WMRD_INF0( (TEXT("[WMRD:Close]<->")) );
	return hResult;
}

#ifdef SUPPORT_GETAUDIOVIDEOFORMAT_API
WM_MEDIA_TYPE* CVidiatorWMSourceReaderFilter::GetVideoFormat(void)
{
	//_ASSERT(m_outputFomratMap.size() <= 2);
	std::vector<WM_MEDIA_TYPE*>::iterator pIter;
	for(pIter = m_VecOutFormat.begin() ; pIter != m_VecOutFormat.end() ; ++pIter)
	{
		WM_MEDIA_TYPE *pMediaType = *pIter;
		if(pMediaType)
		{
			if(pMediaType->majortype == WMMEDIATYPE_Video)
			{
				return pMediaType;
			}
		}
	}
	return NULL;
}

bool CVidiatorWMSourceReaderFilter::GetVideoFormat(WMVIDEOINFOHEADER *pVideoFormat)
{
	_ASSERT(pVideoFormat);
	if(!pVideoFormat)
	{
		return false;
	}

	WM_MEDIA_TYPE *pMediaType = NULL;
	pMediaType = GetVideoFormat();
	if(pMediaType)
	{
		if(pMediaType->majortype == WMMEDIATYPE_Video)
		{
			if(pMediaType->formattype == WMFORMAT_VideoInfo)
			{
				WMVIDEOINFOHEADER *pVideoInfoHeader = reinterpret_cast<WMVIDEOINFOHEADER*>(pMediaType->pbFormat);
				_ASSERT(pVideoInfoHeader);
				if(pVideoInfoHeader)
				{
					pVideoFormat->bmiHeader.biWidth = pVideoInfoHeader->bmiHeader.biWidth;
					pVideoFormat->bmiHeader.biHeight = pVideoInfoHeader->bmiHeader.biHeight;
					if(static_cast<int>(pVideoInfoHeader->AvgTimePerFrame) > 0)
					{
//						pVideoFormat->nFrameRate = (int)(10000000000 / static_cast<int>(pVideoInfoHeader->AvgTimePerFrame));	
						pVideoFormat->AvgTimePerFrame = pVideoInfoHeader->AvgTimePerFrame;	
					}
					else
					{
//						pVideoFormat->nFrameRate = 0;
						pVideoFormat->AvgTimePerFrame = 0;
					}
				}
			}
			else if(pMediaType->formattype == WMFORMAT_MPEG2Video)
			{
				WMMPEG2VIDEOINFO *pMpeg2VideoInfo = reinterpret_cast<WMMPEG2VIDEOINFO*>(pMediaType->pbFormat);
				_ASSERT(pMpeg2VideoInfo);
				if(pMpeg2VideoInfo)
				{
					WMVIDEOINFOHEADER2 *pVideoInfoHeader2 = reinterpret_cast<WMVIDEOINFOHEADER2*>(&pMpeg2VideoInfo->hdr);
					_ASSERT(pVideoInfoHeader2);
					if(pVideoInfoHeader2)
					{
						pVideoFormat->bmiHeader.biWidth = pVideoInfoHeader2->bmiHeader.biWidth;
						pVideoFormat->bmiHeader.biHeight = pVideoInfoHeader2->bmiHeader.biHeight;
						if(static_cast<int>(pVideoInfoHeader2->AvgTimePerFrame) > 0)
						{
//							pVideoFormat->nFrameRate = (int)(10000000000 / static_cast<int>(pVideoInfoHeader2->AvgTimePerFrame));	
							pVideoFormat->AvgTimePerFrame = pVideoInfoHeader2->AvgTimePerFrame;	
						}
						else
						{
//							pVideoFormat->nFrameRate = 0;
							pVideoFormat->AvgTimePerFrame = 0;	
						}
					}
				}
			}
			else
			{
				_ASSERT(!_T("Not expected video format type!!!"));
			}
		}		
		else
		{
			_ASSERT(!_T("Not expected media type!!!"));
		}
	}

	return true;
}
WM_MEDIA_TYPE* CVidiatorWMSourceReaderFilter::GetAudioFormat(void)
{
	//_ASSERT(m_outputFomratMap.size() <= 2);
	std::vector<WM_MEDIA_TYPE*>::iterator pIter;
	for(pIter = m_VecOutFormat.begin() ; pIter != m_VecOutFormat.end() ; ++pIter)
	{
		WM_MEDIA_TYPE *pMediaType = *pIter;
		if(pMediaType)
		{
			if(pMediaType->majortype == WMMEDIATYPE_Audio)
			{
				return pMediaType;
			}
		}
	}

	return NULL;
}

bool CVidiatorWMSourceReaderFilter::GetAudioFormat(WAVEFORMATEX *pAudioFormat)
{
	if(!pAudioFormat)
	{
		_ASSERT(pAudioFormat);
		return false;
	}

	WM_MEDIA_TYPE *pMediaType = NULL;
	pMediaType = GetAudioFormat();
	if(pMediaType)
	{
		if(pMediaType->majortype == WMMEDIATYPE_Audio)
		{
			if(pMediaType->formattype == WMFORMAT_WaveFormatEx)
			{
				WAVEFORMATEX *pWaveFormatEx = reinterpret_cast<WAVEFORMATEX*>(pMediaType->pbFormat);
				_ASSERT(pWaveFormatEx);
				if(pWaveFormatEx)
				{
					pAudioFormat->nSamplesPerSec = static_cast<int>(pWaveFormatEx->nSamplesPerSec);
					pAudioFormat->nChannels = static_cast<int>(pWaveFormatEx->nChannels);
					pAudioFormat->wBitsPerSample = static_cast<int>(pWaveFormatEx->wBitsPerSample);	
				}
			}
			else
			{
				_ASSERT(!_T("Not expected audio format type!!!"));
			}
		}
		else
		{
			_ASSERT(!_T("Not expected media type!!!"));
		}
	}

	return true;
}
#endif
#ifdef SUPPORT_CALLBACKS
STDMETHODIMP CVidiatorWMSourceReaderFilter::SetSampleCallback(ISampleCallback *pSampleCallback)
{	
	if(!pSampleCallback)
	{
		_ASSERT(pSampleCallback);
		return false;
	}

	m_pSampleCallback = pSampleCallback;

	return true;
}

STDMETHODIMP CVidiatorWMSourceReaderFilter::SetSampleCallback(BYTE *pSampleCallback)
{	
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//avoid forcasting!!!!!!!!!!!!!!!!
	//this is just for temp test !!!!!
	return SetSampleCallback( (ISampleCallback *)pSampleCallback );
}

STDMETHODIMP CVidiatorWMSourceReaderFilter::SetEventCallback(IEventCallback *pEventCallback)
{
	if(!pEventCallback)
	{
		_ASSERT(pEventCallback);
		return false;
	}

	m_pEventCallback = pEventCallback;

	return true;
}
#endif
HRESULT CVidiatorWMSourceReaderFilter::OnSample(DWORD dwOutputNum, QWORD cnsSampleTime, QWORD cnsSampleDuration, DWORD dwFlags, INSSBuffer *pSample, void *pvContext)
{
	std::vector<WM_MEDIA_TYPE*>::iterator pIter;
	WM_MEDIA_TYPE *pOutputMediaType = NULL;

	//media format changed since our last init ? Maybe we should try to reinitialize
	if( dwOutputNum >= m_VecOutFormat.size() )
	{
		return S_OK;
	}
	pIter = m_VecOutFormat.begin() + dwOutputNum;
	pOutputMediaType = *pIter;
	
	if(!pOutputMediaType)
	{
		return S_OK;
	}

	LONGLONG llTimeStamp = cnsSampleTime / 10000; // from 100nano to 1ms
	LONGLONG llSendTS = 0;
	BYTE * pData = NULL;
	BYTE * pDataGot = NULL;
	DWORD dwDataLen = 0;
	
	pSample->AddRef(); //we are working with this, pls do not touch it
	HRESULT hr = pSample->GetBufferAndLength(&pDataGot, &dwDataLen);	

	if( !pDataGot || (dwDataLen <= 0) )
	{
		_ASSERT(pDataGot);
		_ASSERT(dwDataLen > 0);
		pSample->Release();
		return S_FALSE;
	}

	if( (pOutputMediaType->majortype == WMMEDIATYPE_Video) || (pOutputMediaType->majortype == WMFORMAT_MPEG2Video) )
	{
		EnterCriticalSection(&m_CSIYUVBuff);
		switch(m_eInputSubtype)
		{
			case RGB24:
				if(m_pIYUVTempBuff)
				{
					NEW_DIB_RGB24_to_IYUV(m_pIYUVTempBuff,pDataGot,m_dwSrcWidth,m_dwSrcHeight);
					pData = m_pIYUVTempBuff;
				}
				break; 
			case YV12:
				if(m_pIYUVTempBuff)
				{
					memcpy(m_pIYUVTempBuff, pDataGot, m_dwSrcWidth*m_dwSrcHeight );
					memcpy(m_pIYUVTempBuff + m_dwSrcWidth*m_dwSrcHeight,pDataGot + m_dwSrcWidth*m_dwSrcHeight*5/4,m_dwSrcWidth*m_dwSrcHeight/4);
					memcpy(m_pIYUVTempBuff + m_dwSrcWidth*m_dwSrcHeight*5/4,pDataGot + m_dwSrcWidth*m_dwSrcHeight,m_dwSrcWidth*m_dwSrcHeight/4);

					pData = m_pIYUVTempBuff;
				}
				break;
			case IYUV:
				pData = pDataGot;
				break;
			default:
				pData = NULL;	//unhandled input type, we will not deliver this buffer
				break;
		}

		if( m_VideoLastTS == TIME_Unknown )
		{
			WMRD_INF0( (TEXT("[WMRD:OnSample] Video First PTS(%I64d)"),llTimeStamp) );
		}

		if( m_VideoLastTS != TIME_Unknown && llTimeStamp > m_VideoLastTS )
		{
			//m_VideoAvgDur = (m_VideoLastTS - llTimeStamp);
			m_VideoAvgDur = (llTimeStamp - m_VideoLastTS);			
		}
		llSendTS  = m_StartTimeOffset + llTimeStamp;
		m_VideoLastTS = llTimeStamp;

		if( pData )
		{
#ifdef SUPPORT_CALLBACKS
			if(m_pSampleCallback)
			{
				m_pSampleCallback->OnSample(0, llSendTS, static_cast<unsigned char*>(pData), dwDataLen, SOURCE_MEDIATYPE_VIDEO);
			}		
#endif
#ifdef SUPPORT_OUTPUT_PINS
			MEDIAPacket Packet;
			Packet.Data = pData;
			Packet.DataLen = Packet.Allocated = dwDataLen;
			Packet.Type = MEDIA_Video;
			Packet.DTS = m_VideoAvgDur;
			Packet.Start = llSendTS;
			Packet.End = llSendTS + m_VideoAvgDur;
			m_pVideoOut->QueueDataToPush( &Packet );
#endif
		}
		LeaveCriticalSection(&m_CSIYUVBuff);
	}
	else if(pOutputMediaType->majortype == WMMEDIATYPE_Audio)
	{	
		if( m_AudioLastTS == TIME_Unknown )
		{
			WMRD_INF0( (TEXT("[WMRD:OnSample] Audio First PTS(%I64d)"),llTimeStamp) );
		}
#ifdef SUPPORT_GETAUDIOFORMAT_API
		if( m_nAudioByteRate > 0 )
			m_AudioLastDur = dwDataLen*1000/m_nAudioByteRate;
		else
#endif
			if( m_AudioLastTS != TIME_Unknown && llTimeStamp > m_AudioLastTS )
				m_AudioLastDur = llTimeStamp - m_AudioLastTS;

		llSendTS  = m_StartTimeOffset + llTimeStamp;
		m_AudioLastTS = llTimeStamp;
	
#ifdef SUPPORT_CALLBACKS
		if(m_pSampleCallback)
			m_pSampleCallback->OnSample(0, llSendTS, static_cast<unsigned char*>(pDataGot), dwDataLen, SOURCE_MEDIATYPE_AUDIO);
#endif
#ifdef SUPPORT_OUTPUT_PINS
		MEDIAPacket Packet;
		Packet.Data = pDataGot;
		Packet.DataLen = Packet.Allocated = dwDataLen;
		Packet.Type = MEDIA_Audio;
		Packet.DTS = m_AudioLastDur;
		Packet.Start = llSendTS;
		Packet.End = llSendTS + m_AudioLastDur;
		m_pAudioOut->QueueDataToPush( &Packet );
#endif
	}

	pSample->Release();
	return S_OK;
}

//function call is not async. We can stop the filter on error
HRESULT CVidiatorWMSourceReaderFilter::OnStatus(WMT_STATUS Status, HRESULT hr, WMT_ATTR_DATATYPE dwType, BYTE *pValue, void *pvContext)
{
	m_wmStatus = Status;
	m_hWMResult = hr;

	switch(Status)
	{
	case WMT_ERROR:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_ERROR hr(%d)"),hr) );
		//if(m_pEventCallback) //remove it because of wmv hang, 2010-02-17, jerry 
		//{
		//	m_pEventCallback->OnEvent(ISM_SOURCE_ERROR, NULL, NULL, (void*)this);
		//}
		break;
	case WMT_OPENED:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_OPENED hr(%d)"),hr) );
		SaveMediaFormat( );
#ifndef USE_POLLING_EVENTS
		::SetEvent(m_pMethodHandleArray[OPEN_METHOD]);
#else
		Events[OPEN_METHOD]=1;
#endif
		break;
	case WMT_BUFFERING_START:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_BUFFERING_START hr(%d)"),hr) );
		break;
	case WMT_BUFFERING_STOP:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_BUFFERING_STOP hr(%d)"),hr) );
		break;
	case WMT_END_OF_FILE:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_END_OF_FILE hr(%d)"),hr) );
		break;
	case WMT_END_OF_SEGMENT:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_END_OF_SEGMENT hr(%d)"),hr) );
		break;
	case WMT_END_OF_STREAMING:		
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_END_OF_STREAMING hr(%d)"),hr) );
		//if(m_bLoop)
		//{
		//	hr2 = m_pWMReader->Start(0, 0, 1.0, (void*)this);
		//	_ASSERT(SUCCEEDED(hr2));
		//}
		break;
	case WMT_CONNECTING:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_CONNECTING hr(%d)"),hr) );
		break;
	case WMT_NO_RIGHTS:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_NO_RIGHTS hr(%d)"),hr) );
		break;
	case WMT_MISSING_CODEC:
		break;
	case WMT_STARTED:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_SOURCE_STARTED hr(%d)"),hr) );
#ifndef USE_POLLING_EVENTS
		::SetEvent(m_pMethodHandleArray[START_METHOD]);
#else
		Events[START_METHOD]=1;
#endif
		break;
	case WMT_STOPPED:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_SOURCE_STOPPED hr(%d)"),hr) );
#ifndef USE_POLLING_EVENTS
		::SetEvent(m_pMethodHandleArray[STOP_METHOD]);
#else
		Events[STOP_METHOD]=1;
#endif
		break;
	case WMT_CLOSED:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_SOURCE_CLOSED hr(%d)"),hr) );
#ifndef USE_POLLING_EVENTS
		::SetEvent(m_pMethodHandleArray[CLOSE_METHOD]);
#else
		Events[CLOSE_METHOD]=1;
#endif
		break;
	case WMT_STRIDING:
		break;
	case WMT_TIMER:
		break;
	case WMT_SOURCE_SWITCH:
		WMRD_INF0( (TEXT("[WMRD:OnStatus] WMT_SOURCE_SWITCH hr(%d)"),hr) );
#ifdef SIGNAL_UNINISHED_MEDIA_SWITCH_SUPPORT
		_ASSERT( false );
#else
		//!!!!!!!!!!!! did not test this
		//i can imagine we should disconnect pins, set new media types, reinitialize allocators, .......start pins
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		SetYUVOutputProp();
		SetPCMOutputProp();
		SaveMediaFormat(); // ISM_SOURCE_SWITCH event will be sent if the media format had been changed in this method.
		ResetTimeStamp();
		hr2 = m_pWMReader->Start(0, 0, 1.0, (void*)this);
		_ASSERT(SUCCEEDED(hr2));
#endif
		break;
	}

	return S_OK;
}
//called on file open or file source switch
bool CVidiatorWMSourceReaderFilter::SaveMediaFormat(void)
{

	WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<+>")) );
	//_ASSERT(m_pWMReader);
	//if(!m_pWMReader)
	if( !m_bReaderInit )
	{
		WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<-><ERROR> Reader was not initialized.")) );
		return false;
	}

	DWORD dwOutputNum = 0;	
	HRESULT hResult = m_pWMReader->GetOutputCount( &dwOutputNum );
	if(FAILED(hResult) || (dwOutputNum <= 0))
	{
		WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<-><ERROR> hResult(0x%x), Output Num(%d)"), hResult, dwOutputNum ) );
		return false;
	}

	std::vector<WM_MEDIA_TYPE*> VecNewMediaType;

	int No;	
	for( No = 0 ; No < (int)dwOutputNum ; ++No )
	{
		IWMOutputMediaProps *pOutputMediaProps = NULL;
		hResult = m_pWMReader->GetOutputProps(No, &pOutputMediaProps);
		if( FAILED(hResult) )
		{
			WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<ERROR> Fail to get ouput properties. hResult(0x%x)"), hResult ) );
			SAFE_RELEASE( pOutputMediaProps ); //memory-leak? 2010-02-11, jerry 			
			continue;
		}

		DWORD dwMediaTypeSize = 0;
		hResult = pOutputMediaProps->GetMediaType(NULL, &dwMediaTypeSize);
		if(FAILED(hResult) || (dwMediaTypeSize <= 0))
		{
			WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<ERROR> Fail to get mediatype hResult(0x%x), dwMediaTypeSize(%d)"), hResult, dwMediaTypeSize ) );
			SAFE_RELEASE( pOutputMediaProps );
			continue;
		}

		// Realloc for the new media type
		WM_MEDIA_TYPE *pOutputMediaType = NULL;
		pOutputMediaType = (WM_MEDIA_TYPE*)new BYTE[dwMediaTypeSize];
		_ASSERT(pOutputMediaType);
		if(!pOutputMediaType)
		{
			WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<ERROR> Fail to allocate mediatype"), hResult, dwMediaTypeSize ) );
			SAFE_RELEASE( pOutputMediaProps );
			SAFE_DELETE_ARRAY( pOutputMediaType );
			continue;
		}

		hResult = pOutputMediaProps->GetMediaType(pOutputMediaType, &dwMediaTypeSize);
		if(FAILED(hResult) || (dwMediaTypeSize <= 0))
		{

			SAFE_DELETE_ARRAY( pOutputMediaType );
			SAFE_RELEASE( pOutputMediaProps );
			WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<ERROR> Fail to get mediatype"), hResult, dwMediaTypeSize ) );
			continue;
		}

		VecNewMediaType.push_back( pOutputMediaType );
		SAFE_RELEASE( pOutputMediaProps );
	}

	BOOL IsFormatChanged = FALSE;

	if( m_VecOutFormat.size() > 0 )
	{
		if( m_VecOutFormat.size() != VecNewMediaType.size() )
		{
			WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<WARN> Number of Ouput is differnet, Cur(%d), New(%d) "), m_VecOutFormat.size(), VecNewMediaType.size() ) );
			GLOG( (ISMLOG, TEXT("[WMReader:SaveMediaFormat]<WARN> URL(%s)/Number of Ouput is differnet, Cur(%d), New(%d) "), m_strURL4Log.c_str(), m_VecOutFormat.size(), VecNewMediaType.size() ) );		
		}

		BOOL VideoUpdated = FALSE;
		BOOL AudioUpdated = FALSE;
		No = 0;

		std::vector<WM_MEDIA_TYPE*>::iterator pCurTypeIter;
		for( pCurTypeIter = m_VecOutFormat.begin(); pCurTypeIter != m_VecOutFormat.end(); ++pCurTypeIter, ++No )
		{			
			std::vector<WM_MEDIA_TYPE*>::iterator pNewTypeIter;
			WM_MEDIA_TYPE * pCurType = *pCurTypeIter;
			WM_MEDIA_TYPE * pNewType = NULL;

			WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat] Stream Compare No(%d)"), No) );

			if( pCurType && pCurType->majortype == WMMEDIATYPE_Video && !VideoUpdated )
			{	
				WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat] Stream Compare Video(%d)"), No) );
				pNewType = NULL;
				for( pNewTypeIter = VecNewMediaType.begin(); pNewTypeIter != VecNewMediaType.end(); ++pNewTypeIter )
				{
					pNewType  = *pNewTypeIter;
					if( pNewType && pNewType->majortype == WMMEDIATYPE_Video )
					{
						break;
					}
				}
				if( pNewType )
				{
					WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat] Find New Type Video(%d)"), No) );
					if( !CompareMediaType( pCurType , pNewType ) )
					{
						IsFormatChanged = TRUE;
					}						
				}
				else
				{
					WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<ERROR> ew stream have no video stream!!!!! ")) );
					GLOG( (ISMLOG, TEXT("[WMReader:SaveMediaFormat]<ERROR> URL(%s)/New stream have no video stream!!!!! "), m_strURL4Log.c_str()) );
				}
				VideoUpdated = TRUE;
			}
			else
			if( pCurType && pCurType->majortype == WMMEDIATYPE_Audio && !AudioUpdated )
			{
				WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat] Stream Compare Audio(%d)"), No) );
				pNewType = NULL;
				for( pNewTypeIter = VecNewMediaType.begin(); pNewTypeIter != VecNewMediaType.end(); ++pNewTypeIter )
				{
					pNewType  = *pNewTypeIter;
					if( pNewType && pNewType->majortype == WMMEDIATYPE_Audio )
					{
						break;
					}
				}

				if( pNewType )
				{
					WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat] Find New Type Audio(%d)"), No) );
					if( !CompareMediaType( pCurType , pNewType ) )
					{
						IsFormatChanged = TRUE;
					}
				}
				else
				{
					WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<ERROR> New stream have no audio stream!!!!! ")) );
					GLOG( (ISMLOG, TEXT("[WMReader:SaveMediaFormat]<ERROR> URL(%s)/New stream have no audio stream!!!!! "), m_strURL4Log.c_str()) );
				}				
				AudioUpdated = TRUE;
			}
			else
			{
				WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat] Stream Compare Unkonwn(%d)"), No, pCurType) );
			}
		}

	}//if( m_VecOutFormat.size() > 0 )
	
	std::vector<WM_MEDIA_TYPE*>::iterator pTypeIter;
	for( pTypeIter = m_VecOutFormat.begin(); pTypeIter != m_VecOutFormat.end(); ++pTypeIter )
	{
		WM_MEDIA_TYPE * pType = *pTypeIter;
		if( pType )
		{
			delete [] pType;
		}
	}
	m_VecOutFormat.clear();

	for( pTypeIter = VecNewMediaType.begin(); pTypeIter != VecNewMediaType.end(); ++pTypeIter )
	{
		m_VecOutFormat.push_back( *pTypeIter );
	}
	VecNewMediaType.clear();

#ifdef SUPPORT_CALLBACKS
	if( IsFormatChanged )
		m_pEventCallback->OnEvent(ISM_SOURCE_SWITCH, NULL, NULL, (void*)this);
#endif

#if defined( DMSG_ON_WMReader )
	PrintOutputStreams();
#endif

	WMRD_INF0( (TEXT("[WMRD:SaveMediaFormat]<->")) );

	return true;
}

bool CVidiatorWMSourceReaderFilter::WaitforWMStatus(SYNC_METHOD_NAMES methodName, DWORD dwTimeout)
{
	// Wait until the delivered status are same with the requested status
#ifndef USE_POLLING_EVENTS
	if(::WaitForSingleObject(m_pMethodHandleArray[methodName], dwTimeout) == WAIT_TIMEOUT)
	{
		return false;
	}
#else
	LONG start = GetTickCount();
	while( Events[methodName]==0 )
	{
		Sleep( 10 );
		LONG now = GetTickCount();
		if( now - start > WAIT_TIMEOUT )
			return false;
	}
#endif
	return true;
}

bool CVidiatorWMSourceReaderFilter::CompareMediaType(WM_MEDIA_TYPE *pType1, WM_MEDIA_TYPE *pType2)
{
	_ASSERT(pType1 && pType2);
	if(!pType1 || !pType2)
	{
		return false;
	}

	if(pType1->majortype == pType2->majortype)
	{
		if(pType1->formattype == pType2->formattype)
		{
			if(pType1->formattype == WMFORMAT_VideoInfo)
			{
				WMVIDEOINFOHEADER *pVideoInfoHeader1 = reinterpret_cast<WMVIDEOINFOHEADER*>(pType1->pbFormat);
				WMVIDEOINFOHEADER *pVideoInfoHeader2 = reinterpret_cast<WMVIDEOINFOHEADER*>(pType2->pbFormat);
			
				if((pVideoInfoHeader1->bmiHeader.biWidth == pVideoInfoHeader2->bmiHeader.biWidth) &&
					(pVideoInfoHeader1->bmiHeader.biHeight == pVideoInfoHeader2->bmiHeader.biHeight))
				{
					return true;
				}
			}
			else if(pType1->formattype == WMFORMAT_MPEG2Video)
			{
				WMMPEG2VIDEOINFO *pMpeg2VideoInfo1 = reinterpret_cast<WMMPEG2VIDEOINFO*>(pType1->pbFormat);
				WMVIDEOINFOHEADER2 *pVideoInfoHeader1 = reinterpret_cast<WMVIDEOINFOHEADER2*>(&pMpeg2VideoInfo1->hdr);
				WMMPEG2VIDEOINFO *pMpeg2VideoInfo2 = reinterpret_cast<WMMPEG2VIDEOINFO*>(pType2->pbFormat);
				WMVIDEOINFOHEADER2 *pVideoInfoHeader2 = reinterpret_cast<WMVIDEOINFOHEADER2*>(&pMpeg2VideoInfo2->hdr);

				if((pVideoInfoHeader1->bmiHeader.biWidth == pVideoInfoHeader2->bmiHeader.biWidth) &&
					(pVideoInfoHeader1->bmiHeader.biHeight == pVideoInfoHeader2->bmiHeader.biHeight))
				{
					return true;
				}
			}
			else if(pType1->formattype == WMFORMAT_WaveFormatEx)
			{
				WAVEFORMATEX *pWaveFormatEx1 = reinterpret_cast<WAVEFORMATEX*>(pType1->pbFormat);
				WAVEFORMATEX *pWaveFormatEx2 = reinterpret_cast<WAVEFORMATEX*>(pType2->pbFormat);

				if((pWaveFormatEx1->nSamplesPerSec == pWaveFormatEx2->nSamplesPerSec) &&
					(pWaveFormatEx1->nChannels == pWaveFormatEx2->nChannels) &&
					(pWaveFormatEx1->wBitsPerSample == pWaveFormatEx2->wBitsPerSample))
				{
					return true;
				}
			}
		}
	}		

	return false;
}

#ifdef DMSG_ON_WMReader
void CVidiatorWMSourceReaderFilter::PrintOutputStreams( )
{
	int No = 0; 
	WM_MEDIA_TYPE * pMediaType = NULL; 
	std::vector<WM_MEDIA_TYPE*>::iterator pIter;

	for( pIter = m_VecOutFormat.begin(); pIter < m_VecOutFormat.end(); ++pIter, ++No )
	{
		TCHAR szBuf[LOCAL_STRING_BUFFER_SIZE];

		pMediaType = *pIter;
		if( !pMediaType )
		{
			continue;
		}

		if(pMediaType->majortype == WMMEDIATYPE_Video)
		{
			if(pMediaType->formattype == WMFORMAT_VideoInfo)
			{
				WMVIDEOINFOHEADER *pVideoInfoHeader = reinterpret_cast<WMVIDEOINFOHEADER*>(pMediaType->pbFormat);
				_ASSERT(pVideoInfoHeader);
				if(!pVideoInfoHeader)
				{
					continue;
				}

				_stprintf_s(szBuf, LOCAL_STRING_BUFFER_SIZE, _T("Video[%d] width: %d height: %d AvgTimePerFrame: %I64d"), No, pVideoInfoHeader->bmiHeader.biWidth, 
					pVideoInfoHeader->bmiHeader.biHeight,
					pVideoInfoHeader->AvgTimePerFrame);
			}
			else if(pMediaType->formattype == WMFORMAT_MPEG2Video)
			{
				WMMPEG2VIDEOINFO *pMpeg2VideoInfo = reinterpret_cast<WMMPEG2VIDEOINFO*>(pMediaType->pbFormat);
				_ASSERT(pMpeg2VideoInfo);
				if(!pMpeg2VideoInfo)
				{
					continue;
				}

				WMVIDEOINFOHEADER2 *pVideoInfoHeader2 = reinterpret_cast<WMVIDEOINFOHEADER2*>(&pMpeg2VideoInfo->hdr);
				_ASSERT(pVideoInfoHeader2);
				if(!pVideoInfoHeader2)
				{
					continue;
				}

				_stprintf_s(szBuf, LOCAL_STRING_BUFFER_SIZE, _T("Video[%d] width: %d height: %d AvgTimePerFrame: %I64d"), No, pVideoInfoHeader2->bmiHeader.biWidth, 
					pVideoInfoHeader2->bmiHeader.biHeight,
					pVideoInfoHeader2->AvgTimePerFrame);	
			}
			else
			{
				_ASSERT(!_T("Not expected video format type!!!"));
			}
		}		
		else if(pMediaType->majortype == WMMEDIATYPE_Audio)
		{
			if(pMediaType->formattype == WMFORMAT_WaveFormatEx)
			{
				WAVEFORMATEX *pWaveFormatEx = reinterpret_cast<WAVEFORMATEX*>(pMediaType->pbFormat);
				_ASSERT(pWaveFormatEx);
				if(!pWaveFormatEx)
				{
					continue;
				}

				_stprintf_s(szBuf, LOCAL_STRING_BUFFER_SIZE, _T("Audio[%d] Samplingrate: %d ChannelCnt: %d Bitrate: %d"), No, pWaveFormatEx->nSamplesPerSec, 
					pWaveFormatEx->nChannels,
					pWaveFormatEx->wBitsPerSample);
			}
			else
			{
				_ASSERT(!_T("Not expected audio format type!!!"));
			}
		}
		else
		{
			_ASSERT(!_T("Not expected media type!!!"));
			WMRD_INF0( (TEXT("[WMRD:Info] Not expected media type!!! ")) );
		}
		//OutputDebugString(szBuf);
		WMRD_INF0( (TEXT("[WMRD:Info] %s"), szBuf) );
	}	
}
#endif

STDMETHODIMP CVidiatorWMSourceReaderFilter::Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt)
{
	m_pFileName = LPOLESTR( pwszFileName );
	HRESULT hr = Open( LPOLESTR( pwszFileName ) );
#ifdef USE_FILE_SOURCE_INTERFACE_ONLY
	Start();
#endif
	return hr;
}

STDMETHODIMP CVidiatorWMSourceReaderFilter::GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
	CheckPointer(ppszFileName,E_POINTER);
    *ppszFileName = NULL;

    if (m_pFileName != NULL) 
    {
        size_t len = 1+lstrlenW(m_pFileName);
        *ppszFileName = (LPOLESTR)
        QzTaskMemAlloc(sizeof(WCHAR) * (len));

        if (*ppszFileName != NULL) 
        {
            HRESULT hr = StringCchCopyW(*ppszFileName, len, m_pFileName);
        }
    }

    if(pmt) 
    {
        ZeroMemory(pmt, sizeof(*pmt));
        pmt->majortype = MEDIATYPE_NULL;
        pmt->subtype = MEDIASUBTYPE_NULL;
		//what if user wanted to get audio media type ?
		if( m_bReaderOpened )
		{
			CMediaType mt;
			m_pVideoOut->GetMediaType( &mt );
			pmt->majortype = mt.majortype;
			pmt->subtype = mt.subtype;
			pmt->bFixedSizeSamples = mt.bFixedSizeSamples;
			pmt->bTemporalCompression = mt.bTemporalCompression;
			pmt->lSampleSize = mt.lSampleSize;
		}
    }

    return S_OK;	return S_OK;
}
