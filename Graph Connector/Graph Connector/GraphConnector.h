#ifndef _GRAPH_CONNECTOR_H_
#define _GRAPH_CONNECTOR_H_

#include <list>
#include "VidiatorFilterGuids.h"

//the memory allocator will block when he is out of buffers. This will happen when input is pushing too fast and output cannot consume them
// !! This needs to be large enough so that one thread will not block the other in the source filter. Ex : there are 4 audio packets while 1 video packet
#define CONNECTOR_BUFFERS_UNTIL_LOCK	3
//Can convert this to dynamic list later. For beta build will leave it fixed cause it is much easier to debug
#define CONNECTOR_MAX_OUTPUT_COUNT		4

//#define DEBUGGING_IN_TESTBENCH 

class CGraphConnectorInputFilter;
class CGraphConnectorInputPin;
class CGraphConnectorInputPinAsync;
class CGraphConnectorOutputFilter;
class CGraphConnectorOutputPin;
class CGraphConnectorOutputPinAsync;

enum GrpahConnectorAllocatorType
{
	GCAT_INVALID			= 0,
	GCAT_USE_FROM_INPUT		= 1, //might block input deallocation until output uses up all the buffers. Passthrough mode
	GCAT_USE_INTERNAL		= 2, //costs 1 extra memcopy. Blocks after internal buffer pool exhaust. 
	GCAT_TOTAL
};

// Be able to return a filter we can use to connect to filter graph1
// Input is able to receive data and will give it to CGraphConnector, which will cache it
// Be able to return a filter we can use to connect to filter graph2
// Output is able to read data from CGraphConnector and continuesly push it to connected graphs
class CGraphConnector : public CUnknown, public IVidiatorGraphConnector
{
    CGraphConnectorInputFilter   *m_pInputFilter;	
	CBaseInputPin				 *m_pInputPin;
    CGraphConnectorInputPin		 *m_pInputPinForPush;		
    CGraphConnectorInputPinAsync *m_pInputPinForPull;		

	//right now we have only 1 output. Maybe later implement more then 1 output
    CGraphConnectorOutputFilter   *m_pOutputFilter[CONNECTOR_MAX_OUTPUT_COUNT];	
    CBasePin					  *m_pOutputPin[CONNECTOR_MAX_OUTPUT_COUNT];	
    CGraphConnectorOutputPin	  *m_pOutputPinForPush[CONNECTOR_MAX_OUTPUT_COUNT];	
    CGraphConnectorOutputPinAsync *m_pOutputPinForPull[CONNECTOR_MAX_OUTPUT_COUNT];	

    CCritSec					  m_Lock;			// Main renderer critical section

public:
    DECLARE_IUNKNOWN

    CGraphConnector(LPUNKNOWN pUnk, HRESULT *phr);
    ~CGraphConnector();

    static CUnknown * WINAPI	CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    // Overriden to say what interfaces we support where
    STDMETHODIMP				NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	//will return the filter we are using to dump data into "us"
    HRESULT STDMETHODCALLTYPE GetInputFilter( 
        /* [out] */ IBaseFilter **InputFilter);
	//create a new output connection(filter). All input will be queued for this output also
    HRESULT STDMETHODCALLTYPE GetCreateOutputFilter( IBaseFilter **OutputFilter );
	//will return the filter we are using to dump data into "us"
    HRESULT STDMETHODCALLTYPE GetOutputFilter( 
        /* [out] */ IBaseFilter **OutputFilter, int OutInd );
	HRESULT STDMETHODCALLTYPE GetConnectorInputPin( IPin **InputPin);
    HRESULT STDMETHODCALLTYPE GetConnectorOutputPin( IPin **OutputPin, int OutInd );
	HRESULT STDMETHODCALLTYPE ClearQueueContent();
	HRESULT STDMETHODCALLTYPE ClearQueueContent( int OutInd );
	HRESULT STDMETHODCALLTYPE AddOutput();
	HRESULT STDMETHODCALLTYPE IsOuputQueueEmpty();
	HRESULT STDMETHODCALLTYPE SetAllocatorType( int Type )
	{
		if( Type == GCAT_USE_FROM_INPUT || Type == GCAT_USE_INTERNAL )
		{
			m_nAllocatorType = (GrpahConnectorAllocatorType)Type;
			return S_OK;
		}
		return S_FALSE;
	}
	HRESULT STDMETHODCALLTYPE SetBehaviorType( int Type );
	HRESULT STDMETHODCALLTYPE SwitchToAsyncPullInputPin( );
	HRESULT STDMETHODCALLTYPE SwitchToAsyncReaderOutputPin( );
	//Async input related API calls for output PINs
	HRESULT STDMETHODCALLTYPE GetAsyncReadLength( LONGLONG* pTotal, LONGLONG* pAvailable );
	HRESULT STDMETHODCALLTYPE SyncRead( LONGLONG llPosition, LONG lLength, BYTE* pBuffer);

	//will be called by input pin on data receive. Right now we only put a ref on the sample and push it into our queue. This is a blocking method
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP FetchPacket(IMediaSample **pSample, int OutInd );
	CBasePin *GetPin(int n);
	HRESULT CheckMediaType(const CMediaType *);
	HRESULT SetMediaType(const CMediaType *);
	HRESULT GetMediaType(CMediaType *);
	HRESULT SetAllocatorProperties( int RequestedSize=0 ); //internal
	HRESULT GetAllocatorProperties( ALLOCATOR_PROPERTIES* pProperties );
	HRESULT GetAllocator( ALLOCATOR_PROPERTIES* pProperties );
	HRESULT IncreaseBufferSizeRuntime( int RequestedSize );	//this blocks until output is empty or timeout. !!! Has a chance to fail and desyncronize video / audio !!!
private:
	HRESULT CopySample(IMediaSample *pMSsrc, IMediaSample *pMSdst );

	CMediaType					m_mMediaType;
    CCritSec					m_QueueLock;        
	std::list< IMediaSample* >	m_pSampleQueue[CONNECTOR_MAX_OUTPUT_COUNT];
	GraphConnectorBehavior		m_Type;
	int							m_nNumOutputs;
	IMemAllocator				*m_pAllocator;
	BOOL						m_bBufferDecided;
	GrpahConnectorAllocatorType m_nAllocatorType;
	BOOL						m_bUseAsyncReaderOutput;
#ifdef DEBUGGING_IN_TESTBENCH
	int							PacketCounterOut;
	int							PacketCounterIn;
	FILE						*m_fdLogs;
#endif
};

#endif