#ifndef _GRAPH_CONNECTOR_OUTPUT_H_
#define _GRAPH_CONNECTOR_OUTPUT_H_

#define DEFAULT_THREAD_WAIT_SLEEP_MS 4 // latency will be added to packet delivery in case send pool is empty

/*
Output part of the graph connector object
- When someone connects to us, we will query the render surface ( media pool ) for intialized media type
- we have a thread that will continuesly poll the media pool and try to push to the filter connected to us
*/

class CGraphConnector;

////////////////////////////////////////////////////////////////////////
// Filter 
////////////////////////////////////////////////////////////////////////

//filter we will use to connect to graph1
//will not care about media type. Will accept anything and will try to push any received data into Connector
class CGraphConnectorOutputFilter : public CSource
{
public:
    CGraphConnectorOutputFilter(CGraphConnector *pConnector, LPUNKNOWN pUnk, CCritSec *pLock, HRESULT *phr, int ind);
	//if you are using these then you are doing it wrong !
    CGraphConnectorOutputFilter(LPUNKNOWN pUnk, HRESULT *phr);
	 static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr); 

    // Pin enumeration
    CBasePin			*GetPin(int n);
    int					GetPinCount();
//private:
    CGraphConnector		*m_pConnector;
	int					m_nConnectorInd;
};

////////////////////////////////////////////////////////////////////////
// Push model output PIN 
////////////////////////////////////////////////////////////////////////

class CGraphConnectorOutputPin: public CSourceStream
{
    CGraphConnector		*const m_pConnector;       // Main renderer object
    CCritSec			*const m_pReceiveLock;		// Sample critical section
	int					m_nConnectorInd;
public:
    CGraphConnectorOutputPin(CGraphConnector *pConnector,
                  LPUNKNOWN   pUnk,
                  CSource     *pFilter,
                  CCritSec    *pLock,
                  CCritSec    *pReceiveLock,
                  HRESULT     *phr,
				  int			ind
				  );
	~CGraphConnectorOutputPin(){};
protected:
	///////////////////////
	// Derived from CBaseOuputPIn
	HRESULT DecideBufferSize(IMemAllocator* pMemAlloc, ALLOCATOR_PROPERTIES* pProperties);

	///////////////////////
	// Derived from CSourceStream
	HRESULT GetMediaType( CMediaType *pMediaType );	
	HRESULT DoBufferProcessingLoop(void);
	//polling queue until data is available to be pushed
	HRESULT FillBuffer(IMediaSample* pSample){ return S_FALSE; }

	///////////////////////
	// Active & InActive
	HRESULT			Active(void);    // Starts up the worker thread
	HRESULT			Inactive(void);  // Exits the worker thread.

	HRESULT			SetMediaType( CMediaType *pMediaType );	

	BOOL			m_bFirstPacket;
	BOOL			m_bEndOfStream;
};

////////////////////////////////////////////////////////////////////////
// Pull model output Pin 
////////////////////////////////////////////////////////////////////////

// the filter class (defined below)
class CAsyncReader;


// the output pin class
class CGraphConnectorOutputPinAsync : public IAsyncReader, public CBasePin
{
    CGraphConnector		*const m_pConnector;       // Main renderer object
    CCritSec			*const m_pReceiveLock;		// Sample critical section
	int					m_nConnectorInd;
protected:
    IMemAllocator		*m_pAlloc;

public:
    // constructor and destructor
    CGraphConnectorOutputPinAsync(CGraphConnector *pConnector,
                  LPUNKNOWN   pUnk,
                  CSource     *pFilter,
                  CCritSec    *pLock,
                  CCritSec    *pReceiveLock,
                  HRESULT     *phr,
				  int			ind);

    ~CGraphConnectorOutputPinAsync();

//    STDMETHODIMP GetAllocator( __deref_out IMemAllocator ** ppAllocator );
    STDMETHODIMP SetAllocator( __deref_in IMemAllocator *pAllocator );

    // --- CUnknown ---
    // need to expose IAsyncReader
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    // --- IPin methods ---
//    STDMETHODIMP Connect( IPin * pReceivePin, const AM_MEDIA_TYPE *pmt );

    // --- CBasePin methods ---

    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT CheckMediaType(const CMediaType* pType);

    // --- IAsyncReader methods ---
    // pass in your preferred allocator and your preferred properties.
    // method returns the actual allocator to be used. Call GetProperties
    // on returned allocator to learn alignment and prefix etc chosen.
    // this allocator will be not be committed and decommitted by
    // the async reader, only by the consumer.
    STDMETHODIMP RequestAllocator( IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator ** ppActual);

    // queue a request for data.
    // media sample start and stop times contain the requested absolute
    // byte position (start inclusive, stop exclusive).
    // may fail if sample not obtained from agreed allocator.
    // may fail if start/stop position does not match agreed alignment.
    // samples allocated from source pin's allocator may fail
    // GetPointer until after returning from WaitForNext.
    STDMETHODIMP Request( IMediaSample* pSample, DWORD_PTR dwUser);      

    // block until the next sample is completed or the timeout occurs.
    // timeout (millisecs) may be 0 or INFINITE. Samples may not
    // be delivered in order. If there is a read error of any sort, a
    // notification will already have been sent by the source filter,
    // and STDMETHODIMP will be an error.
    STDMETHODIMP WaitForNext( DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR * pdwUser);     // user context

    // sync read of data. Sample passed in must have been acquired from
    // the agreed allocator. Start and stop position must be aligned.
    // equivalent to a Request/WaitForNext pair, but may avoid the
    // need for a thread on the source filter.
    STDMETHODIMP SyncReadAligned( IMediaSample* pSample );


    // sync read. works in stopped state as well as run state.
    // need not be aligned. Will fail if read is beyond actual total
    // length.
    STDMETHODIMP SyncRead( LONGLONG llPosition, LONG lLength, BYTE* pBuffer); 

    // return total length of stream, and currently available length.
    // reads for beyond the available length but within the total length will
    // normally succeed but may block for a long period.
    STDMETHODIMP Length( LONGLONG* pTotal, LONGLONG* pAvailable );

    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);

};

#endif