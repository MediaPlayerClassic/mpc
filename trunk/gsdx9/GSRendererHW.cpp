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
#include "GSRendererHW.h"

inline BYTE SCALE_ALPHA(BYTE a) 
{
	return (((a)&0x80)?0xff:((a)<<1));
}

//

GSRendererHW::GSRendererHW()
	: m_width(1024)
	, m_height(1024)
{
}

GSRendererHW::~GSRendererHW()
{
}

void GSRendererHW::ResetState()
{
	m_tc.RemoveAll();
	m_pRTs.RemoveAll();
	m_pDSs.RemoveAll();

	__super::ResetState();
}

HRESULT GSRendererHW::ResetDevice(bool fForceWindowed)
{
	m_pRTs.RemoveAll();
	m_pDSs.RemoveAll();
	m_tc.RemoveAll();

	return __super::ResetDevice(fForceWindowed);
}

void GSRendererHW::VertexKick(bool skip)
{
	static const float one_over_log_2pow32 = 1.0f / (log(2.0f)*32);

	GSVertexHW& v = m_vl.AddTail();

	v.x = (float)((int)m_v.XYZ.X - (int)m_context->XYOFFSET.OFX) * (1.0f/16);
	v.y = (float)((int)m_v.XYZ.Y - (int)m_context->XYOFFSET.OFY) * (1.0f/16);
	//if(m_v.XYZ.Z && m_v.XYZ.Z < 0x100) m_v.XYZ.Z = 0x100;
	//v.z = 1.0f * (m_v.XYZ.Z>>8)/(UINT_MAX>>8);
	v.z = log(1.0f + m_v.XYZ.Z) * one_over_log_2pow32;
	//v.z = (float)m_v.XYZ.Z / UINT_MAX;
	//v.rhw = v.z ? 1.0f/v.z : 1.0f;
	v.rhw = m_v.RGBAQ.Q > 0 ? m_v.RGBAQ.Q : 1.0f; // TODO
	//v.rhw = m_v.RGBAQ.Q;

	v.color = m_v.RGBAQ.ai32[0];

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.tu = (float)(int)m_v.UV.U / (16 << m_context->TEX0.TW);
			v.tv = (float)(int)m_v.UV.V / (16 << m_context->TEX0.TH);
			v.rhw = 1.0f;
		}
		else
		{
			float w = m_v.RGBAQ.Q ? 1.0f / m_v.RGBAQ.Q : 1.0f;
			v.tu = m_v.ST.S * w;
			v.tv = m_v.ST.T * w;
		}
	}
	else
	{
		v.a = SCALE_ALPHA(v.a);
	}

	v.fog = (m_pPRIM->FGE ? m_v.FOG.F : 0xff) << 24;

	__super::VertexKick(skip);
}

int GSRendererHW::DrawingKick(bool skip)
{
	GSVertexHW* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	CRect sc(m_context->SCISSOR.SCAX0, m_context->SCISSOR.SCAY0, m_context->SCISSOR.SCAX1+1, m_context->SCISSOR.SCAY1+1);

	switch(m_pPRIM->PRIM)
	{
	case GS_POINTLIST:
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left
		|| pVertices[nVertices-1].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom)
			return 0;
		break;
	case GS_LINELIST:
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left && pVertices[nVertices-2].x < sc.left
		|| pVertices[nVertices-1].y < sc.top && pVertices[nVertices-2].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right && pVertices[nVertices-2].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom && pVertices[nVertices-2].y >= sc.bottom)
			return 0;
		break;
	case GS_LINESTRIP:
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left && pVertices[nVertices-2].x < sc.left
		|| pVertices[nVertices-1].y < sc.top && pVertices[nVertices-2].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right && pVertices[nVertices-2].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom && pVertices[nVertices-2].y >= sc.bottom)
			return 0;
		break;
	case GS_TRIANGLELIST:
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left && pVertices[nVertices-2].x < sc.left && pVertices[nVertices-3].x < sc.left
		|| pVertices[nVertices-1].y < sc.top && pVertices[nVertices-2].y < sc.top && pVertices[nVertices-3].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right && pVertices[nVertices-2].x >= sc.right && pVertices[nVertices-3].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom && pVertices[nVertices-2].y >= sc.bottom && pVertices[nVertices-3].y >= sc.bottom)
			return 0;
		break;
	case GS_TRIANGLESTRIP:
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left && pVertices[nVertices-2].x < sc.left && pVertices[nVertices-3].x < sc.left
		|| pVertices[nVertices-1].y < sc.top && pVertices[nVertices-2].y < sc.top && pVertices[nVertices-3].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right && pVertices[nVertices-2].x >= sc.right && pVertices[nVertices-3].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom && pVertices[nVertices-2].y >= sc.bottom && pVertices[nVertices-3].y >= sc.bottom)
			return 0;
		break;
	case GS_TRIANGLEFAN:
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left && pVertices[nVertices-2].x < sc.left && pVertices[nVertices-3].x < sc.left
		|| pVertices[nVertices-1].y < sc.top && pVertices[nVertices-2].y < sc.top && pVertices[nVertices-3].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right && pVertices[nVertices-2].x >= sc.right && pVertices[nVertices-3].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom && pVertices[nVertices-2].y >= sc.bottom && pVertices[nVertices-3].y >= sc.bottom)
			return 0;
		break;
	case GS_SPRITE:
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < sc.left && pVertices[nVertices-2].x < sc.left
		|| pVertices[nVertices-1].y < sc.top && pVertices[nVertices-2].y < sc.top
		|| pVertices[nVertices-1].x >= sc.right && pVertices[nVertices-2].x >= sc.right
		|| pVertices[nVertices-1].y >= sc.bottom && pVertices[nVertices-2].y >= sc.bottom)
			return 0;
		nVertices += 2;
/*
		float lod;
		if(m_context->TEX1.LCM) lod = -log(pVertices[nVertices-1].rhw)/log(2.0f) * (1 << m_context->TEX1.L) + m_context->TEX1.K;
		else lod = m_context->TEX1.K;

		int filter;
		if(lod < 0) filter = m_context->TEX1.MMAG&1;
		else filter = m_context->TEX1.MMIN&1;

//		if(!filter)
		{
			pVertices[nVertices-2].x -= 0.5f;
			pVertices[nVertices-2].y -= 0.5f;
			pVertices[nVertices-1].x -= 0.5f;
			pVertices[nVertices-1].y -= 0.5f;
		}
*/
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
	default:
		//ASSERT(0);
		m_vl.RemoveAll();
		return 0;
	}

	if(skip || !m_regs.IsEnabled(0) && !m_regs.IsEnabled(1))
	{
		return 0;
	}

	if(!m_pPRIM->IIP)
	{
		pVertices[0].color = pVertices[nVertices-1].color;

		if(m_pPRIM->PRIM == 6)
		{
			pVertices[3].color = pVertices[5].color;
		}

		/*for(int i = nVertices-1; i > 0; i--)
			pVertices[i-1].color = pVertices[i].color;*/
	}

	return nVertices;
}

void GSRendererHW::FlushPrim()
{
	if(m_nVertices > 0 && !(m_pPRIM->TME && HasSharedBits(m_context->TEX0.TBP0, m_context->TEX0.PSM, m_context->FRAME.Block(), m_context->FRAME.PSM)))
	do
	{
		D3DPRIMITIVETYPE primtype;

		int nPrims = 0;

		switch(m_pPRIM->PRIM)
		{
		case GS_POINTLIST:
			primtype = D3DPT_POINTLIST;
			nPrims = m_nVertices;
			break;
		case GS_LINELIST: 
		case GS_LINESTRIP:
			primtype = D3DPT_LINELIST;
			nPrims = m_nVertices / 2; 
			break;
		case GS_TRIANGLELIST: 
		case GS_TRIANGLESTRIP: 
		case GS_TRIANGLEFAN: 
		case GS_SPRITE:
			primtype = D3DPT_TRIANGLELIST;
			nPrims = m_nVertices / 3; 
			break;
		default:
#ifdef _DEBUG
			ASSERT(0);
			break;
#else
			__assume(0);
#endif
		}

		m_perfmon.IncCounter(GSPerfMon::c_prim, nPrims);

		//////////////////////

		HRESULT hr;

		scale_t scale(
			(float)m_width / (m_context->FRAME.FBW*64), 
//			(float)m_width / m_regs.GetFrameSize(m_regs.IsEnabled(1)?1:0).cx, 
			(float)m_height / m_regs.GetDisplaySize(m_regs.IsEnabled(1)?1:0).cy); 

		//////////////////////

		CComPtr<IDirect3DTexture9> pRT;
		CComPtr<IDirect3DSurface9> pDS;

		bool fClearRT = false;
		bool fClearDS = false;

		if(!m_pRTs.Lookup(m_context->FRAME.Block(), pRT))
		{
			hr = m_pD3DDev->CreateTexture(m_width, m_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);
			if(S_OK != hr) {ASSERT(0); return;}
			m_pRTs[m_context->FRAME.Block()] = pRT;
			fClearRT = true;
		}

		if(!m_pDSs.Lookup(m_context->ZBUF.ZBP, pDS))
		{
			hr = m_pD3DDev->CreateDepthStencilSurface(m_width, m_height, m_fmtDepthStencil, D3DMULTISAMPLE_NONE, 0, FALSE, &pDS, NULL);
			if(S_OK != hr) {ASSERT(0); return;}
			m_pDSs[m_context->ZBUF.ZBP] = pDS;
			fClearDS = true;
		}

		if(!pRT || !pDS) {ASSERT(0); return;}

		//////////////////////

		GSTextureBase t;

		if(m_pPRIM->TME)
		{
			bool fFetched = 
				m_fPalettizedTextures && m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) ? m_tc.FetchP(this, t) : 
				m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) ? m_tc.FetchNP(this, t) :
				m_tc.Fetch(this, t);

			if(!fFetched) break;

			if(nPrims > 100 && t.m_pPalette) // TODO: find the optimal value for nPrims > ?
			{
				CComPtr<IDirect3DTexture9> pRT;

				hr = m_pD3DDev->CreateTexture(t.m_desc.Width, t.m_desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);

				CComPtr<IDirect3DSurface9> pRTSurf;

				hr = pRT->GetSurfaceLevel(0, &pRTSurf);

				hr = m_pD3DDev->SetRenderTarget(0, pRTSurf);
				hr = m_pD3DDev->SetDepthStencilSurface(NULL);

				hr = m_pD3DDev->SetTexture(0, t.m_pTexture);
				hr = m_pD3DDev->SetTexture(1, t.m_pPalette);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
				hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
				hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
				hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
				hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
				hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

				hr = m_pD3DDev->SetPixelShader(m_pHLSLTFX[37]);

				struct
				{
					float x, y, z, rhw;
					float tu, tv;
				}
				pVertices[] =
				{
					{0, 0, 0.5f, 2.0f, 0, 0},
					{(float)t.m_desc.Width, 0, 0.5f, 2.0f, 1, 0},
					{0, (float)t.m_desc.Height, 0.5f, 2.0f, 0, 1},
					{(float)t.m_desc.Width, (float)t.m_desc.Height, 0.5f, 2.0f, 1, 1},
				};

				hr = m_pD3DDev->BeginScene();
				hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
				hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
				hr = m_pD3DDev->EndScene();

				t.m_pTexture = pRT;
				t.m_pPalette = NULL;

				t.m_pTexture->GetLevelDesc(0, &t.m_desc);
			}
		}

		//////////////////////
/*

static int n = 0;
static bool dump = false;

if(m_perfmon.GetFrame() == 200)
{
	if(n > 20000 && !m_pPRIM->TME)
	{
		dump = true;
	}

	if(dump)
	{
		CString str;
		str.Format(_T("c:\\temp2\\_%05d_%05x.bmp"), n, m_context->TEX0.TBP0);
		// if(m_pPRIM->TME) ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, t.m_pTexture, NULL);
		str.Format(_T("c:\\temp2\\_%05drt0_%05x.bmp"), n, m_context->FRAME.FBP);
		// ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, pRT, NULL);
	}
}
*/
		hr = m_pD3DDev->BeginScene();

		//////////////////////

		CComPtr<IDirect3DSurface9> pSurf;
		hr = pRT->GetSurfaceLevel(0, &pSurf);
		hr = m_pD3DDev->SetRenderTarget(0, pSurf);
		hr = m_pD3DDev->SetDepthStencilSurface(pDS);
		if(fClearRT) hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		if(fClearDS) hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

		D3DSURFACE_DESC rd;
		memset(&rd, 0, sizeof(rd));
		pRT->GetLevelDesc(0, &rd);

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_SHADEMODE, m_pPRIM->IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT);

		//////////////////////

		SetupTexture(t);

		//////////////////////

		SetupAlphaBlend();

		//////////////////////

		SetupColorMask();

		//////////////////////

		SetupZBuffer();

		//////////////////////

		SetupAlphaTest();

		//////////////////////

		SetupScissor(scale);

		scale.Set(pRT);

		//////////////////////

		// ASSERT(!m_env.PABE.PABE); // bios
		// ASSERT(!m_context->FBA.FBA); // bios
		// ASSERT(!m_context->TEST.DATE); // sfex3 (after the capcom logo), vf4 (first menu fading in)

		//////////////////////

		// TODO: this could be done in a vertex shader, if we had one...

		{
			GSVertexHW* v = m_pVertices;

			for(int i = 0, j = m_nVertices; i < j; i++)
			{
				v[i].x *= scale.x;
				v[i].y *= scale.y;
			}

			if(m_pPRIM->TME)
			{
				float tsx = 1.0f, tsy = 1.0f;

				tsx = 1.0f * (1 << m_context->TEX0.TW) / t.m_desc.Width * t.m_scale.x;
				tsy = 1.0f * (1 << m_context->TEX0.TH) / t.m_desc.Height * t.m_scale.y;

				ASSERT(abs(tsx - 1.0f) < 0.005 && abs(tsy - 1.0f) < 0.005);

				if(tsx != 1 || tsy != 1)
				{
					for(int i = 0, j = m_nVertices; i < j; i++)
					{
						// FIXME
						float base, fract;
						fract = modf(v[i].tu, &base);
						fract *= tsx;
						//ASSERT(-1 <= fract && fract <= 1.01);
						v[i].tu = base + fract;
						fract = modf(v[i].tv, &base);
						fract *= tsy;
						//ASSERT(-1 <= fract && fract <= 1.01);
						v[i].tv = base + fract;
					}
				}
			}

			if(m_pPRIM->FGE)
			{
				for(int i = 0, j = m_nVertices; i < j; i++)
				{
					v[i].fog = (v[i].fog & 0xff000000) | (m_env.FOGCOL.ai32[0] & 0x00ffffff);
					// D3DCOLOR_ARGB(v[i].fog >> 24, m_env.FOGCOL.FCB, m_env.FOGCOL.FCG, m_env.FOGCOL.FCR)
				}
			}
		}

		hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1);

		if(1)//!m_env.PABE.PABE)
		{
			hr = m_pD3DDev->DrawPrimitiveUP(primtype, nPrims, m_pVertices, sizeof(GSVertexHW));
		}
/*		else
		{
			ASSERT(!m_context->TEST.ATE); // TODO

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xfe);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			hr = m_pD3DDev->DrawPrimitive(primtype, 0, nPrims);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			hr = m_pD3DDev->DrawPrimitive(primtype, 0, nPrims);
		}
*/
		if(m_context->TEST.ATE && m_context->TEST.AFAIL && m_context->TEST.ATST != 1)
		{
			ASSERT(!m_env.PABE.PABE);

			static const DWORD iafunc[] = {D3DCMP_ALWAYS, D3DCMP_NEVER, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL};

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, iafunc[m_context->TEST.ATST]);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_context->TEST.AREF));

			DWORD mask = 0;
			bool zwrite = false;

			hr = m_pD3DDev->GetRenderState(D3DRS_COLORWRITEENABLE, &mask);

			switch(m_context->TEST.AFAIL)
			{
			case 0: mask = 0; break; // keep
			case 1: break; // fbuf
			case 2: mask = 0; zwrite = true; break; // zbuf
			case 3: mask &= ~D3DCOLORWRITEENABLE_ALPHA; break; // fbuf w/o alpha
			default: __assume(0);
			}

			hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
			hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

			if(mask || zwrite)
			{
				hr = m_pD3DDev->DrawPrimitiveUP(primtype, nPrims, m_pVertices, sizeof(GSVertexHW));
			}
		}

		hr = m_pD3DDev->EndScene();
/*
if(m_perfmon.GetFrame() == 200)
{
	if(dump)
	{
		CString str;
		str.Format(_T("c:\\temp2\\_%05drt1_%05x.bmp"), n, m_context->FRAME.FBP);
		// ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, pRT, NULL);
	}

	n++;
}

*/
		//////////////////////

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = m_context->FRAME.Block();
		TEX0.TBW = m_context->FRAME.FBW;
		TEX0.PSM = m_context->FRAME.PSM;

		m_tc.AddRT(TEX0, pRT, scale); 
	}
	while(0);

	__super::FlushPrim();
}

void GSRendererHW::Flip()
{
	HRESULT hr;

	FlipInfo src[2];

	for(int i = 0; i < countof(src); i++)
	{
		if(!m_regs.IsEnabled(i))
		{
			continue;
		}

		DWORD FBP = m_regs.pDISPFB[i]->FBP << 5;

		CSurfMap<IDirect3DTexture9>::CPair* pPair = m_pRTs.Lookup(FBP);

		if(!pPair)
		{
			POSITION pos = m_pRTs.GetStartPosition(); 
			
			while(pos)
			{
				CSurfMap<IDirect3DTexture9>::CPair* pPair2 = m_pRTs.GetNext(pos);

				if(pPair2->m_key <= FBP && (!pPair || pPair2->m_key >= pPair->m_key))
				{
					pPair = pPair2;
				}
			}
		}

		if(!pPair)
		{
			CComPtr<IDirect3DTexture9> pRT;

			if(S_OK == m_pD3DDev->CreateTexture(m_width, m_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL))
			{
				m_pRTs[FBP] = pRT;

				EXECUTE_ASSERT(pPair = m_pRTs.Lookup(FBP));

				CRect r = m_regs.GetFrameRect(m_regs.IsEnabled(1)?1:0);

				scale_t scale(
					(float)m_width / (m_regs.pDISPFB[i]->FBW*64), 
			//		(float)m_width / m_regs.GetDisplayRect(m_regs.IsEnabled(1)?1:0).right, 
					(float)m_height / m_regs.GetDisplaySize(m_regs.IsEnabled(1)?1:0).cy);

				scale.Set(pRT);

				GIFRegTEX0 TEX0;

				TEX0.TBP0 = m_regs.pDISPFB[i]->FBP;
				TEX0.TBW = m_regs.pDISPFB[i]->FBW;
				TEX0.PSM = m_regs.pDISPFB[i]->PSM;

				m_tc.AddRT(TEX0, pRT, scale);

				GIFRegBITBLTBUF BITBLTBUF;

				BITBLTBUF.DBP = TEX0.TBP0;
				BITBLTBUF.DBW = TEX0.TBW;
				BITBLTBUF.DPSM = TEX0.PSM;

				m_tc.InvalidateTexture(this, BITBLTBUF, r);
			}
		}

		if(pPair)
		{
			m_tc.ResetAge(pPair->m_key);

			src[i].tex = pPair->m_value;

			memset(&src[i].desc, 0, sizeof(src[i].desc));

			hr = src[i].tex->GetLevelDesc(0, &src[i].desc);

			src[i].scale.Get(src[i].tex);
		}
	}

	FinishFlip(src, m_regs.pSMODE2->INT && m_regs.pSMODE2->FFMD ? 0.5f : 1.0f);

	m_tc.IncAge(m_pRTs);
}

void GSRendererHW::InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r)
{
	m_tc.InvalidateTexture(this, BITBLTBUF, &r);
}

void GSRendererHW::InvalidateLocalMem(DWORD TBP0, DWORD BW, DWORD PSM, CRect r)
{
	m_tc.InvalidateLocalMem(this, TBP0, BW, PSM, &r);
}

void GSRendererHW::MinMaxUV(int w, int h, CRect& r)
{
	r.SetRect(0, 0, w, h);

	uvmm_t uv;
	CSize bsm;

	if(m_context->CLAMP.WMS < 3 || m_context->CLAMP.WMT < 3)
	{
		UVMinMax(m_nVertices, (vertex_t*)m_pVertices, &uv);
		CSize bs = GSLocalMemory::m_psmtbl[m_context->TEX0.PSM].bs;
		bsm.SetSize(bs.cx-1, bs.cy-1);
	}

	// FIXME: region clamp returns the right rect but we should still update the whole texture later in TC...

	if(m_context->CLAMP.WMS < 3)
	{
		if(m_context->CLAMP.WMS == 0)
		{
			float fmin = floor(uv.umin);
			float fmax = floor(uv.umax);

			if(fmin != fmax) {uv.umin = 0; uv.umax = 1.0f;}
			else {uv.umin -= fmin; uv.umax -= fmax;}

			// FIXME
			if(uv.umin == 0 && uv.umax != 1.0f) uv.umax = 1.0f;
		}
		else if(m_context->CLAMP.WMS == 1)
		{
			if(uv.umin < 0) uv.umin = 0;
			if(uv.umax > 1.0f) uv.umax = 1.0f;
		}
		else if(m_context->CLAMP.WMS == 2)
		{
			float minu = 1.0f * m_context->CLAMP.MINU / w;
			float maxu = 1.0f * m_context->CLAMP.MAXU / w;
			if(uv.umin < minu) uv.umin = minu;
			if(uv.umax > maxu) uv.umax = maxu;
		}

		r.left = max((int)(uv.umin * w) & ~bsm.cx, 0);
		r.right = min(((int)(uv.umax * w) + bsm.cx + 1) & ~bsm.cx, w);
	}

	if(m_context->CLAMP.WMT < 3)
	{
		if(m_context->CLAMP.WMT == 0)
		{
			float fmin = floor(uv.vmin);
			float fmax = floor(uv.vmax);

			if(fmin != fmax) {uv.vmin = 0; uv.vmax = 1.0f;}
			else {uv.vmin -= fmin; uv.vmax -= fmax;}

			// FIXME
			if(uv.vmin == 0 && uv.vmax != 1.0f) uv.vmax = 1.0f;
		}
		else if(m_context->CLAMP.WMT == 1)
		{
			if(uv.vmin < 0) uv.vmin = 0;
			if(uv.vmax > 1.0f) uv.vmax = 1.0f;
		}
		else if(m_context->CLAMP.WMT == 2)
		{
			float minv = 1.0f * m_context->CLAMP.MINV / h;
			float maxv = 1.0f * m_context->CLAMP.MAXV / h;
			if(uv.vmin < minv) uv.vmin = minv;
			if(uv.vmax > maxv) uv.vmax = maxv;
		}
		
		r.top = max((int)(uv.vmin * h) & ~bsm.cy, 0);
		r.bottom = min(((int)(uv.vmax * h) + bsm.cy + 1) & ~bsm.cy, h);
	}

	//ASSERT(r.left <= r.right);
	//ASSERT(r.top <= r.bottom);
}

void GSRendererHW::SetupTexture(const GSTextureBase& t)
{
	HRESULT hr;

	int tw = 0, th = 0;
	float rw = 0, rh = 0;

	IDirect3DPixelShader9* pPixelShader = NULL;

	if(m_pPRIM->TME && t.m_pTexture)
	{
		tw = t.m_desc.Width;
		th = t.m_desc.Height;
		rw = 1.0f / tw;
		rh = 1.0f / th;

		hr = m_pD3DDev->SetTexture(0, t.m_pTexture);
		hr = m_pD3DDev->SetTexture(1, t.m_pPalette);

		D3DTEXTUREADDRESS u, v;

		switch(m_context->CLAMP.WMS)
		{
		case 0: case 3: u = D3DTADDRESS_WRAP; break; // repeat
		case 1: case 2: u = D3DTADDRESS_CLAMP; break; // clamp
		default: __assume(0);
		}

		switch(m_context->CLAMP.WMT)
		{
		case 0: case 3: v = D3DTADDRESS_WRAP; break; // repeat
		case 1: case 2: v = D3DTADDRESS_CLAMP; break; // clamp
		default: __assume(0);
		}

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, u);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, v);

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, t.m_pPalette ? D3DTEXF_POINT : m_nTextureFilter);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, t.m_pPalette ? D3DTEXF_POINT : m_nTextureFilter);
		hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MAGFILTER, t.m_pPalette ? D3DTEXF_POINT : m_nTextureFilter);
		hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MINFILTER, t.m_pPalette ? D3DTEXF_POINT : m_nTextureFilter);

		if(!pPixelShader && m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) && m_pHLSLTFX[m_context->TEX0.TFX])
		{
			int i = m_context->TEX0.TFX;

			switch(t.m_desc.Format)
			{
			default: 
				ASSERT(0);
				break;
			case D3DFMT_A8R8G8B8:
				//ASSERT(m_context->TEX0.PSM != PSM_PSMCT24); // format must be D3DFMT_X8R8G8B8 for PSM_PSMCT24
				//if(m_context->TEX0.PSM == PSM_PSMCT24) {i += 4; if(m_env.TEXA.AEM) i += 4;}
				if(t.m_pPalette && m_context->TEX0.PSM == PSM_PSMT8H) i += 32;
				break;
			case D3DFMT_X8R8G8B8:
				i += 4; if(m_env.TEXA.AEM) i += 4;
				break;
			case D3DFMT_A1R5G5B5:
				i += 12; if(m_env.TEXA.AEM) i += 4; 
				break;
			case D3DFMT_L8:
				i += 24;
				ASSERT(t.m_pPalette);
				break;
			}

			pPixelShader = m_pHLSLTFX[i];
		}
		else
		{
			ASSERT(0);
		}
	}
	else
	{
		hr = m_pD3DDev->SetTexture(0, NULL);
		hr = m_pD3DDev->SetTexture(1, NULL);

		if(!pPixelShader && m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) && m_pHLSLTFX[36])
		{
			pPixelShader = m_pHLSLTFX[36];
		}
		else
		{
			ASSERT(0);
		}
	}

	float fConstData[][4] = 
	{
		{(float)m_context->TEX0.TCC - 0.5f, t.m_fRT ? 1.0f : 2.0f, min(2.0f * m_env.TEXA.TA0 / 255, 1), min(2.0f * m_env.TEXA.TA1 / 255, 1)},
		{(float)tw, (float)th, 0, 0},
		{rw, rh, 0, 0},
		{rw, 0, 0, 0},
		{0, rh, 0, 0},
	};

	hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

	hr = m_pD3DDev->SetPixelShader(pPixelShader);
}

void GSRendererHW::SetupAlphaBlend()
{
	HRESULT hr;

	DWORD ABE = FALSE;

	hr = m_pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &ABE);

	bool fABE = m_pPRIM->ABE || (m_pPRIM->PRIM == 1 || m_pPRIM->PRIM == 2) && m_pPRIM->AA1; // FIXME

	if(fABE != !!ABE)
	{
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, fABE);
	}

	if(fABE)
	{
		// (A:Cs/Cd/0 - B:Cs/Cd/0) * C:As/Ad/FIX + D:Cs/Cd/0

		BYTE FIX = SCALE_ALPHA(m_context->ALPHA.FIX);

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

		int i = (((m_context->ALPHA.A&3)*3+(m_context->ALPHA.B&3))*3+(m_context->ALPHA.C&3))*3+(m_context->ALPHA.D&3);

		ASSERT(m_context->ALPHA.A != 3);
		ASSERT(m_context->ALPHA.B != 3);
		ASSERT(m_context->ALPHA.C != 3);
		ASSERT(m_context->ALPHA.D != 3);

		ASSERT(!blendmap[i].bogus);

		hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOP, blendmap[i].op);
		hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, blendmap[i].src);
		hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, blendmap[i].dst);
	}
}

void GSRendererHW::SetupColorMask()
{
	HRESULT hr;

	// close approx., to be tested...
	int mask = D3DCOLORWRITEENABLE_RGBA;
	if((m_context->FRAME.FBMSK&0xff000000) == 0xff000000) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
	if((m_context->FRAME.FBMSK&0x00ff0000) == 0x00ff0000) mask &= ~D3DCOLORWRITEENABLE_BLUE;
	if((m_context->FRAME.FBMSK&0x0000ff00) == 0x0000ff00) mask &= ~D3DCOLORWRITEENABLE_GREEN;
	if((m_context->FRAME.FBMSK&0x000000ff) == 0x000000ff) mask &= ~D3DCOLORWRITEENABLE_RED;
	//if(m_context->FRAME.PSM == PSM_PSMCT24) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
	hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

	// ASSERT(m_context->FRAME.FBMSK == 0); // wild arms (also 8H+pal on RT...)
}

void GSRendererHW::SetupZBuffer()
{
	HRESULT hr;

	if(m_context->TEST.ZTE && m_context->TEST.ZTST == 1 && m_context->ZBUF.ZMSK)
	{
		hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		return;
	}

	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, m_context->TEST.ZTE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, !m_context->ZBUF.ZMSK);

	if(m_context->TEST.ZTE)
	{
		static const DWORD zfunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_GREATEREQUAL, D3DCMP_GREATER};

		hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, zfunc[m_context->TEST.ZTST]);
//		hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

		/*
		// FIXME
		if(m_context->ZBUF.ZMSK && m_context->TEST.ZTST == 1)
		{
			for(int i = 0; i < m_nVertices; i++)
				m_pVertices[i].z = 0;
		}
		*/
	}
}

void GSRendererHW::SetupAlphaTest()
{
	HRESULT hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, m_context->TEST.ATE);

	if(m_context->TEST.ATE)
	{
		static const DWORD afunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL};

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, afunc[m_context->TEST.ATST]);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_context->TEST.AREF));
	}
}

void GSRendererHW::SetupScissor(scale_t& s)
{
	HRESULT hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	CRect r(
		(int)(s.x * m_context->SCISSOR.SCAX0),
		(int)(s.y * m_context->SCISSOR.SCAY0), 
		(int)(s.x * (m_context->SCISSOR.SCAX1+1)),
		(int)(s.y * (m_context->SCISSOR.SCAY1+1)));

	r &= CRect(0, 0, m_width, m_height);

	hr = m_pD3DDev->SetScissorRect(r);
}
