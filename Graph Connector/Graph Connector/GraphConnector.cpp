#include "StdAfx.h"
#include "VidiatorFilterGuids.h"
//#include "GraphConnector.h"

#if defined( DEBUGGING_IN_TESTBENCH )
	#define DlogP(x) { printf(x); }
	#define DlogP2(a,b,c) { printf(a,b,c); }
	#define Dlog(a) { printf(a); if( m_fdLogs ) fprintf( m_fdLogs, a ); }
	#define Dlog1(a,b) { printf(a,b); if( m_fdLogs ) fprintf( m_fdLogs, a, b ); }
	#define Dlog2(a,b,c) { printf(a,b,c); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c ); }
	#define Dlog3(a,b,c,d) { printf(a,b,c,d); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d ); }
	#define Dlog4(a,b,c,d,e) { printf(a,b,c,d,e); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e ); }
	#define Dlog5(a,b,c,d,e,f) { printf(a,b,c,d,e,f); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e, f ); }
	#define DlogVideo3(a,b,c,d) { if( m_mMediaType.majortype == MEDIATYPE_Video ) printf(a,b,c,d); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d ); }
	#define DlogAudio3(a,b,c,d) { if( m_mMediaType.majortype == MEDIATYPE_Audio ) printf(a,b,c,d); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d ); }
//	#define DlogVideo4(a,b,c,d,e) { if( m_mMediaType.majortype == MEDIATYPE_Video ) printf(a,b,c,d,e); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e ); }
//	#define DlogAudio4(a,b,c,d,e) { if( m_mMediaType.majortype == MEDIATYPE_Audio ) printf(a,b,c,d,e); if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e ); }
	#define DlogVideo4(a,b,c,d,e) { if( m_mMediaType.majortype == MEDIATYPE_Video ) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e ); }
	#define DlogAudio4(a,b,c,d,e) { if( m_mMediaType.majortype == MEDIATYPE_Audio ) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e ); }
	#define DlogVideo6(a,b,c,d,e,f,g) { if( m_mMediaType.majortype == MEDIATYPE_Video ) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e, f, g ); }
	#define DlogAudio6(a,b,c,d,e,f,g) { if( m_mMediaType.majortype == MEDIATYPE_Audio ) if( m_fdLogs ) fprintf( m_fdLogs, a, b, c, d, e, f, g ); }
#else
	#define DlogP(x)
	#define DlogP2(a,b,c)
	#define Dlog(x)
	#define Dlog1(a,b) 
	#define Dlog2(a,b,c) 
	#define Dlog3(a,b,c,d)
	#define Dlog4(a,b,c,d,e)
	#define Dlog5(a,b,c,d,e,f) 
	#define DlogVideo3(a,b,c,d) 
	#define DlogAudio3(a,b,c,d) 
	#define DlogVideo4(a,b,c,d,e) 
	#define DlogAudio4(a,b,c,d,e) 
#endif


// Setup data
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudConnector =
{
    &CLSID_GraphConnector,                // Filter CLSID
    L"Vidiator Graph Connector",          // String name
    MERIT_DO_NOT_USE,           // Filter merit
    0,                          // Number pins
    NULL                    // Pin details
};

const AMOVIESETUP_PIN sudPinsInput =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudConnectorInput =
{
    &CLSID_GraphConnectorInput,      // Filter CLSID
    L"Vidiator Graph Connector Input", // String name
    MERIT_DO_NOT_USE,				 // Filter merit
    1,								 // Number pins
    &sudPinsInput					 // Pin details
};

const AMOVIESETUP_PIN sudPinsOutput =
{
    L"Output",                  // Pin string name
    FALSE,                      // Is it rendered
    TRUE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Input",                   // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudConnectorOutput =
{
    &CLSID_GraphConnectorOutput,      // Filter CLSID
    L"Vidiator Graph Connector Output", // String name
    MERIT_DO_NOT_USE,				 // Filter merit
    1,								 // Number pins
    &sudPinsOutput					 // Pin details
};

CFactoryTemplate g_Templates[]=
{
	{L"ConnectorInput", &CLSID_GraphConnectorInput, CGraphConnectorInputFilter::CreateInstance, NULL, &sudConnectorInput},
	{L"ConnectorOutput", &CLSID_GraphConnectorOutput, CGraphConnectorOutputFilter::CreateInstance, NULL, &sudConnectorOutput},
	{L"Connector", &CLSID_GraphConnector, CGraphConnector::CreateInstance, NULL, &sudConnector}
};

int g_cTemplates = 3; 

#ifdef DEBUGGING_IN_TESTBENCH
#include <sstream>
void DumpSampleInfo(IMediaSample *pSample, void *OwnerID, const char *OwnerMSG)
{
    PBYTE pbData;
    REFERENCE_TIME tStart, tStop;
	std::stringstream ss;
	ss << "DumpSampleInfo ( " << OwnerMSG << OwnerID << " ) : \n dms = " << timeGetTime();
	ss << "\n Bytes " << pSample->GetActualDataLength() << " max space " << pSample->GetSize();
    HRESULT hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) 
	{
		printf("DumpSampleInfo : Early exit due to missing sample content\n");
        return;
	}
	ss << "\n Few sample bytes : " ;
	for( int i=0;i<10;i++)
		ss << (int)(pbData[i]) << " ";
	/*
	ss << "\n tStart " << tStart << " tStop " << tStop ;
    pSample->GetTime(&tStart, &tStop);
    hr = pSample->GetMediaTime(&tStart, &tStop);
    if (hr == NOERROR) 
    {
		ss << "\n Start media time " << tStart;
		ss << "\n End media time " << tStop;
    }
    hr = pSample->IsSyncPoint();
	ss << "\n Sync point " << (hr == S_OK);
    hr = pSample->IsPreroll();
	ss << "\n Preroll " << (hr == S_OK);
    hr = pSample->IsDiscontinuity();
	ss << "\n Discontinuity " << (hr == S_OK);
    AM_MEDIA_TYPE *pMediaType;
    pSample->GetMediaType(&pMediaType);
	ss << "\n Type changed " << (pMediaType ? TRUE : FALSE);
    DeleteMediaType(pMediaType);
	/**/
	printf( "%s\n", ss.str().c_str() );
}
#endif
CGraphConnector::CGraphConnector(LPUNKNOWN pUnk, HRESULT *phr) :
    CUnknown(NAME("CGraphConnector"), pUnk),
    m_pInputFilter(NULL),
    m_pInputPin(NULL),
	m_nNumOutputs(0)
{
	m_pInputPinForPush = NULL;
	m_pInputPin = NULL;
	for( int i=0;i<CONNECTOR_MAX_OUTPUT_COUNT;i++)
	{
		m_pOutputFilter[i] = NULL;
		m_pOutputPin[i] = NULL;
		m_pOutputPinForPush[i] = NULL;
		m_pOutputPinForPull[i] = NULL;
	}
	m_Type = GCB_THROW_PACKET_UNLESS_OUTPUT_CONNECTED;
	m_nAllocatorType = GCAT_USE_INTERNAL;
//	m_nAllocatorType = GCAT_USE_FROM_INPUT;	//passthrough mode, when you want to make sure input is the same as output

    ASSERT(phr);
    HRESULT hr = CreateMemoryAllocator( &m_pAllocator );
    if ( FAILED(hr) )
	{
        return;
    }
	m_bBufferDecided = FALSE;
    
	IBaseFilter* pFilter = NULL;
	hr = CoCreateInstance(CLSID_GraphConnectorInput, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pFilter);
    if (pFilter == NULL) 
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
        return;
    }

    m_pInputFilter = (CGraphConnectorInputFilter*)pFilter; //new CGraphConnectorInputFilter(this, GetOwner(), &m_Lock, phr);
	m_pInputFilter->m_pConnector = this;
	m_pInputFilter->m_pPosition = NULL;

    m_pInputPinForPush = new CGraphConnectorInputPin(this,GetOwner(),
                               m_pInputFilter,
                               &m_Lock,
                               &m_QueueLock,
                               phr);

	m_pInputPin = m_pInputPinForPush;

	if ( m_pInputPin == NULL )
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
        return;
    }

	m_pInputPinForPull = NULL;
	m_bUseAsyncReaderOutput = FALSE;

	//add one default output
//	AddOutput();
//	memset( &m_mMediaType, 0, sizeof( m_mMediaType )); //constructor already solves the initialization
#ifdef DEBUGGING_IN_TESTBENCH
	PacketCounterOut = 0;
	PacketCounterIn = 0;
	m_fdLogs = NULL;
	char FileName[100];
	//it's all because of Danilo and he's special PC
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"./Log/GC_PIN_%d_%p_%d.txt", (char)(m_mMediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"./Logs/GC_PIN_%d_%p_%d.txt", (char)(m_mMediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"C:/temp/GC_PIN_%d_%p_%d.txt", (char)(m_mMediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
	if( m_fdLogs == NULL )
	{
		sprintf( FileName,"C:/GC_PIN_%d_%p_%d.txt", (char)(m_mMediaType.majortype==MEDIATYPE_Video), (int)this, GetTickCount() );
		m_fdLogs = fopen( FileName, "wt" );
	}
#endif
}

CGraphConnector::~CGraphConnector()
{
	CAutoLock lock( &m_QueueLock );
	ClearQueueContent();
	//maybe we should wait until these are properly disconnected 
	if( m_pInputPinForPush )
		delete m_pInputPinForPush;
	m_pInputPinForPush = NULL;
	if( m_pInputPinForPull )
		delete m_pInputPinForPull;
	m_pInputPinForPull = NULL;
	if( m_pInputFilter )
		m_pInputFilter->Release();
	m_pInputFilter = NULL;
	for( int i=0;i<CONNECTOR_MAX_OUTPUT_COUNT;i++)
	{
		if( m_pOutputPinForPush[i] )
			delete m_pOutputPinForPush[i];
		m_pOutputPinForPush[i] = NULL;

		if( m_pOutputPinForPull[i] )
			delete m_pOutputPinForPull[i];
		m_pOutputPinForPull[i] = NULL;

		m_pOutputPin[i] = NULL;

		if( m_pOutputFilter[i] )
			m_pOutputFilter[i]->Release();
		m_pOutputFilter[i] = NULL;
	}
	if( m_pAllocator )
		m_pAllocator->Release();
	m_pAllocator = NULL;
	m_nNumOutputs = 0;
	m_nAllocatorType = GCAT_INVALID;
}

CUnknown * WINAPI CGraphConnector::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    ASSERT(phr);
    
    CGraphConnector *pNewObject = new CGraphConnector(punk, phr);
    if (pNewObject == NULL) 
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return pNewObject;

} // CreateInstance

STDMETHODIMP CGraphConnector::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

	if( riid == __uuidof(IVidiatorGraphConnector) )
	{		
        return GetInterface((IVidiatorGraphConnector*)this, ppv);
	}

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);

}

HRESULT CGraphConnector::CopySample(IMediaSample *pMSsrc, IMediaSample *pMSdst )
{
    BYTE* pDest;
    BYTE* pSrc;
    if( pMSdst->GetPointer(&pDest) != S_OK )
	{
		DlogP("Error, target buffer pointer is missing");
		return S_FALSE;
	}
    if( pMSsrc->GetPointer(&pSrc) != S_OK )
	{
		DlogP("Error, source buffer pointer is missing");
		return S_FALSE;
	}

    long cOut = pMSdst->GetSize();
    long cIn = pMSsrc->GetActualDataLength();
    if( pMSdst->SetActualDataLength( cIn ) != S_OK )
	{
		DlogP2("Error 1, target buffer size is too small need %d, have %d \n", cIn, cOut );
		return VFW_E_BUFFER_OVERFLOW;
	}
	//this will most probably never execute
    if( cOut < cIn )
	{
		DlogP2("Error 2, target buffer size is too small need %d, have %d \n", cIn, cOut );
        return VFW_E_BUFFER_OVERFLOW;
	}
    
    CopyMemory( pDest, pSrc, cIn );

    REFERENCE_TIME tStart, tEnd;
	switch( pMSsrc->GetTime(&tStart,&tEnd) )
	{
		case S_OK: pMSdst->SetTime(&tStart,&tEnd); break;
		case VFW_S_NO_STOP_TIME: pMSdst->SetTime(&tStart,NULL); break;
		case VFW_E_SAMPLE_TIME_NOT_SET: pMSdst->SetTime(NULL,NULL);	break;
		default: 
			{
				DlogP("Error, source render duration missing");
				return VFW_E_SAMPLE_TIME_NOT_SET;
			}
	}
	switch( pMSsrc->GetMediaTime(&tStart,&tEnd) )
	{
		case S_OK: pMSdst->SetMediaTime(&tStart,&tEnd); break;
		case VFW_E_MEDIA_TIME_NOT_SET: pMSdst->SetMediaTime(NULL,NULL); break;
		default: 
			{
				DlogP("Error, source render duration missing");
				return VFW_E_SAMPLE_TIME_NOT_SET;
			}
	}

    if (pMSsrc->IsSyncPoint() == S_OK)
        pMSdst->SetSyncPoint(true);
	else
        pMSdst->SetSyncPoint(false);
    if (pMSsrc->IsDiscontinuity() == S_OK)
        pMSdst->SetDiscontinuity(true);
	else
        pMSdst->SetDiscontinuity(false);
    if (pMSsrc->IsPreroll() == S_OK)
        pMSdst->SetPreroll(true);
	else
        pMSdst->SetPreroll(false);

    AM_MEDIA_TYPE *pMediaType;
	HRESULT hr;
    hr = pMSsrc->GetMediaType( &pMediaType );
	if( hr == S_OK && pMediaType != NULL )
	{
		hr = pMSdst->SetMediaType( pMediaType );
		// update filter's media type
		m_mMediaType = *pMediaType;
		for( int i=0;i<CONNECTOR_MAX_OUTPUT_COUNT;i++)
		{
			if (m_pOutputPin[i])
			{
				m_pOutputPin[i]->SetMediaType(&m_mMediaType);
			}
		}
	}
    DeleteMediaType(pMediaType);
	
	return S_OK;
}

HRESULT CGraphConnector::IncreaseBufferSizeRuntime( int RequestedSize )
{
	//need to wait until all output is flushed
	bool NeedWait = false;
	int RetrysRemaining = 1000;	//wait aprox 1 second then give up
	do{
		if( NeedWait )
		{
			Sleep( 1 );
			RetrysRemaining--;
		}
		NeedWait = (IsOuputQueueEmpty() == S_OK) ? false : true;
	}while( NeedWait == true && RetrysRemaining > 0 );
	if( NeedWait == false )
	{
		IMemAllocator* pNewAllocator = NULL;
	    if (FAILED(CreateMemoryAllocator( &pNewAllocator )))
		{
			return S_FALSE;
		}
		m_pAllocator->Release();
		m_pAllocator = pNewAllocator;
		m_bBufferDecided = false;
		SetAllocatorProperties( RequestedSize );
		return S_OK;
	}
	return S_FALSE;
}

HRESULT CGraphConnector::SetAllocatorProperties( int RequestedSize )
{
	if( m_pInputPin->IsConnected() )
	{
		ALLOCATOR_PROPERTIES props;
		ALLOCATOR_PROPERTIES propsActual;
		HRESULT hr;
		IMemAllocator	*InputAllocator;
		hr = m_pInputPin->GetAllocator( &InputAllocator );
		if( hr != S_OK || InputAllocator == NULL )
		{
			Dlog("Could not set allocator properties 1\n");
			return S_FALSE;
		}
		hr = InputAllocator->GetProperties( &props );
		if( hr != S_OK || props.cBuffers == 0 || props.cbBuffer == 0 )
		{
			Dlog("Could not set allocator properties 2\n");
			return S_FALSE;
		}

		//more like a hack. I found some http input stream that was sending larger then agreed media buffers
		//If we reject the too large packet then input will DC. If we pass on the large packet then probably the next PIN will DC. Hope for the best
		//video stream packet can be for VBR anything. For CBR the rule is to be 3xbitrate
		if( ( m_mMediaType.majortype == MEDIATYPE_Video || m_mMediaType.majortype == MEDIATYPE_Stream ) && props.cbBuffer < 1024 * 8 ) //less then 8k ? probably size is not set. We need to know the size
			props.cbBuffer = 1024 * 8 * 1024; //8 Mbit ? I really hope this will be enough

		props.cbBuffer = max( props.cbBuffer * 2, RequestedSize ); 

		if( CONNECTOR_BUFFERS_UNTIL_LOCK > props.cBuffers )
			props.cBuffers = CONNECTOR_BUFFERS_UNTIL_LOCK;
		hr = m_pAllocator->SetProperties( &props, &propsActual );
		if( hr != S_OK )
		{
			Dlog1("Could not set allocator properties 4 %p\n",this);
			return S_FALSE;
		}
		hr = m_pAllocator->Commit();
		m_bBufferDecided = TRUE;
	}
}

STDMETHODIMP CGraphConnector::Receive(IMediaSample *pSample)
{
	//!!!make sure to be after buffer fetch or it will deadlock !
	//!!!do not use Autolock
//	CAutoLock lock( &m_QueueLock );
	
#ifdef DEBUGGING_IN_TESTBENCH
	LONGLONG start,end;
	pSample->GetTime( &start, &end );
	DlogVideo6("%d)Video input sample size %d for connector %p, queued %d, start %d, end %d\n", PacketCounterIn++, pSample->GetActualDataLength(), this, m_pSampleQueue[0].size(),(int)start,(int)end );
	DlogAudio6("%d)Audio input sample size %d for connector %p, queued %d, start %d, end %d\n", PacketCounterIn++, pSample->GetActualDataLength(), this, m_pSampleQueue[0].size(),(int)start,(int)end );
#endif
	IMediaSample *pLocalSample = NULL;
	if( m_nAllocatorType == GCAT_USE_FROM_INPUT )
	{
		pLocalSample = pSample;
		pLocalSample->AddRef(); 
	}
	else if( m_nAllocatorType == GCAT_USE_INTERNAL )
	{
		if( m_nAllocatorType == GCAT_USE_INTERNAL && m_bBufferDecided == FALSE )
			SetAllocatorProperties();
		if( m_bBufferDecided == FALSE )
		{
			Dlog("Error : Could not set allocator properties 5\n");
			return S_FALSE;
		}
		//!!! this can deadlock until fetchbuffer will release some buffers. Make sure to be outside of our list lock !
		int RetryCount = 1;
		HRESULT hr;
		do{
			hr = m_pAllocator->GetBuffer( &pLocalSample, NULL, NULL, 0 );
			if( FAILED(hr) ) 
			{
				Dlog("Error : Could not get free buffer to store sample! Should we block ?\n");
				return hr;
			}
			// copy the input sample content into the local sample
			hr = CopySample( pSample, pLocalSample );
			if ( hr != S_OK )
			{
				pLocalSample->Release();
				pLocalSample = NULL;
				HRESULT hr1 = IncreaseBufferSizeRuntime( pSample->GetActualDataLength() * 2 + 1024 );
				if( hr1 != S_OK )
					Dlog1("Error : Could not resize buffer pool, buffer sizes to %d\n", pSample->GetActualDataLength() + 1024);
				RetryCount--;
			}
		}while( hr != S_OK && RetryCount >= 0 );
		if( hr != S_OK )
		{
			Dlog("Error : Could not copy input into output buffer\n");
			return S_OK;
		}
	}
#ifdef DEBUGGING_IN_TESTBENCH
/*
	DumpSampleInfo( pSample, this, " Connector input(from src) " );
	if( pLocalSample != pSample )
		DumpSampleInfo( pLocalSample, this, " Connector input(local pooled) " );
	Dlog("Graph Connector received a media packet % d \n", PacketCounterIn++);
	*/
#endif
	m_QueueLock.Lock();
	if( m_Type == GCB_THROW_PACKET_UNLESS_OUTPUT_CONNECTED )
	{
		for( int i=0;i<m_nNumOutputs;i++)
			if( m_pOutputPin[i]->IsConnected() == TRUE )
			{
				pLocalSample->AddRef();
				m_pSampleQueue[i].push_front( pLocalSample );
			}
	}
	else if( m_Type == GCB_KEEP_PACKET_UNTIL_PUSHED )
	{
		for( int i=0;i<m_nNumOutputs;i++)
		{
			pLocalSample->AddRef();
			m_pSampleQueue[i].push_front( pLocalSample );
		}
	}
	pLocalSample->Release();
	m_QueueLock.Unlock();
	return S_OK;
}

STDMETHODIMP CGraphConnector::FetchPacket(IMediaSample **pSample, int OutInd)
{
	CAutoLock lock( &m_QueueLock );
	CheckPointer(pSample,E_POINTER);
	if( OutInd >= m_nNumOutputs )
	{
		*pSample = NULL;
		return S_FALSE;
	}
	if( m_pSampleQueue[OutInd].empty() )
	{
		*pSample = NULL;
		return S_FALSE;
	}
	if( m_Type == GCB_KEEP_PACKET_UNTIL_PUSHED || m_Type == GCB_THROW_PACKET_UNLESS_OUTPUT_CONNECTED )
	{
		*pSample = m_pSampleQueue[OutInd].back();
		m_pSampleQueue[OutInd].pop_back();
#ifdef DEBUGGING_IN_TESTBENCH
/*
		printf("Graph Connector is trying to send out a media packet % d\n", PacketCounterOut++ );
		DumpSampleInfo( *pSample, this, " Connector output " );
		*/
		//simulate a processing lag so we can see how the input handles it 
//		if( m_mMediaType.majortype == MEDIATYPE_Video )	Sleep( 500 ); //test uneven processing lag to stress test queue blocking
//		if( m_mMediaType.majortype == MEDIATYPE_Audio )	Sleep( 500 ); //test uneven processing lag to stress test queue blocking
		LONGLONG start,end;
		(*pSample)->GetTime( &start, &end );
		DlogVideo6("%d)Video output sample size %d for connector %p, queued %d, start %d, end %d \n", PacketCounterOut++, (*pSample)->GetActualDataLength(), this, (int)m_pSampleQueue[OutInd].size(),(int)start,(int)end );
		DlogAudio6("%d)Audio output sample size %d for connector %p, queued %d, start %d, end %d \n", PacketCounterOut++, (*pSample)->GetActualDataLength(), this, (int)m_pSampleQueue[OutInd].size(),(int)start,(int)end );
#endif
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::GetInputFilter( IBaseFilter **InputFilter )
{ 
	CheckPointer(InputFilter,E_POINTER);
	return m_pInputFilter->NonDelegatingQueryInterface( __uuidof( IBaseFilter ), (void**)InputFilter );
}
HRESULT STDMETHODCALLTYPE CGraphConnector::GetOutputFilter( IBaseFilter **OutputFilter, int OutInd )
{ 
	CheckPointer(OutputFilter,E_POINTER);
	if( OutInd >= m_nNumOutputs )
	{
		*OutputFilter = NULL;
		return S_FALSE;
	}
	return m_pOutputFilter[OutInd]->NonDelegatingQueryInterface( __uuidof( IBaseFilter ), (void**)OutputFilter );
}
HRESULT STDMETHODCALLTYPE CGraphConnector::GetConnectorInputPin( IPin **InputPin)
{
	CheckPointer(InputPin,E_POINTER);
	return m_pInputPin->NonDelegatingQueryInterface( __uuidof( IPin ), (void**)InputPin );
}
HRESULT STDMETHODCALLTYPE CGraphConnector::GetConnectorOutputPin( IPin **OutputPin, int OutInd )
{
	CheckPointer(OutputPin,E_POINTER);
	if( OutInd >= m_nNumOutputs )
	{
		*OutputPin = NULL;
		return S_FALSE;
	}
	return m_pOutputPin[OutInd]->NonDelegatingQueryInterface( __uuidof( IPin ), (void**)OutputPin );
}

HRESULT STDMETHODCALLTYPE CGraphConnector::GetCreateOutputFilter( IBaseFilter **OutputFilter )
{ 
	CheckPointer(OutputFilter,E_POINTER);
	HRESULT hr = AddOutput();
	*OutputFilter = NULL;
	if( hr != S_OK )
		return hr;
	return m_pOutputFilter[m_nNumOutputs-1]->NonDelegatingQueryInterface( __uuidof( IBaseFilter ), (void**)OutputFilter );
}

HRESULT STDMETHODCALLTYPE CGraphConnector::AddOutput()
{
	CAutoLock lock( &m_QueueLock );
	if( m_nNumOutputs >= CONNECTOR_MAX_OUTPUT_COUNT )
		return S_FALSE;
	HRESULT phr;
	IBaseFilter* pFilter = NULL;
	phr = CoCreateInstance(CLSID_GraphConnectorOutput, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pFilter);
	if( pFilter == NULL ) 
	{
		return E_OUTOFMEMORY;
	}
	m_pOutputFilter[m_nNumOutputs] = (CGraphConnectorOutputFilter*)pFilter;
	m_pOutputFilter[m_nNumOutputs]->m_pConnector = this;
	m_pOutputFilter[m_nNumOutputs]->m_nConnectorInd = m_nNumOutputs;

	if( m_bUseAsyncReaderOutput == FALSE )
	{
		m_pOutputPinForPush[m_nNumOutputs] = new CGraphConnectorOutputPin(this,GetOwner(),
									m_pOutputFilter[m_nNumOutputs],
									&m_Lock,
									&m_QueueLock,
									&phr,
									m_nNumOutputs);
		m_pOutputPin[m_nNumOutputs] = m_pOutputPinForPush[m_nNumOutputs];
	}
	else
	{
		m_pOutputPinForPull[m_nNumOutputs] = new CGraphConnectorOutputPinAsync(this,GetOwner(),
									m_pOutputFilter[m_nNumOutputs],
									&m_Lock,
									&m_QueueLock,
									&phr,
									m_nNumOutputs);
		m_pOutputPinForPull[m_nNumOutputs]->SetAllocator( m_pAllocator );
		m_pOutputPin[m_nNumOutputs] = m_pOutputPinForPull[m_nNumOutputs];
	}

	if (m_pOutputPin[m_nNumOutputs] == NULL)
	{
		return E_OUTOFMEMORY;
	}
	m_nNumOutputs++;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::IsOuputQueueEmpty()
{
	CAutoLock lock( &m_QueueLock );
	for( int i=0;i<m_nNumOutputs;i++)
		if( m_pSampleQueue[i].empty() == FALSE )
			return S_FALSE;
	return S_OK;
}

CBasePin *CGraphConnector::GetPin(int n)
{
	if( n == 0 )
	{
		return (CBasePin *)m_pInputPin;
	}
	else if( n >= 1 && n <= m_nNumOutputs )
	{
		return (CBasePin *)m_pOutputPin[n-1];
	}
	return NULL;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::ClearQueueContent( int OutInd )
{
	CAutoLock lock(&m_QueueLock);
	if( OutInd >= m_nNumOutputs )
		return S_FALSE;
	while( m_pSampleQueue[OutInd].empty() == false )
	{
		IMediaSample *p = m_pSampleQueue[OutInd].back();
		m_pSampleQueue[OutInd].pop_back();
		p->Release();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::ClearQueueContent()
{
	CAutoLock lock(&m_QueueLock);
	for( int i=0;i<m_nNumOutputs;i++)
		ClearQueueContent( i );
	return S_OK;
}

HRESULT CGraphConnector::CheckMediaType(const CMediaType *mt)
{
	//not yet initialized ? We accept anything
	if( m_mMediaType.majortype == GUID_NULL )
		return S_OK;
	if( mt->majortype != m_mMediaType.majortype )
		return S_FALSE;
	//maybe we should check for compatible media types
	if( mt->subtype != m_mMediaType.subtype )
		return S_FALSE;
	return S_OK;
}

HRESULT CGraphConnector::SetMediaType(const CMediaType *mt)
{
	m_mMediaType = *mt;
	return S_OK;
}

HRESULT CGraphConnector::GetMediaType(CMediaType *mt)
{
	*mt = m_mMediaType;
	return S_OK;
}

HRESULT CGraphConnector::GetAllocatorProperties( ALLOCATOR_PROPERTIES* pProperties )
{
	pProperties->cbAlign = 1;
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1;
	//try to fetch latest still connected input PIN properties
	if( m_pInputPin->IsConnected() == true )
	{
		HRESULT hr;
		IMemAllocator	*InputAllocator;
		hr = m_pInputPin->GetAllocator( &InputAllocator );
		if( hr != S_OK || InputAllocator == NULL )
		{
			Dlog("Could not set allocator properties 1\n");
			return S_FALSE;
		}
		hr = InputAllocator->GetProperties( pProperties );
		if( hr != S_OK )
		{
			Dlog("Could not set allocator properties 2\n");
			return S_FALSE;
		}
		//try to fetch buffer properties from input. Update it, even if we already had a previous connection
		// GetAllocatorProperties function is called when the filters are getting connected
		//SetAllocatorProperties();
	}
	//if right now input PIN is not connected then try to fetch our memorized properties
	else if( m_bBufferDecided == TRUE )
	{
		m_pAllocator->GetProperties( pProperties );
	}
	else
	{
		return S_FALSE;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::SetBehaviorType( int Type )
{
	if( Type == GCB_KEEP_PACKET_UNTIL_PUSHED || Type == GCB_THROW_PACKET_UNLESS_OUTPUT_CONNECTED )
	{
		m_Type = (GraphConnectorBehavior)Type;
		return S_OK;
	}
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::SwitchToAsyncPullInputPin( )
{
	if( m_pInputPinForPull != NULL )
		return S_FALSE;

	//we already have outputs assigned ? Until tested disabling mixed mode. In theory they might work
	if( m_nNumOutputs != 0 )
		return S_FALSE;

	m_bUseAsyncReaderOutput = TRUE;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::SwitchToAsyncReaderOutputPin( )
{
	if( m_pInputPinForPull != NULL )
		return S_FALSE;

	HRESULT hr;
    m_pInputPinForPull = new CGraphConnectorInputPinAsync(this,GetOwner(),
                               m_pInputFilter,
                               &m_Lock,
                               &m_QueueLock,
                               &hr);

	m_pInputPin = m_pInputPinForPull;

	//share our allocator. No need to complicate mem copies
	m_pInputPinForPull->SetAllocator( m_pAllocator );

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::GetAsyncReadLength( LONGLONG* pTotal, LONGLONG* pAvailable )
{
	if( m_pInputPinForPull && m_pInputPinForPull->IsConnected() )
		return m_pInputPinForPull->Length( pTotal, pAvailable );
	//unknown ? 
	*pTotal = 0;
	*pAvailable = 0;
	return E_NOT_VALID_STATE;
}

HRESULT STDMETHODCALLTYPE CGraphConnector::SyncRead( LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	if( m_pInputPinForPull && m_pInputPinForPull->IsConnected() )
		return m_pInputPinForPull->SyncRead( llPosition, lLength, pBuffer );
	return E_NOT_VALID_STATE;
}

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} 


STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} 

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

