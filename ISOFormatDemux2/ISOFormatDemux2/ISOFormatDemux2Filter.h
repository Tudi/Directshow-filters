#ifndef _WMSOURCE_FILTER_H
#define _WMSOURCE_FILTER_H

#include "VidiatorFilterGuids.h"
#include "defines.h"

#include "../../NxMP4FAPI/nxmp4fapi.h"
#include "ReaderBuffer.h"
#include "C3GPFileReaderClock.h"

#define MAX_VIDEO_TRACK		5
#define MAX_AUDIO_TRACK		5

#define		OT_H263						0xC0
#define		OT_MPEG4Visual				0x20
#define		OT_AMR						0xD0
#define		OT_EVRC						0xD1
#define		OT_QCELP					0xD2
#define		OT_AMRWB					0xD4
#define		OT_AMRWP					0xD5
#define		OT_AAC						0x40
#define		OT_HEAAC					0xE2
#define		OT_EAACP					0xE3
#define		OT_MP3						0x6B
#define		OT_H264						0xC1

typedef enum
{
	READER_OPENED = 0,
	READER_CLOSED,
	READER_STARTED,
	READER_STOPPED, 
	READER_EOF
} READER_STATUS;

class ISOFormatPushPin;
class ISOFormatPushStreamPin;

//Can be used asa async reader
class CVidiatorISOFormatDemuxFilter : public CSource, public IFileSourceFilter, public IISOFormatDemux2 
{
public:
	CVidiatorISOFormatDemuxFilter(LPUNKNOWN pUnk,HRESULT *phr);
	~CVidiatorISOFormatDemuxFilter(void);

	//general use lock. Will span over multiple child objects( ex PINS )
	CCritSec				m_csFilter;
protected:

    // Implements the IBaseFilter and IMediaFilter interfaces
    DECLARE_IUNKNOWN
	/////////////////////////////
	// Override CUnknown / IUnkonwn interface : to create reference of this filter.
	STDMETHODIMP			NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	////////////////////////////
	// Override CBaseFilter / IBaseFilter interface 	
	STDMETHODIMP			Stop();
	STDMETHODIMP			Pause();
	STDMETHODIMP			Run(REFERENCE_TIME tStart);
	int						GetPinCount() { return (m_nVideoTrackCount+m_nAudioTrackCount); }
	CBasePin				*GetPin(int n)
	{
		if( n < m_nVideoTrackCount )
			return (CBasePin*)m_pVideoOut[ n ];
		else if( n < m_nVideoTrackCount + m_nAudioTrackCount )
			return (CBasePin*)m_pAudioOut[ n - m_nVideoTrackCount ];
		return NULL;
	}

	////////////////////////////
	// Implement IFileSourceFilter interface
	STDMETHODIMP			Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP			GetCurFile(LPOLESTR *ppwszFileName, AM_MEDIA_TYPE *pmt);

	//API calls
	STDMETHODIMP			Open( TCHAR *lpszURL );
	STDMETHODIMP			Start();
	STDMETHODIMP			Close();
	STDMETHODIMP			Restart(); //close + open + start

//	BOOL					CheckIOError( )	 { return m_bFileIOError; }
//	int						GetAVTrackCount()	 { return m_nAVTrackCount; }
	int						GetVideoTrackCount() { return m_nVideoTrackCount; }
	int						GetAudioTrackCount() { return m_nAudioTrackCount; }
	S_VIDEO_INFO*			GetVideoInfo(); // !! non thread safe !
	S_AUDIO_INFO*			GetAudioInfo(); // !! non thread safe !
//	NXMP4F_H264_AVCC_BOX	*GetAvccBox(){ return m_pAvcConfig; }
//	int						GetConstLengthNal()	{ return m_nConstLengthNAL; }
//	unsigned char			GetVideoSpecificInfo(unsigned char **buf, unsigned int *size);
	NxMP4FTrackInfo*		GetAudioMultiTrackInfo(int nTrackIndex);
	NxMP4FTrackInfo*		GetVideoMultiTrackInfo(int nTrackIndex);
//	TCHAR*					GetFileName() { return m_szFilePathName; }

protected:
	ISOFormatPushStreamPin	*m_pVideoOut[MAX_VIDEO_TRACK];
	ISOFormatPushStreamPin	*m_pAudioOut[MAX_AUDIO_TRACK];

	static DWORD WINAPI		ReadFileThread( LPVOID pObj );
	int						ReadVideoTrackFromFile();
	int						ReadAudioTrackFromFile();

	void					SetFileReaderStatus(READER_STATUS status) { m_fileReaderStatus = status; }
	void					SetStatus(READER_STATUS status, BOOL bPost = FALSE );
	bool					Start( LONGLONG llStartPos );
	bool					Seek( unsigned int nSeekPos );

protected:
	//maybe should make it thread safe to protect against Start/Stop spams
	BOOL					m_hVideoTracksPendingEOF[MAX_VIDEO_TRACK];
	BOOL					m_hAudioTracksPendingEOF[MAX_AUDIO_TRACK];
	HANDLE					m_hFileThreadFinished;
	bool					m_bVideoInturrupted;
	bool					m_bAudioInturrupted;
	int						m_nFixAudioTimeStamp;
	unsigned int			m_nTimeStampInc;

protected:
	NxMP4FReaderHandle		m_p3GPReader;
	int						m_nAVTrackCount;
	int						m_nAudioTrackCount;
	int						m_nVideoTrackCount;
	READER_STATUS			m_fileReaderStatus;
	NxMP4FTrackInfo			m_pVideoTrackInfo[MAX_VIDEO_TRACK];
	NxMP4FTrackInfo			m_pAudioTrackInfo[MAX_AUDIO_TRACK];
	S_VIDEO_INFO			m_VideoInfo;
	S_AUDIO_INFO			m_AudioInfo;
	NXMP4F_H264_AVCC_BOX	*m_pAvcConfig;
	int						m_nConstLengthNAL;
	TCHAR					m_szFilePathName[MAX_PATH+MAX_PATH];
protected:
	BOOL					m_bStarted;
	BOOL					m_bFileIOError;
	LONGLONG				m_llFileDuration;
	LONGLONG				m_llStartPos;
	////////////////////
	//
	BYTE					*m_pVideoReadBuffer;
	BYTE					*m_pAudioReadBuffer;
	unsigned int			m_nVideoReadBufferLen; 
	unsigned int			m_nAudioReadBufferLen;
	READER_STATUS			m_status; 
	//there are more audio packet then video packets
	LONGLONG				m_nLastVideoTimeStamp[MAX_VIDEO_TRACK];
	LONGLONG				m_nLastAudioTimeStamp[MAX_AUDIO_TRACK]; 
	//stream durations pushed out. This is required to simulate buffer starvation in case renderer does not syncronize audio with video stream
	LONGLONG				m_nVideoDurationPushed[MAX_VIDEO_TRACK];
	LONGLONG				m_nAudioDurationPushed[MAX_AUDIO_TRACK]; 

public:
    // called from output pins for seeking support
    bool					SelectSeekingPin(ISOFormatPushStreamPin* pPin);
    void					DeselectSeekingPin(ISOFormatPushStreamPin* pPin);
    REFERENCE_TIME			GetDuration();
    void					GetSeekingParams(REFERENCE_TIME* ptStart, REFERENCE_TIME* ptStop, double* pdRate);
    HRESULT					Seek(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT					SetRate(double dRate);
    HRESULT					SetStopTime(REFERENCE_TIME tStop);
	BOOL					CanSendSample( REFERENCE_TIME PacketTime ) { return ( PacketTime >= m_tStart && PacketTime <= m_tStop ); }
	void					UpdateSentDuration( int ID, int PinType, LONGLONG Dur )
	{
		if( PinType == 0 && ID < m_nVideoTrackCount )
			m_nVideoDurationPushed[ ID ] += Dur;
		else if( PinType == 1 && ID < m_nAudioTrackCount )
			m_nAudioDurationPushed[ ID ] += Dur;
	}
private:
    // for seeking
    CCritSec				m_csSeeking;
    REFERENCE_TIME			m_tStart;
    REFERENCE_TIME			m_tStop;
    double					m_dRate;		//not yet implemented
    ISOFormatPushStreamPin	*m_pSeekingPin; //only 1 pin at a time has the right to adjust seeking params
};

#endif