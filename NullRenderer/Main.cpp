#include "stdafx.h"
#include "NullRenderers.h"

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

const AMOVIESETUP_MEDIATYPE g_SubVideoType[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NULL }
};
const AMOVIESETUP_MEDIATYPE g_SubAudioType[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL }
};

const AMOVIESETUP_PIN g_VideoSubPin[] = 
{
	{
		L"Input",           // pin name
			FALSE,              // is rendered?    
			FALSE,              // is output?
			FALSE,              // zero instances allowed?
			FALSE,              // many instances allowed?
			&CLSID_NULL,        // connects to filter (for bridge pins)
			NULL,               // connects to pin (for bridge pins)
			1,                  // count of registered media types
			g_SubVideoType       // list of registered media types    
	}
};
const AMOVIESETUP_PIN g_AudioSubPin[] = 
{
	{
		L"Input",           // pin name
			FALSE,              // is rendered?    
			FALSE,              // is output?
			FALSE,              // zero instances allowed?
			FALSE,              // many instances allowed?
			&CLSID_NULL,        // connects to filter (for bridge pins)
			NULL,               // connects to pin (for bridge pins)
			1,                  // count of registered media types
			g_SubAudioType       // list of registered media types    
	}
};

const AMOVIESETUP_FILTER g_SudFilter[]= 
{
	{
		&__uuidof(CNullVideoRenderer),		// filter clsid
			L"Vidiator Video Null Render",			// filter name
			MERIT_UNLIKELY,                   // ie default for auto graph building
			1,                              // count of registered pins
			g_VideoSubPin					// list of pins to register	
	},
	{
		&__uuidof(CNullAudioRenderer),		// filter clsid
			L"Vidiator Audio Null Render",			// filter name
			MERIT_UNLIKELY,                   // ie default for auto graph building
			1,                              // count of registered pins
			g_AudioSubPin					// list of pins to register	
	}	
};

// --- COM factory table and registration code --------------
//#if !defined(_DEBUG) 
//#	define USE_FILTER_PROTECT
//#endif 

#if defined( USE_FILTER_PROTECT )
inline static BOOL CheckMemKey(void)
{
	//#define BUF_SIZE 256
	HANDLE hMapFile;
	//LPCTSTR pBuf;
	TCHAR szName[MAX_PATH];
	_stprintf_s(szName, MAX_PATH, _T("Global\\NxWmSvr%08x"), GetCurrentProcessId());

	hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,   // read/write access
		FALSE,                 // do not inherit the name
		szName);               // name of mapping object 

	if (hMapFile == NULL) 
	{ 
		_ASSERT(0);
		//printf("Could not open file mapping object (%d).\n", GetLastError());
		return FALSE;
	} 

	//pBuf = (LPCTSTR)MapViewOfFile(hMapFile,    // handle to mapping object
	//					FILE_MAP_ALL_ACCESS,  // read/write permission
	//					0,                    
	//					0,                    
	//					MAPFILE_BUF_SIZE);                   

	//if (pBuf == NULL) 
	//{ 
	//	//printf("Could not map view of file (%d).\n", GetLastError()); 
	//	return FALSE;
	//}

	//MessageBox(NULL, pBuf, TEXT("Process2"), MB_OK);

	//UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return TRUE;
}
#endif 


template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
#ifdef USE_FILTER_PROTECT
	if (!CheckMemKey())
	{
		*phr = E_NOINTERFACE;
		return NULL;
	}
#endif
	CUnknown* punk = new T(lpunk, phr);
	if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}
CFactoryTemplate g_Templates[] = 
{
	{
		g_SudFilter[0].strName,
		g_SudFilter[0].clsID,
		CreateInstance<CNullVideoRenderer>,
		NULL,
		&g_SudFilter[0]
	},
	{
		g_SudFilter[1].strName,
		g_SudFilter[1].clsID,
		CreateInstance<CNullAudioRenderer>,
		NULL,
		&g_SudFilter[1]
	}
};
HMODULE g_instance = NULL;
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved)
{
	g_instance = (HINSTANCE)hModule;
	return DllEntryPoint((HINSTANCE) hModule, ul_reason_for_call, lpReserved);
}

// self-registration entrypoint
STDAPI DllRegisterServer()
{
	// base classes will handle registration using the factory template table

	HRESULT hr = AMovieDllRegisterServer2(true);	
	return hr;
}

STDAPI DllUnregisterServer()
{
	// base classes will handle de-registration using the factory template table
	HRESULT hr = AMovieDllRegisterServer2(false);	
	return hr;
}

