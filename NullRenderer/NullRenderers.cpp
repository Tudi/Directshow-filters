#include "StdAfx.h"
#include "NullRenderers.h"

//
// CNullRenderer
//

CNullRenderer::CNullRenderer(REFCLSID clsid, TCHAR* pName, LPUNKNOWN pUnk, HRESULT* phr) 
	: CBaseRenderer(clsid, pName, pUnk, phr)
{

}

//
// CNullVideoRenderer
//
CNullVideoRenderer::CNullVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
	: CNullRenderer(__uuidof(this), NAME("Vidiator Null Video Renderer"), pUnk, phr)
{
}

HRESULT CNullVideoRenderer::CheckMediaType(const CMediaType* pmt)
{

	return (pmt->majortype == MEDIATYPE_Video && 
		pmt->subtype == MEDIASUBTYPE_RGB24)?S_OK: E_FAIL;

#if 0
		&& ( //pmt->subtype == MEDIASUBTYPE_YV12
		|| pmt->subtype == MEDIASUBTYPE_I420
		|| pmt->subtype == MEDIASUBTYPE_YUYV
		|| pmt->subtype == MEDIASUBTYPE_IYUV
		|| pmt->subtype == MEDIASUBTYPE_YVU9
		|| pmt->subtype == MEDIASUBTYPE_Y411
		|| pmt->subtype == MEDIASUBTYPE_Y41P
		|| pmt->subtype == MEDIASUBTYPE_YUY2
		|| pmt->subtype == MEDIASUBTYPE_YVYU
		|| pmt->subtype == MEDIASUBTYPE_UYVY
		|| pmt->subtype == MEDIASUBTYPE_Y211
		|| pmt->subtype == MEDIASUBTYPE_RGB1
		|| pmt->subtype == MEDIASUBTYPE_RGB4
		|| pmt->subtype == MEDIASUBTYPE_RGB8
		|| pmt->subtype == MEDIASUBTYPE_RGB565
		|| pmt->subtype == MEDIASUBTYPE_RGB555
		|| pmt->subtype == MEDIASUBTYPE_RGB24
		|| pmt->subtype == MEDIASUBTYPE_RGB32
		|| pmt->subtype == MEDIASUBTYPE_ARGB1555
		|| pmt->subtype == MEDIASUBTYPE_ARGB4444
		|| pmt->subtype == MEDIASUBTYPE_ARGB32
		|| pmt->subtype == MEDIASUBTYPE_A2R10G10B10
		|| pmt->subtype == MEDIASUBTYPE_A2B10G10R10)
		? S_OK
		: E_FAIL;


	return S_OK;
#endif 
}

//
// CNullAudioRenderer
//

CNullAudioRenderer::CNullAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CNullRenderer(__uuidof(this), NAME("Vidiator Null Audio Renderer"), pUnk, phr)
{
}

HRESULT CNullAudioRenderer::CheckMediaType(const CMediaType* pmt)
{

	return (pmt->majortype == MEDIATYPE_Audio && 
		pmt->subtype == MEDIASUBTYPE_PCM)?S_OK: E_FAIL;
#if 0
	return pmt->majortype == MEDIATYPE_Audio
		&& (pmt->subtype == MEDIASUBTYPE_PCM
		|| pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT
		|| pmt->subtype == MEDIASUBTYPE_DRM_Audio
		|| pmt->subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF
		|| pmt->subtype == MEDIASUBTYPE_RAW_SPORT
		|| pmt->subtype == MEDIASUBTYPE_SPDIF_TAG_241h)
		? S_OK
		: E_FAIL;
	return S_OK;
#endif
}
