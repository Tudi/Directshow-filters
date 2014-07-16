#include "StdAfx.h"
#include "VidiatorFilterGuids.h"

////////////////////////////////////////////////////////////////////////
// Filter 
////////////////////////////////////////////////////////////////////////

// Constructor
CGraphConnectorOutputFilter::CGraphConnectorOutputFilter(CGraphConnector *pConnector,
                         LPUNKNOWN pUnk,
                         CCritSec *pLock,
                         HRESULT *phr,
						 int ind
						 ) :
    CSource(NAME("CGraphConnectorOutputFilter"), pUnk, CLSID_GraphConnectorOutput),
    m_pConnector(pConnector)
{
	m_nConnectorInd = ind;
}

CGraphConnectorOutputFilter::CGraphConnectorOutputFilter( LPUNKNOWN pUnk, HRESULT *phr ) :
    CSource(NAME("CGraphConnectorOutputFilter"), pUnk, CLSID_GraphConnectorOutput)
{
	m_pConnector = NULL;
	m_nConnectorInd = -1;
}

CUnknown * WINAPI CGraphConnectorOutputFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    ASSERT(phr);
    CGraphConnectorOutputFilter *pNewObject = new CGraphConnectorOutputFilter(punk, phr);
    if (pNewObject == NULL) 
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
} // CreateInstance

CBasePin * CGraphConnectorOutputFilter::GetPin(int n)
{
    if (n == 0) 
	{
		return m_pConnector->GetPin( m_nConnectorInd + 1 );
    }
    return NULL;
}

int CGraphConnectorOutputFilter::GetPinCount()
{
    return 1;
}

////////////////////////////////////////////////////////////////////////
// Push model output Pin 
////////////////////////////////////////////////////////////////////////

CGraphConnectorOutputPin::CGraphConnectorOutputPin(CGraphConnector *pConnector,
                             LPUNKNOWN pUnk,
                             CSource  *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr,
							 int ind
							 ) :

    CSourceStream(NAME("CGraphConnectorOutputPin"),
                  phr,                       // Return code
                  pFilter,
                  L"Output"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pConnector(pConnector),
	m_bEndOfStream( FALSE ),
	m_bFirstPacket( FALSE )
//	m_PrevStartTime( TIME_Unknown ),
//	m_pRefClock( NULL ),
//	m_ReportTime( 0 ),
{
	m_nConnectorInd = ind;
}

HRESULT CGraphConnectorOutputPin::DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	HRESULT hResult = NOERROR;

	CheckPointer(pMemAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	ALLOCATOR_PROPERTIES ConnectorProperties;
	m_pConnector->GetAllocatorProperties( &ConnectorProperties );

	if( pProperties->cBuffers < ConnectorProperties.cBuffers )
		pProperties->cBuffers = ConnectorProperties.cBuffers;

	if( pProperties->cbBuffer < ConnectorProperties.cbBuffer )
		pProperties->cbBuffer = ConnectorProperties.cbBuffer;

	if( pProperties->cBuffers < 1 )
		pProperties->cBuffers = 1;

	if( pProperties->cbBuffer < 1 )
		pProperties->cbBuffer = 1;

	ALLOCATOR_PROPERTIES Actual;
	if( FAILED(hResult = pMemAlloc->SetProperties(pProperties, &Actual)) )
	{
		return hResult;
	}

	if(Actual.cbBuffer < pProperties->cbBuffer ||
		Actual.cBuffers < pProperties->cBuffers ) 
		return E_FAIL;

	return NOERROR;
}

//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CGraphConnectorOutputPin::DoBufferProcessingLoop(void) 
{
	Command com;
	OnThreadStartPlay();

	do 
	{
		while (!CheckRequest(&com)) 
		{
			IMediaSample *pSample = NULL;
			HRESULT hr = S_FALSE;
			if( m_pConnector )
				hr = m_pConnector->FetchPacket( &pSample, m_nConnectorInd );
//			pSample->SetSyncPoint( TRUE );	
			if (hr == S_OK) 
			{
				REFERENCE_TIME tStart, tEnd;
				pSample->GetTime(&tStart, &tEnd);
				if (tStart < 0 || tEnd < 0)
				{
					// skip negative timestamp samples, they can drive ffdshow decoder crazy
					pSample->Release();
				}
				else
				{
					if( m_bFirstPacket == TRUE )
					{
						pSample->SetDiscontinuity( TRUE );
						//pSample->SetPreroll( TRUE );
						//pSample->SetSyncPoint( TRUE );
						m_bFirstPacket = FALSE;
					}
					hr = Deliver(pSample);
					pSample->Release();
					// downstream filter returns S_FALSE if it wants us to
					// stop or an error if it's reporting an error.
					if(hr != S_OK)
					{
						return S_OK;
					}
				}

			} 
			else if (hr == S_FALSE) 
			{
				// derived class wants us to stop pushing data
				if( pSample )
					pSample->Release();
				if( m_bEndOfStream )
				{
					DeliverEndOfStream();
					return S_OK;
				}				
				//Maybe we should wait until there is some buffer to be pushed ? Might lead to high CPU usage
				Sleep( DEFAULT_THREAD_WAIT_SLEEP_MS );
			} 
			else 
			{
				// derived class encountered an error
				if( pSample )
					pSample->Release();
				DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}

			// all paths release the sample
			Sleep(1);
		}
		// For all commands sent to us there must be a Reply call!
		if (com == CMD_RUN || com == CMD_PAUSE)
		{
			Reply(NOERROR);
		} 
		else if (com != CMD_STOP)
		{
			Reply((DWORD) E_UNEXPECTED);
		}
	} while (com != CMD_STOP);

	return S_FALSE;
}
//
// Active
//
// The pin is active - start up the worker thread
HRESULT CGraphConnectorOutputPin::Active(void) 
{
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;

	if (m_pFilter->IsActive()) 
	{
		return S_FALSE;	// succeeded, but did not allocate resources (they already exist...)
	}

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if (!IsConnected()) 
	{
		return NOERROR;
	}

	m_bFirstPacket = TRUE;
//	m_PrevStartTime = TIME_Unknown;

	hr = CBaseOutputPin::Active();
	if (FAILED(hr)) 
	{
		return hr;
	}

	ASSERT(!ThreadExists());
	
	//////////////////////
	// Get
//	if( m_pRefClock )
//	{ 
//		m_pRefClock->Release(); 
//		m_pRefClock = NULL;
//	}	
	
//	if( FAILED( m_pFilter->GetSyncSource( &m_pRefClock ) ) || m_pRefClock == NULL ) {}
		
//	m_TimerReportStream.Reset( );
//	m_ReportTime = 0;
//	m_RefClockStart = 0;
//	if( m_pRefClock )
//		m_pRefClock->GetTime( &m_RefClockStart );

	// start the thread
	if (!Create()) 
	{
		return E_FAIL;
	}

	// Tell thread to initialize. If OnThreadCreate Fails, so does this.
	hr = Init();
	if (FAILED(hr))
		return hr;

	return Pause();
}

HRESULT CGraphConnectorOutputPin::Inactive(void) {

	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;

	// do nothing if not connected - its ok not to connect to
	// all pins of a source filter
	if( !IsConnected() )
	{
		return NOERROR;
	}

	// !!! need to do this before trying to stop the thread, because
	// we may be stuck waiting for our own allocator!!!

	hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
	if (FAILED(hr))
	{
		return hr;
	}

	if (ThreadExists()) 
	{
		hr = Stop();

		if (FAILED(hr))
		{
			return hr;
		}

		hr = Exit();
		if (FAILED(hr))
		{
			return hr;
		}

		Close();	// Wait for the thread to exit, then tidy up.
	}

//	if( m_pRefClock )
//	{ 
//		m_pRefClock->Release(); 
//		m_pRefClock = NULL; 
//	}	

	return NOERROR;
}

HRESULT CGraphConnectorOutputPin::GetMediaType( CMediaType *mt)
{
	CAutoLock Lock(m_pFilter->pStateLock());
	CheckPointer(mt, E_POINTER);

	if( m_pConnector )
		m_pConnector->GetMediaType( mt );

	return S_OK;
}

HRESULT CGraphConnectorOutputPin::SetMediaType( CMediaType *mt )
{
	return __super::SetMediaType( mt );
}

////////////////////////////////////////////////////////////////////////
// Pull model output Pin 
////////////////////////////////////////////////////////////////////////


CGraphConnectorOutputPinAsync::CGraphConnectorOutputPinAsync(CGraphConnector *pConnector,
                  LPUNKNOWN   pUnk,
                  CSource     *pFilter,
                  CCritSec    *pLock,
                  CCritSec    *pReceiveLock,
                  HRESULT     *phr,
				  int			ind)
  : CBasePin(
    NAME("Async output pin"),
    pFilter,
    pLock,
    phr,
    L"Output",PINDIR_OUTPUT),
    m_pConnector(pConnector),
	m_nConnectorInd(ind),
	m_pReceiveLock( pReceiveLock )
{
	m_pAlloc = NULL;
}

CGraphConnectorOutputPinAsync::~CGraphConnectorOutputPinAsync()
{
}

STDMETHODIMP CGraphConnectorOutputPinAsync::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if(riid == IID_IAsyncReader)
    {
        return GetInterface((IAsyncReader*) this, ppv);
    }
    else
    {
        return CBasePin::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT CGraphConnectorOutputPinAsync::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if(iPosition < 0)
    {
        return E_INVALIDARG;
    }
    if(iPosition > 0)
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER); 
    CheckPointer(m_pConnector,E_UNEXPECTED); 
    
	m_pConnector->GetMediaType( pMediaType );

    return S_OK;
}

HRESULT CGraphConnectorOutputPinAsync::CheckMediaType(const CMediaType *mt)
{
    CAutoLock lock( m_pLock );
	if( m_pConnector )
		return m_pConnector->CheckMediaType( mt );

    return S_FALSE;
}

// we need to return an addrefed allocator, even if it is the preferred
// one, since he doesn't know whether it is the preferred one or not.
STDMETHODIMP CGraphConnectorOutputPinAsync::RequestAllocator( IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator ** ppActual)
{
    CheckPointer(pPreferred,E_POINTER);
    CheckPointer(pProps,E_POINTER);
    CheckPointer(ppActual,E_POINTER);

    HRESULT hr = S_FALSE;
    // we care about alignment but nothing else
    pProps->cbAlign = 1;

    ALLOCATOR_PROPERTIES Actual;

    if(pPreferred)
    {
        hr = pPreferred->SetProperties(pProps, &Actual);

        if(SUCCEEDED(hr) && Actual.cbAlign)
        {
            pPreferred->AddRef();
            *ppActual = pPreferred;
            return S_OK;
        }
    }

    hr = m_pAlloc->SetProperties(pProps, &Actual);
    if( SUCCEEDED(hr) )
    {
		if( Actual.cbAlign )
		{
			*ppActual = m_pAlloc;
			return S_OK;
		}
		else
	        hr = VFW_E_BADALIGN;
    }
    return hr;
}

// queue an aligned read request. call WaitForNext to get completion.
STDMETHODIMP CGraphConnectorOutputPinAsync::Request( IMediaSample* pSample, DWORD_PTR dwUser)           // user context
{
    CheckPointer(pSample,E_POINTER);

    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if(FAILED(hr))
    {
        return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal=0, llAvailable=0;

/*    hr = m_pIo->Length(&llTotal, &llAvailable);
    if(llPos + lLength > llTotal)
    {
        // the end needs to be aligned, but may have been aligned
        // on a coarser alignment.
        LONG lAlign;
        m_pIo->Alignment(&lAlign);

        llTotal = (llTotal + lAlign -1) & ~(lAlign-1);

        if(llPos + lLength > llTotal)
        {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }
    }

    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if(FAILED(hr))
    {
        return hr;
    }

    return m_pIo->Request(llPos,
                          lLength,
                          TRUE,
                          pBuffer,
                          (LPVOID)pSample,
                          dwUser);
						  */
}

// sync-aligned request. just like a request/waitfornext pair.
STDMETHODIMP CGraphConnectorOutputPinAsync::SyncReadAligned(IMediaSample* pSample)
{
	return S_FALSE; //do not support it yet, we are transparent and passing input as we received it :(
/*
    CheckPointer(pSample,E_POINTER);

    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if(FAILED(hr))
    {
        return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal;
    LONGLONG llAvailable;

    hr = m_pIo->Length(&llTotal, &llAvailable);
    if(llPos + lLength > llTotal)
    {
        // the end needs to be aligned, but may have been aligned
        // on a coarser alignment.
        LONG lAlign;
        m_pIo->Alignment(&lAlign);

        llTotal = (llTotal + lAlign -1) & ~(lAlign-1);

        if(llPos + lLength > llTotal)
        {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }
    }

    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if(FAILED(hr))
    {
        return hr;
    }

    LONG cbActual;
    hr = m_pIo->SyncReadAligned(llPos,
                                lLength,
                                pBuffer,
                                &cbActual,
                                pSample);

    pSample->SetActualDataLength(cbActual); 
    return hr; */
}


//
// collect the next ready sample
STDMETHODIMP CGraphConnectorOutputPinAsync::WaitForNext( DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR * pdwUser)
{
    CheckPointer(ppSample,E_POINTER);

    LONG cbActual;
    IMediaSample* pSample=0;
	HRESULT hr;

/*    hr = m_pIo->WaitForNext(dwTimeout,
                                    (LPVOID*) &pSample,
                                    pdwUser,
                                    &cbActual);
									*/
    if(SUCCEEDED(hr))
    {
        pSample->SetActualDataLength(cbActual);
    }

    *ppSample = pSample;
    return hr;
}

STDMETHODIMP CGraphConnectorOutputPinAsync::SyncRead( LONGLONG llPosition, LONG lLength, BYTE* pBuffer)  
{
 //   return m_pIo->SyncRead(llPosition, lLength, pBuffer);
	return S_FALSE; //do not support it yet, we are transparent and passing input as we received it :(
}


// return the length of the file, and the length currently
// available locally. We only support locally accessible files,
// so they are always the same
STDMETHODIMP CGraphConnectorOutputPinAsync::Length( LONGLONG* pTotal, LONGLONG* pAvailable )
{
    return m_pConnector->GetAsyncReadLength(pTotal, pAvailable);
}


STDMETHODIMP CGraphConnectorOutputPinAsync::BeginFlush(void)
{
	m_pConnector->ClearQueueContent( m_nConnectorInd );
//    return m_pIo->BeginFlush();
	return S_OK;
}


STDMETHODIMP CGraphConnectorOutputPinAsync::EndFlush(void)
{
//    return m_pIo->EndFlush();
	return S_OK;
}

STDMETHODIMP CGraphConnectorOutputPinAsync::SetAllocator( __deref_in IMemAllocator *pAllocator )
{
	CheckPointer(pAllocator, E_POINTER);
	if( m_pAlloc != NULL )
		return S_FALSE;
	m_pAlloc = pAllocator;
}
/*
STDMETHODIMP CGraphConnectorOutputPinAsync::Connect( IPin * pReceivePin, const AM_MEDIA_TYPE *pmt )
{
//    return m_pConnector->Connect( pReceivePin, pmt );
	return S_OK;
}*/


