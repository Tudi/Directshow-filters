#pragma once

#include "FilterGuids.h"

// {109D4E8B-2A0E-4753-BC6A-86BE2343E131}
DEFINE_GUID(CLSID_ImagePreProcessorProperties, 
0x109d4e8b, 0x2a0e, 0x4753, 0xbc, 0x6a, 0x86, 0xbe, 0x23, 0x43, 0xe1, 0x31);


class CImagePreProcessorProperties :
	public CBasePropertyPage
{

protected:
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	inline void SetDirty();
	void OnSelChangeComboBox(HWND hwnd, LONG nID);

	void UpdateControls();

public:
	CImagePreProcessorProperties(LPUNKNOWN pUnk, HRESULT *phr);
	~CImagePreProcessorProperties(void);


private:
	CComPtr<IImageResize>	m_pVideoResizer;
	CComboBox					m_ctrlVideoSize;
	CButton						m_chkEnableResize;
	CEdit						m_editWidth;
	CEdit						m_editHeight;
	CStatic						m_stcWidth;
	CStatic						m_stcPrefix;
	CStatic						m_stcHeight;
};
