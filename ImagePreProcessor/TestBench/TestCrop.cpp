#include "StdAfx.h"

void TestCrop( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut )
{
	HRESULT					hr;

	printf("Starting crop filter test\n");
	printf("Expected buffer format is IYUV ( planar 422 format )\n");
	int CropLeft = IMG_OUT_X / 3;
	int CropTop = IMG_OUT_Y / 3;
	int OutWidth = IMG_OUT_X - CropLeft - IMG_OUT_X / 3;
	int OutHeight = IMG_OUT_Y - CropTop - IMG_OUT_Y / 3;
//	int CropLeft = 100;
//	int CropTop = 100;
//	int OutWidth = IMG_IN_X - 100;
//	int OutHeight = IMG_IN_Y - 100;
	int InWidth = IMG_IN_X;
	int InHeight = IMG_IN_Y;
	printf("Test params are hardcoded for now %dx%d -> %dx%d\n",InWidth,InHeight,OutWidth,OutHeight );
	//set crop specific params
	//just in case other test initialized the filter the we stop it
	hr = pFilter->Stop();
	if( hr != S_OK )
		printf("Could not stop filter \n");
	//set our filter type
	hr = pControl->SetFilterType( VIDEOTrans_Crop );
	if( hr != S_OK )
		printf("Could not set filter type \n");

	//define output for the filter
	hr = pControl->SetImageOutSize( OutWidth, OutHeight, false, 30 );
	if( hr != S_OK )
		printf("Could not set output details \n");

	//set crop details
	hr = pControl->SetCropParams( CropLeft, CropTop, CropLeft+OutWidth, CropTop+OutHeight );
	if( hr != S_OK )
		printf("Could not set crop details \n");

	//we can start the filter again
	hr = pFilter->Run( 0 );
	if( hr != S_OK )
		printf("Could not start filter \n");

	//set buffer sizes for this test
	pIn->SetSize( InWidth * InHeight * 3 / 2 );
	pOut->SetSize( OutWidth * OutHeight * 3 / 2 );

	//are we really done with setup ?
//	hr = pFilter->CheckTransform( pMTIn, pMTOut );
//	if( hr != S_OK )
//		printf("Filter refuses to use our in / out setup \n");

	//initialize our buffers so we may check later the output
	BYTE *pBuffIn;
	hr = pIn->GetPointer( &pBuffIn );
	BYTE *pBuffOut;
	hr = pOut->GetPointer( &pBuffOut );
	for( int i = 0; i < pIn->GetSize(); i++ )
		pBuffIn[i] = i;

	//do the actual transform
	hr = pFilter->Transform( pIn, pOut );
	if( hr != S_OK )
		printf("Input / Output transform failed \n");

	//check if what we did is right
	hr = pIn->GetPointer( &pBuffIn );
	hr = pOut->GetPointer( &pBuffOut );
	//check Luma
	int LumaShiftIn = CropTop * InWidth + CropLeft;
	int LumaShiftOut = 0;
	for( int y=0;y<OutHeight;y++)
		for( int x=0;x<OutWidth;x++)
			if( pBuffIn[ LumaShiftIn + y * InWidth + x ] != pBuffOut[ LumaShiftOut + y * OutWidth + x ] )
			{
				printf("First Unexpected mismatch at pos %d - %d : %d-%d\n",LumaShiftIn + y * InWidth + x, LumaShiftOut + y * OutWidth + x,x,y );
				break;
			}
	//check U
	int UShiftIn = InWidth * InHeight + CropTop * InWidth / 4 + CropLeft / 2;
	int UShiftOut = OutHeight * OutWidth;
	for( int y=0;y<OutHeight / 2;y++)
		for( int x=0;x<OutWidth / 2;x++)
			if( pBuffIn[ UShiftIn + y * InWidth / 2 + x ] != pBuffOut[ UShiftOut + y * OutWidth / 2 + x ] )
			{
				printf("First Unexpected mismatch at pos %d - %d\n",LumaShiftIn + y * InWidth + x, LumaShiftOut + y * OutWidth + x );
				break;
			}
	//check V
	int VShiftIn = UShiftIn + InWidth / 2 * InHeight / 2;
	int VShiftOut = UShiftOut + OutHeight / 2 * OutWidth / 2;
	for( int y=0;y<OutHeight / 2;y++)
		for( int x=0;x<OutWidth / 2;x++)
			if( pBuffIn[ VShiftIn + y * InWidth / 2 + x ] != pBuffOut[ VShiftOut + y * OutWidth / 2 + x ] )
			{
				printf("First Unexpected mismatch at pos %d - %d\n",LumaShiftIn + y * InWidth + x, LumaShiftOut + y * OutWidth + x );
				break;
			}

	//we are done
	hr = pFilter->Stop();
	if( hr != S_OK )
		printf("Could not stop filter \n");
	printf("Done testing Crop\n");
	printf("=============================================================\n");
}