#include "StdAfx.h"

#include "ResamplerLib/LinearResampler.cpp"

void TestResize( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut )
{
	HRESULT					hr;

	printf("Starting resize filter test\n");
	printf("Expected buffer format is IYUV ( planar 422 format )\n");
	float Step_x = 0.50f; // less then 1 = enlarge
	float Step_y = 0.50f; // greater then 1 = shrink
	int OutWidth = (int)( IMG_IN_X / Step_x );
	int OutHeight = (int)( IMG_IN_Y / Step_y );
	int InWidth = IMG_IN_X;
	int InHeight = IMG_IN_Y;
	printf("Test params are hardcoded for now %dx%d -> %dx%d\n",InWidth,InHeight,OutWidth,OutHeight );
	//set crop specific params
	//just in case other test initialized the filter the we stop it
	hr = pFilter->Stop();
	if( hr != S_OK )
		printf("Could not stop filter \n");
	//set our filter type
	hr = pControl->SetFilterType( VIDEOTrans_Resize );
	if( hr != S_OK )
		printf("Could not set filter type \n");

	//define output for the filter
	hr = pControl->SetImageOutSize( OutWidth, OutHeight, false, 30 );
	if( hr != S_OK )
		printf("Could not set output details \n");

	//we can start the filter again
	hr = pFilter->Run( 0 );
	if( hr != S_OK )
		printf("Could not start filter \n");

	//set buffer sizes for this test
	pIn->SetSize( InWidth * InHeight * 3 / 2 );
	pOut->SetSize( OutWidth * OutHeight * 3 / 2 );

	//are we really done with setup ?
	hr = pFilter->CheckTransform( pMTIn, pMTOut );
	if( hr != S_OK )
		printf("Filter refuses to use our in / out setup \n");

	//initialize our buffers so we may check later the output
	BYTE *pBuffIn;
	hr = pIn->GetPointer( &pBuffIn );
	BYTE *pBuffOut;
	hr = pOut->GetPointer( &pBuffOut );
	for( int i = 0; i < pIn->GetSize(); i++ )
		pBuffIn[i] = i;

	//do the actual transform
	int StartT = GetTickCount();
	for( int i=0;i<100;i++)
		hr = pFilter->Transform( pIn, pOut );
	if( hr != S_OK )
		printf("Input / Output transform failed \n");
	int EndT = GetTickCount();
	printf("Avg transform time per 100 frame %d ms = %d for default resize alg\n", EndT - StartT, (EndT - StartT)/100 );
	{
		int StartT = GetTickCount();
		for( int i=0;i<100;i++)
//			ResampleYUV420Liniar( pBuffIn, pBuffOut, InWidth, InHeight, OutWidth, OutHeight );
			hr = pFilter->Transform( pIn, pOut );
		if( hr != S_OK )
			printf("Input / Output transform failed \n");
		int EndT = GetTickCount();
		printf("Avg transform time per 100 frame %d ms = %d for fast alg\n", EndT - StartT, (EndT - StartT)/100 );
	}/**/
	//check if what we did is right
	//check Luma
	int LumaShiftIn = 0;
	int LumaShiftOut = 0;
	int ErrorCounterY = 0;
	for( int y=0;y<OutHeight;y++)
		for( int x=0;x<OutWidth;x++)
		{
			int val1 = pBuffIn[ LumaShiftIn + (int)( y * InWidth * Step_y ) + (int)( x * Step_x ) ];
			int val2 = pBuffOut[ LumaShiftOut + y * OutWidth + x ];
			if( val1 != val2 ) //due to filtering the 2 values will not be exactly same
//			if( (val1 & (~1)) != (val2 & (~1)) ) //due to filtering the 2 values will not be exactly same. The test precision here is input dependent ! We just know our input right now
			{
//				printf("First Unexpected mismatch at pos %d - %d\n",LumaShiftIn + y * InWidth * Step_y + x * Step_x, LumaShiftOut + y * OutWidth + x );
//				break;
				ErrorCounterY++;
			}
		}
	//check U
	int UShiftIn = InWidth * InHeight;
	int UShiftOut = OutHeight * OutWidth;
	int ErrorCounterU = 0;
	for( int y=0;y<OutHeight / 2;y++)
		for( int x=0;x<OutWidth / 2;x++)
			if( pBuffIn[ UShiftIn + (int)( y * InWidth / 2 * Step_y ) + (int)( x * Step_x ) ] != pBuffOut[ UShiftOut + y * OutWidth / 2 + x ] )
			{
//				printf("First Unexpected mismatch at pos %d - %d\n",LumaShiftIn + y * InWidth + x, LumaShiftOut + y * OutWidth + x );
//				break;
				ErrorCounterU++;
			}
	//check V
	int VShiftIn = UShiftIn + InWidth / 2 * InHeight / 2;
	int VShiftOut = UShiftOut + OutHeight / 2 * OutWidth / 2;
	int ErrorCounterV = 0;
	for( int y=0;y<OutHeight / 2;y++)
		for( int x=0;x<OutWidth / 2;x++)
			if( pBuffIn[ VShiftIn + (int)( y * InWidth / 2 * Step_y ) + (int)( x * Step_x ) ] != pBuffOut[ VShiftOut + y * OutWidth / 2 + x ] )
			{
//				printf("First Unexpected mismatch at pos %d - %d\n",LumaShiftIn + y * InWidth + x, LumaShiftOut + y * OutWidth + x );
//				break;
				ErrorCounterV++;
			}

	printf("Number of mismatches found in Y %d out of %d due to resize algorithm\n",ErrorCounterY, OutWidth * OutHeight );
	printf("Number of mismatches found in U %d out of %d due to resize algorithm\n",ErrorCounterU, OutWidth * OutHeight / 4 );
	printf("Number of mismatches found in V %d out of %d due to resize algorithm\n",ErrorCounterV, OutWidth * OutHeight / 4 );
	//we are done
	hr = pFilter->Stop();
	if( hr != S_OK )
		printf("Could not stop filter \n");
	printf("Done testing Resize\n");
	printf("=============================================================\n");
}