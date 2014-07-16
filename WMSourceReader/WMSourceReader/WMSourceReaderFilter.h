#ifndef _WMSOURCE_FILTER_H
#define _WMSOURCE_FILTER_H

#include "VidiatorFilterGuids.h"
#include "defines.h"

//enable this to write debug messages about the reader. Needs to be implemented
//#define DMSG_ON_WMReader
//Callback events on source change or other events. Using these until we will support Pins
//#define SUPPORT_CALLBACKS
//On loading we should register input types and use it until we die. If we change format then we should stop filter ? Not sure why these need to exist. Removing for now
//#define SUPPORT_GETAUDIOVIDEOFORMAT_API
//our other filters rely on IYUV. The less code the less space for errors
#define SUPPORT_IYUV_OUTPUT_ONLY
//we will swap callbacks to PINs. Right now leaving old callbacks until PINs are tested
#define SUPPORT_OUTPUT_PINS
//right now for some reason events are crashing. Temp method until i find real crash reason
//Found the crash : try to not use forceast in the future
//#define USE_POLLING_EVENTS
#define SIGNAL_UNINISHED_MEDIA_SWITCH_SUPPORT
//this means we will open the file on start and close it on stop
#define USE_FILE_SOURCE_INTERFACE_ONLY

class CVidiatorWMSourceOutputPin;
class PusherStreamPin;
class ISampleCallback;

//Can be used asa async reader
class CVidiatorWMSourceReaderFilter : public IWMReaderCallback, public CSource, public IFileSourceFilter, public IVidiatorWMSourceReader
{
public:
	CVidiatorWMSourceReaderFilter(LPUNKNOWN pUnk,HRESULT *phr);
	~CVidiatorWMSourceReaderFilter(void);

	//general use lock. Will span over multiple child objects( ex PINS )
	CCritSec				m_csFilter;
	FILTER_STATE_INTERNAL	GetStateInternal( ){ return m_INState; };
protected:

    // Implements the IBaseFilter and IMediaFilter interfaces
    DECLARE_IUNKNOWN
	/////////////////////////////
	// Override CUnknown / IUnkonwn interface : to create reference of this filter.
	STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	////////////////////////////
	// Override CBaseFilter / IBaseFilter interface 	
	STDMETHODIMP		Stop();
	STDMETHODIMP		Pause();
	STDMETHODIMP		Run(REFERENCE_TIME tStart);

	////////////////////////////
	// Implement IFileSourceFilter interface
	STDMETHODIMP		Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP		GetCurFile(LPOLESTR *ppwszFileName, AM_MEDIA_TYPE *pmt);

	//API calls
#ifdef SUPPORT_CALLBACKS
	//register callback when the reader has a new sample ready for us. 
	//Note this will block the reader until we are done using the buffer
	//If you want true Async then add a new pusher thread / queue for our prepared data
	STDMETHODIMP		SetSampleCallback(ISampleCallback *pSampleCallback);
	STDMETHODIMP		SetSampleCallback(BYTE *pSampleCallback);
	STDMETHODIMP		SetEventCallback(IEventCallback *pEventCallback);
#endif
	STDMETHODIMP		Open( TCHAR *lpszURL );
	STDMETHODIMP		Start();
	STDMETHODIMP		Close();
protected:
	// IWMReaderCallback interface
	HRESULT	STDMETHODCALLTYPE		OnSample(DWORD dwOutputNum, QWORD cnsSampleTime, QWORD cnsSampleDuration, DWORD dwFlags, INSSBuffer *pSample, void *pvContext);
	// IWMStatusCallback interface
	HRESULT	STDMETHODCALLTYPE		OnStatus(WMT_STATUS Status, HRESULT hr, WMT_ATTR_DATATYPE dwType, BYTE *pValue, void *pvContext);

	//Internal only. Block filter until specific events are done. Used to remove async features of the reader
	bool							WaitforWMStatus(SYNC_METHOD_NAMES methodName, DWORD dwTimeout);

	bool							InitReader( );
	void							DoneReader( );
	void							SetYUVOutputProp();
	void							SetPCMOutputProp();
	BOOL							SetMatchedOutputProps(DWORD dwStreamIdx,const IID & guid);
	bool							SaveMediaFormat(void);
	void							ResetTimeStamp( );
	FILTER_STATE_INTERNAL			m_INState;
#ifdef DMSG_ON_WMReader
	void							PrintOutputStreams( );
#endif
#ifdef SUPPORT_GETAUDIOVIDEOFORMAT_API
	WM_MEDIA_TYPE*					GetVideoFormat(void);
	bool							GetVideoFormat(WMVIDEOINFOHEADER *pVideoFormat);
	WM_MEDIA_TYPE*					GetAudioFormat(void);
	bool							GetAudioFormat(WAVEFORMATEX *pAudioFormat);
#endif
//	void							SetLoop(bool bLoop) { m_bLoop = bLoop; }
//	WMT_STATUS						GetWMStatus(void);
	WMT_STATUS						m_wmStatus;
	HRESULT							m_hWMResult;
	bool							CompareMediaType(WM_MEDIA_TYPE *pType1, WM_MEDIA_TYPE *pType2);
	
	// IFileSourceFilter
	LPOLESTR						m_pFileName;
	IWMReader						*m_pWMReader;
#ifdef SUPPORT_CALLBACKS
	ISampleCallback					*m_pSampleCallback;
	IEventCallback					*m_pEventCallback;
#endif
#ifndef USE_POLLING_EVENTS
	HANDLE							m_pMethodHandleArray[SYNC_METHOD_MAX];
#else
	int								Events[SYNC_METHOD_MAX];
#endif

	E_INPUT_SUBTYPE					m_eInputSubtype;
	DWORD							m_dwSrcWidth;
	DWORD							m_dwSrcHeight;

	BYTE							*m_pIYUVTempBuff;
	DWORD							m_nIYUVTempBuff;
	CRITICAL_SECTION				m_CSIYUVBuff;				
	std::vector<WM_MEDIA_TYPE*>		m_VecOutFormat;
//	bool							m_bLoop;	
#ifdef SUPPORT_GETAUDIOVIDEOFORMAT_API
	int								m_nAudioByteRate;
#endif
	MTime							m_VideoLastTS;
	MTime							m_VideoAvgDur;
	MTime							m_AudioLastTS;
	MTime							m_AudioLastDur;
	MTime							m_StartTimeOffset;
//	zstring							m_strURL4Log;
	BOOL							m_bReaderInit;		  
	BOOL							m_bReaderOpened;
	BOOL							m_bReaderStarted;
#ifdef SUPPORT_OUTPUT_PINS
	PusherStreamPin					*m_pVideoOut;
	PusherStreamPin					*m_pAudioOut;
#endif
};

#endif