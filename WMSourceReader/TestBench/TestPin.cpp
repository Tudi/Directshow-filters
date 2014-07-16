#include "StdAfx.h"
//#include "../WMSourceReader/WMSourceReaderFilter.h"
#include "../WMSourceReader/defines.h"
#include <conio.h>

void TestPins( IFileSourceFilter *pControl, IBaseFilter *pFilter, TCHAR *FileName )
{
	printf("Testing PIN support for source reader\n");
	HRESULT		hr;

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

	IPin *InPINVid;
	IPin *InPINAud;
	hr = pdumpFilterVid->FindPin( L"Input", &InPINVid );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");
	hr = pdumpFilterAud->FindPin( L"Input", &InPINAud );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");

	//open file
//	hr = pControl->Load( FileName, NULL );
	hr = pControl->Load( L"mms://media.sonoma.edu/pinball.wmv", NULL );
	if( hr != S_OK )
	{
		printf("Failed to open input file. Exiting test\n");
		return;
	}
	//get the filter pins and try to connect them to our pins
	IPin *OutPINVid;
	IPin *OutPINAud;
	hr = GetPin( ((IBaseFilter*)pFilter), PINDIR_OUTPUT, &OutPINVid, L"Video" );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");
	hr = GetPin( ((IBaseFilter*)pFilter), PINDIR_OUTPUT, &OutPINAud, L"Audio" );
	if( hr != S_OK )
		printf("Could not fetch audio output PIN\n");

	CMediaType MTVidout;
	CMediaType MTAudout;
	hr = ((CBasePin*)OutPINVid)->GetMediaType( 0, &MTVidout );
	if( hr != S_OK )
		printf("Could not fetch vid output media type\n");
	hr = ((CBasePin*)OutPINAud)->GetMediaType( 0, &MTAudout );
	if( hr != S_OK )
		printf("Could not fetch aud output media type\n");
	hr = ((CBasePin*)InPINVid)->SetMediaType( &MTVidout );
	if( hr != S_OK )
		printf("Could not set vid input media type\n");
	hr = ((CBasePin*)InPINAud)->SetMediaType( &MTAudout );
	if( hr != S_OK )
		printf("Could not set vid input media type\n");
	hr = OutPINVid->Connect( InPINVid, &MTVidout );
	if( hr != S_OK )
		printf("Could not connect video output PIN\n");
	hr = OutPINAud->Connect( InPINAud, &MTAudout );
	if( hr != S_OK )
		printf("Could not connect audio output PIN\n");

	hr = pdumpFilterVid->Run( 0 );
	if( hr != S_OK )
		printf("Could not start video dumper\n");
	hr = pdumpFilterAud->Run( 0 );
	if( hr != S_OK )
		printf("Could not start audio dumper\n");

	hr = pFilter->Run( 0 );
	if( hr != S_OK )
		printf("Failed to start reading\n");
	//Sleep( 500 );
	getch();

	hr = ((IBaseFilter*)pFilter)->Stop();
	if( hr != S_OK )
		printf("Could not stop reader\n");
	hr = pdumpFilterVid->Stop();
	if( hr != S_OK )
		printf("Could not stop video dumper\n");
	hr = pdumpFilterAud->Stop();
	if( hr != S_OK )
		printf("Could not stop audio\n");

//	hr = pControl->Close( );
//	if( hr != S_OK )
//		printf("Close Filter.\n");

	hr = InPINVid->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = InPINAud->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");
	hr = OutPINVid->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = OutPINAud->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");
	InPINVid->Release();
	InPINAud->Release();
	pdumpFilterVid->Release();
	pdumpFilterAud->Release();
	OutPINVid->Release();
	OutPINAud->Release();
	printf("Done testing Source filter with PINs\n");
	printf("=============================================================\n");
}
