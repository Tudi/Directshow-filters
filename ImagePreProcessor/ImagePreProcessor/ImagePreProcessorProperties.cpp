#include "StdAfx.h"
#include "resource.h"
#include "ImagePreProcessorProperties.h"

CImagePreProcessorProperties::CImagePreProcessorProperties(LPUNKNOWN pUnk, HRESULT *phr) :
			CBasePropertyPage(NAME(" Subtitle Parser Property Page"), pUnk,IDD_PROGDIALOG, IDS_TITLE)
{
}

CImagePreProcessorProperties::~CImagePreProcessorProperties(void)
{
	OnDisconnect();
}


HRESULT CImagePreProcessorProperties::OnConnect(IUnknown *pUnknown)
{
	CheckPointer(pUnknown, E_POINTER);
	if( m_pVideoResizer )
	{
		m_pVideoResizer = NULL;
	}
	return pUnknown->QueryInterface( &m_pVideoResizer );
}

HRESULT CImagePreProcessorProperties::OnDisconnect()
{
	if( m_pVideoResizer )
	{
		m_pVideoResizer = NULL;
	}
	return S_OK;
}

HRESULT CImagePreProcessorProperties::OnActivate()
{
	return S_OK;
}

HRESULT CImagePreProcessorProperties::OnDeactivate()
{
	return S_OK;
}

HRESULT CImagePreProcessorProperties::OnApplyChanges()
{
	if( !m_pVideoResizer ) return E_FAIL;

	if( m_chkEnableResize.GetCheck() == BST_CHECKED )
	{
		DWORD_PTR data = m_ctrlVideoSize.GetItemData( m_ctrlVideoSize.GetCurSel() );
		LONG w, h;
		if( data != 0 && data != (DWORD_PTR)-1 )
		{
			DWORD dwData = (DWORD)m_ctrlVideoSize.GetItemData( m_ctrlVideoSize.GetCurSel() );
			w = LOWORD(dwData);
			h = HIWORD(dwData);
		}
		else
		{
			TCHAR szBuffer[64];

			m_ctrlVideoSize.GetWindowText( szBuffer, 63 );
			if( wcscmp( L"Same as source size", szBuffer) == 0 )
			{
				m_pVideoResizer->GetImageInputSize( &w, &h );
			}
			else
			{
				m_editWidth.GetWindowText( szBuffer, 63);
				if( wcslen(szBuffer) == 0 )
				{
					MessageBox( m_editWidth, L"Width is needed.", L" Image Pre-Processor Filter", MB_OK );
					m_editWidth.SetFocus();
					return E_FAIL;
				}

				w = _ttoi( szBuffer );
				if( w <= 0 )
				{
					MessageBox( m_editWidth, L"Width is wrong. \n\nPlease, insert valid numbers(0-65535)", L" Image Pre-Processor Filter", MB_OK );
					return E_FAIL;
				}
				m_editHeight.GetWindowText( szBuffer, 63);
				if( wcslen(szBuffer) == 0 )
				{
					MessageBox( m_editHeight, L"Width is needed.", L" Image Pre-Processor Filter", MB_OK );
					m_editHeight.SetFocus();
					return E_FAIL;
				}
				h = _ttoi( szBuffer );
				if( h <= 0 )
				{
					MessageBox( m_editHeight, L"Height is wrong. \n\nPlease, insert valid numbers(0-65535)", L" Image Pre-Processor Filter", MB_OK );
					m_editHeight.SetFocus();
					return E_FAIL;
				}
			}
		}

		m_pVideoResizer->SetFilterType(VIDEOTrans_Resize);
		HRESULT hr = m_pVideoResizer->SetImageOutSize(w,h,true, 30);
		if( FAILED(hr) ) return hr;
	}
	else
	{
		m_pVideoResizer->SetFilterType(VIDEOTrans_ColorConv);
		//LONG w, h;
		//HRESULT hr = m_pVideoResizer->GetImageInputSize( &w, &h );
		//if( FAILED(hr) ) return hr;
		//hr = m_pVideoResizer->SetImageOutSize( w, h, true, 30 );
		//if( FAILED(hr) ) return hr;
	}

	m_bDirty = false;

	return S_OK;
}

INT_PTR CImagePreProcessorProperties::OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch( uMsg )
	{
	case WM_INITDIALOG:
		m_chkEnableResize.m_hWnd = GetDlgItem( hwnd, IDC_CHECK_ENABLE );
		m_ctrlVideoSize.m_hWnd = GetDlgItem( hwnd, IDC_COMBO_PREFIX );
		m_editWidth.m_hWnd = GetDlgItem( hwnd, IDC_EDIT_WIDTH );
		m_editHeight.m_hWnd = GetDlgItem( hwnd, IDC_EDIT_HEIGHT );
		m_stcWidth.m_hWnd = GetDlgItem( hwnd, IDC_STATIC_WIDTH );
		m_stcHeight.m_hWnd = GetDlgItem( hwnd, IDC_STATIC_HEIGHT );
		m_stcPrefix.m_hWnd = GetDlgItem( hwnd, IDC_STATIC_PREFIX );

		int idx;
		idx = m_ctrlVideoSize.AddString(_T("SQCIF [128x96]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(128, 96) );
		idx = m_ctrlVideoSize.AddString(_T("QSIF [160x120]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(160, 120) );
		idx = m_ctrlVideoSize.AddString(_T("QCIF [176x144]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(176, 144) );
		idx = m_ctrlVideoSize.AddString(_T("QVGA [320x240]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(320, 240) );
		idx = m_ctrlVideoSize.AddString(_T("CIF [352x288]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(320, 240) );
		idx = m_ctrlVideoSize.AddString(_T("[352x576]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(352, 288) );
		idx = m_ctrlVideoSize.AddString(_T("[400x224]")); m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(400, 224));
		idx = m_ctrlVideoSize.AddString(_T("QSVGA [400x300]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(400, 300) );
		idx = m_ctrlVideoSize.AddString(_T("HSVGA [480x320]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(480, 320) );
		idx = m_ctrlVideoSize.AddString(_T("nHD [640x360]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(640, 360) );
		idx = m_ctrlVideoSize.AddString(_T("VGA [640x480]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(640, 480) );
		idx = m_ctrlVideoSize.AddString(_T("525SD [720x480]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(720, 480) );
		idx = m_ctrlVideoSize.AddString(_T("625SD [720x576]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(720, 576) );
		idx = m_ctrlVideoSize.AddString(_T("SVGA [800x600]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(800, 600) );
		idx = m_ctrlVideoSize.AddString(_T("XVGA [1024x768]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(1024, 768) );
		idx = m_ctrlVideoSize.AddString(_T("720p HD [1280x720]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(1280, 720) );
		idx = m_ctrlVideoSize.AddString(_T("1080p HD [1920x1080]"));	m_ctrlVideoSize.SetItemData( idx, MAKEWPARAM(1920, 1080) );
		idx = m_ctrlVideoSize.AddString(_T("Same as source size"));	m_ctrlVideoSize.SetItemData( idx, 0 );
		idx = m_ctrlVideoSize.AddString(_T("Custom"));	m_ctrlVideoSize.SetItemData( idx, -1 );

		if( m_pVideoResizer )
		{
			boolean bEnabled;
			m_pVideoResizer->GetImageResizeEnabled(&bEnabled);
			if( bEnabled )
			{
				TCHAR szText[64];
				LONG w,h, i, count;
				LONG idx = -1;

				m_pVideoResizer->GetImageOutSize( &w, &h );
				count = m_ctrlVideoSize.GetCount();
				for( i = 0; i < count; i ++ )
				{
					if( m_ctrlVideoSize.GetItemData(i) == MAKELPARAM(w,h) )
					{
						idx = i;
						break;
					}
				}

				if( idx == -1 )
				{
					idx = m_ctrlVideoSize.FindString(0, L"Custom" );
					
					swprintf( szText, 63, L"%d", w );
					m_editWidth.SetWindowText( szText );
					swprintf( szText, 63, L"%d", h );
					m_editHeight.SetWindowText( szText );
				}
				else
				{
					m_ctrlVideoSize.SetCurSel(idx);
					OnSelChangeComboBox( hwnd, IDC_COMBO_PREFIX );
				}
			}
			else
			{
				m_ctrlVideoSize.SetCurSel( m_ctrlVideoSize.FindString(0, _T("Same as source size")) );
			}
			CheckDlgButton( hwnd, IDC_CHECK_ENABLE, bEnabled ? BST_CHECKED : BST_UNCHECKED );
		}

		UpdateControls();
		break;
	case WM_COMMAND:
		if( HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == LBN_SELCHANGE )
		{
			OnSelChangeComboBox( hwnd, LOWORD(wParam) );
		}
		else if(HIWORD(wParam) == BN_CLICKED)
		{
			UpdateControls();
		}
		break;
	}
	return S_OK;
}


inline void CImagePreProcessorProperties::SetDirty()
{
    m_bDirty = TRUE;

    if(m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


void CImagePreProcessorProperties::OnSelChangeComboBox(HWND hwnd, LONG nID)
{
	if( nID == IDC_COMBO_PREFIX )
	{
		long idx = m_ctrlVideoSize.GetCurSel();
		LONG data = (LONG)m_ctrlVideoSize.GetItemData(idx);
		LONG w, h;

		if( data == 0 )
		{
			if( !FAILED(m_pVideoResizer->GetImageInputSize( &w, &h ) ) )
			{
				SetDlgItemInt( hwnd, IDC_EDIT_WIDTH, w, false );
				SetDlgItemInt( hwnd, IDC_EDIT_HEIGHT, h, false );
			}
			else
			{
				m_editWidth.SetWindowText( _T("") );
				m_editHeight.SetWindowText( _T("") );
			}
		}
		else if( data == -1 )
		{
		}
		else
		{
			w = LOWORD(data);
			h = HIWORD(data);

			SetDlgItemInt( hwnd, IDC_EDIT_WIDTH, w, false );
			SetDlgItemInt( hwnd, IDC_EDIT_HEIGHT, h, false );
		}
		UpdateControls();
	}

	SetDirty();
}


void CImagePreProcessorProperties::UpdateControls( )
{
	bool bEnable = (m_chkEnableResize.GetCheck() == BST_CHECKED);

	m_ctrlVideoSize.EnableWindow( bEnable );
	m_stcPrefix.EnableWindow( bEnable );

	bEnable &= (m_ctrlVideoSize.GetItemData( m_ctrlVideoSize.GetCurSel() ) == -1);
	m_editWidth.EnableWindow( bEnable );
	m_editHeight.EnableWindow( bEnable );
	m_stcWidth.EnableWindow( bEnable );
	m_stcHeight.EnableWindow( bEnable );
}