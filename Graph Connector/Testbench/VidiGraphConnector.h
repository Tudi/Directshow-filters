#ifndef _VIDI_GRAPH_CONNECTOR_H_
#define _VIDI_GRAPH_CONNECTOR_H_

interface IVidiatorGraphConnector;

class VidiGraphConnector
{
public:
	VidiGraphConnector();
	~VidiGraphConnector();
    HRESULT GetInputFilter( IBaseFilter **InputFilter );
	//will return the filter we are using to dump data out
    HRESULT GetCreateOutputFilter( IBaseFilter **OutputFilter );
	//will return the filter we are using to dump data out
    HRESULT GetOutputFilter( IBaseFilter **OutputFilter, int OutInd);
	//we might want to run output more then the input. Query if any of the outputs still have pending data
	HRESULT IsOuputQueueEmpty();
	//required if we are using input source allocator. We will need to release the buffer locks to be able to stop other filters
	// real case scenario this is not required to be called 99% of the time. Our input buffer will be instantly processed and released
	HRESULT ClearQueueContent();

private:
	IVidiatorGraphConnector *m_pConnector;
};

#endif