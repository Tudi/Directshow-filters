#include "StdAfx.h"
#include "utils.h"
#include "..\ISOFormatDemux2\ISOFormatDemux2Filter.h"

DEFINE_GUID(CLSID_Dump,
0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

void main()
{
	HRESULT					hr;

	printf( "Testbench project : Test creation and connection of a file source filter to a dump filter\n");

	CoInitialize( NULL );

	IBaseFilter						*pSourceFilter;
	IFileSourceFilter				*pSourceControl;
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_ISOFormatDemux2, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pSourceFilter ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 ISOFormatDemux2.dll ?" );
		return;
	}
	if(FAILED( hr = pSourceFilter->QueryInterface( __uuidof(IFileSourceFilter), (void**)&pSourceControl ) ))
	{
		printf( "Failed to get reader 2" );
		return;
	}
//#define TEST_INTERFACE_CALLS
#ifdef TEST_INTERFACE_CALLS
	IISOFormatDemux2				*pSourceTrackControl;
	if(FAILED( hr = pSourceFilter->QueryInterface( __uuidof(IISOFormatDemux2), (void**)&pSourceTrackControl ) ))
	{
		printf( "Failed to get reader 3" );
		return;
	}
#endif

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
//	hr = pSourceControl->Load( L"d:/film/3GP/gonzo.3gp", NULL );
//	hr = pSourceControl->Load( L"d:/film/3GP/dance_QVGA_24fps_no_audio.3gp", NULL );
	hr = pSourceControl->Load( L"d:/film/3GP/MultiRate/MULTIRATE_MPEG-4_Simple_AAC-LC_Stereo.3gp", NULL );
//	hr = pSourceControl->Load( L"d:/film/mp4/HaroldAndKumar_trailer.mp4", NULL );
	if( hr != S_OK )
	{
		printf("Failed to open input file. Exiting test\n");
		return;
	}
#ifdef TEST_INTERFACE_CALLS
	S_VIDEO_INFO *VI = pSourceTrackControl->GetVideoInfo();
	S_AUDIO_INFO *AI = pSourceTrackControl->GetAudioInfo();
#endif
//#define TEMP_DISABLE_PIN_USE
#ifndef TEMP_DISABLE_PIN_USE
	//get the filter pins and try to connect them to our pins
	IPin *OutPINVid;
	IPin *OutPINAud;
	hr = GetPin( ((IBaseFilter*)pSourceFilter), PINDIR_OUTPUT, &OutPINVid, L"Video" );
	if( hr != S_OK )
		printf("Could not fetch video output PIN\n");
	hr = GetPin( ((IBaseFilter*)pSourceFilter), PINDIR_OUTPUT, &OutPINAud, L"Audio" );
	if( hr != S_OK )
		printf("Could not fetch audio output PIN\n");
//#define TEMP_DISABLE_FILTER_CONNECT
#ifndef TEMP_DISABLE_FILTER_CONNECT
	hr = OutPINVid->Connect( InPINVid, NULL );
	if( hr != S_OK )
		printf("Could not connect video output PIN\n");
	if( OutPINAud )
	{
		hr = OutPINAud->Connect( InPINAud, NULL );
		if( hr != S_OK )
			printf("Could not connect audio output PIN\n");
	}
//#define TEMP_DISABLE_FILTER_RUN
#ifndef TEMP_DISABLE_FILTER_RUN
	hr = pdumpFilterVid->Run( 0 );
	if( hr != S_OK )
		printf("Could not start video dumper\n");
	hr = pdumpFilterAud->Run( 0 );
	if( hr != S_OK )
		printf("Could not start audio dumper\n");

	hr = pSourceFilter->Run( 0 );
	if( hr != S_OK )
		printf("Failed to start reading\n");

	printf("\nRead Data\n");
	Sleep( 1000 );
//#define TEST_SEEK_INTERFACE
#ifdef TEST_SEEK_INTERFACE
	IMediaSeeking				*SeekControl;
	if(FAILED( hr = OutPINVid->QueryInterface( __uuidof(IMediaSeeking), (void**)&SeekControl ) ))
	{
		printf( "Failed to get seek control" );
		return;
	}
	LONGLONG start, end;
	SeekControl->GetAvailable( &start, &end );
	printf( "\nMovie duration %d - %d\n", (int)start, (int)end );
	LONGLONG JumpStart = ( end / 2 ) * 1000;
	LONGLONG JumpEnd = JumpStart + ( end * 10 / 100 ) * 1000;
	printf("\nSeeking to midle of the movie\n");
	SeekControl->SetPositions( &JumpStart, AM_SEEKING_AbsolutePositioning, &JumpEnd, AM_SEEKING_AbsolutePositioning );
	Sleep( 10 );
	printf("\nRewinding to the beggining of the file\n");
	JumpStart = 0;
	JumpEnd = end * 1000; 
	SeekControl->SetPositions( &JumpStart, AM_SEEKING_AbsolutePositioning, &JumpEnd, AM_SEEKING_AbsolutePositioning );
	printf("\nFinished Rewind\n");
	Sleep( 10 );
	SeekControl->Release();
	printf("\nRead Data\n");
	Sleep( 1000 );
#endif
//	Sleep( 60*1000 );

	hr = ((IBaseFilter*)pSourceFilter)->Stop();
	if( hr != S_OK )
		printf("Could not stop reader\n");
	hr = pdumpFilterVid->Stop();
	if( hr != S_OK )
		printf("Could not stop video dumper\n");
	hr = pdumpFilterAud->Stop();
	if( hr != S_OK )
		printf("Could not stop audio\n");
#endif
	hr = InPINVid->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = OutPINVid->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	if( OutPINAud )
	{
		hr = OutPINAud->Disconnect();
		if( hr != S_OK )
			printf("Could not disconnect audio pin.\n");
		hr = InPINAud->Disconnect();
		if( hr != S_OK )
			printf("Could not disconnect audio pin.\n");
	}
#endif
	InPINVid->Release();
	InPINAud->Release();
	if( OutPINVid )
		OutPINVid->Release();
	if( OutPINAud )
		OutPINAud->Release();
#endif
	pdumpFilterVid->Release();
	pdumpFilterAud->Release();
	pSourceControl->Release();
	pSourceFilter->Release();
#ifdef TEST_INTERFACE_CALLS
	pSourceTrackControl->Release();
#endif

	CoUninitialize();

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