#include "StdAfx.h"
#include "utils.h"

void main()
{
	HRESULT					hr;
	IBaseFilter				* pPreProcFilter;
	IVidiatorImageResize	* pPreProcControl;
	IPin					* pInPin_Pre = NULL;
	IPin					* pOutPin_Pre = NULL;
	CMediaType				* pMTIn = new CMediaType;
	CMediaType				* pMTOut = new CMediaType;
	VIDEOINFOHEADER			* VIHIn = (VIDEOINFOHEADER*)pMTIn->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	VIDEOINFOHEADER			* VIHOut = (VIDEOINFOHEADER*)pMTOut->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	VidiMedia				* pIn = new VidiMedia;
	VidiMedia				* pOut = new VidiMedia;

	//////////////////////////////////////////////////////
	//init
	//////////////////////////////////////////////////////
	CoInitialize( NULL );
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_VidiatorImagePreProcessor, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pPreProcFilter ) ))
	{
		printf( "Failed to get PreProcessor.Did you use regsvr32 VidiatorImagePreProcessor_d.dll ?" );
	}
	if(FAILED( hr =pPreProcFilter->QueryInterface( __uuidof(IVidiatorImageResize), (void**)&pPreProcControl ) ))
	{
		printf( "Failed to get PreProcessor" );
	}

	// add input / output pins to it. This will auto add reference
	if(FAILED( hr = GetPin(pPreProcFilter, PINDIR_INPUT, &pInPin_Pre, NULL) ))
	{
		printf( (("Failed to get input Pin of PreProcessor")) );
	}
	//we are using this PIN. Do not delete it in other thread. This will auto add reference
	if(FAILED( hr =GetPin(pPreProcFilter, PINDIR_OUTPUT, &pOutPin_Pre,NULL) ))
	{
		printf( (("Failed to get output Pin of PreProcessor")) );
	}
//	pMTIn->InitMediaType();
	pMTIn->majortype = MEDIATYPE_Video;
	pMTIn->formattype = FORMAT_VideoInfo;
	pMTIn->subtype = MEDIASUBTYPE_IYUV;
	memset( VIHIn, 0, sizeof(VIDEOINFOHEADER) );
	pMTIn->pbFormat = (BYTE*)VIHIn;
	pMTIn->cbFormat = 0;	//do not auto deallocate mem
	VIHIn->bmiHeader.biSize = sizeof( VIHIn->bmiHeader );
	VIHIn->bmiHeader.biBitCount = 24;
	VIHIn->bmiHeader.biWidth = IMG_IN_X;
	VIHIn->bmiHeader.biHeight = IMG_IN_Y;
	VIHIn->bmiHeader.biPlanes = 1;
	VIHIn->bmiHeader.biSizeImage = IMG_IN_X * IMG_IN_Y * 3 / 2;

//	pMTOut->InitMediaType();
	pMTOut->majortype = MEDIATYPE_Video;
	pMTOut->formattype = FORMAT_VideoInfo;
	pMTOut->subtype = MEDIASUBTYPE_IYUV;
	memset( VIHOut, 0, sizeof(VIDEOINFOHEADER) );
	pMTOut->pbFormat = (BYTE*)VIHIn;
	pMTOut->cbFormat = 0;	//do not auto deallocate mem
	VIHOut->bmiHeader.biSize = sizeof( VIHOut->bmiHeader );
	VIHOut->bmiHeader.biBitCount = 12;
	VIHOut->bmiHeader.biWidth = IMG_IN_X;
	VIHOut->bmiHeader.biHeight = IMG_IN_Y;
	VIHOut->bmiHeader.biPlanes = 1;
	VIHOut->bmiHeader.biSizeImage = IMG_IN_X * IMG_IN_Y * 3 / 2;

	//set our filter type
	hr = pPreProcControl->SetFilterType( VIDEOTrans_Null );
	if( hr != S_OK )
		printf("Could not set filter type \n");

	//init the filter with media types we will be using
	hr = pInPin_Pre->QueryAccept( pMTIn );
	if( hr != S_OK )
		printf("Did not set up IN pin details correctly \n");
	hr = pOutPin_Pre->QueryAccept( pMTOut );
	if( hr != S_OK )
		printf("Did not set up OUT pin details correctly \n");

	hr = ((CTransformInputPin*)pInPin_Pre)->SetMediaType( pMTIn );
	if( hr != S_OK )
		printf("Could not set input pin media type \n");
	((CTransformOutputPin*)pOutPin_Pre)->SetMediaType( pMTOut );
	if( hr != S_OK )
		printf("Could not set output pin media type \n");
	hr = pPreProcControl->SetFilterType( VIDEOTrans_Crop );
	if( hr != S_OK )
		printf("Could not set filter type \n");
	hr = ((CTransformFilter*)pPreProcFilter)->SetMediaType( PINDIR_INPUT, pMTIn );
	if( hr != S_OK )
		printf("Could not set filter input type \n");
	hr = ((CTransformFilter*)pPreProcFilter)->SetMediaType( PINDIR_OUTPUT, pMTOut );
	if( hr != S_OK )
		printf("Could not set filter output type \n");
	hr = pPreProcControl->SetImageOutSize( IMG_OUT_X, IMG_OUT_Y, false, 30 );
	if( hr != S_OK )
		printf("Could not set output resolution for resize filter \n");

	pIn->SetMediaType( pMTIn );
	hr = pIn->SetSize( VIHIn->bmiHeader.biSizeImage );
	if( hr != S_OK )
		printf("Could not resize input buffer. More then 1 object is using it \n");
	pOut->SetMediaType( pMTOut );
	hr = pOut->SetSize( VIHOut->bmiHeader.biSizeImage );
	if( hr != S_OK )
		printf("Could not resize output buffer. More then 1 object is using it \n");

	//////////////////////////////////////////////////////
	//do some tests
	//////////////////////////////////////////////////////
	TestCrop( pPreProcControl, ((CTransformFilter*)pPreProcFilter), pMTIn, pMTOut, pIn, pOut );
	TestResize( pPreProcControl, ((CTransformFilter*)pPreProcFilter), pMTIn, pMTOut, pIn, pOut );
	TestGreenEnh( pPreProcControl, ((CTransformFilter*)pPreProcFilter), pMTIn, pMTOut, pIn, pOut );
	TestDeinterlace( pPreProcControl, ((CTransformFilter*)pPreProcFilter), pMTIn, pMTOut, pIn, pOut );
	TestDeNoise( pPreProcControl, ((CTransformFilter*)pPreProcFilter), pMTIn, pMTOut, pIn, pOut );
	TestColorConv( pPreProcControl, ((CTransformFilter*)pPreProcFilter), pMTIn, pMTOut, pIn, pOut );
	// create reader, resampler, resizer and dumper filters. Chain them, run them, stop them
	TestPIN( );


	//////////////////////////////////////////////////////
	//release
	//////////////////////////////////////////////////////
	pOutPin_Pre->Release();
	pInPin_Pre->Release();
	if( pPreProcFilter )
		pPreProcFilter->Release();
	if( pPreProcControl )
		pPreProcControl->Release();
	delete pMTIn;
	delete pMTOut;
	pIn->Release();
	pOut->Release();
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