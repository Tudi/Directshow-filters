// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <streams.h>
#include <mmreg.h>
#include <dvdmedia.h>
#include <ks.h>
#include <ksmedia.h>
#include <memory>
//#include <atlbase.h>
//#include <atlapp.h>
//#include <atlgdi.h>
//#include <atlwin.h>
//#include <atlctrls.h>
#include <wmsdk.h>
#include <vector>
#include <map>
#include <strmif.h>
#include <InitGuid.h>

#pragma warning(disable:4355)

#ifndef IMPLEMENT_QUERY_INTERFACE
#define IMPLEMENT_QUERY_INTERFACE( x ) if( riid == IID_##x ) return GetInterface( (x*)this, ppv )
#endif

using namespace std;

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

#include "GraphConnector.h"
#include "GraphConnectorInput.h"
#include "GraphConnectorOutput.h"

// TODO: reference additional headers your program requires here
