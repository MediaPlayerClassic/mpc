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
#include "GSStateSoft.h"

GSStateSoft::GSStateSoft(HWND hWnd, HRESULT& hr)
	: GSState(hWnd, hr)
{
	if(FAILED(hr)) return;

	m_pVertices = (GSSoftVertex*)_aligned_malloc(sizeof(GSSoftVertex) * (m_nMaxVertices = 256), 16);
}

GSStateSoft::~GSStateSoft()
{
	_aligned_free(m_pVertices);
}

void GSStateSoft::Reset()
{
	m_primtype = PRIM_NONE;
	m_nVertices = m_nPrims = 0;

	__super::Reset();
}

void GSStateSoft::NewPrim()
{
	m_vl.RemoveAll();
}

void GSStateSoft::VertexKick(bool fSkip)
{
	LOG((_T("VertexKick(%d)\n"), fSkip));

	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	GSSoftVertex v;

	v.x = ((float)m_v.XYZ.X - ctxt->XYOFFSET.OFX) / 16;
	v.y = ((float)m_v.XYZ.Y - ctxt->XYOFFSET.OFY) / 16;
	v.z = (float)m_v.XYZ.Z / UINT_MAX;
	v.w = m_v.RGBAQ.Q == 0 ? 1.0f : m_v.RGBAQ.Q;

	v.r = (float)m_v.RGBAQ.R;
	v.g = (float)m_v.RGBAQ.G;
	v.b = (float)m_v.RGBAQ.B;
	v.a = (float)m_v.RGBAQ.A;

	v.fog = (float)m_v.FOG.F;

	if(m_de.PRIM.TME)
	{
		if(m_de.PRIM.FST)
		{
			v.u = (float)m_v.UV.U / (16<<ctxt->TEX0.TW);
			v.v = (float)m_v.UV.V / (16<<ctxt->TEX0.TH);
			v.w = 1.0f;
		}
		else
		{
			v.u = m_v.ST.S;
			v.v = m_v.ST.T;
		}
	}

	m_vl.AddTail(v);

	static const int vmin[8] = {1, 2, 2, 3, 3, 3, 2, 0};
	while(m_vl.GetCount() >= vmin[m_de.PRIM.PRIM])
	{
		if(m_nVertices+6 > m_nMaxVertices)
		{
			GSSoftVertex* pVertices = (GSSoftVertex*)_aligned_malloc(sizeof(GSSoftVertex) * (m_nMaxVertices <<= 1), 16);
			memcpy(pVertices, m_pVertices, m_nVertices*sizeof(GSSoftVertex));
			_aligned_free(m_pVertices);
			m_pVertices = pVertices;
		}

		DrawingKick(fSkip);
	}
}

void GSStateSoft::DrawingKick(bool fSkip)
{
	LOG((_T("DrawingKick %d\n"), m_de.PRIM.PRIM));

	if(m_PRIM != m_de.PRIM.PRIM && m_nVertices > 0) FlushPrim();
	m_PRIM = m_de.PRIM.PRIM;

	GSSoftVertex* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	LOG2((_T("Prim %05x %05x %04x\n"), 
		ctxt->FRAME.Block(), m_de.PRIM.TME ? (UINT32)ctxt->TEX0.TBP0 : 0xfffff,
		(m_de.PRIM.ABE || (m_PRIM == 1 || m_PRIM == 2) && m_de.PRIM.AA1)
			? ((ctxt->ALPHA.A<<12)|(ctxt->ALPHA.B<<8)|(ctxt->ALPHA.C<<4)|ctxt->ALPHA.D) 
			: 0xffff));

	switch(m_PRIM)
	{
	case 3: // triangle list
		m_primtype = PRIM_TRIANGLE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriList")));
		LOGV((pVertices[1], _T("TriList")));
		LOGV((pVertices[2], _T("TriList")));
		break;
	case 4: // triangle strip
		m_primtype = PRIM_TRIANGLE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriStrip")));
		LOGV((pVertices[1], _T("TriStrip")));
		LOGV((pVertices[2], _T("TriStrip")));
		break;
	case 5: // triangle fan
		m_primtype = PRIM_TRIANGLE;
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriFan")));
		LOGV((pVertices[1], _T("TriFan")));
		LOGV((pVertices[2], _T("TriFan")));
		break;
	case 6: // sprite
		m_primtype = PRIM_SPRITE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		nVertices += 2;
		// ASSERT(pVertices[0].z == pVertices[1].z);
		pVertices[0].z = pVertices[1].z;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].y = pVertices[0].y;
		pVertices[1].v = pVertices[0].v;
		pVertices[2].x = pVertices[0].x;
		pVertices[2].u = pVertices[0].u;
		LOGV((pVertices[0], _T("Sprite")));
		LOGV((pVertices[1], _T("Sprite")));
		LOGV((pVertices[2], _T("Sprite")));
		LOGV((pVertices[3], _T("Sprite")));
		break;
	case 1: // line
		m_primtype = PRIM_LINE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineList")));
		LOGV((pVertices[1], _T("LineList")));
		break;
	case 2: // line strip
		m_primtype = PRIM_LINE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineStrip")));
		LOGV((pVertices[1], _T("LineStrip")));
		break;
	case 0: // point
		m_primtype = PRIM_POINT;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("PointList")));
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
		for(int i = 4; i < 8; i++)
			pVertices[0].f[i] = pVertices[nVertices-1].f[i];
/*
	if(::GetAsyncKeyState(VK_SPACE)&0x80000000)
	{
		FlushPrim();
		Flip();
	}
*/
}

void GSStateSoft::FlushPrim()
{
	if(m_nVertices == 0) return;

	if(m_nVertices && m_de.PRIM.TME)
	{
		GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];
		m_lm.setupCLUT(ctxt->TEX0, m_de.TEXCLUT, m_de.TEXA);
	}

	DWORD nPrims = 0;
	GSSoftVertex* pVertices = m_pVertices;

	switch(m_primtype)
	{
	case PRIM_SPRITE:
		ASSERT(!(m_nVertices&3));
		nPrims = m_nVertices / 4;
		LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
		for(int i = 0; i < nPrims; i++, pVertices += 4) DrawSprite(pVertices);
		break;
	case PRIM_TRIANGLE:
		ASSERT(!(m_nVertices%3));
		nPrims = m_nVertices / 3;
		LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
		for(int i = 0; i < nPrims; i++, pVertices += 3) DrawTriangle(pVertices);
		break;
	case PRIM_LINE: 
		ASSERT(!(m_nVertices&1));
		nPrims = m_nVertices / 2;
		LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
		for(int i = 0; i < nPrims; i++, pVertices += 2) DrawLine(pVertices);
		break;
	case PRIM_POINT:
		nPrims = m_nVertices;
		LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
		for(int i = 0; i < nPrims; i++, pVertices++) DrawPoint(pVertices);
		break;
	default:
		ASSERT(m_nVertices == 0);
		return;
	}

	m_stats.IncPrims(nPrims);

	m_PRIM = 7;
	m_primtype = PRIM_NONE;
	m_nVertices = 0;
}

void GSStateSoft::Flip()
{
	__super::Flip();

	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	HRESULT hr;

	CComPtr<IDirect3DSurface9> pBackBuff;
	hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuff);

	D3DSURFACE_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	pBackBuff->GetDesc(&bd);

	CRect dst(0, 0, bd.Width, bd.Height);

	struct
	{
		CComPtr<IDirect3DTexture9> pRT;
		D3DSURFACE_DESC rd;
		scale_t scale;
		CRect src;
	} rt[2];

	for(int i = 0; i < countof(rt); i++)
	{
		if(m_rs.IsEnabled(i))
		{
			CSize size = m_rs.GetSize(i);

			int tw = size.cx;
			int th = size.cy;

			hr = m_pD3DDev->CreateTexture(tw, th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &rt[i].pRT, NULL);
			if(S_OK != hr) continue;

			ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
			hr = rt[i].pRT->GetLevelDesc(0, &rt[i].rd);

			rt[i].scale = scale_t(1, 1);
			rt[i].src = CRect(0, 0, size.cx, size.cy);

			D3DLOCKED_RECT r;
			if(FAILED(hr = rt[i].pRT->LockRect(0, &r, NULL, 0)))
				continue;

			BYTE* dst = (BYTE*)r.pBits;

			GSLocalMemory::readTexel readTexel = m_lm.GetReadTexel(m_rs.DISPFB[i].PSM);
			GIFRegTEX0 TEX0;
			TEX0.TBP0 = m_rs.DISPFB[i].FBP<<5;
			TEX0.TBW = m_rs.DISPFB[i].FBW;
			TEX0.TCC = ctxt->TEX0.TCC;

			for(int y = 0, diff = r.Pitch - tw*4; y < th; y++, dst += diff)
				for(int x = 0; x < tw; x++, dst += 4)
					*(DWORD*)dst = (m_lm.*readTexel)(x, y, TEX0, m_de.TEXA);

			rt[i].pRT->UnlockRect(0);
		}
	}

	bool fShiftField = false;
		// m_rs.SMODE2.INT && !!(ctxt->XYOFFSET.OFY&0xf);
		// m_rs.CSRr.FIELD && m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/;

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
	}

	hr = m_pD3DDev->SetTexture(0, rt[0].pRT);
	hr = m_pD3DDev->SetTexture(1, rt[1].pRT);

	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

	hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX2);

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

void GSStateSoft::EndFrame()
{
}

void GSStateSoft::DrawPoint(GSSoftVertex* v)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	int xmin = max(ctxt->SCISSOR.SCAX0, 0);
	int xmax = min(ctxt->SCISSOR.SCAX1+1, ctxt->FRAME.FBW * 64);
	int ymin = max(ctxt->SCISSOR.SCAY0, 0);
	int ymax = min(ctxt->SCISSOR.SCAY1+1, 4096);

	if(v->x >= xmin && v->x < xmax && v->y >= ymin && v->y < ymax)
		DrawVertex((int)v->x, (int)v->y, *v);
}

void GSStateSoft::DrawLine(GSSoftVertex* v)
{
	// TODO
}

void GSStateSoft::DrawTriangle(GSSoftVertex* v)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(v[1].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp;}
	if(v[2].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[2]; v[2] = tmp;}
	if(v[2].y < v[1].y)  {GSSoftVertex tmp = v[1]; v[1] = v[2]; v[2] = tmp;}

	if(v[0].y >= v[2].y) return;

	int xmin = max(ctxt->SCISSOR.SCAX0, 0);
	int xmax = min(ctxt->SCISSOR.SCAX1+1, ctxt->FRAME.FBW * 64);
	int ymin = max(ctxt->SCISSOR.SCAY0, 0);
	int ymax = min(ctxt->SCISSOR.SCAY1+1, 4096);

	float temp = (v[1].y - v[0].y) / (v[2].y - v[0].y);
	float longest = temp * (v[2].x - v[0].x) + (v[0].x - v[1].x);

	GSSoftVertex edge[2], dedge[2], scan, dscan;

	int ledge, redge;
	if(longest > 0) {ledge = 0; redge = 1;}
	else if(longest < 0) {ledge = 1; redge = 0;}
	else return;

	memset(dedge, 0, sizeof(dedge));
	if(v[1].y > v[0].y) dedge[ledge] = (v[1] - v[0]) / (v[1].y - v[0].y);
	if(v[2].y > v[0].y) dedge[redge] = (v[2] - v[0]) / (v[2].y - v[0].y);

	memset(&dscan, 0, sizeof(dscan));
	dscan = ((v[2] - v[0]) * temp + (v[0] - v[1])) / longest;

	edge[ledge] = edge[redge] = v[0];

	for(int i = 0; i < 2; i++, v++)
	{
		int top = int(v[0].y), bottom = int(v[1].y);

		if(top < ymin)
		{
			for(int i = 0; i < 2; i++) edge[i] += dedge[i] * (min(ymin, bottom) - top);
			edge[0].y = edge[1].y = top = ymin;
		}

		if(bottom > ymax)
		{
			bottom = ymax;
		}

		for(; top < bottom; top++)
		{
			scan = edge[0];

			int left = int(edge[0].x), right = int(edge[1].x);

			if(left < xmin)
			{
				scan += dscan * (xmin - left);
				scan.x = left = xmin;
			}

			if(right > xmax)
			{
				right = xmax;
			}

			for(; left < right; left++)
			{
				DrawVertex(left, top, scan);
				scan += dscan;
			}

			for(int i = 0; i < 2; i++) edge[i] += dedge[i];
		}

		if(v[2].y > v[1].y)
		{
			edge[ledge] = v[1];
			dedge[ledge] = (v[2] - v[1]) / (v[2].y - v[1].y);
		}
	}
}

void GSStateSoft::DrawSprite(GSSoftVertex* v)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(v[2].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[2]; v[2] = tmp; tmp = v[1]; v[1] = v[3]; v[3] = tmp;}
	if(v[1].x < v[0].x) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp; tmp = v[2]; v[2] = v[3]; v[3] = tmp;}

	if(v[0].x == v[1].x || v[0].y == v[2].y) return;

	int xmin = max(ctxt->SCISSOR.SCAX0, 0);
	int xmax = min(ctxt->SCISSOR.SCAX1+1, ctxt->FRAME.FBW * 64);
	int ymin = max(ctxt->SCISSOR.SCAY0, 0);
	int ymax = min(ctxt->SCISSOR.SCAY1+1, 4096);

	GSSoftVertex edge[2], dedge[2], scan, dscan;

	memset(dedge, 0, sizeof(dedge));
	for(int i = 0; i < 2; i++) dedge[i] = (v[i+2] - v[i]) / (v[i+2].y - v[i].y);

	memset(&dscan, 0, sizeof(dscan));
	dscan = (v[1] - v[0]) / (v[1].x - v[0].x);

	edge[0] = v[0];
	edge[1] = v[1];

	int top = int(v[0].y), bottom = int(v[2].y);

	if(top < ymin)
	{
		for(int i = 0; i < 2; i++) edge[i] += dedge[i] * (min(ymin, bottom) - top);
		edge[0].y = edge[1].y = top = ymin;
	}

	if(bottom > ymax)
	{
		bottom = ymax;
	}

	for(; top < bottom; top++)
	{
		scan = edge[0];

		int left = int(edge[0].x), right = int(edge[1].x);

		if(left < xmin)
		{
			scan += dscan * (xmin - left);
			scan.x = left = xmin;
		}

		if(right > xmax)
		{
			right = xmax;
		}

		for(; left < right; left++)
		{
			DrawVertex(left, top, scan);
			scan += dscan;
		}

		for(int i = 0; i < 2; i++) edge[i] += dedge[i];
	}
}

void GSStateSoft::DrawVertex(int x, int y, GSSoftVertex& v)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	DWORD FBP = ctxt->FRAME.FBP<<5, FBW = ctxt->FRAME.FBW;

	int Rf = (int)v.r;
	int Gf = (int)v.g;
	int Bf = (int)v.b;
	int Af = (int)v.a;

	if(m_de.PRIM.TME)
	{
		int tw = 1<<ctxt->TEX0.TW;
		int th = 1<<ctxt->TEX0.TH;

		float tu = v.u / v.w * tw;
		float tv = v.v / v.w * th;

		float ftu = modf(tu /*- 0.5f*/, &tu), iftu = 1.0f - ftu;
		float ftv = modf(tv /*- 0.5f*/, &tv), iftv = 1.0f - ftv;

		int itu[2] = {tu, tu+1};
		int itv[2] = {tv, tv+1};

		for(int i = 0; i < countof(itu); i++)
		switch(ctxt->CLAMP.WMS)
		{
		case 0: itu[i] = itu[i] & (tw-1); break;
		case 1: itu[i] = itu[i] < 0 ? 0 : itu[i] > tw ? itu[i] = tw : itu[i]; break;
		case 2: itu[i] = itu[i] < ctxt->CLAMP.MINU ? ctxt->CLAMP.MINU : itu[i] > ctxt->CLAMP.MAXU ? ctxt->CLAMP.MAXU : itu[i]; break;
		case 3: itu[i] = (int(itu[i]) & ctxt->CLAMP.MINU) | ctxt->CLAMP.MAXU; break;
		}

		for(int i = 0; i < countof(itv); i++)
		switch(ctxt->CLAMP.WMT)
		{
		case 0: itv[i] = itv[i] & (th-1); break;
		case 1: itv[i] = itv[i] < 0 ? 0 : itv[i] > th ? itv[i] = th : itv[i]; break;
		case 2: itv[i] = itv[i] < ctxt->CLAMP.MINV ? ctxt->CLAMP.MINV : itv[i] > ctxt->CLAMP.MAXV ? ctxt->CLAMP.MAXV : itv[i]; break;
		case 3: itv[i] = (int(itv[i]) & ctxt->CLAMP.MINV) | ctxt->CLAMP.MAXV; break;
		}

		DWORD c[4];

		c[0] = (m_lm.*ctxt->rt)(itu[0], itv[0], ctxt->TEX0, m_de.TEXA);
		c[1] = (m_lm.*ctxt->rt)(itu[1], itv[0], ctxt->TEX0, m_de.TEXA);
		c[2] = (m_lm.*ctxt->rt)(itu[0], itv[1], ctxt->TEX0, m_de.TEXA);
		c[3] = (m_lm.*ctxt->rt)(itu[1], itv[1], ctxt->TEX0, m_de.TEXA);

		BYTE Bt = (BYTE)(iftu*iftv*(c[0]&0xff) + ftu*iftv*(c[1]&0xff) + iftu*ftv*(c[2]&0xff) + ftu*ftv*(c[3]&0xff) + 0.5f);
		BYTE Gt = (BYTE)(iftu*iftv*((c[0]>>8)&0xff) + ftu*iftv*((c[1]>>8)&0xff) + iftu*ftv*((c[2]>>8)&0xff) + ftu*ftv*((c[3]>>8)&0xff) + 0.5f);
		BYTE Rt = (BYTE)(iftu*iftv*((c[0]>>16)&0xff) + ftu*iftv*((c[1]>>16)&0xff) + iftu*ftv*((c[2]>>16)&0xff) + ftu*ftv*((c[3]>>16)&0xff) + 0.5f);
		BYTE At = (BYTE)(iftu*iftv*((c[0]>>24)&0xff) + ftu*iftv*((c[1]>>24)&0xff) + iftu*ftv*((c[2]>>24)&0xff) + ftu*ftv*((c[3]>>24)&0xff) + 0.5f);
/*
		BYTE Bt = (BYTE)((c[0]>>0)&0xff);
		BYTE Gt = (BYTE)((c[0]>>8)&0xff);
		BYTE Rt = (BYTE)((c[0]>>16)&0xff);
		BYTE At = (BYTE)((c[0]>>24)&0xff);
*/
		switch(ctxt->TEX0.TFX)
		{
		case 0:
			Rf = Rf * Rt >> 7;
			Gf = Gf * Gt >> 7;
			Bf = Bf * Bt >> 7;
			Af = ctxt->TEX0.TCC ? (Af * At >> 7) : Af;
			break;
		case 1:
			Rf = Rt;
			Gf = Gt;
			Bf = Bt;
			Af = At;
			break;
		case 2:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = ctxt->TEX0.TCC ? (Af + At) : Af;
			break;
		case 3:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = ctxt->TEX0.TCC ? At : Af;
			break;
		}

		if(Rf > 255) Rf = 255;
		if(Gf > 255) Gf = 255;
		if(Bf > 255) Bf = 255;
		if(Af > 255) Af = 255;
	}

	if(m_de.PRIM.FGE)
	{
		BYTE F = (BYTE)v.fog;
		Rf = (F * Rf + (255 - F) * m_de.FOGCOL.FCR) >> 8;
		Gf = (F * Gf + (255 - F) * m_de.FOGCOL.FCG) >> 8;
		Bf = (F * Bf + (255 - F) * m_de.FOGCOL.FCB) >> 8;
	}

	BOOL ZMSK = ctxt->ZBUF.ZMSK;
	DWORD FBMSK = ctxt->FRAME.FBMSK;

	if(ctxt->TEST.ATE)
	{
		bool fPass = true;

		switch(ctxt->TEST.ATST)
		{
		case 0: fPass = false; break;
		case 1: fPass = true; break;
		case 2: fPass = Af < ctxt->TEST.AREF; break;
		case 3: fPass = Af <= ctxt->TEST.AREF; break;
		case 4: fPass = Af == ctxt->TEST.AREF; break;
		case 5: fPass = Af >= ctxt->TEST.AREF; break;
		case 6: fPass = Af > ctxt->TEST.AREF; break;
		case 7: fPass = Af != ctxt->TEST.AREF; break;
		}

		if(!fPass)
		{
			switch(ctxt->TEST.AFAIL)
			{
			case 0: return;
			case 1: ZMSK = 1; break; // RGBA
			case 2: FBMSK = 0xffffffff; break; // Z
			case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
			}
		}
	}

	if(ctxt->TEST.DATE && ctxt->FRAME.PSM <= PSM_PSMCT16S)
	{
		GSLocalMemory::readPixel rp = m_lm.GetReadPixel(ctxt->FRAME.PSM);
		BYTE A = (m_lm.*rp)(x, y, FBP, FBW) >> (ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15);
		if(A ^ ctxt->TEST.DATM) return;
	}

	if(ctxt->TEST.ZTE && ctxt->rz)
	{
		if(ctxt->TEST.ZTST == 0)
			return;

		if(ctxt->TEST.ZTST != 1)
		{
			float z = (float)(m_lm.*ctxt->rz)(x, y, ctxt->ZBUF.ZBP<<5, FBW) / UINT_MAX;
			if(ctxt->TEST.ZTST == 2 && v.z < z || ctxt->TEST.ZTST == 3 && v.z <= z)
				return;
		}
	}

	if(!ZMSK && ctxt->wz)
	{
		(m_lm.*ctxt->wz)(x, y, (DWORD)(v.z * UINT_MAX), ctxt->ZBUF.ZBP<<5, FBW);
	}

	if((m_de.PRIM.ABE || (m_de.PRIM.PRIM == 1 || m_de.PRIM.PRIM == 2) && m_de.PRIM.AA1)
	&& (!m_de.PABE.PABE || (Af&0x80)))
	{
		GIFRegTEX0 TEX0;
		TEX0.TBP0 = FBP;
		TEX0.TBW = FBW;
		TEX0.TCC = ctxt->TEX0.TCC;
		DWORD Cd = (m_lm.*ctxt->rp)(x, y, TEX0, m_de.TEXA);

		BYTE R[3] = {Rf, (Cd>>16)&0xff, 0};
		BYTE G[3] = {Gf, (Cd>>8)&0xff, 0};
		BYTE B[3] = {Bf, (Cd>>0)&0xff, 0};
		BYTE A[3] = {Af, (Cd>>24)&0xff, ctxt->ALPHA.FIX};

		Rf = ((R[ctxt->ALPHA.A] - R[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + R[ctxt->ALPHA.D];
		Gf = ((G[ctxt->ALPHA.A] - G[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + G[ctxt->ALPHA.D];
		Bf = ((B[ctxt->ALPHA.A] - B[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + B[ctxt->ALPHA.D];
	}

	if(m_de.COLCLAMP.CLAMP)
	{
		if(Rf > 255) Rf = 255;
		else if(Rf < 0) Rf = 0;
		if(Gf > 255) Gf = 255;
		else if(Gf < 0) Gf = 0;
		if(Bf > 255) Bf = 255;
		else if(Bf < 0) Bf = 0;
		if(Af > 255) Af = 255;
		else if(Af < 0) Af = 0;
	}

	if(ctxt->FBA.FBA)
	{
		Af |= 0x80;
	}

	DWORD color = (Af << 24) | (Bf << 16) | (Gf << 8) | (Rf << 0);

	color &= ~FBMSK;

	(m_lm.*ctxt->wp)(x, y, color, FBP, FBW);
}
