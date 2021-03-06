#include "StdAfx.h"


HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin, WCHAR *pPinName)
{
	IEnumPins  *pEnum = NULL;
	IPin       *pPin = NULL;
	HRESULT    hr;

	if (ppPin == NULL)
	{
		return E_POINTER;
	}

	hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	PIN_INFO sInfo;
	while(pEnum->Next(1, &pPin, 0) == S_OK)
	{
		PIN_DIRECTION PinDirThis;
		hr = pPin->QueryDirection(&PinDirThis);
		if (FAILED(hr))
		{
			pPin->Release();
			pEnum->Release();
			return hr;
		}
		hr = pPin->QueryPinInfo(&sInfo);
		if (FAILED(hr))
		{
			pPin->Release();
			pEnum->Release();
			return hr;
		}

		sInfo.pFilter->Release();

		if (PinDir == PinDirThis) 
		{
			// Found a match. Return the IPin pointer to the caller.
			if(pPinName== NULL || _tcscmp(sInfo.achName, pPinName) ==0)
			{
				*ppPin = pPin;
				pEnum->Release();
				return S_OK;
			}
		}
		// Release the pin for the next time through the loop.
		pPin->Release();
	}
	// No more pins. We did not find a match.
	pEnum->Release();
	return E_FAIL;  
}
