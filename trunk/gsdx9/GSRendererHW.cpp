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

#define INTERNALRES 1024

inline BYTE SCALE_ALPHA(BYTE a) 
{
	return (((a)&0x80)?0xff:((a)<<1));
}

static const double log_2pow32 = log(2.0)*32;

//

GSRendererHW::GSRendererHW(HWND hWnd, HRESULT& hr)
	: GSRenderer<HWVERTEX>(1024, 1024, hWnd, hr)
//	: GSRenderer<HWVERTEX>(512, 224, hWnd, hr)
{
	Reset();

	hr = m_pD3DDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
	hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
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
	//v.z = (float)m_v.XYZ.Z / UINT_MAX;
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
	v.fog = (m_de.PRIM.FGE ? m_v.FOG.F : 0xff) << 24;

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
		LOGV((pVertices[0], _T("TriList")));
		LOGV((pVertices[1], _T("TriList")));
		LOGV((pVertices[2], _T("TriList")));
		break;
	case 4: // triangle strip
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriStrip")));
		LOGV((pVertices[1], _T("TriStrip")));
		LOGV((pVertices[2], _T("TriStrip")));
		break;
	case 5: // triangle fan
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriFan")));
		LOGV((pVertices[1], _T("TriFan")));
		LOGV((pVertices[2], _T("TriFan")));
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
		LOGV((pVertices[0], _T("Sprite")));
		LOGV((pVertices[1], _T("Sprite")));
		LOGV((pVertices[2], _T("Sprite")));
		LOGV((pVertices[3], _T("Sprite")));
		nVertices += 2;
		pVertices[5] = pVertices[3];
		pVertices[3] = pVertices[1];
		pVertices[4] = pVertices[2];
		break;
	case 1: // line
		m_primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineList")));
		LOGV((pVertices[1], _T("LineList")));
		break;
	case 2: // line strip
		m_primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineStrip")));
		LOGV((pVertices[1], _T("LineStrip")));
		break;
	case 0: // point
		m_primtype = D3DPT_POINTLIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("PointList")));
		break;
	default:
		ASSERT(0);
		m_vl.RemoveAll();
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
		if(m_PRIM == 6) pVertices[3].color = pVertices[5].color;
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

		hr = m_pD3DDev->BeginScene();

		scale_t scale((float)INTERNALRES / (m_ctxt->FRAME.FBW*64), (float)INTERNALRES / m_rs.GetSize(m_rs.IsEnabled(1)?1:0).cy);
		if(m_fHalfVRes) scale.y /= 2;

		//////////////////////

		CComPtr<IDirect3DTexture9> pRT;
		CComPtr<IDirect3DSurface9> pDS;

		bool fClearRT = false;
		bool fClearDS = false;

		if(!m_pRenderTargets.Lookup(m_ctxt->FRAME.Block(), pRT))
		{
			hr = m_pD3DDev->CreateTexture(INTERNALRES, INTERNALRES, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);
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
			hr = m_pD3DDev->CreateDepthStencilSurface(INTERNALRES, INTERNALRES, m_fmtDepthStencil, D3DMULTISAMPLE_NONE, 0, FALSE, &pDS, NULL);
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
			//t.m_pTexture->PreLoad();
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

		SetupTexture(t);

		//////////////////////

		SetupAlphaBlend();

		//////////////////////

		// close approx., to be tested...
		int mask = D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED;
		if(m_ctxt->FRAME.FBMSK&0xff000000) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
		if(m_ctxt->FRAME.FBMSK&0x00ff0000) mask &= ~D3DCOLORWRITEENABLE_BLUE;
		if(m_ctxt->FRAME.FBMSK&0x0000ff00) mask &= ~D3DCOLORWRITEENABLE_GREEN;
		if(m_ctxt->FRAME.FBMSK&0x000000ff) mask &= ~D3DCOLORWRITEENABLE_RED;
		//if(m_ctxt->FRAME.PSM == PSM_PSMCT24) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

		// ASSERT(m_ctxt->FRAME.FBMSK == 0); // wild arms (also 8H+pal on RT...)

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
		scissor.IntersectRect(scissor, CRect(0, 0, INTERNALRES, INTERNALRES));
		hr = m_pD3DDev->SetScissorRect(scissor);

		//////////////////////

		// ASSERT(!m_de.PABE.PABE); // bios
		// ASSERT(!m_ctxt->FBA.FBA); // bios
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

				if(m_de.PRIM.FGE)
				{
					pVertices->fog = D3DCOLOR_ARGB(pVertices->fog>>24, m_de.FOGCOL.FCR, m_de.FOGCOL.FCG, m_de.FOGCOL.FCB);
				}
			}
		}

if(m_de.PRIM.TME && /*(m_ctxt->FRAME.Block()) == 0x00000 &&*/ m_ctxt->TEX0.TBP0 == 0x02320)
{
	if(m_stats.GetFrame() > 1200)
	{
		//hr = D3DXSaveTextureToFile(_T("c:\\rtbefore.bmp"), D3DXIFF_BMP, pRT, NULL);
	}
}

		hr = m_pD3DDev->SetFVF(D3DFVF_HWVERTEX);

		if(1)//!m_de.PABE.PABE)
		{
			hr = m_pD3DDev->DrawPrimitiveUP(m_primtype, nPrims, m_pVertices, sizeof(HWVERTEX));
		}
/*		else
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
*/
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

			if(mask || zwrite)
				hr = m_pD3DDev->DrawPrimitiveUP(m_primtype, nPrims, m_pVertices, sizeof(HWVERTEX));
		}

if(m_de.PRIM.TME && /*(m_ctxt->FRAME.Block()) == 0x00000 &&*/ m_ctxt->TEX0.TBP0 == 0x02320)
{
	if(m_stats.GetFrame() > 1200)
	{
		//hr = D3DXSaveTextureToFile(_T("c:\\rtafter.bmp"), D3DXIFF_BMP, pRT, NULL);
		//if(t.m_pTexture) hr = D3DXSaveTextureToFile(_T("c:\\tx.bmp"), D3DXIFF_BMP, t.m_pTexture, NULL);
	}
}
		hr = m_pD3DDev->EndScene();

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
	HRESULT hr;

	FlipSrc rt[2];

	for(int i = 0; i < countof(rt); i++)
	{
		if(!m_rs.IsEnabled(i)) continue;

		DWORD FBP = /*(::GetAsyncKeyState(VK_SPACE)&0x80000000) ? m_ctxt->FRAME.Block() :*/ (m_rs.DISPFB[i].FBP<<5);

		CSurfMap<IDirect3DTexture9>::CPair* pPair = m_pRenderTargets.PLookup(FBP);

		if(!pPair)
		{
			for(CSurfMap<IDirect3DTexture9>::CPair* pPair2 = m_pRenderTargets.PGetFirstAssoc(); 
				pPair2; 
				pPair2 = m_pRenderTargets.PGetNextAssoc(pPair2))
			{
				if(pPair2->key <= FBP && (!pPair || pPair2->key >= pPair->key))
				{
					pPair = pPair2;
				}
			}
		}

		if(pPair)
		{
			rt[i].pRT = pPair->value;
			m_tc.ResetAge(pPair->key);
			ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
			hr = rt[i].pRT->GetLevelDesc(0, &rt[i].rd);

			DWORD ssize = sizeof(rt[i].scale);
			rt[i].pRT->GetPrivateData(GUID_NULL, &rt[i].scale, &ssize);

			CSize size = m_rs.GetSize(i);
			rt[i].src = CRect(0, 0, rt[i].scale.x*size.cx, rt[i].scale.y*size.cy);
		}
	}

	bool fShiftField = m_rs.SMODE2.INT && !!(m_ctxt->XYOFFSET.OFY&0xf);
		// m_rs.CSRr.FIELD && m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/;

	FinishFlip(rt, fShiftField);

#ifdef DEBUG_RENDERTARGETS
	CRect dst(0, 0, m_bd.Width, m_bd.Height);

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

				hr = m_pD3DDev->SetPixelShader(NULL);

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

void GSRendererHW::InvalidateTexture(DWORD TBP0, int x, int y)
{
	if(m_rs.TRXREG.RRW <= 16 && m_rs.TRXREG.RRH <= 16) // allowing larger isn't worth, most of the time the size of the texture changes too
	{
		GSTexture t;
		D3DSURFACE_DESC desc;
		D3DLOCKED_RECT lr;
		CRect r(m_rs.TRXPOS.DSAX, y, m_rs.TRXREG.RRW, min(m_x == m_rs.TRXPOS.DSAX ? m_y : m_y+1, m_rs.TRXREG.RRH));
		int w = r.right, h = r.bottom;

		if(m_tc.LookupByTBP(TBP0, t) && !t.m_fRT
		&& !(t.m_tex.CLAMP.WMS&2) && !(t.m_tex.CLAMP.WMT&2)
		&& t.m_scale.x == 1.0f && t.m_scale.y == 1.0f
		&& S_OK == t.m_pTexture->GetLevelDesc(0, &desc)
		&& desc.Width >= w && desc.Height >= h
		&& S_OK == t.m_pTexture->LockRect(0, &lr, r, 0))
		{
			m_lm.setupCLUT(m_ctxt->TEX0, m_de.TEXCLUT, m_de.TEXA);

			GSLocalMemory::unSwizzleTexture st = m_lm.GetUnSwizzleTexture(m_ctxt->TEX0.PSM);
			(m_lm.*st)(w, h, (BYTE*)lr.pBits, lr.Pitch, m_ctxt->TEX0, m_de.TEXA);
/*
			BYTE* dst = (BYTE*)lr.pBits;
			GSLocalMemory::readTexel rt = m_lm.GetReadTexel(t.m_tex.TEX0.PSM);

			for(int y = 0, diff = lr.Pitch - w*4; y < h; y++, dst += diff)
				for(int x = 0; x < w; x++, dst += 4)
					*(DWORD*)dst = SwapRB((m_lm.*rt)(r.left + x, r.top + y, t.m_tex.TEX0, t.m_tex.TEXA));
*/
			t.m_pTexture->UnlockRect(0);

			m_stats.IncReads(w*h);
		}
	}
	else
	{
		m_tc.InvalidateByTBP(TBP0);
	}

	m_tc.InvalidateByCBP(TBP0);
}

void GSRendererHW::CalcRegionToUpdate(int& tw, int& th)
{
	if(m_ctxt->CLAMP.WMS < 3 && m_ctxt->CLAMP.WMT < 3)
	{
		float tumin, tvmin, tumax, tvmax;
		tumin = tvmin = +1e10;
		tumax = tvmax = -1e10;

		HWVERTEX* pVertices = m_pVertices;
		for(int i = m_nVertices; i-- > 0; pVertices++)
		{
			float tu = pVertices->tu;
			if(tumax < tu) tumax = tu;
			if(tumin > tu) tumin = tu;
			float tv = pVertices->tv;
			if(tvmax < tv) tvmax = tv;
			if(tvmin > tv) tvmin = tv;
		}

		if(m_ctxt->CLAMP.WMS == 0)
		{
			float fmin = floor(tumin);
			float fmax = floor(tumax);

			if(fmin != fmax) {tumin = 0; tumax = 1.0f;}
			else {tumin -= fmin; tumax -= fmax;}

			// FIXME
			if(tumin == 0 && tumax != 1.0f) tumax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMS == 1)
		{
			if(tumin < 0) tumin = 0;
			if(tumax > 1.0f) tumax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMS == 2)
		{
			float minu = 1.0f * m_ctxt->CLAMP.MINU / (1<<m_ctxt->TEX0.TW);
			float maxu = 1.0f * m_ctxt->CLAMP.MAXU / (1<<m_ctxt->TEX0.TW);
			if(tumin < minu) tumin = minu;
			if(tumax > maxu) tumax = maxu;
		}

		if(m_ctxt->CLAMP.WMT == 0)
		{
			float fmin = floor(tvmin);
			float fmax = floor(tvmax);

			if(fmin != fmax) {tvmin = 0; tvmax = 1.0f;}
			else {tvmin -= fmin; tvmax -= fmax;}

			// FIXME
			if(tvmin == 0 && tvmax != 1.0f) tvmax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMT == 1)
		{
			if(tvmin < 0) tvmin = 0;
			if(tvmax > 1.0f) tvmax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMT == 2)
		{
			float minv = 1.0f * m_ctxt->CLAMP.MINV / (1<<m_ctxt->TEX0.TH);
			float maxv = 1.0f * m_ctxt->CLAMP.MAXV / (1<<m_ctxt->TEX0.TH);
			if(tvmin < minv) tvmin = minv;
			if(tvmax > maxv) tvmax = maxv;
		}

		tumin *= tw;
		tumax *= tw;
		tvmin *= th;
		tvmax *= th;

		// TODO
		// tx = ;
		// ty = ;
		tw = min(((int)tumax + 1 + 31) & ~31, tw);
		th = min(((int)tvmax + 1 + 15) & ~15, th);
	}
}

bool GSRendererHW::CreateTexture(GSTexture& t)
{
	int tw = 1 << m_ctxt->TEX0.TW, tw0 = tw;
	int th = 1 << m_ctxt->TEX0.TH, th0 = th;

	HRESULT hr;
	CComPtr<IDirect3DTexture9> pTexture;

	tex_t tex;
	tex.TEX0 = m_ctxt->TEX0;
	tex.CLAMP = m_ctxt->CLAMP;
	tex.TEXA = m_de.TEXA;
	tex.TEXCLUT = m_de.TEXCLUT;

	if(m_tc.Lookup(tex, t))
	{
		if(t.m_fRT) return(true);

		CalcRegionToUpdate(tw, th);
		if(t.m_valid.cx >= tw && t.m_valid.cy >= th)
			return(true);

		pTexture = t.m_pTexture;
	}
	else
	{
		hr = m_tc.CreateTexture(m_ctxt->TEX0, m_pD3DDev, &pTexture);
		//hr = m_pD3DDev->CreateTexture(tw, th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
		if(FAILED(hr) || !pTexture) return(false);

		CalcRegionToUpdate(tw, th);
	}

	m_lm.setupCLUT(m_ctxt->TEX0, m_de.TEXCLUT, m_de.TEXA);

	GSLocalMemory::readTexel rt = m_lm.GetReadTexel(m_ctxt->TEX0.PSM);

	RECT rlock = {0, 0, tw, th};

	D3DLOCKED_RECT r;
	if(FAILED(hr = pTexture->LockRect(0, &r, tw == tw0 && th == th0 ? NULL : &rlock, 0)))
		return(false);

	BYTE* dst = (BYTE*)r.pBits;

#ifdef DEBUG
	memset(dst, 0xff, r.Pitch*(1<<m_ctxt->TEX0.TH));
#endif

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

				*(DWORD*)dst = SwapRB((m_lm.*rt)(tx, ty, m_ctxt->TEX0, m_de.TEXA));
			}
		}
	}
	else
	{
		GSLocalMemory::unSwizzleTexture st = m_lm.GetUnSwizzleTexture(m_ctxt->TEX0.PSM);
		(m_lm.*st)(tw, th, dst, r.Pitch, m_ctxt->TEX0, m_de.TEXA);
	}

	pTexture->UnlockRect(0);

	m_tc.Add(tex, scale_t(1, 1), pTexture, CSize(tw, th));
	if(!m_tc.Lookup(tex, t)) ASSERT(0); // ehe

	m_stats.IncReads(tw*th);

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

void GSRendererHW::SetupTexture(const GSTexture& t)
{
	HRESULT hr;

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	float fConstData[] = 
	{
		m_ctxt->TEX0.TFX, !!m_ctxt->TEX0.TCC, t.m_fRT, !!(m_de.PRIM.TME && t.m_pTexture),
		m_ctxt->TEX0.PSM, m_de.TEXA.AEM, (float)m_de.TEXA.TA0 / 255, (float)m_de.TEXA.TA1 / 255,
	};

	hr = m_pD3DDev->SetPixelShaderConstantF(0, fConstData, countof(fConstData)/4);

	if(m_de.PRIM.TME && t.m_pTexture)
	{
		hr = m_pD3DDev->SetTexture(0, t.m_pTexture);

		D3DTEXTUREADDRESS u, v;

		switch(m_ctxt->CLAMP.WMS)
		{
		case 0: case 3: u = D3DTADDRESS_WRAP; break; // repeat
		case 1: case 2: u = D3DTADDRESS_CLAMP; break; // clamp
		}

		switch(m_ctxt->CLAMP.WMT)
		{
		case 0: case 3: v = D3DTADDRESS_WRAP; break; // repeat
		case 1: case 2: v = D3DTADDRESS_CLAMP; break; // clamp
		}

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, u);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, v);

		if(!pPixelShader && !m_fDisableShaders && m_caps.PixelShaderVersion >= D3DVS_VERSION(2, 0))
		{
			pPixelShader = m_pPixelShaderTFX[m_ctxt->TEX0.TFX];
		}

		if(!pPixelShader && !m_fDisableShaders && m_caps.PixelShaderVersion >= D3DVS_VERSION(1, 1))
		{
			switch(m_ctxt->TEX0.TFX)
			{
			case 0:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[0];
				else if(!t.m_fRT) pPixelShader = m_pPixelShaders[1];
				else pPixelShader = m_pPixelShaders[2];
				break;
			case 1:
				if(!t.m_fRT) pPixelShader = m_pPixelShaders[3];
				else pPixelShader = m_pPixelShaders[4];
				break;
			case 2:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[5];
				else if(!t.m_fRT) pPixelShader = m_pPixelShaders[6];
				else pPixelShader = m_pPixelShaders[7];
				break;
			case 3:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[8];
				else if(!t.m_fRT) pPixelShader = m_pPixelShaders[9];
				else pPixelShader = m_pPixelShaders[10];
				break;
			}
		}

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
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, t.m_fRT ? D3DTOP_MODULATE2X : D3DTOP_MODULATE4X);
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
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, t.m_fRT ? D3DTOP_SELECTARG1 : D3DTOP_ADD);
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
					ASSERT(!t.m_fRT); // FIXME
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
					ASSERT(!t.m_fRT); // FIXME
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
		hr = m_pD3DDev->SetTexture(0, NULL);

		if(!pPixelShader && !m_fDisableShaders && m_caps.PixelShaderVersion >= D3DVS_VERSION(2, 0))
		{
			pPixelShader = m_pPixelShaderTFX[4];
		}

		if(!pPixelShader && !m_fDisableShaders && m_caps.PixelShaderVersion >= D3DVS_VERSION(1, 1))
		{
			pPixelShader = m_pPixelShaders[11];
		}

		if(!pPixelShader)
		{
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
	}

	hr = m_pD3DDev->SetTexture(1, NULL);

	hr = m_pD3DDev->SetPixelShader(pPixelShader);
}

void GSRendererHW::SetupAlphaBlend()
{
	HRESULT hr;

	DWORD ABE = FALSE;
	hr = m_pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &ABE);

	bool fABE = m_de.PRIM.ABE || (m_primtype == D3DPT_LINELIST || m_primtype == D3DPT_LINESTRIP) && m_de.PRIM.AA1; // FIXME
	if(fABE != !!ABE)
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
	}
}
