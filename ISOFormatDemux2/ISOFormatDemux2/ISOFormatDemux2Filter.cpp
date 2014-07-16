#include "StdAfx.h"
#include "ISOFormatDemux2Filter.h"
#include "MediaTypeUtil.h"
#include "PushPins.h"
#include <process.h>

//!! copy pasted this from MPEG4Demux filter project !!
// {08E22ADA-B715-45ed-9D20-7B87750301D4}
DEFINE_GUID(MEDIASUBTYPE_MP4,
			0x8e22ada, 0xb715, 0x45ed, 0x9d, 0x20, 0x7b, 0x87, 0x75, 0x3, 0x1, 0xd4);

#define WMRD_INF0(a)	1//
//disable debugging for now. Will implement later
#ifdef GLOG
	#undef GLOG
	#define GLOG(a)			1//
#else
	#define GLOG(a)			1//
#endif
#define ISMLOG			1//

const AMOVIESETUP_MEDIATYPE m_sudType[] =
{
    {
        &MEDIATYPE_Video,
        &MEDIASUBTYPE_NULL      // wild card
    },
    {
        &MEDIATYPE_Audio,
        &MEDIASUBTYPE_NULL
    },
};

// registration of our pins for auto connect and render operations
const AMOVIESETUP_PIN m_sudPin[] =
{
    {
        L"Video",          // pin name
        FALSE,              // is rendered?
        TRUE,               // is output?
        FALSE,              // zero instances allowed?
        FALSE,              // many instances allowed?
        &CLSID_NULL,        // connects to filter (for bridge pins)
        NULL,               // connects to pin (for bridge pins)
        1,                  // count of registered media types
        &m_sudType[0]       // list of registered media types
    },
    {
        L"Audio",          // pin name
        FALSE,              // is rendered?
        TRUE,               // is output?
        FALSE,              // zero instances allowed?
        FALSE,              // many instances allowed?
        &CLSID_NULL,        // connects to filter (for bridge pins)
        NULL,               // connects to pin (for bridge pins)
        1,                  // count of registered media types
        &m_sudType[1]       // list of registered media types
    },
};

// Setup data
const AMOVIESETUP_FILTER sudFilterInfo =
{
    &CLSID_ISOFormatDemux2,					// CLSID of filter
    L"Vidiator ISO Format Demux 2",			// Filter's name
    MERIT_DO_NOT_USE,						// Filter merit
    2,
    m_sudPin
};

template <class T> static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
    CUnknown* punk = new T(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

// List of class IDs and creator functions for class factory
CFactoryTemplate g_Templates []  = {
    { L"VidiatorISOFormatDemuxFilter"
    , &CLSID_ISOFormatDemux2
    , CreateInstance<CVidiatorISOFormatDemuxFilter>
    , NULL
    , &sudFilterInfo }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
 
//Methods to get Frame size from audio bitstream
bool get_samr_frame_size(unsigned char buf_8, int *pSampleSize)
{
	bool ret	= true;
	if (!pSampleSize)
		return false;

	switch ((buf_8>>3)&0x0F) {
	case 0:	(*pSampleSize)	= 12;	break;	// 4750
	case 1:	(*pSampleSize)	= 13;	break;	// 5150
	case 2:	(*pSampleSize)	= 15;	break;	// 5900
	case 3:	(*pSampleSize)	= 17;	break;	// 6700
	case 4:	(*pSampleSize)	= 19;	break;	// 7400
	case 5:	(*pSampleSize)	= 20;	break;	// 7950
	case 6:	(*pSampleSize)	= 26;	break;	// 10200
	case 7:	(*pSampleSize)	= 31;	break;	// 12200
	case 8:	(*pSampleSize)	= 5;	break;	// SID (comfort noise generation)
	case 15:	// silence frame
	default:
		(*pSampleSize)	= 0;	break;
	}
	return ret;
}

bool get_sawb_frame_size(unsigned char buf_8, int *pSampleSize)
{
	bool ret	= true;
	if (!pSampleSize)
		return false;

	switch ((buf_8>>3)&0x0F) {
	case 0:	(*pSampleSize)	= 17;	break;	// 6600
	case 1:	(*pSampleSize)	= 23;	break;	// 8850
	case 2:	(*pSampleSize)	= 32;	break;	// 12650
	case 3:	(*pSampleSize)	= 36;	break;	// 14250
	case 4:	(*pSampleSize)	= 40;	break;	// 15850
	case 5:	(*pSampleSize)	= 46;	break;	// 18250
	case 6:	(*pSampleSize)	= 50;	break;	// 19850
	case 7:	(*pSampleSize)	= 58;	break;	// 23050
	case 8:	(*pSampleSize)	= 60;	break;	// 23850
	case 9:	(*pSampleSize)	= 5;	break;	// SID (comfort noise generation)
	case 14:	// speech lost
	case 15:	// no data (no transmission / no reception)
	default:
		(*pSampleSize)	= 0;	break;
	}
	return ret;
}

bool get_sevc_frame_size(unsigned char buf_8, int *pSampleSize)
{
	bool ret	= true;
	if (!pSampleSize)
		return false;

	switch (buf_8) {
	case 1:	(*pSampleSize)	= 2;	break;	// rate : 1/8
	case 3:	(*pSampleSize)	= 10;	break;	// rate : 1/2
	case 4:	(*pSampleSize)	= 22;	break;	// rate : 1
	case 0:		// blank
	case 14:	//
	default:
		(*pSampleSize)	= 0;	break;
	}
	return ret;
}

bool get_sqcp_frame_size(unsigned char buf_8, int *pSampleSize)
{
	bool ret	= true;
	if (!pSampleSize)
		return false;

	switch (buf_8) {
	case 1:	(*pSampleSize)	= 3;	break;	// rate : 1/8
	case 2:	(*pSampleSize)	= 7;	break;	// rate : 1/4
	case 3:	(*pSampleSize)	= 16;	break;	// rate : 1/2
	case 4:	(*pSampleSize)	= 34;	break;	// rate : 1
	case 0:		// blank
	case 14:	// erasure
	default:
		(*pSampleSize)	= 0;	break;
	}
	return ret;
}

DWORD WINAPI CVidiatorISOFormatDemuxFilter::ReadFileThread( LPVOID pObj )
{
	CVidiatorISOFormatDemuxFilter *pReader = reinterpret_cast<CVidiatorISOFormatDemuxFilter*>(pObj);

	bool bInterrupted = false;
	int nVideoTrackCount = pReader->GetVideoTrackCount();
	int nAudioTrackCount = pReader->GetAudioTrackCount();
	int nHandleCount = nVideoTrackCount + nAudioTrackCount;
	while( 1 )
	{
		Sleep( 1 ); //avoid killing CPU if output is full
		if(((nVideoTrackCount > 0) && pReader->m_bVideoInturrupted) || ((nAudioTrackCount > 0) && pReader->m_bAudioInturrupted))
		{
			// Interrupted
			bInterrupted = true;
			break;
		}

		//do not read from file until filter gets started
		if( pReader->IsActive() == FALSE )
		{
			Sleep( 2 );
			continue;
		}

		//check if we still have data to read
		BOOL HaveDataToRead = FALSE;
		for(int i = 0 ; i < nVideoTrackCount ; ++i)
			HaveDataToRead |= pReader->m_hVideoTracksPendingEOF[i];
		for(int i = 0 ; i < nAudioTrackCount ; ++i)
			HaveDataToRead |= pReader->m_hAudioTracksPendingEOF[i];

		//we are done here
		if( HaveDataToRead == FALSE )
		{
			break;
		}

#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
		//starve video send queue until audio catches up with video. Most of the time more audio is sent then video packets
		BOOL IsAudioBehindVideoDuration = FALSE;
		for(int i=0;i<nVideoTrackCount;i++)
			for(int j=i;j<nAudioTrackCount;j++)
				if( pReader->m_nVideoDurationPushed[i] > pReader->m_nAudioDurationPushed[j] )
				{
					IsAudioBehindVideoDuration = TRUE;
					break;
				}
		//starve audio send queue until video rendering catches up. This will rarely happen since audio has a faster send rate
		BOOL IsVideoBehindAudioDuration = FALSE;
		for(int i=0;i<nVideoTrackCount;i++)
			for(int j=i;j<nAudioTrackCount;j++)
				if( pReader->m_nVideoDurationPushed[i] < pReader->m_nAudioDurationPushed[j] )
				{
					IsVideoBehindAudioDuration = TRUE;
					break;
				}
		if( IsVideoBehindAudioDuration == TRUE && IsAudioBehindVideoDuration == TRUE )
		{
			IsVideoBehindAudioDuration = FALSE;
			IsAudioBehindVideoDuration = FALSE;
		}
		// Read data from the file
		pReader->m_csSeeking.Lock();
		if( nVideoTrackCount > 0 &&	IsAudioBehindVideoDuration == FALSE )
			pReader->ReadVideoTrackFromFile();
		if( nAudioTrackCount > 0 && IsVideoBehindAudioDuration == FALSE )
			pReader->ReadAudioTrackFromFile();
		pReader->m_csSeeking.Unlock();
#else
		// Read data from the file
		pReader->m_csSeeking.Lock();
		if( nVideoTrackCount > 0 )
		{
			int nSampleCount = 0;
			int nIterations = 0;
			do
			{
				nSampleCount = pReader->ReadVideoTrackFromFile();
			}
			while (nSampleCount > 0 && ++nIterations < 10);
		}
		if( nAudioTrackCount > 0 )
		{
			int nSampleCount = 0;
			int nIterations = 0;
			do
			{
				nSampleCount = pReader->ReadAudioTrackFromFile();
			}
			while (nSampleCount > 0 && ++nIterations < 10);
		}
		pReader->m_csSeeking.Unlock();
#endif
	}

	for(int i = 0 ; i < nVideoTrackCount ; ++i)
		pReader->m_pVideoOut[i]->DeliverEndOfStream();
	for(int i = 0 ; i < nAudioTrackCount ; ++i)
		pReader->m_pAudioOut[i]->DeliverEndOfStream();

	if(bInterrupted)
		pReader->SetFileReaderStatus(READER_STOPPED);
	else
		pReader->SetFileReaderStatus(READER_EOF);

	return 0;
}

CVidiatorISOFormatDemuxFilter::CVidiatorISOFormatDemuxFilter(LPUNKNOWN pUnk, HRESULT *phr) :
//CBaseFilter( NAME("CVidiatorISOFormatDemuxFilter"), pUnk, &m_csFilter, CLSID_ISOFormatDemux2 ),
CSource( NAME("CVidiatorISOFormatDemuxFilter"), pUnk, CLSID_ISOFormatDemux2, phr ),
m_p3GPReader(NULL)
, m_nAVTrackCount(0)
, m_nVideoTrackCount(0)
, m_nAudioTrackCount(0)
, m_bVideoInturrupted(false)
, m_bAudioInturrupted(false)
, m_pAvcConfig(NULL)
, m_nConstLengthNAL(0)
, m_nTimeStampInc(0)
, m_hFileThreadFinished(NULL)
, m_nFixAudioTimeStamp(1)
, m_bStarted( FALSE )
, m_bFileIOError( TRUE )
, m_llStartPos( 0 )
{
	for(int i = 0 ; i < MAX_VIDEO_TRACK ; ++i)
		m_pVideoOut[i] = NULL;
	for(int i = 0 ; i < MAX_AUDIO_TRACK ; ++i)
		m_pAudioOut[i] = NULL;		
	m_status = READER_STOPPED;
	memset( &m_VideoInfo, 0, sizeof( m_VideoInfo ) );
	memset( &m_AudioInfo, 0, sizeof( m_VideoInfo ) );
	_tcscpy_s( m_szFilePathName, MAX_PATH, L"");
	m_tStart = 0;
	m_tStop = _I64_MAX / 2;
	m_dRate = 1.0;
	m_pSeekingPin = NULL;
	for(int i=0;i<MAX_VIDEO_TRACK;i++)
		m_nLastVideoTimeStamp[i] = 0;
	for(int i=0;i<MAX_AUDIO_TRACK;i++)
		m_nLastAudioTimeStamp[i] = 0;
	for(int i=0;i<MAX_VIDEO_TRACK;i++)
		m_nVideoDurationPushed[i] = 0;
	for(int i=0;i<MAX_AUDIO_TRACK;i++)
		m_nAudioDurationPushed[i] = 0;
}

CVidiatorISOFormatDemuxFilter::~CVidiatorISOFormatDemuxFilter(void)
{
	Close( );
	for(int i = 0 ; i < MAX_VIDEO_TRACK ; ++i)
	{
		if( m_pVideoOut[i] )
		{
			if( m_pVideoOut[i]->IsConnected() )
				m_pVideoOut[i]->BreakConnect(); //just in case we forgot to do it
			delete m_pVideoOut[i];
		}
		m_pVideoOut[i] = NULL;
	}
	for(int i = 0 ; i < MAX_AUDIO_TRACK ; ++i)
	{
		if( m_pAudioOut[i] )
		{
			if( m_pAudioOut[i]->IsConnected() )
				m_pAudioOut[i]->BreakConnect(); //just in case we forgot to do it
			delete m_pAudioOut[i];
		}
		m_pAudioOut[i] = NULL;
	}
	m_nVideoTrackCount = 0;
	m_nAudioTrackCount = 0;
}

STDMETHODIMP CVidiatorISOFormatDemuxFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
//	IMPLEMENT_QUERY_INTERFACE( ISpecifyPropertyPages );
	IMPLEMENT_QUERY_INTERFACE( IFileSourceFilter );
	if( riid == __uuidof(IISOFormatDemux2) )
	{		
        return GetInterface((IISOFormatDemux2*)this, ppv);
	}
	return __super::NonDelegatingQueryInterface(riid, ppv);
//	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
//	return CSource::NonDelegatingQueryInterface(riid, ppv);
} // NonDelegatingQueryInterface


HRESULT CVidiatorISOFormatDemuxFilter::Start(void)
{
	if( m_status != READER_OPENED )
	{
		return S_FALSE;
	}

	Start( 0 );

	if( m_status != READER_STARTED )
		return S_FALSE;

	HRESULT hr;
	//all pins get activated when the filter is started with (run->pause)
	for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
	{
		hr = m_pVideoOut[i]->Run( );
		if( hr != S_OK )
			dLog( "Could not start video pin\n");
		//start pusher thread. We could delay this until we actually have some data to push
		hr = m_pVideoOut[i]->Active();
		if( hr != S_OK )
			dLog( "Could not activate video pin\n");
	}
	for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
	{
		hr = m_pAudioOut[i]->Run( );
		if( hr != S_OK )
			dLog( "Could not start video pin\n");
		//start pusher thread. We could delay this until we actually have some data to push
		hr = m_pAudioOut[i]->Active();
		if( hr != S_OK )
			dLog( "Could not activate video pin\n");
	}

	return S_OK;
}

//need to be stopped before closed !
STDMETHODIMP CVidiatorISOFormatDemuxFilter::Stop()
{
	HRESULT hResult;

	if( m_status != READER_STARTED && m_status != READER_EOF )
	{
		return S_FALSE;
	}

	// these are not required. _super will iterate through the PINs and stop + inactivate them
	for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
	{
		m_pVideoOut[i]->Inactive( );
		m_pVideoOut[i]->Stop();
	}
	for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
	{
		m_pAudioOut[i]->Inactive( ); 
		m_pAudioOut[i]->Stop();
	}

	if( !m_bStarted )
	{
		return S_OK;
	}

	int i = 0;

	// Stop Read Thread
	m_bVideoInturrupted = true;
	m_bAudioInturrupted = true;

	if( m_hFileThreadFinished )
	{
		if( WaitForSingleObject( m_hFileThreadFinished, 50000 ) != WAIT_OBJECT_0 )
		{
			_ASSERT(!_T("File Reading Thread couldn't finish by time out"));
			TerminateThread( m_hFileThreadFinished, (DWORD)(-10) );
		}
		CloseHandle( m_hFileThreadFinished );
		m_hFileThreadFinished = NULL;
	}
	if( m_pVideoReadBuffer )
	{
		free( m_pVideoReadBuffer );
		m_pVideoReadBuffer = NULL;
	}
	m_nVideoReadBufferLen=0;

	if( m_pAudioReadBuffer )
	{
		free( m_pAudioReadBuffer );
		m_pAudioReadBuffer = NULL;
	}
	m_nAudioReadBufferLen=0;

	m_bStarted = FALSE;

	//also close the file
	Close();

	hResult = __super::Stop();
	return hResult;
}

HRESULT CVidiatorISOFormatDemuxFilter::Open(TCHAR *lpszFilePath)
{
	if(!lpszFilePath || (_tcslen(lpszFilePath) <= 0))
	{
		return S_FALSE;
	}
	if( m_status != READER_STOPPED && m_status != READER_CLOSED )
	{
		return S_OK;
	}

	if(m_status == READER_OPENED)
		return S_OK;

	unsigned int nErrorCode = 0;
	unsigned int nGetData;
	int nRet;

	m_p3GPReader = NxMP4FReaderInit(const_cast<_TCHAR*>(lpszFilePath), &nErrorCode);
	if(!m_p3GPReader)
	{
		return S_FALSE;
	}

	if( lstrlenW( lpszFilePath ) < MAX_PATH+MAX_PATH )
		_tcscpy_s( m_szFilePathName, MAX_PATH, lpszFilePath );

	// Get File Information
	m_nVideoTrackCount = NxMP4FReaderGetVideoTrackNum(m_p3GPReader);
	m_nAudioTrackCount = NxMP4FReaderGetAudioTrackNum(m_p3GPReader);

	//by kwanghyun won, 2008/10/22, PCL 3gp contents audio sync problem
	unsigned int nMultitrackFlag;
	NxMP4FReaderCheckMultiTrackInfos(m_p3GPReader,&nMultitrackFlag);
	if (nMultitrackFlag == 2)
	{
		m_nVideoTrackCount = 1;
		m_nAudioTrackCount = 1;
	}

	m_nAVTrackCount = m_nVideoTrackCount + m_nAudioTrackCount;

	memset(&m_VideoInfo, 0, sizeof(S_VIDEO_INFO));
	memset(&m_AudioInfo, 0, sizeof(S_AUDIO_INFO));
	
	if(NxMP4FReaderGetVideoCoder(m_p3GPReader, &nGetData) == 0)
	{
		m_VideoInfo.m_nCodecType = nGetData;
		nRet = NxMP4FReaderGetVideoAverageBitrate(m_p3GPReader, &nGetData, 0);
		m_VideoInfo.m_nBandwidth = nGetData;

		nRet = NxMP4FReaderGetVideoTrackID(m_p3GPReader, &nGetData);
		m_VideoInfo.m_nTrackID = nGetData;

		unsigned char *pDSI;
		nRet = NxMP4FReaderGetVideoDecoderSpecificInfo(m_p3GPReader, &pDSI, &nGetData);
		m_VideoInfo.m_nDSISize = nGetData;

		if (m_VideoInfo.m_pDSI)
			delete [] m_VideoInfo.m_pDSI;

		if (m_VideoInfo.m_nDSISize > 0)
		{
			m_VideoInfo.m_pDSI = new unsigned char[m_VideoInfo.m_nDSISize];
			memcpy(m_VideoInfo.m_pDSI, pDSI, m_VideoInfo.m_nDSISize);
		}
		else
		{
			m_VideoInfo.m_pDSI = NULL;
		}

		m_pAvcConfig = NULL;
		if (m_VideoInfo.m_nCodecType == OT_H264)
		{
			// 264용 구현 필요. -wuli
			m_pAvcConfig = NxMP4FReaderGetAVCConfigurationInformation(m_p3GPReader);
			m_nConstLengthNAL = 3;
		}

		nRet = NxMP4FReaderGetVideoProfile(m_p3GPReader, &nGetData);
		m_VideoInfo.m_nProfile = nGetData;

		nRet = NxMP4FReaderGetVideoLevel(m_p3GPReader, &nGetData);
		m_VideoInfo.m_nLevel = nGetData;

		nRet = NxMP4FReaderGetVideoWidth(m_p3GPReader, &nGetData);
		m_VideoInfo.m_nWidth = nGetData;

		nRet = NxMP4FReaderGetVideoHeight(m_p3GPReader, &nGetData);
		m_VideoInfo.m_nHeight = nGetData;

		nRet = NxMP4FReaderGetVideoFramerate(m_p3GPReader, &nGetData);
		m_VideoInfo.m_nTargetFramerate = static_cast<short>(nGetData);

		nRet = NxMP4FReaderGetVideoMultirateInfo(m_p3GPReader, m_pVideoTrackInfo, NXMP4F_MAX_TRACK_NUM);
	}

	if (NxMP4FReaderGetAudioCoder(m_p3GPReader, &nGetData) == 0)
	{
		m_AudioInfo.m_nCodecType = nGetData;
		nRet = NxMP4FReaderGetAudioTrackID(m_p3GPReader, &nGetData);
		m_AudioInfo.m_nTrackID = nGetData;

		nRet = NxMP4FReaderGetAudioAverageBitrate(m_p3GPReader, &nGetData, 0);
		m_AudioInfo.m_nBandwidth = nGetData;

		if (m_AudioInfo.m_pDSI)
			delete [] m_AudioInfo.m_pDSI;
		
		unsigned char *pDSI;
		nRet = NxMP4FReaderGetAudioDecoderSpecificInfo(m_p3GPReader, &pDSI, &nGetData);
		m_AudioInfo.m_nDSISize = nGetData;

		if (m_AudioInfo.m_nDSISize > 0)
		{
			m_AudioInfo.m_pDSI = new unsigned char[m_AudioInfo.m_nDSISize];
			memcpy(m_AudioInfo.m_pDSI, pDSI, m_AudioInfo.m_nDSISize);
		}
		else
		{
			m_AudioInfo.m_pDSI = NULL;
		}

		nRet = NxMP4FReaderGetAudioSamplingFrequency(m_p3GPReader, &nGetData);
		m_AudioInfo.m_nSamplingRate = nGetData;

		//Speech codec's time per frame is 20ms = 1/50 sec
		m_nTimeStampInc = m_AudioInfo.m_nSamplingRate / 50;

		m_AudioInfo.m_nBitPerSample = 16;

		nRet = NxMP4FReaderGetAudioChannels(m_p3GPReader, &nGetData);
		m_AudioInfo.m_nChannelNum = nGetData;

		if (m_AudioInfo.m_nCodecType == NXMP4F_OBJECTTYPE_AAC)
		{
			//Get the information about SBR and PS
			NXMP4F_ASC_INFO2 ascInfo2;
			if(NxMP4FReaderGetAudioASCInfo2(m_p3GPReader, &ascInfo2) == 0)
			{
				if(ascInfo2.ext_audio_object_type == 5)
				{
					m_AudioInfo.m_nCodecType = NXMP4F_OBJECTTYPE_HEAAC;
					m_AudioInfo.m_nSamplingRate = ascInfo2.ext_sampling_frequency;
					if(ascInfo2.ps_present_flag == 1)
					{
						m_AudioInfo.m_nCodecType = NXMP4F_OBJECTTYPE_EAACP;
					}
					//m_AudioInfo.m_nTimeStampInc = (unsigned int)(2048/(double)m_AudioInfo.m_nSamplingRate*1000);
					m_AudioInfo.m_nTimeStampIncMediaScale = 2048;
				}
				else if(ascInfo2.ext_audio_object_type == 29)
				{
					m_AudioInfo.m_nCodecType = NXMP4F_OBJECTTYPE_EAACP;
					m_AudioInfo.m_nSamplingRate = ascInfo2.ext_sampling_frequency;
					//m_AudioInfo.m_nTimeStampInc = (unsigned int)(2048/(double)m_AudioInfo.m_nSamplingRate*1000);
					m_AudioInfo.m_nTimeStampIncMediaScale = 2048;
				}
				else
				{ // pure aac
					//m_AudioInfo.m_nTimeStampInc = (unsigned int)(1024/(double)m_AudioInfo.m_nSamplingRate*1000);
					m_AudioInfo.m_nTimeStampIncMediaScale = 1024;
				}
			}
		
		}
		else if (m_AudioInfo.m_nCodecType == NXMP4F_OBJECTTYPE_MP3)
		{
			//m_AudioInfo.m_nTimeStampInc = (unsigned int)(1152/(double)m_AudioInfo.m_nSamplingRate*1000);
			m_AudioInfo.m_nTimeStampIncMediaScale = 1152;
		}
		else if (m_AudioInfo.m_nCodecType == NXMP4F_OBJECTTYPE_AMRWB)
		{
			m_AudioInfo.m_nTimeStampIncMediaScale = 320;
		}
		else 
		{
			//m_AudioInfo.m_nTimeStampIncMediaScale = 20;
			m_AudioInfo.m_nTimeStampIncMediaScale = 160;
		}

		nRet = NxMP4FReaderGetAudioMultirateInfo(m_p3GPReader, m_pAudioTrackInfo, NXMP4F_MAX_TRACK_NUM);
	}

	nRet = NxMP4FReaderGetVideoMultirateInfo(m_p3GPReader, m_pVideoTrackInfo, MAX_VIDEO_TRACK);
	nRet = NxMP4FReaderGetAudioMultirateInfo(m_p3GPReader, m_pAudioTrackInfo, MAX_AUDIO_TRACK);

	// Init handles to check if reading each track is finished
	for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
		m_hVideoTracksPendingEOF[i] = TRUE;
	for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
		m_hAudioTracksPendingEOF[i] = TRUE;

	m_bFileIOError = FALSE;
	m_llFileDuration = GetDuration( );

	SetStatus(READER_OPENED, FALSE);
	SetFileReaderStatus(READER_OPENED);

	//generate media types for each track
	MediaStreamInfoParam MSI;
	HRESULT hr;
	memset( &MSI, 0, sizeof( MSI ) );
	for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
	{
		//create a PIN
		if( m_pVideoOut[i] == NULL )
			m_pVideoOut[i] = new ISOFormatPushStreamPin( L"ISOFormatDemux Video Out ", &hr, this, L"Video" );
		//set pin media info
		CMediaType tType;
		MSI.m_cDecoderSpecific = m_pVideoTrackInfo[i].param.VidInfo.VidDecSpecificInfoSize;
		MSI.m_pDecoderSpecific = m_pVideoTrackInfo[i].param.VidInfo.VidDecSpecificInfo;
		MSI.m_cy = m_pVideoTrackInfo[i].param.VidInfo.VidHeight;
		MSI.m_cx = m_pVideoTrackInfo[i].param.VidInfo.VidWidth;
		MSI.m_tFrame = m_pVideoTrackInfo[i].param.VidInfo.VidFrameRate;
		if( m_pVideoTrackInfo[i].param.VidInfo.VidAVCCBox )
			MSI.m_AVCLengthSizeMinusOne = m_pVideoTrackInfo[i].param.VidInfo.VidAVCCBox->lengthSizeMinusOne;
		else
			MSI.m_AVCLengthSizeMinusOne = 3; //panic ? Used only for H264
		GetTypeMediaTyeParams( &tType, m_VideoInfo.m_nCodecType, &MSI );
		m_pVideoOut[i]->SetMediaTypeInternal( &tType );
		//note that buffer size is not a golden rule for VBR !
		int buffers = max( DEFAULT_PACKET_QUEUE_SIZE, 1 + m_pVideoTrackInfo[i].param.VidInfo.VidFrameRate / 1000 );
		m_pVideoOut[i]->SetAllocatorProps( buffers , ( m_pVideoTrackInfo[i].param.VidInfo.VidAvgBitrate / 8 ) * 6 ); 
#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
		m_pVideoOut[i]->SetTrackID( i, 0 );
#endif
	}
	for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
	{
		//create a PIN
		if( m_pAudioOut[i] == NULL )
			m_pAudioOut[i] = new ISOFormatPushStreamPin( L"ISOFormatDemux Audio Out ", &hr, this, L"Audio" );
		//set pin media info
		CMediaType tType;
		MSI.m_cDecoderSpecific = m_pAudioTrackInfo[i].param.AudInfo.AudDecSpecificInfoSize;
		MSI.m_pDecoderSpecific = m_pAudioTrackInfo[i].param.AudInfo.AudDecSpecificInfo;
		MSI.m_llBitPerSample = m_AudioInfo.m_nBitPerSample;
		MSI.m_llSamplingRate = m_pAudioTrackInfo[i].param.AudInfo.AudSamplingFreq;
		MSI.m_llChannelCount = m_pAudioTrackInfo[i].param.AudInfo.AudChannels;
		MSI.m_BytesPerSec = m_AudioInfo.m_nBandwidth / 8;
		GetTypeMediaTyeParams( &tType, m_AudioInfo.m_nCodecType, &MSI );
		m_pAudioOut[i]->SetMediaTypeInternal( &tType );
		//my guess for 3 second data
		int buffers = max( DEFAULT_PACKET_QUEUE_SIZE, 3000 / ( m_pAudioTrackInfo[i].param.AudInfo.AudDuration / m_pAudioTrackInfo[i].param.AudInfo.AudTimescale ) );
		m_pAudioOut[i]->SetAllocatorProps( buffers, m_AudioInfo.m_nBandwidth / 8 * 3 ); 
#ifdef SYNC_AUDIO_VIDEO_TIMESTAMPS_AT_SEND
		m_pAudioOut[i]->SetTrackID( i, 1 );
#endif
	}

	return S_OK;
}

HRESULT CVidiatorISOFormatDemuxFilter::Close()
{
	HRESULT hResult = S_OK;

	if(m_status == READER_CLOSED)
		return true;

	if( m_bStarted )
		Stop( );

	if(m_p3GPReader)
	{
		NxMP4FReaderClose(m_p3GPReader);
		m_p3GPReader = NULL;
	}

	if(m_VideoInfo.m_pDSI)
	{
		delete [] m_VideoInfo.m_pDSI;
		m_VideoInfo.m_pDSI = NULL;
	}
	if(m_AudioInfo.m_pDSI)
	{
		delete [] m_AudioInfo.m_pDSI;
		m_AudioInfo.m_pDSI = NULL;
	}

	SetStatus(READER_CLOSED, FALSE);
	SetFileReaderStatus(READER_CLOSED);

	return true;
}

bool CVidiatorISOFormatDemuxFilter::Start(LONGLONG llStartPos)
{
	bool bRet = true;

	if(m_status == READER_STARTED)
		return true;

	if( m_p3GPReader == NULL )
	{
		return false; // unexpected.  
	}

	m_llStartPos = llStartPos;
	if( m_bStarted )
		Stop( );

	for(int i=0;i<MAX_VIDEO_TRACK;i++)
		m_nLastVideoTimeStamp[i] = 0;
	for(int i=0;i<MAX_AUDIO_TRACK;i++)
		m_nLastAudioTimeStamp[i] = 0;
	for(int i=0;i<MAX_VIDEO_TRACK;i++)
		m_nVideoDurationPushed[i] = 0;
	for(int i=0;i<MAX_AUDIO_TRACK;i++)
		m_nAudioDurationPushed[i] = 0;

	m_bVideoInturrupted = false;
	m_bAudioInturrupted = false;
	// Move file reader to the start position
	if(llStartPos > 0)
	{
		unsigned int nVideoTrackPos = 0;
		unsigned int nAudioTrackPos = 0;
		unsigned int nTextTrackPos = 0;

		NxMP4FReaderRandomAccess(m_p3GPReader, (unsigned int)llStartPos, &nAudioTrackPos, &nVideoTrackPos, &nTextTrackPos, NXMP4F_RA_SEEK_SYNC, NXMP4F_FLAG_MSEC_TIMESCALE);
	}

	///////////////////////
	// The Reader Buffer  must be allocated before creating threads. jerry, 2010-10-19
	m_pVideoReadBuffer = NULL;
	m_nVideoReadBufferLen = 0;
	if( m_nVideoTrackCount > 0 )
	{
		m_nVideoReadBufferLen = (m_VideoInfo.m_nBandwidth/ONE_FRAMESIZE+1) * ONE_FRAMESIZE;
		m_pVideoReadBuffer = (BYTE*)malloc( m_nVideoReadBufferLen );
		memset( m_pVideoReadBuffer, 0, m_nVideoReadBufferLen );
		if( !m_pVideoReadBuffer )
		{			
			m_nVideoReadBufferLen=0;
			return false;
		}
	}

	m_pAudioReadBuffer = NULL;
	m_nAudioReadBufferLen = 0;
	if( m_nAudioTrackCount > 0 )
	{
		m_nAudioReadBufferLen = (m_AudioInfo.m_nBandwidth/ONE_FRAMESIZE+1) *ONE_FRAMESIZE;
		m_pAudioReadBuffer = (BYTE*)malloc( m_nAudioReadBufferLen );
		memset( m_pAudioReadBuffer, 0, m_nAudioReadBufferLen );
		if( !m_pAudioReadBuffer )
		{			
			if( m_pVideoReadBuffer )
			{				
				free( m_pVideoReadBuffer );
				m_pVideoReadBuffer = NULL;
			}
			m_nVideoReadBufferLen=0;
			m_nAudioReadBufferLen=0;
			return false;
		}
	}
	//} jerry, 2010-10-19
	/////////////////////////

	// Create reader buffer
	DWORD threadid = 0;	
	m_hFileThreadFinished = (HANDLE)CreateThread(NULL, 65536, ReadFileThread, this, CREATE_SUSPENDED|STACK_SIZE_PARAM_IS_A_RESERVATION, &threadid);
	if( m_hFileThreadFinished != 0)
	{
		// Start Thread
		::ResumeThread((HANDLE)m_hFileThreadFinished);
	}
	else
	{
		bRet = false;
	}
	m_bStarted = TRUE;

	if(bRet)
	{
		SetStatus(READER_STARTED,FALSE);
	}

	return bRet;
}

bool CVidiatorISOFormatDemuxFilter::Seek(unsigned int nSeekPos)
{
	unsigned int nVideoTrackPos = 0;
	unsigned int nAudioTrackPos = 0;
	unsigned int nTextTrackPos = 0;
	int Result;

	_ASSERT(m_p3GPReader);
	if(!m_p3GPReader)
	{
		return false;
	}

	Result = NxMP4FReaderRandomAccess(m_p3GPReader, nSeekPos, &nAudioTrackPos, &nVideoTrackPos, &nTextTrackPos, NXMP4F_RA_SEEK_SYNC, NXMP4F_FLAG_MSEC_TIMESCALE);
	if( Result != 0 )
		dLog1( "Seek returned error code %d\n",Result);
	return true;
}


void CVidiatorISOFormatDemuxFilter::SetStatus(READER_STATUS status, BOOL bPost )
{
	// Event callback
	m_status = status;
}

int CVidiatorISOFormatDemuxFilter::ReadVideoTrackFromFile()
{
	int nSampleCount = 0;
	BOOL AllSeekReady = TRUE;
	BOOL NotEOF = TRUE;
	do
	{
		AllSeekReady = TRUE;
		for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
		{
			//only read tracks for which we have PINS
			if( m_pVideoOut[i]->IsConnected() == false || m_hVideoTracksPendingEOF[i] == FALSE )
			{
				m_hVideoTracksPendingEOF[i] = FALSE;
				continue;
			}
			if( m_pVideoOut[i]->IsQueueFull() == TRUE )
			{
				continue;
			}
			// Read data from the file if the bufferfulless is less than 60%
			{
				unsigned char nResult = 1;
				unsigned int nSize = 0;
				unsigned int nTimeStamp = 0;
				unsigned char * pReadData = m_pVideoReadBuffer;

				if(m_nVideoTrackCount > 1)
				{
					// Multi track reading
					nResult = NxMP4FReaderVideoFrameMultiRead(m_p3GPReader, 
															&m_pVideoTrackInfo[i], 
															pReadData, 
															m_nVideoReadBufferLen,
															&nSize,
															&nTimeStamp,
															NXMP4F_FLAG_MSEC_TIMESCALE);
				}
				else
				{
					//by kwanghyun won, 2008/10/22, PCL 3gp contents audio sync problem
					unsigned int nMultitrackFlag;
					NxMP4FReaderCheckMultiTrackInfos(m_p3GPReader,&nMultitrackFlag);
					if (nMultitrackFlag == 2)
					{
						// Multi track reading
						nResult = NxMP4FReaderVideoFrameMultiRead(	m_p3GPReader, 
																	&m_pVideoTrackInfo[i], 
																	pReadData, 
																	m_nVideoReadBufferLen,
																	&nSize,
																	&nTimeStamp,
																	NXMP4F_FLAG_MSEC_TIMESCALE);
					}
					else
					{
						// Single track reading
						nResult = NxMP4FReaderVideoFrameRead(m_p3GPReader, 
														pReadData, 
														m_nVideoReadBufferLen,
														&nSize,
														&nTimeStamp,
														NXMP4F_FLAG_MSEC_TIMESCALE);
					}
				}

				if(nResult == 0)
				{
#if defined( MEDIA_TIMESTAMP_FROM_SAMPLINGRATE )
					//This was added because MP4Reader was adding latency to timestamps and was creating renderer shutter
					LONGLONG DisplayTime = UNITS / m_AudioInfo.m_nSamplingRate * 1000;
#elif defined( MEDIA_TIMESTAMP_FROM_DURATION_AND_PACKETCOUNT )
					unsigned char error;
					unsigned int SampleSize,*ppEntrySize,pSampleCount;
					error = NxMP4FReaderGetVideoSampleSizeList( m_p3GPReader, &SampleSize, &ppEntrySize, &pSampleCount );
					unsigned int duration = GetDuration();
					unsigned int avgsampledur = duration * 10000 / pSampleCount;
					LONGLONG DisplayTime = avgsampledur;
#elif defined( MEDIA_TIMESTAMP_FROM_MP4API )
					m_nLastVideoTimeStamp[i] = (LONGLONG)10000 * NxMP4FReaderGetCurrentVideoSampleTime( m_p3GPReader, NXMP4F_FLAG_MSEC_TIMESCALE );
					LONGLONG DisplayTime = (LONGLONG)10000 * NxMP4FReaderGetNextVideoSampleTime( m_p3GPReader, NXMP4F_FLAG_MSEC_TIMESCALE ) - m_nLastVideoTimeStamp[i];
#elif defined( MEDIA_TIMESTAMP_FROM_MP4API_WITH_RATE_SUPPORT ) || defined( MEDIA_TIMESTAMP_FROM_NEXT_PACKET )
					LONGLONG DisplayTime = (LONGLONG)10000 * NxMP4FReaderGetNextVideoSampleTime( m_p3GPReader, NXMP4F_FLAG_MSEC_TIMESCALE ) - m_nLastVideoTimeStamp[i];
					DisplayTime = LONGLONG( DisplayTime * m_dRate );
#endif
					if( m_nLastVideoTimeStamp[i] >= m_tStart && m_nLastVideoTimeStamp[i] <= m_tStop )
					{
						//do not deliver empty packets. Might crash some decoders ?
						if( nSize <= 4 )
							continue;
//printf("Video track %d - %d\n", i, (int) m_nLastVideoTimeStamp[i] );
						m_pVideoOut[i]->QueueDataToPush( pReadData, nSize, m_nLastVideoTimeStamp[i] - m_tStart, DisplayTime );
						++nSampleCount;
					}
					else if( m_nLastVideoTimeStamp[i] > m_tStop )
					{
						m_hVideoTracksPendingEOF[i] = FALSE;
						NotEOF = FALSE;
					}
					else
						AllSeekReady = FALSE;
					m_nLastVideoTimeStamp[i] += DisplayTime;
				}
				else
				{
					if( nResult != 1 )
						m_bFileIOError = TRUE;
					// Finish reading this track
					m_hVideoTracksPendingEOF[i] = FALSE;
					NotEOF = FALSE;
				}
			}	
		}
	}
	while( AllSeekReady == FALSE && NotEOF == TRUE );
	return nSampleCount;
}

int CVidiatorISOFormatDemuxFilter::ReadAudioTrackFromFile()
{
	int nSampleCount = 0;
	BOOL AllSeekReady = TRUE;
	BOOL NotEOF = TRUE;
	do
	{
		AllSeekReady = TRUE;
		for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
		{
		//only read tracks for which we have PINS
			if( m_pAudioOut[i]->IsConnected() == false || m_hAudioTracksPendingEOF[i] == FALSE )
			{
				m_hAudioTracksPendingEOF[i] = FALSE;
				continue;
			}
			if( m_pAudioOut[i]->IsQueueFull() == TRUE )
				continue;
			// Read data from the file if the bufferfulless is less than 60%
			{
				unsigned char nResult = 1;
				unsigned int nSize = 0;
				unsigned int nTimeStamp = 0;
				unsigned int nTimeScale = 0;
				unsigned char * pReadData = m_pAudioReadBuffer;

				if(m_nAudioTrackCount > 1)
				{
					// Multi track reading
					nResult = NxMP4FReaderAudioFrameMultiRead(m_p3GPReader, 
															&m_pAudioTrackInfo[i],
															pReadData, 
															m_nAudioReadBufferLen,
															&nSize,
															&nTimeStamp,
															NXMP4F_FLAG_MEDIA_TIMESCALE);

					//by kwanghyun won, 2008/10/24, PCL 3gp contents audio sync problem
					NxMP4FReaderGetAudioTimescale(m_p3GPReader, &nTimeScale);
				}
				else
				{
					//by kwanghyun won, 2008/10/22, PCL 3gp contents audio sync problem
					unsigned int nMultitrackFlag;
					NxMP4FReaderCheckMultiTrackInfos(m_p3GPReader,&nMultitrackFlag);
					if (nMultitrackFlag == 2) //PCL
					{
						// Multi track reading
						nResult = NxMP4FReaderAudioFrameMultiRead(m_p3GPReader, 
																&m_pAudioTrackInfo[i], 
																pReadData, 
																m_nAudioReadBufferLen,
																&nSize,
																&nTimeStamp,
																NXMP4F_FLAG_MEDIA_TIMESCALE);

						NxMP4FReaderGetAudioTimescale(m_p3GPReader, &nTimeScale);

					}
					else
					{
						// Single track reading
						nResult = NxMP4FReaderAudioFrameRead(m_p3GPReader, 
															pReadData,
															m_nAudioReadBufferLen,
															&nSize,
															&nTimeStamp,
															NXMP4F_FLAG_MEDIA_TIMESCALE);

							NxMP4FReaderGetAudioTimescale(m_p3GPReader, &nTimeScale);
					}
				}

				if(nResult == 0)
				{
#if defined( MEDIA_TIMESTAMP_FROM_SAMPLINGRATE )
					//This was added because MP4Reader was adding latency to timestamps and was creating renderer shutter
					LONGLONG DisplayTime = UNITS / m_AudioInfo.m_nSamplingRate * 1000;
#elif defined( MEDIA_TIMESTAMP_FROM_DURATION_AND_PACKETCOUNT )
					unsigned char error;
					unsigned int SampleSize,*ppEntrySize,pSampleCount;
					error = NxMP4FReaderGetAudioSampleSizeList( m_p3GPReader, &SampleSize, &ppEntrySize, &pSampleCount );
					unsigned int duration = GetDuration();
					unsigned int avgsampledur = duration * 10000 / pSampleCount;
					LONGLONG DisplayTime = avgsampledur;
#elif defined( MEDIA_TIMESTAMP_FROM_MP4API )
					m_nLastAudioTimeStamp[i] = (LONGLONG)10000 * NxMP4FReaderGetCurrentAudioSampleTime( m_p3GPReader, NXMP4F_FLAG_MSEC_TIMESCALE );
					LONGLONG DisplayTime = (LONGLONG)10000 * NxMP4FReaderGetNextAudioSampleTime( m_p3GPReader, NXMP4F_FLAG_MSEC_TIMESCALE ) - m_nLastAudioTimeStamp[i];
#elif defined( MEDIA_TIMESTAMP_FROM_MP4API_WITH_RATE_SUPPORT ) || defined( MEDIA_TIMESTAMP_FROM_NEXT_PACKET )
					LONGLONG DisplayTime = (LONGLONG)10000 * NxMP4FReaderGetNextAudioSampleTime( m_p3GPReader, NXMP4F_FLAG_MSEC_TIMESCALE ) - m_nLastAudioTimeStamp[i];
					DisplayTime = LONGLONG( DisplayTime * m_dRate );
#endif
					if( m_nLastAudioTimeStamp[i] >= m_tStart && m_nLastAudioTimeStamp[i] <= m_tStop )
					{
						//do not deliver empty packets. Might crash some decoders ?
						if( nSize <= 4 )
							continue;
						//printf("Audio track %d - %d\n", i, (int) m_nLastAudioTimeStamp[i] );

						BOOL bFragmentSample = false;
						switch(m_AudioInfo.m_nCodecType)
						{
						case OT_AMR:
						case OT_EVRC:
						case OT_QCELP:
						case OT_AMRWB:
						//case OT_AMRWP:
							bFragmentSample = TRUE;
							break;
						}
						
						if (bFragmentSample)
						{
							unsigned int nTotalDeliveredSize = 0;
							LONGLONG llTimeStamp = m_nLastAudioTimeStamp[i] - m_tStart;
							int nFrameSize = 0;
							unsigned char *pData = m_pAudioReadBuffer;

							while(nTotalDeliveredSize < nSize)
							{
								int nFrameSize = 0;
							
								switch(m_AudioInfo.m_nCodecType)
								{
								case OT_AMR:
									get_samr_frame_size(pData[0], &nFrameSize);
									break;
								case OT_EVRC:
									get_sevc_frame_size(pData[0], &nFrameSize);
									break;
								case OT_QCELP:
									get_sqcp_frame_size(pData[0], &nFrameSize);
									break;
								case OT_AMRWB:
									get_sawb_frame_size(pData[0], &nFrameSize);
									break;
								case OT_AMRWP:
									//get_sawb_frame_size(pABitStream[0], &nFrameSize);
									break;
								default:
									_ASSERT(!_T("Invalid Audio Type!!!"));
								}

								if(nFrameSize > 0)
								{
									LONGLONG ll20msDisplayTime = 200000;

									// Included frame size filed
									nFrameSize += 1; 

									m_pAudioOut[i]->QueueDataToPush(pData, nFrameSize, llTimeStamp, ll20msDisplayTime);
									++nSampleCount;

									// Next time stamp
									llTimeStamp += ll20msDisplayTime; // Voice codec duration 20ms
									pData += nFrameSize;
									nTotalDeliveredSize += nFrameSize;
								}
							}
						}
						else // bFragmentSample - other audio codecs
						{
							m_pAudioOut[i]->QueueDataToPush(pReadData, nSize, m_nLastAudioTimeStamp[i] - m_tStart, DisplayTime);
							++nSampleCount;
						}
					}
					else if( m_nLastAudioTimeStamp[i] > m_tStop )
					{
						m_hAudioTracksPendingEOF[i] = FALSE;
						NotEOF = FALSE;
					}
					else
						AllSeekReady = FALSE;
					m_nLastAudioTimeStamp[i] += DisplayTime;
				}
				else
				{
					GLOG( (ISMLOG, TEXT("[3GPR:ReadAudioTrackFromFile] File (%s) read error (%d)"), m_szFilePathName, nResult ) );
					if( nResult != 1 )
						m_bFileIOError = TRUE;
					// Finish reading this track
					m_hAudioTracksPendingEOF[i] = FALSE;
					NotEOF = FALSE;
				}
			}	
		}//for i
	}
	while( AllSeekReady == FALSE && NotEOF == TRUE );
	return nSampleCount;
}

S_VIDEO_INFO * CVidiatorISOFormatDemuxFilter::GetVideoInfo()
{
	if (m_VideoInfo.m_nTrackID == 0)
		return NULL;
	else
		return &m_VideoInfo;
}

S_AUDIO_INFO * CVidiatorISOFormatDemuxFilter::GetAudioInfo()
{
	if (m_AudioInfo.m_nTrackID == 0)
		return NULL;
	else
		return &m_AudioInfo;
}

NxMP4FTrackInfo* CVidiatorISOFormatDemuxFilter::GetAudioMultiTrackInfo(int nTrackIndex)
{
	if((nTrackIndex < 0) || (nTrackIndex > NXMP4F_MAX_TRACK_NUM))
		return NULL;
	return &m_pAudioTrackInfo[nTrackIndex];
}

NxMP4FTrackInfo* CVidiatorISOFormatDemuxFilter::GetVideoMultiTrackInfo(int nTrackIndex)
{
	if((nTrackIndex < 0) ||	(nTrackIndex > NXMP4F_MAX_TRACK_NUM))
		return NULL;
	return &m_pVideoTrackInfo[nTrackIndex];
}
/*
unsigned char CVidiatorISOFormatDemuxFilter::GetVideoSpecificInfo(unsigned char **buf, unsigned int *size)
{
	return NxMP4FReaderGetVideoDecoderSpecificInfo(m_p3GPReader, buf, size);
} */

REFERENCE_TIME CVidiatorISOFormatDemuxFilter::GetDuration()
{
	if(!m_p3GPReader)
		return 0;
	unsigned int nDuration = 0;
	NxMP4FReaderGetMovieDuration(m_p3GPReader, &nDuration, NXMP4F_FLAG_MSEC_TIMESCALE);
	return nDuration;
}

STDMETHODIMP CVidiatorISOFormatDemuxFilter::Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt)
{
	HRESULT hr = Open( LPOLESTR( pwszFileName ) );
	Start();
	return hr;
}

STDMETHODIMP CVidiatorISOFormatDemuxFilter::Run(REFERENCE_TIME tStart)
{
	HRESULT hr = S_OK;
	hr = __super::Run(tStart);
	return hr;
}

STDMETHODIMP CVidiatorISOFormatDemuxFilter::Pause()
{
	HRESULT hr = S_OK;
	//we probably stopped the filter in GraphEdt and we are trying to restart it
	if( m_p3GPReader == NULL && lstrlenW(m_szFilePathName) != 0 )
	{
		Open( m_szFilePathName );
		Start( 0 );
	}
	hr = __super::Pause();
	return hr;
}

STDMETHODIMP CVidiatorISOFormatDemuxFilter::GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
	CheckPointer(ppszFileName,E_POINTER);
    *ppszFileName = NULL;
    if(pmt) 
    {
        ZeroMemory(pmt, sizeof(*pmt));
        pmt->majortype = MEDIATYPE_NULL;
        pmt->subtype = MEDIASUBTYPE_NULL;
	}

    size_t len = 1+lstrlenW(m_szFilePathName);
    *ppszFileName = (LPOLESTR)QzTaskMemAlloc(sizeof(WCHAR) * (len));

    if (*ppszFileName != NULL) 
		HRESULT hr = StringCchCopyW(*ppszFileName, len, m_szFilePathName);

    if( pmt ) 
    {
		//what if user wanted to get audio media type ?
		CMediaType mt;
		if (m_pVideoOut[0])
		{
			m_pVideoOut[0]->GetMediaType( &mt );
		}
		else if (m_pAudioOut[0])
		{
			m_pAudioOut[0]->GetMediaType( &mt );
		}
		*pmt = mt;
    }

    return S_OK;	
}

bool CVidiatorISOFormatDemuxFilter::SelectSeekingPin(ISOFormatPushStreamPin* pPin)
{
    CAutoLock lock(&m_csSeeking);
    if (m_pSeekingPin == NULL)
        m_pSeekingPin = pPin;
    return(m_pSeekingPin == pPin);
}

void CVidiatorISOFormatDemuxFilter::DeselectSeekingPin(ISOFormatPushStreamPin* pPin)
{
    CAutoLock lock(&m_csSeeking);
    if (pPin == m_pSeekingPin)
        m_pSeekingPin = pPin;
}

void CVidiatorISOFormatDemuxFilter::GetSeekingParams(REFERENCE_TIME* ptStart, REFERENCE_TIME* ptStop, double* pdRate)
{
    if (ptStart != NULL)
        *ptStart = m_tStart;
    if (ptStop != NULL)
        *ptStop = m_tStop;
    if (pdRate != NULL)
        *pdRate = m_dRate;
}

HRESULT CVidiatorISOFormatDemuxFilter::SetRate(double dRate)
{
    CAutoLock lock(&m_csSeeking);
    m_dRate = dRate;
    return S_OK;
}

HRESULT CVidiatorISOFormatDemuxFilter::SetStopTime(REFERENCE_TIME tStop)
{
    CAutoLock lock(&m_csSeeking);
    // this does not guarantee that a stop change only, while running,
    // will stop at the right point -- but most filters only
    // implement stop/rate changes when the current position changes
    m_tStop = tStop;
    return S_OK;
}

HRESULT CVidiatorISOFormatDemuxFilter::Seek(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	//not sure if this will properly work
	if( tStart < m_nLastVideoTimeStamp[0] || tStart < m_nLastAudioTimeStamp[0] )
		Restart();
    // new seeking params
    m_tStart = tStart;
    m_tStop = tStop;
    m_dRate = dRate;

    return S_OK;
}

HRESULT CVidiatorISOFormatDemuxFilter::Restart()
{
	if( m_p3GPReader == NULL || lstrlenW(m_szFilePathName) == 0 )
	{
		return S_FALSE;
	}
	m_csSeeking.Lock();
	// file reader jump to the beggining of the File
	Seek( 0 );
	// reset EOF flags
	for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
		m_hVideoTracksPendingEOF[i] = TRUE;
	for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
		m_hAudioTracksPendingEOF[i] = TRUE;
	m_bFileIOError = FALSE;
	//time stamp to 0
	for(int i=0;i<MAX_VIDEO_TRACK;i++)
		m_nLastVideoTimeStamp[i] = 0;
	for(int i=0;i<MAX_AUDIO_TRACK;i++)
		m_nLastAudioTimeStamp[i] = 0;
	for(int i=0;i<MAX_VIDEO_TRACK;i++)
		m_nVideoDurationPushed[i] = 0;
	for(int i=0;i<MAX_AUDIO_TRACK;i++)
		m_nAudioDurationPushed[i] = 0;
	//reset file reader external interrupt state
	m_bVideoInturrupted = false;
	m_bAudioInturrupted = false;
	//flush output queues
	for(int i = 0 ; i < m_nVideoTrackCount ; ++i)
	{
		m_pVideoOut[i]->DeliverBeginFlush();
		m_pVideoOut[i]->FlushQueue();
		m_pVideoOut[i]->DeliverEndFlush();
		m_pVideoOut[i]->SetDiscontinuity( TRUE );
	}
	for(int i = 0 ; i < m_nAudioTrackCount ; ++i)
	{
		m_pAudioOut[i]->DeliverBeginFlush();
		m_pAudioOut[i]->FlushQueue();
		m_pAudioOut[i]->DeliverEndFlush();
		m_pAudioOut[i]->SetDiscontinuity( TRUE );
	}
	m_csSeeking.Unlock();
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}
