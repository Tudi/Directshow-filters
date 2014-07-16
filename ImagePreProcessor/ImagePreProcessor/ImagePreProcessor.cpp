// ImagePreProcessor.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <initguid.h>
#include "ImagePreProcessorFilter.h"
#include "ImagePreProcessorProperties.h"

// Using this pointer in constructor
//#pragma warning(disable:4355 4127)

// Setup data
const AMOVIESETUP_FILTER sudFilterInfo =
{
    &CLSID_ImagePreProcessor,           // CLSID of filter
    L" Image Pre-Processor Filter",		// Filter's name
    MERIT_DO_NOT_USE,							// Filter merit
    0,
    NULL
};


template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
    CUnknown* punk = new T(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}



// List of class IDs and creator functions for class factory
CFactoryTemplate g_Templates [2]  = {
    { L"ImagePreProcessor"
    , &CLSID_ImagePreProcessor
    , CreateInstance<CImagePreProcessorFilter>
    , NULL
    , &sudFilterInfo }
,
    { L"ImagePreProcessor Property Page"
    , &CLSID_ImagePreProcessorProperties
    , CreateInstance<CImagePreProcessorProperties>
    , NULL
    , NULL }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


#if 0

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}


#endif


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}


#if 0
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, __inout_opt LPVOID);

BOOL 
WINAPI
DllMain(
    HINSTANCE hInstance, 
    ULONG ulReason, 
    __inout_opt LPVOID pv
    )
{
    return DllEntryPoint(hInstance, ulReason, pv);
}
#endif