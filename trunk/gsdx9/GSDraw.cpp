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

#include "stdafx.h"
#include "GSState.h"

extern "C" void __stdcall GSvsync();

inline BYTE SCALE_ALPHA(BYTE a) 
{
	return (((a)&0x80)?0xff:((a)<<1));
	return a;
	return (((a)&0x80)?0xff:(a));
	return (((a)&0xa0)?0xff:((a+a+a)>>1));
}

void GSState::VertexKick(bool fSkip)
{
	LOG((_T("VertexKick(%d)\n"), fSkip));

	DrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	CUSTOMVERTEX v;

	v.x = ((float)m_v.XYZ.X - ctxt->XYOFFSET.OFX)/16;
	v.y = ((float)m_v.XYZ.Y - ctxt->XYOFFSET.OFY)/16;
//	v.x = (float)m_v.XYZ.X/16 - (ctxt->XYOFFSET.OFX>>4);
//	v.y = (float)m_v.XYZ.Y/16 - (ctxt->XYOFFSET.OFY>>4);
	// if(m_v.XYZ.Z && m_v.XYZ.Z < 0x100) m_v.XYZ.Z = 0x100;
	// v.z = 1.0f * (m_v.XYZ.Z>>8)/(UINT_MAX>>8);
	v.z = log(1.0 + m_v.XYZ.Z)/log(2.0)/32;
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
			v.tu = (float)m_v.UV.U / (16<<ctxt->TEX0.TW);
			v.tv = (float)m_v.UV.V / (16<<ctxt->TEX0.TH);
			v.rhw = 2.0f;
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

	static const int vmin[8] = {1, 2, 2, 3, 3, 3, 2, 0};
	while(m_vl.GetCount() >= vmin[m_de.PRIM.PRIM])
		DrawingKick(fSkip);
}

///////////////////////////////////////////////

void GSState::DrawingKick(bool fSkip)
{
	LOG((_T("DrawingKick %d\n"), m_de.PRIM.PRIM));

	DrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	D3DPRIMITIVETYPE primtype;
	CUSTOMVERTEX pVertices[4];
	int nVertices = 0;

	switch(m_de.PRIM.PRIM)
	{
	case 3: // triangle
		primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 4: // triangle strip
#ifdef ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLESTRIP;
#else ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLELIST;
#endif
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		break;
	case 5: // triangle fan
#ifdef ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLEFAN;
#else ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLELIST;
#endif
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		break;
	case 6: // sprite
		primtype = D3DPT_TRIANGLELIST;
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
		break;
	case 1: // line
		primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 2: // line strip
#ifdef ENABLE_STRIPFAN
		primtype = D3DPT_LINESTRIP;
#else
		primtype = D3DPT_LINELIST;
#endif
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		break;
	case 0: // point
		primtype = D3DPT_POINTLIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	default:
		ASSERT(0);
		return;
	}
/*
	POSITION pos;

	switch(m_de.PRIM.PRIM)
	{
	case 3: // triangle
		primtype = D3DPT_TRIANGLELIST;
		pVertices[nVertices++] = m_vl.RemoveHead();
		pVertices[nVertices++] = m_vl.RemoveHead();
		pVertices[nVertices++] = m_vl.RemoveHead();
		break;
	case 4: // triangle strip
#ifdef ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLESTRIP;
#else ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLELIST;
#endif
		pVertices[nVertices++] = m_vl.RemoveHead();
		pos = m_vl.GetHeadPosition();
		pVertices[nVertices++] = m_vl.GetNext(pos);
		pVertices[nVertices++] = m_vl.GetNext(pos);
		break;
	case 5: // triangle fan
#ifdef ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLEFAN;
#else ENABLE_STRIPFAN
		primtype = D3DPT_TRIANGLELIST;
#endif
		pos = m_vl.GetHeadPosition();
		pVertices[nVertices++] = m_vl.GetNext(pos);
		pVertices[nVertices++] = m_vl.GetNext(pos);
		pVertices[nVertices++] = m_vl.GetNext(pos);
		m_vl.RemoveAt(m_vl.FindIndex(1));
		break;
	case 6: // sprite
		primtype = D3DPT_TRIANGLELIST;
		pVertices[nVertices++] = m_vl.RemoveHead();
		pVertices[nVertices++] = m_vl.RemoveHead();
		nVertices += 2;
		// ASSERT(pVertices[0].z == pVertices[1].z);
		pVertices[0].z = pVertices[1].z;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].y = pVertices[0].y;
		pVertices[1].tv = pVertices[0].tv;
		pVertices[2].x = pVertices[0].x;
		pVertices[2].tu = pVertices[0].tu;
		break;
	case 1: // line
		primtype = D3DPT_LINELIST;
		pVertices[nVertices++] = m_vl.RemoveHead();
		pVertices[nVertices++] = m_vl.RemoveHead();
		break;
	case 2: // line strip
#ifdef ENABLE_STRIPFAN
		primtype = D3DPT_LINESTRIP;
#else
		primtype = D3DPT_LINELIST;
#endif
		pVertices[nVertices++] = m_vl.RemoveHead();
		pVertices[nVertices++] = m_vl.GetHead();
		break;
	case 0: // point
		primtype = D3DPT_POINTLIST;
		pVertices[nVertices++] = m_vl.RemoveHead();
		break;
	default:
		ASSERT(0);
		return;
	}
*/
	if(!(m_rs.PMODE.EN1|m_rs.PMODE.EN2) || fSkip)
	{
#ifdef ENABLE_STRIPFAN
		FlushPrim();
#endif
		return;
	}

	LOG2((_T("Prim %05x %05x %04x\n"), 
		ctxt->FRAME.Block(), m_de.PRIM.TME ? (UINT32)ctxt->TEX0.TBP0 : 0xfffff,
		(m_de.PRIM.ABE || (m_primtype == D3DPT_LINELIST || m_primtype == D3DPT_LINESTRIP) && m_de.PRIM.AA1)
			? ((ctxt->ALPHA.A<<12)|(ctxt->ALPHA.B<<8)|(ctxt->ALPHA.C<<4)|ctxt->ALPHA.D) 
			: 0xffff));

	switch(primtype)
	{
	case D3DPT_TRIANGLELIST:
		if(nVertices == 4)
		{
			LOGV((pVertices[0], _T("Sprite")));
			LOGV((pVertices[1], _T("Sprite")));
			LOGV((pVertices[2], _T("Sprite")));
			LOGV((pVertices[3], _T("Sprite")));
			QueuePrim(pVertices, D3DPT_TRIANGLELIST);
			QueuePrim(pVertices+1, D3DPT_TRIANGLELIST);
		}
		else
		{
			LOGV((pVertices[0], _T("TriList")));
			LOGV((pVertices[1], _T("TriList")));
			LOGV((pVertices[2], _T("TriList")));
			QueuePrim(pVertices, D3DPT_TRIANGLELIST);
		}
		break;
	case D3DPT_TRIANGLESTRIP:
		LOGV((pVertices[0], _T("TriStrip")));
		LOGV((pVertices[1], _T("TriStrip")));
		LOGV((pVertices[2], _T("TriStrip")));
		QueuePrim(pVertices, D3DPT_TRIANGLESTRIP);
		break;
	case D3DPT_TRIANGLEFAN:
		LOGV((pVertices[0], _T("TriFan")));
		LOGV((pVertices[1], _T("TriFan")));
		LOGV((pVertices[2], _T("TriFan")));
		QueuePrim(pVertices, D3DPT_TRIANGLEFAN);
		break;
	case D3DPT_LINELIST:
		LOGV((pVertices[0], _T("LineList")));
		LOGV((pVertices[1], _T("LineList")));
		QueuePrim(pVertices, D3DPT_LINELIST);
		break;
	case D3DPT_LINESTRIP:
		LOGV((pVertices[0], _T("LineStrip")));
		LOGV((pVertices[1], _T("LineStrip")));
		QueuePrim(pVertices, D3DPT_LINESTRIP);
		break;
	case D3DPT_POINTLIST:
		LOGV((pVertices[0], _T("PointList")));
		QueuePrim(pVertices, D3DPT_POINTLIST);
		break;
	default:
		break;
	}
}

void GSState::QueuePrim(CUSTOMVERTEX* pNewVertices, D3DPRIMITIVETYPE pt)
{
	if(m_primtype != pt && m_nVertices > 0)
		FlushPrim();

	m_primtype = pt;

	int nNewVertices = 0;

	switch(m_primtype)
	{
	case D3DPT_TRIANGLELIST: nNewVertices = 3; break;
	case D3DPT_TRIANGLESTRIP: nNewVertices = 3; if(m_nVertices > 0) {nNewVertices-=2; pNewVertices+=2;} break;
	case D3DPT_TRIANGLEFAN: nNewVertices = 3; if(m_nVertices > 0) {nNewVertices-=2; pNewVertices+=2;} break;
	case D3DPT_LINELIST: nNewVertices = 2; break;
	case D3DPT_LINESTRIP: nNewVertices = 2; if(m_nVertices > 0) {nNewVertices--; pNewVertices++;} break;
	case D3DPT_POINTLIST: nNewVertices = 1; break;
	default: ASSERT(0); return;
	}

	if(nNewVertices > 1)
	{
		int j = nNewVertices-1;

		for(int i = 1; i < nNewVertices; i++, j--)
		{
			if(pNewVertices[i-1].x != pNewVertices[i].x
			|| pNewVertices[i-1].y != pNewVertices[i].y
			|| pNewVertices[i-1].z != pNewVertices[i].z)
				break;
		}

		if(j == 0) return;
	}

	if(m_nVertices+nNewVertices > m_nMaxVertices)
	{
		CUSTOMVERTEX* pVertices = new CUSTOMVERTEX[m_nMaxVertices <<= 1];
		memcpy(pVertices, m_pVertices, m_nVertices*sizeof(CUSTOMVERTEX));
		delete [] m_pVertices;
		m_pVertices = pVertices;
	}

	if(!m_de.PRIM.IIP)
	{
		for(int i = nNewVertices-1; i > 0; i--)
			pNewVertices[i-1].color = pNewVertices[i].color;
	}

	memcpy(&m_pVertices[m_nVertices], pNewVertices, sizeof(CUSTOMVERTEX)*nNewVertices);
	m_nVertices += nNewVertices;

	if(::GetAsyncKeyState(VK_SPACE)&0x80000000)
	{
		FlushPrim();
		Flip();
	}
}

void GSState::FlushPrim()
{
	if(m_nVertices == 0) return;

	HRESULT hr;

	int nPrims = 0;

	switch(m_primtype)
	{
	case D3DPT_TRIANGLELIST: ASSERT(!(m_nVertices%3)); nPrims = m_nVertices/3; break;
	case D3DPT_TRIANGLESTRIP: ASSERT(m_nVertices > 2); nPrims = m_nVertices-2; break;
	case D3DPT_TRIANGLEFAN: ASSERT(m_nVertices > 2); nPrims = m_nVertices-2; break;
	case D3DPT_LINELIST: ASSERT(!(m_nVertices&1)); nPrims = m_nVertices/2; break;
	case D3DPT_LINESTRIP: ASSERT(m_nVertices > 1); nPrims = m_nVertices-1; break;
	case D3DPT_POINTLIST: nPrims = m_nVertices; break;
	default: ASSERT(m_nVertices == 0); return;
	}

	LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));

	m_stats.IncPrims(nPrims);

	//////////////////////

	DrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	//////////////////////

	CComPtr<IDirect3DSurface9> pBackBuff;
	hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuff);

	D3DSURFACE_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	pBackBuff->GetDesc(&bd);

	CSize size = m_rs.GetSize(m_rs.PMODE.EN1?0:1);
	float xscale = (float)bd.Width / (ctxt->FRAME.FBW*64);
	float yscale = (float)bd.Height / size.cy;

	//////////////////////

	CComPtr<IDirect3DTexture9> pRT;
	CComPtr<IDirect3DSurface9> pDS;

	bool fClearRT = false;
	bool fClearDS = false;

	if(!m_pRenderTargets.Lookup(ctxt->FRAME.Block(), pRT))
	{
		hr = m_pD3DDev->CreateTexture(bd.Width, bd.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);
		if(S_OK != hr) {ASSERT(0); return;}
		m_pRenderTargets[ctxt->FRAME.Block()] = pRT;
#ifdef DEBUG_RENDERTARGETS
		CGSWnd* pWnd = new CGSWnd();
		CString str; str.Format(_T("%05x"), ctxt->FRAME.Block());
		pWnd->Create(str);
		m_pRenderWnds[ctxt->FRAME.Block()] = pWnd;
		pWnd->Show();
#endif
		fClearRT = true;
	}

	if(!m_pDepthStencils.Lookup(ctxt->ZBUF.ZBP, pDS))
	{
		hr = m_pD3DDev->CreateDepthStencilSurface(bd.Width, bd.Height, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, FALSE, &pDS, NULL);
		if(S_OK != hr) {ASSERT(0); return;}
		m_pDepthStencils[ctxt->ZBUF.ZBP] = pDS;
		fClearDS = true;
	}

	if(!pRT || !pDS) {ASSERT(0); return;}

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

		switch(ctxt->CLAMP.WMS&1)
		{
		case 0: u = D3DTADDRESS_WRAP; break; // repeat
		case 1: u = D3DTADDRESS_CLAMP; break; // clamp
		}

		switch(ctxt->CLAMP.WMT&1)
		{
		case 0: v = D3DTADDRESS_WRAP; break; // repeat
		case 1: v = D3DTADDRESS_CLAMP; break; // clamp
		}

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, u);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, v);

		bool fRT = IsRenderTarget(t.m_pTexture); // RTs already have a correctly scaled alpha

		switch(ctxt->TEX0.TFX)
		{
		case 0:
			if(!ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[0];
			else if(!fRT) pPixelShader = m_pPixelShaders[1];
			else pPixelShader = m_pPixelShaders[2];
			break;
		case 1:
			if(!fRT) pPixelShader = m_pPixelShaders[3];
			else pPixelShader = m_pPixelShaders[4];
			break;
		case 2:
			if(!ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[5];
			else if(!fRT) pPixelShader = m_pPixelShaders[6];
			else pPixelShader = m_pPixelShaders[7];
			break;
		case 3:
			if(!ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[8];
			else if(!fRT) pPixelShader = m_pPixelShaders[9];
			else pPixelShader = m_pPixelShaders[10];
			break;
		}

		ASSERT(pPixelShader);
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

		BYTE FIX = SCALE_ALPHA(ctxt->ALPHA.FIX);

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

		int i = (((ctxt->ALPHA.A&3)*3+(ctxt->ALPHA.B&3))*3+(ctxt->ALPHA.C&3))*3+(ctxt->ALPHA.D&3);

		ASSERT(ctxt->ALPHA.A != 3);
		ASSERT(ctxt->ALPHA.B != 3);
		ASSERT(ctxt->ALPHA.C != 3);
		ASSERT(ctxt->ALPHA.D != 3);
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
	if(ctxt->FRAME.FBMSK&0xff000000) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
	if(ctxt->FRAME.FBMSK&0x00ff0000) mask &= ~D3DCOLORWRITEENABLE_BLUE;
	if(ctxt->FRAME.FBMSK&0x0000ff00) mask &= ~D3DCOLORWRITEENABLE_GREEN;
	if(ctxt->FRAME.FBMSK&0x000000ff) mask &= ~D3DCOLORWRITEENABLE_RED;
	hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

	ASSERT(ctxt->FRAME.FBMSK == 0); // wild arms (also 8H+pal on RT...)

	//////////////////////

	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, ctxt->TEST.ZTE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, !ctxt->ZBUF.ZMSK);
	if(ctxt->TEST.ZTE)
	{
		DWORD zfunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_GREATEREQUAL, D3DCMP_GREATER};
		hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, zfunc[ctxt->TEST.ZTST]);

		// FIXME
		if(ctxt->ZBUF.ZMSK && ctxt->TEST.ZTST == 1)
		{
			CUSTOMVERTEX* pVertices = m_pVertices;
			for(int i = m_nVertices; i-- > 0; pVertices++)
				pVertices->z = 0;
		}
	}

	//////////////////////

	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, ctxt->TEST.ATE); 
	if(ctxt->TEST.ATE)
	{
		DWORD afunc[] =
		{
			D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_LESS, D3DCMP_LESSEQUAL, 
			D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL
		};

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, afunc[ctxt->TEST.ATST]);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(ctxt->TEST.AREF));
	}

	//////////////////////

	hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	CRect scissor(xscale * ctxt->SCISSOR.SCAX0, yscale * ctxt->SCISSOR.SCAY0, xscale * (ctxt->SCISSOR.SCAX1+1), yscale * (ctxt->SCISSOR.SCAY1+1));
	scissor.IntersectRect(scissor, CRect(0, 0, bd.Width, bd.Height));
	hr = m_pD3DDev->SetScissorRect(scissor);

	//////////////////////
//////
	ASSERT(!m_de.PABE.PABE);
	ASSERT(!ctxt->FBA.FBA);
	ASSERT(!ctxt->TEST.DATE); // sfex3, after the capcom logo

	//////////////////////

	{
		CUSTOMVERTEX* pVertices = m_pVertices;
		for(int i = m_nVertices; i-- > 0; pVertices++)
		{
			pVertices->x *= xscale;
			pVertices->y *= yscale;

			if(m_de.PRIM.TME)
			{
				float base, fract;
				fract = modf(pVertices->tu, &base);
				fract = fract * (1<<ctxt->TEX0.TW) / td.Width * t.m_xscale;
				ASSERT(-1 <= fract && fract <= 1.01);
				pVertices->tu = base + fract;
				fract = modf(pVertices->tv, &base);
				fract = fract * (1<<ctxt->TEX0.TH) / td.Height * t.m_yscale;
				//ASSERT(-1 <= fract && fract <= 1.01);
				pVertices->tv = base + fract;
			}
		}
	}


if(m_de.PRIM.TME && m_nVertices == 6 && (ctxt->FRAME.Block()) == 0x00000 && ctxt->TEX0.TBP0 == 0x00f00)
{
	if(m_stats.GetFrame() > 500)
	{
//		hr = D3DXSaveTextureToFile(_T("c:\\rtbefore.bmp"), D3DXIFF_BMP, pRT, NULL);
	}
}

	CComPtr<IDirect3DVertexBuffer9> pVB;

	hr = m_pD3DDev->CreateVertexBuffer(
		m_nVertices*sizeof(CUSTOMVERTEX), 
		D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC/**/,
		D3DFVF_CUSTOMVERTEX,
		/*D3DPOOL_MANAGED*/D3DPOOL_DEFAULT,
		&pVB, NULL);

	CUSTOMVERTEX* pVertices = NULL;
	if(S_OK == pVB->Lock(0, 0, (void**)&pVertices, D3DLOCK_DISCARD|D3DLOCK_NOOVERWRITE))
	{
		memcpy(pVertices, m_pVertices, m_nVertices*sizeof(CUSTOMVERTEX));
		pVB->Unlock();
	}

	hr = m_pD3DDev->SetStreamSource(0, pVB, 0, sizeof(CUSTOMVERTEX));

	hr = m_pD3DDev->BeginScene();

	hr = m_pD3DDev->SetFVF(D3DFVF_CUSTOMVERTEX);

	if(1)//!m_de.PABE.PABE)
	{
		if(pPixelShader) hr = m_pD3DDev->SetPixelShader(pPixelShader);

		hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);

		if(pPixelShader) hr = m_pD3DDev->SetPixelShader(NULL);
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
				CUSTOMVERTEX v[4];
				memcpy(v, &m_pVertices[i*3], sizeof(CUSTOMVERTEX)*3);
				v[3] = m_pVertices[i*3];
				hr = m_pD3DDev->DrawPrimitive(D3DPT_LINESTRIP, 0, nPrims);
			}
		}
		else
		{
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
		}
*/	}
/*	else
	{
		ASSERT(!ctxt->TEST.ATE); // TODO

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xfe);

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
	}

	if(ctxt->TEST.ATE && ctxt->TEST.AFAIL && ctxt->TEST.ATST != 1)
	{
		ASSERT(!m_de.PABE.PABE);

		DWORD iafunc[] =
		{
			D3DCMP_ALWAYS, D3DCMP_NEVER, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, 
			D3DCMP_NOTEQUAL, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL
		};

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, iafunc[ctxt->TEST.ATST]);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(ctxt->TEST.AREF));

		int mask = 0;
		bool zwrite = false;

		switch(ctxt->TEST.AFAIL)
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
/*	else if(m_de.PABE.PABE)
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

if(m_de.PRIM.TME && m_nVertices == 6 && (ctxt->FRAME.Block()) == 0x00000 && ctxt->TEX0.TBP0 == 0x00f00)
{
	if(m_stats.GetFrame() > 500)
	{
//		hr = D3DXSaveTextureToFile(_T("c:\\rtafter.bmp"), D3DXIFF_BMP, pRT, NULL);
//		if(t.m_pTexture) hr = D3DXSaveTextureToFile(_T("c:\\tx.bmp"), D3DXIFF_BMP, t.m_pTexture, NULL);
	}
}

	//////////////////////

	GIFRegTEX0 TEX0;
	TEX0.TBP0 = ctxt->FRAME.Block();
	TEX0.PSM = PSM_PSMCT32;
	TEX0.CBP = -1;
	GIFRegCLAMP CLAMP;
	CLAMP.WMS = CLAMP.WMT = 0;
	m_tc.Update(TEX0, CLAMP, m_de.TEXA, xscale, yscale, pRT);

	//////////////////////

	m_primtype = D3DPT_FORCE_DWORD;
	m_nVertices = 0;

	LOG((_T("FlushPrim() finished\n")));
	// GSvsync();
}

/////////////////////

// slooooooooooooooow

/*
void GSState::ConvertRT(CComPtr<IDirect3DTexture9>& pTexture)
{
	DrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(ctxt->TEX0.PSM == PSM_PSMCT32)
		return;

	HRESULT hr;

	D3DSURFACE_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	pTexture->GetLevelDesc(0, &desc);

	CComPtr<IDirect3DTexture9> pRT;
	hr = m_pD3DDev->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);
	if(S_OK != hr) {ASSERT(0); return;}

	CComPtr<IDirect3DSurface9> pDS;
	hr = m_pD3DDev->CreateDepthStencilSurface(desc.Width, desc.Height, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, FALSE, &pDS, NULL);
	if(S_OK != hr) {ASSERT(0); return;}

	CComPtr<IDirect3DSurface9> pSurf;
	hr = pRT->GetSurfaceLevel(0, &pSurf);
	hr = m_pD3DDev->SetRenderTarget(0, pSurf);
	hr = m_pD3DDev->SetDepthStencilSurface(pDS);

	struct CUSTOMVERTEX
	{
		float x, y, z, rhw;
		float tu, tv;
	}
	pVertices[] =
	{
		{(float)0, (float)0, 1.0f, 2.0f, 0.0f, 0.0f},
		{(float)desc.Width, (float)0, 1.0f, 2.0f, 1.0f, 0.0f},
		{(float)0, (float)desc.Height, 1.0f, 2.0f, 0.0f, 1.0f},
		{(float)desc.Width, (float)desc.Height, 1.0f, 2.0f, 1.0f, 1.0f},
	};

	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0);
	hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
	hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);

	hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

	for(int i = 0; i < 8; i++)
	{
		hr = m_pD3DDev->SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
		hr = m_pD3DDev->SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		hr = m_pD3DDev->SetTextureStageState(i, D3DTSS_RESULTARG, D3DTA_CURRENT);
		hr = m_pD3DDev->SetTextureStageState(i, D3DTSS_CONSTANT, 0xffffffff);
		hr = m_pD3DDev->SetTexture(i, NULL);
	}

	hr = m_pD3DDev->SetTexture(0, pTexture);

//	hr = D3DXSaveTextureToFile(_T("c:\\tx.bmp"), D3DXIFF_BMP, pTexture, NULL);

	if(ctxt->TEX0.PSM == PSM_PSMCT16 || ctxt->TEX0.PSM == PSM_PSMCT16S)
	{
		// TA1

		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_CONSTANT, (DWORD)SCALE_ALPHA(m_de.TEXA.TA1)<<24);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CONSTANT);

		for(int i = 0; i < countof(pVertices); i++) pVertices[i].z = 0.9f;

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

		hr = m_pD3DDev->BeginScene();
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_pD3DDev->EndScene();

//		hr = D3DXSaveTextureToFile(_T("c:\\txta1a.bmp"), D3DXIFF_BMP, pRT, NULL);

		for(int i = 0; i < countof(pVertices); i++) pVertices[i].z = 0.0f;

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xff);
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

		hr = m_pD3DDev->BeginScene();
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_pD3DDev->EndScene();
	
//		hr = D3DXSaveTextureToFile(_T("c:\\txta1c.bmp"), D3DXIFF_BMP, pRT, NULL);
	}

	if(ctxt->TEX0.PSM == PSM_PSMCT24 || ctxt->TEX0.PSM == PSM_PSMCT16 || ctxt->TEX0.PSM == PSM_PSMCT16S)
	{
		// TA0

		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_CONSTANT, (DWORD)SCALE_ALPHA(m_de.TEXA.TA0)<<24);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CONSTANT);

		for(int i = 0; i < countof(pVertices); i++) pVertices[i].z = 0.9f;

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

		hr = m_pD3DDev->BeginScene();
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_pD3DDev->EndScene();

//		hr = D3DXSaveTextureToFile(_T("c:\\txta0a.bmp"), D3DXIFF_BMP, pRT, NULL);

		for(int i = 0; i < countof(pVertices); i++) pVertices[i].z = 0.0f;

	if(ctxt->TEX0.PSM == PSM_PSMCT16 || ctxt->TEX0.PSM == PSM_PSMCT16S)
	{
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xff);
	}
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

		hr = m_pD3DDev->BeginScene();
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_pD3DDev->EndScene();
	
//		hr = D3DXSaveTextureToFile(_T("c:\\txta0c.bmp"), D3DXIFF_BMP, pRT, NULL);
	}

	if(m_de.TEXA.AEM && (ctxt->TEX0.PSM == PSM_PSMCT24 || ctxt->TEX0.PSM == PSM_PSMCT16 || ctxt->TEX0.PSM == PSM_PSMCT16S))
	{
		// AEM

		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DOTPRODUCT3);
		hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_CONSTANT, (DWORD)0x80<<24);
		hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
		hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_ADDSIGNED);
		hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CONSTANT);

		for(int i = 0; i < countof(pVertices); i++) pVertices[i].z = 0.0f;

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_EQUAL);
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0x00);
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

		hr = m_pD3DDev->BeginScene();
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_pD3DDev->EndScene();

//		hr = D3DXSaveTextureToFile(_T("c:\\txaem.bmp"), D3DXIFF_BMP, pRT, NULL);
	}

	pTexture = pRT;

}
*/
void GSState::Flip()
{
	DrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	HRESULT hr;

	hr = m_pD3DDev->SetRenderTarget(0, m_pOrgRenderTarget);
	hr = m_pD3DDev->SetDepthStencilSurface(m_pOrgDepthStencil);
	hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
    hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    // hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	{
		float c[] = 
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

				CSize size = m_rs.GetSize(0);
				float xscale = (float)bd.Width / size.cx;
				float yscale = (float)bd.Height / size.cy;

				CRect src(0, 0, xscale*size.cx, yscale*size.cy);

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
				if(fbp == (ctxt->FRAME.Block()))
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
			str.Format(_T("PCSX2 - %05x"), ctxt->FRAME.Block());
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
		CSize size;
		float xscale, yscale;
		CRect src;
	} rt[3];

	for(int i = 0; i < countof(rt); i++)
	{
		if(m_rs.PMODE.EN1 && i == 0 || m_rs.PMODE.EN2 && i == 1 || i == 2)
		{
			UINT32 FBP = i == 2 || (::GetAsyncKeyState(VK_SPACE)&0x80000000) ? ctxt->FRAME.Block() : (m_rs.DISPFB[i].FBP<<5);

			if(i < 2)
			{
				rt[i].size = m_rs.GetSize(i);
				rt[i].xscale = rt[i].yscale = 1;
				rt[i].src = CRect(0, 0, rt[i].size.cx, rt[i].size.cy);
			}

			if(CSurfMap<IDirect3DTexture9>::CPair* pPair = m_pRenderTargets.PLookup(FBP))
			{
				rt[i].pRT = pPair->value;
				m_tc.ResetAge(pPair->key);
				ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
				hr = rt[i].pRT->GetLevelDesc(0, &rt[i].rd);
				rt[i].size = m_rs.GetSize(i);
				rt[i].xscale = (float)bd.Width / rt[i].size.cx;
				rt[i].yscale = (float)bd.Height / rt[i].size.cy;
				rt[i].src = CRect(0, 0, rt[i].xscale*rt[i].size.cx, rt[i].yscale*rt[i].size.cy);
			}
		}
	}

	bool fShiftField = m_rs.SMODE2.INT && !!(ctxt->XYOFFSET.OFY&0xf);
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
			pVertices[i].tv1 += rt[0].yscale*0.5f / rt[0].rd.Height;
			pVertices[i].tv2 += rt[1].yscale*0.5f / rt[1].rd.Height;
			pVertices[i].tv3 += rt[2].yscale*0.5f / rt[2].rd.Height;
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

	if(m_rs.PMODE.EN1 && m_rs.PMODE.EN2 && rt[0].pRT && rt[1].pRT) // RAO1 + RAO2
	{
		pPixelShader = m_pPixelShaders[11];
	}
	else if(m_rs.PMODE.EN1 && rt[0].pRT) // RAO1
	{
		pPixelShader = m_pPixelShaders[12];
	}
	else if(m_rs.PMODE.EN2 && rt[1].pRT) // RAO2
	{
		pPixelShader = m_pPixelShaders[13];
	}
	else if((m_rs.PMODE.EN1 || m_rs.PMODE.EN2) && rt[2].pRT)
	{
		pPixelShader = m_pPixelShaders[14];
	}

	if(pPixelShader)
	{
		hr = m_pD3DDev->BeginScene();
		hr = m_pD3DDev->SetPixelShader(pPixelShader);
		hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_pD3DDev->SetPixelShader(NULL);
		hr = m_pD3DDev->EndScene();
	}
/*
	HDC hDC;
	if(S_OK == pBackBuff->GetDC(&hDC))
	{
		// SetBkMode(hDC, TRANSPARENT);
		SetBkColor(hDC, 0);
		SetTextColor(hDC, 0xffffff);
		TextOut(hDC, 10, 10, str, str.GetLength());
		pBackBuff->ReleaseDC(hDC);
	}
*/
	hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
}
