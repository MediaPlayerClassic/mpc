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
#include "GSRendererSoft.h"
#include "x86.h"

template <class Vertex>
GSRendererSoft<Vertex>::GSRendererSoft(HWND hWnd, HRESULT& hr)
	: GSRenderer<Vertex>(640, 512, hWnd, hr)
{
	float f = 0.8;
	f *= UINT_MAX;
	DWORD dw = (DWORD)f;

	Reset();

	int i = SHRT_MIN, j = 0;
	for(; i < 0; i++, j++) m_clip[j] = 0, m_mask[j] = j&255;
	for(; i < 256; i++, j++) m_clip[j] = i, m_mask[j] = j&255;
	for(; i < SHRT_MAX; i++, j++) m_clip[j] = 255, m_mask[j] = j&255;
}

template <class Vertex>
GSRendererSoft<Vertex>::~GSRendererSoft()
{
}

template <class Vertex>
HRESULT GSRendererSoft<Vertex>::ResetDevice(bool fForceWindowed)
{
	m_pRT[0] = NULL;
	m_pRT[1] = NULL;

	return __super::ResetDevice(fForceWindowed);
}

template <class Vertex>
void GSRendererSoft<Vertex>::Reset()
{
	m_primtype = PRIM_NONE;
	m_pTexture = NULL;

	__super::Reset();
}

template <class Vertex>
int GSRendererSoft<Vertex>::DrawingKick(bool fSkip)
{
	Vertex* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

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
		/*
		m_primtype = PRIM_TRIANGLE;
		nVertices += 2;
		pVertices[5] = pVertices[3];
		pVertices[3] = pVertices[1];
		pVertices[4] = pVertices[2];
		*/
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
		return 0;
	}

	if(fSkip || !m_rs.IsEnabled(0) && !m_rs.IsEnabled(1))
		return 0;

	if(!m_de.pPRIM->IIP)
		memcpy(&pVertices[0], &pVertices[nVertices-1], sizeof(DWORD)*4); // copy RGBA-only

	return nVertices;
}

template <class Vertex>
void GSRendererSoft<Vertex>::FlushPrim()
{
	if(m_nVertices > 0)
	{
		SetTexture();

		m_scissor.SetRect(
			max(m_ctxt->SCISSOR.SCAX0, 0),
			max(m_ctxt->SCISSOR.SCAY0, 0),
			min(m_ctxt->SCISSOR.SCAX1+1, m_ctxt->FRAME.FBW * 64),
			min(m_ctxt->SCISSOR.SCAY1+1, 4096));

		m_clamp = (m_de.COLCLAMP.CLAMP ? m_clip : m_mask) + 32768;

		int nPrims = 0;
		Vertex* pVertices = m_pVertices;

		switch(m_primtype)
		{
		case PRIM_SPRITE:
			ASSERT(!(m_nVertices&3));
			nPrims = m_nVertices / 4;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices += 4) DrawSprite(pVertices);
			break;
		case PRIM_TRIANGLE:
			ASSERT(!(m_nVertices%3));
			nPrims = m_nVertices / 3;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices += 3) DrawTriangle(pVertices);
			break;
		case PRIM_LINE: 
			ASSERT(!(m_nVertices&1));
			nPrims = m_nVertices / 2;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices += 2) DrawLine(pVertices);
			break;
		case PRIM_POINT:
			nPrims = m_nVertices;
			LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);
			for(int i = 0; i < nPrims; i++, pVertices++) DrawPoint(pVertices);
			break;
		default:
			ASSERT(m_nVertices == 0);
			return;
		}

		m_perfmon.IncCounter(GSPerfMon::c_prim, nPrims);
	}

	m_primtype = PRIM_NONE;

	__super::FlushPrim();
}

template <class Vertex>
void GSRendererSoft<Vertex>::Flip()
{
	HRESULT hr;

	FlipInfo rt[2];

	for(int i = 0; i < countof(rt); i++)
	{
		if(m_rs.IsEnabled(i))
		{
			CRect rect = CRect(CPoint(0, 0), m_rs.GetDispRect(i).BottomRight());

			//GSLocalMemory::RoundUp(, GSLocalMemory::GetBlockSize(m_rs.DISPFB[i].PSM));

			ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
			if(m_pRT[i]) m_pRT[i]->GetLevelDesc(0, &rt[i].rd);

			if(rt[i].rd.Width != rect.right || rt[i].rd.Height != rect.bottom)
				m_pRT[i] = NULL;

			if(!m_pRT[i])
			{
				CComPtr<IDirect3DTexture9> pRT;
				D3DLOCKED_RECT lr;
				int nTries = 0, nMaxTries = 10;
				do
				{
					pRT = NULL;
					hr = m_pD3DDev->CreateTexture(rect.right, rect.bottom, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pRT, NULL);
					if(FAILED(hr)) break;
					if(SUCCEEDED(pRT->LockRect(0, &lr, NULL, 0)))
						pRT->UnlockRect(0);
					m_pRT[i] = pRT;
				}
				while((((DWORD_PTR)lr.pBits & 0xf) || (lr.Pitch & 0xf)) && ++nTries < nMaxTries);

				if(nTries == nMaxTries) continue;

				ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
				hr = m_pRT[i]->GetLevelDesc(0, &rt[i].rd);
			}

			rt[i].pRT = m_pRT[i];

			rt[i].scale = scale_t(1, 1);

			D3DLOCKED_RECT lr;
			if(FAILED(hr = rt[i].pRT->LockRect(0, &lr, NULL, 0)))
				continue;

			GIFRegTEX0 TEX0;
			TEX0.TBP0 = m_rs.DISPFB[i].FBP<<5;
			TEX0.TBW = m_rs.DISPFB[i].FBW;
			TEX0.PSM = m_rs.DISPFB[i].PSM;

			GIFRegCLAMP CLAMP;
			CLAMP.WMS = CLAMP.WMT = 1;

#ifdef DEBUG_RENDERTARGETS
			if(::GetAsyncKeyState(VK_SPACE)&0x80000000) TEX0.TBP0 = m_ctxt->FRAME.Block();

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
#endif

			m_lm.ReadTexture(rect, (BYTE*)lr.pBits, lr.Pitch, TEX0, m_de.TEXA, CLAMP);

			rt[i].pRT->UnlockRect(0);
		}
	}

	FinishFlip(rt);
}

template <class Vertex>
void GSRendererSoft<Vertex>::EndFrame()
{
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawPoint(Vertex* v)
{
	CPoint p = *v;
	if(m_scissor.PtInRect(p)) DrawVertex(p.x, p.y, *v);
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawLine(Vertex* v)
{
	Vertex::scalar_t dx = abs(v[1].x - v[0].x);
	Vertex::scalar_t dy = abs(v[1].y - v[0].y);

	if(dx == 0 && dy == 0) return;

	int i = dx > dy ? 4 : 5;

	Vertex edge = v[0];
	Vertex dedge = (v[1] - v[0]) / abs(v[1].f[i] - v[0].f[i]);

	// TODO: clip with the scissor

	int steps = abs(Vertex::Int(v[0].f[i]) - Vertex::Int(v[1].f[i]));

	while(steps-- > 0)
	{
		CPoint p = edge;
		if(m_scissor.PtInRect(p)) DrawVertex(p.x, p.y, edge);
		edge += dedge;
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawTriangle(Vertex* v)
{
	if(v[1].y < v[0].y) {Vertex::Exchange(v[0], v[1]);}
	if(v[2].y < v[0].y) {Vertex::Exchange(v[0], v[2]);}
	if(v[2].y < v[1].y) {Vertex::Exchange(v[1], v[2]);}

	if(v[0].y >= v[2].y) return;

	Vertex v01 = v[1] - v[0];
	Vertex v02 = v[2] - v[0];

	Vertex::scalar_t temp = Vertex::Div(v01.y, v02.y);
	Vertex::scalar_t longest = Vertex::Mul(temp, v02.x) - v01.x;

	int ledge, redge;
	if(longest > 0) {ledge = 0; redge = 1;}
	else if(longest < 0) {ledge = 1; redge = 0;}
	else return;

	Vertex edge[2];
	edge[ledge] = edge[redge] = v[0];

	Vertex dedge[2];
	dedge[0].y = dedge[1].y = Vertex::Scalar(1);
	if(v01.y) dedge[ledge] = v01 / v01.y;
	if(v02.y) dedge[redge] = v02 / v02.y;

	Vertex dscan = (v02 * temp - v01) / longest;
	dscan.y = 0;

	for(int i = 0; i < 2; i++, v++)
	{ 
		int top = Vertex::CeilInt(edge[0].y), bottom = Vertex::CeilInt(v[1].y);
		if(top < m_scissor.top) top = min(m_scissor.top, bottom);
		if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;
		if(Vertex::Scalar(top) > edge[0].y) // for(int j = 0; j < 2; j++) edge[j] += dedge[j] * ((float)top - edge[0].y);
		{
			Vertex::scalar_t dy = Vertex::Scalar(top) - edge[0].y;
			edge[0] += dedge[0] * dy;
			edge[1].x += Vertex::Mul(dedge[1].x, dy);
			edge[0].y = edge[1].y = Vertex::Scalar(top);
		}

		ASSERT(top >= bottom || Vertex::Int(abs(edge[1].y - edge[0].y) * 10) == 0);

		for(; top < bottom; top++)
		{
			Vertex scan = edge[0];

			int left = Vertex::CeilInt(edge[0].x), right = Vertex::CeilInt(edge[1].x);
			if(left < m_scissor.left) left = m_scissor.left;
			if(right > m_scissor.right) right = m_scissor.right;
			if(Vertex::Scalar(left) > edge[0].x) scan += dscan * (Vertex::Scalar(left) - edge[0].x);

			for(; left < right; left++)
			{
				//ASSERT(left == Vertex::Int(scan.x) && top == Vertex::Int(scan.y));

				DrawVertex(left, top, scan);
				scan += dscan;
			}

			// for(int j = 0; j < 2; j++) edge[j] += dedge[j];
			edge[0] += dedge[0];
			edge[1].x += dedge[1].x;
			edge[1].y += dedge[1].y;
		}

		if(v[2].y > v[1].y)
		{
			edge[ledge] = v[1];
			dedge[ledge] = (v[2] - v[1]) / (v[2].y - v[1].y);
			edge[ledge] += dedge[ledge] * (Vertex::Ceil(edge[ledge].y) - edge[ledge].y);
		}
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawSprite(Vertex* v)
{
	if(v[2].y < v[0].y) {Vertex::Exchange(v[0], v[2]); Vertex::Exchange(v[1], v[3]);}
	if(v[1].x < v[0].x) {Vertex::Exchange(v[0], v[1]); Vertex::Exchange(v[2], v[3]);}

	if(v[0].x == v[1].x || v[0].y == v[2].y) return;

	Vertex v01 = v[1] - v[0];
	Vertex v02 = v[2] - v[0];

	Vertex edge = v[0];
	Vertex dedge = v02 / v02.y;
	Vertex dscan = v01 / v01.x;

	int top = Vertex::CeilInt(v[0].y), bottom = Vertex::CeilInt(v[2].y);
	if(top < m_scissor.top) top = min(m_scissor.top, bottom);
	if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;
	if(Vertex::Scalar(top) > v[0].y) edge += dedge * (Vertex::Scalar(top) - v[0].y);

	int left = Vertex::CeilInt(v[0].x), right = Vertex::CeilInt(v[1].x);
	if(left < m_scissor.left) left = m_scissor.left;
	if(right > m_scissor.right) right = m_scissor.right;
	if(Vertex::Scalar(left) > v[0].x) edge += dscan * (Vertex::Scalar(left) - v[0].x);

	if(DrawFilledRect(left, top, right, bottom, edge))
		return;

	for(; top < bottom; top++)
	{
		Vertex scan = edge;

		for(int x = left; x < right; x++)
		{
			DrawVertex(x, top, scan);
			scan += dscan;
		}

		edge += dedge;
	}
}

template <class Vertex>
bool GSRendererSoft<Vertex>::DrawFilledRect(int left, int top, int right, int bottom, Vertex& v)
{
	if(left >= right || top >= bottom)
		return false;

	ASSERT(top >= 0);
	ASSERT(bottom >= 0);

	if(m_de.pPRIM->IIP
	|| m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST != 1
	|| m_ctxt->TEST.ATE && m_ctxt->TEST.ATST != 1
	|| m_ctxt->TEST.DATE
	|| m_de.pPRIM->TME
	|| m_de.pPRIM->ABE
	|| m_de.pPRIM->FGE
	|| m_de.DTHE.DTHE
	|| m_ctxt->FRAME.FBMSK)
		return false;

	DWORD FBP = m_ctxt->FRAME.FBP<<5, FBW = m_ctxt->FRAME.FBW;
	DWORD ZBP = m_ctxt->ZBUF.ZBP<<5;

	if(!m_ctxt->ZBUF.ZMSK)
	{
		m_lm.FillRect(CRect(left, top, right, bottom), v.GetZ(), m_ctxt->ZBUF.PSM, ZBP, FBW);
	}

	__declspec(align(16)) union {struct {int Rf, Gf, Bf, Af;}; int Cf[4];};
	v.GetColor(Cf);

	Rf = m_clamp[Rf];
	Gf = m_clamp[Gf];
	Bf = m_clamp[Bf];
	Af = m_clamp[Af]; // ?

	Af |= (m_ctxt->FBA.FBA << 7);

	DWORD c = (Af << 24) | (Bf << 16) | (Gf << 8) | (Rf << 0);
	if(m_ctxt->FRAME.PSM == PSM_PSMCT16 || m_ctxt->FRAME.PSM == PSM_PSMCT16S)
		c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	m_lm.FillRect(CRect(left, top, right, bottom), c, m_ctxt->FRAME.PSM, FBP, FBW);

	return true;
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawVertex(int x, int y, Vertex& v)
{
	GSLocalMemory::psmtbl_t& ttbl = GSLocalMemory::m_psmtbl[m_ctxt->TEX0.PSM];
	GSLocalMemory::psmtbl_t& ztbl = GSLocalMemory::m_psmtbl[m_ctxt->ZBUF.PSM];
	GSLocalMemory::psmtbl_t& ftbl = GSLocalMemory::m_psmtbl[m_ctxt->FRAME.PSM];

	DWORD addrz = 0;

	if(m_ctxt->ZBUF.ZMSK == 0 || m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST >= 2)
	{
		addrz = (ztbl.pa)(x, y, m_ctxt->ZBUF.ZBP<<5, m_ctxt->FRAME.FBW);
	}

	DWORD vz = v.GetZ();

	if(m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST != 1)
	{
		if(m_ctxt->TEST.ZTST == 0)
			return;

		DWORD z = (m_lm.*ztbl.rpa)(addrz);
		if(m_ctxt->TEST.ZTST == 2 && vz < z || m_ctxt->TEST.ZTST == 3 && vz <= z)
			return;
	}

	__declspec(align(16)) union {struct {int Rf, Gf, Bf, Af;}; int Cf[4]; __m128i RGBAf;};
	v.GetColor(Cf);

	if(m_de.pPRIM->TME)
	{
		int tw = 1 << m_ctxt->TEX0.TW;
		int th = 1 << m_ctxt->TEX0.TH;

		float rq = 1.0f / v.q;
		float tu = (float)v.u * rq * tw;
		float tv = (float)v.v * rq * th;

		// TODO
		// float lod = m_ctxt->TEX1.K;
		// if(!m_ctxt->TEX1.LCM) lod += log2(1/v.q) << m_ctxt->TEX1.L;

		float ftu = modf(tu, &tu); // tu - 0.5f
		float ftv = modf(tv, &tv); // tv - 0.5f
		if(ftu < 0) ftu += 1;
		if(ftv < 0) ftv += 1;
		float iftu = 1.0f - ftu;
		float iftv = 1.0f - ftv;

		int itu[2] = {(int)tu, (int)tu+1};
		int itv[2] = {(int)tv, (int)tv+1};

		for(int i = 0; i < countof(itu); i++)
		{
			switch(m_ctxt->CLAMP.WMS)
			{
			case 0: itu[i] = itu[i] & (tw-1); break;
			case 1: itu[i] = itu[i] < 0 ? 0 : itu[i] >= tw ? itu[i] = tw-1 : itu[i]; break;
			case 2: itu[i] = itu[i] < m_ctxt->CLAMP.MINU ? m_ctxt->CLAMP.MINU : itu[i] > m_ctxt->CLAMP.MAXU ? m_ctxt->CLAMP.MAXU : itu[i]; break;
			case 3: itu[i] = (itu[i] & m_ctxt->CLAMP.MINU) | m_ctxt->CLAMP.MAXU; break;
			}

			ASSERT(itu[i] >= 0 && itu[i] < tw);
		}

		for(int i = 0; i < countof(itv); i++)
		{
			switch(m_ctxt->CLAMP.WMT)
			{
			case 0: itv[i] = itv[i] & (th-1); break;
			case 1: itv[i] = itv[i] < 0 ? 0 : itv[i] >= th ? itv[i] = th-1 : itv[i]; break;
			case 2: itv[i] = itv[i] < m_ctxt->CLAMP.MINV ? m_ctxt->CLAMP.MINV : itv[i] > m_ctxt->CLAMP.MAXV ? m_ctxt->CLAMP.MAXV : itv[i]; break;
			case 3: itv[i] = (itv[i] & m_ctxt->CLAMP.MINV) | m_ctxt->CLAMP.MAXV; break;
			}

			ASSERT(itv[i] >= 0 && itv[i] < th);
		}

		DWORD c[4];
		WORD Rt, Gt, Bt, At;

		// if(m_ctxt->TEX1.MMAG&1) // FIXME
		{
			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[0]];
				c[1] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[1]];
				c[2] = m_pTexture[(itv[1] << m_ctxt->TEX0.TW) + itu[0]];
				c[3] = m_pTexture[(itv[1] << m_ctxt->TEX0.TW) + itu[1]];
			}
			else
			{
				c[0] = (m_lm.*ttbl.rt)(itu[0], itv[0], m_ctxt->TEX0, m_de.TEXA);
				c[1] = (m_lm.*ttbl.rt)(itu[1], itv[0], m_ctxt->TEX0, m_de.TEXA);
				c[2] = (m_lm.*ttbl.rt)(itu[0], itv[1], m_ctxt->TEX0, m_de.TEXA);
				c[3] = (m_lm.*ttbl.rt)(itu[1], itv[1], m_ctxt->TEX0, m_de.TEXA);
			}

			float iuiv = iftu*iftv;
			float uiv = ftu*iftv;
			float iuv = iftu*ftv;
			float uv = ftu*ftv;

			Rt = (WORD)(iuiv*((c[0]>> 0)&0xff) + uiv*((c[1]>> 0)&0xff) + iuv*((c[2]>> 0)&0xff) + uv*((c[3]>> 0)&0xff) + 0.5f);
			Gt = (WORD)(iuiv*((c[0]>> 8)&0xff) + uiv*((c[1]>> 8)&0xff) + iuv*((c[2]>> 8)&0xff) + uv*((c[3]>> 8)&0xff) + 0.5f);
			Bt = (WORD)(iuiv*((c[0]>>16)&0xff) + uiv*((c[1]>>16)&0xff) + iuv*((c[2]>>16)&0xff) + uv*((c[3]>>16)&0xff) + 0.5f);
			At = (WORD)(iuiv*((c[0]>>24)&0xff) + uiv*((c[1]>>24)&0xff) + iuv*((c[2]>>24)&0xff) + uv*((c[3]>>24)&0xff) + 0.5f);

			// TESTME
			/*
			float c01, c23;
			c01 = c[0] + (c[0] - c[1])*ftu;
			c23 = c[2] + (c[2] - c[3])*ftu;
			Rt = c01 + (c23 - c01)*ftv;
			for(int i = 0; i < 4; i++) c[i] >>= 8;
			c01 = c[0] + (c[0] - c[1])*ftu;
			c23 = c[2] + (c[2] - c[3])*ftu;
			Gt = c01 + (c23 - c01)*ftv;
			for(int i = 0; i < 4; i++) c[i] >>= 8;
			c01 = c[0] + (c[0] - c[1])*ftu;
			c23 = c[2] + (c[2] - c[3])*ftu;
			Bt = c01 + (c23 - c01)*ftv;
			for(int i = 0; i < 4; i++) c[i] >>= 8;
			c01 = c[0] + (c[0] - c[1])*ftu;
			c23 = c[2] + (c[2] - c[3])*ftu;
			At = c01 + (c23 - c01)*ftv;
			*/
		}
/*		else 
		{
			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[0]];
			}
			else
			{
				c[0] = (m_lm.*m_ctxt->rt)(itu[0], itv[0], m_ctxt->TEX0, m_de.TEXA);
			}

			Rt = (BYTE)((c[0]>>0)&0xff);
			Gt = (BYTE)((c[0]>>8)&0xff);
			Bt = (BYTE)((c[0]>>16)&0xff);
			At = (BYTE)((c[0]>>24)&0xff);
		}
*/
		switch(m_ctxt->TEX0.TFX)
		{
		case 0:
			Rf = Rf * Rt >> 7;
			Gf = Gf * Gt >> 7;
			Bf = Bf * Bt >> 7;
			Af = m_ctxt->TEX0.TCC ? (Af * At >> 7) : Af;
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
			Af = m_ctxt->TEX0.TCC ? (Af + At) : Af;
			break;
		case 3:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = m_ctxt->TEX0.TCC ? At : Af;
			break;
		}
	}

	SaturateColor(&Cf[0]);

	if(m_de.pPRIM->FGE)
	{
		BYTE F = v.GetFog();
		Rf = (F * Rf + (255 - F) * m_de.FOGCOL.FCR) >> 8;
		Gf = (F * Gf + (255 - F) * m_de.FOGCOL.FCG) >> 8;
		Bf = (F * Bf + (255 - F) * m_de.FOGCOL.FCB) >> 8;
	}

	BOOL ZMSK = m_ctxt->ZBUF.ZMSK;
	DWORD FBMSK = m_ctxt->FRAME.FBMSK;

	if(m_ctxt->TEST.ATE)
	{
		bool fPass = true;

		switch(m_ctxt->TEST.ATST)
		{
		case 0: fPass = false; break;
		case 1: fPass = true; break;
		case 2: fPass = Af < m_ctxt->TEST.AREF; break;
		case 3: fPass = Af <= m_ctxt->TEST.AREF; break;
		case 4: fPass = Af == m_ctxt->TEST.AREF; break;
		case 5: fPass = Af >= m_ctxt->TEST.AREF; break;
		case 6: fPass = Af > m_ctxt->TEST.AREF; break;
		case 7: fPass = Af != m_ctxt->TEST.AREF; break;
		}

		if(!fPass)
		{
			switch(m_ctxt->TEST.AFAIL)
			{
			case 0: return;
			case 1: ZMSK = 1; break; // RGBA
			case 2: FBMSK = 0xffffffff; break; // Z
			case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
			}
		}
	}

	if(!ZMSK)
	{
		(m_lm.*ztbl.wpa)(addrz, vz);
	}

	if(FBMSK != ~0)
	{
		DWORD addr = (ftbl.pa)(x, y, m_ctxt->FRAME.FBP<<5, m_ctxt->FRAME.FBW);

		if(m_ctxt->TEST.DATE && m_ctxt->FRAME.PSM <= PSM_PSMCT16S && m_ctxt->FRAME.PSM != PSM_PSMCT24)
		{
			BYTE A = (BYTE)((m_lm.*ftbl.rpa)(addr) >> (m_ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15));
			if(A ^ m_ctxt->TEST.DATM) return; // FIXME: vf4 missing mem card screen / the blue background of the text
		}

		// FIXME: for AA1 the value of Af should be calculated from the pixel coverage...

		bool fABE = (m_de.pPRIM->ABE || (m_de.pPRIM->PRIM == 1 || m_de.pPRIM->PRIM == 2) && m_de.pPRIM->AA1) && (!m_de.PABE.PABE || (Af&0x80));

		DWORD Cd = 0;

		if(FBMSK || fABE)
		{
			Cd = (m_lm.*ftbl.rta)(addr, m_ctxt->TEX0, m_de.TEXA);
		}

		if(fABE)
		{
			BYTE R[3] = {Rf, (Cd>>0)&0xff, 0};
			BYTE G[3] = {Gf, (Cd>>8)&0xff, 0};
			BYTE B[3] = {Bf, (Cd>>16)&0xff, 0};
			BYTE A[3] = {Af, (Cd>>24)&0xff, m_ctxt->ALPHA.FIX};

			BYTE ALPHA_A = m_ctxt->ALPHA.A;
			BYTE ALPHA_B = m_ctxt->ALPHA.B;
			BYTE ALPHA_C = m_ctxt->ALPHA.C;
			BYTE ALPHA_D = m_ctxt->ALPHA.D;

			Rf = ((R[ALPHA_A] - R[ALPHA_B]) * A[ALPHA_C] >> 7) + R[ALPHA_D];
			Gf = ((G[ALPHA_A] - G[ALPHA_B]) * A[ALPHA_C] >> 7) + G[ALPHA_D];
			Bf = ((B[ALPHA_A] - B[ALPHA_B]) * A[ALPHA_C] >> 7) + B[ALPHA_D];
		}

		if(m_de.DTHE.DTHE)
		{
			WORD DMxy = (*((WORD*)&m_de.DIMX.i64 + (y&3)) >> ((x&3)<<2)) & 7;
			Rf += DMxy;
			Gf += DMxy;
			Bf += DMxy;
		}

		ASSERT(Rf >= SHRT_MIN && Rf < SHRT_MAX);
		ASSERT(Gf >= SHRT_MIN && Gf < SHRT_MAX);
		ASSERT(Bf >= SHRT_MIN && Bf < SHRT_MAX);
		ASSERT(Af >= SHRT_MIN && Af < SHRT_MAX);

		Rf = m_clamp[Rf];
		Gf = m_clamp[Gf];
		Bf = m_clamp[Bf];
		Af = m_clamp[Af]; // ?

		Af |= (m_ctxt->FBA.FBA << 7);

		Cd = (((Af << 24) | (Bf << 16) | (Gf << 8) | (Rf << 0)) & ~FBMSK) | (Cd & FBMSK);

		(m_lm.*ftbl.wfa)(addr, Cd);
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::SetTexture()
{
	if(m_de.pPRIM->TME)
	{
		m_lm.SetupCLUT32(m_ctxt->TEX0, m_de.TEXA);
	}
}

//
// GSRendererSoftFP
//

GSRendererSoftFP::GSRendererSoftFP(HWND hWnd, HRESULT& hr)
	: GSRendererSoft<GSSoftVertexFP>(hWnd, hr)
{
}

void GSRendererSoftFP::VertexKick(bool fSkip)
{
	GSSoftVertexFP& v = m_vl.AddTail();

	v.x = ((float)m_v.XYZ.X - m_ctxt->XYOFFSET.OFX) / 16;
	v.y = ((float)m_v.XYZ.Y - m_ctxt->XYOFFSET.OFY) / 16;
	v.z = (float)m_v.XYZ.Z / UINT_MAX;
	v.q = m_v.RGBAQ.Q == 0 ? 1.0f : m_v.RGBAQ.Q;

	v.r = (float)m_v.RGBAQ.R;
	v.g = (float)m_v.RGBAQ.G;
	v.b = (float)m_v.RGBAQ.B;
	v.a = (float)m_v.RGBAQ.A;

	v.fog = (float)m_v.FOG.F;

	if(m_de.pPRIM->TME)
	{
		if(m_de.pPRIM->FST)
		{
			v.u = (float)m_v.UV.U / (16<<m_ctxt->TEX0.TW);
			v.v = (float)m_v.UV.V / (16<<m_ctxt->TEX0.TH);
			v.q = 1.0f;
		}
		else
		{
			v.u = m_v.ST.S;
			v.v = m_v.ST.T;
		}
	}

	__super::VertexKick(fSkip);
}

//
// GSRendererSoftFX
//

GSRendererSoftFX::GSRendererSoftFX(HWND hWnd, HRESULT& hr)
	: GSRendererSoft<GSSoftVertexFX>(hWnd, hr)
{
}

void GSRendererSoftFX::VertexKick(bool fSkip)
{
	GSSoftVertexFX& v = m_vl.AddTail();

	v.x = ((int)m_v.XYZ.X - m_ctxt->XYOFFSET.OFX) << 12;
	v.y = ((int)m_v.XYZ.Y - m_ctxt->XYOFFSET.OFY) << 12;
	v.z = (unsigned __int64)m_v.XYZ.Z << 32;
	v.q = m_v.RGBAQ.Q == 0 ? INT_MAX : (__int64)(m_v.RGBAQ.Q * INT_MAX);

	v.r = m_v.RGBAQ.R << 16;
	v.g = m_v.RGBAQ.G << 16;
	v.b = m_v.RGBAQ.B << 16;
	v.a = m_v.RGBAQ.A << 16;

	v.fog = m_v.FOG.F << 16;

	if(m_de.pPRIM->TME)
	{
		if(m_de.pPRIM->FST)
		{
			v.u = (__int64)(((float)m_v.UV.U / (16<<m_ctxt->TEX0.TW)) * INT_MAX);
			v.v = (__int64)(((float)m_v.UV.V / (16<<m_ctxt->TEX0.TH)) * INT_MAX);
			v.q = INT_MAX;
		}
		else
		{
			v.u = (__int64)(m_v.ST.S * INT_MAX);
			v.v = (__int64)(m_v.ST.T * INT_MAX);
		}
	}

	__super::VertexKick(fSkip);
}

void GSRendererSoftFX::DrawVertex(int x, int y, GSSoftVertexFX& v)
{
	GSLocalMemory::psmtbl_t& ttbl = GSLocalMemory::m_psmtbl[m_ctxt->TEX0.PSM];
	GSLocalMemory::psmtbl_t& ztbl = GSLocalMemory::m_psmtbl[m_ctxt->ZBUF.PSM];
	GSLocalMemory::psmtbl_t& ftbl = GSLocalMemory::m_psmtbl[m_ctxt->FRAME.PSM];

	DWORD addrz = 0;

	if(m_ctxt->ZBUF.ZMSK == 0 || m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST >= 2)
	{
		addrz = (ztbl.pa)(x, y, m_ctxt->ZBUF.ZBP<<5, m_ctxt->FRAME.FBW);
	}

	DWORD vz = v.GetZ();

	if(m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST != 1)
	{
		if(m_ctxt->TEST.ZTST == 0)
			return;

		DWORD z = (m_lm.*ztbl.rpa)(addrz);
		if(m_ctxt->TEST.ZTST == 2 && vz < z || m_ctxt->TEST.ZTST == 3 && vz <= z)
			return;
	}

	__declspec(align(16)) union {struct {int Rf, Gf, Bf, Af;}; int Cf[4];};
	v.GetColor(Cf);

	if(m_de.pPRIM->TME)
	{
		int tw = 1 << m_ctxt->TEX0.TW;
		int th = 1 << m_ctxt->TEX0.TH;
		__int64 tu = (v.u << 16 << m_ctxt->TEX0.TW) / v.q;
		__int64 tv = (v.v << 16 << m_ctxt->TEX0.TH) / v.q;

		// TODO
/*		static const float log_2 = log(2.0f);
		float lod = m_ctxt->TEX1.K;
		//if(!m_ctxt->TEX1.LCM) lod += (int)(-log((float)v.q/INT_MAX)/log_2) << m_ctxt->TEX1.L;
		if(!m_ctxt->TEX1.LCM) lod += (int)(-log((float)v.q) / log_2 + 31) << m_ctxt->TEX1.L;
*/
		DWORD ftu = (((DWORD)tu)&0xffff) >> 1, iftu = (1<<15) - ftu;
		DWORD ftv = (((DWORD)tv)&0xffff) >> 1, iftv = (1<<15) - ftv;
		tu >>= 16;
		tv >>= 16;

		int itu[2] = {(int)tu, (int)tu+1};
		int itv[2] = {(int)tv, (int)tv+1};

		for(int i = 0; i < countof(itu); i++)
		{
			switch(m_ctxt->CLAMP.WMS)
			{
			case 0: itu[i] = itu[i] & (tw-1); break;
			case 1: itu[i] = itu[i] < 0 ? 0 : itu[i] >= tw ? itu[i] = tw-1 : itu[i]; break;
			case 2: itu[i] = itu[i] < m_ctxt->CLAMP.MINU ? m_ctxt->CLAMP.MINU : itu[i] > m_ctxt->CLAMP.MAXU ? m_ctxt->CLAMP.MAXU : itu[i]; if(itu[i] > tw-1) itu[i] = tw-1; break;
			case 3: itu[i] = (itu[i] & m_ctxt->CLAMP.MINU) | m_ctxt->CLAMP.MAXU; break;
			}

			ASSERT(itu[i] >= 0 && itu[i] < tw);
		}

		for(int i = 0; i < countof(itv); i++)
		{
			switch(m_ctxt->CLAMP.WMT)
			{
			case 0: itv[i] = itv[i] & (th-1); break;
			case 1: itv[i] = itv[i] < 0 ? 0 : itv[i] >= th ? itv[i] = th-1 : itv[i]; break;
			case 2: itv[i] = itv[i] < m_ctxt->CLAMP.MINV ? m_ctxt->CLAMP.MINV : itv[i] > m_ctxt->CLAMP.MAXV ? m_ctxt->CLAMP.MAXV : itv[i]; if(itv[i] > th-1) itv[i] = th-1; break;
			case 3: itv[i] = (itv[i] & m_ctxt->CLAMP.MINV) | m_ctxt->CLAMP.MAXV; break;
			}

			ASSERT(itv[i] >= 0 && itv[i] < th);
		}

		DWORD c[4];
		WORD Bt, Gt, Rt, At;

//		if((lod <= 0 ? m_ctxt->TEX1.MMAG : m_ctxt->TEX1.MMIN) & 1)
		{
			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[0]];
				c[1] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[1]];
				c[2] = m_pTexture[(itv[1] << m_ctxt->TEX0.TW) + itu[0]];
				c[3] = m_pTexture[(itv[1] << m_ctxt->TEX0.TW) + itu[1]];
			}
			else
			{
				c[0] = (m_lm.*ttbl.rt)(itu[0], itv[0], m_ctxt->TEX0, m_de.TEXA);
				c[1] = (m_lm.*ttbl.rt)(itu[1], itv[0], m_ctxt->TEX0, m_de.TEXA);
				c[2] = (m_lm.*ttbl.rt)(itu[0], itv[1], m_ctxt->TEX0, m_de.TEXA);
				c[3] = (m_lm.*ttbl.rt)(itu[1], itv[1], m_ctxt->TEX0, m_de.TEXA);
			}

			DWORD iuiv = iftu*iftv >> 15;
			DWORD uiv = ftu*iftv >> 15;
			DWORD iuv = iftu*ftv >> 15;
			DWORD uv = ftu*ftv >> 15;

			Rt = (WORD)((iuiv*((c[0]>> 0)&0xff) + uiv*((c[1]>> 0)&0xff) + iuv*((c[2]>> 0)&0xff) + uv*((c[3]>> 0)&0xff)) >> 15);
			Gt = (WORD)((iuiv*((c[0]>> 8)&0xff) + uiv*((c[1]>> 8)&0xff) + iuv*((c[2]>> 8)&0xff) + uv*((c[3]>> 8)&0xff)) >> 15);
			Bt = (WORD)((iuiv*((c[0]>>16)&0xff) + uiv*((c[1]>>16)&0xff) + iuv*((c[2]>>16)&0xff) + uv*((c[3]>>16)&0xff)) >> 15);
			At = (WORD)((iuiv*((c[0]>>24)&0xff) + uiv*((c[1]>>24)&0xff) + iuv*((c[2]>>24)&0xff) + uv*((c[3]>>24)&0xff)) >> 15);
		}
		/*
		else
		{
			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[0]];
			}
			else
			{
				c[0] = (m_lm.*m_ctxt->rt)(itu[0], itv[0], m_ctxt->TEX0, m_de.TEXA);
			}

			Rt = (BYTE)((c[0]>>0)&0xff);
			Gt = (BYTE)((c[0]>>8)&0xff);
			Bt = (BYTE)((c[0]>>16)&0xff);
			At = (BYTE)((c[0]>>24)&0xff);
		}
		*/

		switch(m_ctxt->TEX0.TFX)
		{
		case 0:
			Rf = Rf * Rt >> 7;
			Gf = Gf * Gt >> 7;
			Bf = Bf * Bt >> 7;
			Af = m_ctxt->TEX0.TCC ? (Af * At >> 7) : Af;
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
			Af = m_ctxt->TEX0.TCC ? (Af + At) : Af;
			break;
		case 3:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = m_ctxt->TEX0.TCC ? At : Af;
			break;
		}
	}

	SaturateColor(&Cf[0]);

	if(m_de.pPRIM->FGE)
	{
		BYTE F = v.GetFog();
		Rf = (F * Rf + (255 - F) * m_de.FOGCOL.FCR) >> 8;
		Gf = (F * Gf + (255 - F) * m_de.FOGCOL.FCG) >> 8;
		Bf = (F * Bf + (255 - F) * m_de.FOGCOL.FCB) >> 8;
	}

	BOOL ZMSK = m_ctxt->ZBUF.ZMSK;
	DWORD FBMSK = m_ctxt->FRAME.FBMSK;

	if(m_ctxt->TEST.ATE)
	{
		bool fPass = true;

		switch(m_ctxt->TEST.ATST)
		{
		case 0: fPass = false; break;
		case 1: fPass = true; break;
		case 2: fPass = Af < m_ctxt->TEST.AREF; break;
		case 3: fPass = Af <= m_ctxt->TEST.AREF; break;
		case 4: fPass = Af == m_ctxt->TEST.AREF; break;
		case 5: fPass = Af >= m_ctxt->TEST.AREF; break;
		case 6: fPass = Af > m_ctxt->TEST.AREF; break;
		case 7: fPass = Af != m_ctxt->TEST.AREF; break;
		}

		if(!fPass)
		{
			switch(m_ctxt->TEST.AFAIL)
			{
			case 0: return;
			case 1: ZMSK = 1; break; // RGBA
			case 2: FBMSK = 0xffffffff; break; // Z
			case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
			}
		}
	}

	if(!ZMSK)
	{
		(m_lm.*ztbl.wpa)(addrz, vz);
	}

	if(FBMSK != ~0)
	{
		DWORD addr = (ftbl.pa)(x, y, m_ctxt->FRAME.FBP<<5, m_ctxt->FRAME.FBW);

		if(m_ctxt->TEST.DATE && m_ctxt->FRAME.PSM <= PSM_PSMCT16S && m_ctxt->FRAME.PSM != PSM_PSMCT24)
		{
			BYTE A = (m_lm.*ftbl.rpa)(addr) >> (m_ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15);
			if(A ^ m_ctxt->TEST.DATM) return; // FIXME: vf4 missing mem card screen / the blue background of the text
		}

		// FIXME: for AA1 the value of Af should be calculated from the pixel coverage...

		bool fABE = (m_de.pPRIM->ABE || (m_de.pPRIM->PRIM == 1 || m_de.pPRIM->PRIM == 2) && m_de.pPRIM->AA1) && (!m_de.PABE.PABE || (Af&0x80));

		DWORD Cd = 0;

		if(FBMSK || fABE)
		{
			Cd = (m_lm.*ftbl.rta)(addr, m_ctxt->TEX0, m_de.TEXA);
		}

		if(fABE)
		{
			BYTE R[3] = {Rf, (Cd>>0)&0xff, 0};
			BYTE G[3] = {Gf, (Cd>>8)&0xff, 0};
			BYTE B[3] = {Bf, (Cd>>16)&0xff, 0};
			BYTE A[3] = {Af, (Cd>>24)&0xff, m_ctxt->ALPHA.FIX};

			BYTE ALPHA_A = m_ctxt->ALPHA.A;
			BYTE ALPHA_B = m_ctxt->ALPHA.B;
			BYTE ALPHA_C = m_ctxt->ALPHA.C;
			BYTE ALPHA_D = m_ctxt->ALPHA.D;

			Rf = ((R[ALPHA_A] - R[ALPHA_B]) * A[ALPHA_C] >> 7) + R[ALPHA_D];
			Gf = ((G[ALPHA_A] - G[ALPHA_B]) * A[ALPHA_C] >> 7) + G[ALPHA_D];
			Bf = ((B[ALPHA_A] - B[ALPHA_B]) * A[ALPHA_C] >> 7) + B[ALPHA_D];
		}

		if(m_de.DTHE.DTHE)
		{
			WORD DMxy = (*((WORD*)&m_de.DIMX.i64 + (y&3)) >> ((x&3)<<2)) & 7;
			Rf += DMxy;
			Gf += DMxy;
			Bf += DMxy;
		}

		ASSERT(Rf >= SHRT_MIN && Rf < SHRT_MAX);
		ASSERT(Gf >= SHRT_MIN && Gf < SHRT_MAX);
		ASSERT(Bf >= SHRT_MIN && Bf < SHRT_MAX);
		ASSERT(Af >= SHRT_MIN && Af < SHRT_MAX);

		Rf = m_clamp[Rf];
		Gf = m_clamp[Gf];
		Bf = m_clamp[Bf];
		Af = m_clamp[Af]; // ?

		Af |= (m_ctxt->FBA.FBA << 7);

		Cd = (((Af << 24) | (Bf << 16) | (Gf << 8) | (Rf << 0)) & ~FBMSK) | (Cd & FBMSK);

		(m_lm.*ftbl.wfa)(addr, Cd);
	}
}
