#ifndef _GRAPH_CONNECTOR_INPUT_H_
#define _GRAPH_CONNECTOR_INPUT_H_

/*
Input part of the graph connector object
- will accept any type of media until initialized
- will pass incomming media packet to a a buffer ( considered render surface )
*/
class CGraphConnector;

////////////////////////////////////////////////////////////////////////
// Filter 
////////////////////////////////////////////////////////////////////////

//filter we will use to connect to graph1
//will not care about media type. Will accept anything and will try to push any received data into Connector
class CGraphConnectorInputFilter : public CBaseFilter
{
public:
    CGraphConnectorInputFilter(CGraphConnector *pConnector, LPUNKNOWN pUnk, CCritSec *pLock, HRESULT *phr);
	//if you are using these then you are doing it wrong !
    CGraphConnectorInputFilter(LPUNKNOWN pUnk, HRESULT *phr);
    ~CGraphConnectorInputFilter();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr); 
	
    // Pin enumeration
    CBasePin			*GetPin(int n);
    int					GetPinCount();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

//private:
    CGraphConnector		*m_pConnector;
    CCritSec			m_Lock;		
    CPosPassThru* m_pPosition; // position controls
};

////////////////////////////////////////////////////////////////////////
// Push model input pin 
////////////////////////////////////////////////////////////////////////

class CGraphConnectorInputPin : public CRenderedInputPin
{
    CGraphConnector     *const m_pConnector;       // Main renderer object
    CCritSec			*const m_pReceiveLock;		// Sample critical section
public:

    CGraphConnectorInputPin(CGraphConnector *pConnector,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr);

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP ReceiveCanBlock();

    // Check if the pin can support this specific proposed type and format
    HRESULT		CheckMediaType(const CMediaType *mt);
	HRESULT		SetMediaType(const CMediaType *mt);
    HRESULT		CompleteConnect(IPin *pReceivePin);
};

////////////////////////////////////////////////////////////////////////
// Pull model input pin 
////////////////////////////////////////////////////////////////////////

class CGraphConnectorInputPinAsync : public CAMThread, public CBaseInputPin
{
    IAsyncReader*       m_pReader;
    REFERENCE_TIME      m_tStart;
    REFERENCE_TIME      m_tStop;
    REFERENCE_TIME      m_tDuration;
    BOOL                m_bSync;
    CGraphConnector     *const m_pConnector;       // Main renderer object
    CCritSec			*const m_pReceiveLock;		// Sample critical section

    enum ThreadMsg 
	{
		TM_Pause,       // stop pulling and wait for next message
		TM_Start,       // start pulling
		TM_Exit,        // stop and exit
    };

    ThreadMsg m_State;

    // override pure thread proc from CAMThread
    DWORD ThreadProc(void);

    // running pull method (check m_bSync)
    void Process(void);

    // clean up any cancelled i/o after a flush
    void CleanupCancelled(void);

    // suspend thread from pulling, eg during seek
    HRESULT PauseThread();

    // start thread pulling - create thread if necy
    HRESULT StartThread();

    // stop and close thread
    HRESULT StopThread();

    // called from ProcessAsync to queue and collect requests
    HRESULT QueueSample( __inout REFERENCE_TIME& tCurrent, REFERENCE_TIME tAlignStop, BOOL bDiscontinuity);

    HRESULT CollectAndDeliver( REFERENCE_TIME tStart, REFERENCE_TIME tStop );

    HRESULT DeliverSample( IMediaSample* pSample, REFERENCE_TIME tStart, REFERENCE_TIME tStop);

protected:
    IMemAllocator *     m_pAlloc;

public:
    CGraphConnectorInputPinAsync(CGraphConnector *pConnector,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr);
    virtual ~CGraphConnectorInputPinAsync();

    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP ReceiveCanBlock();

    // Check if the pin can support this specific proposed type and format
    HRESULT		CheckMediaType(const CMediaType *mt);
	HRESULT		SetMediaType(const CMediaType *mt);
	HRESULT		CheckConnect(IPin *);
    HRESULT		CompleteConnect(IPin *pReceivePin);

    // disconnect any connection made in Connect
    STDMETHODIMP Disconnect();

    virtual HRESULT DecideAllocator( IMemAllocator* pAlloc, __inout_opt ALLOCATOR_PROPERTIES * pProps);

    STDMETHODIMP GetAllocator(__deref_out IMemAllocator ** ppAllocator);
    STDMETHODIMP SetAllocator(__deref_in IMemAllocator *pAllocator);

    // set start and stop position. if active, will start immediately at
    // the new position. Default is 0 to duration
    HRESULT Seek(REFERENCE_TIME tStart, REFERENCE_TIME tStop);

    // return the total duration
    HRESULT Duration(__out REFERENCE_TIME* ptDuration);

    // start pulling data
    HRESULT Active(void);

    // stop pulling data
    HRESULT Inactive(void);

    // helper functions
    LONGLONG AlignDown(LONGLONG ll, LONG lAlign)
	{
		// aligning downwards is just truncation
		return ll & ~(lAlign-1);
    };

    LONGLONG AlignUp(LONGLONG ll, LONG lAlign) 
	{
		// align up: round up to next boundary
		return (ll + (lAlign -1)) & ~(lAlign -1);
    };

    // GetReader returns the (addrefed) IAsyncReader interface
    // for SyncRead etc
    IAsyncReader* GetReader() 
	{
		m_pReader->AddRef();
		return m_pReader;
    };

    // override this to handle end-of-stream
	virtual STDMETHODIMP EndOfStream(void){ return S_OK; };

	virtual void OnError(HRESULT hr){};
    // flush this pin and all downstream
	virtual STDMETHODIMP BeginFlush(){ return S_OK; };
	virtual STDMETHODIMP EndFlush(){ return S_OK; };

	HRESULT STDMETHODCALLTYPE Length( LONGLONG* pTotal, LONGLONG* pAvailable );
	HRESULT STDMETHODCALLTYPE SyncRead( LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
};

#endif