#include "stdafx.h"
#include "MediaTypeUtil.h"
#include <initguid.h>
#include "ExtraGuids.h"

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

bool GetTypeMediaTyeParams(CMediaType* pmt, int nType, MediaStreamInfoParam *pmsi)
{
    switch (nType)
    {
    case OT_H264:
        return GetType_H264(pmt,pmsi);
    case OT_MPEG4Visual:
        return GetType_Mpeg4V(pmt,pmsi);
    case OT_H263:
        return GetType_H263(pmt,pmsi);
    case OT_AAC:
        return GetType_AAC(pmt,pmsi);
    case OT_EAACP:
        return GetType_EAACP(pmt,pmsi);
	case OT_HEAAC:
        return GetType_HEAAC(pmt,pmsi);
    case OT_MP3:
        return GetType_MP3(pmt,pmsi);
    case OT_AMR:
        return GetType_AMR(pmt,pmsi);
	case OT_AMRWB: //<dt4634>[jerry:2010-12-22]
		return GetType_AMRWB(pmt,pmsi);
	case OT_EVRC:
		return GetType_EVRC(pmt,pmsi);
	case OT_QCELP:
		return GetType_QCELP(pmt,pmsi); 
/*    case Audio_WAVEFORMATEX:
        return GetType_WAVEFORMATEX(pmt,pmsi);
	case Audio_PCM:
		return GetType_PCM(pmt,pmsi);*/
	}

    return false;
}

#if 1
bool GetType_H264(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Video);

    const DWORD AVC1FOURCC = DWORD('1CVA');
    FOURCCMap avc1(AVC1FOURCC);
    pmt->SetSubtype(&avc1);
    pmt->SetFormatType(&FORMAT_MPEG2Video);

    // following the Nero/Elecard standard, we use an mpeg2videoinfo block
    // with dwFlags for nr of bytes in length field, and param sets in
    // sequence header (allows 1 DWORD already -- extend this).

    const BYTE* pconfig = pmsi->m_pDecoderSpecific;
    // count param set bytes (including 2-byte length)
    int cParams = 0;
    int cSeq = pconfig[5-5] & 0x1f;
    const BYTE* p = &pconfig[6-5];
    for (int i = 0; i < cSeq; i++)
    {
        int c = 2 + (p[0] << 8) + p[1];
        cParams += c;
        p += c;
    }
    int cPPS = *p++;
    for (int i = 0; i < cPPS; i++)
    {
        int c = 2 + (p[0] << 8) + p[1];
        cParams += c;
        p += c;
    }


//    int cFormat = sizeof(MPEG2VIDEOINFO) + cParams - sizeof(DWORD);
    int cFormat = sizeof(MPEG2VIDEOINFO) + cParams;
    MPEG2VIDEOINFO* pVI = (MPEG2VIDEOINFO*)pmt->AllocFormatBuffer(cFormat);
    ZeroMemory(pVI, cFormat);
    pVI->hdr.bmiHeader.biSize = sizeof(pVI->hdr.bmiHeader);
    pVI->hdr.bmiHeader.biWidth = pmsi->m_cx;
    pVI->hdr.bmiHeader.biHeight = pmsi->m_cy;
    pVI->hdr.AvgTimePerFrame = pmsi->m_tFrame;
    pVI->hdr.bmiHeader.biPlanes = 1;
    pVI->hdr.bmiHeader.biBitCount = 24;
    pVI->hdr.bmiHeader.biCompression = AVC1FOURCC;
    pVI->dwProfile = pconfig[1+3];
    pVI->dwLevel = pconfig[3+3];

    // nr of bytes used in length field, for length-preceded NALU format.
//    pVI->dwFlags = (pconfig[4] & 3) + 1;
	pVI->dwFlags = pmsi->m_AVCLengthSizeMinusOne+1;
    pVI->cbSequenceHeader = cParams;

    // copy all param sets
    cSeq = pconfig[5-5] & 0x1f;
    p = &pconfig[6-5];
    BYTE* pDest = (BYTE*) &pVI->dwSequenceHeader;
    for (int i = 0; i < cSeq; i++)
    {
        int c = 2 + (p[0] << 8) + p[1];
        CopyMemory(pDest, p, c);
        pDest += c;
        p += c;
    }
    cPPS = *p++;
    for (int i = 0; i < cPPS; i++)
    {
        int c = 2 + (p[0] << 8) + p[1];
        CopyMemory(pDest, p, c);
        pDest += c;
        p += c;
    }

    return true;
}
#endif

#if 0
void header_3gp_to_annex_b(BYTE *dat, int size, BYTE *&convdat, int &convsize)
{
	BYTE *dptr=dat;
	int i,n,len,k;
	int alocsz=size*2+512;
	BYTE *outd=(BYTE *) malloc(alocsz);
	BYTE *oup=outd;
	convdat = NULL;
	convsize = 0;
	for (k=0; k<2; k++)
	{
		n=dptr[0];
		for (i=0; i<n; i++)
		{
			if (dptr+3>dat+size) 
				return;
			len=256*dptr[1]+dptr[2];			
			dptr+=3;
			if (dptr+len>dat+size) 
				return;
			if (oup+len+4>outd+alocsz) 
				return;
			memcpy(oup,"\0\0\0\1",4);
			oup+=4;
			memcpy(oup,dptr,len);
			dptr+=len;
			oup+=len;
		}
	}
	convdat=outd;
	convsize=oup-outd;
}

bool GetType_H264(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Video);
    const DWORD AVC1FOURCC = DWORD('1CVA');
    FOURCCMap avc1(AVC1FOURCC);
    pmt->SetSubtype(&avc1);
    pmt->SetFormatType(&FORMAT_VideoInfo);
	BYTE *DSI;
	int DSISize;
	header_3gp_to_annex_b( pmsi->m_pDecoderSpecific, pmsi->m_cDecoderSpecific, DSI, DSISize );
    VIDEOINFOHEADER* pVI = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + DSISize);
    ZeroMemory(pVI, sizeof(VIDEOINFOHEADER));
    pVI->bmiHeader.biSize = sizeof(pVI->bmiHeader);
    pVI->bmiHeader.biPlanes = 1;
    pVI->bmiHeader.biBitCount = 24;
    pVI->bmiHeader.biWidth = pmsi->m_cx;
    pVI->bmiHeader.biHeight = pmsi->m_cy;
    pVI->bmiHeader.biCompression = AVC1FOURCC;
    pVI->AvgTimePerFrame = pmsi->m_tFrame;

    BYTE* pDecSpecific = (BYTE*)(pVI+1);
    CopyMemory(pDecSpecific, DSI,  DSISize);
	free( DSI );

    return true;
}
#endif

bool GetType_H263(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    const DWORD H263FOURCC = DWORD('362s');

    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Video);
    FOURCCMap h263(H263FOURCC);
    pmt->SetSubtype(&h263);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    VIDEOINFOHEADER* pVI = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pmsi->m_cDecoderSpecific);
    ZeroMemory(pVI, sizeof(VIDEOINFOHEADER));
    pVI->bmiHeader.biSize = sizeof(pVI->bmiHeader);
    pVI->bmiHeader.biPlanes = 1;
    pVI->bmiHeader.biBitCount = 24;
    pVI->bmiHeader.biWidth = pmsi->m_cx;
    pVI->bmiHeader.biHeight = pmsi->m_cy;
    pVI->bmiHeader.biCompression = H263FOURCC;
    pVI->AvgTimePerFrame = pmsi->m_tFrame;

    BYTE* pDecSpecific = (BYTE*)(pVI+1);
    CopyMemory(pDecSpecific, pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

    return true;
}

bool GetType_Mpeg4V(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    const DWORD DIVXFOURCC = DWORD('XVID');

    // for standard mpeg-4 video, we set the media type
    // up for the DivX decoder
    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Video);
    FOURCCMap divx(DIVXFOURCC);
    pmt->SetSubtype(&divx);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    VIDEOINFOHEADER* pVI = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pmsi->m_cDecoderSpecific);
    ZeroMemory(pVI, sizeof(VIDEOINFOHEADER));
    pVI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pVI->bmiHeader.biPlanes = 1;
    pVI->bmiHeader.biBitCount = 24;
    pVI->bmiHeader.biWidth = pmsi->m_cx;
    pVI->bmiHeader.biHeight = pmsi->m_cy;
    pVI->bmiHeader.biSizeImage = DIBSIZE(pVI->bmiHeader);
    pVI->bmiHeader.biCompression = DIVXFOURCC;
    pVI->AvgTimePerFrame = pmsi->m_tFrame;

    BYTE* pDecSpecific = (BYTE*)(pVI+1);
    CopyMemory(pDecSpecific, pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

    return true;
}

// static
const int SamplingFrequencies[] =
{
    96000,
    88200,
    64000,
    48000,
    44100,
    32000,
    24000,
    22050,
    16000,
    12000,
    11025,
    8000,
    7350,
    0,
    0,
    0,
};

bool GetType_AAC(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    // set for Free AAC Decoder faad

    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Audio);
    FOURCCMap faad(WAVE_FORMAT_AAC);
    pmt->SetSubtype(&faad);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);
    WAVEFORMATEX* pwfx;
	{
		pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
	    ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
		pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
		CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);
	}/**/
/*	{
		pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer( sizeof(WAVEFORMATEX) );
	    ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
		pwfx->cbSize = 0;
	}/**/

    // parse decoder-specific info to get rate/channels
    long samplerate = ((pmsi->m_pDecoderSpecific[0] & 0x7) << 1) + ((pmsi->m_pDecoderSpecific[1] & 0x80) >> 7);
    pwfx->nSamplesPerSec = SamplingFrequencies[samplerate];
//    pwfx->nBlockAlign = 1;
    pwfx->nBlockAlign = 4;
    pwfx->wBitsPerSample = pmsi->m_llBitPerSample;
    pwfx->wFormatTag = WAVE_FORMAT_AAC;
    pwfx->nChannels = (pmsi->m_pDecoderSpecific[1] & 0x78) >> 3;
	pwfx->nAvgBytesPerSec = pmsi->m_BytesPerSec;
	pmt->bFixedSizeSamples = FALSE;

    return true;
}

bool GetType_EAACP(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
	GetType_AAC( pmt, pmsi );
	//i'm not an audio decoder specialist. EAACP might have double sampling rate. Our 3gp API should have determined correct sampling rate
	if( pmt->formattype == FORMAT_WaveFormatEx && pmt->pbFormat != NULL )
	{
		WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->pbFormat;
		pwfx->nSamplesPerSec = pmsi->m_llSamplingRate; //get this from 3GP
	}
	return true;
}

bool GetType_HEAAC(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
	GetType_AAC( pmt, pmsi );
	//i'm not an audio decoder specialist. HEAAC has double sampling rate. Our 3gp API should have determined correct sampling rate
	if( pmt->formattype == FORMAT_WaveFormatEx && pmt->pbFormat != NULL )
	{
		WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->pbFormat;
		pwfx->nSamplesPerSec = pmsi->m_llSamplingRate; //get this from 3GP
	}
	return true;
}

bool GetType_MP3(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    // set for Free AAC Decoder faad
    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Audio);
    FOURCCMap faad(WAVE_FORMAT_MP3);
    pmt->SetSubtype(&faad);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);
    WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
    ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
    pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
    CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

    pwfx->nSamplesPerSec = pmsi->m_llSamplingRate;
//    pwfx->nBlockAlign = 1;
    pwfx->nBlockAlign = 4;
    pwfx->wBitsPerSample = pmsi->m_llBitPerSample;
    pwfx->wFormatTag = WAVE_FORMAT_MP3;
    pwfx->nChannels = pmsi->m_llChannelCount;

    return true;
}

bool GetType_AMR(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    const DWORD AMRFOURCC = DWORD('rmas');
    FOURCCMap SAMR(AMRFOURCC);

    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Audio);
    FOURCCMap amr(SAMR);
    pmt->SetSubtype(&amr);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);
	pmt->lSampleSize = 256 * 1024; //should fit
    WAVEFORMATEX* pwfx;
/*	{
		pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
		pwfx->cbSize = 0;
	}/**/
	{
		pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
		ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
		pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
		CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);
	}/**/

    pwfx->nSamplesPerSec = pmsi->m_llSamplingRate;
//    pwfx->nBlockAlign = 0;
    pwfx->nBlockAlign = 4;
    pwfx->wBitsPerSample = pmsi->m_llBitPerSample;
	pwfx->nAvgBytesPerSec = pmsi->m_BytesPerSec;
    pwfx->nChannels = pmsi->m_llChannelCount;
    if (!(AMRFOURCC & 0xffff0000))
        pwfx->wFormatTag = AMRFOURCC;

    return true;
}

bool GetType_AMRWB(CMediaType* pmt, MediaStreamInfoParam *pmsi) //<dt4634>[jerry:2010-12-22]
{

	const DWORD AMRWBFOURCC = DWORD('bwas');
	//FOURCCMap SAMR(AMRFOURCC);

	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Audio);
#if 0
	FOURCCMap amr(SAMR);
	pmt->SetSubtype(&amr);
	pmt->SetFormatType(&FORMAT_WaveFormatEx);
#else	
	pmt->SetSubtype( &MEDIASUBTYPE_AMRWB_VID );
	pmt->SetFormatType( &FORMAT_NxWaveFormatEx );
#endif 
	WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
	ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
	pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
	CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

	pwfx->nSamplesPerSec = pmsi->m_llSamplingRate;
	pwfx->nBlockAlign = 0;
	pwfx->wBitsPerSample = pmsi->m_llBitPerSample;
	if (!(AMRWBFOURCC & 0xffff0000))
		pwfx->wFormatTag = AMRWBFOURCC;

	pwfx->nChannels = pmsi->m_llChannelCount;
	return true;
}

//2010/03/30 mark yh kim -  PCM
bool GetType_PCM(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{

	//const DWORD AMRFOURCC = DWORD('sowt');
	//const DWORD TOWFOURCC = DWORD('twos');
	// FOURCCMap Stow(TOWFOURCC);
	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Audio);
	
	//pmt->SetSubtype(&Stow);
	FOURCCMap pcm(WAVE_FORMAT_PCM);
	pmt->SetSubtype(&pcm);
	pmt->SetFormatType(&FORMAT_WaveFormatEx);
	WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
	ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
	pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
	CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

	pwfx->nSamplesPerSec = pmsi->m_llSamplingRate;
	pwfx->nBlockAlign = 0;
	pwfx->wBitsPerSample = pmsi->m_llBitPerSample;
	//if (!(TOWFOURCC & 0xffff0000))
	//	pwfx->wFormatTag = TOWFOURCC;
	pwfx->wFormatTag =  WAVE_FORMAT_PCM;

	pwfx->nChannels = pmsi->m_llChannelCount;
	return true;
}

bool GetType_WAVEFORMATEX(CMediaType* pmt, MediaStreamInfoParam *pmsi)
{
    // common to standard audio types that have known atoms
    // in the mpeg-4 file format

    // the dec-specific info is a waveformatex
    WAVEFORMATEX* pwfx = (WAVEFORMATEX*)(BYTE*)pmsi->m_pDecoderSpecific;
    if ((pmsi->m_cDecoderSpecific < sizeof(WAVEFORMATEX)) ||
            (pmsi->m_cDecoderSpecific < int(sizeof(WAVEFORMATEX) + pwfx->cbSize)))
    {
        return false;
    }

    pmt->InitMediaType();
    pmt->SetType(&MEDIATYPE_Audio);
    FOURCCMap subtype(pwfx->wFormatTag);
    pmt->SetSubtype(&subtype);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);

    int cLen = pwfx->cbSize + sizeof(WAVEFORMATEX);
    WAVEFORMATEX* pwfxMT = (WAVEFORMATEX*)pmt->AllocFormatBuffer(cLen);
    CopyMemory(pwfxMT, pwfx, cLen);

    return true;
}

bool GetType_EVRC(CMediaType* pmt, MediaStreamInfoParam *pmsi) 
{

	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Audio);

	pmt->SetSubtype( &MEDIASUBTYPE_EVRC_VID );
	pmt->SetFormatType( &FORMAT_NxWaveFormatEx );

	WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
	ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
	pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
	CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

	pwfx->nSamplesPerSec = pmsi->m_llSamplingRate;
	pwfx->nBlockAlign = 0;
	pwfx->wBitsPerSample = pmsi->m_llBitPerSample;
	

	pwfx->nChannels = pmsi->m_llChannelCount;
	return true;
}

bool GetType_QCELP(CMediaType* pmt, MediaStreamInfoParam *pmsi) 
{

	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Audio);

	pmt->SetSubtype( &MEDIASUBTYPE_QCELP_VID );
	pmt->SetFormatType( &FORMAT_NxWaveFormatEx );

	WAVEFORMATEX* pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmsi->m_cDecoderSpecific);
	ZeroMemory(pwfx,  sizeof(WAVEFORMATEX));
	pwfx->cbSize = WORD(pmsi->m_cDecoderSpecific);
	CopyMemory((pwfx+1),  pmsi->m_pDecoderSpecific,  pmsi->m_cDecoderSpecific);

	pwfx->nSamplesPerSec = pmsi->m_llSamplingRate;
	pwfx->nBlockAlign = 0;
	pwfx->wBitsPerSample = pmsi->m_llBitPerSample;


	pwfx->nChannels = pmsi->m_llChannelCount;
	return true;
}