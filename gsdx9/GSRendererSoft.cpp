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
#include "GSRendererSoft.h"
#include "x86.h"

template <class Vertex>
GSRendererSoft<Vertex>::GSRendererSoft(HWND hWnd, HRESULT& hr)
	: GSRenderer<Vertex>(640, 512, hWnd, hr)
{
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
		pVertices[0].p.z = pVertices[1].p.z;
		pVertices[0].p.q = pVertices[1].p.q;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].p.y = pVertices[0].p.y;
		pVertices[1].t.y = pVertices[0].t.y;
		pVertices[2].p.x = pVertices[0].p.x;
		pVertices[2].t.x = pVertices[0].t.x;
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

	if(!m_pPRIM->IIP)
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
	Vertex dv = v[1] - v[0];

	Vertex::Vector dp = dv.p;
	dp.x.abs_s();
	dp.y.abs_s();

	int dx = (int)dp.x;
	int dy = (int)dp.y;

	if(dx == 0 && dy == 0) return;

	int i = dx > dy ? 0 : 1;

	Vertex edge = v[0];
	Vertex dedge = dv / dp.v[i];

	// TODO: clip with the scissor

	int steps = (int)dp.v[i];

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
	if(v[1].p.y < v[0].p.y) {Vertex::Exchange(&v[0], &v[1]);}
	if(v[2].p.y < v[0].p.y) {Vertex::Exchange(&v[0], &v[2]);}
	if(v[2].p.y < v[1].p.y) {Vertex::Exchange(&v[1], &v[2]);}

	if(!(v[0].p.y < v[2].p.y)) return;

	Vertex v01 = v[1] - v[0];
	Vertex v02 = v[2] - v[0];

	Vertex::Scalar temp = v01.p.y / v02.p.y;
	Vertex::Scalar longest = temp * v02.p.x - v01.p.x;

	int ledge, redge;
	if(Vertex::Scalar(0) < longest) {ledge = 0; redge = 1; if(longest < Vertex::Scalar(1)) longest = Vertex::Scalar(1);}
	else if(longest < Vertex::Scalar(0)) {ledge = 1; redge = 0; if(Vertex::Scalar(-1) < longest) longest = Vertex::Scalar(-1);}
	else return;

	Vertex edge[2] = {v[0], v[0]};

	Vertex dedge[2];
	dedge[0].p.y = dedge[1].p.y = Vertex::Scalar(1);
	if(Vertex::Scalar(0) < v01.p.y) dedge[ledge] = v01 / v01.p.y;
	if(Vertex::Scalar(0) < v02.p.y) dedge[redge] = v02 / v02.p.y;

	Vertex scan;

	Vertex dscan = (v02 * temp - v01) / longest;
	dscan.p.y = 0;

	for(int i = 0; i < 2; i++, v++)
	{ 
		int top = edge[0].p.y.ceil_i(), bottom = v[1].p.y.ceil_i();
		if(top < m_scissor.top) top = min(m_scissor.top, bottom);
		if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;
		if(edge[0].p.y < Vertex::Scalar(top)) // for(int j = 0; j < 2; j++) edge[j] += dedge[j] * ((float)top - edge[0].p.y);
		{
			Vertex::Scalar dy = Vertex::Scalar(top) - edge[0].p.y;
			edge[0] += dedge[0] * dy;
			edge[1].p.x += dedge[1].p.x * dy;
			edge[0].p.y = edge[1].p.y = Vertex::Scalar(top);
		}

		ASSERT(top >= bottom || (int)((edge[1].p.y - edge[0].p.y) * 10) == 0);

		for(; top < bottom; top++)
		{
			scan = edge[0];

			int left = edge[0].p.x.ceil_i(), right = edge[1].p.x.ceil_i();
			if(left < m_scissor.left) left = m_scissor.left;
			if(right > m_scissor.right) right = m_scissor.right;
			if(edge[0].p.x < Vertex::Scalar(left))
			{
				scan += dscan * (Vertex::Scalar(left) - edge[0].p.x);
				scan.p.x = Vertex::Scalar(left);
			}

			for(; left < right; left++)
			{
				DrawVertex(left, top, scan);
				scan += dscan;
			}

			// for(int j = 0; j < 2; j++) edge[j] += dedge[j];
			edge[0] += dedge[0];
			edge[1].p += dedge[1].p;
		}

		if(v[1].p.y < v[2].p.y)
		{
			edge[ledge] = v[1];
			dedge[ledge] = (v[2] - v[1]) / (v[2].p.y - v[1].p.y);
			edge[ledge] += dedge[ledge] * (edge[ledge].p.y.ceil_s() - edge[ledge].p.y);
		}
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawSprite(Vertex* v)
{
	if(v[2].p.y < v[0].p.y) {Vertex::Exchange(&v[0], &v[2]); Vertex::Exchange(&v[1], &v[3]);}
	if(v[1].p.x < v[0].p.x) {Vertex::Exchange(&v[0], &v[1]); Vertex::Exchange(&v[2], &v[3]);}

	if(v[0].p.x == v[1].p.x || v[0].p.y == v[2].p.y) return;

	Vertex v01 = v[1] - v[0];
	Vertex v02 = v[2] - v[0];

	Vertex edge = v[0];
	Vertex dedge = v02 / v02.p.y;
	Vertex scan;
	Vertex dscan = v01 / v01.p.x;

	int top = v[0].p.y.ceil_i(), bottom = v[2].p.y.ceil_i();
	if(top < m_scissor.top) top = min(m_scissor.top, bottom);
	if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;
	if(v[0].p.y < Vertex::Scalar(top)) edge += dedge * (Vertex::Scalar(top) - v[0].p.y);

	int left = v[0].p.x.ceil_i(), right = v[1].p.x.ceil_i();
	if(left < m_scissor.left) left = m_scissor.left;
	if(right > m_scissor.right) right = m_scissor.right;
	if(v[0].p.x < Vertex::Scalar(left)) edge += dscan * (Vertex::Scalar(left) - v[0].p.x);

	if(DrawFilledRect(left, top, right, bottom, edge))
		return;

	for(; top < bottom; top++)
	{
		scan = edge;

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

	if(m_pPRIM->IIP
	|| m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST != 1
	|| m_ctxt->TEST.ATE && m_ctxt->TEST.ATST != 1
	|| m_ctxt->TEST.DATE
	|| m_pPRIM->TME
	|| m_pPRIM->ABE
	|| m_pPRIM->FGE
	|| m_de.DTHE.DTHE
	|| m_ctxt->FRAME.FBMSK)
		return false;

	DWORD FBP = m_ctxt->FRAME.FBP<<5, FBW = m_ctxt->FRAME.FBW;
	DWORD ZBP = m_ctxt->ZBUF.ZBP<<5;

	if(!m_ctxt->ZBUF.ZMSK)
	{
		m_lm.FillRect(CRect(left, top, right, bottom), v.GetZ(), m_ctxt->ZBUF.PSM, ZBP, FBW);
	}

	union {struct {BYTE Rf, Gf, Bf, Af;}; DWORD Cdw;};
	Cdw = v.c;

	Rf = m_clamp[Rf];
	Gf = m_clamp[Gf];
	Bf = m_clamp[Bf];
	Af = m_clamp[Af]; // ?

	Af |= (m_ctxt->FBA.FBA << 7);

	if(m_ctxt->FRAME.PSM == PSM_PSMCT16 || m_ctxt->FRAME.PSM == PSM_PSMCT16S)
		Cdw = ((Cdw>>16)&0x8000)|((Cdw>>9)&0x7c00)|((Cdw>>6)&0x03e0)|((Cdw>>3)&0x001f);
	m_lm.FillRect(CRect(left, top, right, bottom), Cdw, m_ctxt->FRAME.PSM, FBP, FBW);

	return true;
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawVertex(int x, int y, const Vertex& v)
{
	ASSERT(x == (int)v.p.x && y == (int)v.p.y);

	DWORD addrz = 0;

	if(m_ctxt->ZBUF.ZMSK == 0 || m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST >= 2)
	{
		addrz = (m_ctxt->ztbl->pa)(x, y, m_ctxt->ZBUF.ZBP<<5, m_ctxt->FRAME.FBW);
	}

	DWORD vz = v.GetZ();

	if(m_ctxt->TEST.ZTE && m_ctxt->TEST.ZTST != 1)
	{
		if(m_ctxt->TEST.ZTST == 0)
			return;

		DWORD z = (m_lm.*m_ctxt->ztbl->rpa)(addrz);
		if(m_ctxt->TEST.ZTST == 2 && vz < z || m_ctxt->TEST.ZTST == 3 && vz <= z)
			return;
	}

	Vertex::Vector Cf = v.c;

	if(m_pPRIM->TME)
	{
		DrawVertexTFX(Cf, v);
	}

	if(m_pPRIM->FGE)
	{
		Vertex::Scalar a = Cf.a;
		Vertex::Vector Cfog((DWORD)m_de.FOGCOL.ai32[0]);
		Cf = Cfog + (Cf - Cfog) * v.t.z;
		Cf.a = a;
	}

	BOOL ZMSK = m_ctxt->ZBUF.ZMSK;
	DWORD FBMSK = m_ctxt->FRAME.FBMSK;

	if(m_ctxt->TEST.ATE)
	{
		bool fPass = true;

		BYTE Af = (BYTE)(int)Cf.a;

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
		default: __assume(0);
		}

		if(!fPass)
		{
			switch(m_ctxt->TEST.AFAIL)
			{
			case 0: return;
			case 1: ZMSK = 1; break; // RGBA
			case 2: FBMSK = 0xffffffff; break; // Z
			case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
			default: __assume(0);
			}
		}
	}

	if(!ZMSK)
	{
		(m_lm.*m_ctxt->ztbl->wpa)(addrz, vz);
	}

	if(FBMSK != ~0)
	{
		DWORD addr = (m_ctxt->ftbl->pa)(x, y, m_ctxt->FRAME.FBP<<5, m_ctxt->FRAME.FBW);

		if(m_ctxt->TEST.DATE && m_ctxt->FRAME.PSM <= PSM_PSMCT16S && m_ctxt->FRAME.PSM != PSM_PSMCT24)
		{
			BYTE A = (BYTE)((m_lm.*m_ctxt->ftbl->rpa)(addr) >> (m_ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15));
			if(A ^ m_ctxt->TEST.DATM) return;
		}

		// FIXME: for AA1 the value of Af should be calculated from the pixel coverage...

		bool fABE = (m_pPRIM->ABE || (m_pPRIM->PRIM == 1 || m_pPRIM->PRIM == 2) && m_pPRIM->AA1) && (!m_de.PABE.PABE || ((int)Cf.a >= 0x80));

		Vertex::Vector Cd;

		if(FBMSK || fABE)
		{
			Cd = (m_lm.*m_ctxt->ftbl->rta)(addr, m_ctxt->TEX0, m_de.TEXA);
		}

		if(fABE)
		{
			Vertex::Vector CsCdA[3] = {Cf, Cd, Vertex::Vector(Vertex::Scalar(0), Vertex::Scalar(0), Vertex::Scalar(0), Vertex::Scalar((int)m_ctxt->ALPHA.FIX))};

			Vertex::Scalar a = Cf.a;
			Cf = (CsCdA[m_ctxt->ALPHA.A] - CsCdA[m_ctxt->ALPHA.B]) * CsCdA[m_ctxt->ALPHA.C].a * Vertex::Scalar(2.0f/256) + CsCdA[m_ctxt->ALPHA.D];
			Cf.a = a;
		}

		union {struct {BYTE Rf, Gf, Bf, Af;}; DWORD Cdw;};
		Cdw = Cf;

		if(m_de.DTHE.DTHE)
		{
			WORD DMxy = (*((WORD*)&m_de.DIMX.i64 + (y&3)) >> ((x&3)<<2)) & 7;
			Rf += DMxy;
			Gf += DMxy;
			Bf += DMxy;
		}

		Rf = m_clamp[Rf];
		Gf = m_clamp[Gf];
		Bf = m_clamp[Bf];
		Af = m_clamp[Af]; // ?

		Af |= (m_ctxt->FBA.FBA << 7);

		Cdw = (Cdw & ~FBMSK) | (Cd & FBMSK);

		(m_lm.*m_ctxt->ftbl->wfa)(addr, Cdw);
	}
}

static const float one_over_log2 = 1.0f / log(2.0f);

template <class Vertex>
void GSRendererSoft<Vertex>::DrawVertexTFX(typename Vertex::Vector& Cf, const Vertex& v)
{
	ASSERT(m_pPRIM->TME);

	int tw = 1 << m_ctxt->TEX0.TW;
	int th = 1 << m_ctxt->TEX0.TH;

	Vertex::Scalar w = Vertex::Scalar(v.t.q);
	w.rcp();

	Vertex::Scalar tu = v.t.x * w * tw;
	Vertex::Scalar tv = v.t.y * w * th;

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
		default: __assume(0);
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
		default: __assume(0);
		}

		ASSERT(itv[i] >= 0 && itv[i] < th);
	}

	Vertex::Vector Ct[4];

	float lod = (float)m_ctxt->TEX1.K;
	if(!m_ctxt->TEX1.LCM) lod += log(fabs(w)) * one_over_log2 * (1 << m_ctxt->TEX1.L);

	if(lod <= 0 && (m_ctxt->TEX1.MMAG & 1) || lod > 0 && (m_ctxt->TEX1.MMIN & 1))
	{
		if(m_pTexture)
		{
			Ct[0] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[0]];
			Ct[1] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[1]];
			Ct[2] = m_pTexture[(itv[1] << m_ctxt->TEX0.TW) + itu[0]];
			Ct[3] = m_pTexture[(itv[1] << m_ctxt->TEX0.TW) + itu[1]];
		}
		else
		{
			Ct[0] = (m_lm.*m_ctxt->ttbl->rt)(itu[0], itv[0], m_ctxt->TEX0, m_de.TEXA);
			Ct[1] = (m_lm.*m_ctxt->ttbl->rt)(itu[1], itv[0], m_ctxt->TEX0, m_de.TEXA);
			Ct[2] = (m_lm.*m_ctxt->ttbl->rt)(itu[0], itv[1], m_ctxt->TEX0, m_de.TEXA);
			Ct[3] = (m_lm.*m_ctxt->ttbl->rt)(itu[1], itv[1], m_ctxt->TEX0, m_de.TEXA);
		}

		Vertex::Scalar ftu = tu - tu.floor_s();
		Vertex::Scalar ftv = tv - tv.floor_s();		

		Ct[0] = Ct[0] + (Ct[1] - Ct[0]) * ftu;
		Ct[2] = Ct[2] + (Ct[3] - Ct[2]) * ftu;
		Ct[0] = Ct[0] + (Ct[2] - Ct[0]) * ftv;
	}
	else 
	{
		if(m_pTexture)
		{
			Ct[0] = m_pTexture[(itv[0] << m_ctxt->TEX0.TW) + itu[0]];
		}
		else
		{
			Ct[0] = (m_lm.*m_ctxt->ttbl->rt)(itu[0], itv[0], m_ctxt->TEX0, m_de.TEXA);
		}
	}

	Vertex::Scalar a = Cf.a;

	// switch(m_ctxt->TEX0.TFX)
	switch((m_ctxt->TEX0.ai32[1]>>3)&3)
	{
	case 0:
		Cf = Cf * Ct[0] * Vertex::Scalar(2.0f/256);
		if(!m_ctxt->TEX0.TCC) Cf.a = a;
		break;
	case 1:
		Cf = Ct[0];
		break;
	case 2:
		Cf = Cf * Ct[0] * Vertex::Scalar(2.0f/256) + Cf.a;
		Cf.a = !m_ctxt->TEX0.TCC ? a : (Ct[0].a + a);
		break;
	case 3:
		Cf = Cf * Ct[0] * Vertex::Scalar(2.0f/256) + Cf.a;
		Cf.a = !m_ctxt->TEX0.TCC ? a : Ct[0].a;
		break;
	default: 
		__assume(0);
	}

	Cf.sat();
}

template <class Vertex>
void GSRendererSoft<Vertex>::SetTexture()
{
	if(m_pPRIM->TME)
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

	v.c = (DWORD)m_v.RGBAQ.ai32[0];

	v.p.x = (int)m_v.XYZ.X - (int)m_ctxt->XYOFFSET.OFX;
	v.p.y = (int)m_v.XYZ.Y - (int)m_ctxt->XYOFFSET.OFY;
	v.p *= GSSoftVertexFP::Scalar(1.0f/16);
	v.p.z = (float)(m_v.XYZ.Z >> 16);
	v.p.q = (float)(m_v.XYZ.Z & 0xffff);

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.t.x = (float)m_v.UV.U / (16 << m_ctxt->TEX0.TW);
			v.t.y = (float)m_v.UV.V / (16 << m_ctxt->TEX0.TH);
			v.t.q = 1.0f;
		}
		else
		{
			v.t.x = m_v.ST.S;
			v.t.y = m_v.ST.T;
			v.t.q = m_v.RGBAQ.Q;
		}
	}

	if(m_pPRIM->FGE)
	{
		v.t.z = (float)m_v.FOG.F * (1.0f/255);
	}

	__super::VertexKick(fSkip);
}
/*
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

	v.c = (DWORD)m_v.RGBAQ.ai32[0];

	v.p.x = ((int)m_v.XYZ.X - (int)m_ctxt->XYOFFSET.OFX) << 12;
	v.p.y = ((int)m_v.XYZ.Y - (int)m_ctxt->XYOFFSET.OFY) << 12;
	v.p.z = (int)((m_v.XYZ.Z & 0xffff0000) >> 1);
	v.p.q = (int)((m_v.XYZ.Z & 0x0000ffff) << 15);

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.t.x = ((int)m_v.UV.U << (12 >> m_ctxt->TEX0.TW));
			v.t.y = ((int)m_v.UV.V << (12 >> m_ctxt->TEX0.TH));
			v.t.q = 1<<16;
		}
		else
		{
			// TODO
			v.t.x = m_v.ST.S;
			v.t.y = m_v.ST.T;
			v.t.q = m_v.RGBAQ.Q;
		}
	}

	if(m_pPRIM->FGE)
	{
		v.t.z = (int)m_v.FOG.F << 8;
	}

	__super::VertexKick(fSkip);
}
*/