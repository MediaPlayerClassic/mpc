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
#include "GSRendererHW.h"

inline BYTE SCALE_ALPHA(BYTE a) 
{
	return (((a)&0x80)?0xff:((a)<<1));
}

static const double log_2pow32 = log(2.0)*32;

//

GSRendererHW::GSRendererHW(HWND hWnd, HRESULT& hr)
	: GSRenderer<HWVERTEX>(1024, 1024, hWnd, hr)
//	: GSRenderer<HWVERTEX>(640, 224, hWnd, hr)
{
	Reset();
}

GSRendererHW::~GSRendererHW()
{
}

void GSRendererHW::Reset()
{
	m_primtype = D3DPT_FORCE_DWORD;
	
	m_tc.RemoveAll();

	m_pRenderTargets.RemoveAll();

	POSITION pos = m_pRenderWnds.GetStartPosition();
	while(pos)
	{
		DWORD FBP;
		CGSWnd* pWnd = NULL;
		m_pRenderWnds.GetNextAssoc(pos, FBP, pWnd);
		pWnd->DestroyWindow();
		delete pWnd;
	}
	m_pRenderWnds.RemoveAll();

	__super::Reset();
}

void GSRendererHW::VertexKick(bool fSkip)
{
	HWVERTEX v;

	v.x = ((float)m_v.XYZ.X - m_ctxt->XYOFFSET.OFX)/16;
	v.y = ((float)m_v.XYZ.Y - m_ctxt->XYOFFSET.OFY)/16;
//	v.x = (float)m_v.XYZ.X/16 - (m_ctxt->XYOFFSET.OFX>>4);
//	v.y = (float)m_v.XYZ.Y/16 - (m_ctxt->XYOFFSET.OFY>>4);
	//if(m_v.XYZ.Z && m_v.XYZ.Z < 0x100) m_v.XYZ.Z = 0x100;
	//v.z = 1.0f * (m_v.XYZ.Z>>8)/(UINT_MAX>>8);
	v.z = log(1.0 + m_v.XYZ.Z)/log_2pow32;
	v.rhw = m_v.RGBAQ.Q;

	BYTE R = m_v.RGBAQ.R;
	BYTE G = m_v.RGBAQ.G;
	BYTE B = m_v.RGBAQ.B;
	BYTE A = SCALE_ALPHA(m_v.RGBAQ.A);
/*
	if(m_de.PRIM.FGE)
	{
		R = (m_v.FOG.F * R + (0xff - m_v.FOG.F) * m_de.FOGCOL.FCR + 127) >> 8;
		G = (m_v.FOG.F * G + (0xff - m_v.FOG.F) * m_de.FOGCOL.FCG + 127) >> 8;
		B = (m_v.FOG.F * B + (0xff - m_v.FOG.F) * m_de.FOGCOL.FCB + 127) >> 8;
	}
*/
	if(m_de.PRIM.TME)
	{
		A = m_v.RGBAQ.A;

		if(m_de.PRIM.FST)
		{
			v.tu = (float)m_v.UV.U / (16<<m_ctxt->TEX0.TW);
			v.tv = (float)m_v.UV.V / (16<<m_ctxt->TEX0.TH);
			v.rhw = 1.0f;
		}
		else if(m_v.RGBAQ.Q != 0)
		{
			v.tu = m_v.ST.S / m_v.RGBAQ.Q;
			v.tv = m_v.ST.T / m_v.RGBAQ.Q;
		}
		else
		{
			v.tu = m_v.ST.S;
			v.tv = m_v.ST.T;
		}
	}

	v.color = D3DCOLOR_ARGB(A, R, G, B);

	m_vl.AddTail(v);

	__super::VertexKick(fSkip);
}

void GSRendererHW::DrawingKick(bool fSkip)
{
	LOG((_T("DrawingKick %d\n"), m_de.PRIM.PRIM));

	if(m_PRIM != m_de.PRIM.PRIM && m_nVertices > 0) FlushPrim();
	m_PRIM = m_de.PRIM.PRIM;

	HWVERTEX* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	LOG2((_T("Prim %05x %05x %04x\n"), 
		m_ctxt->FRAME.Block(), m_de.PRIM.TME ? (UINT32)m_ctxt->TEX0.TBP0 : 0xfffff,
		(m_de.PRIM.ABE || (m_PRIM == 1 || m_PRIM == 2) && m_de.PRIM.AA1)
			? ((m_ctxt->ALPHA.A<<12)|(m_ctxt->ALPHA.B<<8)|(m_ctxt->ALPHA.C<<4)|m_ctxt->ALPHA.D) 
			: 0xffff));

	switch(m_PRIM)
	{
	case 3: // triangle list
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 4: // triangle strip
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		break;
	case 5: // triangle fan
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		break;
	case 6: // sprite
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		nVertices += 2;
		// ASSERT(pVertices[0].z == pVertices[1].z);
		pVertices[0].z = pVertices[1].z;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].y = pVertices[0].y;
		pVertices[1].tv = pVertices[0].tv;
		pVertices[2].x = pVertices[0].x;
		pVertices[2].tu = pVertices[0].tu;
		nVertices += 2;
		pVertices[5] = pVertices[3];
		pVertices[3] = pVertices[1];
		pVertices[4] = pVertices[2];
		break;
	case 1: // line
		m_primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 2: // line strip
		m_primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		break;
	case 0: // point
		m_primtype = D3DPT_POINTLIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	default:
		ASSERT(0);
		return;
	}

	if(fSkip || !m_rs.IsEnabled(0) && !m_rs.IsEnabled(1))
	{
#ifdef ENABLE_STRIPFAN
		FlushPrim();
#endif
		return;
	}

	m_nVertices += nVertices;

	if(!m_de.PRIM.IIP)
	{
		pVertices[0].color = pVertices[nVertices-1].color;
		/*for(int i = nVertices-1; i > 0; i--)
			pVertices[i-1].color = pVertices[i].color;*/
	}
/*
	if(::GetAsyncKeyState(VK_SPACE)&0x80000000)
	{
		FlushPrim();
		Flip();
	}
*/
}

void GSRendererHW::FlushPrim()
{
	if(m_nVertices > 0)
	{
		int nPrims = 0;

		switch(m_primtype)
		{
		case D3DPT_TRIANGLELIST: ASSERT(!(m_nVertices%3)); nPrims = m_nVertices/3; break;
		case D3DPT_TRIANGLESTRIP: ASSERT(m_nVertices > 2); nPrims = m_nVertices-2; break;
		case D3DPT_TRIANGLEFAN: ASSERT(m_nVertices > 2); nPrims = m_nVertices-2; break;
		case D3DPT_LINELIST: ASSERT(!(m_nVertices&1)); nPrims = m_nVertices/2; break;
		case D3DPT_LINESTRIP: ASSERT(m_nVertices > 1); nPrims = m_nVertices-1; break;
		case D3DPT_POINTLIST: nPrims = m_nVertices; break;
		default: ASSERT(0); return;
		}

		LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));

		m_stats.IncPrims(nPrims);

		//////////////////////

		HRESULT hr;

		CComPtr<IDirect3DSurface9> pBackBuff;
		hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuff);

		D3DSURFACE_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		pBackBuff->GetDesc(&bd);

		scale_t scale((float)bd.Width / (m_ctxt->FRAME.FBW*64), (float)bd.Height / m_rs.GetSize(m_rs.IsEnabled(1)?1:0).cy);
		if(m_fHalfVRes) scale.y /= 2;

		//////////////////////

		CComPtr<IDirect3DTexture9> pRT;
		CComPtr<IDirect3DSurface9> pDS;

		bool fClearRT = false;
		bool fClearDS = false;

		if(!m_pRenderTargets.Lookup(m_ctxt->FRAME.Block(), pRT))
		{
			hr = m_pD3DDev->CreateTexture(bd.Width, bd.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);
			if(S_OK != hr) {ASSERT(0); return;}
			m_pRenderTargets[m_ctxt->FRAME.Block()] = pRT;
#ifdef DEBUG_RENDERTARGETS
			CGSWnd* pWnd = new CGSWnd();
			CString str; str.Format(_T("%05x"), m_ctxt->FRAME.Block());
			pWnd->Create(str);
			m_pRenderWnds[m_ctxt->FRAME.Block()] = pWnd;
			pWnd->Show();
#endif
			fClearRT = true;
		}

		if(!m_pDepthStencils.Lookup(m_ctxt->ZBUF.ZBP, pDS))
		{
			hr = m_pD3DDev->CreateDepthStencilSurface(bd.Width, bd.Height, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, FALSE, &pDS, NULL);
			if(S_OK != hr) {ASSERT(0); return;}
			m_pDepthStencils[m_ctxt->ZBUF.ZBP] = pDS;
			fClearDS = true;
		}

		if(!pRT || !pDS) {ASSERT(0); return;}

		pRT->SetPrivateData(GUID_NULL, &scale, sizeof(scale), 0);

		//////////////////////

		GSTexture t;
		D3DSURFACE_DESC td;
		ZeroMemory(&td, sizeof(td));

		if(m_de.PRIM.TME && CreateTexture(t))
		{
			// if(IsRenderTarget(t.m_pTexture)) ConvertRT(t.m_pTexture);
			hr = t.m_pTexture->GetLevelDesc(0, &td);
		}

		//////////////////////

		CComPtr<IDirect3DSurface9> pSurf;
		hr = pRT->GetSurfaceLevel(0, &pSurf);
		hr = m_pD3DDev->SetRenderTarget(0, pSurf);
		hr = m_pD3DDev->SetDepthStencilSurface(pDS);
		if(fClearRT) hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		if(fClearDS) hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

		D3DSURFACE_DESC rd;
		ZeroMemory(&rd, sizeof(rd));
		pRT->GetLevelDesc(0, &rd);

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_SHADEMODE, m_de.PRIM.IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT);

		//////////////////////

		CComPtr<IDirect3DPixelShader9> pPixelShader;

		if(m_de.PRIM.TME && t.m_pTexture)
		{
			hr = m_pD3DDev->SetTexture(0, t.m_pTexture);

			D3DTEXTUREADDRESS u, v;

			switch(m_ctxt->CLAMP.WMS&1)
			{
			case 0: u = D3DTADDRESS_WRAP; break; // repeat
			case 1: u = D3DTADDRESS_CLAMP; break; // clamp
			}

			switch(m_ctxt->CLAMP.WMT&1)
			{
			case 0: v = D3DTADDRESS_WRAP; break; // repeat
			case 1: v = D3DTADDRESS_CLAMP; break; // clamp
			}

			hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, u);
			hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, v);

			bool fRT = IsRenderTarget(t.m_pTexture); // RTs already have a correctly scaled alpha

			switch(m_ctxt->TEX0.TFX)
			{
			case 0:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[0];
				else if(!fRT) pPixelShader = m_pPixelShaders[1];
				else pPixelShader = m_pPixelShaders[2];
				break;
			case 1:
				if(!fRT) pPixelShader = m_pPixelShaders[3];
				else pPixelShader = m_pPixelShaders[4];
				break;
			case 2:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[5];
				else if(!fRT) pPixelShader = m_pPixelShaders[6];
				else pPixelShader = m_pPixelShaders[7];
				break;
			case 3:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[8];
				else if(!fRT) pPixelShader = m_pPixelShaders[9];
				else pPixelShader = m_pPixelShaders[10];
				break;
			}

			// ASSERT(pPixelShader);

			if(!pPixelShader)
			{
				int stage = 0;

				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
				hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);

				switch(m_ctxt->TEX0.TFX)
				{
				case 0:
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
					if(m_ctxt->TEX0.TCC)
					{
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, fRT ? D3DTOP_MODULATE2X : D3DTOP_MODULATE4X);
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
					}
					else
					{
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
					}
					stage++;
					break;
				case 1:
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, fRT ? D3DTOP_SELECTARG1 : D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
					stage++;
					break;
				case 2:
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
					if(m_ctxt->TEX0.TCC)
					{
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
						ASSERT(!fRT); // FIXME
					}
					stage++;
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_ALPHAREPLICATE|D3DTA_DIFFUSE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
					stage++;
					break;
				case 3:
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
					if(m_ctxt->TEX0.TCC)
					{
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
						hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
						ASSERT(!fRT); // FIXME
					}
					stage++;
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_ALPHAREPLICATE|D3DTA_DIFFUSE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
					stage++;
					break;
				}

				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			}
		}
		else
		{
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			hr = m_pD3DDev->SetTexture(0, NULL);
		}

		//////////////////////

		bool fABE = m_de.PRIM.ABE || (m_primtype == D3DPT_LINELIST || m_primtype == D3DPT_LINESTRIP) && m_de.PRIM.AA1;
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, fABE);
		if(fABE)
		{
			// (A:Cs/Cd/0 - B:Cs/Cd/0) * C:As/Ad/FIX + D:Cs/Cd/0

			BYTE FIX = SCALE_ALPHA(m_ctxt->ALPHA.FIX);

			hr = m_pD3DDev->SetRenderState(D3DRS_BLENDFACTOR, (0x010101*FIX)|(FIX<<24));

			D3DBLENDOP op = D3DBLENDOP_ADD;
			D3DBLEND src = D3DBLEND_SRCALPHA, dst = D3DBLEND_INVSRCALPHA;

			static const struct {bool bogus; D3DBLENDOP op; D3DBLEND src, dst;} blendmap[3*3*3*3] =
			{
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0000: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0001: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0002: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0010: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0011: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0012: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0020: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0021: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0022: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{1, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// * 0100: (Cs - Cd)*As + Cs ==> Cs*(As + 1) - Cd*As
				{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA},			// 0101: (Cs - Cd)*As + Cd ==> Cs*As + Cd*(1 - As)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// 0102: (Cs - Cd)*As + 0 ==> Cs*As - Cd*As
				{1, D3DBLENDOP_SUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},		// * 0110: (Cs - Cd)*Ad + Cs ==> Cs*(Ad + 1) - Cd*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA},			// 0111: (Cs - Cd)*Ad + Cd ==> Cs*Ad + Cd*(1 - Ad)
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},			// 0112: (Cs - Cd)*Ad + 0 ==> Cs*Ad - Cd*Ad
				{1, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// * 0120: (Cs - Cd)*F + Cs ==> Cs*(F + 1) - Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_INVBLENDFACTOR},		// 0121: (Cs - Cd)*F + Cd ==> Cs*F + Cd*(1 - F)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// 0122: (Cs - Cd)*F + 0 ==> Cs*F - Cd*F
				{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// * 0200: (Cs - 0)*As + Cs ==> Cs*(As + 1)
				{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE},					// 0201: (Cs - 0)*As + Cd ==> Cs*As + Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// 0202: (Cs - 0)*As + 0 ==> Cs*As
				{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// * 0210: (Cs - 0)*Ad + Cs ==> Cs*(As + 1)
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ONE},					// 0211: (Cs - 0)*Ad + Cd ==> Cs*Ad + Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},					// 0212: (Cs - 0)*Ad + 0 ==> Cs*Ad
				{1, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// * 0220: (Cs - 0)*F + Cs ==> Cs*(F + 1)
				{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},				// 0221: (Cs - 0)*F + Cd ==> Cs*F + Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// 0222: (Cs - 0)*F + 0 ==> Cs*F
				{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA},			// 1000: (Cd - Cs)*As + Cs ==> Cd*As + Cs*(1 - As)
				{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// * 1001: (Cd - Cs)*As + Cd ==> Cd*(As + 1) - Cs*As
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// 1002: (Cd - Cs)*As + 0 ==> Cd*As - Cs*As
				{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_DESTALPHA},			// 1010: (Cd - Cs)*Ad + Cs ==> Cd*Ad + Cs*(1 - Ad)
				{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// * 1011: (Cd - Cs)*Ad + Cd ==> Cd*(Ad + 1) - Cs*Ad
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// 1012: (Cd - Cs)*Ad + 0 ==> Cd*Ad - Cs*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_BLENDFACTOR},		// 1020: (Cd - Cs)*F + Cs ==> Cd*F + Cs*(1 - F)
				{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// * 1021: (Cd - Cs)*F + Cd ==> Cd*(F + 1) - Cs*F
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// 1022: (Cd - Cs)*F + 0 ==> Cd*F - Cs*F
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1100: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1101: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1102: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1110: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1111: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1112: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1120: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1121: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1122: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_SRCALPHA},					// 1200: (Cd - 0)*As + Cs ==> Cs + Cd*As
				{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// * 1201: (Cd - 0)*As + Cd ==> Cd*(1 + As)
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// 1202: (Cd - 0)*As + 0 ==> Cd*As
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_DESTALPHA},					// 1210: (Cd - 0)*Ad + Cs ==> Cs + Cd*Ad
				{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_DESTALPHA},					// * 1211: (Cd - 0)*Ad + Cd ==> Cd*(1 + Ad)
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_DESTALPHA},					// 1212: (Cd - 0)*Ad + 0 ==> Cd*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},				// 1220: (Cd - 0)*F + Cs ==> Cs + Cd*F
				{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_BLENDFACTOR},				// * 1221: (Cd - 0)*F + Cd ==> Cd*(1 + F)
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_BLENDFACTOR},				// 1222: (Cd - 0)*F + 0 ==> Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_ZERO},				// 2000: (0 - Cs)*As + Cs ==> Cs*(1 - As)
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE},			// 2001: (0 - Cs)*As + Cd ==> Cd - Cs*As
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},			// 2002: (0 - Cs)*As + 0 ==> 0 - Cs*As
				{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ZERO},				// 2010: (0 - Cs)*Ad + Cs ==> Cs*(1 - Ad)
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ONE},			// 2011: (0 - Cs)*Ad + Cd ==> Cd - Cs*Ad
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},			// 2012: (0 - Cs)*Ad + 0 ==> 0 - Cs*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_ZERO},			// 2020: (0 - Cs)*F + Cs ==> Cs*(1 - F)
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},		// 2021: (0 - Cs)*F + Cd ==> Cd - Cs*F
				{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},		// 2022: (0 - Cs)*F + 0 ==> 0 - Cs*F
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_SRCALPHA},				// 2100: (0 - Cd)*As + Cs ==> Cs - Cd*As
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVSRCALPHA},				// 2101: (0 - Cd)*As + Cd ==> Cd*(1 - As)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},				// 2102: (0 - Cd)*As + 0 ==> 0 - Cd*As
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2110: (0 - Cd)*Ad + Cs ==> Cs - Cd*Ad
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVDESTALPHA},				// 2111: (0 - Cd)*Ad + Cd ==> Cd*(1 - Ad)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2112: (0 - Cd)*Ad + 0 ==> 0 - Cd*Ad
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2120: (0 - Cd)*F + Cs ==> Cs - Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVBLENDFACTOR},			// 2121: (0 - Cd)*F + Cd ==> Cd*(1 - F)
				{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2122: (0 - Cd)*F + 0 ==> 0 - Cd*F
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2200: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2201: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2202: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2210: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2211: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2212: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
				{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2220: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2221: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
				{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2222: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			};

			// bogus: 0100, 0110, 0120, 0200, 0210, 0220, 1001, 1011, 1021, 1201, 1211, 1221

			int i = (((m_ctxt->ALPHA.A&3)*3+(m_ctxt->ALPHA.B&3))*3+(m_ctxt->ALPHA.C&3))*3+(m_ctxt->ALPHA.D&3);

			ASSERT(m_ctxt->ALPHA.A != 3);
			ASSERT(m_ctxt->ALPHA.B != 3);
			ASSERT(m_ctxt->ALPHA.C != 3);
			ASSERT(m_ctxt->ALPHA.D != 3);
			ASSERT(!blendmap[i].bogus);

			hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOP, blendmap[i].op);
			hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, blendmap[i].src);
			hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, blendmap[i].dst);

			hr = m_pD3DDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
			hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
			hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
			hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
		}

		//////////////////////

		// close approx., to be tested...
		int mask = D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED;
		if(m_ctxt->FRAME.FBMSK&0xff000000) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
		if(m_ctxt->FRAME.FBMSK&0x00ff0000) mask &= ~D3DCOLORWRITEENABLE_BLUE;
		if(m_ctxt->FRAME.FBMSK&0x0000ff00) mask &= ~D3DCOLORWRITEENABLE_GREEN;
		if(m_ctxt->FRAME.FBMSK&0x000000ff) mask &= ~D3DCOLORWRITEENABLE_RED;
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

		ASSERT(m_ctxt->FRAME.FBMSK == 0); // wild arms (also 8H+pal on RT...)

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, m_ctxt->TEST.ZTE);
		hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, !m_ctxt->ZBUF.ZMSK);
		if(m_ctxt->TEST.ZTE)
		{
			DWORD zfunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_GREATEREQUAL, D3DCMP_GREATER};
			hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, zfunc[m_ctxt->TEST.ZTST]);

			// FIXME
			if(m_ctxt->ZBUF.ZMSK && m_ctxt->TEST.ZTST == 1)
			{
				HWVERTEX* pVertices = m_pVertices;
				for(int i = m_nVertices; i-- > 0; pVertices++)
					pVertices->z = 0;
			}
		}

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, m_ctxt->TEST.ATE); 
		if(m_ctxt->TEST.ATE)
		{
			DWORD afunc[] =
			{
				D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_LESS, D3DCMP_LESSEQUAL, 
				D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL
			};

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, afunc[m_ctxt->TEST.ATST]);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_ctxt->TEST.AREF));
		}

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
		CRect scissor(scale.x * m_ctxt->SCISSOR.SCAX0, scale.y * m_ctxt->SCISSOR.SCAY0, scale.x * (m_ctxt->SCISSOR.SCAX1+1), scale.y * (m_ctxt->SCISSOR.SCAY1+1));
		scissor.IntersectRect(scissor, CRect(0, 0, bd.Width, bd.Height));
		hr = m_pD3DDev->SetScissorRect(scissor);

		//////////////////////

		ASSERT(!m_de.PABE.PABE); // bios
		ASSERT(!m_ctxt->FBA.FBA); // bios
		// ASSERT(!m_ctxt->TEST.DATE); // sfex3 (after the capcom logo), vf4 (first menu fading in)

		//////////////////////

		{
			// hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
			// hr = m_pD3DDev->SetTexture(1, pRT);

			HWVERTEX* pVertices = m_pVertices;
			for(int i = m_nVertices; i-- > 0; pVertices++)
			{
				pVertices->x *= scale.x;
				pVertices->y *= scale.y;

				// pVertices->tu2 = pVertices->x / rd.Width;
				// pVertices->tv2 = pVertices->y / rd.Height;

				if(m_de.PRIM.TME)
				{
					float base, fract;
					fract = modf(pVertices->tu, &base);
					fract = fract * (1<<m_ctxt->TEX0.TW) / td.Width * t.m_scale.x;
					ASSERT(-1 <= fract && fract <= 1.01);
					pVertices->tu = base + fract;
					fract = modf(pVertices->tv, &base);
					fract = fract * (1<<m_ctxt->TEX0.TH) / td.Height * t.m_scale.y;
					//ASSERT(-1 <= fract && fract <= 1.01);
					pVertices->tv = base + fract;
				}
			}
		}

if(m_de.PRIM.TME && m_nVertices == 6 && (m_ctxt->FRAME.Block()) == 0x00000 && m_ctxt->TEX0.TBP0 == 0x00e00)
{
	if(m_stats.GetFrame() > 1500)
	{
//		hr = D3DXSaveTextureToFile(_T("c:\\rtbefore.bmp"), D3DXIFF_BMP, pRT, NULL);
	}
}
/*
		int Size = m_nVertices*sizeof(HWVERTEX);
		D3DVERTEXBUFFER_DESC vbdesc;
		memset(&vbdesc, 0, sizeof(vbdesc));
		if(m_pVertexBuffer) hr = m_pVertexBuffer->GetDesc(&vbdesc);
		if(vbdesc.Size < Size) m_pVertexBuffer = NULL;
		if(!m_pVertexBuffer)
		{
			hr = m_pD3DDev->CreateVertexBuffer(
				Size, D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, D3DFVF_CUSTOMVERTEX, 
				D3DPOOL_DEFAULT, &m_pVertexBuffer, NULL);
		}
		HWVERTEX* pVertices = NULL;
		if(S_OK == m_pVertexBuffer->Lock(0, 0, (void**)&pVertices, D3DLOCK_DISCARD|D3DLOCK_NOOVERWRITE))
		{
			memcpy(pVertices, m_pVertices, Size);
			m_pVertexBuffer->Unlock();
		}

		hr = m_pD3DDev->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(HWVERTEX));
*/
/*
		CComPtr<IDirect3DVertexBuffer9> pVB;

		hr = m_pD3DDev->CreateVertexBuffer(
			m_nVertices*sizeof(HWVERTEX), 
			D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC,
			D3DFVF_CUSTOMVERTEX,
			D3DPOOL_DEFAULT,
			&pVB, NULL);

		HWVERTEX* pVertices = NULL;
		if(S_OK == pVB->Lock(0, 0, (void**)&pVertices, D3DLOCK_DISCARD|D3DLOCK_NOOVERWRITE))
		{
			memcpy(pVertices, m_pVertices, m_nVertices*sizeof(HWVERTEX));
			pVB->Unlock();
		}

		hr = m_pD3DDev->SetStreamSource(0, pVB, 0, sizeof(HWVERTEX));
*/
		hr = m_pD3DDev->BeginScene();

		hr = m_pD3DDev->SetFVF(D3DFVF_HWVERTEX);

		if(pPixelShader) hr = m_pD3DDev->SetPixelShader(pPixelShader);

		if(1)//!m_de.PABE.PABE)
		{
			hr = m_pD3DDev->DrawPrimitiveUP(m_primtype, nPrims, m_pVertices, sizeof(HWVERTEX));
//			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
/*
			if(m_primtype == D3DPT_TRIANGLELIST && m_nVertices != 6)
			{
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTexture(0, NULL);
				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
				hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, 0);
				hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0); 
				for(int i = 0; i < nPrims; i++)
				{
					m_pVertices[i*3].color = 0xffffffff;
					m_pVertices[i*3+1].color = 0xffffffff;
					m_pVertices[i*3+2].color = 0xffffffff;
					HWVERTEX v[4];
					memcpy(v, &m_pVertices[i*3], sizeof(HWVERTEX)*3);
					v[3] = m_pVertices[i*3];
					hr = m_pD3DDev->DrawPrimitive(D3DPT_LINESTRIP, 0, nPrims);
				}
			}
			else
			{
				hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
			}
*/		}
		else
		{
			ASSERT(!m_ctxt->TEST.ATE); // TODO

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xfe);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
		}
	    
		if(pPixelShader) hr = m_pD3DDev->SetPixelShader(NULL);
/*
		if(m_ctxt->TEST.ATE && m_ctxt->TEST.AFAIL && m_ctxt->TEST.ATST != 1)
		{
			ASSERT(!m_de.PABE.PABE);

			DWORD iafunc[] =
			{
				D3DCMP_ALWAYS, D3DCMP_NEVER, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, 
				D3DCMP_NOTEQUAL, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL
			};

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, iafunc[m_ctxt->TEST.ATST]);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_ctxt->TEST.AREF));

			int mask = 0;
			bool zwrite = false;

			switch(m_ctxt->TEST.AFAIL)
			{
			case 0: break; // keep
			case 1: mask = D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED; break; // fbuf
			case 2: zwrite = true; break; // zbuf
			case 3: mask = D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED; break; // fbuf w/o alpha
			}

			hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
			hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
		}
*/
/*
		else if(m_de.PABE.PABE)
		{
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xfe);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
		}
*/
		hr = m_pD3DDev->EndScene();

		hr = m_pD3DDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);

if(m_de.PRIM.TME && m_nVertices == 6 && (m_ctxt->FRAME.Block()) == 0x00000 && m_ctxt->TEX0.TBP0 == 0x00e00)
{
	if(m_stats.GetFrame() > 1500)
	{
//		hr = D3DXSaveTextureToFile(_T("c:\\rtafter.bmp"), D3DXIFF_BMP, pRT, NULL);
//		if(t.m_pTexture) hr = D3DXSaveTextureToFile(_T("c:\\tx.bmp"), D3DXIFF_BMP, t.m_pTexture, NULL);
	}
}
		//////////////////////

		tex_t tex;
		tex.TEX0.TBP0 = m_ctxt->FRAME.Block();
		tex.TEX0.PSM = PSM_PSMCT32;
		tex.TEX0.CBP = -1;
		tex.CLAMP.WMS = tex.CLAMP.WMT = 0;
		m_tc.Update(tex, scale, pRT);

		//////////////////////
	}

	m_primtype = D3DPT_FORCE_DWORD;

	__super::FlushPrim();
}

void GSRendererHW::Flip()
{
	__super::Flip();

	HRESULT hr;

	CComPtr<IDirect3DSurface9> pBackBuff;
	hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuff);

	D3DSURFACE_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	pBackBuff->GetDesc(&bd);

	CRect dst(0, 0, bd.Width, bd.Height);

#ifdef DEBUG_RENDERTARGETS
	if(!m_pRenderWnds.IsEmpty())
	{
		POSITION pos = m_pRenderWnds.GetStartPosition();
		while(pos)
		{
			DWORD fbp;
			CGSWnd* pWnd = NULL;
			m_pRenderWnds.GetNextAssoc(pos, fbp, pWnd);

			CComPtr<IDirect3DTexture9> pRT;
			if(m_pRenderTargets.Lookup(fbp, pRT))
			{
				D3DSURFACE_DESC rd;
				ZeroMemory(&rd, sizeof(rd));
				hr = pRT->GetLevelDesc(0, &rd);

				hr = m_pD3DDev->SetTexture(0, pRT);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

				scale_t scale;
				DWORD ssize = sizeof(scale);
				pRT->GetPrivateData(GUID_NULL, &scale, &ssize);

				CSize size = m_rs.GetSize(m_rs.IsEnabled(1)?1:0);
				CRect src = CRect(0, 0, scale.x*size.cx, scale.y*size.cy);

				struct
				{
					float x, y, z, rhw;
					float tu, tv;
				}
				pVertices[] =
				{
					{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / rd.Width, (float)src.top / rd.Height},
					{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / rd.Width, (float)src.top / rd.Height},
					{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / rd.Width, (float)src.bottom / rd.Height},
					{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / rd.Width, (float)src.bottom / rd.Height},
				};

				for(int i = 0; i < countof(pVertices); i++)
				{
					pVertices[i].x -= 0.5;
					pVertices[i].y -= 0.5;
				}

				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

				hr = m_pD3DDev->BeginScene();
				hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
				hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
				hr = m_pD3DDev->EndScene();

				hr = m_pD3DDev->Present(NULL, NULL, pWnd->m_hWnd, NULL);

				hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

				CString str;
				str.Format(_T("PCSX2 - %05x"), fbp);
				if(fbp == (m_ctxt->FRAME.Block()))
				{
					// pWnd->SetFocus();
					str += _T(" - Drawing");
				}
				pWnd->SetWindowText(str);

				MSG msg;
				ZeroMemory(&msg, sizeof(msg));
				while(msg.message != WM_QUIT)
				{
					if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					else if(!(::GetAsyncKeyState(VK_RCONTROL)&0x80000000))
					{
						break;
					}
				}

				if(::GetAsyncKeyState(VK_LCONTROL)&0x80000000)
					Sleep(500);
			}
			else
			{
				if(IsWindow(pWnd->m_hWnd)) pWnd->DestroyWindow();
				m_pRenderWnds.RemoveKey(fbp);
			}

			CString str;
			str.Format(_T("PCSX2 - %05x"), m_ctxt->FRAME.Block());
			SetWindowText(m_hWnd, str);
		}
	}
	else
	{
		SetWindowText(m_hWnd, _T("PCSX2"));
	}
#endif

	struct
	{
		CComPtr<IDirect3DTexture9> pRT;
		D3DSURFACE_DESC rd;
		scale_t scale;
		CRect src;
	} rt[3];

	for(int i = 0; i < countof(rt); i++)
	{
		if(m_rs.IsEnabled(0) && i == 0 || m_rs.IsEnabled(1) && i == 1 || i == 2)
		{
			UINT32 FBP = i == 2 || (::GetAsyncKeyState(VK_SPACE)&0x80000000) ? m_ctxt->FRAME.Block() : (m_rs.DISPFB[i].FBP<<5);

			if(CSurfMap<IDirect3DTexture9>::CPair* pPair = m_pRenderTargets.PLookup(FBP))
			{
				rt[i].pRT = pPair->value;
				m_tc.ResetAge(pPair->key);
				ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
				hr = rt[i].pRT->GetLevelDesc(0, &rt[i].rd);

				DWORD ssize = sizeof(rt[i].scale);
				rt[i].pRT->GetPrivateData(GUID_NULL, &rt[i].scale, &ssize);

				CSize size = m_rs.GetSize(i < 2 ? i : m_rs.IsEnabled(1)?1:0);
				rt[i].src = CRect(0, 0, rt[i].scale.x*size.cx, rt[i].scale.y*size.cy);
			}
		}
	}

	bool fShiftField = m_rs.SMODE2.INT && !!(m_ctxt->XYOFFSET.OFY&0xf);
		// m_rs.CSRr.FIELD && m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/;

	struct
	{
		float x, y, z, rhw;
		float tu1, tv1;
		float tu2, tv2;
		float tu3, tv3;
	}
	pVertices[] =
	{
		{(float)dst.left, (float)dst.top, 0.5f, 2.0f, 
			(float)rt[0].src.left / rt[0].rd.Width, (float)rt[0].src.top / rt[0].rd.Height, 
			(float)rt[1].src.left / rt[1].rd.Width, (float)rt[1].src.top / rt[1].rd.Height, 
			(float)rt[2].src.left / rt[2].rd.Width, (float)rt[2].src.top / rt[2].rd.Height},
		{(float)dst.right, (float)dst.top, 0.5f, 2.0f, 
			(float)rt[0].src.right / rt[0].rd.Width, (float)rt[0].src.top / rt[0].rd.Height, 
			(float)rt[1].src.right / rt[1].rd.Width, (float)rt[1].src.top / rt[1].rd.Height, 
			(float)rt[2].src.right / rt[2].rd.Width, (float)rt[2].src.top / rt[2].rd.Height},
		{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, 
			(float)rt[0].src.left / rt[0].rd.Width, (float)rt[0].src.bottom / rt[0].rd.Height, 
			(float)rt[1].src.left / rt[1].rd.Width, (float)rt[1].src.bottom / rt[1].rd.Height, 
			(float)rt[2].src.left / rt[2].rd.Width, (float)rt[2].src.bottom / rt[2].rd.Height},
		{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, 
			(float)rt[0].src.right / rt[0].rd.Width, (float)rt[0].src.bottom / rt[0].rd.Height, 
			(float)rt[1].src.right / rt[1].rd.Width, (float)rt[1].src.bottom / rt[1].rd.Height, 
			(float)rt[2].src.right / rt[2].rd.Width, (float)rt[2].src.bottom / rt[2].rd.Height},
	};

	for(int i = 0; i < countof(pVertices); i++)
	{
		pVertices[i].x -= 0.5;
		pVertices[i].y -= 0.5;

		if(fShiftField)
		{
			pVertices[i].tv1 += rt[0].scale.y*0.5f / rt[0].rd.Height;
			pVertices[i].tv2 += rt[1].scale.y*0.5f / rt[1].rd.Height;
			pVertices[i].tv3 += rt[2].scale.y*0.5f / rt[2].rd.Height;
		}
	}

	hr = m_pD3DDev->SetTexture(0, rt[0].pRT);
	hr = m_pD3DDev->SetTexture(1, rt[1].pRT);
	hr = m_pD3DDev->SetTexture(2, rt[2].pRT);

	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	hr = m_pD3DDev->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 2);

	hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX3);

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	if(m_rs.IsEnabled(0) && m_rs.IsEnabled(1) && rt[0].pRT && rt[1].pRT) // RAO1 + RAO2
	{
		pPixelShader = m_pPixelShaders[11];
	}
	else if(m_rs.IsEnabled(0) && rt[0].pRT) // RAO1
	{
		pPixelShader = m_pPixelShaders[12];
	}
	else if(m_rs.IsEnabled(1) && rt[1].pRT) // RAO2
	{
		pPixelShader = m_pPixelShaders[13];
	}
	else if((m_rs.IsEnabled(0) || m_rs.IsEnabled(1)) && rt[2].pRT)
	{
		pPixelShader = m_pPixelShaders[14];
	}
	else
	{
		hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
		return;
	}

	if(!pPixelShader)
	{
		int stage = 0;

		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		stage++;

		if(m_rs.IsEnabled(0) && m_rs.IsEnabled(1) && rt[0].pRT && rt[1].pRT) // RAO1 + RAO2
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
		else if(m_rs.IsEnabled(0) && rt[0].pRT) // RAO1
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
		else if(m_rs.IsEnabled(1) && rt[1].pRT) // RAO2
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
		else if((m_rs.IsEnabled(0) || m_rs.IsEnabled(1)) && rt[2].pRT)
		{
			hr = m_pD3DDev->SetTexture(0, rt[2].pRT);
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 2);

			// FIXME
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
			for(int i = 0; i < countof(pVertices); i++)
			{
				pVertices[i].tu1 = pVertices[i].tu3;
				pVertices[i].tv1 = pVertices[i].tv3;
			}
		}

		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	hr = m_pD3DDev->BeginScene();
	hr = m_pD3DDev->SetPixelShader(pPixelShader);
	hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
	hr = m_pD3DDev->SetPixelShader(NULL);
	hr = m_pD3DDev->EndScene();

	hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
}

void GSRendererHW::EndFrame()
{
	m_tc.IncAge(m_pRenderTargets);
}

void GSRendererHW::InvalidateTexture(DWORD TBP0)
{
/*	CComPtr<IDirect3DTexture9> pRT;
	if(m_pRenderTargets.Lookup(TBP0, pRT))
*/	{
		m_tc.InvalidateByTBP(TBP0);
		m_tc.InvalidateByCBP(TBP0);
	}
}

bool GSRendererHW::CreateTexture(GSTexture& t)
{
	int tw = 1<<m_ctxt->TEX0.TW;
	int th = 1<<m_ctxt->TEX0.TH;
/*
	CComPtr<IDirect3DTexture9> pRT;
	if(!m_pRenderTargets.Lookup(m_ctxt->TEX0.TBP0, pRT))
	{
		if(m_lm.IsDirty(m_ctxt->TEX0))
		{
			LOG((_T("TBP0 dirty: %08x\n"), m_ctxt->TEX0.TBP0));
			m_tc.InvalidateByTBP(m_ctxt->TEX0.TBP0);
		}

		if(m_lm.IsPalDirty(m_ctxt->TEX0))
		{
			LOG((_T("CBP dirty: %08x\n"), m_ctxt->TEX0.CBP));
			m_tc.InvalidateByCBP(m_ctxt->TEX0.CBP);
		}
	}
*/
	tex_t tex;
	tex.TEX0 = m_ctxt->TEX0;
	tex.CLAMP = m_ctxt->CLAMP;
	tex.TEXA = m_de.TEXA;
	tex.TEXCLUT = m_de.TEXCLUT;

	if(m_tc.Lookup(tex, t))
		return(true);

//	LOG((_T("TBP0/CBP: %08x/%08x\n"), m_ctxt->TEX0.TBP0, m_ctxt->TEX0.CBP));

	m_lm.setupCLUT(m_ctxt->TEX0, m_de.TEXCLUT, m_de.TEXA);

	GSLocalMemory::readTexel rt = m_lm.GetReadTexel(m_ctxt->TEX0.PSM);

	CComPtr<IDirect3DTexture9> pTexture;
	HRESULT hr = m_pD3DDev->CreateTexture(tw, th, 0, 0 , D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
	if(FAILED(hr) || !pTexture) return(false);

	D3DLOCKED_RECT r;
	if(FAILED(hr = pTexture->LockRect(0, &r, NULL, 0)))
		return(false);

	BYTE* dst = (BYTE*)r.pBits;

	if((m_ctxt->CLAMP.WMS&2) || (m_ctxt->CLAMP.WMT&2))
	{
		int tx, ty;

		for(int y = 0, diff = r.Pitch - tw*4; y < th; y++, dst += diff)
		{
			for(int x = 0; x < tw; x++, dst += 4)
			{
				switch(m_ctxt->CLAMP.WMS)
				{
				default: tx = x; break;
				case 2: tx = x < m_ctxt->CLAMP.MINU ? m_ctxt->CLAMP.MINU : x > m_ctxt->CLAMP.MAXU ? m_ctxt->CLAMP.MAXU : x; break;
				case 3: tx = (x & m_ctxt->CLAMP.MINU) | m_ctxt->CLAMP.MAXU; break;
				}

				switch(m_ctxt->CLAMP.WMT)
				{
				default: ty = y; break;
				case 2: ty = y < m_ctxt->CLAMP.MINV ? m_ctxt->CLAMP.MINV : y > m_ctxt->CLAMP.MAXV ? m_ctxt->CLAMP.MAXV : y; break;
				case 3: ty = (y & m_ctxt->CLAMP.MINV) | m_ctxt->CLAMP.MAXV; break;
				}

				*(DWORD*)dst = (m_lm.*rt)(tx, ty, m_ctxt->TEX0, m_de.TEXA);
			}
		}
	}
	else
	{
		for(int y = 0, diff = r.Pitch - tw*4; y < th; y++, dst += diff)
			for(int x = 0; x < tw; x++, dst += 4)
				*(DWORD*)dst = (m_lm.*rt)(x, y, m_ctxt->TEX0, m_de.TEXA);
	}

	pTexture->UnlockRect(0);

	m_tc.Add(tex, scale_t(1, 1), pTexture);
	if(!m_tc.Lookup(tex, t)) // ehe
		ASSERT(0);
	
//	t.m_pTexture = pTexture;
//	t.m_tex = tex;
//	t.m_scale = scale_t(1, 1);

#ifdef DEBUG_SAVETEXTURES
	CString fn;
	fn.Format(_T("c:\\%08I64x_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d-%I64d_%I64d-%I64d.bmp"), 
		m_ctxt->TEX0.TBP0, m_ctxt->TEX0.PSM, m_ctxt->TEX0.TBW, 
		m_ctxt->TEX0.TW, m_ctxt->TEX0.TH,
		m_ctxt->CLAMP.WMS, m_ctxt->CLAMP.WMT, m_ctxt->CLAMP.MINU, m_ctxt->CLAMP.MAXU, m_ctxt->CLAMP.MINV, m_ctxt->CLAMP.MAXV);
	D3DXSaveTextureToFile(fn, D3DXIFF_BMP, pTexture, NULL);
#endif

	return(true);
}

