#ifndef _STDAFX_TESTBENCH_H_
#define _STDAFX_TESTBENCH_H_

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <strmif.h>
#include <string.h>
#include <tchar.h>
#include <mtype.h>
#include <uuids.h>
#include <dshow.h>
#include <Streams.h>

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

#include "../../../SRC/Common/VidiatorFilterGuids.h" 

//you might need to compile "dump" filter
DEFINE_GUID(CLSID_Dump,
0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

#endif