#include "StdAfx.h"
//#include "../WMSourceReader/WMSourceReaderFilter.h"
#include "../WMSourceReader/defines.h"

#ifdef SUPPORT_CALLBACKS
class Ccallback: public ISampleCallback
{
public:
	int OnSample(int nTrackIndex, LONGLONG llTimeStamp, unsigned char *pData, int nDataLen, int nMediaType)
	{
		printf("Sample callback received params track=%d,Stamp=%d,BuffLen=%d,Type=%d\n",nTrackIndex,(int)llTimeStamp,nDataLen,nMediaType);
		return 1;
	}
};

void TestCallback( IVidiatorWMSourceReader *pControl, IFileSourceFilter *pFilter, TCHAR *FileName )
{
	printf("Note : This test presumes we enabled callback support for our filters\n");
	HRESULT		hr;
	Ccallback	MyCallback;
	hr = pControl->SetSampleCallback( (BYTE*)&MyCallback );
	hr = pControl->Open( FileName );
	if( hr != S_OK )
	{
		printf("Failed to open input file. Exiting test\n");
		return;
	}

	hr = pControl->Start();
	if( hr != S_OK )
		printf("Failed to start reading\n");
	Sleep( 500 );

	((IBaseFilter*)pFilter)->Stop();

	hr = pControl->Close( );
	if( hr != S_OK )
		printf("Close Filter.\n");
	printf("Done testing Source filter with callbacks\n");
	printf("=============================================================\n");
}
#else
void TestCallback( IVidiatorWMSourceReader *pControl, IFileSourceFilter *pFilter, TCHAR *FileName )
{
	printf("Callback support is disabled right now\n");
}
#endif