#pragma once

class CNullRenderer : public CBaseRenderer
{
protected:
	HRESULT DoRenderSample(IMediaSample* pSample) {return S_OK;}

public:
	CNullRenderer(REFCLSID clsid, TCHAR* pName, LPUNKNOWN pUnk, HRESULT* phr);
};


class DECLSPEC_UUID("E8BF88B8-6A62-418c-B49A-2245A7805F14")
CNullVideoRenderer : public CNullRenderer
{
protected:
	HRESULT CheckMediaType(const CMediaType* pmt);

public:
	CNullVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr);
};

class DECLSPEC_UUID("1F02D5BA-38E9-48e4-90B3-D037A62F8660")
CNullAudioRenderer : public CNullRenderer
{
protected:
	HRESULT CheckMediaType(const CMediaType* pmt);

public:
	CNullAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr);
};



