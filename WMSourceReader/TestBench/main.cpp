#include "StdAfx.h"

void main()
{
	HRESULT							hr;
	IBaseFilter						*pSourceFilter;
	IVidiatorWMSourceReader			*pSourceControl;
	IFileSourceFilter				*pSourceControl2;

	//////////////////////////////////////////////////////
	//init
	//////////////////////////////////////////////////////
	CoInitialize( NULL );
	//create a filter
	if(FAILED( hr = CoCreateInstance(CLSID_WMSourceReader, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pSourceFilter ) ))
	{
		printf( "Failed to get reader.Did you use regsvr32 WMSourceReader_d.dll ?" );
		return;
	}
	if(FAILED( hr =pSourceFilter->QueryInterface( __uuidof(IVidiatorWMSourceReader), (void**)&pSourceControl ) ))
	{
		printf( "Failed to get reader" );
		return;
	}
	if(FAILED( hr =pSourceFilter->QueryInterface( __uuidof(IFileSourceFilter), (void**)&pSourceControl2 ) ))
	{
		printf( "Failed to get reader 2" );
		return;
	}

	//////////////////////////////////////////////////////
	//do some tests
	//////////////////////////////////////////////////////
	TestCallback( pSourceControl, ((IFileSourceFilter*)pSourceFilter), L"d:/film/asf/g1.asf" );
//	TestPins( pSourceControl2, ((IBaseFilter*)pSourceFilter), L"d:/film/asf/g1.asf" );
//	TestPins( pSourceControl2, ((IBaseFilter*)pSourceFilter), L"mms://livewm.orange.fr/live-multicanaux" );
	TestPins( pSourceControl2, ((IBaseFilter*)pSourceFilter), L"mms://media.sonoma.edu/pinball.wmv" );

	//////////////////////////////////////////////////////
	//release
	//////////////////////////////////////////////////////
	if( pSourceFilter )
		pSourceFilter->Release();
	if( pSourceControl )
		pSourceControl->Release();
	if( pSourceControl2 )
		pSourceControl2->Release();
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