#ifndef _MEDIA_FORMAT_UTILS_H_
#define _MEDIA_FORMAT_UTILS_H_

struct MediaStreamInfoParam
{
	long	m_cDecoderSpecific;
	BYTE	*m_pDecoderSpecific;
	int		m_cx;
	int		m_cy;
	int		m_tFrame;
	int		m_llBitPerSample;
	int		m_llSamplingRate;
	int		m_llChannelCount;
	int		m_AVCLengthSizeMinusOne;
	int		m_BytesPerSec;
};

bool GetTypeMediaTyeParams(CMediaType* pmt, int nType, MediaStreamInfoParam *pmsi );
bool GetType_H264(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_H263(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_Mpeg4V(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_AAC(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_EAACP(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_HEAAC(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_MP3(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_AMR(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_AMRWB(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_EVRC(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_QCELP(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_PCM(CMediaType* pmt, MediaStreamInfoParam *pmsi);
bool GetType_WAVEFORMATEX(CMediaType* pmt, MediaStreamInfoParam *pmsi);

#endif