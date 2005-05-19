/* 
 *	Copyright (C) 2003-2004 Gabest
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
#include "GSUtil.h"
#include "resource.h"

//
// GSState
//

GSState::GSState(int w, int h, HWND hWnd, HRESULT& hr) 
	: m_hWnd(hWnd)
	, m_width(w)
	, m_height(h)
	, m_PRIM(8)
	, m_ctxt(NULL)
	, m_sfp(NULL)
	, m_fp(NULL)
	, m_iOSD(1)
{
	hr = E_FAIL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWinApp* pApp = AfxGetApp();

	m_v.RGBAQ.Q = m_q = 1.0f;

	memset(&m_tag, 0, sizeof(m_tag));
	m_nreg = 0;

	m_pTrBuff = (BYTE*)_aligned_malloc(1024*1024*4, 16);
	m_nTrBytes = 0;

	m_pCSRr = &m_rs.CSRr;
	m_fpGSirq = NULL;

	m_ctxt = &m_de.CTXT[0];

	for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;

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
		m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNull;

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

	// DD

	CComPtr<IDirectDraw7> pDD; 
	if(FAILED(DirectDrawCreateEx(0, (void**)&pDD, IID_IDirectDraw7, 0)))
		return;

	m_ddcaps.dwSize = sizeof(DDCAPS); 
	if(FAILED(pDD->GetCaps(&m_ddcaps, NULL)))
		return;

	pDD = NULL;

	// D3D

	if(!(m_pD3D = Direct3DCreate9(D3D_SDK_VERSION))
	&& !(m_pD3D = Direct3DCreate9(D3D9b_SDK_VERSION)))
		return;

	ZeroMemory(&m_caps, sizeof(m_caps));
	m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_caps);

	m_fmtDepthStencil = 
		IsDepthFormatOk(m_pD3D, D3DFMT_D32, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8) ? D3DFMT_D32 :
		IsDepthFormatOk(m_pD3D, D3DFMT_D24X8, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8) ? D3DFMT_D24X8 :
		D3DFMT_D16;

	// D3D Device

	if(FAILED(ResetDevice(true)))
		return;

	// Shaders

	DWORD PixelShaderVersion = pApp->GetProfileInt(_T("Settings"), _T("PixelShaderVersion2"), D3DPS_VERSION(2, 0));

	if(PixelShaderVersion > m_caps.PixelShaderVersion)
	{
		CString str;
		str.Format(_T("Supported pixel shader version is too low!\n\nSupported: %d.%d\nSelected: %d.%d"),
			D3DSHADER_VERSION_MAJOR(m_caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(m_caps.PixelShaderVersion),
			D3DSHADER_VERSION_MAJOR(PixelShaderVersion), D3DSHADER_VERSION_MINOR(PixelShaderVersion));
		AfxMessageBox(str);
		m_pD3DDev = NULL;
		return;
	}

	m_caps.PixelShaderVersion = min(PixelShaderVersion, m_caps.PixelShaderVersion);

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
		_T("main_8PTo32"),
	};

	// ps_3_0

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
	{
		for(int i = 0; i < countof(hlsl_tfx); i++)
		{
			if(m_pHLSLTFX[i]) continue;

			CompileShaderFromResource(m_pD3DDev, IDR_HLSL_TFX, hlsl_tfx[i], _T("ps_3_0"),
				D3DXSHADER_AVOID_FLOW_CONTROL|D3DXSHADER_PARTIALPRECISION, &m_pHLSLTFX[i]);
		}

		for(int i = 0; i < 3; i++)
		{
			if(m_pHLSLMerge[i]) continue;

			CString main;
			main.Format(_T("main%d"), i);
			CompileShaderFromResource(m_pD3DDev, IDR_HLSL_MERGE, main, _T("ps_3_0"), 
				D3DXSHADER_AVOID_FLOW_CONTROL|D3DXSHADER_PARTIALPRECISION, &m_pHLSLMerge[i]);
		}
	}

	// ps_2_0

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
	{
		for(int i = 0; i < countof(hlsl_tfx); i++)
		{
			if(m_pHLSLTFX[i]) continue;

			CompileShaderFromResource(m_pD3DDev, IDR_HLSL_TFX, hlsl_tfx[i], _T("ps_2_0"), 
				D3DXSHADER_PARTIALPRECISION, &m_pHLSLTFX[i]);
		}

		for(int i = 0; i < 3; i++)
		{
			if(m_pHLSLMerge[i]) continue;

			CString main;
			main.Format(_T("main%d"), i);
			CompileShaderFromResource(m_pD3DDev, IDR_HLSL_MERGE, main, _T("ps_2_0"), 
				D3DXSHADER_PARTIALPRECISION, &m_pHLSLMerge[i]);
		}
	}

	// ps_1_1 + ps_1_4

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
	{
		static const UINT nShaderIDs[] = 
		{
			IDR_PS11_TFX000, IDR_PS11_TFX010, IDR_PS11_TFX011, 
			IDR_PS11_TFX1x0, IDR_PS11_TFX1x1,
			IDR_PS11_TFX200, IDR_PS11_TFX210, IDR_PS11_TFX211,
			IDR_PS11_TFX300, IDR_PS11_TFX310, IDR_PS11_TFX311,
			IDR_PS11_TFX4xx,
			IDR_PS11_EN11, IDR_PS11_EN01, IDR_PS11_EN10, IDR_PS11_EN00,
			IDR_PS14_EN11, IDR_PS14_EN01, IDR_PS14_EN10, IDR_PS14_EN00
		};

		for(int i = 0; i < countof(nShaderIDs); i++)
		{
			if(m_pPixelShaders[i]) continue;
			AssembleShaderFromResource(m_pD3DDev, nShaderIDs[i], 0, &m_pPixelShaders[i]);
		}

		if(!m_pHLSLRedBlue)
		{
			CompileShaderFromResource(m_pD3DDev, IDR_HLSL_RB, _T("main"), _T("ps_1_1"), 
				D3DXSHADER_PARTIALPRECISION, &m_pHLSLRedBlue);
		}
	}

	//

	m_strDefaultTitle.ReleaseBufferSetLength(GetWindowText(m_hWnd, m_strDefaultTitle.GetBuffer(256), 256));

	m_fEnablePalettizedTextures = !!pApp->GetProfileInt(_T("Settings"), _T("fEnablePalettizedTextures"), FALSE);

	hr = S_OK;

	Reset();

#if defined(DEBUG_LOG) || defined(DEBUG_LOG2)
	::DeleteFile(_T("g:\\gs.txt"));
	m_fp = _tfopen(_T("g:\\gs.txt"), _T("at"));
#endif

//	m_pCSRr->REV = 0x20;
}

GSState::~GSState()
{
	Reset();
	_aligned_free(m_pTrBuff);
	if(m_sfp) fclose(m_sfp);
	if(m_fp) fclose(m_fp);
}

HRESULT GSState::ResetDevice(bool fForceWindowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HMODULE hModule = AfxGetResourceHandle();
    CWinApp* pApp = AfxGetApp();

	HRESULT hr;

	if(!m_pD3D)
		return E_FAIL;

	m_pOrgRenderTarget = NULL;
	m_pTmpRenderTarget = NULL;
	m_pD3DXFont = NULL;

	ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
	m_d3dpp.Windowed = TRUE;
	m_d3dpp.hDeviceWindow = m_hWnd;
	m_d3dpp.SwapEffect = /*D3DSWAPEFFECT_COPY*/D3DSWAPEFFECT_DISCARD/*D3DSWAPEFFECT_FLIP*/;
	m_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_d3dpp.BackBufferWidth = m_width;
	m_d3dpp.BackBufferHeight = m_height;
	//m_d3dpp.BackBufferCount = 3;
	m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if(!!pApp->GetProfileInt(_T("Settings"), _T("fEnableTvOut"), FALSE))
		m_d3dpp.Flags |= D3DPRESENTFLAG_VIDEO;

	int ModeWidth = pApp->GetProfileInt(_T("Settings"), _T("ModeWidth"), 0);
	int ModeHeight = pApp->GetProfileInt(_T("Settings"), _T("ModeHeight"), 0);
	int ModeRefreshRate = pApp->GetProfileInt(_T("Settings"), _T("ModeRefreshRate"), 0);

	if(!fForceWindowed && ModeWidth > 0 && ModeHeight > 0 && ModeRefreshRate >= 0)
	{
		m_d3dpp.Windowed = FALSE;
		m_d3dpp.BackBufferWidth = ModeWidth;
		m_d3dpp.BackBufferHeight = ModeHeight;
		m_d3dpp.FullScreen_RefreshRateInHz = ModeRefreshRate;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

		::SetWindowLong(m_hWnd, GWL_STYLE, ::GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME));
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		::SetMenu(m_hWnd, NULL);

		m_iOSD = 0;
	}

	if(!m_pD3DDev)
	{
		if(FAILED(hr = m_pD3D->CreateDevice(
			// m_pD3D->GetAdapterCount()-1, D3DDEVTYPE_REF,
			D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
			m_hWnd,
			m_caps.VertexProcessingCaps ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
			&m_d3dpp, &m_pD3DDev)))
			return hr;
	}
	else
	{
		if(FAILED(hr = m_pD3DDev->Reset(&m_d3dpp)))
		{
			if(D3DERR_DEVICELOST == hr)
			{
				Sleep(1000);
				if(FAILED(hr = m_pD3DDev->Reset(&m_d3dpp)))
					return hr;
			}
			else
			{
				return hr;
			}
		}
	}

	CComPtr<IDirect3DSurface9> pBackBuff;
	hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuff);

	ZeroMemory(&m_bd, sizeof(m_bd));
	pBackBuff->GetDesc(&m_bd);

	hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    hr = m_pD3DDev->GetRenderTarget(0, &m_pOrgRenderTarget);

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(1, 1) && m_caps.PixelShaderVersion <= D3DPS_VERSION(1, 3)
		&& m_pHLSLRedBlue)
	{
		hr = m_pD3DDev->CreateTexture(
			m_bd.Width, m_bd.Height, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, 
			D3DPOOL_DEFAULT, &m_pTmpRenderTarget, NULL);
	}

	D3DXFONT_DESC fd;
	memset(&fd, 0, sizeof(fd));
	_tcscpy(fd.FaceName, _T("Arial"));
	fd.Height = -(int)(sqrt((float)m_bd.Height) * 0.7);
	hr = D3DXCreateFontIndirect(m_pD3DDev, &fd, &m_pD3DXFont);

    hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);

	m_texfilter = (D3DTEXTUREFILTERTYPE)pApp->GetProfileInt(_T("Settings"), _T("TexFilter"), D3DTEXF_LINEAR);

	for(int i = 0; i < 8; i++)
	{
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, m_texfilter);
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, m_texfilter);
		// hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, m_texfilter);
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hr = m_pD3DDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
	hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);

	return S_OK;
}

UINT32 GSState::Freeze(freezeData* fd, bool fSizeOnly)
{
	int size = sizeof(m_version)
		+ sizeof(m_de) + sizeof(m_rs) + sizeof(m_v) 
		+ sizeof(m_x) + sizeof(m_y) + 1024*1024*4
		+ sizeof(m_tag) + sizeof(m_nreg)
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

	FlushWriteTransfer();

	FlushPrimInternal();

	BYTE* data = fd->data;
	memcpy(data, &m_version, sizeof(m_version)); data += sizeof(m_version);
	memcpy(data, &m_de, sizeof(m_de)); data += sizeof(m_de);
	memcpy(data, &m_rs, sizeof(m_rs)); data += sizeof(m_rs);
	memcpy(data, &m_v, sizeof(m_v)); data += sizeof(m_v);
	memcpy(data, &m_x, sizeof(m_x)); data += sizeof(m_x);
	memcpy(data, &m_y, sizeof(m_y)); data += sizeof(m_y);
	memcpy(data, m_lm.GetVM(), 1024*1024*4); data += 1024*1024*4;
	memcpy(data, &m_tag, sizeof(m_tag)); data += sizeof(m_tag);
	memcpy(data, &m_nreg, sizeof(m_nreg)); data += sizeof(m_nreg);
	// memcpy(data, &m_vl, sizeof(m_vl)); data += sizeof(m_vl);

	return 0;
}

UINT32 GSState::Defrost(const freezeData* fd)
{
	if(!fd || !fd->data || fd->size == 0) 
		return -1;

	int size = sizeof(m_version)
		+ sizeof(m_de) + sizeof(m_rs) + sizeof(m_v) 
		+ sizeof(m_x) + sizeof(m_y) + 1024*1024*4
		+ sizeof(m_tag) + sizeof(m_nreg)
		/*+ sizeof(m_vl)*/;

	if(fd->size != size) 
		return -1;

	BYTE* data = fd->data;

	int version = 0;
	memcpy(&version, data, sizeof(version)); data += sizeof(version);
	if(m_version != version) return -1;

	FlushPrimInternal();

	memcpy(&m_de, data, sizeof(m_de)); data += sizeof(m_de);
	memcpy(&m_rs, data, sizeof(m_rs)); data += sizeof(m_rs);
	memcpy(&m_v, data, sizeof(m_v)); data += sizeof(m_v);
	memcpy(&m_x, data, sizeof(m_x)); data += sizeof(m_x);
	memcpy(&m_y, data, sizeof(m_y)); data += sizeof(m_y);
	memcpy(m_lm.GetVM(), data, 1024*1024*4); data += 1024*1024*4;
	memcpy(&m_tag, data, sizeof(m_tag)); data += sizeof(m_tag);
	memcpy(&m_nreg, data, sizeof(m_nreg)); data += sizeof(m_nreg);
	// memcpy(&m_vl, data, sizeof(m_vl)); data += sizeof(m_vl);

	m_de.pPRIM = !m_de.PRMODECONT.AC ? (GIFRegPRIM*)&m_de.PRMODE : &m_de.PRIM;
	m_ctxt = &m_de.CTXT[m_de.pPRIM->CTXT];

	return 0;
}

void GSState::Write(GS_REG mem, GSReg* r, UINT64 mask)
{
	ASSERT(r);

	GSPerfMonAutoTimer at(m_perfmon);

#ifdef ENABLE_CAPTURE_STATE
	if(m_sfp)
	{
		fputc(ST_WRITE, m_sfp);
		fwrite(&mem, 4, 1, m_sfp);
		fwrite(r, 8, 1, m_sfp);
		fwrite(&mask, 8, 1, m_sfp);
	}
#endif

	#define AssignReg(reg) m_rs.##reg##.i64 = (r->i64 & mask) | (m_rs.##reg##.i64 & ~mask);

	switch(mem)
	{
		case GS_PMODE:
			AssignReg(PMODE);
			LOG(_T("Write(GS_PMODE, EN1=%x EN2=%x CRTMD=%x MMOD=%x AMOD=%x SLBG=%x ALP=%x)\n"), 
				r->PMODE.EN1,
				r->PMODE.EN2,
				r->PMODE.CRTMD,
				r->PMODE.MMOD,
				r->PMODE.AMOD,
				r->PMODE.SLBG,
				r->PMODE.ALP);
			break;

		case GS_SMODE1:
			AssignReg(SMODE1);
			LOG(_T("Write(GS_SMODE1, CMOD=%x)\n"), 
				r->SMODE1.CMOD);
			break;

		case GS_SMODE2:
			AssignReg(SMODE2);
			LOG(_T("Write(GS_SMODE2, INT=%x FFMD=%x DPMS=%x)\n"), 
				r->SMODE2.INT,
				r->SMODE2.FFMD,
				r->SMODE2.DPMS);
			break;

		case GS_SRFSH:
			LOG(_T("Write(GS_SRFSH, %016I64x)\n"), r->i64);
			break;

		case GS_SYNCH1:
			LOG(_T("Write(GS_SYNCH1, %016I64x)\n"), r->i64);
			break;

		case GS_SYNCH2:
			LOG(_T("Write(GS_SYNCH2, %016I64x)\n"), r->i64);
			break;

		case GS_SYNCV:
			LOG(_T("Write(GS_SYNCV, %016I64x)\n"), r->i64);
			break;

		case GS_DISPFB1:
			AssignReg(DISPFB[0]);
			LOG(_T("Write(GS_DISPFB1, FBP=%x FBW=%d PSM=%x DBX=%x DBY=%x)\n"), 
				r->DISPFB.FBP<<5,
				r->DISPFB.FBW*64,
				r->DISPFB.PSM,
				r->DISPFB.DBX,
				r->DISPFB.DBY);
			break;

		case GS_DISPLAY1:
			AssignReg(DISPLAY[0]);
			LOG(_T("Write(GS_DISPLAY1, DX=%x DY=%x MAGH=%x MAGV=%x DW=%x DH=%x)\n"),
				r->DISPLAY.DX,
				r->DISPLAY.DY,
				r->DISPLAY.MAGH,
				r->DISPLAY.MAGV,
				r->DISPLAY.DW,
				r->DISPLAY.DH);
			break;

		case GS_DISPFB2:
			AssignReg(DISPFB[1]);
			LOG(_T("Write(GS_DISPFB2, FBP=%x FBW=%d PSM=%x DBX=%x DBY=%x)\n"), 
				r->DISPFB.FBP<<5,
				r->DISPFB.FBW*64,
				r->DISPFB.PSM,
				r->DISPFB.DBX,
				r->DISPFB.DBY);
			break;

		case GS_DISPLAY2:
			AssignReg(DISPLAY[1]);
			LOG(_T("Write(GS_DISPLAY2, DX=%x DY=%x MAGH=%x MAGV=%x DW=%x DH=%x)\n"),
				r->DISPLAY.DX,
				r->DISPLAY.DY,
				r->DISPLAY.MAGH,
				r->DISPLAY.MAGV,
				r->DISPLAY.DW,
				r->DISPLAY.DH);
			break;

		case GS_EXTBUF:
			AssignReg(EXTBUF);
			LOG(_T("Write(GS_EXTBUF, EXBP=%x EXBW=%x FBIN=%x WFFMD=%x EMODA=%x EMODC=%x WDX=%x WDY=%x)\n"),
				r->EXTBUF.EXBP,
				r->EXTBUF.EXBW,
				r->EXTBUF.FBIN,
				r->EXTBUF.WFFMD,
				r->EXTBUF.EMODA,
				r->EXTBUF.EMODC,
				r->EXTBUF.WDX,
				r->EXTBUF.WDY);
			break;

		case GS_EXTDATA:
			AssignReg(EXTDATA);
			LOG(_T("Write(GS_EXTDATA, SX=%x SY=%x SMPH=%x SMPV=%x WW=%x WH=%x)\n"), 
				r->EXTDATA.SX,
				r->EXTDATA.SY,
				r->EXTDATA.SMPH,
				r->EXTDATA.SMPV,
				r->EXTDATA.WW,
				r->EXTDATA.WH);
			break;

		case GS_EXTWRITE:
			AssignReg(EXTWRITE);
			LOG(_T("Write(GS_EXTWRITE, WRITE=%x)\n"),
				r->EXTWRITE.WRITE);
			break;

		case GS_BGCOLOR:
			AssignReg(BGCOLOR);
			LOG(_T("Write(GS_BGCOLOR, R=%x G=%x B=%x)\n"),
				r->BGCOLOR.R,
				r->BGCOLOR.G,
				r->BGCOLOR.B);
			break;

		case GS_CSR:
			AssignReg(CSRw);
			LOG(_T("Write(GS_CSR, SIGNAL=%x FINISH=%x HSINT=%x VSINT=%x EDWINT=%x ZERO1=%x ZERO2=%x FLUSH=%x RESET=%x NFIELD=%x FIELD=%x FIFO=%x REV=%x ID=%x)\n"),
				r->CSR.SIGNAL,
				r->CSR.FINISH,
				r->CSR.HSINT,
				r->CSR.VSINT,
				r->CSR.EDWINT,
				r->CSR.ZERO1,
				r->CSR.ZERO2,
				r->CSR.FLUSH,
				r->CSR.RESET,
				r->CSR.NFIELD,
				r->CSR.FIELD,
				r->CSR.FIFO,
				r->CSR.REV,
				r->CSR.ID);
			if(m_rs.CSRw.SIGNAL) m_pCSRr->SIGNAL = 0;
			if(m_rs.CSRw.FINISH) m_pCSRr->FINISH = 0;
			if(m_rs.CSRw.RESET) Reset();
			break;

		case GS_IMR:
			AssignReg(IMR);
			LOG(_T("Write(GS_IMR, _PAD1=%x SIGMSK=%x FINISHMSK=%x HSMSK=%x VSMSK=%x EDWMSK=%x)\n"),
				r->IMR._PAD1,
				r->IMR.SIGMSK,
				r->IMR.FINISHMSK,
				r->IMR.HSMSK,
				r->IMR.VSMSK,
				r->IMR.EDWMSK);
			break;

		case GS_BUSDIR:
			AssignReg(BUSDIR);
			LOG(_T("Write(GS_BUSDIR, DIR=%x)\n"),
				r->BUSDIR.DIR);
			break;

		case GS_SIGLBLID:
			AssignReg(SIGLBLID);
			LOG(_T("Write(GS_SIGLBLID, SIGID=%x LBLID=%x)\n"),
				r->SIGLBLID.SIGID,
				r->SIGLBLID.LBLID);
			break;

		default:
			LOG(_T("*** WARNING *** Write(?????????, %016I64x)\n"), r->i64);
			ASSERT(0);
			break;
	}
}

UINT64 GSState::Read(GS_REG mem)
{
	if(mem == GS_CSR)
		return m_pCSRr->i64;

	GSPerfMonAutoTimer at(m_perfmon);

	GSReg* r = NULL;

	switch(mem)
	{
		case GS_CSR:
			r = reinterpret_cast<GSReg*>(m_pCSRr);
			LOG(_T("Read(GS_CSR, SIGNAL=%x FINISH=%x HSINT=%x VSINT=%x EDWINT=%x ZERO1=%x ZERO2=%x FLUSH=%x RESET=%x NFIELD=%x FIELD=%x FIFO=%x REV=%x ID=%x)\n"),
				r->CSR.SIGNAL,
				r->CSR.FINISH,
				r->CSR.HSINT,
				r->CSR.VSINT,
				r->CSR.EDWINT,
				r->CSR.ZERO1,
				r->CSR.ZERO2,
				r->CSR.FLUSH,
				r->CSR.RESET,
				r->CSR.NFIELD,
				r->CSR.FIELD,
				r->CSR.FIFO,
				r->CSR.REV,
				r->CSR.ID);
			break;

		case GS_SIGLBLID:
			r = reinterpret_cast<GSReg*>(&m_rs.SIGLBLID);
			LOG(_T("Read(GS_SIGLBLID, SIGID=%x LBLID=%x)\n"),
				r->SIGLBLID.SIGID,
				r->SIGLBLID.LBLID);
			break;

		case GS_UNKNOWN:
			LOG(_T("*** WARNING *** Read(%08x)\n"), mem);
			return m_pCSRr->FIELD << 13;
			break;

		default:
			LOG(_T("*** WARNING *** Read(%08x)\n"), mem);
			ASSERT(0);
			break;
	}

	return r ? r->i64 : 0;
}

void GSState::ReadFIFO(BYTE* pMem)
{
	GSPerfMonAutoTimer at(m_perfmon);

	FlushWriteTransfer();

	LOG(_T("*** WARNING *** ReadFIFO(%08x)\n"), pMem);
	ReadTransfer(pMem, 16);
}

void GSState::Transfer1(BYTE* pMem, UINT32 addr)
{
//	GSPerfMonAutoTimer at(m_perfmon);

	LOG(_T("Transfer1(%08x, %d)\n"), pMem, addr);

	// TODO: this is too cheap...
	static BYTE buff[0x4000];
	addr &= 0x3fff;
	memcpy(buff, pMem + addr, 0x4000 - addr);
	memcpy(buff + 0x4000 - addr, pMem, addr);
	Transfer(buff, -1);
}

void GSState::Transfer(BYTE* pMem)
{
	Transfer(pMem, -1);
}

void GSState::Transfer(BYTE* pMem, UINT32 size)
{
	GSPerfMonAutoTimer at(m_perfmon);

	BYTE* pMemOrg = pMem;
	UINT32 sizeOrg = size;

	while(size > 0)
	{
		LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);

		bool fEOP = false;

		if(m_tag.NLOOP == 0)
		{
			m_tag = *(GIFTag*)pMem;
			m_nreg = 0;
/*
			LOG(_T("GIFTag NLOOP=%x EOP=%x PRE=%x PRIM=%x FLG=%x NREG=%x REGS=%x\n"), 
				m_tag.NLOOP,
				m_tag.EOP,
				m_tag.PRE,
				m_tag.PRIM,
				m_tag.FLG,
				m_tag.NREG,
				m_tag.REGS);
*/
			pMem += sizeof(GIFTag);
			size--;

			if(m_tag.PRE)
			{
				LOG(_T("PRE "));
				GIFReg r;
				r.i64 = m_tag.PRIM;
				(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r);
			}

			if(m_tag.EOP)
			{
				LOG(_T("EOP\n"));
				fEOP = true;
			}
			else if(m_tag.NLOOP == 0)
			{
				LOG(_T("*** WARNING *** m_tag.NLOOP == 0 && EOP == 0\n"));
				fEOP = true;
				// ASSERT(0);
			}
		}

		switch(m_tag.FLG)
		{
		case GIF_FLG_PACKED:
			for(GIFPackedReg* r = (GIFPackedReg*)pMem; m_tag.NLOOP > 0 && size > 0; r++, size--, pMem += sizeof(GIFPackedReg))
			{
				DWORD reg = (m_tag.ai32[2 + (m_nreg >> 3)] >> ((m_nreg & 7) << 2)) & 0xf;
				// BYTE reg = GET_GIF_REG(m_tag, m_nreg);
				(this->*m_fpGIFPackedRegHandlers[reg])(r);
				if((m_nreg=(m_nreg+1)&0xf) == m_tag.NREG) {m_nreg = 0; m_tag.NLOOP--;}
			}
			break;
		case GIF_FLG_REGLIST:
			size *= 2;
			for(GIFReg* r = (GIFReg*)pMem; m_tag.NLOOP > 0 && size > 0; r++, size--, pMem += sizeof(GIFReg))
			{
				DWORD reg = (m_tag.ai32[2 + (m_nreg >> 3)] >> ((m_nreg & 7) << 2)) & 0xf;
				// BYTE reg = GET_GIF_REG(m_tag, m_nreg);
				(this->*m_fpGIFRegHandlers[reg])(r);
				if((m_nreg=(m_nreg+1)&0xf) == m_tag.NREG) {m_nreg = 0; m_tag.NLOOP--;}
			}
			if(size&1) pMem += sizeof(GIFReg);
			size /= 2;
			break;
		case GIF_FLG_IMAGE2:
			LOG(_T("*** WARNING **** Unexpected GIFTag flag\n"));
m_tag.NLOOP = 0;
break;
			ASSERT(0);
		case GIF_FLG_IMAGE:
			{
				int len = min(size, m_tag.NLOOP);
				//ASSERT(!(len&3));
				switch(m_rs.TRXDIR.XDIR)
				{
				case 0:
					WriteTransfer(pMem, len*16);
					break;
				case 1: 
					ReadTransfer(pMem, len*16);
					break;
				case 2: 
					MoveTransfer();
					break;
				case 3: 
					ASSERT(0);
					break;
				}
				pMem += len*16;
				m_tag.NLOOP -= len;
				size -= len;
			}
			break;
		}

		LOG(_T("Transfer(%08x, %d) END\n"), pMem, size);

		if(fEOP && (INT32)size <= 0)
		{
			break;
		}
	}

#ifdef ENABLE_CAPTURE_STATE
	if(m_sfp)
	{
		fputc(ST_TRANSFER, m_sfp);
		fwrite(&sizeOrg, 4, 1, m_sfp);
		UINT32 len = (UINT32)(pMem - pMemOrg);
		fwrite(&len, 4, 1, m_sfp);
		fwrite(pMemOrg, len, 1, m_sfp);
	}
#endif
}

UINT32 GSState::MakeSnapshot(char* path)
{
	GSPerfMonAutoTimer at(m_perfmon);

	CString fn;
	fn.Format(_T("%sgsdx9_%s.bmp"), CString(path), CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
	return D3DXSaveSurfaceToFile(fn, D3DXIFF_BMP, m_pOrgRenderTarget, NULL, NULL);
}

void GSState::Capture()
{
	GSPerfMonAutoTimer at(m_perfmon);

	if(!m_capture.IsCapturing()) m_capture.BeginCapture(m_pD3DDev, m_rs.GetFPS());
	else m_capture.EndCapture();
}

void GSState::ToggleOSD()
{
	if(m_d3dpp.Windowed)
	{
		if(m_iOSD == 1) SetWindowText(m_hWnd, m_strDefaultTitle);
		m_iOSD = ++m_iOSD % 3;
	}
	else
	{
		m_iOSD = m_iOSD ? 0 : 2;
	}
}

void GSState::CaptureState(CString fn)
{
#ifdef ENABLE_CAPTURE_STATE
	if(!m_sfp) m_sfp = !fn.IsEmpty() ? _tfopen(fn, _T("wb")) : NULL;
#endif
}

void GSState::VSync()
{
	GSPerfMonAutoTimer at(m_perfmon);

	LOG(_T("VSync(%d)\n"), m_perfmon.GetFrame());

#ifdef ENABLE_CAPTURE_STATE
	if(m_sfp) fputc(ST_VSYNC, m_sfp);
#endif

	FlushPrimInternal();

	m_pCSRr->NFIELD = 1; // ?
	if(m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/);
		m_pCSRr->FIELD = 1 - m_pCSRr->FIELD;

	Flip();

	EndFrame();
}

void GSState::Reset()
{
	GSPerfMonAutoTimer at(m_perfmon);

	memset(&m_de, 0, sizeof(m_de));
	memset(&m_rs, 0, sizeof(m_rs));
	memset(&m_tag, 0, sizeof(m_tag));
	memset(&m_v, 0, sizeof(m_v));
	m_nreg = 0;

	m_de.PRMODECONT.AC = 1;
	m_de.pPRIM = &m_de.PRIM;
	m_ctxt = &m_de.CTXT[0];

	m_PRIM = 8;

	if(m_pD3DDev) m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET/*|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL*/, 0, 1.0f, 0);
}

void GSState::FinishFlip(FlipSrc rt[2], bool fShiftField)
{
	HRESULT hr;

	bool fEN[2];
	for(int i = 0; i < countof(fEN); i++)
	{
		fEN[i] = m_rs.IsEnabled(i) && rt[i].pRT;
		if(!fEN[i]) rt[i].rd.Width = rt[i].rd.Height = 1; // to avoid div by zero below
	}

	CRect dst(0, 0, m_bd.Width, m_bd.Height);

	struct
	{
		float x, y, z, rhw;
		float tu1, tv1;
		float tu2, tv2;
	}
	pVertices[] =
	{
		{(float)dst.left, (float)dst.top, 0.5f, 2.0f, 
			(float)rt[0].src.left / rt[0].rd.Width, (float)rt[0].src.top / rt[0].rd.Height, 
			(float)rt[1].src.left / rt[1].rd.Width, (float)rt[1].src.top / rt[1].rd.Height},
		{(float)dst.right, (float)dst.top, 0.5f, 2.0f, 
			(float)rt[0].src.right / rt[0].rd.Width, (float)rt[0].src.top / rt[0].rd.Height, 
			(float)rt[1].src.right / rt[1].rd.Width, (float)rt[1].src.top / rt[1].rd.Height},
		{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, 
			(float)rt[0].src.left / rt[0].rd.Width, (float)rt[0].src.bottom / rt[0].rd.Height, 
			(float)rt[1].src.left / rt[1].rd.Width, (float)rt[1].src.bottom / rt[1].rd.Height},
		{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, 
			(float)rt[0].src.right / rt[0].rd.Width, (float)rt[0].src.bottom / rt[0].rd.Height, 
			(float)rt[1].src.right / rt[1].rd.Width, (float)rt[1].src.bottom / rt[1].rd.Height},
	};

	for(int i = 0; i < countof(pVertices); i++)
	{
		pVertices[i].x -= 0.5;
		pVertices[i].y -= 0.5;

		if(fShiftField)
		{
			pVertices[i].tv1 += rt[0].scale.y*0.5f / rt[0].rd.Height;
			pVertices[i].tv2 += rt[1].scale.y*0.5f / rt[1].rd.Height;
		}
/**/
	}

	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
    hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	hr = m_pD3DDev->SetTexture(0, rt[0].pRT);
	hr = m_pD3DDev->SetTexture(1, rt[1].pRT);

	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	IDirect3DPixelShader9* pPixelShader = NULL;

	if(!pPixelShader && m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) && m_pHLSLMerge[PS_M32])
	{
		pPixelShader = m_pHLSLMerge[PS_M32];
	}

	if(!pPixelShader && m_caps.PixelShaderVersion >= D3DPS_VERSION(1, 4))
	{
		if(fEN[0] && fEN[1]) // RAO1 + RAO2
		{
			pPixelShader = m_pPixelShaders[PS14_EN11];
		}
		else if(fEN[0]) // RAO1
		{
			pPixelShader = m_pPixelShaders[PS14_EN10];
		}
		else if(fEN[1]) // RAO2
		{
			pPixelShader = m_pPixelShaders[PS14_EN01];
		}
	}

	if(!pPixelShader && m_caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
	{
		if(fEN[0] && fEN[1]) // RAO1 + RAO2
		{
			pPixelShader = m_pPixelShaders[PS11_EN11];
		}
		else if(fEN[0]) // RAO1
		{
			pPixelShader = m_pPixelShaders[PS11_EN10];
		}
		else if(fEN[1]) // RAO2
		{
			pPixelShader = m_pPixelShaders[PS11_EN01];
		}
	}

	if(!pPixelShader)
	{
		int stage = 0;

		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		stage++;

		if(fEN[0] && fEN[1]) // RAO1 + RAO2
		{
			if(m_rs.PMODE.ALP < 0xff)
			{
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_LERP);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, m_rs.PMODE.SLBG ? D3DTA_CONSTANT : D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG0, D3DTA_ALPHAREPLICATE|(m_rs.PMODE.MMOD ? D3DTA_CONSTANT : D3DTA_TEXTURE));
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_CONSTANT, D3DCOLOR_ARGB(m_rs.PMODE.ALP, m_rs.BGCOLOR.R, m_rs.BGCOLOR.G, m_rs.BGCOLOR.B));
				stage++;
			}
		}
		else if(fEN[0]) // RAO1
		{
			if(m_rs.PMODE.ALP < 0xff)
			{
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_ALPHAREPLICATE|(m_rs.PMODE.MMOD ? D3DTA_CONSTANT : D3DTA_TEXTURE));
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_CONSTANT, D3DCOLOR_ARGB(m_rs.PMODE.ALP, 0, 0, 0));
				stage++;
			}
		}
		else if(fEN[1]) // RAO2
		{
			hr = m_pD3DDev->SetTexture(0, rt[1].pRT);
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

			// FIXME
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
			for(int i = 0; i < countof(pVertices); i++)
			{
				pVertices[i].tu1 = pVertices[i].tu2;
				pVertices[i].tv1 = pVertices[i].tv2;
			}
		}

		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	if(pPixelShader)
	{
		const float c[] = 
		{
			(float)m_rs.BGCOLOR.B / 255, (float)m_rs.BGCOLOR.G / 255, (float)m_rs.BGCOLOR.R / 255, (float)m_rs.PMODE.ALP / 255,
			(float)m_rs.PMODE.AMOD, (float)m_rs.IsEnabled(0), (float)m_rs.IsEnabled(1), (float)m_rs.PMODE.MMOD,
			(float)m_de.TEXA.AEM, (float)m_de.TEXA.TA0 / 255, (float)m_de.TEXA.TA1 / 255, (float)m_rs.PMODE.SLBG
		};

		hr = m_pD3DDev->SetPixelShaderConstantF(0, c, countof(c)/4);
	}

	if(fEN[0] || fEN[1])
	{
		if(m_pTmpRenderTarget && m_pHLSLRedBlue)
		{
			CComPtr<IDirect3DSurface9> pSurf;
			hr = m_pTmpRenderTarget->GetSurfaceLevel(0, &pSurf);
			hr = m_pD3DDev->SetRenderTarget(0, pSurf);
		}
		else
		{
			hr = m_pD3DDev->SetRenderTarget(0, m_pOrgRenderTarget);
		}

		hr = m_pD3DDev->BeginScene();

		hr = m_pD3DDev->SetPixelShader(pPixelShader);
		hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX2);
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		int w, h;
		CComPtr<IDirect3DSurface9> pRTSurf;

		if(m_capture.BeginFrame(w, h, &pRTSurf))
		{
			pVertices[0].x = pVertices[0].y = pVertices[2].x = pVertices[1].y = 0;
			pVertices[1].x = pVertices[3].x = (float)w;
			pVertices[2].y = pVertices[3].y = (float)h;
			for(int i = 0; i < countof(pVertices); i++) {pVertices[i].x -= 0.5; pVertices[i].y -= 0.5;}
			hr = m_pD3DDev->SetRenderTarget(0, pRTSurf);
			hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
			m_capture.EndFrame();
		}

		hr = m_pD3DDev->EndScene();

		if(m_pTmpRenderTarget && m_pHLSLRedBlue)
		{
			hr = m_pD3DDev->SetRenderTarget(0, m_pOrgRenderTarget);
			hr = m_pD3DDev->SetTexture(0, m_pTmpRenderTarget);

			hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);

			hr = m_pD3DDev->SetPixelShader(m_pHLSLRedBlue);

			struct
			{
				float x, y, z, rhw;
				float tu, tv;
			}
			pVertices[] =
			{
				{0, 0, 0.5f, 2.0f, 0, 0},
				{(float)m_bd.Width, 0, 0.5f, 2.0f, 1, 0},
				{0, (float)m_bd.Height, 0.5f, 2.0f, 0, 1},
				{(float)m_bd.Width, (float)m_bd.Height, 0.5f, 2.0f, 1, 1},
			};

			hr = m_pD3DDev->BeginScene();
			hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
			hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
			hr = m_pD3DDev->EndScene();
		}
	}

	//

	m_perfmon.IncCounter(GSPerfMon::c_frame);

	static CString s_stats;

	if(!(m_perfmon.GetFrame()&15)) 
	{
		s_stats = m_perfmon.ToString(m_rs.GetFPS());
		// stats.Format(_T("%s - %.2f MB"), CString(stats), 1.0f*m_pD3DDev->GetAvailableTextureMem()/1024/1024);

		if(m_iOSD == 1)
		{
			SetWindowText(m_hWnd, s_stats);
		}
	}

	if(m_iOSD == 2)
	{
		hr = m_pD3DDev->BeginScene();
		CRect r = dst;
		D3DCOLOR c = D3DCOLOR_ARGB(255, 0, 255, 0);
		if(m_pD3DXFont->DrawText(NULL, s_stats, -1, &r, DT_CALCRECT|DT_LEFT|DT_WORDBREAK, c))
			m_pD3DXFont->DrawText(NULL, s_stats, -1, &r, DT_LEFT|DT_WORDBREAK, c);
		hr = m_pD3DDev->EndScene();
	}

	//

	hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
}

void GSState::FlushPrimInternal()
{
	FlushWriteTransfer();

	FlushPrim();
}
