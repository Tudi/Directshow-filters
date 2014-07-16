#include "StdAfx.h"
#include "VidiGraphConnector.h"

HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin, WCHAR *pPinName)
{
	IEnumPins  *pEnum = NULL;
	IPin       *pPin = NULL;
	HRESULT    hr;

	if (ppPin == NULL)
	{
		return E_POINTER;
	}

	hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	PIN_INFO sInfo;
	while(pEnum->Next(1, &pPin, 0) == S_OK)
	{
		PIN_DIRECTION PinDirThis;
		hr = pPin->QueryDirection(&PinDirThis);
		if (FAILED(hr))
		{
			pPin->Release();
			pEnum->Release();
			return hr;
		}
		hr = pPin->QueryPinInfo(&sInfo);
		if (FAILED(hr))
		{
			pPin->Release();
			pEnum->Release();
			return hr;
		}

		sInfo.pFilter->Release();

		if (PinDir == PinDirThis) 
		{
			// Found a match. Return the IPin pointer to the caller.
			if(pPinName== NULL || _tcscmp(sInfo.achName, pPinName) ==0)
			{
				*ppPin = pPin;
				pEnum->Release();
				return S_OK;
			}
		}
		// Release the pin for the next time through the loop.
		pPin->Release();
	}
	// No more pins. We did not find a match.
	pEnum->Release();
	return E_FAIL;  
}

void main()
{
	HRESULT	hr;
	HRESULT hRes = ::CoInitialize(NULL);
	IBaseFilter						*pSourceFilter;
	IFileSourceFilter				*pSourceControl;
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_WMSourceReader, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pSourceFilter ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 WMSourceReader_d.dll ?" );
		return;
	}
	if(FAILED( hr = pSourceFilter->QueryInterface( __uuidof(IFileSourceFilter), (void**)&pSourceControl ) ))
	{
		printf( "Failed to get reader" );
		return;
	}
	IBaseFilter						*pdumpFilterVid;
	IBaseFilter						*pdumpFilterAud;
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_Dump, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pdumpFilterVid ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 Dump.dll ?" );
		return;
	}
	if(FAILED( hr = CoCreateInstance(CLSID_Dump, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pdumpFilterAud ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 Dump.dll ?" );
		return;
	}

	//////////////////////////////
	//create a bidge between video reader and video dumper : this will allow us to deconnect / stop one filter ( example reader ) while dumper will continue working
	//////////////////////////////
	VidiGraphConnector *pConnector = new VidiGraphConnector;

	//////////////////////////////
	//open file to get the video / audio propertiese
	//////////////////////////////
	hr = pSourceControl->Load( L"d:/film/asf/g1.asf", NULL );
	if( hr != S_OK )
	{
		printf("Failed to open input file. Exiting test\n");
		return;
	}
	//////////////////////////////
	//get the source filter pins 
	//////////////////////////////
	IPin *OutPINVidReader;
	IPin *OutPINAudReader;
	hr = GetPin( ((IBaseFilter*)pSourceFilter), PINDIR_OUTPUT, &OutPINVidReader, L"Video" );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");
	hr = GetPin( ((IBaseFilter*)pSourceFilter), PINDIR_OUTPUT, &OutPINAudReader, L"Audio" );
	if( hr != S_OK )
		printf("Could not fetch audio output PIN\n");
	//////////////////////////////
	//get the connector filter pins 
	//////////////////////////////
	IBaseFilter *ConnectorInput;
	hr = pConnector->GetInputFilter( &ConnectorInput );
	if( hr != S_OK )
		printf("Could not fetch connector input Filter\n");
	IBaseFilter *ConnectorOutput;
	hr = pConnector->GetCreateOutputFilter( &ConnectorOutput );
	if( hr != S_OK )
		printf("Could not fetch connector output Filter\n");
	IPin *InPINVidConnector;
	hr = GetPin( ConnectorInput, PINDIR_INPUT, &InPINVidConnector, NULL );
	if( hr != S_OK )
		printf("Could not fetch connector input PIN\n");
	IPin *OutPINVidConnector;
	hr = GetPin( ConnectorOutput, PINDIR_OUTPUT, &OutPINVidConnector, NULL );
//	hr = pConnector->GetConnectorOutputPin( &OutPINVidConnector );
	if( hr != S_OK )
		printf("Could not fetch connector output PIN\n");

	//////////////////////////////
	//get dumper pins ( video is order 3, audio is order 3 )
	//////////////////////////////
	//get dumper pins
	IPin *InPINVidDump;
	IPin *InPINAudDump;
	hr = pdumpFilterVid->FindPin( L"Input", &InPINVidDump );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");
	hr = pdumpFilterAud->FindPin( L"Input", &InPINAudDump );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");

	//////////////////////////////
	//connect PINs
	//////////////////////////////
	hr = OutPINVidReader->Connect( InPINVidConnector, 0 );
	if( hr != S_OK )
		printf("Could not connect video reader output to video dump in PIN\n");
	hr = OutPINVidConnector->Connect( InPINVidDump, 0 );
	if( hr != S_OK )
		printf("Could not connect video reader output to video dump in PIN\n");
	hr = OutPINAudReader->Connect( InPINAudDump, 0 );
	if( hr != S_OK )
		printf("Could not connect audio reader output to audio dump in PIN\n");
	//////////////////////////////
	//start everything
	//////////////////////////////
	printf("1]Starting all filters\n");
	hr = pdumpFilterVid->Run( 0 );
	if( hr != S_OK )
		printf("Could not start video dumper\n");
	hr = pdumpFilterAud->Run( 0 );
	if( hr != S_OK )
		printf("Could not start audio dumper\n");
	hr = ConnectorInput->Run( 0 );
	if( hr != S_OK )
		printf("Could not start video connector input\n");
	hr = ConnectorOutput->Run( 0 );
	if( hr != S_OK )
		printf("Could not start video connector output\n");
	hr = pSourceFilter->Run( 0 );
	if( hr != S_OK )
		printf("Failed to start reading\n");

	//////////////////////////////
	//Let the filters run for a while. This is quite wrong. We should monitor output and decide how much to run it :(
	//////////////////////////////
	printf("1]Waiting a few seconds for filters to run\n");
	Sleep( 500 );

	//////////////////////////////
	//stop file reader, resizer, dumpers
	//////////////////////////////
	printf("1]Stopping File reader(input) + connector input\n");
	hr = pSourceFilter->Stop();
	if( hr != S_OK )
		printf("Could not stop reader\n");
	hr = ConnectorInput->Stop();
	if( hr != S_OK )
		printf("Could not stop video connector input\n");
	hr = OutPINVidReader->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = InPINVidConnector->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video connector input pin.\n");

	//////////////////////////////
	//wait until output can process all the input ( in case output has larger latency then input )
	//////////////////////////////
	printf("1]Waiting until connector queue is empty\n");
	while( pConnector->IsOuputQueueEmpty() == S_FALSE )
		Sleep( 500 );
	//////////////////////////////
	//stop dumpers
	//////////////////////////////
	printf("1]Stopping rest of the filters\n");
	hr = pdumpFilterVid->Stop();
	if( hr != S_OK )
		printf("Could not stop video dumper\n");
	hr = pdumpFilterAud->Stop();
	if( hr != S_OK )
		printf("Could not stop audio dumper\n");
	hr = ConnectorOutput->Stop();
	if( hr != S_OK )
		printf("Could not stop video connector output\n");

	//this most probably will do nothing
	pConnector->ClearQueueContent(); //dump the media samples in queue so we are able to stop reader

	//////////////////////////////
	//disconnect pins to remove cross references
	//////////////////////////////
	hr = OutPINAudReader->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");
	hr = OutPINVidConnector->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio connector output pin.\n");
	hr = InPINVidDump->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video dumper pin.\n");
	hr = InPINAudDump->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio dumper pin.\n");

	pSourceFilter->Release();
	pSourceControl->Release();
	pdumpFilterVid->Release();
	pdumpFilterAud->Release();
	InPINVidDump->Release();
	InPINAudDump->Release();
	OutPINVidReader->Release();
	OutPINAudReader->Release();
	InPINVidConnector->Release();
	OutPINVidConnector->Release();
	ConnectorInput->Release();
	ConnectorOutput->Release();

	delete pConnector;

	::CoUninitialize();

#if (defined(_CRTDBG_MAP_ALLOC) && defined(_DEBUG))
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
	int ret;
	ret=_CrtCheckMemory( );
	_tprintf(_T("_CrtCheckMemory::ret=%d \n"), ret);
	ret=_CrtDumpMemoryLeaks();
	_tprintf(_T("_CrtDumpMemoryLeaks::ret=%d \n"), ret);
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif 
}