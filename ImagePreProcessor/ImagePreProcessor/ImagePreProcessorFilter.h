#pragma once

#include "FilterGuids.h"
#include <memory>
#include "ResamplerLib/RatioResampler2.h"
#include "VideoTransform/DeInterlace.h"
#include "VideoTransform/GreenEnhance.h"
#include "VideoTransform/DeNoise.h"

//debugging tool to dump filter YUV input and output to a file that you can open with a YUV movie player(raw format)
//#define DEBUG_DUMP_YUV_FRAMES_COUNT	30

enum ResizeAlgorithm
{
	ResizeAlgorithmBilinear = 0,
	ResizeAlgorithmLinear	= 1,
};

class CImagePreProcessorFilter;

class CYuvTransOutputPin : public CTransformOutputPin
{
public:
    CYuvTransOutputPin(
        __in_opt LPCTSTR pObjectName,
        __inout CTransformFilter *pTransformFilter,
        __inout HRESULT * phr,
        __in_opt LPCWSTR pName);
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT SetMediaType(const CMediaType* pmt);
	HRESULT DecideBufferSize( IMemAllocator * pAlloc,__inout ALLOCATOR_PROPERTIES *pProp);
	HRESULT DecideAllocator( IMemInputPin * pPin, __deref_out IMemAllocator ** pAlloc );
	//required to implement media type iterator
    HRESULT GetMediaType( int iPosition, __inout CMediaType *pMediaType);
	IMemAllocator* GetAllocator() { return m_pAllocator; };

	// Used to create output queue objects
    HRESULT Active();
    HRESULT Inactive();

	// Overriden to pass data to the output queues
    HRESULT Deliver(IMediaSample *pMediaSample);
    HRESULT DeliverEndOfStream();
    HRESULT DeliverBeginFlush();
    HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);
protected:
	CTransformFilter *m_pTransformFilter;
    friend class CImagePreProcessorFilter;

	COutputQueue*	m_pOutputQueue;
};

class CYuvTransInputPin : public CTransformInputPin
{
public:
    CYuvTransInputPin(
        __in_opt LPCTSTR pObjectName,
        __inout CTransformFilter *pTransformFilter,
        __inout HRESULT * phr,
        __in_opt LPCWSTR pName);
    HRESULT CheckMediaType(const CMediaType* pmt);

	// This will cause procesing to be done on separate threads
	STDMETHODIMP ReceiveCanBlock() {return S_OK;};
};

class CImagePreProcessorFilter : public CTransformFilter, public ISpecifyPropertyPages, public IImageResize
{

	friend class CImagePreProcessorInputPin;
public:
	CImagePreProcessorFilter(LPUNKNOWN pUnk,HRESULT *phr);
	~CImagePreProcessorFilter(void);

protected:

    // Implements the IBaseFilter and IMediaFilter interfaces
    DECLARE_IUNKNOWN
    // Overriden to say what interfaces we support where:
    // ISpecifyPropertyPages, IMpeg2PsiParser
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);


	// IImagePreProcessor interface
    HRESULT STDMETHODCALLTYPE GetImageInputSize( 
        /* [out] */ long *pWidth,
        /* [out] */ long *pHeight);
    
    HRESULT STDMETHODCALLTYPE GetImageResizeEnabled( 
        /* [out] */ boolean *pEnabled);
    
    HRESULT STDMETHODCALLTYPE GetImageOutSize( 
        /* [out] */ long *pWidth,
        /* [out] */ long *pHeight);
    
    HRESULT STDMETHODCALLTYPE SetImageOutSize( 
        /* [in] */ long nWidth,
        /* [in] */ long nHeight,
		/* [in] */ bool bKeepAspectRatio,
		/* [in] */ long nDesiredFPS);

    HRESULT STDMETHODCALLTYPE GetFilterType( 
        /* [out] */ long *pFilterType);

	HRESULT STDMETHODCALLTYPE SetFilterType( 
        /* [in] */ long nFilterType);

	HRESULT STDMETHODCALLTYPE SetCropParams( 
        /* [in] */ long nOffsetLeft,
        /* [in] */ long nOffsetTop,
        /* [in] */ long nWidth,
        /* [in] */ long nHeight
		);

    HRESULT STDMETHODCALLTYPE SetAspectRatio(REFERENCE_TIME nSampleTimestamp, long nAspectRatioWidth, long nAspectRatioHeight);
	HRESULT STDMETHODCALLTYPE GetAspectRatio(long* pAspectRatioWidth, long* pAspectRatioHeight);

	// Send EndOfStream if no input connection
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();


	// CTransform Filter pure functions.
    HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);


	// check if you can support mtIn
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckOutputType(const CMediaType* mtIn);

    // check if you can support the transform from this input to this output
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);

    // override this to know when the media type is actually set
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

    // chance to grab extra interfaces on connection
    HRESULT CheckConnect(PIN_DIRECTION dir,IPin *pPin);
    HRESULT BreakConnect(PIN_DIRECTION dir);
    HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);

    // call the SetProperties function with appropriate arguments
    HRESULT DecideBufferSize(
                        IMemAllocator * pAllocator,
                        ALLOCATOR_PROPERTIES *pProperties);

    // override to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, __inout CMediaType *pMediaType);


    HRESULT Receive(IMediaSample *pSample);

private:

	HRESULT initializeTransform(bool bForce);
	void cleanUp();
	HRESULT canPassThrough(IMediaSample* pSample);

	CMediaType					m_InputMediaType;
	CMediaType					m_OutputMediaType;
	BITMAPINFOHEADER			m_biInput;
	BITMAPINFOHEADER			m_biOutput;
	_VIDEOTransType				m_Type;			
	// for : VIDEOTrans_Resize
	CRatioResampler2			*m_pResampler;
	volatile REFERENCE_TIME		m_nAspectRatioChangeTimestamp;
	volatile long				m_nNewAspectRatioWidth;
	volatile long				m_nNewAspectRatioHeight;
	int							m_nResizeCurrentFPS;
	int							m_nResizeFrames;
	DWORD						m_nResizeTicks;
	int							m_nResizeDesiredFPS;
	BOOL						m_bKeepAspectRatio;
	double						m_fInputAspectRatio;
	// for : VIDEOTrans_Crop
	CRatioResampler2			*m_Crop;
	int							m_nCropTop, m_nCropLeft, m_nCropWidth, m_nCropHeight;
	// for : VIDEOTrans_Deinter
	DeInterlace					*m_DeInterlace;
	// for : VIDEOTrans_Denoise
	DeNoise						*m_DeNoise;
	// for : VIDEOTrans_GreenEnhance
	GreenEnhance				*m_GreenEnhance;
	// for : VIDEOTrans_ColorConv
	bool						m_ColorConvert;
	bool						m_bMediatypeChanged;

#ifdef DEBUG_DUMP_YUV_FRAMES_COUNT
	FILE	*YUVFileOut,*YUVFileIn;
	int		FramesDumpedIn,FramesDumpedOut;
#endif
};
