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
#include "resource.h"

GSState::GSState(int w, int h, HWND hWnd, HRESULT& hr) 
	: m_hWnd(hWnd)
	, m_fp(NULL)
	, m_PRIM(7)
{
	hr = E_FAIL;

    CWinApp* pApp = AfxGetApp();
	m_fDisableShaders = !!pApp->GetProfileInt(_T("Settings"), _T("DisableShaders"), FALSE);
	m_fHalfVRes = !!pApp->GetProfileInt(_T("Settings"), _T("HalfVRes"), FALSE);

	memset(&m_tag, 0, sizeof(m_tag));
	m_nreg = 0;

	m_fpGSirq = NULL;

	for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;

	m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerPRIM;
	m_fpGIFPackedRegHandlers[GIF_REG_RGBAQ] = &GSState::GIFPackedRegHandlerRGBAQ;
	m_fpGIFPackedRegHandlers[GIF_REG_ST] = &GSState::GIFPackedRegHandlerST;
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

	// D3D

	if(!(m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return;

	D3DCAPS9 d3dcaps;
	ZeroMemory(&d3dcaps, sizeof(d3dcaps));
	m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dcaps);
/*
	if(d3dcaps.VertexShaderVersion < D3DVS_VERSION(1, 1))
		return;

	if(d3dcaps.PixelShaderVersion < D3DPS_VERSION(1, 1))
		return;
*/
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY/*D3DSWAPEFFECT_DISCARD*//*D3DSWAPEFFECT_FLIP*/;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	//d3dpp.BackBufferCount = 2;
	d3dpp.BackBufferWidth = w;
	d3dpp.BackBufferHeight = h;
//	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
/*
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
*/

	DWORD BehaviorFlags = d3dcaps.VertexProcessingCaps
		? D3DCREATE_HARDWARE_VERTEXPROCESSING
		: D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	if(FAILED(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		BehaviorFlags, &d3dpp, &m_pD3DDev)))
		return;

	hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET/*|D3DCLEAR_ZBUFFER*/, 0, 1.0f, 0);

    hr = m_pD3DDev->GetRenderTarget(0, &m_pOrgRenderTarget);
	// hr = m_pD3DDev->GetDepthStencilSurface(&m_pOrgDepthStencil);

    hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);

	for(int i = 0; i < 8; i++)
	{
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		// hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = m_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	static const UINT nShaderIDs[] = 
	{
		IDR_PIXELSHADER000, IDR_PIXELSHADER010, IDR_PIXELSHADER011,
		IDR_PIXELSHADER1x0, IDR_PIXELSHADER1x1,
		IDR_PIXELSHADER200, IDR_PIXELSHADER210, IDR_PIXELSHADER211,
		IDR_PIXELSHADER300, IDR_PIXELSHADER310, IDR_PIXELSHADER311,
		IDR_PIXELSHADER_EN11, IDR_PIXELSHADER_EN10, IDR_PIXELSHADER_EN01, IDR_PIXELSHADER_EN00,
	};

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if(!m_fDisableShaders)
	for(int i = 0; i < countof(nShaderIDs); i++)
	{
		CStringA str;
		HRSRC hRsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(nShaderIDs[i]), _T("PixelShader"));
		HGLOBAL hGlobal = LoadResource(AfxGetResourceHandle(), hRsrc);
		DWORD size = SizeofResource(AfxGetResourceHandle(), hRsrc);
		memcpy(str.GetBufferSetLength(size), LockResource(hGlobal), size);

		CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
		hr = D3DXAssembleShader((LPCSTR)str, str.GetLength(), NULL, NULL, 0, &pShader, &pErrorMsgs);

		if(FAILED(hr))
		{
			TRACE(_T("%s\n"), CString((char*)pErrorMsgs->GetBufferPointer())); 
			ASSERT(0);
		}
		else
		{
			hr = m_pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &m_pPixelShaders[i]);
			ASSERT(SUCCEEDED(hr));
		}
	}

	hr = S_OK;

	Reset();

	::DeleteFile(_T("c:\\gs.txt"));
	m_fp = _tfopen(_T("c:\\gs.txt"), _T("at"));

//	m_rs.CSRr.REV = 0x20;

}

GSState::~GSState()
{
	Reset();

	if(m_fp) fclose(m_fp);
}

UINT32 GSState::Freeze(freezeData* fd)
{
	fd->size = sizeof(m_version)
		+ sizeof(m_de) + sizeof(m_rs) + sizeof(m_v) 
		+ sizeof(m_x) + sizeof(m_y) + 1024*1024*4
		+ sizeof(m_tag) + sizeof(m_nreg)
		/*+ sizeof(m_vl)*/;

	if(!(fd->data = (BYTE*)malloc(fd->size))) 
		return -1;

	FlushPrim();

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

	FlushPrim();

	memcpy(&m_de, data, sizeof(m_de)); data += sizeof(m_de);
	memcpy(&m_rs, data, sizeof(m_rs)); data += sizeof(m_rs);
	memcpy(&m_v, data, sizeof(m_v)); data += sizeof(m_v);
	memcpy(&m_x, data, sizeof(m_x)); data += sizeof(m_x);
	memcpy(&m_y, data, sizeof(m_y)); data += sizeof(m_y);
	memcpy(m_lm.GetVM(), data, 1024*1024*4); data += 1024*1024*4;
	memcpy(&m_tag, data, sizeof(m_tag)); data += sizeof(m_tag);
	memcpy(&m_nreg, data, sizeof(m_nreg)); data += sizeof(m_nreg);
	// memcpy(&m_vl, data, sizeof(m_vl)); data += sizeof(m_vl);

	return 0;
}

void GSState::Write64(GS_REG mem, GSReg* r)
{
	ASSERT(r);

	switch(mem)
	{
		case GS_PMODE:
			m_rs.PMODE.i64 = r->i64;
			LOG((_T("Write64(GS_PMODE, EN1=%x EN2=%x CRTMD=%x MMOD=%x AMOD=%x SLBG=%x ALP=%x)\n"), 
				r->PMODE.EN1,
				r->PMODE.EN2,
				r->PMODE.CRTMD,
				r->PMODE.MMOD,
				r->PMODE.AMOD,
				r->PMODE.SLBG,
				r->PMODE.ALP));
			break;

		case GS_SMODE1:
			m_rs.SMODE1.i64 = r->i64;
			LOG((_T("Write64(GS_SMODE1, VMODE=%x)\n"), 
				r->SMODE1.VMODE));
			break;

		case GS_SMODE2:
			m_rs.SMODE2.i64 = r->i64;
			LOG((_T("Write64(GS_SMODE2, INT=%x FFMD=%x DPMS=%x)\n"), 
				r->SMODE2.INT,
				r->SMODE2.FFMD,
				r->SMODE2.DPMS));
			break;

		case GS_SRFSH:
			LOG((_T("Write64(GS_SRFSH, %016I64x)\n"), r->i64));
			break;

		case GS_SYNCH1:
			LOG((_T("Write64(GS_SYNCH1, %016I64x)\n"), r->i64));
			break;

		case GS_SYNCH2:
			LOG((_T("Write64(GS_SYNCH2, %016I64x)\n"), r->i64));
			break;

		case GS_SYNCV:
			LOG((_T("Write64(GS_SYNCV, %016I64x)\n"), r->i64));
			break;

		case GS_DISPFB1:
			m_rs.DISPFB[0].i64 = r->i64;
			LOG((_T("Write64(GS_DISPFB1, FBP=%x FBW=%x PSM=%x DBX=%x DBY=%x)\n"), 
				r->DISPFB.FBP<<5,
				r->DISPFB.FBW*64,
				r->DISPFB.PSM,
				r->DISPFB.DBX,
				r->DISPFB.DBY));
			break;

		case GS_DISPLAY1:
			m_rs.DISPLAY[0].i64 = r->i64;
			LOG((_T("Write64(GS_DISPLAY1, DX=%x DY=%x MAGH=%x MAGV=%x DW=%x DH=%x)\n"),
				r->DISPLAY.DX,
				r->DISPLAY.DY,
				r->DISPLAY.MAGH,
				r->DISPLAY.MAGV,
				r->DISPLAY.DW,
				r->DISPLAY.DH));
			break;

		case GS_DISPFB2:
			m_rs.DISPFB[1].i64 = r->i64;
			LOG((_T("Write64(GS_DISPFB2, FBP=%x FBW=%x PSM=%x DBX=%x DBY=%x)\n"), 
				r->DISPFB.FBP<<5,
				r->DISPFB.FBW*64,
				r->DISPFB.PSM,
				r->DISPFB.DBX,
				r->DISPFB.DBY));
			break;

		case GS_DISPLAY2:
			m_rs.DISPLAY[1].i64 = r->i64;
			LOG((_T("Write64(GS_DISPLAY2, DX=%x DY=%x MAGH=%x MAGV=%x DW=%x DH=%x)\n"),
				r->DISPLAY.DX,
				r->DISPLAY.DY,
				r->DISPLAY.MAGH,
				r->DISPLAY.MAGV,
				r->DISPLAY.DW,
				r->DISPLAY.DH));
			break;

		case GS_EXTBUF:
			m_rs.EXTBUF.i64 = r->i64;
			LOG((_T("Write64(GS_EXTBUF, EXBP=%x EXBW=%x FBIN=%x WFFMD=%x EMODA=%x EMODC=%x WDX=%x WDY=%x)\n"),
				r->EXTBUF.EXBP,
				r->EXTBUF.EXBW,
				r->EXTBUF.FBIN,
				r->EXTBUF.WFFMD,
				r->EXTBUF.EMODA,
				r->EXTBUF.EMODC,
				r->EXTBUF.WDX,
				r->EXTBUF.WDY));
			break;

		case GS_EXTDATA:
			m_rs.EXTDATA.i64 = r->i64;
			LOG((_T("Write64(GS_EXTDATA, SX=%x SY=%x SMPH=%x SMPV=%x WW=%x WH=%x)\n"), 
				r->EXTDATA.SX,
				r->EXTDATA.SY,
				r->EXTDATA.SMPH,
				r->EXTDATA.SMPV,
				r->EXTDATA.WW,
				r->EXTDATA.WH));
			break;

		case GS_EXTWRITE:
			m_rs.EXTWRITE.i64 = r->i64;
			LOG((_T("Write64(GS_EXTWRITE, WRITE=%x)\n"),
				r->EXTWRITE.WRITE));
			break;

		case GS_BGCOLOR:
			m_rs.BGCOLOR.i64 = r->i64;
			LOG((_T("Write64(GS_BGCOLOR, R=%x G=%x B=%x)\n"),
				r->BGCOLOR.R,
				r->BGCOLOR.G,
				r->BGCOLOR.B));
			break;

		case GS_CSR:
			m_rs.CSRw.i64 = r->i64;
			LOG((_T("Write64(GS_CSR, SIGNAL=%x FINISH=%x HSINT=%x VSINT=%x EDWINT=%x ZERO1=%x ZERO2=%x FLUSH=%x RESET=%x NFIELD=%x FIELD=%x FIFO=%x REV=%x ID=%x)\n"),
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
				r->CSR.ID));
			if(m_rs.CSRw.SIGNAL) m_rs.CSRr.SIGNAL = 0;
			if(m_rs.CSRw.FINISH) m_rs.CSRr.FINISH = 0;
			if(m_rs.CSRw.RESET) Reset();
			break;

		case GS_IMR:
			m_rs.IMR.i64 = r->i64;
			LOG((_T("Write64(GS_IMR, _PAD1=%x SIGMSK=%x FINISHMSK=%x HSMSK=%x VSMSK=%x EDWMSK=%x)\n"),
				r->IMR._PAD1,
				r->IMR.SIGMSK,
				r->IMR.FINISHMSK,
				r->IMR.HSMSK,
				r->IMR.VSMSK,
				r->IMR.EDWMSK));
			break;

		case GS_BUSDIR:
			m_rs.BUSDIR.i64 = r->i64;
			LOG((_T("Write64(GS_BUSDIR, DIR=%x)\n"),
				r->BUSDIR.DIR));
			break;

		case GS_SIGLBLID:
			m_rs.SIGLBLID.i64 = r->i64;
			LOG((_T("Write64(GS_SIGLBLID, SIGID=%x LBLID=%x)\n"),
				r->SIGLBLID.SIGID,
				r->SIGLBLID.LBLID));
			break;

		default:
			LOG((_T("*** WARNING *** Write64(?????????, %016I64x)\n"), r->i64));
			ASSERT(0);
			break;
	}
}

UINT32 GSState::Read32(GS_REG mem)
{
	if(mem == GS_CSR) return m_rs.CSRr.ai32[0];

	return (UINT32)Read64(mem);
}

UINT64 GSState::Read64(GS_REG mem)
{
	if(mem == GS_CSR) return m_rs.CSRr.i64;

	GSReg* r = NULL;

	switch(mem)
	{
		case GS_CSR:
			r = reinterpret_cast<GSReg*>(&m_rs.CSRr);
			LOG((_T("Read64(GS_CSR, SIGNAL=%x FINISH=%x HSINT=%x VSINT=%x EDWINT=%x ZERO1=%x ZERO2=%x FLUSH=%x RESET=%x NFIELD=%x FIELD=%x FIFO=%x REV=%x ID=%x)\n"),
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
				r->CSR.ID));
			break;

		case GS_SIGLBLID:
			r = reinterpret_cast<GSReg*>(&m_rs.SIGLBLID);
			LOG((_T("Read64(GS_SIGLBLID, SIGID=%x LBLID=%x)\n"),
				r->SIGLBLID.SIGID,
				r->SIGLBLID.LBLID));
			break;

		case GS_UNKNOWN:
			LOG((_T("*** WARNING *** Read64(%08x)\n"), mem));
			return m_rs.CSRr.FIELD << 13;
			break;

		default:
			LOG((_T("*** WARNING *** Read64(%08x)\n"), mem));
			ASSERT(0);
			break;
	}

	return r ? r->i64 : 0;
}

void GSState::ReadFIFO(BYTE* pMem)
{
	LOG((_T("*** WARNING *** ReadFIFO(%08x)\n"), pMem));
	ReadTransfer(pMem, 16);
}

void GSState::Transfer(BYTE* pMem)
{
	Transfer(pMem, -1);
}

void GSState::Transfer(BYTE* pMem, UINT32 size)
{
	while(size > 0)
	{
		LOG((_T("Transfer(%08x, %d) START\n"), pMem, size));

		bool fEOP = false;

		if(m_tag.NLOOP == 0)
		{
			m_tag = *(GIFTag*)pMem;
			m_nreg = 0;

			LOG((_T("GIFTag NLOOP=%x EOP=%x PRE=%x PRIM=%x FLG=%x NREG=%x REGS=%x\n"), 
				m_tag.NLOOP,
				m_tag.EOP,
				m_tag.PRE,
				m_tag.PRIM,
				m_tag.FLG,
				m_tag.NREG,
				m_tag.REGS));

			pMem += sizeof(GIFTag);
			size--;

			if(m_tag.PRE)
			{
				LOG((_T("PRE ")));
				GIFReg r;
				r.i64 = m_tag.PRIM;
				(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r);
			}

			if(m_tag.EOP)
			{
				LOG((_T("EOP\n")));
				fEOP = true;
			}
			else if(m_tag.NLOOP == 0)
			{
				LOG((_T("*** WARNING *** m_tag.NLOOP == 0 && EOP == 0\n")));
				fEOP = true;
				// ASSERT(0);
			}
		}

		switch(m_tag.FLG)
		{
		case GIF_FLG_PACKED:
			for(GIFPackedReg* r = (GIFPackedReg*)pMem; m_tag.NLOOP > 0 && size > 0; r++, size--, pMem += sizeof(GIFPackedReg))
			{
				BYTE reg = GET_GIF_REG(m_tag, m_nreg);
				(this->*m_fpGIFPackedRegHandlers[reg])(r);
				if((m_nreg=(m_nreg+1)&0xf) == m_tag.NREG) {m_nreg = 0; m_tag.NLOOP--;}
			}
			break;
		case GIF_FLG_REGLIST:
			size *= 2;
			for(GIFReg* r = (GIFReg*)pMem; m_tag.NLOOP > 0 && size > 0; r++, size--, pMem += sizeof(GIFReg))
			{
				BYTE reg = GET_GIF_REG(m_tag, m_nreg);
				(this->*m_fpGIFRegHandlers[reg])(r);
				if((m_nreg=(m_nreg+1)&0xf) == m_tag.NREG) {m_nreg = 0; m_tag.NLOOP--;}
			}
			if(size&1) pMem += sizeof(GIFReg);
			size /= 2;
			break;
		case GIF_FLG_IMAGE2:
			LOG((_T("*** WARNING **** Unexpected GIFTag flag\n")));
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

		LOG((_T("Transfer(%08x, %d) END\n"), pMem, size));

		if(fEOP && (INT32)size <= 0)
		{
			break;
		}
	}
}

UINT32 GSState::MakeSnapshot(char* path)
{
	CString fn;
	fn.Format(_T("%sgsdx9_%s.bmp"), CString(path), CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
	return D3DXSaveSurfaceToFile(fn, D3DXIFF_BMP, m_pOrgRenderTarget, NULL, NULL);
}

void GSState::VSync()
{
	FlushPrim();

	m_stats.VSync();
	CString str = m_stats.ToString((m_rs.SMODE1.VMODE == GS_PAL ? 50 : 60) / (m_rs.SMODE2.INT ? 1 : 2));
	LOG((_T("VSync(%s)\n"), str));
	if(!(m_stats.GetFrame()&7)) SetWindowText(m_hWnd, str);

	m_rs.CSRr.NFIELD = 1; // ?
	if(m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/);
		m_rs.CSRr.FIELD = 1 - m_rs.CSRr.FIELD;

	Flip();

	EndFrame();
}

void GSState::Reset()
{
	memset(&m_de, 0, sizeof(m_de));
	memset(&m_rs, 0, sizeof(m_rs));
	memset(&m_tag, 0, sizeof(m_tag));
	memset(&m_v, 0, sizeof(m_v));
	m_nreg = 0;

	m_PRIM = 7;

	if(m_pD3DDev) m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET/*|D3DCLEAR_ZBUFFER*/, 0, 1.0f, 0);
}

void GSState::Flip()
{
	HRESULT hr;

	hr = m_pD3DDev->SetRenderTarget(0, m_pOrgRenderTarget);
	hr = m_pD3DDev->SetDepthStencilSurface(m_pOrgDepthStencil);
	// hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
    hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	const float c[] = 
	{
		1.0f * m_rs.BGCOLOR.B / 255,
		1.0f * m_rs.BGCOLOR.G / 255, 
		1.0f * m_rs.BGCOLOR.R / 255,
		1.0f * m_rs.PMODE.ALP / 255,
		0.0f,
		0.0f,
		0.0f,
		m_rs.PMODE.MMOD ? 1.0f : 0.0f,
		0.0f,
		0.0f,
		0.0f,
		m_rs.PMODE.SLBG ? 1.0f : 0.0f,
	};

	hr = m_pD3DDev->SetPixelShaderConstantF(0, c, 3);
}
