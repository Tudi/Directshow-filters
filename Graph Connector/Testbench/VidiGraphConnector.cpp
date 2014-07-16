#include "StdAfx.h"
#include "VidiGraphConnector.h"
#include "VidiatorFilterGuids.h"

#ifdef _DEBUG
	#define Dlog(x) printf(x);
#else
	#define Dlog(x)
#endif

VidiGraphConnector::VidiGraphConnector()
{
	m_pConnector = NULL;
	//create a bidge
	HRESULT					hr;
	IUnknown				*pUnkConnector;
	IVidiatorGraphConnector *pConnector;
	if(FAILED( hr = CoCreateInstance(CLSID_GraphConnector, NULL, CLSCTX_INPROC, IID_IUnknown, (void**)&pUnkConnector ) ))
	{
		Dlog( "Failed to get reader.Did you use regsvr32 GraphConnector.dll ?" );
		return;
	}
	if(FAILED( hr = pUnkConnector->QueryInterface( __uuidof(IVidiatorGraphConnector), (void**)&pConnector ) ))
	{
		Dlog( "Failed to get proper connector version" );
		return;
	}
	m_pConnector = pConnector;
//	m_pConnector->SetAllocatorType( 1 ); //testing passthrough mode
	//we are not using this object anymore 
	pUnkConnector->Release();
}


VidiGraphConnector::~VidiGraphConnector()
{
	if( m_pConnector )
		m_pConnector->Release();
}

HRESULT VidiGraphConnector::GetInputFilter( IBaseFilter **InputFilter )
{
	return m_pConnector->GetInputFilter( InputFilter );
}
//will return the filter we are using to dump data out
HRESULT VidiGraphConnector::GetCreateOutputFilter( IBaseFilter **OutputFilter )
{
	return m_pConnector->GetCreateOutputFilter( OutputFilter );
}
//will return the filter we are using to dump data out
HRESULT VidiGraphConnector::GetOutputFilter( IBaseFilter **OutputFilter, int OutInd)
{
	return m_pConnector->GetOutputFilter( OutputFilter, OutInd );
}
//we might want to run output more then the input. Query if any of the outputs still have pending data
HRESULT VidiGraphConnector::IsOuputQueueEmpty()
{
	return m_pConnector->IsOuputQueueEmpty( );
}
//required if we are using input source allocator. We will need to release the buffer locks to be able to stop other filters
// real case scenario this is not required to be called 99% of the time. Our input buffer will be instantly processed and released
HRESULT VidiGraphConnector::ClearQueueContent()
{
	return m_pConnector->ClearQueueContent( );
}