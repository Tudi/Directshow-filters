#include "StdAfx.h"
#include "VidiatorFilterGuids.h"
#include <wmsdk.h>
#include "utils.h"

DEFINE_GUID(CLSID_Dump,
0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

void TestPIN( )
{
	HRESULT					hr;
	printf("Starting INPUT and OUTPUT filter PIN test\n");
	printf("We will use a video file as input and rescale it to half and upsample to double\n");
	printf("Rescale filter output will be dumped to 'dump' filter \n");
	printf("Resample filter output will be dumped to 'dump' filter \n");
	//////////////////////////////
	//create a file reader filter ( order 1 )
	//////////////////////////////
	IBaseFilter						*pSourceFilter;
	IFileSourceFilter				*pSourceControl;
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_WMSourceReader, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pSourceFilter ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 WMSourceReader_d.dll ?" );
		return;
	}
	if(FAILED( hr =pSourceFilter->QueryInterface( __uuidof(IFileSourceFilter), (void**)&pSourceControl ) ))
	{
		printf( "Failed to get reader" );
		return;
	}
	//////////////////////////////
	//create audio resampler filter ( order 2 )
	//////////////////////////////
	IBaseFilter						*pResamplerFilter;
	IVidiatorAudioResample			*pResamplerControl;
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_VidiatorAudioPreProcessor, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pResamplerFilter ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 VidiatorAudioPreProcessor_d.dll ?" );
		return;
	}
	if(FAILED( hr =pResamplerFilter->QueryInterface( __uuidof(IVidiatorAudioResample), (void**)&pResamplerControl ) ))
	{
		printf( "Failed to get PreProcessor" );
		return;
	}
	//set our filter type
	hr = pResamplerControl->SetFilterType( AUDIOTrans_Resample );
	if( hr != S_OK )
		printf("Could not set filter type \n");
	//////////////////////////////
	//create video resize filter ( order 2 )
	//////////////////////////////
	IBaseFilter				*pResizeFilter;
	IVidiatorImageResize	*pResizeControl;
	if(FAILED( hr = CoCreateInstance(CLSID_VidiatorImagePreProcessor, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pResizeFilter ) ))
	{
		printf( "Failed to get PreProcessor.Did you use regsvr32 VidiatorImagePreProcessor_d.dll ?" );
	}
	if(FAILED( hr =pResizeFilter->QueryInterface( __uuidof(IVidiatorImageResize), (void**)&pResizeControl ) ))
	{
		printf( "Failed to get PreProcessor" );
	}
	//set our filter type
	hr = pResizeControl->SetFilterType( VIDEOTrans_Resize );
	if( hr != S_OK )
		printf("Could not set filter type \n");

	//////////////////////////////
	//create a dump filter ( order 3 )
	//////////////////////////////
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
	//get the resizer filter pins 
	//////////////////////////////
	IPin *InPINVidResize;
	IPin *OutPINVidResize;
	hr = GetPin( ((IBaseFilter*)pResizeFilter), PINDIR_INPUT, &InPINVidResize, L"Video" );
	if( hr != S_OK )
		printf("Could not fetch resizer video input PIN\n");
	hr = GetPin( ((IBaseFilter*)pResizeFilter), PINDIR_OUTPUT, &OutPINVidResize, L"Video" );
	if( hr != S_OK )
		printf("Could not fetch resizer video output PIN\n");
	//////////////////////////////
	//get the resampler filter pins 
	//////////////////////////////
	IPin *InPINAudResampler;
	IPin *OutPINAudResampler;
	hr = GetPin( ((IBaseFilter*)pResamplerFilter), PINDIR_INPUT, &InPINAudResampler, L"Audio" );
	if( hr != S_OK )
		printf("Could not fetch resampler audio input PIN\n");
	hr = GetPin( ((IBaseFilter*)pResamplerFilter), PINDIR_OUTPUT, &OutPINAudResampler, L"Audio" );
	if( hr != S_OK )
		printf("Could not fetch resampler audio output PIN\n"); 
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
	//set media type for the resizer
	//////////////////////////////
	{
		CMediaType MTResizerVidIn;
		hr = ((CBasePin*)OutPINVidReader)->GetMediaType( 0, &MTResizerVidIn );
		if( hr != S_OK )
			printf("Could not fetch vid output media type\n");
		//changed the Interface since last time
		hr = pResizeControl->SetImageOutSize( ((VIDEOINFOHEADER*)MTResizerVidIn.pbFormat)->bmiHeader.biWidth * 2, ((VIDEOINFOHEADER*)MTResizerVidIn.pbFormat)->bmiHeader.biHeight * 2, false, 30 );
		if( hr != S_OK )
			printf("Could not set output details \n");
	}
	//////////////////////////////
	//set media type for the resampler
	//////////////////////////////
	{
		CMediaType MTResamplerAudIn;
		hr = ((CBasePin*)OutPINAudReader)->GetMediaType( 0, &MTResamplerAudIn );
		if( hr != S_OK )
			printf("Could not fetch audio output media type\n");
		hr = pResamplerControl->SetSampleRate( ((WAVEFORMATEX*)MTResamplerAudIn.pbFormat )->nChannels, ((WAVEFORMATEX*)MTResamplerAudIn.pbFormat )->nSamplesPerSec / 2 );
		if( hr != S_OK )
			printf("Could not set filter params \n");
	}
	//////////////////////////////
	//connect PINs
	//////////////////////////////
	//file reader vid out to resizer in
	hr = OutPINVidReader->Connect( InPINVidResize, 0 );
	if( hr != S_OK )
		printf("Could not connect video reader output to video resize in PIN\n");
	//connect resizer out to dumper
	hr = OutPINVidResize->Connect( InPINVidDump, 0 );
	if( hr != S_OK )
		printf("Could not connect video resize output to video dump input PIN\n");
	//reader audio out to resampler audio in
	hr = OutPINAudReader->Connect( InPINAudResampler, 0 );
	if( hr != S_OK )
		printf("Could not connect audio reader output to audio ressampler in PIN\n");
	//connect resampler out to audio dumper
	hr = OutPINAudResampler->Connect( InPINAudDump, 0 );
	if( hr != S_OK )
		printf("Could not connect resampler audio output to audio dumper in PIN\n");
	//////////////////////////////
	//start everything
	//////////////////////////////
	hr = pdumpFilterVid->Run( 0 );
	if( hr != S_OK )
		printf("Could not start video dumper\n");
	hr = pdumpFilterAud->Run( 0 );
	if( hr != S_OK )
		printf("Could not start audio dumper\n");
	hr = ((IBaseFilter*)pResizeFilter)->Run( 0 );
	if( hr != S_OK )
		printf("Could not start resizer\n");
	hr = ((IBaseFilter*)pResamplerFilter)->Run( 0 );
	if( hr != S_OK )
		printf("Could not start resampler\n");
	hr = pSourceFilter->Run( 0 );
	if( hr != S_OK )
		printf("Failed to start reading\n");

	//////////////////////////////
	//Let the filters run for a while. This is quite wrong. We should monitor output and decide how much to run it :(
	//////////////////////////////
	Sleep( 500 );

	//////////////////////////////
	//stop everything : file reader, resizer, dumpers
	//////////////////////////////
	hr = pSourceFilter->Stop();
	if( hr != S_OK )
		printf("Could not stop reader\n");
	hr = ((IBaseFilter*)pResizeFilter)->Stop();
	if( hr != S_OK )
		printf("Could not stop resizer\n");
	hr = ((IBaseFilter*)pResamplerFilter)->Stop();
	if( hr != S_OK )
		printf("Could not stop resampler\n");
	hr = pdumpFilterVid->Stop();
	if( hr != S_OK )
		printf("Could not stop video dumper\n");
	hr = pdumpFilterAud->Stop();
	if( hr != S_OK )
		printf("Could not stop audio dumper\n");
	//////////////////////////////
	//disconnect pins to remove cross references
	//////////////////////////////
	hr = InPINAudResampler->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");
	hr = OutPINAudResampler->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");
	hr = OutPINVidReader->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = OutPINAudReader->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");
	hr = InPINVidResize->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = OutPINVidResize->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = InPINVidDump->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect video pin.\n");
	hr = InPINAudDump->Disconnect();
	if( hr != S_OK )
		printf("Could not disconnect audio pin.\n");

	if( pSourceFilter )
		pSourceFilter->Release();
	if( pSourceControl )
		pSourceControl->Release();
	if( pResamplerFilter )
		pResamplerFilter->Release();
	if( pResizeFilter )
		pResizeFilter->Release();
	if( pResizeControl )
		pResizeControl->Release();
	if( pResamplerControl )
		pResamplerControl->Release();
	InPINVidDump->Release();
	InPINAudDump->Release();
	InPINAudResampler->Release();
	OutPINAudResampler->Release();
	InPINVidResize->Release();
	OutPINVidResize->Release();
	pdumpFilterVid->Release();
	pdumpFilterAud->Release();
	OutPINVidReader->Release();
	OutPINAudReader->Release();
	printf("Done testing INPUT/OUTPUT filter PINs\n");
	printf("=============================================================\n");
}