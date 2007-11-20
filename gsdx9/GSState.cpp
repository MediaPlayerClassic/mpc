/* 
 *	Copyright (C) 2003-2005 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "GSState.h"
#include "GSdx9.h"
#include "GSUtil.h"
#include "GSSettingsDlg.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(GSState, CWnd)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

GSState::GSState() 
	: m_osd(true)
	, m_field(0)
	, m_irq(NULL)
	, m_q(1.0f)
	, m_crc(0)
	, m_options(0)
	, m_path3hack(0)
{
	m_nInterlace = AfxGetApp()->GetProfileInt(_T("Settings"), _T("Interlace"), 0);
	m_nAspectRatio = AfxGetApp()->GetProfileInt(_T("Settings"), _T("AspectRatio"), 1);
	m_nTextureFilter = (D3DTEXTUREFILTERTYPE)AfxGetApp()->GetProfileInt(_T("Settings"), _T("TextureFilter"), D3DTEXF_LINEAR);
	m_nloophack = AfxGetApp()->GetProfileInt(_T("Settings"), _T("nloophack"), 2) == 1;
	m_vsync = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("vsync"), TRUE);

//	m_regs.pCSR->rREV = 0x20;

	m_env.PRMODECONT.AC = 1;

	m_pPRIM = &m_env.PRIM;

	m_pTransferBuffer = (BYTE*)_aligned_malloc(1024*1024*4, 16);
	m_nTransferBytes = 0;

	for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
	{
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;
	}

	m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerPRIM;
	m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerRGBA;
	m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerSTQ;
	m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerUV;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = &GSState::GIFPackedRegHandlerTEX0_1;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = &GSState::GIFPackedRegHandlerTEX0_2;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GSState::GIFPackedRegHandlerCLAMP_1;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GSState::GIFPackedRegHandlerCLAMP_2;
	m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerXYZF3;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerXYZ3;
	m_fpGIFPackedRegHandlers[GIF_REG_A_D] = &GSState::GIFPackedRegHandlerA_D;
	m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

	for(int i = 0; i < countof(m_fpGIFRegHandlers); i++)
	{
		m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNull;
	}

	m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
	m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
	m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
	m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerXYZF2;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerXYZ2;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GSState::GIFRegHandlerTEX0_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GSState::GIFRegHandlerTEX0_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GSState::GIFRegHandlerCLAMP_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GSState::GIFRegHandlerCLAMP_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOG] = &GSState::GIFRegHandlerFOG;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerXYZF3;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerXYZ3;
	m_fpGIFRegHandlers[GIF_A_D_REG_NOP] = &GSState::GIFRegHandlerNOP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_1] = &GSState::GIFRegHandlerTEX1_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_2] = &GSState::GIFRegHandlerTEX1_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_1] = &GSState::GIFRegHandlerTEX2_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_2] = &GSState::GIFRegHandlerTEX2_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_1] = &GSState::GIFRegHandlerXYOFFSET_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_2] = &GSState::GIFRegHandlerXYOFFSET_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerPRMODECONT;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerPRMODE;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXCLUT] = &GSState::GIFRegHandlerTEXCLUT;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCANMSK] = &GSState::GIFRegHandlerSCANMSK;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_1] = &GSState::GIFRegHandlerMIPTBP1_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_2] = &GSState::GIFRegHandlerMIPTBP1_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_1] = &GSState::GIFRegHandlerMIPTBP2_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_2] = &GSState::GIFRegHandlerMIPTBP2_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXA] = &GSState::GIFRegHandlerTEXA;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOGCOL] = &GSState::GIFRegHandlerFOGCOL;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXFLUSH] = &GSState::GIFRegHandlerTEXFLUSH;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_1] = &GSState::GIFRegHandlerSCISSOR_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_2] = &GSState::GIFRegHandlerSCISSOR_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_1] = &GSState::GIFRegHandlerALPHA_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_2] = &GSState::GIFRegHandlerALPHA_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_DIMX] = &GSState::GIFRegHandlerDIMX;
	m_fpGIFRegHandlers[GIF_A_D_REG_DTHE] = &GSState::GIFRegHandlerDTHE;
	m_fpGIFRegHandlers[GIF_A_D_REG_COLCLAMP] = &GSState::GIFRegHandlerCOLCLAMP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_1] = &GSState::GIFRegHandlerTEST_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_2] = &GSState::GIFRegHandlerTEST_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_PABE] = &GSState::GIFRegHandlerPABE;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_1] = &GSState::GIFRegHandlerFBA_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_2] = &GSState::GIFRegHandlerFBA_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_1] = &GSState::GIFRegHandlerFRAME_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_2] = &GSState::GIFRegHandlerFRAME_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_1] = &GSState::GIFRegHandlerZBUF_1;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_2] = &GSState::GIFRegHandlerZBUF_2;
	m_fpGIFRegHandlers[GIF_A_D_REG_BITBLTBUF] = &GSState::GIFRegHandlerBITBLTBUF;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXPOS] = &GSState::GIFRegHandlerTRXPOS;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXREG] = &GSState::GIFRegHandlerTRXREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXDIR] = &GSState::GIFRegHandlerTRXDIR;
	m_fpGIFRegHandlers[GIF_A_D_REG_HWREG] = &GSState::GIFRegHandlerHWREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GSState::GIFRegHandlerSIGNAL;
	m_fpGIFRegHandlers[GIF_A_D_REG_FINISH] = &GSState::GIFRegHandlerFINISH;
	m_fpGIFRegHandlers[GIF_A_D_REG_LABEL] = &GSState::GIFRegHandlerLABEL;

	ResetState();
}

GSState::~GSState()
{
	ResetState();

	_aligned_free(m_pTransferBuffer);

	DestroyWindow();
}

bool GSState::Create(LPCTSTR title)
{
	// window

	CRect r;
	GetDesktopWindow()->GetWindowRect(r);
	CSize s(r.Width()/3, r.Width()/4);
	r = CRect(r.CenterPoint() - CSize(s.cx/2, s.cy/2), s);

	LPCTSTR wc = AfxRegisterWndClass(CS_VREDRAW|CS_HREDRAW|CS_DBLCLKS, AfxGetApp()->LoadStandardCursor(IDC_ARROW), 0, 0);

	if(!CreateEx(0, wc, title, WS_OVERLAPPEDWINDOW, r, NULL, 0))
	{
		return false;
	}

	// ddraw

	CComPtr<IDirectDraw7> pDD; 

	if(FAILED(DirectDrawCreateEx(0, (void**)&pDD, IID_IDirectDraw7, 0)))
	{
		return false;
	}

	m_ddcaps.dwSize = sizeof(DDCAPS); 

	if(FAILED(pDD->GetCaps(&m_ddcaps, NULL)))
	{
		return false;
	}

	pDD = NULL;

	// d3d

	if(!(m_d3d = Direct3DCreate9(D3D_SDK_VERSION)))
	{
		return false;
	}

	memset(&m_caps, 0, sizeof(m_caps));

	m_d3d->GetDeviceCaps(
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
		// D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, 
		&m_caps);

	// d3d device

	if(FAILED(ResetDevice()))
	{
		return false;
	}

	// shaders

	DWORD PixelShaderVersion = AfxGetApp()->GetProfileInt(_T("Settings"), _T("PixelShaderVersion2"), D3DPS_VERSION(2, 0));

	if(PixelShaderVersion > m_caps.PixelShaderVersion)
	{
		CString str;

		str.Format(_T("Supported pixel shader version is too low!\n\nSupported: %d.%d\nSelected: %d.%d"),
			D3DSHADER_VERSION_MAJOR(m_caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(m_caps.PixelShaderVersion),
			D3DSHADER_VERSION_MAJOR(PixelShaderVersion), D3DSHADER_VERSION_MINOR(PixelShaderVersion));

		AfxMessageBox(str);

		m_dev = NULL;

		return false;
	}

	m_caps.PixelShaderVersion = min(PixelShaderVersion, m_caps.PixelShaderVersion);

	m_caps.VertexShaderVersion = min(m_caps.PixelShaderVersion & ~0x10000, m_caps.VertexShaderVersion);

	static const TCHAR* hlsl_tfx[] = 
	{
		_T("main_tfx0_32"), _T("main_tfx1_32"), _T("main_tfx2_32"), _T("main_tfx3_32"),
		_T("main_tfx0_24"), _T("main_tfx1_24"), _T("main_tfx2_24"), _T("main_tfx3_24"),
		_T("main_tfx0_24AEM"), _T("main_tfx1_24AEM"), _T("main_tfx2_24AEM"), _T("main_tfx3_24AEM"),
		_T("main_tfx0_16"), _T("main_tfx1_16"), _T("main_tfx2_16"), _T("main_tfx3_16"), 
		_T("main_tfx0_16AEM"), _T("main_tfx1_16AEM"), _T("main_tfx2_16AEM"), _T("main_tfx3_16AEM"), 
		_T("main_tfx0_8P_pt"), _T("main_tfx1_8P_pt"), _T("main_tfx2_8P_pt"), _T("main_tfx3_8P_pt"), 
		_T("main_tfx0_8P_ln"), _T("main_tfx1_8P_ln"), _T("main_tfx2_8P_ln"), _T("main_tfx3_8P_ln"), 
		_T("main_tfx0_8HP_pt"), _T("main_tfx1_8HP_pt"), _T("main_tfx2_8HP_pt"), _T("main_tfx3_8HP_pt"), 
		_T("main_tfx0_8HP_ln"), _T("main_tfx1_8HP_ln"), _T("main_tfx2_8HP_ln"), _T("main_tfx3_8HP_ln"),
		_T("main_notfx"),
	};

	// ps_3_0

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
	{
		DWORD flags = D3DXSHADER_PARTIALPRECISION|D3DXSHADER_AVOID_FLOW_CONTROL;

		for(int i = 0; i < countof(hlsl_tfx); i++)
		{
			if(!m_pHLSLTFX[i])
			{
				CompileShaderFromResource(m_dev, IDR_HLSL_TFX, hlsl_tfx[i], _T("ps_3_0"), flags, &m_pHLSLTFX[i]);
			}
		}

		for(int i = 0; i < 3; i++)
		{
			if(!m_pHLSLMerge[i])
			{
				CString main;
				main.Format(_T("main%d"), i);
				CompileShaderFromResource(m_dev, IDR_HLSL_MERGE, main, _T("ps_3_0"), flags, &m_pHLSLMerge[i]);
			}
		}

		for(int i = 0; i < 4; i++)
		{
			if(!m_pHLSLInterlace[i])
			{
				CString main;
				main.Format(_T("main%d"), i);
				CompileShaderFromResource(m_dev, IDR_HLSL_INTERLACE, main, _T("ps_3_0"), flags, &m_pHLSLInterlace[i]);
			}
		}
	}

	// ps_2_0

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
	{
		DWORD flags = D3DXSHADER_PARTIALPRECISION;

		for(int i = 0; i < countof(hlsl_tfx); i++)
		{
			if(!m_pHLSLTFX[i])
			{
				CompileShaderFromResource(m_dev, IDR_HLSL_TFX, hlsl_tfx[i], _T("ps_2_0"), flags, &m_pHLSLTFX[i]);
			}
		}

		for(int i = 0; i < 3; i++)
		{
			if(!m_pHLSLMerge[i])
			{
				CString main;
				main.Format(_T("main%d"), i);
				CompileShaderFromResource(m_dev, IDR_HLSL_MERGE, main, _T("ps_2_0"), flags, &m_pHLSLMerge[i]);
			}
		}

		for(int i = 0; i < 4; i++)
		{
			if(!m_pHLSLInterlace[i])
			{
				CString main;
				main.Format(_T("main%d"), i);
				CompileShaderFromResource(m_dev, IDR_HLSL_INTERLACE, main, _T("ps_2_0"), flags, &m_pHLSLInterlace[i]);
			}
		}
	}

	ResetState();

	return true;
}

void GSState::Show()
{
	SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow();
	ShowWindow(SW_SHOWNORMAL);
}

void GSState::Hide()
{
	ShowWindow(SW_HIDE);
}

bool GSState::OnMsg(const MSG& msg)
{
	if(msg.message == WM_KEYDOWN)
	{
		int step = (::GetAsyncKeyState(VK_SHIFT) & 0x80000000) ? -1 : 1;

		if(msg.wParam == VK_F5)
		{
			m_nInterlace = (m_nInterlace + 7 + step) % 7;
			return true;
		}

		if(msg.wParam == VK_F6)
		{
			m_nAspectRatio = (m_nAspectRatio + 3 + step) % 3;
			return true;
		}			

		if(msg.wParam == VK_F7)
		{
			SetWindowText(_T("PCSX2"));
			m_osd = !m_osd;
			return true;
		}
	}

	return false;
}

void GSState::OnClose()
{
	Hide();

	ResetDevice(true);

	PostMessage(WM_QUIT);
}

void GSState::ResetState()
{
	memset(&m_env, 0, sizeof(m_env));
	memset(m_path, 0, sizeof(m_path));
	memset(&m_v, 0, sizeof(m_v));

//	m_env.PRMODECONT.AC = 1;
//	m_pPRIM = &m_env.PRIM;

	m_context = &m_env.CTXT[0];

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].FRAME.PSM];
	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].ZBUF.PSM];
	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].TEX0.PSM];

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].FRAME.PSM];
	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].ZBUF.PSM];
	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].TEX0.PSM];

	if(m_dev) m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
}

HRESULT GSState::ResetDevice(bool fForceWindowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HMODULE hModule = AfxGetResourceHandle();

    CWinApp* pApp = AfxGetApp();

	HRESULT hr;

	if(!m_d3d) return E_FAIL;

	m_pSwapChain = NULL;
	m_pMergeTexture = NULL;
	m_pInterlaceTexture = NULL;
	m_pDeinterlaceTexture = NULL;
	m_pCurrentFrame = NULL;
	m_pD3DXFont = NULL;

	memset(&m_d3dpp, 0, sizeof(m_d3dpp));

	m_d3dpp.Windowed = TRUE;
	m_d3dpp.hDeviceWindow = m_hWnd;
	m_d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
	m_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_d3dpp.BackBufferWidth = 1;
	m_d3dpp.BackBufferHeight = 1;
	m_d3dpp.PresentationInterval = m_vsync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;

	if(!!pApp->GetProfileInt(_T("Settings"), _T("fEnableTvOut"), FALSE))
	{
		m_d3dpp.Flags |= D3DPRESENTFLAG_VIDEO;
	}

	int ModeWidth = pApp->GetProfileInt(_T("Settings"), _T("ModeWidth"), 0);
	int ModeHeight = pApp->GetProfileInt(_T("Settings"), _T("ModeHeight"), 0);
	int ModeRefreshRate = pApp->GetProfileInt(_T("Settings"), _T("ModeRefreshRate"), 0);

	if(!fForceWindowed && ModeWidth > 0 && ModeHeight > 0 && ModeRefreshRate >= 0)
	{
		m_d3dpp.Windowed = FALSE;
		m_d3dpp.BackBufferWidth = ModeWidth;
		m_d3dpp.BackBufferHeight = ModeHeight;
		// m_d3dpp.FullScreen_RefreshRateInHz = ModeRefreshRate;

		::SetWindowLong(m_hWnd, GWL_STYLE, ::GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME));
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		SetMenu(NULL);

		m_osd = false;
	}

	if(!m_dev)
	{
		if(FAILED(hr = m_d3d->CreateDevice(
			// m_d3d->GetAdapterCount()-1, D3DDEVTYPE_REF,
			// D3DADAPTER_DEFAULT, D3DDEVTYPE_REF,
			D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
			m_hWnd,
			D3DCREATE_MULTITHREADED | (m_caps.VertexProcessingCaps ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING), 
			&m_d3dpp, &m_dev)))
		{
			return hr;
		}
	}
	else
	{
		if(FAILED(hr = m_dev->Reset(&m_d3dpp)))
		{
			if(D3DERR_DEVICELOST == hr)
			{
				Sleep(1000);

				if(FAILED(hr = m_dev->Reset(&m_d3dpp)))
				{
					return hr;
				}
			}
			else
			{
				return hr;
			}
		}
	}

	if(m_d3dpp.Windowed)
	{
		m_d3dpp.BackBufferWidth = 1;
		m_d3dpp.BackBufferHeight = 1;

		if(FAILED(hr = m_dev->CreateAdditionalSwapChain(&m_d3dpp, &m_pSwapChain)))
		{
			return hr;
		}
	}

	CComPtr<IDirect3DSurface9> pBackBuffer = GetBackBuffer();
	
	hr = m_dev->SetRenderTarget(0, pBackBuffer);

	hr = m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	pBackBuffer->GetDesc(&desc);

	D3DXFONT_DESC fd;
	memset(&fd, 0, sizeof(fd));
	_tcscpy(fd.FaceName, _T("Arial"));
	fd.Height = -(int)(sqrt((float)desc.Height) * 0.7);
	hr = D3DXCreateFontIndirect(m_dev, &fd, &m_pD3DXFont);

    hr = m_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_dev->SetRenderState(D3DRS_LIGHTING, FALSE);

	for(int i = 0; i < 8; i++)
	{
		hr = m_dev->SetSamplerState(i, D3DSAMP_MAGFILTER, m_nTextureFilter);
		hr = m_dev->SetSamplerState(i, D3DSAMP_MINFILTER, m_nTextureFilter);
		// hr = m_dev->SetSamplerState(i, D3DSAMP_MIPFILTER, m_nTextureFilter);
		hr = m_dev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_dev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hr = m_dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	hr = m_dev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
	hr = m_dev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	hr = m_dev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);

	return S_OK;
}

CComPtr<IDirect3DSurface9> GSState::GetBackBuffer()
{
	HRESULT hr;

	CComPtr<IDirect3DSurface9> pBackBuffer = NULL;

	if(m_pSwapChain)
	{
		hr = m_pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	}
	else
	{
		hr = m_dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	}

	return pBackBuffer;
}

UINT32 GSState::Freeze(freezeData* fd, bool fSizeOnly)
{
	int size = sizeof(m_version)
		+ sizeof(m_env)
		+ sizeof(m_v) 
		+ sizeof(m_x) 
		+ sizeof(m_y) 
		+ 1024*1024*4
		+ sizeof(m_path) 
		+ sizeof(m_q)
		/*+ sizeof(m_vl)*/;

	if(fSizeOnly)
	{
		fd->size = size;
		return 0;
	}
	else if(!fd->data || fd->size < size)
	{
		return -1;
	}

	Flush();

	BYTE* data = fd->data;
	memcpy(data, &m_version, sizeof(m_version)); data += sizeof(m_version);
	memcpy(data, &m_env, sizeof(m_env)); data += sizeof(m_env); 
	memcpy(data, &m_v, sizeof(m_v)); data += sizeof(m_v);
	memcpy(data, &m_x, sizeof(m_x)); data += sizeof(m_x);
	memcpy(data, &m_y, sizeof(m_y)); data += sizeof(m_y);
	memcpy(data, m_mem.GetVM(), 1024*1024*4); data += 1024*1024*4;
	memcpy(data, m_path, sizeof(m_path)); data += sizeof(m_path);
	memcpy(data, &m_q, sizeof(m_q)); data += sizeof(m_q);
	// memcpy(data, &m_vl, sizeof(m_vl)); data += sizeof(m_vl);

	return 0;
}

UINT32 GSState::Defrost(const freezeData* fd)
{
	if(!fd || !fd->data || fd->size == 0) 
		return -1;

	int size = sizeof(m_version)
		+ sizeof(m_env) 
		+ sizeof(m_v) 
		+ sizeof(m_x) 
		+ sizeof(m_y) 
		+ 1024*1024*4
		+ sizeof(m_path)
		+ sizeof(m_q)
		/*+ sizeof(m_vl)*/;

	if(fd->size != size) 
		return -1;

	BYTE* data = fd->data;

	int version = 0;
	memcpy(&version, data, sizeof(version)); data += sizeof(version);
	if(m_version != version) return -1;

	Flush();

	memcpy(&m_env, data, sizeof(m_env)); data += sizeof(m_env); 
	memcpy(&m_v, data, sizeof(m_v)); data += sizeof(m_v);
	memcpy(&m_x, data, sizeof(m_x)); data += sizeof(m_x);
	memcpy(&m_y, data, sizeof(m_y)); data += sizeof(m_y);
	memcpy(m_mem.GetVM(), data, 1024*1024*4); data += 1024*1024*4;
	memcpy(&m_path, data, sizeof(m_path)); data += sizeof(m_path);
	memcpy(&m_q, data, sizeof(m_q)); data += sizeof(m_q);
	// memcpy(&m_vl, data, sizeof(m_vl)); data += sizeof(m_vl);

	m_pPRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	m_context = &m_env.CTXT[m_pPRIM->CTXT];

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].FRAME.PSM];
	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].ZBUF.PSM];
	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].TEX0.PSM];

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].FRAME.PSM];
	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].ZBUF.PSM];
	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].TEX0.PSM];
/*
m_perfmon.SetFrame(4950);
*/
	return 0;
}

void GSState::Reset()
{
	ResetState();
}

void GSState::WriteCSR(UINT32 csr)
{
	m_regs.pCSR->ai32[1] = csr;
}

void GSState::ReadFIFO(BYTE* mem)
{
	FlushWriteTransfer();

	ReadTransfer(mem, 16);
}

void GSState::Transfer(BYTE* mem, UINT32 size, int index)
{
	GIFPath& path = m_path[index];

	while(size > 0)
	{
		bool fEOP = false;

		if(path.tag.NLOOP == 0)
		{
			path.tag = *(GIFTag*)mem;
			path.nreg = 0;

			mem += sizeof(GIFTag);
			size--;

			m_q = 1.0f;

			if(index == 2 && path.tag.EOP)
			{
				m_path3hack = 1;
			}

			if(path.tag.PRE)
			{
				GIFReg r;
				r.i64 = path.tag.PRIM;
				(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r);
			}

			if(path.tag.EOP)
			{
				fEOP = true;
			}
			else if(path.tag.NLOOP == 0)
			{
				if(index == 0 && m_nloophack)
				{
					continue;
				}

				fEOP = true;
			}
		}

		UINT32 size_msb = size & (1<<31);

		switch(path.tag.FLG)
		{
		case GIF_FLG_PACKED:

			for(GIFPackedReg* r = (GIFPackedReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFPackedReg))
			{
				(this->*m_fpGIFPackedRegHandlers[path.GetGIFReg()])(r);

				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG) 
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}

			break;

		case GIF_FLG_REGLIST:

			size *= 2;

			for(GIFReg* r = (GIFReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFReg))
			{
				(this->*m_fpGIFRegHandlers[path.GetGIFReg()])(r);

				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG)
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}
			
			if(size & 1) mem += sizeof(GIFReg);

			size /= 2;
			size |= size_msb; // a bit lame :P
			
			break;

		case GIF_FLG_IMAGE2: // hmmm
			
			path.tag.NLOOP = 0;

			break;

		case GIF_FLG_IMAGE:
			{
				int len = min(size, path.tag.NLOOP);

				//ASSERT(!(len&3));

				switch(m_env.TRXDIR.XDIR)
				{
				case 0:
					WriteTransfer(mem, len*16);
					break;
				case 1: 
					ReadTransfer(mem, len*16); // TODO: writing access violation with aqtime
					break;
				case 2: 
					//MoveTransfer();
					break;
				case 3: 
					ASSERT(0);
					break;
				default: 
					__assume(0);
				}

				mem += len*16;
				path.tag.NLOOP -= len;
				size -= len;
			}

			break;

		default: 
			__assume(0);
		}

		if(fEOP && (INT32)size <= 0)
		{
			break;
		}
	}
}

void GSState::VSync(int field)
{
	m_field = !!field;

	Flush();

	Flip();

	m_perfmon.Put(GSPerfMon::Frame);

	Present();
}

UINT32 GSState::MakeSnapshot(char* path)
{
	CString fn;
	fn.Format(_T("%sgsdx9_%s.bmp"), CString(path), CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
	return D3DXSaveSurfaceToFile(fn, D3DXIFF_BMP, m_pCurrentFrame, NULL, NULL);
}

void GSState::SetGameCRC(int crc, int options)
{
	m_crc = crc;
	m_options = options;

	if(AfxGetApp()->GetProfileInt(_T("Settings"), _T("nloophack"), 2) == 2)
	{
		switch(crc)
		{
		case 0xa39517ab: // ffx pal
			m_nloophack = true;
			break;
		}
	}
}

static bool CheckSize(IDirect3DTexture9* t, CSize s)
{
	D3DSURFACE_DESC desc;

	memset(&desc, 0, sizeof(desc));

	if(t && SUCCEEDED(t->GetLevelDesc(0, &desc)))
	{
		return desc.Width == s.cx && desc.Height == s.cy;
	}

	return false;
}

void GSState::FinishFlip(FlipInfo src[2])
{
	HRESULT hr;

	CSize fs(0, 0);
	CSize ds(0, 0);

	for(int i = 0; i < 2; i++)
	{
		if(src[i].tex)
		{
			CSize s = m_regs.GetFrameSize(i);

			s.cx = (int)(src[i].scale.x * s.cx);
			s.cy = (int)(src[i].scale.y * s.cy);

			ASSERT(fs.cx == 0 || fs.cx == s.cx);
			ASSERT(fs.cy == 0 || fs.cy == s.cy || fs.cy+1 == s.cy);

			fs.cx = s.cx;
			fs.cy = s.cy;

			if(m_regs.pSMODE2->INT && m_regs.pSMODE2->FFMD) s.cy *= 2;

			ASSERT(ds.cx == 0 || ds.cx == s.cx);
			ASSERT(ds.cy == 0 || ds.cy == s.cy || ds.cy+1 == s.cy);

			ds.cx = s.cx;
			ds.cy = s.cy;
		}
	}

	if(fs.cx == 0 || fs.cy == 0)
	{
		return;
	}

	CComPtr<IDirect3DSurface9> dst;
	CComPtr<IDirect3DSurface9> surf[3];

	// merge

	if(!CheckSize(m_pMergeTexture, fs))
	{
		m_pMergeTexture = NULL;
	}

	if(!m_pMergeTexture) 
	{
		hr = m_dev->CreateTexture(fs.cx, fs.cy, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pMergeTexture, NULL);

		if(FAILED(hr)) return;
	}

	hr = m_pMergeTexture->GetSurfaceLevel(0, &surf[0]);

	Merge(src, surf[0]);

	dst = surf[0];

	if(m_regs.pSMODE2->INT && m_nInterlace > 0)
	{
		int field = 1 - ((m_nInterlace - 1) & 1);
		int mode = (m_nInterlace - 1) >> 1;

		if(!CheckSize(m_pInterlaceTexture, ds))
		{
			m_pInterlaceTexture = NULL;
		}

		if(!m_pInterlaceTexture) 
		{
			hr = m_dev->CreateTexture(ds.cx, ds.cy, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pInterlaceTexture, NULL);

			if(FAILED(hr)) return;
		}

		hr = m_pInterlaceTexture->GetSurfaceLevel(0, &surf[1]);

		if(mode == 0 || mode == 2) // weave or blend
		{
			// weave first

			Interlace(m_pMergeTexture, surf[1], field ^ m_field, D3DTEXF_POINT);

			dst = surf[1];

			if(mode == 2)
			{
				// blend

				if(!CheckSize(m_pDeinterlaceTexture, ds))
				{
					m_pDeinterlaceTexture = NULL;
				}

				if(!m_pDeinterlaceTexture) 
				{
					hr = m_dev->CreateTexture(ds.cx, ds.cy, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pDeinterlaceTexture, NULL);

					if(FAILED(hr)) return;
				}

				hr = m_pDeinterlaceTexture->GetSurfaceLevel(0, &surf[2]);

				if(m_field == field) return;

				Interlace(m_pInterlaceTexture, surf[2], 2, D3DTEXF_POINT);

				dst = surf[2];
			}
		}
		else if(mode == 1) // bob
		{
			Interlace(m_pMergeTexture, surf[1], 3, D3DTEXF_LINEAR, src[1].scale.y * (field ^ m_field));

			dst = surf[1];
		}
	}

	m_pCurrentFrame = dst;
}

void GSState::Merge(FlipInfo src[2], IDirect3DSurface9* dst)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	hr = dst->GetDesc(&desc);

	hr = m_dev->SetRenderTarget(0, dst);
	hr = m_dev->SetDepthStencilSurface(NULL);

	hr = m_dev->SetTexture(0, src[0].tex);
	hr = m_dev->SetTexture(1, src[1].tex);

    hr = m_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	hr = m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

	hr = m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hr = m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	hr = m_dev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hr = m_dev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	hr = m_dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	hr = m_dev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_dev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	const float c[] = 
	{
		(float)m_regs.pBGCOLOR->B / 255, (float)m_regs.pBGCOLOR->G / 255, (float)m_regs.pBGCOLOR->R / 255, (float)m_regs.pPMODE->ALP / 255,
		(float)m_regs.pPMODE->AMOD - 0.1f, (float)m_regs.IsEnabled(0), (float)m_regs.IsEnabled(1), (float)m_regs.pPMODE->MMOD - 0.1f,
		(float)m_env.TEXA.AEM, (float)m_env.TEXA.TA0 / 255, (float)m_env.TEXA.TA1 / 255, (float)m_regs.pPMODE->SLBG - 0.1f,
	};

	hr = m_dev->SetPixelShaderConstantF(0, c, countof(c) / 4);

	hr = m_dev->SetPixelShader(m_pHLSLMerge[PS_M32]); // TODO: if m_regs.pSMODE2->INT do a field masked output

	hr = m_dev->BeginScene();

	CRect r[2];
	
	r[0] = m_regs.GetFrameRect(0);
	r[1] = m_regs.GetFrameRect(1);

	struct
	{
		float x, y, z, rhw;
		float tu1, tv1;
		float tu2, tv2;
	}
	vertices[] =
	{
		{0, 0, 0.5f, 2.0f, 
			src[0].scale.x * r[0].left / src[0].desc.Width, src[0].scale.y * r[0].top / src[0].desc.Height,
			src[1].scale.x * r[1].left / src[1].desc.Width, src[1].scale.y * r[1].top / src[1].desc.Height},
		{(float)desc.Width, 0, 0.5f, 2.0f, 
			src[0].scale.x * r[0].right / src[0].desc.Width, src[0].scale.y * r[0].top / src[0].desc.Height,
			src[1].scale.x * r[1].right / src[1].desc.Width, src[1].scale.y * r[1].top / src[1].desc.Height},
		{(float)desc.Width, (float)desc.Height, 0.5f, 2.0f, 
			src[0].scale.x * r[0].right / src[0].desc.Width, src[0].scale.y * r[0].bottom / src[0].desc.Height,
			src[1].scale.x * r[1].right / src[1].desc.Width, src[1].scale.y * r[1].bottom / src[1].desc.Height}, 
		{0, (float)desc.Height, 0.5f, 2.0f, 
			src[0].scale.x * r[0].left / src[0].desc.Width, src[0].scale.y * r[0].bottom / src[0].desc.Height,
			src[1].scale.x * r[1].left / src[1].desc.Width, src[1].scale.y * r[1].bottom / src[1].desc.Height}, 
	};

	for(int i = 0; i < countof(vertices); i++)
	{
		vertices[i].x -= 0.5f;
		vertices[i].y -= 0.5f;
	}

	hr = m_dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX2);

	hr = m_dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(vertices[0]));

	hr = m_dev->EndScene();
}

void GSState::Interlace(IDirect3DTexture9* src, IDirect3DSurface9* dst, int shader, D3DTEXTUREFILTERTYPE filter, int yoffset)
{
	HRESULT hr;

	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	hr = dst->GetDesc(&desc);

	hr = m_dev->SetRenderTarget(0, dst);
	hr = m_dev->SetDepthStencilSurface(NULL);

	hr = m_dev->SetTexture(0, src);
	
    hr = m_dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	hr = m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

	hr = m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);
	hr = m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, filter);

	hr = m_dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	const float c[] = 
	{
		(float)desc.Height, (float)m_field, 0, 0
	};

	hr = m_dev->SetPixelShaderConstantF(0, c, countof(c) / 4);

	hr = m_dev->SetPixelShader(m_pHLSLInterlace[shader]);

	hr = m_dev->BeginScene();

	struct
	{
		float x, y, z, rhw;
		float tu, tv;
	}
	vertices[] =
	{
		{0, 0, 0.5f, 2.0f, 0.0f, 0.0f},
		{(float)desc.Width, 0, 0.5f, 2.0f, 1.0f, 0.0f},
		{(float)desc.Width, (float)desc.Height, 0.5f, 2.0f, 1.0f, 1.0f},
		{0, (float)desc.Height, 0.5f, 2.0f, 0.0f, 1.0f},
	};

	for(int i = 0; i < countof(vertices); i++)
	{
		vertices[i].x -= 0.5f;
		vertices[i].y -= 0.5f + yoffset;
	}

	hr = m_dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);

	hr = m_dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(vertices[0]));

	hr = m_dev->EndScene();
}

void GSState::Present()
{
	HRESULT hr;

	CRect cr;

	GetClientRect(&cr);

	CComPtr<IDirect3DSurface9> pBackBuffer = GetBackBuffer();

	if(!pBackBuffer) return;

	D3DSURFACE_DESC desc;

	hr = pBackBuffer->GetDesc(&desc);

	if(desc.Width != cr.Width() || desc.Height != cr.Height())
	{
		m_pSwapChain = NULL;
		pBackBuffer = NULL;

		m_d3dpp.BackBufferWidth = cr.Width();
		m_d3dpp.BackBufferHeight = cr.Height();

		hr = m_dev->CreateAdditionalSwapChain(&m_d3dpp, &m_pSwapChain);

		if(FAILED(hr)) return;

		hr = m_pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	}

	hr = m_dev->SetRenderTarget(0, pBackBuffer);

	hr = m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	if(m_pCurrentFrame)
	{
		static int ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

		int arx = ar[m_nAspectRatio][0];
		int ary = ar[m_nAspectRatio][1];

		CRect r = cr;

		if(arx > 0 && ary > 0)
		{
			if(r.Width() * ary > r.Height() * arx)
			{
				int w = r.Height() * arx / ary;
				r.left = r.CenterPoint().x - w / 2;
				if(r.left & 1) r.left++;
				r.right = r.left + w;
			}
			else
			{
				int h = r.Width() * ary / arx;
				r.top = r.CenterPoint().y - h / 2;
				if(r.top & 1) r.top++;
				r.bottom = r.top + h;
			}
		}

		r &= cr;
		
		hr = m_dev->StretchRect(m_pCurrentFrame, NULL, pBackBuffer, r, D3DTEXF_LINEAR);
	}

	// osd

	static UINT64 s_frame = 0;
	static CString s_stats;

	if(m_perfmon.GetFrame() - s_frame >= 30)
	{
		m_perfmon.Update();

		s_frame = m_perfmon.GetFrame();

		double fps = 1000.0f / m_perfmon.Get(GSPerfMon::Frame);
		
		s_stats.Format(
			_T("%I64d | %.2f fps (%d%%) | %s - %s | %s | %d | %.2f | %.2f | %.2f"), 
			m_perfmon.GetFrame(), fps, (int)(100.0 * fps / m_regs.GetFPS()),
			m_regs.pSMODE2->INT ? (CString(_T("Interlaced ")) + (m_regs.pSMODE2->FFMD ? _T("(frame)") : _T("(field)"))) : _T("Progressive"),
			g_interlace[m_nInterlace].name,
			g_aspectratio[m_nAspectRatio].name,
			(int)m_perfmon.Get(GSPerfMon::Prim),
			m_perfmon.Get(GSPerfMon::Swizzle) / 1024,
			m_perfmon.Get(GSPerfMon::Unswizzle) / 1024,
			m_perfmon.Get(GSPerfMon::Texture) / 1024
			);

		if(m_osd && m_d3dpp.Windowed)
		{
			SetWindowText(s_stats);
		}
	}

	if(m_osd && !m_d3dpp.Windowed)
	{
		hr = m_dev->BeginScene();

		hr = m_dev->SetRenderTarget(0, pBackBuffer);
		hr = m_dev->SetDepthStencilSurface(NULL);

		CRect r;
		
		GetClientRect(r);

		D3DCOLOR c = D3DCOLOR_ARGB(255, 0, 255, 0);

		CString str = s_stats;

		str += _T("\n\nF5: interlace mode\nF6: aspect ratio\nF7: OSD");

		if(m_pD3DXFont->DrawText(NULL, str, -1, &r, DT_CALCRECT|DT_LEFT|DT_WORDBREAK, c))
		{
			m_pD3DXFont->DrawText(NULL, str, -1, &r, DT_LEFT|DT_WORDBREAK, c);
		}

		hr = m_dev->EndScene();
	}

	if(m_pSwapChain)
	{
		hr = m_pSwapChain->Present(NULL, NULL, NULL, NULL, 0);
	}
	else
	{
		hr = m_dev->Present(NULL, NULL, NULL, NULL);
	}
}

void GSState::Flush()
{
	FlushWriteTransfer();

	FlushPrim();
}

