#include "StdAfx.h"
#include "ImagePreProcessorFilter.h"
#include "ImagePreProcessorProperties.h"
#include "FilterGuids.h"
#include "utils.h"
#include "ExtraGuids.h"
#include "ResamplerLib/BilinearResampler.h"
#include "ResamplerLib/LinearResampler.h"
#include "nxColorConversionLib/nxColorConversion.h"
#include "nxColorConversionLib/PixelFormat.h"

//#define _CHECK_RESIZE_TIME

#define BYTE_ALIGN(X) (X&(~3))

#define INPUT_PIN					((CYuvTransInputPin*)m_pInput)
#define OUTPUT_PIN					((CYuvTransOutputPin*)m_pOutput)
const LONG DEFAULT_BUFFERS	= 2;

#ifdef DEBUG_DUMP_YUV_FRAMES_COUNT
	#define DUMP_A_YUV_OUTPUT(x) if( FramesDumpedOut < DEBUG_DUMP_YUV_FRAMES_COUNT ) \
			{ \
				if( YUVFileOut == NULL ) \
				{ \
					char filename[500]; \
					sprintf( filename, "C:\\temp\\VIDIIPP_OUT_YUV_%d_%d_%d_%d_%d.YUV",m_biOutput.biWidth,m_biOutput.biHeight,(int)(this),(int)GetTickCount(),x ); \
					YUVFileOut = fopen( filename, "wb" ); \
				} \
				if( YUVFileOut != NULL && pDestBuffer != NULL )\
				{\
					fwrite( pDestBuffer, m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2, 1, YUVFileOut ); \
					FramesDumpedOut++; \
				}\
			} 
	#define DUMP_A_YUV_INPUT if( FramesDumpedIn < DEBUG_DUMP_YUV_FRAMES_COUNT ) \
			{ \
				if( YUVFileIn == NULL ) \
				{ \
					char filename[500]; \
					sprintf( filename, "C:\\temp\\VIDIIPP_IN_YUV_%d_%d_%d_%d_%d.YUV",m_biOutput.biWidth,m_biOutput.biHeight,(int)(this),(int)GetTickCount(),lSourceSize ); \
					YUVFileIn = fopen( filename, "wb" ); \
				} \
				if( YUVFileOut != NULL && pSourceBuffer != NULL )\
				{\
					fwrite( pSourceBuffer, m_biInput.biWidth * m_biInput.biHeight * 3 / 2, 1, YUVFileIn ); \
					FramesDumpedIn++; \
				}\
			} 
#else
	#define DUMP_A_YUV_OUTPUT(x)
	#define DUMP_A_YUV_INPUT
#endif

void* fast_memcpy(LPBYTE dst, LPBYTE src, size_t count)
{
	// tested
	if (((((uintptr_t) dst) & 0xf) == 0) && ((((uintptr_t) src) & 0xf) == 0))
	{
		while (count >= (16*4))
		{
			_mm_stream_si128((__m128i *) (dst+ 0*16),  _mm_load_si128((__m128i *) (src+ 0*16)));
			_mm_stream_si128((__m128i *) (dst+ 1*16),  _mm_load_si128((__m128i *) (src+ 1*16)));
			_mm_stream_si128((__m128i *) (dst+ 2*16),  _mm_load_si128((__m128i *) (src+ 2*16)));
			_mm_stream_si128((__m128i *) (dst+ 3*16),  _mm_load_si128((__m128i *) (src+ 3*16)));
			count -= 16*4;
			src += 16*4;
			dst += 16*4;
		}
	}
	else
	{
		while (count >= (16*4))
		{
			_mm_storeu_si128((__m128i *) (dst+ 0*16),  _mm_loadu_si128((__m128i *) (src+ 0*16)));
			_mm_storeu_si128((__m128i *) (dst+ 1*16),  _mm_loadu_si128((__m128i *) (src+ 1*16)));
			_mm_storeu_si128((__m128i *) (dst+ 2*16),  _mm_loadu_si128((__m128i *) (src+ 2*16)));
			_mm_storeu_si128((__m128i *) (dst+ 3*16),  _mm_loadu_si128((__m128i *) (src+ 3*16)));
			count -= 16*4;
			src += 16*4;
			dst += 16*4;
		}
	}

	while (count --)
		*dst++ = *src++;

	return dst;
}

CImagePreProcessorFilter::CImagePreProcessorFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CTransformFilter( NAME("CImagePreProcessorFilter"), pUnk, CLSID_ImagePreProcessor ),
	m_pResampler(NULL),
	m_nAspectRatioChangeTimestamp(-1000000000),
	m_nNewAspectRatioWidth(0),
	m_nNewAspectRatioHeight(0),
	m_bMediatypeChanged(false)
{

	m_pInput = new CYuvTransInputPin(NAME("Transform input pin"),
									  this,              // Owner filter
									  phr,               // Result code
									  L"Video");      // Pin name

	//  Can't fail
	ASSERT(SUCCEEDED(*phr));
	m_pOutput = new CYuvTransOutputPin(NAME("Transform output pin"),
										this,            // Owner filter
										phr,             // Result code
										L"Video");   // Pin name
	ASSERT(SUCCEEDED(*phr));
	m_Type = VIDEOTrans_Unknown;
	m_DeInterlace = NULL;
	m_GreenEnhance = NULL;
	m_DeNoise = NULL;
	m_Crop = NULL;
	m_ColorConvert = false;
	memset( &m_biInput, 0, sizeof( m_biInput ) );
	memset( &m_biOutput, 0, sizeof( m_biOutput ) );
	m_nCropTop = 0;
	m_nCropLeft = 0;
	m_nCropWidth = 0;
	m_nCropHeight = 0;
	m_nResizeCurrentFPS = 0;
	m_nResizeFrames = 0;
	m_nResizeTicks = 0;
	m_nResizeDesiredFPS = 45;
	m_bKeepAspectRatio = FALSE;
	m_fInputAspectRatio = 0.0;
#ifdef DEBUG_DUMP_YUV_FRAMES_COUNT
	YUVFileOut = NULL;
	YUVFileIn = NULL;
	FramesDumpedOut = 0;
	FramesDumpedIn = 0;
#endif

}

CImagePreProcessorFilter::~CImagePreProcessorFilter(void)
{
	cleanUp();
	m_Type = VIDEOTrans_Unknown;
#ifdef DEBUG_DUMP_YUV_FRAMES_COUNT
	if( YUVFileOut )
		fclose( YUVFileOut );
	if( YUVFileIn )
		fclose( YUVFileIn );
#endif
}
//

// NonDelegatingQueryInterface
//
// Reveals IMpeg2PsiParser and ISpecifyPropertyPages
//
STDMETHODIMP CImagePreProcessorFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
	IMPLEMENT_QUERY_INTERFACE( ISpecifyPropertyPages );
	if (riid == __uuidof(IImageResize))
	{		
        return GetInterface((IImageResize*)this, ppv);
	}
	return __super::NonDelegatingQueryInterface(riid, ppv);
} // NonDelegatingQueryInterface

//
// GetPages
//
// This is the sole member of ISpecifyPropertyPages
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CImagePreProcessorFilter::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages,E_POINTER);
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	CheckPointer(pPages->pElems,E_OUTOFMEMORY);
    *(pPages->pElems) = CLSID_ImagePreProcessorProperties;
    return NOERROR;
} // GetPages

// Send EndOfStream if no input connection
STDMETHODIMP CImagePreProcessorFilter::Run(REFERENCE_TIME tStart)
{
	return __super::Run(tStart);
}

void CImagePreProcessorFilter::cleanUp()
{

	if( m_pResampler )
	{
		delete m_pResampler;
		m_pResampler = NULL;
	}

	if( m_DeInterlace )
	{
		delete m_DeInterlace;
		m_DeInterlace = NULL;
	}

	if( m_GreenEnhance )
	{
		delete m_GreenEnhance;
		m_GreenEnhance = NULL;
	}

	if( m_DeNoise )
	{
		delete m_DeNoise;
		m_DeNoise = NULL;
	}

	if( m_Crop )
	{
		delete m_Crop;
		m_Crop = NULL;
	}

	m_ColorConvert = false;

}

HRESULT CImagePreProcessorFilter::initializeTransform(bool bForce)
{

	if (bForce)
	{
		// clean up all pre-processors when media format changed
		cleanUp();
	}

	if( m_Type == VIDEOTrans_Resize && m_pResampler == NULL )
	{

		if( m_biInput.biWidth > 0 && m_biInput.biHeight > 0 && m_biOutput.biWidth > 0 && m_biOutput.biHeight > 0 )
		{

			LONG nInputWidth = m_biInput.biWidth;
			LONG nInputHeight = m_biInput.biHeight;
			m_fInputAspectRatio = 0.0;
			double fOutputAspectRatio = 0.0;

			if (m_bKeepAspectRatio)
			{
				m_fInputAspectRatio = (double)nInputWidth / nInputHeight;
				fOutputAspectRatio = (double)m_biOutput.biWidth / m_biOutput.biHeight;

				VIDEOINFOHEADER2* vih = NULL;

				if (m_InputMediaType.pbFormat)
				{
					if (m_InputMediaType.formattype == FORMAT_VideoInfo2)
					{
						vih = (VIDEOINFOHEADER2*)m_InputMediaType.pbFormat;
					}
					else if (m_InputMediaType.formattype == FORMAT_MPEG2_VIDEO)
					{
						vih = &((MPEG2VIDEOINFO*)m_InputMediaType.pbFormat)->hdr;
					}
					else if (m_InputMediaType.formattype == FORMAT_DiracVideoInfo)
					{
						vih = &((DIRACINFOHEADER*)m_InputMediaType.pbFormat)->hdr;
					}
					if (vih && vih->dwPictAspectRatioX > 0 && vih->dwPictAspectRatioY > 0)
					{
						m_fInputAspectRatio = (double)vih->dwPictAspectRatioX / vih->dwPictAspectRatioY;
					}
				}
			}
			
			if( m_biOutput.biWidth != nInputWidth || 
				abs(m_biOutput.biHeight) != abs(nInputHeight) || 
				fOutputAspectRatio != m_fInputAspectRatio )
			{
				// initialize main resizer if necessary
				try
				{
					m_pResampler = new CRatioResampler2( m_biInput.biWidth, abs(m_biInput.biHeight), m_biOutput.biWidth, abs(m_biOutput.biHeight), m_fInputAspectRatio );

					m_nResizeCurrentFPS = 0;
					m_nResizeFrames = 0;
					m_nResizeTicks = 0;
				}
				catch(...)
				{
					return E_OUTOFMEMORY;
				}
			}
		}

	}

	if( m_Type == VIDEOTrans_Crop && m_Crop == NULL )
	{
		if( m_biOutput.biWidth > 0 && m_biOutput.biHeight > 0 )
		{
			if( m_biOutput.biWidth != m_biInput.biWidth || abs(m_biOutput.biHeight) != m_biInput.biHeight )
			{
				try
				{
					m_Crop = new CRatioResampler2( m_biInput.biWidth, m_biInput.biHeight, m_biOutput.biWidth, abs(m_biOutput.biHeight), false );
				}
				catch(...)
				{
					return E_OUTOFMEMORY;
				}
				//default init. We will most probably rewrite this later
				m_Crop->SetCropParams( m_nCropTop * m_biInput.biHeight / 100, m_nCropLeft * m_biInput.biWidth / 100, m_biOutput.biWidth, m_biOutput.biHeight );
			}
		}
	}

	if( m_Type == VIDEOTrans_Deinter && m_DeInterlace == NULL )
	{
		try
		{
			m_DeInterlace = new DeInterlace;
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}
	}

	if( m_Type == VIDEOTrans_Denoise && m_DeNoise == NULL )
	{
		try
		{
			m_DeNoise = new DeNoise( m_biInput.biWidth, m_biInput.biHeight );
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}
	}

	if( m_Type == VIDEOTrans_GreenEnhance && m_GreenEnhance == NULL )
	{
		try
		{
			m_GreenEnhance = new GreenEnhance( m_biInput.biWidth, m_biInput.biHeight );
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}
	}

	if( m_Type == VIDEOTrans_ColorConv )
	{
		if ( m_InputMediaType.subtype != MEDIASUBTYPE_IYUV )
		{
			m_ColorConvert = true;
		}
	}

	return S_OK;
}

STDMETHODIMP CImagePreProcessorFilter::Pause()
{
	CAutoLock lock( &m_csFilter );
	HRESULT hr = S_OK;
	if( IsStopped() )
	{
		if (FAILED(hr = initializeTransform(false)))
		{
			return hr;
		}
	}
	hr = __super::Pause();
	return hr;
}

STDMETHODIMP CImagePreProcessorFilter::Stop()
{

	CAutoLock lock( &m_csFilter );
	HRESULT hr = __super::Stop();

	cleanUp();

	return hr;

}

HRESULT CImagePreProcessorFilter::Receive(IMediaSample *pSample)
{

    /*  Check for other streams and pass them on */
    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    if (pProps->dwStreamId != AM_STREAM_MEDIA) {
        return OUTPUT_PIN->Deliver(pSample);
    }

    HRESULT hr;
    ASSERT(pSample);

	// check if we need to adjust output allocator
	long lDataLength = pSample->GetActualDataLength();

	ALLOCATOR_PROPERTIES curProperties = {0,};
	OUTPUT_PIN->GetAllocator()->GetProperties(&curProperties);

	if ( lDataLength > curProperties.cbBuffer )
	{

		// we have to adjust allocator

        // A zero allignment does not make any sense.
        if (0 == curProperties.cbAlign)
		{
            curProperties.cbAlign = 1;
        }

		// set buffer size to 150% of input sample's size
		curProperties.cbBuffer = lDataLength * 3 / 2;

        HRESULT hr = OUTPUT_PIN->GetAllocator()->Decommit();
        if (FAILED(hr))
		{
            return E_ABORT;
        }

		ALLOCATOR_PROPERTIES actual = {0,};
		hr = OUTPUT_PIN->GetAllocator()->SetProperties(&curProperties, &actual);
        if (FAILED(hr))
		{
            return E_ABORT;
        }

        hr = OUTPUT_PIN->GetAllocator()->Commit();
        if (FAILED(hr))
		{
            return E_ABORT;
        }

	}
	
	// check if we can just pass the sample through
	hr = canPassThrough(pSample);
	if (FAILED(hr))
	{
		return E_ABORT;
	}
	if (hr == S_OK)
	{
    	hr = OUTPUT_PIN->Deliver(pSample);
        m_bSampleSkipped = FALSE;
		return hr;
	}

    IMediaSample * pOutSample;

    // If no output to deliver to then no point sending us data

    ASSERT (m_pOutput != NULL) ;

    // Set up the output sample
    hr = InitializeOutputSample(pSample, &pOutSample);

    if (FAILED(hr)) {
        return hr;
    }

    // Start timing the transform (if PERF is defined)
    MSR_START(m_idTransform);

    // have the derived class transform the data

    hr = Transform(pSample, pOutSample);

    // Stop the clock and log it (if PERF is defined)
    MSR_STOP(m_idTransform);

    if (FAILED(hr)) {
	DbgLog((LOG_TRACE,1,TEXT("Error from transform")));
    } else {
        // the Transform() function can return S_FALSE to indicate that the
        // sample should not be delivered; we only deliver the sample if it's
        // really S_OK (same as NOERROR, of course.)
        if (hr == NOERROR) {
    	    hr = OUTPUT_PIN->Deliver(pOutSample);
            m_bSampleSkipped = FALSE;	// last thing no longer dropped
        } else {
            // S_FALSE returned from Transform is a PRIVATE agreement
            // We should return NOERROR from Receive() in this cause because returning S_FALSE
            // from Receive() means that this is the end of the stream and no more data should
            // be sent.
            if (S_FALSE == hr) {

                //  Release the sample before calling notify to avoid
                //  deadlocks if the sample holds a lock on the system
                //  such as DirectDraw buffers do
                pOutSample->Release();
                m_bSampleSkipped = TRUE;
                if (!m_bQualityChanged) {
                    NotifyEvent(EC_QUALITY_CHANGE,0,0);
                    m_bQualityChanged = TRUE;
                }
                return NOERROR;
            }
        }
    }

    // release the output buffer. If the connected pin still needs it,
    // it will have addrefed it itself.
    pOutSample->Release();

    return hr;

}

HRESULT CImagePreProcessorFilter::canPassThrough(IMediaSample* pSample)
{

	// check if source media type was changed
	AM_MEDIA_TYPE* pNewMT = NULL;
    HRESULT hr = S_FALSE;

	if (m_nAspectRatioChangeTimestamp != -1000000000)
	{
		REFERENCE_TIME tStart(0), tStop(0);
		pSample->GetTime( &tStart, &tStop);
		if (m_nAspectRatioChangeTimestamp <= tStart)
		{
			hr = pSample->GetMediaType(&pNewMT);
			if ( FAILED(hr) || pNewMT == NULL )
			{
				// add new media type manually
				hr = S_OK;
				pNewMT = CreateMediaType(&m_InputMediaType);
				VIDEOINFOHEADER2* vih = NULL;
				if (pNewMT->formattype == FORMAT_VideoInfo2)
				{
					vih = (VIDEOINFOHEADER2*)pNewMT->pbFormat;
				}
				else if (pNewMT->formattype == FORMAT_MPEG2_VIDEO)
				{
					vih = &((MPEG2VIDEOINFO*)pNewMT->pbFormat)->hdr;
				}
				else if (pNewMT->formattype == FORMAT_DiracVideoInfo)
				{
					vih = &((DIRACINFOHEADER*)pNewMT->pbFormat)->hdr;
				}
				if (vih)
				{
					vih->dwPictAspectRatioX = m_nNewAspectRatioWidth;
					vih->dwPictAspectRatioY = m_nNewAspectRatioHeight;
				}
			}
			m_nAspectRatioChangeTimestamp = -1000000000;
		}
		else
		{
			hr = pSample->GetMediaType(&pNewMT);
		}
	}
	else
	{
		hr = pSample->GetMediaType(&pNewMT);
	}

	if ( SUCCEEDED(hr) && pNewMT )
	{

		m_bMediatypeChanged = true;

		CMediaType cmt(*pNewMT);

		switch (m_Type)
		{

		case VIDEOTrans_Unknown:
		case VIDEOTrans_Null:
		case VIDEOTrans_Deinter:
		case VIDEOTrans_Denoise:
		case VIDEOTrans_GreenEnhance:
		case VIDEOTrans_ColorConv:
		case VIDEOTrans_Crop:
			{
				// these transforms use the same output resolution as the input one
				memset( &m_biOutput, 0, sizeof( m_biOutput ) );
				// set new media type
				if (FAILED(hr = SetMediaType(PINDIR_INPUT, &cmt)))
				{
					pSample->Release();
					return hr;
				}
				break;
			}
		case VIDEOTrans_Resize:
			{
				// resize transform has the same output resolution always
				// and don't need to pass through new media type
				// set new media type
				if (FAILED(hr = SetMediaType(PINDIR_INPUT, &cmt)))
				{
					pSample->Release();
					return hr;
				}
				m_bMediatypeChanged = false;
				break;
			}

		}

		{
			CAutoLock lock( &m_csFilter );
			if (FAILED(hr = initializeTransform(true)))
			{
				pSample->Release();
				return hr;
			}
		}

		DeleteMediaType(pNewMT);

		// media type changed, so we can't pass through the sample
		return S_FALSE;

	}

	switch (m_Type)
	{
	case VIDEOTrans_Unknown:
	case VIDEOTrans_Null:
		hr = S_OK;
		break;
	case VIDEOTrans_Crop:
		if (m_Crop == NULL)
		{
			hr = S_OK;
		}
		break;
	case VIDEOTrans_Resize:
		if (m_pResampler == NULL)
		{
			hr = S_OK;
		}
		break;
	case VIDEOTrans_ColorConv:
		if (!m_ColorConvert)
		{
			hr = S_OK;
		}
		break;
	}

	return hr;

}

HRESULT CImagePreProcessorFilter::Transform(IMediaSample * pIn, IMediaSample *pOut)
{

	//first things first. Add thread safety
	pIn->AddRef();
	pOut->AddRef();

    HRESULT hr = S_OK;
    REFERENCE_TIME tStart(0), tStop(0);
    const BOOL bTime = S_OK == pIn->GetTime( &tStart, &tStop);

	TCHAR szTest[128] = { 0, };
#ifdef _CHECK_RESIZE_TIME
	DWORD dwTime = timeGetTime();
#endif
	GUID InputSubFormat = m_InputMediaType.subtype;
	GUID OutputSubFormat = m_OutputMediaType.subtype;

    IMediaSample2 *pSample2;
    if (SUCCEEDED(pOut->QueryInterface(IID_IMediaSample2, (void **)&pSample2))) 
	{
        HRESULT hrProps = pSample2->SetProperties(
            FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, pbBuffer),
            (PBYTE)m_pInput->SampleProps());
        pSample2->Release();
        if (FAILED(hrProps)) 
		{
            pIn->Release();
            pOut->Release();
            return NULL;
        }
    } 
	else 
	{
        if (bTime) 
		{
            pOut->SetTime(&tStart, &tStop);
        }
        if (S_OK == pIn->IsSyncPoint()) 
		{
            pOut->SetSyncPoint(TRUE);
        }
        if (S_OK == pIn->IsDiscontinuity() || m_bSampleSkipped) 
		{
           pOut->SetDiscontinuity(TRUE);
        }
        if (S_OK == pIn->IsPreroll()) 
		{
            pOut->SetPreroll(TRUE);
        }
    }

	if (m_bMediatypeChanged)
	{
		// prepare new output media type
		m_bMediatypeChanged = false;
		CMediaType outmt;
		GetMediaType(1, &outmt);
		if (FAILED(hr = pOut->SetMediaType(&outmt)))
		{
			pIn->Release();
			pOut->Release();
			return hr;
		}
	}
	else
	{
		pOut->SetMediaType(NULL);
	}

    m_bSampleSkipped = FALSE;
    // Copy the sample media times
    REFERENCE_TIME TimeStart, TimeEnd;
    if (pIn->GetMediaTime(&TimeStart,&TimeEnd) == NOERROR) 
	{
        pOut->SetMediaTime(&TimeStart,&TimeEnd);
    }
	hr = S_OK;

	// Copy the sample data
	{

		BYTE *pSourceBuffer, *pDestBuffer;
		long lSourceSize  = pIn->GetSize();
		long lDestSize = pOut->GetSize();
		long lSourceDataLength = pIn->GetActualDataLength();
		if (FAILED(pIn->GetPointer(&pSourceBuffer)) ||
			FAILED(pOut->GetPointer(&pDestBuffer)) ||
			lSourceDataLength == 0 )
		{
				pIn->Release();
				pOut->Release();
				return NULL;
		}
		DUMP_A_YUV_INPUT
#if defined( AUTOFIX_FFDSHOW_AT_INPUT_WIDTH_AUTOADJUST ) && ( defined( WIN64 ) || defined( _WIN64 ) )
		if( m_biInput.biWidth % AUTOFIX_FFDSHOW_AT_INPUT_WIDTH_AUTOADJUST != 0 )
		{
			int RoundupInputWidth = m_biInput.biWidth + ( AUTOFIX_FFDSHOW_AT_INPUT_WIDTH_AUTOADJUST - (m_biInput.biWidth % AUTOFIX_FFDSHOW_AT_INPUT_WIDTH_AUTOADJUST) );
			BYTE *srcY,*srcU,*srcV;
			BYTE *dstY,*dstU,*dstV;
			srcY = pSourceBuffer;
			dstY = pSourceBuffer;
			srcU = srcY + RoundupInputWidth * m_biInput.biHeight;
			dstU = dstY + m_biInput.biWidth * m_biInput.biHeight;
			srcV = srcU + RoundupInputWidth / 2 * m_biInput.biHeight / 2;
			dstV = dstU + m_biInput.biWidth / 2 * m_biInput.biHeight / 2;
			for( int y = 0; y < m_biInput.biHeight; y ++ )
				fast_memcpy( dstY+m_biInput.biWidth * y, srcY + RoundupInputWidth * y, m_biInput.biWidth );
			for( int y = 0; y < m_biInput.biHeight / 2; y ++ )
				fast_memcpy( dstU+m_biInput.biWidth / 2 * y, srcU+RoundupInputWidth / 2 * y, m_biInput.biWidth / 2 );
			for( int y = 0; y < m_biInput.biHeight / 2; y ++ )
				fast_memcpy( dstV+m_biInput.biWidth / 2 * y, srcV+RoundupInputWidth / 2 * y, m_biInput.biWidth / 2 );
		}
#endif
		ASSERT(pSourceBuffer != NULL && pDestBuffer != NULL);

		// first step should be color conversion as the other filters rely on IYUV buffer format
		if( m_ColorConvert )
		{
			//right now we only support IYUV output
			if( OutputSubFormat != MEDIASUBTYPE_IYUV )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
			//sanity check for proper buffer initializations
			if( m_biInput.biWidth != m_biOutput.biWidth || m_biInput.biHeight != m_biOutput.biHeight )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
			if( lDestSize < m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
			
			if( InputSubFormat == MEDIASUBTYPE_RGB24 )
			{
				NEW_DIB_RGB24_to_IYUV( pDestBuffer, pSourceBuffer, m_biOutput.biWidth, m_biOutput.biHeight );
			}
			else if( InputSubFormat == MEDIASUBTYPE_RGB32 )
			{
				NEW_DIB_RGB32_to_IYUV( pDestBuffer, pSourceBuffer, m_biOutput.biWidth, m_biOutput.biHeight, m_biOutput.biWidth, false );
			}
			else if( InputSubFormat == MEDIASUBTYPE_YV12 )
			{
				CopyIYUV( pDestBuffer, pSourceBuffer, pSourceBuffer + m_biInput.biWidth * m_biInput.biHeight * 5 / 4, pSourceBuffer + m_biInput.biWidth * m_biInput.biHeight, m_biInput.biWidth, m_biInput.biHeight, m_biInput.biWidth);
			}
			else if( InputSubFormat == MEDIASUBTYPE_UYVY || InputSubFormat == MEDIASUBTYPE_HDYC)
			{
				ConvertUYVYToIYUV( pSourceBuffer, pDestBuffer, m_biInput.biWidth, m_biInput.biHeight );
			}
			else if( InputSubFormat == MEDIASUBTYPE_YUY2 || InputSubFormat == MEDIASUBTYPE_YUYV)
			{
				ConvertYUY2ToIYUV( pSourceBuffer, pDestBuffer, m_biInput.biWidth, m_biInput.biHeight );
			}
			else if( InputSubFormat == MEDIASUBTYPE_CLJR)
			{
				ConvertCLJRToIYUV( pSourceBuffer, pDestBuffer, m_biInput.biWidth, m_biInput.biHeight );
			}
			else if( InputSubFormat == MEDIASUBTYPE_IYUV || InputSubFormat == MEDIASUBTYPE_I420)
				
			{
				memcpy( pDestBuffer, pSourceBuffer, lDestSize );
			}
			pOut->SetActualDataLength( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
			DUMP_A_YUV_OUTPUT(0)

            pIn->Release();
            pOut->Release();
			return S_OK;
		}

		if( m_pResampler )
		{

			//make sure we have enough space for the output
			if( lDestSize < m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 )
			{
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}

			if( InputSubFormat != MEDIASUBTYPE_IYUV )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}

			if (m_pResampler != NULL)
			{
				if(m_nResizeCurrentFPS > 0)
				{
					if(m_nResizeCurrentFPS > m_nResizeDesiredFPS)
						m_pResampler->Process( pSourceBuffer, pDestBuffer, CRatioResampler2::IYUV );
					else
						ResampleYUV420Liniar( pSourceBuffer, pDestBuffer, m_biInput.biWidth, m_biInput.biHeight, m_biOutput.biWidth, m_biOutput.biHeight, m_fInputAspectRatio );
				}
				else // benchmark phase
				{
					int StartT = GetTickCount();
					m_pResampler->Process( pSourceBuffer, pDestBuffer, CRatioResampler2::IYUV );
					m_nResizeTicks += (GetTickCount() - StartT);
					++m_nResizeFrames;

					if(m_nResizeFrames > 200 && m_nResizeTicks > 0)
						m_nResizeCurrentFPS = 1000000 * m_nResizeFrames / m_nResizeTicks;
				}			
			}

#ifdef _CHECK_RESIZE_TIME
			swprintf( szTest, _countof(szTest), L"Transform Time : %dms\n", timeGetTime() - dwTime );
			OutputDebugString( szText );
#endif

			pOut->SetActualDataLength( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
			DUMP_A_YUV_OUTPUT(1)
            pIn->Release();
            pOut->Release();
			return S_OK;

		}

		if( m_Crop )
		{
			//if( InputSubFormat == MEDIASUBTYPE_RGB24 )
			//{
			//	pOut->SetActualDataLength( ( m_biOutput.biWidth * abs(m_biOutput.biHeight) * m_biOutput.biBitCount ) >> 3 );
			//	m_Crop->Crop( pSourceBuffer, pDestBuffer, CRatioResampler2::RGB24 );
			//}
			//else
			if( InputSubFormat == MEDIASUBTYPE_IYUV )
			{
				//sanity check, in case someone changed input / output parameters without letting us know. This should not happen
				if( m_Crop->m_nDesHeight * m_Crop->m_nDesWidth * 3 / 2 > lSourceSize || lSourceSize < lDestSize )
				{
					pOut->SetActualDataLength( 0 );
					pIn->Release();
					pOut->Release();
					return E_ABORT;
				}
				if( lDestSize < m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 )
				{
					pOut->SetActualDataLength( 0 );
					pIn->Release();
					pOut->Release();
					return E_ABORT;
				}
				m_Crop->Crop( pSourceBuffer, pDestBuffer, CRatioResampler2::IYUV );
			}
			else
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
#ifdef _CHECK_RESIZE_TIME
			swprintf( szTest, _countof(szTest), L"Transform Time : %dms\n", timeGetTime() - dwTime );
			OutputDebugString( szText );
#endif
			pOut->SetActualDataLength( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
			DUMP_A_YUV_OUTPUT(2)
			pIn->Release();
			pOut->Release();
			return S_OK;
		}

		if( m_DeInterlace )
		{
			//avoid wrtiting more to dest buffer then it's size
			if( m_biInput.biWidth != m_biOutput.biWidth || m_biInput.biHeight != m_biOutput.biHeight || lSourceDataLength != m_biOutput.biSizeImage )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
			if( InputSubFormat == MEDIASUBTYPE_IYUV )
			{
				if( lDestSize < m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 )
				{
					pOut->SetActualDataLength( 0 );
					pIn->Release();
					pOut->Release();
					return E_ABORT;
				}
				memcpy( pDestBuffer, pSourceBuffer, lSourceDataLength );
				m_DeInterlace->DeinterlaceYUV( pDestBuffer, m_biOutput.biWidth, BYTE_ALIGN(m_biOutput.biHeight));
				m_DeInterlace->DeinterlaceYUV( pDestBuffer + m_biOutput.biWidth*m_biOutput.biHeight, m_biOutput.biWidth/2, BYTE_ALIGN(m_biOutput.biHeight/2));
				m_DeInterlace->DeinterlaceYUV( pDestBuffer + m_biOutput.biWidth*m_biOutput.biHeight*5/4, m_biOutput.biWidth/2, BYTE_ALIGN(m_biOutput.biHeight/2));
#ifdef _CHECK_RESIZE_TIME
				swprintf( szTest, _countof(szTest), L"Transform Time : %dms\n", timeGetTime() - dwTime );
				OutputDebugString( szText );
#endif
				pOut->SetActualDataLength( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
				DUMP_A_YUV_OUTPUT(3)
				pIn->Release();
				pOut->Release();
				return S_OK;
			}
			else
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
		}

		if( m_DeNoise )
		{
			//avoid wrtiting more to dest buffer then it's size
			if( m_biInput.biWidth != m_biOutput.biWidth || m_biInput.biHeight != m_biOutput.biHeight || lSourceDataLength != m_biOutput.biSizeImage )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
			if( InputSubFormat == MEDIASUBTYPE_IYUV )
			{
				if( lDestSize < m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 )
				{
					pOut->SetActualDataLength( 0 );
					pIn->Release();
					pOut->Release();
					return E_ABORT;
				}
				memcpy( pDestBuffer, pSourceBuffer, lSourceDataLength );
				m_DeNoise->Denoising_HM( pDestBuffer, m_biOutput.biWidth, m_biOutput.biHeight, 1 );
#ifdef _CHECK_RESIZE_TIME
				swprintf( szTest, _countof(szTest), L"Transform Time : %dms\n", timeGetTime() - dwTime );
				OutputDebugString( szText );
#endif
				pOut->SetActualDataLength( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
				DUMP_A_YUV_OUTPUT(4)
				pIn->Release();
				pOut->Release();
				return S_OK;
			}
			else
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
		}

		if( m_GreenEnhance )
		{
			//avoid wrtiting more to dest buffer then it's size
			if( m_biInput.biWidth != m_biOutput.biWidth || m_biInput.biHeight != m_biOutput.biHeight || lSourceDataLength != m_biOutput.biSizeImage )
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
			if( InputSubFormat == MEDIASUBTYPE_IYUV )
			{
				//ensure our output buffer is large enough( maybe this is first time initialization ? )
				if( lDestSize < m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 )
				{
					pOut->SetActualDataLength( 0 );
					pIn->Release();
					pOut->Release();
					return E_ABORT;
				}
				memcpy( pDestBuffer, pSourceBuffer, lSourceDataLength );
				m_GreenEnhance->GE_HM( pDestBuffer, m_biOutput.biWidth, m_biOutput.biHeight );
#ifdef _CHECK_RESIZE_TIME
				swprintf( szTest, _countof(szTest), L"Transform Time : %dms\n", timeGetTime() - dwTime );
				OutputDebugString( szText );
#endif
				pOut->SetActualDataLength( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
				DUMP_A_YUV_OUTPUT(5)
				pIn->Release();
				pOut->Release();
				return S_OK;
			}
			else
			{
				pOut->SetActualDataLength( 0 );
				pIn->Release();
				pOut->Release();
				return E_ABORT;
			}
		}

		//else is null filter = copy src to dest
		{
			pOut->SetActualDataLength( lSourceDataLength );
			LONG hi = abs(m_biInput.biHeight);
			LONG ho = abs(m_biOutput.biHeight);
			LONG nStrideIn = m_biInput.biWidth*(m_biInput.biBitCount>>3);
			LONG nStrideOut = m_biOutput.biWidth*(m_biOutput.biBitCount>>3);
			if( pDestBuffer == pSourceBuffer )
			{
				//do nothing, we are copying the content to the source
			}
			else if( m_biOutput.biWidth == m_biInput.biWidth && m_biOutput.biHeight > 0 && nStrideIn == nStrideOut )
			{
				memcpy( pDestBuffer, pSourceBuffer, lSourceDataLength );
			}
			else if( m_biOutput.biWidth == m_biInput.biWidth && m_biOutput.biHeight > 0 )
			{
				fast_memcpy( pDestBuffer, pSourceBuffer, lSourceDataLength );
			}
			else
			{
				long y = 0;
				if( m_biOutput.biHeight > 0 )
				{
					for( y = 0; y < hi; y ++ )
					{
						fast_memcpy( pDestBuffer+nStrideOut*y, pSourceBuffer+nStrideIn*y, nStrideIn );
					}
				}
				else
				{
					for( y = 0; y < hi; y ++ )
					{
						fast_memcpy( pDestBuffer+nStrideOut*y, pSourceBuffer+nStrideIn*(ho-y-1), nStrideIn );
					}
				}
			}
			DUMP_A_YUV_OUTPUT(6)
		}

	}

	pIn->Release();
	pOut->Release();
	return S_OK;

}

// check if you can support mtIn
HRESULT CImagePreProcessorFilter::CheckInputType(const CMediaType* mtIn)
{
	HRESULT hr = S_OK;
	BITMAPINFOHEADER bih;
	ExtractBIH(mtIn, &bih);
	//sanity check for proper initializations. Later we might support flipped in/out
	if( mtIn->lSampleSize == 0 // can be 0 only for compressed data. We want PLANAR YUV
		|| bih.biWidth <= 0 || bih.biHeight <= 0 )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	// process only video only
	if( mtIn->majortype != MEDIATYPE_Video )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	//apart from color conversion filter all filters will require IYUV format
	if( m_Type != VIDEOTrans_ColorConv && mtIn->subtype != MEDIASUBTYPE_IYUV )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	//color conversion filter only accepts the following types atm 
	if( m_Type == VIDEOTrans_ColorConv )
	{
		if( mtIn->subtype != MEDIASUBTYPE_IYUV
			&& mtIn->subtype != MEDIASUBTYPE_I420

			&& mtIn->subtype != MEDIASUBTYPE_YV12

			&& mtIn->subtype != MEDIASUBTYPE_UYVY 
			&& mtIn->subtype != MEDIASUBTYPE_HDYC 
			
			&& mtIn->subtype != MEDIASUBTYPE_YUY2
			&& mtIn->subtype != MEDIASUBTYPE_YUYV

			&& mtIn->subtype != MEDIASUBTYPE_CLJR

			&& mtIn->subtype != MEDIASUBTYPE_RGB24 
			&& mtIn->subtype != MEDIASUBTYPE_RGB32)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	if (m_bKeepAspectRatio)
	{
		// if we need to keep aspect ratio then only format with aspect ratio information will be accepted
		if( mtIn->formattype != FORMAT_VideoInfo2 && mtIn->formattype != FORMAT_MPEG2_VIDEO && mtIn->formattype != FORMAT_DiracVideoInfo )
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	return S_OK;
}

HRESULT CImagePreProcessorFilter::CheckOutputType(const CMediaType* mtOut)
{
	BITMAPINFOHEADER bih;
	ExtractBIH(mtOut, &bih);
	//sanity check for proper initializations. Later we might support flipped in/out
	if( bih.biWidth <= 0 || bih.biHeight <= 0 )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	// if output size was set then check if requested size matches
	if ( m_biOutput.biWidth > 0 && m_biOutput.biHeight > 0 )
	{
		if ( bih.biWidth != m_biOutput.biWidth || bih.biHeight != m_biOutput.biHeight )
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	// process only video only
	if( mtOut->majortype != MEDIATYPE_Video )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	//we can only output IYUV atm
	if( mtOut->subtype != MEDIASUBTYPE_IYUV )
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	return S_OK;
}

// check if you can support the transform from this input to this output
HRESULT CImagePreProcessorFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	HRESULT hr = CheckInputType( mtIn );
	if( FAILED(hr) ) 
	{
		return hr;
	}
	hr = CheckOutputType( mtOut );
	if( FAILED(hr) ) 
	{
		return hr;
	}
	return S_OK;
}

// override this to know when the media type is actually set
HRESULT CImagePreProcessorFilter::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
	HRESULT hr = S_OK;
	BITMAPINFOHEADER bih;
	ExtractBIH(pmt, &bih);

	if( direction == PINDIR_INPUT )
	{
		hr = CheckInputType( pmt );
		if( FAILED(hr) ) 
		{
			return hr;
		}
		{
			CAutoLock lock( &m_csFilter );
			m_biInput = bih;
			if (m_biOutput.biWidth == 0 && m_biOutput.biHeight == 0)
			{
				// initialize output bitmap
				m_biOutput = bih;

				if(m_Type == VIDEOTrans_Crop)
				{
					m_biOutput.biWidth = m_nCropWidth * m_biInput.biWidth / 100;
					m_biOutput.biWidth -= m_biOutput.biWidth%4;
					m_biOutput.biHeight = m_nCropHeight * m_biInput.biHeight / 100;
					m_biOutput.biHeight -= m_biOutput.biHeight%4;
					m_biOutput.biSizeImage = m_biOutput.biWidth * m_biOutput.biHeight + (m_biOutput.biWidth >> 1) * (m_biOutput.biHeight >> 1) * 2;
				}
			}
			m_InputMediaType = *pmt;
		}
	}
	else if( direction == PINDIR_OUTPUT )
	{
		hr = CheckOutputType( pmt );
		if( FAILED(hr) ) 
		{
			return hr;
		}
		{
			CAutoLock lock( &m_csFilter );
			m_OutputMediaType = *pmt;
		}
	}
	return __super::SetMediaType(direction, pmt);
}

// chance to grab extra interfaces on connection
HRESULT CImagePreProcessorFilter::CheckConnect(PIN_DIRECTION dir,IPin *pPin)
{
	return __super::CheckConnect(dir, pPin);
}

HRESULT CImagePreProcessorFilter::BreakConnect(PIN_DIRECTION dir)
{
	return __super::BreakConnect(dir);
}

HRESULT CImagePreProcessorFilter::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
	return __super::CompleteConnect(direction, pReceivePin);
}

// this goes in the factory template table to create new instances
// static CCOMObject * CreateInstance(__inout_opt LPUNKNOWN, HRESULT *);
// call the SetProperties function with appropriate arguments
HRESULT CImagePreProcessorFilter::DecideBufferSize(
                    IMemAllocator * pAllocator,
                    ALLOCATOR_PROPERTIES *pProperties)
{
	if( INPUT_PIN->IsConnected() == FALSE )
	{
		return VFW_E_NOT_CONNECTED;
	}
	long cBuffers = OUTPUT_PIN->CurrentMediaType().formattype == FORMAT_VideoInfo ? 1 : DEFAULT_BUFFERS;

	pProperties->cBuffers = DEFAULT_BUFFERS;
	DWORD InputSize = abs( m_biInput.biWidth * m_biInput.biHeight * 3 / 2 );
	DWORD OutputSize = abs( m_biOutput.biWidth * m_biOutput.biHeight * 3 / 2 );
	pProperties->cbBuffer = max( InputSize, OutputSize );
	pProperties->cbAlign = 1;
	pProperties->cbPrefix;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
	{
		return hr;
	}
    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

// override to suggest OUTPUT pin media types
HRESULT CImagePreProcessorFilter::GetMediaType(int iPosition, __inout CMediaType *pMediaType)
{
	if( !INPUT_PIN->IsConnected() ) 
	{
		return VFW_E_NOT_CONNECTED;
	}
	if ( iPosition < 0 )				
	{
		return E_INVALIDARG;
	}
	if( iPosition > 1 )			
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	ZeroMemory(pMediaType, sizeof(CMediaType));
	pMediaType->majortype = MEDIATYPE_Video;

	//input
	if ( iPosition == 0 )
	{

		AM_MEDIA_TYPE	amt;
		INPUT_PIN->ConnectionMediaType( &amt );
		pMediaType->subtype = amt.subtype;
		FreeMediaType(amt);

		BITMAPINFOHEADER bihOut;
		memset(&bihOut, 0, sizeof(bihOut));
		bihOut.biSize = sizeof(bihOut);
		bihOut.biWidth = m_biInput.biWidth;
		bihOut.biHeight = m_biInput.biHeight;
		bihOut.biPlanes = m_biInput.biPlanes;
		bihOut.biBitCount = m_biInput.biBitCount;
		bihOut.biCompression = m_biInput.biCompression;
		bihOut.biXPelsPerMeter = 0;
		bihOut.biYPelsPerMeter = 0;
		bihOut.biSizeImage = m_biInput.biSizeImage;

		pMediaType->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih, 0, sizeof(VIDEOINFOHEADER2));
		vih->bmiHeader = bihOut;

	}
	//output
	else if ( iPosition == 1 )
	{

		pMediaType->subtype = MEDIASUBTYPE_IYUV;

		BITMAPINFOHEADER bihOut;
		memset(&bihOut, 0, sizeof(bihOut));
		bihOut.biSize = sizeof(bihOut);
		bihOut.biWidth = m_biOutput.biWidth;
		if (bihOut.biWidth == 0)
		{
			bihOut.biWidth = m_biInput.biWidth;
		}
		bihOut.biHeight = m_biOutput.biHeight;
		if (bihOut.biHeight == 0)
		{
			bihOut.biHeight = m_biInput.biHeight;
		}
		bihOut.biPlanes = 1;
		bihOut.biBitCount = 12;
		bihOut.biCompression = 'VUYI';
		bihOut.biSizeImage = bihOut.biWidth * bihOut.biHeight + (bihOut.biWidth >> 1) * (bihOut.biHeight >> 1) * 2;
		pMediaType->lSampleSize = bihOut.biSizeImage;

		pMediaType->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih, 0, sizeof(VIDEOINFOHEADER2));
		vih->bmiHeader = bihOut;

		vih->rcSource.right = bihOut.biWidth;
		vih->rcSource.bottom = bihOut.biHeight;
		vih->rcTarget.right = bihOut.biWidth;
		vih->rcTarget.bottom = bihOut.biHeight;

		if (m_Type != VIDEOTrans_Resize)
		{

			VIDEOINFOHEADER2* pSrcVih = NULL;
			if (m_InputMediaType.formattype == FORMAT_VideoInfo2)
			{
				pSrcVih = (VIDEOINFOHEADER2*)m_InputMediaType.pbFormat;
			}
			else if (m_InputMediaType.formattype == FORMAT_MPEG2_VIDEO)
			{
				pSrcVih = &((MPEG2VIDEOINFO*)m_InputMediaType.pbFormat)->hdr;
			}
			else if (m_InputMediaType.formattype == FORMAT_DiracVideoInfo)
			{
				pSrcVih = &((DIRACINFOHEADER*)m_InputMediaType.pbFormat)->hdr;
			}

			if (pSrcVih)
			{
				vih->dwPictAspectRatioX = pSrcVih->dwPictAspectRatioX;
				vih->dwPictAspectRatioY = pSrcVih->dwPictAspectRatioY;
			}

		}
		else
		{
			vih->dwPictAspectRatioX = bihOut.biWidth;
			vih->dwPictAspectRatioY = bihOut.biHeight;
		}

	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::GetImageInputSize( 
    /* [out] */ long *pWidth,
    /* [out] */ long *pHeight)
{
	if( !INPUT_PIN->IsConnected() )
	{
		return VFW_E_NOT_CONNECTED;
	}

    CheckPointer(pWidth,E_POINTER);
    CheckPointer(pHeight,E_POINTER);
	*pWidth = m_biInput.biWidth;
	*pHeight = m_biInput.biHeight;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::GetImageResizeEnabled( 
    /* [out] */ boolean *pEnabled)
{
    CheckPointer(pEnabled,E_POINTER);
	*pEnabled = (m_biOutput.biWidth != m_biInput.biWidth || m_biOutput.biHeight != m_biInput.biHeight);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::GetImageOutSize( 
    /* [out] */ long *pWidth,
    /* [out] */ long *pHeight)
{
    CheckPointer(pWidth,E_POINTER);
    CheckPointer(pHeight,E_POINTER);
	*pWidth = m_biOutput.biWidth;
	*pHeight = m_biOutput.biHeight;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::GetFilterType( 
    /* [out] */ long *pFilterType)
{
    CheckPointer(pFilterType,E_POINTER);
	*pFilterType = m_Type;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::SetFilterType( 
    /* [in] */ long nFilterType)
{
	if( !IsStopped() )	
	{
		return VFW_E_NOT_STOPPED;
	}

	if( nFilterType <= VIDEOTrans_Unknown || nFilterType >= VIDEOTrans_Total )
	{
		return E_INVALIDARG;
	}

	m_Type = (_VIDEOTransType)nFilterType;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::SetCropParams( 
    /* [in] */ long nOffsetLeft,
    /* [in] */ long nOffsetTop, 
    /* [in] */ long nWidth,
    /* [in] */ long nHeight
	)
{
	//do not change output unless we are stopped
	if( !IsStopped() )	
	{
		return VFW_E_NOT_STOPPED;
	} 

	if( m_Type != VIDEOTrans_Crop )
	{
		return E_INVALIDARG;
	}

	m_nCropLeft = nOffsetLeft;
	m_nCropTop = nOffsetTop;
	m_nCropWidth = nWidth;
	m_nCropHeight = nHeight;

	if(m_nCropWidth + m_nCropLeft > 100)
		m_nCropWidth = 100 - m_nCropLeft;
	if(m_nCropHeight + m_nCropTop > 100)
		m_nCropHeight = 100 - m_nCropTop;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::SetImageOutSize( 
    /* [in] */ long nWidth,
    /* [in] */ long nHeight,
	/* [in] */ bool bKeepAspectRatio,
	/* [in] */ long nDesiredFPS)
{
	if( !IsStopped() )	
	{
		return VFW_E_NOT_STOPPED;
	}
	if( nWidth <= 0 || nHeight <= 0 ) 
	{
		return E_INVALIDARG;
	}

	m_bKeepAspectRatio = bKeepAspectRatio;
	m_nResizeDesiredFPS = nDesiredFPS * 3 / 2;

	HRESULT hr= S_OK;
	if( m_biOutput.biWidth == nWidth && m_biOutput.biHeight == nHeight)
	{
		return S_OK;
	}

	m_biOutput.biSize = sizeof(m_biOutput);
	m_biOutput.biWidth = nWidth;
	m_biOutput.biHeight = nHeight;
	m_biOutput.biPlanes = 1;
	m_biOutput.biBitCount = 12;
	m_biOutput.biXPelsPerMeter = 0;
	m_biOutput.biYPelsPerMeter = 0;
	m_biOutput.biCompression = 'VUYI';
	m_biOutput.biSizeImage = m_biOutput.biWidth * m_biOutput.biHeight + (m_biOutput.biWidth >> 1) * (m_biOutput.biHeight >> 1) * 2;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::SetAspectRatio(REFERENCE_TIME nSampleTimestamp, long nAspectRatioWidth, long nAspectRatioHeight)
{

	m_nNewAspectRatioWidth = nAspectRatioWidth;
	m_nNewAspectRatioHeight = nAspectRatioHeight;
	m_nAspectRatioChangeTimestamp = nSampleTimestamp;

	return S_OK;

}

HRESULT STDMETHODCALLTYPE CImagePreProcessorFilter::GetAspectRatio(long* pAspectRatioWidth, long* pAspectRatioHeight)
{

	*pAspectRatioWidth = m_nNewAspectRatioWidth;
	*pAspectRatioHeight = m_nNewAspectRatioHeight;

	return S_OK;

}

CYuvTransOutputPin::CYuvTransOutputPin(__in_opt LPCTSTR pObjectName,
        __inout CTransformFilter *pTransformFilter,
        __inout HRESULT * phr,
        __in_opt LPCWSTR pName) :
        CTransformOutputPin(pObjectName, pTransformFilter, phr, pName)
		, m_pTransformFilter(pTransformFilter)
		, m_pOutputQueue(NULL)
{
}

HRESULT CYuvTransOutputPin::CheckMediaType(const CMediaType* pmt)
{
    CheckPointer(pmt,E_POINTER);
    if( pmt->majortype != MEDIATYPE_Video )
        return S_FALSE;
	return __super::CheckMediaType(pmt);
}

HRESULT CYuvTransOutputPin::GetMediaType( int iPosition, __inout CMediaType *pMediaType)
{
	if( iPosition == 0 )
	{
        return m_pTransformFilter->GetMediaType( 1, pMediaType); //our putput is always on pos 1 not 0 !!!
    } 
	return VFW_S_NO_MORE_ITEMS;
}

HRESULT CYuvTransOutputPin::DecideBufferSize( IMemAllocator * pAlloc,__inout ALLOCATOR_PROPERTIES *pProp)
{
	return __super::DecideBufferSize( pAlloc, pProp );
}

HRESULT CYuvTransOutputPin::DecideAllocator( IMemInputPin * pPin, __deref_out IMemAllocator ** pAlloc )
{
	return __super::DecideAllocator( pPin, pAlloc );
}

HRESULT CYuvTransOutputPin::SetMediaType(const CMediaType* pmt)
{
    CheckPointer(pmt,E_POINTER);
	return __super::SetMediaType(pmt);
}

//
// Active
//
// This is called when we start running or go paused. We create the
// output queue object to send data to our associated peer pin
//
HRESULT CYuvTransOutputPin::Active()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Make sure that the pin is connected
    if(m_Connected == NULL)
        return NOERROR;

    // Create the output queue if we have to
    if(m_pOutputQueue == NULL)
    {
        m_pOutputQueue = new COutputQueue(m_Connected, &hr);
        if(m_pOutputQueue == NULL)
            return E_OUTOFMEMORY;

        // Make sure that the constructor did not return any error
        if(FAILED(hr))
        {
            delete m_pOutputQueue;
            m_pOutputQueue = NULL;
            return hr;
        }
    }

    // Pass the call on to the base class
    __super::Active();
    return NOERROR;
} // Active


//
// Inactive
//
// This is called when we stop streaming
// We delete the output queue at this time
//
HRESULT CYuvTransOutputPin::Inactive()
{
    CAutoLock lock_it(m_pLock);

    // Delete the output queus associated with the pin.
    if(m_pOutputQueue)
    {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }

    __super::Inactive();
    return NOERROR;
} // Inactive


//
// Deliver
//
HRESULT CYuvTransOutputPin::Deliver(IMediaSample *pMediaSample)
{
    CheckPointer(pMediaSample,E_POINTER);

    // Make sure that we have an output queue
    if(m_pOutputQueue == NULL)
        return NOERROR;

    pMediaSample->AddRef();
    return m_pOutputQueue->Receive(pMediaSample);

} // Deliver

//
// DeliverEndOfStream
//
HRESULT CYuvTransOutputPin::DeliverEndOfStream()
{
    // Make sure that we have an output queue
    if(m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->EOS();
    return NOERROR;

} // DeliverEndOfStream


//
// DeliverBeginFlush
//
HRESULT CYuvTransOutputPin::DeliverBeginFlush()
{
    // Make sure that we have an output queue
    if(m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->BeginFlush();
    return NOERROR;

} // DeliverBeginFlush


//
// DeliverEndFlush
//
HRESULT CYuvTransOutputPin::DeliverEndFlush()
{
    // Make sure that we have an output queue
    if(m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->EndFlush();
    return NOERROR;

} // DeliverEndFlish

//
// DeliverNewSegment
//
HRESULT CYuvTransOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, 
                                         REFERENCE_TIME tStop,  
                                         double dRate)
{
    // Make sure that we have an output queue
    if(m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->NewSegment(tStart, tStop, dRate);
    return NOERROR;

} // DeliverNewSegment


CYuvTransInputPin::CYuvTransInputPin(__in_opt LPCTSTR pObjectName,
        __inout CTransformFilter *pTransformFilter,
        __inout HRESULT * phr,
        __in_opt LPCWSTR pName) :
        CTransformInputPin(pObjectName, pTransformFilter, phr, pName)
{
}

HRESULT CYuvTransInputPin::CheckMediaType(const CMediaType* pmt)
{
    CheckPointer(pmt,E_POINTER);
    if( pmt->majortype != MEDIATYPE_Video )
        return S_FALSE;
	return __super::CheckMediaType(pmt);
}
