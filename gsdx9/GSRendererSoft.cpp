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
GSRendererSoft<Vertex>::GSRendererSoft()
{
	int i = SHRT_MIN;
	BYTE j = 0;
	for(; i < 0; i++, j++) m_clip[j] = 0, m_mask[j] = j;
	for(; i < 256; i++, j++) m_clip[j] = (BYTE)i, m_mask[j] = j;
	for(; i < SHRT_MAX; i++, j++) m_clip[j] = 255, m_mask[j] = j;

	m_uv = (uv_wrap_t*)_aligned_malloc(sizeof(uv_wrap_t), 16);

	// w00t :P

	#define InitATST(iZTST, iATST) \
		m_dv[iZTST][iATST] = &GSRendererSoft<Vertex>::DrawVertex<iZTST, iATST>; \

	#define InitZTST(iZTST) \
		InitATST(iZTST, 0) \
		InitATST(iZTST, 1) \
		InitATST(iZTST, 2) \
		InitATST(iZTST, 3) \
		InitATST(iZTST, 4) \
		InitATST(iZTST, 5) \
		InitATST(iZTST, 6) \
		InitATST(iZTST, 7) \

	#define InitDV() \
		InitZTST(0) \
		InitZTST(1) \
		InitZTST(2) \
		InitZTST(3) \

	InitDV();

	#define InitTFX(iLOD, bLCM, bTCC, iTFX) \
		m_dvtfx[iLOD][bLCM][bTCC][iTFX] = &GSRendererSoft<Vertex>::DrawVertexTFX<iLOD, bLCM, bTCC, iTFX>; \

	#define InitTCC(iLOD, bLCM, bTCC) \
		InitTFX(iLOD, bLCM, bTCC, 0) \
		InitTFX(iLOD, bLCM, bTCC, 1) \
		InitTFX(iLOD, bLCM, bTCC, 2) \
		InitTFX(iLOD, bLCM, bTCC, 3) \

	#define InitLCM(iLOD, bLCM) \
		InitTCC(iLOD, bLCM, false) \
		InitTCC(iLOD, bLCM, true) \

	#define InitLOD(iLOD) \
		InitLCM(iLOD, false) \
		InitLCM(iLOD, true) \

	#define InitDVTFX() \
		InitLOD(0) \
		InitLOD(1) \
		InitLOD(2) \
		InitLOD(3) \

	InitDVTFX();
}

template <class Vertex>
GSRendererSoft<Vertex>::~GSRendererSoft()
{
	_aligned_free(m_uv);
}

template <class Vertex>
HRESULT GSRendererSoft<Vertex>::ResetDevice(bool fForceWindowed)
{
	m_pRT[0] = NULL;
	m_pRT[1] = NULL;

	return __super::ResetDevice(fForceWindowed);
}

template <class Vertex>
void GSRendererSoft<Vertex>::ResetState()
{
	m_pTexture = NULL;

	__super::ResetState();
}

template <class Vertex>
int GSRendererSoft<Vertex>::DrawingKick(bool skip)
{
	Vertex* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	switch(m_prim)
	{
	case 0: // point
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 1: // line
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 2: // line strip
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		break;
	case 3: // triangle list
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		break;
	case 4: // triangle strip
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		break;
	case 5: // triangle fan
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		break;
	case 6: // sprite
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
		/*
		m_primtype = PRIM_TRIANGLE;
		nVertices += 2;
		pVertices[5] = pVertices[3];
		pVertices[3] = pVertices[1];
		pVertices[4] = pVertices[2];
		*/
		break;
	default:
		ASSERT(0);
		return 0;
	}

	if(skip || !m_regs.IsEnabled(0) && !m_regs.IsEnabled(1))
	{
		return 0;
	}

	if(!m_pPRIM->IIP)
	{
		Vertex::Vector c = pVertices[nVertices-1].c;

		for(int i = 0; i < nVertices-1; i++) 
		{
			pVertices[i].c = c;
		}
	}

	return nVertices;
}

static int bZTE; // , iZTST, iATST, iLOD, bLCM, bTCC, iTFX;

template <class Vertex>
void GSRendererSoft<Vertex>::FlushPrim()
{
	if(m_nVertices > 0)
	{
		bZTE = m_context->TEST.ZTE && m_context->TEST.ZTST >= 2 || !m_context->ZBUF.ZMSK;

		int iZTST = !m_context->TEST.ZTE ? 1 : m_context->TEST.ZTST;
		int iATST = !m_context->TEST.ATE ? 1 : m_context->TEST.ATST;

		m_pDrawVertex = m_dv[iZTST][iATST];

		if(m_pPRIM->TME)
		{
			int iLOD = (m_context->TEX1.MMAG & 1) + (m_context->TEX1.MMIN & 1);
			int bLCM = m_context->TEX1.LCM ? 1 : 0;
			int bTCC = m_context->TEX0.TCC ? 1 : 0;
			int iTFX = m_context->TEX0.TFX;

			if(m_pPRIM->FST)
			{
				iLOD = 3;
				bLCM = m_context->TEX1.K <= 0 && (m_context->TEX1.MMAG & 1) || m_context->TEX1.K > 0 && (m_context->TEX1.MMIN & 1);
			}

			if(m_nTextureFilter != D3DTEXF_LINEAR)
			{
				if(iLOD == 3) bLCM = 0;
				else iLOD = 0;
			}

			m_pDrawVertexTFX = m_dvtfx[iLOD][bLCM][bTCC][iTFX];

			SetupTexture();
		}
		
		m_scissor.SetRect(
			max(m_context->SCISSOR.SCAX0, 0),
			max(m_context->SCISSOR.SCAY0, 0),
			min(m_context->SCISSOR.SCAX1+1, m_context->FRAME.FBW * 64),
			min(m_context->SCISSOR.SCAY1+1, 4096));

		m_clamp = (m_env.COLCLAMP.CLAMP ? m_clip : m_mask) + 32768;

		int nPrims = 0;
		Vertex* pVertices = m_pVertices;

		switch(m_prim)
		{
		case 0:
			nPrims = m_nVertices;
			for(int i = 0; i < nPrims; i++, pVertices++) DrawPoint(pVertices);
			break;
		case 1: case 2: 
			ASSERT(!(m_nVertices&1));
			nPrims = m_nVertices / 2;
			for(int i = 0; i < nPrims; i++, pVertices += 2) DrawLine(pVertices);
			break;
		case 3: case 4: case 5:
			ASSERT(!(m_nVertices%3));
			nPrims = m_nVertices / 3;
			for(int i = 0; i < nPrims; i++, pVertices += 3) DrawTriangle(pVertices);
			break;
		case 6:
			ASSERT(!(m_nVertices&3));
			nPrims = m_nVertices / 4;
			for(int i = 0; i < nPrims; i++, pVertices += 4) DrawSprite(pVertices);
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
	}

	__super::FlushPrim();
}

template <class Vertex>
void GSRendererSoft<Vertex>::Flip()
{
	HRESULT hr;

	FlipInfo src[2];

	for(int i = 0; i < countof(src); i++)
	{
		if(!m_regs.IsEnabled(i))
		{
			continue;
		}

		CRect r = CRect(CPoint(0, 0), m_regs.GetFrameSize(i));

		//GSLocalMemory::RoundUp(, GSLocalMemory::GetBlockSize(m_regs.DISPFB[i].PSM));

		memset(&src[i].desc, 0, sizeof(src[i].desc));

		if(m_pRT[i])
		{
			m_pRT[i]->GetLevelDesc(0, &src[i].desc);
		}

		if(src[i].desc.Width != (UINT)r.right || src[i].desc.Height != (UINT)r.bottom)
		{
			m_pRT[i] = NULL;
		}

		D3DLOCKED_RECT lr;

		if(!m_pRT[i])
		{
			CComPtr<IDirect3DTexture9> pRT;

			int nTries = 0, nMaxTries = 10;

			do
			{
				pRT = NULL;

				hr = m_pD3DDev->CreateTexture(r.right, r.bottom, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pRT, NULL);

				if(FAILED(hr)) break;

				if(SUCCEEDED(pRT->LockRect(0, &lr, NULL, 0)))
				{
					pRT->UnlockRect(0);
				}

				m_pRT[i] = pRT;
			}
			while((((DWORD_PTR)lr.pBits & 0xf) || (lr.Pitch & 0xf)) && ++nTries < nMaxTries);

			if(nTries == nMaxTries) continue;

			memset(&src[i].desc, 0, sizeof(src[i].desc));

			hr = m_pRT[i]->GetLevelDesc(0, &src[i].desc);
		}

		src[i].tex = m_pRT[i];

		src[i].scale = scale_t(1, 1);

		if(FAILED(hr = src[i].tex->LockRect(0, &lr, NULL, 0)))
		{
			continue;
		}

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = m_regs.pDISPFB[i]->FBP<<5;
		TEX0.TBW = m_regs.pDISPFB[i]->FBW;
		TEX0.PSM = m_regs.pDISPFB[i]->PSM;

		GIFRegCLAMP CLAMP;

		CLAMP.WMS = CLAMP.WMT = 1;

		m_mem.ReadTexture(r, (BYTE*)lr.pBits, lr.Pitch, TEX0, m_env.TEXA, CLAMP);

		src[i].tex->UnlockRect(0);
	}

	FinishFlip(src);
}

template <class Vertex>
void GSRendererSoft<Vertex>::RowInit(int x, int y)
{
	m_faddr_x0 = (m_context->ftbl->pa)(0, y, m_context->FRAME.FBP<<5, m_context->FRAME.FBW);
	m_faddr_ro = &m_context->ftbl->rowOffset[y&7][x];

	if(bZTE)
	{
		m_zaddr_x0 = (m_context->ztbl->pa)(0, y, m_context->ZBUF.ZBP<<5, m_context->FRAME.FBW);
		m_zaddr_ro = &m_context->ztbl->rowOffset[y&7][x];
	}

	m_fx = x-1; // -1 because RowStep() will do +1, yea lame...
	m_fy = y;

	RowStep();
}

template <class Vertex>
void GSRendererSoft<Vertex>::RowStep()
{
	m_fx++;

	m_faddr = m_faddr_x0 + *m_faddr_ro++;

	if(bZTE)
	{
		m_zaddr = m_zaddr_x0 + *m_zaddr_ro++;
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawPoint(Vertex* v)
{
	CPoint p = *v;

	if(m_scissor.PtInRect(p))
	{
		RowInit(p.x, p.y);

		(this->*m_pDrawVertex)(*v);
	}
}

template <class Vertex>
void GSRendererSoft<Vertex>::DrawLine(Vertex* v)
{
	Vertex dv = v[1] - v[0];

	Vertex::Vector dp = dv.p;

	dp.x.abs();
	dp.y.abs();

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

		if(m_scissor.PtInRect(p))
		{
			RowInit(p.x, p.y);

			(this->*m_pDrawVertex)(edge);
		}

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
		int top = edge[0].p.y.ceil_i();
		int bottom = v[1].p.y.ceil_i();

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
			int left = edge[0].p.x.ceil_i();
			int right = edge[1].p.x.ceil_i();

			if(left < m_scissor.left) left = m_scissor.left;
			if(right > m_scissor.right) right = m_scissor.right;

			if(right > left)
			{
				scan = edge[0];

				if(edge[0].p.x < Vertex::Scalar(left))
				{
					scan += dscan * (Vertex::Scalar(left) - edge[0].p.x);
					scan.p.x = Vertex::Scalar(left);
				}

				RowInit(left, top);

				for(int steps = right - left; steps > 0; steps--)
				{
					(this->*m_pDrawVertex)(scan);
					scan += dscan;
					RowStep();
				}
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

	int top = v[0].p.y.ceil_i();
	int bottom = v[2].p.y.ceil_i();

	if(top < m_scissor.top) top = min(m_scissor.top, bottom);
	if(bottom > m_scissor.bottom) bottom = m_scissor.bottom;

	if(v[0].p.y < Vertex::Scalar(top)) edge += dedge * (Vertex::Scalar(top) - v[0].p.y);

	int left = v[0].p.x.ceil_i();
	int right = v[1].p.x.ceil_i();

	if(left < m_scissor.left) left = m_scissor.left;
	if(right > m_scissor.right) right = m_scissor.right;

	if(left >= right || top >= bottom) return;

	if(v[0].p.x < Vertex::Scalar(left)) edge += dscan * (Vertex::Scalar(left) - v[0].p.x);

	if(DrawFilledRect(left, top, right, bottom, edge))
		return;

	for(; top < bottom; top++)
	{
		scan = edge;

		RowInit(left, top);

		for(int steps = right - left; steps > 0; steps--)
		{
			(this->*m_pDrawVertex)(scan);
			scan += dscan;
			RowStep();
		}

		edge += dedge;
	}
}

template <class Vertex>
bool GSRendererSoft<Vertex>::DrawFilledRect(int left, int top, int right, int bottom, const Vertex& v)
{
	if(left >= right || top >= bottom)
		return false;

	ASSERT(top >= 0);
	ASSERT(bottom >= 0);

	if(m_pPRIM->IIP
	|| m_context->TEST.ZTE && m_context->TEST.ZTST != 1
	|| m_context->TEST.ATE && m_context->TEST.ATST != 1
	|| m_context->TEST.DATE
	|| m_pPRIM->TME
	|| m_pPRIM->ABE
	|| m_pPRIM->FGE
	|| m_env.DTHE.DTHE
	|| m_context->FRAME.FBMSK)
		return false;

	DWORD FBP = m_context->FRAME.FBP<<5, FBW = m_context->FRAME.FBW;
	DWORD ZBP = m_context->ZBUF.ZBP<<5;

	if(!m_context->ZBUF.ZMSK)
	{
		m_mem.FillRect(CRect(left, top, right, bottom), v.GetZ(), m_context->ZBUF.PSM, ZBP, FBW);
	}

	__declspec(align(16)) union {struct {short Rf, Gf, Bf, Af;}; UINT64 Cui64;};
	Cui64 = v.c;

	Rf = m_clamp[Rf];
	Gf = m_clamp[Gf];
	Bf = m_clamp[Bf];
	Af |= m_context->FBA.FBA << 7;

	DWORD Cdw;
	
	if(m_context->FRAME.PSM == PSM_PSMCT16 || m_context->FRAME.PSM == PSM_PSMCT16S)
	{
		Cdw = ((DWORD)(Rf&0xf8) >> 3)
			| ((DWORD)(Gf&0xf8) << 2) 
			| ((DWORD)(Bf&0xf8) << 7) 
			| ((DWORD)(Af&0x80) << 8);
	}
	else
	{
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		__m128i r0 = _mm_load_si128((__m128i*)&Cui64);
		Cdw = (DWORD)_mm_cvtsi128_si32(_mm_packus_epi16(r0, r0));
#else
		Cdw = ((DWORD)(Rf&0xff) << 0)
			| ((DWORD)(Gf&0xff) << 8) 
			| ((DWORD)(Bf&0xff) << 16) 
			| ((DWORD)(Af&0xff) << 24);
#endif
	}

	m_mem.FillRect(CRect(left, top, right, bottom), Cdw, m_context->FRAME.PSM, FBP, FBW);

	return true;
}

template <class Vertex>
template <int iZTST, int iATST> 
void GSRendererSoft<Vertex>::DrawVertex(const Vertex& v)
{
	DWORD vz;

	switch(iZTST)
	{
	case 0: return;
	case 1: break;
	case 2: 
		vz = v.GetZ(); 
		if(vz < m_mem.readPixelX(m_context->ZBUF.PSM, m_zaddr)) return; 
		// if(vz < (m_mem.*m_context->ztbl->rpa)(m_zaddr)) return; 
		break;
	case 3: 
		vz = v.GetZ(); 
		if(vz <= m_mem.readPixelX(m_context->ZBUF.PSM, m_zaddr)) return; 
		// if(vz <= (m_mem.*m_context->ztbl->rpa)(m_zaddr)) return; 
		break;
	default:
		__assume(0);
	}

	union
	{
		struct {Vertex::Vector Cf, Cd, Ca;};
		struct {Vertex::Vector Cfda[3];};
	};

	Cf = v.c;

	if(m_pPRIM->TME)
	{
		(this->*m_pDrawVertexTFX)(Cf, v);
	}

	if(m_pPRIM->FGE)
	{
		Vertex::Scalar a = Cf.a;
		Vertex::Vector Cfog((DWORD)m_env.FOGCOL.ai32[0]);
		Cf = Cfog + (Cf - Cfog) * v.t.z;
		Cf.a = a;
	}

	BOOL ZMSK = m_context->ZBUF.ZMSK;
	DWORD FBMSK = m_context->FRAME.FBMSK;

	bool fAlphaPass = true;

	BYTE Af = (BYTE)(int)Cf.a;

	switch(iATST)
	{
	case 0: fAlphaPass = false; break;
	case 1: fAlphaPass = true; break;
	case 2: fAlphaPass = Af < m_context->TEST.AREF; break;
	case 3: fAlphaPass = Af <= m_context->TEST.AREF; break;
	case 4: fAlphaPass = Af == m_context->TEST.AREF; break;
	case 5: fAlphaPass = Af >= m_context->TEST.AREF; break;
	case 6: fAlphaPass = Af > m_context->TEST.AREF; break;
	case 7: fAlphaPass = Af != m_context->TEST.AREF; break;
	default: __assume(0);
	}

	if(!fAlphaPass)
	{
		switch(m_context->TEST.AFAIL)
		{
		case 0: return;
		case 1: ZMSK = 1; break; // RGBA
		case 2: FBMSK = 0xffffffff; break; // Z
		case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
		default: __assume(0);
		}
	}

	if(!ZMSK)
	{
		if(iZTST != 2 && iZTST != 3) vz = v.GetZ(); 
		m_mem.writePixelX(m_context->ZBUF.PSM, m_zaddr, vz);
		// (m_mem.*m_context->ztbl->wpa)(m_zaddr, vz);
	}

	if(FBMSK != ~0)
	{
		if(m_context->TEST.DATE && m_context->FRAME.PSM <= PSM_PSMCT16S && m_context->FRAME.PSM != PSM_PSMCT24)
		{
			DWORD c = m_mem.readPixelX(m_context->FRAME.PSM, m_faddr);
			// DWORD c = (m_mem.*m_context->ftbl->rpa)(m_faddr);
			BYTE A = (BYTE)(c >> (m_context->FRAME.PSM == PSM_PSMCT32 ? 31 : 15));
			if(A ^ m_context->TEST.DATM) return;
		}

		// FIXME: for AA1 the value of Af should be calculated from the pixel coverage...

		bool fABE = (m_pPRIM->ABE || m_pPRIM->AA1 && (m_pPRIM->PRIM == 1 || m_pPRIM->PRIM == 2)) && (!m_env.PABE.PABE || (int)Cf.a >= 0x80);

		if(FBMSK || fABE)
		{
			GIFRegTEXA TEXA;
			/*
			TEXA.AEM = 0;
			TEXA.TA0 = 0;
			TEXA.TA1 = 0x80;
			*/
			TEXA.ai32[0] = 0;
			TEXA.ai32[1] = 0x80;

			Cd = m_mem.readTexelX(m_context->FRAME.PSM, m_faddr, m_context->TEX0, TEXA);
			// Cd = (m_mem.*m_context->ftbl->rta)(m_faddr, m_context->TEX0, TEXA);
		}

		if(fABE)
		{
			Ca = Vertex::Vector(Vertex::Scalar(0));
			Ca.a = Vertex::Scalar((int)m_context->ALPHA.FIX);

			Vertex::Scalar a = Cf.a;
			Cf = ((Cfda[m_context->ALPHA.A] - Cfda[m_context->ALPHA.B]) * Cfda[m_context->ALPHA.C].a >> 7) + Cfda[m_context->ALPHA.D];
			Cf.a = a;
		}

		DWORD Cdw; 

		if(m_env.COLCLAMP.CLAMP && !m_env.DTHE.DTHE)
		{
			Cdw = Cf;
		}
		else
		{
			__declspec(align(16)) union {struct {short Rf, Gf, Bf, Af;}; UINT64 Cui64;};
			Cui64 = Cf;

			if(m_env.DTHE.DTHE)
			{
				short DMxy = (signed char)((*((WORD*)&m_env.DIMX.i64 + (m_fy&3)) >> ((m_fx&3)<<2)) << 5) >> 5;
				Rf = (short)(Rf + DMxy);
				Gf = (short)(Gf + DMxy);
				Bf = (short)(Bf + DMxy);
			}

			Rf = m_clamp[Rf];
			Gf = m_clamp[Gf];
			Bf = m_clamp[Bf];
			Af |= m_context->FBA.FBA << 7;

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
			__m128i r0 = _mm_load_si128((__m128i*)&Cui64);
			Cdw = (DWORD)_mm_cvtsi128_si32(_mm_packus_epi16(r0, r0));
#else
			Cdw = ((DWORD)(Rf&0xff) << 0)
				| ((DWORD)(Gf&0xff) << 8) 
				| ((DWORD)(Bf&0xff) << 16) 
				| ((DWORD)(Af&0xff) << 24);
#endif
		}

		if(FBMSK != 0)
		{
			Cdw = (Cdw & ~FBMSK) | ((DWORD)Cd & FBMSK);
		}

		m_mem.writeFrameX(m_context->FRAME.PSM, m_faddr, Cdw);
		// (m_mem.*m_context->ftbl->wfa)(m_faddr, Cdw);
	}
}

template <class Vertex>
template <int iLOD, bool bLCM, bool bTCC, int iTFX>
void GSRendererSoft<Vertex>::DrawVertexTFX(typename Vertex::Vector& Cf, const Vertex& v)
{
	ASSERT(m_pPRIM->TME);
	
	Vertex::Vector t = v.t;

	bool fBiLinear = iLOD == 2; 

	if(iLOD == 3)
	{
		fBiLinear = !!bLCM;
	}
	else
	{
		t.q.rcp();
		t *= t.q;

		if(iLOD == 1)
		{
			static const float one_over_log2 = 1.0f / log(2.0f);

			float lod = (float)(int)m_context->TEX1.K;
			if(!bLCM) lod += log(fabs((float)t.q)) * one_over_log2 * (1 << m_context->TEX1.L);
			fBiLinear = lod <= 0 && (m_context->TEX1.MMAG & 1) || lod > 0 && (m_context->TEX1.MMIN & 1);
		}
	}

	if(fBiLinear) t -= Vertex::Scalar(0.5f);

	__declspec(align(16)) short ituv[8] = 
	{
		(short)(int)t.x, 
		(short)(int)t.x+1, 
		(short)(int)t.y, 
		(short)(int)t.y+1
	};

#if _M_IX86_FP >= 2 || defined(_M_AMD64)

	__m128i uv = _mm_load_si128((__m128i*)ituv);
	__m128i mask = _mm_load_si128((__m128i*)m_uv->mask);
	__m128i region = _mm_or_si128(_mm_and_si128(uv, *(__m128i*)m_uv->and), *(__m128i*)m_uv->or);
	__m128i clamp = _mm_min_epi16(_mm_max_epi16(uv, *(__m128i*)m_uv->min), *(__m128i*)m_uv->max);
	_mm_store_si128((__m128i*)ituv, _mm_or_si128(_mm_and_si128(region, mask), _mm_andnot_si128(mask, clamp)));

#else

	for(int i = 0; i < 4; i++)
	{
		short region = (ituv[i] & m_uv->and[i]) | m_uv->or[i];
		short clamp = ituv[i] < m_uv->min[i] ? m_uv->min[i] : ituv[i] > m_uv->max[i] ? m_uv->max[i] : ituv[i];
		ituv[i] = (region & m_uv->mask[i]) | (clamp & ~m_uv->mask[i]);
	}

#endif

	Vertex::Vector Ct[4];

	if(fBiLinear)
	{
		if(0 && m_pTexture)
		{
			Ct[0] = m_pTexture[(ituv[2] << m_context->TEX0.TW) + ituv[0]];
			Ct[1] = m_pTexture[(ituv[2] << m_context->TEX0.TW) + ituv[1]];
			Ct[2] = m_pTexture[(ituv[3] << m_context->TEX0.TW) + ituv[0]];
			Ct[3] = m_pTexture[(ituv[3] << m_context->TEX0.TW) + ituv[1]];
		}
		else
		{
			for(int i = 0; i < 4; i++)
			{
				Ct[i] = m_mem.readTexelX(m_context->TEX0.PSM, ituv[i&1], ituv[2+(i>>1)], m_context->TEX0, m_env.TEXA);
				// Ct[i] = (m_mem.*m_context->ttbl->rt)(ituv[i&1], ituv[2+(i>>1)], m_context->TEX0, m_env.TEXA);
			}
		}

		Vertex::Vector ft = t - t.floor();

		Ct[0] = Ct[0] + (Ct[1] - Ct[0]) * ft.x;
		Ct[2] = Ct[2] + (Ct[3] - Ct[2]) * ft.x;
		Ct[0] = Ct[0] + (Ct[2] - Ct[0]) * ft.y;
	}
	else 
	{
		if(0 && m_pTexture)
		{
			Ct[0] = m_pTexture[(ituv[2] << m_context->TEX0.TW) + ituv[0]];
		}
		else
		{
			Ct[0] = m_mem.readTexelX(m_context->TEX0.PSM, ituv[0], ituv[2], m_context->TEX0, m_env.TEXA);
			// Ct[0] = (m_mem.*m_context->ttbl->rt)(ituv[0], ituv[2], m_context->TEX0, m_env.TEXA);
		}
	}

	Vertex::Scalar a = Cf.a;

	switch(iTFX)
	{
	case 0:
		Cf = (Cf * Ct[0] >> 7);
		if(!bTCC) Cf.a = a;
		break;
	case 1:
		Cf = Ct[0];
		break;
	case 2:
		Cf = (Cf * Ct[0] >> 7) + Cf.a;
		Cf.a = !bTCC ? a : (Ct[0].a + a);
		break;
	case 3:
		Cf = (Cf * Ct[0] >> 7) + Cf.a;
		Cf.a = !bTCC ? a : Ct[0].a;
		break;
	default: 
		__assume(0);
	}

	Cf.sat();
}

template <class Vertex>
void GSRendererSoft<Vertex>::SetupTexture()
{
	m_mem.SetupCLUT32(m_context->TEX0, m_env.TEXA);

	//

	int tw = 1 << m_context->TEX0.TW;
	int th = 1 << m_context->TEX0.TH;

	switch(m_context->CLAMP.WMS)
	{
	case 0: m_uv->and[0] = (short)(tw-1); m_uv->or[0] = 0; m_uv->mask[0] = 0xffff; break;
	case 1: m_uv->min[0] = 0; m_uv->max[0] = (short)(tw-1); m_uv->mask[0] = 0; break;
	case 2: m_uv->min[0] = (short)m_context->CLAMP.MINU; m_uv->max[0] = (short)m_context->CLAMP.MAXU; m_uv->mask[0] = 0; break;
	case 3: m_uv->and[0] = (short)m_context->CLAMP.MINU; m_uv->or[0] = (short)m_context->CLAMP.MAXU; m_uv->mask[0] = 0xffff; break;
	default: __assume(0);
	}

	m_uv->and[1] = m_uv->and[0];
	m_uv->or[1] = m_uv->or[0];
	m_uv->min[1] = m_uv->min[0];
	m_uv->max[1] = m_uv->max[0];
	m_uv->mask[1] = m_uv->mask[0];

	switch(m_context->CLAMP.WMT)
	{
	case 0: m_uv->and[2] = (short)(th-1); m_uv->or[2] = 0; m_uv->mask[2] = 0xffff; break;
	case 1: m_uv->min[2] = 0; m_uv->max[2] = (short)(th-1); m_uv->mask[2] = 0; break;
	case 2: m_uv->min[2] = (short)m_context->CLAMP.MINV; m_uv->max[2] = (short)m_context->CLAMP.MAXV; m_uv->mask[2] = 0; break;
	case 3: m_uv->and[2] = (short)m_context->CLAMP.MINV; m_uv->or[2] = (short)m_context->CLAMP.MAXV; m_uv->mask[2] = 0xffff; break;
	default: __assume(0);
	}

	m_uv->and[3] = m_uv->and[2];
	m_uv->or[3] = m_uv->or[2];
	m_uv->min[3] = m_uv->min[2];
	m_uv->max[3] = m_uv->max[2];
	m_uv->mask[3] = m_uv->mask[2];
}

//
// GSRendererSoftFP
//

GSRendererSoftFP::GSRendererSoftFP()
	: GSRendererSoft<GSSoftVertexFP>()
{
}

void GSRendererSoftFP::VertexKick(bool skip)
{
	GSSoftVertexFP& v = m_vl.AddTail();

	v.c = (DWORD)m_v.RGBAQ.ai32[0];

	v.p.x = (int)m_v.XYZ.X - (int)m_context->XYOFFSET.OFX;
	v.p.y = (int)m_v.XYZ.Y - (int)m_context->XYOFFSET.OFY;
	v.p *= GSSoftVertexFP::Scalar(1.0f/16);
	v.p.z = (float)m_v.XYZ.Z;
	//v.p.z = (float)(m_v.XYZ.Z >> 16);
	//v.p.q = (float)(m_v.XYZ.Z & 0xffff);

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.t.x = (float)(int)m_v.UV.U;
			v.t.y = (float)(int)m_v.UV.V;
			v.t *= GSSoftVertexFP::Scalar(1.0f/16);
			v.t.q = 1.0f;
		}
		else
		{
			v.t.x = m_v.ST.S * (1 << m_context->TEX0.TW);
			v.t.y = m_v.ST.T * (1 << m_context->TEX0.TH);
			v.t.q = m_v.RGBAQ.Q;
		}
	}

	if(m_pPRIM->FGE)
	{
		v.t.z = (float)m_v.FOG.F * (1.0f/255);
	}

	__super::VertexKick(skip);
}
/*
//
// GSRendererSoftFX
//

GSRendererSoftFX::GSRendererSoftFX(HWND hWnd, HRESULT& hr)
	: GSRendererSoft<GSSoftVertexFX>(hWnd, hr)
{
}

void GSRendererSoftFX::VertexKick(bool skip)
{
	GSSoftVertexFX& v = m_vl.AddTail();

	v.c = (DWORD)m_v.RGBAQ.ai32[0];

	v.p.x.SetValue(((int)m_v.XYZ.X - (int)m_context->XYOFFSET.OFX) << 12);
	v.p.y.SetValue(((int)m_v.XYZ.Y - (int)m_context->XYOFFSET.OFY) << 12);
	// v.p.zq = (UINT64)m_v.XYZ.Z << 32;
	v.p.z = (int)m_v.XYZ.Z;

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.t.x.SetValue(((int)m_v.UV.U << (12 >> m_context->TEX0.TW)));
			v.t.y.SetValue(((int)m_v.UV.V << (12 >> m_context->TEX0.TH)));
			v.t.q = 1;
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
		v.t.z.SetValue((int)m_v.FOG.F << 8);
	}

	__super::VertexKick(skip);
}
*/