#ifndef _STDAFX_TESTBENCH_H_
#define _STDAFX_TESTBENCH_H_

#define IMG_IN_X	352
#define IMG_IN_Y	288

#define IMG_OUT_X	176
#define IMG_OUT_Y	144

#include <Streams.h>
#include <mmreg.h>
#include <dvdmedia.h>
#include <ks.h>
#include <ksmedia.h>
#include <memory>
#include <atlbase.h>
#include <atlapp.h>
#include <atlgdi.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <wmsdk.h>
#include <strmif.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <strmif.h>
#include <string.h>
#include <tchar.h>
#include <mtype.h>
#include <uuids.h>
#include <dshow.h>


//include mem leak detector in all our CPP files for proper reporting
#if !defined(USE_EXTERNAL_MEMORY_LEAK_AND_BOUNDS_CHECKER) && defined( _DEBUG )
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#ifdef _CRTDBG_MAP_ALLOC
		#include <crtdbg.h>
		//use these only if you are not using external memory debuggers
		#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
		#define new DEBUG_NEW
	#endif
#endif 

#include "VidiatorFilterGuids.h" 
//#include "../../../../../ironman/xle_602/include/descriptions.h" 
#include "VidiMedia.h"

//should move these to a new H later
void TestCrop( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut );
void TestResize( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut );
void TestGreenEnh( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut );
void TestDeinterlace( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut );
void TestDeNoise( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut );
void TestColorConv( IVidiatorImageResize *pControl, CTransformFilter *pFilter, CMediaType *pMTIn, CMediaType *pMTOut, VidiMedia *pIn, VidiMedia *pOut );
void TestPIN( );

#endif