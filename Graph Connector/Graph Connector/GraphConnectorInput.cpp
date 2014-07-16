#include "StdAfx.h"
#include "VidiatorFilterGuids.h"

////////////////////////////////////////////////////////////////////////
// Filter 
////////////////////////////////////////////////////////////////////////

// Constructor
CGraphConnectorInputFilter::CGraphConnectorInputFilter(CGraphConnector *pConnector,
                         LPUNKNOWN pUnk,
                         CCritSec *pLock,
                         HRESULT *phr) :
    CBaseFilter(NAME("CGraphConnectorInputFilter"), pUnk, pLock, CLSID_GraphConnectorInput),
    m_pConnector(pConnector),
	m_pPosition(NULL)
{
}
	// Constructor
CGraphConnectorInputFilter::CGraphConnectorInputFilter( LPUNKNOWN pUnk, HRESULT *phr) :
    CBaseFilter(NAME("CGraphConnectorInputFilter"), pUnk, &m_Lock, CLSID_GraphConnectorInput),
    m_pConnector(NULL),
	m_pPosition(NULL)
{
}

CGraphConnectorInputFilter::~CGraphConnectorInputFilter()
{
	if (m_pPosition)
	{
		delete m_pPosition;
		m_pPosition = NULL;
	}
}

CUnknown * WINAPI CGraphConnectorInputFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    ASSERT(phr);
    CGraphConnectorInputFilter *pNewObject = new CGraphConnectorInputFilter(punk, phr);
    if (pNewObject == NULL) 
	{
        if (phr)
            *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
} // CreateInstance

CBasePin * CGraphConnectorInputFilter::GetPin(int n)
{
    if (n == 0) 
	{
		return m_pConnector->GetPin( 0 );
    }
    return NULL;
}

int CGraphConnectorInputFilter::GetPinCount()
{
    return 1;
}

STDMETHODIMP CGraphConnectorInputFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
	{

        if (m_pPosition == NULL) 
        {

            HRESULT hr = S_OK;
			m_pPosition = new CPosPassThru(NAME("GraphConnectorInputFilter Pass Through"), (IUnknown *) GetOwner(), (HRESULT *) &hr, m_pConnector->GetPin( 0 ));
            if (m_pPosition == NULL) 
                return E_OUTOFMEMORY;

            if (FAILED(hr)) 
            {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }

        }

        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);

    } 

    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);

}

////////////////////////////////////////////////////////////////////////
// Push model input Pin 
////////////////////////////////////////////////////////////////////////

CGraphConnectorInputPin::CGraphConnectorInputPin(CGraphConnector *pConnector,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr) :

    CRenderedInputPin(NAME("CGraphConnectorInputPin"),
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  L"Input"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pConnector(pConnector)
{
}

HRESULT CGraphConnectorInputPin::CheckMediaType(const CMediaType *mt)
{
    CAutoLock lock(m_pReceiveLock);
	if( m_pConnector )
		return m_pConnector->CheckMediaType( mt );
    return S_OK;
}

HRESULT CGraphConnectorInputPin::SetMediaType(const CMediaType *mt)
{
    CAutoLock lock(m_pReceiveLock);
	if( m_pConnector )
		return m_pConnector->SetMediaType( mt );
	return S_OK;
}

STDMETHODIMP CGraphConnectorInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

STDMETHODIMP CGraphConnectorInputPin::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample,E_POINTER);
	//!! do not lock it here or allocator will block fetch thread
//    CAutoLock lock(m_pReceiveLock);
	if( m_pConnector )
		return m_pConnector->Receive( pSample );
	return S_FALSE;
}

HRESULT	CGraphConnectorInputPin::CompleteConnect(IPin *pReceivePin)
{
	if( m_pConnector )
		m_pConnector->SetAllocatorProperties();

	return __super::CompleteConnect(pReceivePin);
}

////////////////////////////////////////////////////////////////////////
// Pull model input Pin 
////////////////////////////////////////////////////////////////////////

CGraphConnectorInputPinAsync::CGraphConnectorInputPinAsync(CGraphConnector *pConnector,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr) :
    CBaseInputPin( L"MGC Async Input",
       pFilter,
        pLock,
        phr,
        L"Input"),
	m_pReader(NULL),
    m_pAlloc(NULL),
    m_State(TM_Exit),
    m_pReceiveLock(pReceiveLock),
    m_pConnector(pConnector)
{
}

CGraphConnectorInputPinAsync::~CGraphConnectorInputPinAsync()
{
    Disconnect();
}

STDMETHODIMP CGraphConnectorInputPinAsync::SetAllocator(__deref_in IMemAllocator *pAllocator)
{
	if( m_pAlloc != NULL )
		return S_FALSE;
	m_pAlloc = pAllocator;
	return S_OK;
}

STDMETHODIMP CGraphConnectorInputPinAsync::GetAllocator(__deref_out IMemAllocator ** ppAllocator)
{
	*ppAllocator = m_pAlloc;
	return S_OK;
}

HRESULT	CGraphConnectorInputPinAsync::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock lock(&m_AccessLock);

    if (m_pReader) 
	{
		return VFW_E_ALREADY_CONNECTED;
    }

    HRESULT hr = pReceivePin->QueryInterface(IID_IAsyncReader, (void**)&m_pReader);
    if (FAILED(hr))
	{
		return(hr);
    }

    LONGLONG llTotal, llAvail;
    hr = m_pReader->Length(&llTotal, &llAvail);
    if (FAILED(hr)) 
	{
		Disconnect();
		return hr;
    }

    // convert from file position to reference time
    m_tDuration = llTotal * UNITS;
    m_tStop = m_tDuration;
    m_tStart = 0;

    m_bSync = 0;

	if( m_pConnector )
		m_pConnector->SetAllocatorProperties();

	return __super::CompleteConnect(pReceivePin);
}

// disconnect any connection made in Connect
HRESULT CGraphConnectorInputPinAsync::Disconnect()
{
    CAutoLock lock(&m_AccessLock);

    StopThread();

    if (m_pReader) 
	{
		m_pReader->Release();
		m_pReader = NULL;
    }

    if (m_pAlloc) 
	{
		m_pAlloc->Release();
		m_pAlloc = NULL;
    }

    return S_OK;
}

// agree an allocator using RequestAllocator - optional
// props param specifies your requirements (non-zero fields).
// returns an error code if fail to match requirements.
// optional IMemAllocator interface is offered as a preferred allocator
// but no error occurs if it can't be met.
HRESULT CGraphConnectorInputPinAsync::DecideAllocator( IMemAllocator * pAlloc,  __inout_opt ALLOCATOR_PROPERTIES * pProps)
{
    ALLOCATOR_PROPERTIES *pRequest;
    ALLOCATOR_PROPERTIES Request;
    if (pProps == NULL) 
	{
		Request.cBuffers = 3;
		Request.cbBuffer = 64*1024;
		Request.cbAlign = 0;
		Request.cbPrefix = 0;
		pRequest = &Request;
    } 
	else 
	{
		pRequest = pProps;
    }
    HRESULT hr = m_pReader->RequestAllocator( pAlloc, pRequest, &m_pAlloc);
    return hr;
}

// start pulling data
HRESULT CGraphConnectorInputPinAsync::Active(void)
{
    ASSERT(!ThreadExists());
    return StartThread();
}

// stop pulling data
HRESULT CGraphConnectorInputPinAsync::Inactive(void)
{
    StopThread();

    return S_OK;
}

HRESULT CGraphConnectorInputPinAsync::Seek(REFERENCE_TIME tStart, REFERENCE_TIME tStop)
{
    CAutoLock lock(&m_AccessLock);

    ThreadMsg AtStart = m_State;

    if (AtStart == TM_Start)
	{
		BeginFlush();
		PauseThread();
		EndFlush();
    }

    m_tStart = tStart;
    m_tStop = tStop;

    HRESULT hr = S_OK;
    if (AtStart == TM_Start) 
	{
		hr = StartThread();
    }

    return hr;
}

HRESULT CGraphConnectorInputPinAsync::Duration(__out REFERENCE_TIME* ptDuration)
{
    *ptDuration = m_tDuration;
    return S_OK;
}


HRESULT CGraphConnectorInputPinAsync::StartThread()
{
    CAutoLock lock(&m_AccessLock);

    if (!m_pAlloc || !m_pReader) 
	{
		return E_UNEXPECTED;
    }

    HRESULT hr;
    if (!ThreadExists())
	{
		// commit allocator
		hr = m_pAlloc->Commit();
		if (FAILED(hr)) 
		{
			return hr;
		}
		// start thread
		if (!Create()) 
		{
			return E_FAIL;
		}
    }

    m_State = TM_Start;
    hr = (HRESULT) CallWorker(m_State);
    return hr;
}

HRESULT CGraphConnectorInputPinAsync::PauseThread()
{
    CAutoLock lock(&m_AccessLock);

    if (!ThreadExists())
	{
		return E_UNEXPECTED;
    }

    // need to flush to ensure the thread is not blocked
    // in WaitForNext
    HRESULT hr = m_pReader->BeginFlush();
    if (FAILED(hr)) 
	{
		return hr;
    }

    m_State = TM_Pause;
    hr = CallWorker(TM_Pause);

    m_pReader->EndFlush();
    return hr;
}

HRESULT CGraphConnectorInputPinAsync::StopThread()
{
    CAutoLock lock(&m_AccessLock);

    if (!ThreadExists())
	{
		return S_FALSE;
    }

    // need to flush to ensure the thread is not blocked
    // in WaitForNext
    HRESULT hr = m_pReader->BeginFlush();
    if (FAILED(hr)) 
	{
		return hr;
    }

    m_State = TM_Exit;
    hr = CallWorker(TM_Exit);

    m_pReader->EndFlush();

    // wait for thread to completely exit
    Close();

    // decommit allocator
    if (m_pAlloc) 
	{
		m_pAlloc->Decommit();
	}

    return S_OK;
}

DWORD CGraphConnectorInputPinAsync::ThreadProc(void)
{
    while(1)
	{
		DWORD cmd = GetRequest();
		switch(cmd) 
		{
		case TM_Exit:
			Reply(S_OK);
			return 0;

		case TM_Pause:
			// we are paused already
			Reply(S_OK);
			break;

		case TM_Start:
			Reply(S_OK);
			Process();
			break;
		}

		// at this point, there should be no outstanding requests on the
		// upstream filter.
		// We should force begin/endflush to ensure that this is true.
		// !!!Note that we may currently be inside a BeginFlush/EndFlush pair
		// on another thread, but the premature EndFlush will do no harm now
		// that we are idle.
		m_pReader->BeginFlush();
		CleanupCancelled();
		m_pReader->EndFlush();
    }
}

HRESULT CGraphConnectorInputPinAsync::QueueSample( __inout REFERENCE_TIME& tCurrent, REFERENCE_TIME tAlignStop, BOOL bDiscontinuity )
{
    IMediaSample* pSample;

    HRESULT hr = m_pAlloc->GetBuffer(&pSample, NULL, NULL, 0);
    if (FAILED(hr)) 
	{
		return hr;
    }

    LONGLONG tStopThis = tCurrent + (pSample->GetSize() * UNITS);
    if (tStopThis > tAlignStop)
	{
		tStopThis = tAlignStop;
    }
    pSample->SetTime(&tCurrent, &tStopThis);
    tCurrent = tStopThis;

    pSample->SetDiscontinuity(bDiscontinuity);

    hr = m_pReader->Request( pSample, 0 );
    if (FAILED(hr)) 
	{
		pSample->Release();
		CleanupCancelled();
		OnError(hr);
    }
    return hr;
}

HRESULT CGraphConnectorInputPinAsync::CollectAndDeliver( REFERENCE_TIME tStart, REFERENCE_TIME tStop)
{
    IMediaSample* pSample = NULL;   // better be sure pSample is set
    DWORD_PTR dwUnused;
    HRESULT hr = m_pReader->WaitForNext( INFINITE, &pSample, &dwUnused );
    if (FAILED(hr)) 
	{
		if (pSample) 
		{
			pSample->Release();
		}
	} 
	else 
	{
		hr = DeliverSample(pSample, tStart, tStop);
	}
	if (FAILED(hr)) 
	{
		CleanupCancelled();
		OnError(hr);
	}
	return hr;
}

HRESULT CGraphConnectorInputPinAsync::DeliverSample(
    IMediaSample* pSample,
    REFERENCE_TIME tStart,
    REFERENCE_TIME tStop
    )
{
    // fix up sample if past actual stop (for sector alignment)
    REFERENCE_TIME t1, t2;
    if (S_OK == pSample->GetTime(&t1, &t2))
	{
        if (t2 > tStop) 
		{
            t2 = tStop;
        }

        // adjust times to be relative to (aligned) start time
        t1 -= tStart;
        t2 -= tStart;
        HRESULT hr = pSample->SetTime(&t1, &t2);
        if (FAILED(hr)) 
		{
            return hr;
        }
    }

    HRESULT hr = Receive(pSample);
    pSample->Release();
    return hr;
}

void CGraphConnectorInputPinAsync::Process(void)
{
    // is there anything to do?
    if (m_tStop <= m_tStart) 
	{
		EndOfStream();
		return;
    }

    BOOL bDiscontinuity = TRUE;

    // if there is more than one sample at the allocator,
    // then try to queue 2 at once in order to overlap.
    // -- get buffer count and required alignment
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = m_pAlloc->GetProperties(&Actual);

    // align the start position downwards
    REFERENCE_TIME tStart = AlignDown(m_tStart / UNITS, Actual.cbAlign) * UNITS;
    REFERENCE_TIME tCurrent = tStart;

    REFERENCE_TIME tStop = m_tStop;
    if (tStop > m_tDuration) 
	{
		tStop = m_tDuration;
    }

    // align the stop position - may be past stop, but that
    // doesn't matter
    REFERENCE_TIME tAlignStop = AlignUp(tStop / UNITS, Actual.cbAlign) * UNITS;

    DWORD dwRequest;

    if (!m_bSync)
	{
		//  Break out of the loop either if we get to the end or we're asked
		//  to do something else
		while (tCurrent < tAlignStop) 
		{
			// Break out without calling EndOfStream if we're asked to
			// do something different
			if (CheckRequest(&dwRequest)) 
			{
				return;
			}

			// queue a first sample
			if (Actual.cBuffers > 1)
			{
				hr = QueueSample(tCurrent, tAlignStop, TRUE);
				bDiscontinuity = FALSE;

				if (FAILED(hr))
				{
					return;
				}
			}
			// loop queueing second and waiting for first..
			while (tCurrent < tAlignStop) 
			{

				hr = QueueSample(tCurrent, tAlignStop, bDiscontinuity);
				bDiscontinuity = FALSE;

				if (FAILED(hr))
				{
					return;
				}

				hr = CollectAndDeliver(tStart, tStop);
				if (S_OK != hr) 
				{
					// stop if error, or if downstream filter said
					// to stop.
					return;
				}
			}

			if (Actual.cBuffers > 1) 
			{
				hr = CollectAndDeliver(tStart, tStop);
				if (FAILED(hr)) 
				{
					return;
				}
			}
		}
    } 
	else 
	{
		// sync version of above loop
		while (tCurrent < tAlignStop)
		{
			// Break out without calling EndOfStream if we're asked to
			// do something different
			if (CheckRequest(&dwRequest))
			{
				return;
			}

			IMediaSample* pSample;

			hr = m_pAlloc->GetBuffer(&pSample, NULL, NULL, 0);
			if (FAILED(hr))
			{
				OnError(hr);
				return;
			}

			LONGLONG tStopThis = tCurrent + (pSample->GetSize() * UNITS);
			if (tStopThis > tAlignStop)
			{
				tStopThis = tAlignStop;
			}
			pSample->SetTime(&tCurrent, &tStopThis);
			tCurrent = tStopThis;

			if (bDiscontinuity) 
			{
				pSample->SetDiscontinuity(TRUE);
				bDiscontinuity = FALSE;
			}

			hr = m_pReader->SyncReadAligned(pSample);

			if (FAILED(hr)) 
			{
				pSample->Release();
				OnError(hr);
				return;
			}

			hr = DeliverSample(pSample, tStart, tStop);
			if (hr != S_OK)
			{
				if (FAILED(hr)) 
					OnError(hr);
				return;
			}
		}
    }

    EndOfStream();
}

void CGraphConnectorInputPinAsync::CleanupCancelled(void)
{
    while (1) 
	{
		IMediaSample * pSample;
		DWORD_PTR dwUnused;

		HRESULT hr = m_pReader->WaitForNext(
					0,          // no wait
					&pSample,
					&dwUnused);
		if(pSample) 
			pSample->Release();
		else 
		{
			return;// no more samples
		}
    }
}

HRESULT CGraphConnectorInputPinAsync::CheckMediaType(const CMediaType *mt)
{
    CAutoLock lock(m_pReceiveLock);
	if( m_pConnector )
		return m_pConnector->CheckMediaType( mt );
    return S_OK;
}

HRESULT CGraphConnectorInputPinAsync::SetMediaType(const CMediaType *mt)
{
    CAutoLock lock(m_pReceiveLock);
	if( m_pConnector )
		return m_pConnector->SetMediaType( mt );
	return S_OK;
}

STDMETHODIMP CGraphConnectorInputPinAsync::ReceiveCanBlock()
{
    return S_FALSE;
}


STDMETHODIMP CGraphConnectorInputPinAsync::Receive(IMediaSample *pSample)
{
    CheckPointer(pSample,E_POINTER);
	//!! do not lock it here or allocator will block fetch thread
//    CAutoLock lock(m_pReceiveLock);
	if( m_pConnector )
		return m_pConnector->Receive( pSample );
	return S_FALSE;
}

HRESULT CGraphConnectorInputPinAsync::CheckConnect(IPin * pPin)
{
    HRESULT hr = CBasePin::CheckConnect(pPin);
    if (FAILED(hr)) 
	{
		return hr;
    }

    // get an input pin and an allocator interface
	IAsyncReader*       m_pInputPin;
    hr = pPin->QueryInterface( IID_IAsyncReader, (void **) &m_pInputPin);
    if (FAILED(hr))
	{
        return hr;
    }
	m_pInputPin->Release();

    return NOERROR;
}

HRESULT CGraphConnectorInputPinAsync::Length( LONGLONG* pTotal, LONGLONG* pAvailable )
{
	if( m_pReader )
		return m_pReader->Length( pTotal, pAvailable );

	*pTotal = 0;
	*pAvailable = 0;
    return S_FALSE;
}

HRESULT CGraphConnectorInputPinAsync::SyncRead( LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	if( m_pReader )
		return m_pReader->SyncRead( llPosition, lLength, pBuffer );

	return E_NOT_VALID_STATE;
}