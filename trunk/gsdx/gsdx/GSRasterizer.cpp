/* 
 *	Copyright (C) 2007 Gabest
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

// TODO: avx (256 bit regs, 8 pixels, 3-4 op instructions), DrawScanline ~50-70% of total time
// TODO: sse is a waste for 1 pixel (not that bad, sse register utilization is 90-95%)
// TODO: sprite doesn't need z/f interpolation
// TODO: eliminate small triangles faster, usually 50% of the triangles do not output any pixel because they are so tiny
// current fillrate is about 20-50M tp/s (depends on the effectiveness of the texture cache), ps2 can do 1.2G, that means we can already hit 1 fps in the worst case :P 
// (in SoTC it can do 125M tp/s and still 4 fps only, insane fillrate needed there)
// TODO: DrawScanline => CUDA impl., input: array of [scan, dscan, index], kernel function: draw pixel at [scan + dscan * index]

#include "StdAfx.h"
#include "GSRasterizer.h"

GSRasterizer::GSRasterizer(GSState* state, int id, int threads)
	: m_state(state)
	, m_id(id)
	, m_threads(threads)
	, m_fbco(NULL)
	, m_zbco(NULL)
{
	// w00t :P

	#define InitDS_IIP(iFPSM, iZPSM, iZTST, iIIP) \
		m_ds[iFPSM][iZPSM][iZTST][iIIP] = &GSRasterizer::DrawScanline<iFPSM, iZPSM, iZTST, iIIP>; \

	#define InitDS_ZTST(iFPSM, iZPSM, iZTST) \
		InitDS_IIP(iFPSM, iZPSM, iZTST, 0) \
		InitDS_IIP(iFPSM, iZPSM, iZTST, 1) \

	#define InitDS_ZPSM(iFPSM, iZPSM) \
		InitDS_ZTST(iFPSM, iZPSM, 0) \
		InitDS_ZTST(iFPSM, iZPSM, 1) \
		InitDS_ZTST(iFPSM, iZPSM, 2) \
		InitDS_ZTST(iFPSM, iZPSM, 3) \

	#define InitDS_FPSM(iFPSM) \
		InitDS_ZPSM(iFPSM, 0) \
		InitDS_ZPSM(iFPSM, 1) \
		InitDS_ZPSM(iFPSM, 2) \

	#define InitDS() \
		InitDS_FPSM(0) \
		InitDS_FPSM(1) \
		InitDS_FPSM(2) \

	InitDS();

	InitEx();
}

GSRasterizer::~GSRasterizer()
{
	POSITION pos = m_comap.GetHeadPosition();

	while(pos)
	{
		ColumnOffset* co = m_comap.GetNextValue(pos);

		for(int i = 0; i < countof(co->col); i++)
		{
			_aligned_free(co->col);
		}

		_aligned_free(co);
	}

	m_comap.RemoveAll();
}

int GSRasterizer::Draw(Vertex* vertices, int count, const GSTextureCacheSW::GSTexture* texture)
{
	GSDrawingEnvironment& env = m_state->m_env;
	GSDrawingContext* context = m_state->m_context;
	GIFRegPRIM* PRIM = m_state->PRIM;

	// sanity check

	if(context->TEST.ZTE && context->TEST.ZTST == ZTST_NEVER)
	{
		return 0;
	}

	// m_scissor

	m_scissor = context->scissor.in;

	// TODO: find a game that overflows and check which one is the right behaviour

	m_scissor.z = min(m_scissor.z, context->FRAME.FBW * 64); 

	// m_sel

	m_sel.dw = 0;

	if(PRIM->AA1)
	{
		// TODO: automatic alpha blending (ABE=1, A=0 B=1 C=0 D=1)
	}

	m_sel.fpsm = GSUtil::EncodePSM(context->FRAME.PSM);
	m_sel.zpsm = GSUtil::EncodePSM(context->ZBUF.PSM);
	m_sel.ztst = context->TEST.ZTE && context->TEST.ZTST > 1 ? context->TEST.ZTST : context->ZBUF.ZMSK ? 0 : 1;
	m_sel.iip = PRIM->PRIM == GS_POINTLIST || PRIM->PRIM == GS_SPRITE ? 0 : PRIM->IIP;
	m_sel.tfx = PRIM->TME ? context->TEX0.TFX : 4;

	if(m_sel.tfx != 4)
	{
		m_sel.tcc = context->TEX0.TCC;
		m_sel.fst = PRIM->FST;
		m_sel.ltf = context->TEX1.LCM 
			? (context->TEX1.K <= 0 && (context->TEX1.MMAG & 1) || context->TEX1.K > 0 && (context->TEX1.MMIN & 1)) 
			: ((context->TEX1.MMAG & 1) | (context->TEX1.MMIN & 1));

		if(m_sel.fst == 0 && PRIM->PRIM == GS_SPRITE)
		{
			// skip per pixel division if q is constant

			m_sel.fst = 1;

			for(int i = 0; i < count; i += 2)
			{
				GSVector4 q = vertices[i + 1].t.zzzz();

				vertices[i + 0].t /= q;
				vertices[i + 1].t /= q;
			}
		}

		if(m_sel.fst && m_sel.ltf)
		{
			// if q is constant we can do the half pel shift for bilinear sampling on the vertices

			GSVector4 half(0.5f, 0.5f, 0.0f, 0.0f);

			for(int i = 0; i < count; i++)
			{
				vertices[i].t -= half;
			}
		}
	}

	m_sel.atst = context->TEST.ATE ? context->TEST.ATST : ATST_ALWAYS;
	m_sel.afail = context->TEST.ATE ? context->TEST.AFAIL : 0;
	m_sel.fge = PRIM->FGE;
	m_sel.date = context->FRAME.PSM != PSM_PSMCT24 ? context->TEST.DATE : 0;
	m_sel.abea = PRIM->ABE ? context->ALPHA.A : 3;
	m_sel.abeb = PRIM->ABE ? context->ALPHA.B : 3;
	m_sel.abec = PRIM->ABE ? context->ALPHA.C : 3;
	m_sel.abed = PRIM->ABE ? context->ALPHA.D : 3;
	m_sel.pabe = PRIM->ABE ? env.PABE.PABE : 0;
	m_sel.rfb = m_sel.date || m_sel.abe != 255 || m_sel.atst != 1 && m_sel.afail == 3 || context->FRAME.FBMSK != 0 && context->FRAME.FBMSK != 0xffffffff;
	m_sel.wzb = context->DepthWrite();
	m_sel.tlu = PRIM->TME && GSLocalMemory::m_psm[context->TEX0.PSM].pal > 0 ? 1 : 0;

	m_dsf = m_ds[m_sel.fpsm][m_sel.zpsm][m_sel.ztst][m_sel.iip];

	CRBMap<DWORD, DrawScanlinePtr>::CPair* pair = m_dsmap2.Lookup(m_sel);

	if(pair)
	{
		m_dsf = pair->m_value;
	}
	else
	{
		pair = m_dsmap.Lookup(m_sel);

		if(pair && pair->m_value)
		{
			m_dsf = pair->m_value;

			m_dsmap2.SetAt(pair->m_key, pair->m_value);
		}
		else if(!pair)
		{
			_tprintf(_T("*** [%d] fpsm %d zpsm %d ztst %d tfx %d tcc %d fst %d ltf %d atst %d afail %d fge %d rfb %d date %d abe %d\n"), 
				m_dsmap.GetCount(), 
				m_sel.fpsm, m_sel.zpsm, m_sel.ztst, 
				m_sel.tfx, m_sel.tcc, m_sel.fst, m_sel.ltf, 
				m_sel.atst, m_sel.afail, m_sel.fge, m_sel.rfb, m_sel.date, m_sel.abe);

			m_dsmap.SetAt(m_sel, NULL);

			if(FILE* fp = _tfopen(_T("c:\\1.txt"), _T("w")))
			{
				POSITION pos = m_dsmap.GetHeadPosition();

				while(pos) 
				{
					pair = m_dsmap.GetNext(pos);

					if(!pair->m_value)
					{
						_ftprintf(fp, _T("m_dsmap.SetAt(0x%08x, &GSRasterizer::DrawScanlineEx<0x%08x>);\n"), pair->m_key, pair->m_key);
					}
				}

				fclose(fp);
			}
		}
	}

	// m_slenv

	SetupColumnOffset(m_fbco, context->FRAME.Block(), context->FRAME.FBW, context->FRAME.PSM);
	SetupColumnOffset(m_zbco, context->ZBUF.Block(), context->FRAME.FBW, context->ZBUF.PSM);

	m_slenv.steps = 0;
	m_slenv.vm = m_state->m_mem.m_vm32;
	m_slenv.fbr = m_fbco->row;
	m_slenv.zbr = m_zbco->row;
	m_slenv.fbc = m_fbco->col;
	m_slenv.zbc = m_zbco->col;
	m_slenv.fm = GSVector4i(context->FRAME.FBMSK);
	m_slenv.zm = GSVector4i(context->ZBUF.ZMSK ? 0xffffffff : 0);
	m_slenv.datm = GSVector4i(context->TEST.DATM ? 0x80000000 : 0);
	m_slenv.colclamp = GSVector4i(env.COLCLAMP.CLAMP ? 0xffffffff : 0x00ff00ff);
	m_slenv.fba = GSVector4i(context->FBA.FBA ? 0x80000000 : 0);
	m_slenv.aref = GSVector4i((int)context->TEST.AREF + (m_sel.atst == ATST_LESS ? -1 : m_sel.atst == ATST_GREATER ? +1 : 0));
	m_slenv.afix = GSVector4((float)(int)context->ALPHA.FIX);
	m_slenv.afix2 = m_slenv.afix * (2.0f / 256);
	m_slenv.fc = GSVector4((DWORD)env.FOGCOL.ai32[0]);

	if(m_sel.fpsm == 1)
	{
		m_slenv.fm |= GSVector4i::xff000000();
	}

	if(PRIM->TME)
	{
		m_slenv.tex = texture->m_buff;
		m_slenv.pal = m_state->m_mem.m_clut;
		m_slenv.tw = texture->m_tw;

		short tw = (short)(1 << context->TEX0.TW);
		short th = (short)(1 << context->TEX0.TH);

		switch(context->CLAMP.WMS)
		{
		case CLAMP_REPEAT: 
			m_slenv.t.min.u16[0] = tw - 1;
			m_slenv.t.max.u16[0] = 0;
			m_slenv.t.mask.u32[0] = 0xffffffff; 
			break;
		case CLAMP_CLAMP: 
			m_slenv.t.min.u16[0] = 0;
			m_slenv.t.max.u16[0] = tw - 1;
			m_slenv.t.mask.u32[0] = 0; 
			break;
		case CLAMP_REGION_REPEAT: 
			m_slenv.t.min.u16[0] = context->CLAMP.MINU;
			m_slenv.t.max.u16[0] = context->CLAMP.MAXU;
			m_slenv.t.mask.u32[0] = 0; 
			break;
		case CLAMP_REGION_CLAMP: 
			m_slenv.t.min.u16[0] = context->CLAMP.MINU;
			m_slenv.t.max.u16[0] = context->CLAMP.MAXU;
			m_slenv.t.mask.u32[0] = 0xffffffff; 
			break;
		default: 
			__assume(0);
		}

		switch(context->CLAMP.WMT)
		{
		case CLAMP_REPEAT: 
			m_slenv.t.min.u16[4] = th - 1;
			m_slenv.t.max.u16[4] = 0;
			m_slenv.t.mask.u32[2] = 0xffffffff; 
			break;
		case CLAMP_CLAMP: 
			m_slenv.t.min.u16[4] = 0;
			m_slenv.t.max.u16[4] = th - 1;
			m_slenv.t.mask.u32[2] = 0; 
			break;
		case CLAMP_REGION_REPEAT: 
			m_slenv.t.min.u16[4] = context->CLAMP.MINV;
			m_slenv.t.max.u16[4] = context->CLAMP.MAXV;
			m_slenv.t.mask.u32[2] = 0; 
			break;
		case CLAMP_REGION_CLAMP: 
			m_slenv.t.min.u16[4] = context->CLAMP.MINV;
			m_slenv.t.max.u16[4] = context->CLAMP.MAXV;
			m_slenv.t.mask.u32[2] = 0xffffffff; 
			break;
		default: 
			__assume(0);
		}

		m_slenv.t.min = m_slenv.t.min.xxxxl().xxxxh();
		m_slenv.t.max = m_slenv.t.max.xxxxl().xxxxh();
		m_slenv.t.mask = m_slenv.t.mask.xxzz();
	}

	//

	m_solidrect = true;

	if(PRIM->IIP || PRIM->TME || PRIM->ABE || PRIM->FGE
	|| context->TEST.ZTE && context->TEST.ZTST != ZTST_ALWAYS 
	|| context->TEST.ATE && context->TEST.ATST != ATST_ALWAYS
	|| context->TEST.DATE
	|| env.DTHE.DTHE
	|| context->FRAME.FBMSK)
	{
		m_solidrect = false;
	}

	//

	switch(PRIM->PRIM)
	{
	case GS_POINTLIST:
		for(int i = 0; i < count; i++, vertices++) DrawPoint(vertices);
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP: 
		ASSERT(!(count & 1));
		count = count / 2;
		for(int i = 0; i < count; i++, vertices += 2) DrawLine(vertices);
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN:
		ASSERT(!(count % 3));
		count = count / 3;
		for(int i = 0; i < count; i++, vertices += 3) DrawTriangle(vertices);
		break;
	case GS_SPRITE:
		ASSERT(!(count & 1));
		count = count / 2;
		for(int i = 0; i < count; i++, vertices += 2) DrawSprite(vertices);
		break;
	default:
		__assume(0);
	}

	m_state->m_perfmon.Put(GSPerfMon::Fillrate, m_slenv.steps); // TODO: move this to the renderer, not thread safe here

	return count;
}

void GSRasterizer::DrawPoint(Vertex* v)
{
	// TODO: round to closest for point, prestep for line

	GSVector4i p(v->p);

	if(m_scissor.x <= p.x && p.x < m_scissor.z && m_scissor.y <= p.y && p.y < m_scissor.w)
	{
		if((p.y % m_threads) == m_id) 
		{
			(this->*m_dsf)(p.y, p.x, p.x + 1, *v);
		}
	}
}

void GSRasterizer::DrawLine(Vertex* v)
{
	Vertex dv = v[1] - v[0];

	GSVector4 dp = dv.p.abs();
	GSVector4i dpi(dp);

	if(dpi.x == 0 && dpi.y == 0) return;

	int i = dpi.x > dpi.y ? 0 : 1;

	Vertex edge = v[0];
	Vertex dedge = dv / dp.v[i];

	// TODO: prestep + clip with the scissor

	int steps = dpi.v[i];

	while(steps-- > 0)
	{
		DrawPoint(&edge);

		edge += dedge;
	}
}

static const int s_abc[8][4] = 
{
	{0, 1, 2, 0},
	{1, 0, 2, 0},
	{0, 0, 0, 0},
	{1, 2, 0, 0},
	{0, 2, 1, 0},
	{0, 0, 0, 0},
	{2, 0, 1, 0},
	{2, 1, 0, 0},
};

void GSRasterizer::DrawTriangle(Vertex* vertices)
{
	Vertex v[3];

	GSVector4 aabb = vertices[0].p.yyyy(vertices[1].p);
	GSVector4 bccb = vertices[1].p.yyyy(vertices[2].p).xzzx();

	int i = (aabb > bccb).mask() & 7;

	v[0] = vertices[s_abc[i][0]];
	v[1] = vertices[s_abc[i][1]];
	v[2] = vertices[s_abc[i][2]];

	aabb = v[0].p.yyyy(v[1].p);
	bccb = v[1].p.yyyy(v[2].p).xzzx();

	i = (aabb == bccb).mask() & 7;

	switch(i)
	{
	case 0: // a < b < c
		DrawTriangleTopBottom(v);
		break;
	case 1: // a == b < c
		DrawTriangleBottom(v);
		break;
	case 4: // a < b == c
		DrawTriangleTop(v);
		break;
	case 7: // a == b == c
		break;
	default:
		__assume(0);
	}
}

void GSRasterizer::DrawTriangleTop(Vertex* v)
{
	Vertex longest = v[2] - v[1];
	
	if((longest.p == GSVector4::zero()).mask() & 1)
	{
		return;
	}

	Vertex dscan = longest * longest.p.xxxx().rcp();

	SetupScanline<true, true, true>(dscan);

	int i = (longest.p > GSVector4::zero()).mask() & 1;

	Vertex& l = v[0];
	GSVector4 r = v[0].p;

	Vertex vl = v[2 - i] - l;
	GSVector4 vr = v[1 + i].p - r;

	Vertex dl = vl / vl.p.yyyy();
	GSVector4 dr = vr / vr.yyyy();

	GSVector4i tb(l.p.xyxy(v[2].p).ceil());

	int top = tb.y;
	int bottom = tb.w;

	if(top < m_scissor.y) top = m_scissor.y;
	if(bottom > m_scissor.w) bottom = m_scissor.w;
			
	if(top < bottom)
	{
		float py = (float)top - l.p.y;

		if(py > 0)
		{
			GSVector4 dy(py);

			l += dl * dy;
			r += dr * dy;
		}

		DrawTriangleSection(top, bottom, l, dl, r, dr, dscan);
	}
}

void GSRasterizer::DrawTriangleBottom(Vertex* v)
{
	Vertex longest = v[1] - v[0];
	
	if((longest.p == GSVector4::zero()).mask() & 1)
	{
		return;
	}
	
	Vertex dscan = longest * longest.p.xxxx().rcp();

	SetupScanline<true, true, true>(dscan);

	int i = (longest.p > GSVector4::zero()).mask() & 1;

	Vertex& l = v[1 - i];
	GSVector4& r = v[i].p;

	Vertex vl = v[2] - l;
	GSVector4 vr = v[2].p - r;

	Vertex dl = vl / vl.p.yyyy();
	GSVector4 dr = vr / vr.yyyy();

	GSVector4i tb(l.p.xyxy(v[2].p).ceil());

	int top = tb.y;
	int bottom = tb.w;

	if(top < m_scissor.y) top = m_scissor.y;
	if(bottom > m_scissor.w) bottom = m_scissor.w;
	
	if(top < bottom)
	{
		float py = (float)top - l.p.y;

		if(py > 0)
		{
			GSVector4 dy(py);

			l += dl * dy;
			r += dr * dy;
		}

		DrawTriangleSection(top, bottom, l, dl, r, dr, dscan);
	}
}

void GSRasterizer::DrawTriangleTopBottom(Vertex* v)
{
	Vertex v01, v02, v12;

	v01 = v[1] - v[0];
	v02 = v[2] - v[0];

	Vertex longest = v[0] + v02 * (v01.p / v02.p).yyyy() - v[1];

	if((longest.p == GSVector4::zero()).mask() & 1)
	{
		return;
	}

	Vertex dscan = longest * longest.p.xxxx().rcp();

	SetupScanline<true, true, true>(dscan);

	Vertex& l = v[0];
	GSVector4 r = v[0].p;

	Vertex dl;
	GSVector4 dr;

	bool b = (longest.p > GSVector4::zero()).mask() & 1;
	
	if(b)
	{
		dl = v01 / v01.p.yyyy();
		dr = v02.p / v02.p.yyyy();
	}
	else
	{
		dl = v02 / v02.p.yyyy();
		dr = v01.p / v01.p.yyyy();
	}

	GSVector4i tb(v[0].p.yyyy(v[1].p).xzyy(v[2].p).ceil());

	int top = tb.x;
	int bottom = tb.y;

	if(top < m_scissor.y) top = m_scissor.y;
	if(bottom > m_scissor.w) bottom = m_scissor.w;

	float py = (float)top - l.p.y;

	if(py > 0)
	{
		GSVector4 dy(py);

		l += dl * dy;
		r += dr * dy;
	}

	if(top < bottom)
	{
		DrawTriangleSection(top, bottom, l, dl, r, dr, dscan);
	}

	if(b)
	{
		v12 = v[2] - v[1];

		l = v[1];

		dl = v12 / v12.p.yyyy();
	}
	else
	{
		v12.p = v[2].p - v[1].p;

		r = v[1].p;

		dr = v12.p / v12.p.yyyy();
	}

	top = tb.y;
	bottom = tb.z;

	if(top < m_scissor.y) top = m_scissor.y;
	if(bottom > m_scissor.w) bottom = m_scissor.w;

	if(top < bottom)
	{
		py = (float)top - l.p.y;

		if(py > 0) l += dl * py;

		py = (float)top - r.y;

		if(py > 0) r += dr * py;

		DrawTriangleSection(top, bottom, l, dl, r, dr, dscan);
	}
}

void GSRasterizer::DrawTriangleSection(int top, int bottom, Vertex& l, const Vertex& dl, GSVector4& r, const GSVector4& dr, const Vertex& dscan)
{
	ASSERT(top < bottom);

	while(1)
	{
		do
		{
			// rarely used (character shadows in ffx-2)

			/*

			int scanmsk = (int)m_state->m_env.SCANMSK.MSK - 2;

			if(scanmsk >= 0)
			{
				if(((top & 1) ^ scanmsk) == 0)
				{
					continue;
				}
			}

			*/

			if((top % m_threads) == m_id) 
			{
				GSVector4i lr(l.p.xyxy(r).ceil());

				int& left = lr.x;
				int& right = lr.z;

				if(left < m_scissor.x) left = m_scissor.x;
				if(right > m_scissor.z) right = m_scissor.z;

				if(left < right)
				{
					Vertex scan = l;

					float px = (float)left - l.p.x;

					if(px > 0) scan += dscan * px;

					(this->*m_dsf)(top, left, right, scan);
				}
			}
		}
		while(0);

		if(++top >= bottom) break;

		l += dl;
		r += dr;
	}
}

void GSRasterizer::DrawSprite(Vertex* vertices)
{
	Vertex v[2];

	GSVector4 mask = vertices[0].p > vertices[1].p;

	v[0].p = vertices[0].p.blend8(vertices[1].p, mask).xyzw(vertices[1].p);
	v[0].t = vertices[0].t.blend8(vertices[1].t, mask).xyzw(vertices[1].t);
	v[0].c = vertices[1].c;

	v[1].p = vertices[1].p.blend8(vertices[0].p, mask);
	v[1].t = vertices[1].t.blend8(vertices[0].t, mask);

	GSVector4i r(v[0].p.xyxy(v[1].p).ceil());

	int& top = r.y;
	int& bottom = r.w;

	int& left = r.x;
	int& right = r.z;

	#if _M_SSE >= 0x401

	r = r.sat_i32(m_scissor);
	
	if((r < r.zwzw()).mask() != 0x00ff) return;

	#else

	if(top < m_scissor.y) top = m_scissor.y;
	if(bottom > m_scissor.w) bottom = m_scissor.w;
	if(top >= bottom) return;

	if(left < m_scissor.x) left = m_scissor.x;
	if(right > m_scissor.z) right = m_scissor.z;
	if(left >= right) return;

	#endif

	Vertex scan = v[0];

	if(m_solidrect)
	{
		if(m_id == 0)
		{
			DrawSolidRect(r, scan);
		}

		return;
	}

	GSVector4 zero = GSVector4::zero();

	Vertex dedge, dscan;

	dedge.p = zero;
	dscan.p = zero;

	if(m_sel.tfx < 4)
	{
		Vertex dv = v[1] - v[0];

		dscan.t = (dv.t / dv.p.xxxx()).xyxy(zero).xwww();
		dedge.t = (dv.t / dv.p.yyyy()).xyxy(zero).wyww();

		if(scan.p.y < (float)top) scan.t += dedge.t * ((float)top - scan.p.y);
		if(scan.p.x < (float)left) scan.t += dscan.t * ((float)left - scan.p.x);

		SetupScanline<true, true, false>(dscan);

		for(; top < bottom; top++, scan.t += dedge.t)
		{
			if((top % m_threads) == m_id) 
			{
				(this->*m_dsf)(top, left, right, scan);
			}
		}
	}
	else
	{
		SetupScanline<true, false, false>(dscan);

		for(; top < bottom; top++)
		{
			if((top % m_threads) == m_id) 
			{
				(this->*m_dsf)(top, left, right, scan);
			}
		}
	}
}

void GSRasterizer::DrawSolidRect(const GSVector4i& r, const Vertex& v)
{
	ASSERT(r.y >= 0);
	ASSERT(r.w >= 0);

	GSDrawingContext* context = m_state->m_context;

	DWORD fbp = context->FRAME.Block();
	DWORD fpsm = context->FRAME.PSM;
	DWORD zbp = context->ZBUF.Block();
	DWORD zpsm = context->ZBUF.PSM;
	DWORD bw = context->FRAME.FBW;

	if(!context->ZBUF.ZMSK)
	{
		m_state->m_mem.FillRect(r, (DWORD)(float)v.p.z, zpsm, zbp, bw);
	}

	DWORD c = v.c.rgba32();

	if(context->FBA.FBA)
	{
		c |= 0x80000000;
	}
	
	if(fpsm == PSM_PSMCT16 || fpsm == PSM_PSMCT16S)
	{
		c = ((c & 0xf8) >> 3) | ((c & 0xf800) >> 6) | ((c & 0xf80000) >> 9) | ((c & 0x80000000) >> 16);
	}

	m_state->m_mem.FillRect(r, c, fpsm, fbp, bw);

	// m_slenv.steps += r.Width() * r.Height();
}

void GSRasterizer::SetupColumnOffset(ColumnOffset*& co, DWORD bp, DWORD bw, DWORD psm)
{
	if(bw == 0) {ASSERT(0); return;}

	DWORD hash = bp | (bw << 14) | (psm << 20);

	if(!co || co->hash != hash)
	{
		CRBMap<DWORD, ColumnOffset*>::CPair* pair = m_comap.Lookup(hash);

		if(pair)
		{
			co = pair->m_value;
		}
		else
		{
			co = (ColumnOffset*)_aligned_malloc(sizeof(ColumnOffset), 16);

			co->hash = hash;

			GSLocalMemory::pixelAddress pa = GSLocalMemory::m_psm[psm].pa;

			for(int i = 0, j = 1024; i < j; i++)
			{
				co->row[i] = GSVector4i((int)pa(0, i, bp, bw));
			}

			int* p = (int*)_aligned_malloc(sizeof(int) * (2048 + 3) * 4, 16);

			for(int i = 0; i < 4; i++)
			{
				co->col[i] = &p[2048 * i + ((4 - (i & 3)) & 3)];

				memcpy(co->col[i], GSLocalMemory::m_psm[psm].rowOffset[0], sizeof(int) * 2048);
			}

			m_comap.SetAt(hash, co);
		}
	}
}

template<bool pos, bool tex, bool col> 
void GSRasterizer::SetupScanline(const Vertex& dv)
{
	if(pos)
	{
		GSVector4 dp = dv.p;

		m_slenv.dp = dp;
		m_slenv.dp4 = dp * 4.0f;
	}

	if(tex)
	{
		GSVector4 dt = dv.t;

		m_slenv.dt = dt;
		m_slenv.dt4 = dt * 4.0f;
	}

	if(col)
	{
		GSVector4 dc = dv.c;

		m_slenv.dc = dc;
		m_slenv.dc4 = dc * 4.0f;
	}
}

template<DWORD fpsm, DWORD zpsm, DWORD ztst, DWORD iip>
void GSRasterizer::DrawScanline(int top, int left, int right, const Vertex& v)	
{
	GSVector4i fa_base = m_slenv.fbr[top];
	GSVector4i* fa_offset = (GSVector4i*)&m_slenv.fbc[left & 3][left];

	GSVector4i za_base = m_slenv.zbr[top];
	GSVector4i* za_offset = (GSVector4i*)&m_slenv.zbc[left & 3][left];

	GSVector4 ps0123 = GSVector4::ps0123();

	GSVector4 vp = v.p;
	GSVector4 dp = m_slenv.dp;
	GSVector4 z = vp.zzzz(); z += dp.zzzz() * ps0123;
	GSVector4 f = vp.wwww(); f += dp.wwww() * ps0123;

	GSVector4 vt = v.t;
	GSVector4 dt = m_slenv.dt;
	GSVector4 s = vt.xxxx(); s += dt.xxxx() * ps0123;
	GSVector4 t = vt.yyyy(); t += dt.yyyy() * ps0123;
	GSVector4 q = vt.zzzz(); q += dt.zzzz() * ps0123;

	GSVector4 vc = v.c;
	GSVector4 dc = m_slenv.dc;
	GSVector4 r = vc.xxxx(); if(iip) r += dc.xxxx() * ps0123;
	GSVector4 g = vc.yyyy(); if(iip) g += dc.yyyy() * ps0123;
	GSVector4 b = vc.zzzz(); if(iip) b += dc.zzzz() * ps0123;
	GSVector4 a = vc.wwww(); if(iip) a += dc.wwww() * ps0123;

	int steps = right - left;

	m_slenv.steps += steps;

	while(1)
	{
		do
		{

		GSVector4i za = za_base + GSVector4i::load<true>(za_offset);
		
		GSVector4i zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::one(za));

		GSVector4i test;

		if(!TestZ(zpsm, ztst, zs, za, test))
		{
			continue;
		}

		int pixels = GSVector4i::store(GSVector4i::load(steps).min_i16(GSVector4i::load(4)));

		GSVector4 c[12];

		if(m_sel.tfx != TFX_NONE)
		{
			GSVector4 u = s;
			GSVector4 v = t;

			if(!m_sel.fst)
			{
				GSVector4 w = q.rcp();

				u *= w;
				v *= w;

				if(m_sel.ltf)
				{
					u -= 0.5f;
					v -= 0.5f;
				}
			}

			SampleTexture(ztst, test, pixels, m_sel.ltf, m_sel.tlu, u, v, c);
		}

		AlphaTFX(m_sel.tfx, m_sel.tcc, a, c[3]);

		GSVector4i fm = m_slenv.fm;
		GSVector4i zm = m_slenv.zm;

		if(!TestAlpha(m_sel.atst, m_sel.afail, c[3], fm, zm, test))
		{
			continue;
		}

		ColorTFX(m_sel.tfx, r, g, b, a, c[0], c[1], c[2]);

		if(m_sel.fge)
		{
			Fog(f, c[0], c[1], c[2]);
		}

		GSVector4i fa = fa_base + GSVector4i::load<true>(fa_offset);

		GSVector4i d = GSVector4i::zero();

		if(m_sel.rfb)
		{
			d = ReadFrameX(fpsm == 1 ? 0 : fpsm, fa);

			if(fpsm != 1 && m_sel.date)
			{
				test |= (d ^ m_slenv.datm).sra32(31);

				if(test.alltrue())
				{
					continue;
				}
			}
		}

		fm |= test;
		zm |= test;

		if(m_sel.abe != 255)
		{
//			GSVector4::expand(d, c[4], c[5], c[6], c[7]);

			c[4] = (d << 24) >> 24;
			c[5] = (d << 16) >> 24;
			c[6] = (d <<  8) >> 24;
			c[7] = (d >> 24);

			if(fpsm == 1)
			{
				c[7] = GSVector4(128.0f);
			}

			c[8] = GSVector4::zero();
			c[9] = GSVector4::zero();
			c[10] = GSVector4::zero();
			c[11] = m_slenv.afix;

			DWORD abea = m_sel.abea;
			DWORD abeb = m_sel.abeb;
			DWORD abec = m_sel.abec;
			DWORD abed = m_sel.abed;

			GSVector4 r = (c[abea*4 + 0] - c[abeb*4 + 0]).mod2x(c[abec*4 + 3]) + c[abed*4 + 0];
			GSVector4 g = (c[abea*4 + 1] - c[abeb*4 + 1]).mod2x(c[abec*4 + 3]) + c[abed*4 + 1];
			GSVector4 b = (c[abea*4 + 2] - c[abeb*4 + 2]).mod2x(c[abec*4 + 3]) + c[abed*4 + 2];

			if(m_sel.pabe)
			{
				GSVector4 mask = c[3] >= GSVector4(128.0f);

				c[0] = c[0].blend8(r, mask);
				c[1] = c[1].blend8(g, mask);
				c[2] = c[2].blend8(b, mask);
			}
			else
			{
				c[0] = r;
				c[1] = g;
				c[2] = b;
			}
		}

		GSVector4i rb = GSVector4i(c[0]).ps32(GSVector4i(c[2]));
		GSVector4i ga = GSVector4i(c[1]).ps32(GSVector4i(c[3]));
		
		GSVector4i rg = rb.upl16(ga) & m_slenv.colclamp;
		GSVector4i ba = rb.uph16(ga) & m_slenv.colclamp;
		
		GSVector4i s = rg.upl32(ba).pu16(rg.uph32(ba));

		if(fpsm != 1)
		{
			s |= m_slenv.fba;
		}

		if(m_sel.rfb)
		{
			s = s.blend(d, fm);
		}

		WriteFrameAndZBufX(fpsm, fa, fm, s, ztst > 0 ? zpsm : 3, za, zm, zs, pixels);

		}
		while(0);

		if(steps <= 4) break;

		steps -= 4;

		fa_offset++;
		za_offset++;

		GSVector4 dp4 = m_slenv.dp4;

		z += dp4.zzzz();
		f += dp4.wwww();

		GSVector4 dt4 = m_slenv.dt4;

		s += dt4.xxxx();
		t += dt4.yyyy();
		q += dt4.zzzz();

		GSVector4 dc4 = m_slenv.dc4;

		if(iip) r += dc4.xxxx();
		if(iip) g += dc4.yyyy();
		if(iip) b += dc4.zzzz();
		if(iip) a += dc4.wwww();
	}
}

void GSRasterizer::SampleTexture(DWORD ztst, const GSVector4i& test, int pixels, DWORD ltf, DWORD tlu, const GSVector4& u, const GSVector4& v, GSVector4* c)
{
	const void* RESTRICT tex = m_slenv.tex;
	const DWORD* RESTRICT pal = m_slenv.pal;
	const DWORD tw = m_slenv.tw;

	if(ltf)
	{
		GSVector4 uf = u.floor();
		GSVector4 vf = v.floor();
		
		GSVector4 uff = u - uf;
		GSVector4 vff = v - vf;

		GSVector4i uv = GSVector4i(uf).ps32(GSVector4i(vf));

		GSVector4i uv0 = Wrap(uv);
		GSVector4i uv1 = Wrap(uv + GSVector4i::x0001(uv));

		int i = 0;

		if(tlu)
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				GSVector4 c00 = GSVector4(pal[((const BYTE*)tex)[(uv0.u16[i + 4] << tw) + uv0.u16[i]]]);
				GSVector4 c01 = GSVector4(pal[((const BYTE*)tex)[(uv0.u16[i + 4] << tw) + uv1.u16[i]]]);
				GSVector4 c10 = GSVector4(pal[((const BYTE*)tex)[(uv1.u16[i + 4] << tw) + uv0.u16[i]]]);
				GSVector4 c11 = GSVector4(pal[((const BYTE*)tex)[(uv1.u16[i + 4] << tw) + uv1.u16[i]]]);

				c00 = c00.lerp(c01, uff.v[i]);
				c10 = c10.lerp(c11, uff.v[i]);
				c00 = c00.lerp(c10, vff.v[i]);

				c[i] = c00;

			}
			while(++i < pixels);
		}
		else
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				GSVector4 c00 = GSVector4(((const DWORD*)tex)[(uv0.u16[i + 4] << tw) + uv0.u16[i]]);
				GSVector4 c01 = GSVector4(((const DWORD*)tex)[(uv0.u16[i + 4] << tw) + uv1.u16[i]]);
				GSVector4 c10 = GSVector4(((const DWORD*)tex)[(uv1.u16[i + 4] << tw) + uv0.u16[i]]);
				GSVector4 c11 = GSVector4(((const DWORD*)tex)[(uv1.u16[i + 4] << tw) + uv1.u16[i]]);

				c00 = c00.lerp(c01, uff.v[i]);
				c10 = c10.lerp(c11, uff.v[i]);
				c00 = c00.lerp(c10, vff.v[i]);

				c[i] = c00;
			}
			while(++i < pixels);
		}

		GSVector4::transpose(c[0], c[1], c[2], c[3]);
	}
	else
	{
		GSVector4i uv = Wrap(GSVector4i(u).ps32(GSVector4i(v)));

		GSVector4i c00;

		int i = 0;

		if(tlu)
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				c00.u32[i] = pal[((const BYTE*)tex)[(uv.u16[i + 4] << tw) + uv.u16[i]]];
			}
			while(++i < pixels);
		}
		else
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				c00.u32[i] = ((const DWORD*)tex)[(uv.u16[i + 4] << tw) + uv.u16[i]];
			}
			while(++i < pixels);
		}

		// GSVector4::expand(c00, c[0], c[1], c[2], c[3]);

		c[0] = (c00 << 24) >> 24;
		c[1] = (c00 << 16) >> 24;
		c[2] = (c00 <<  8) >> 24;
		c[3] = (c00 >> 24);
	}
}

void GSRasterizer::ColorTFX(DWORD tfx, const GSVector4& rf, const GSVector4& gf, const GSVector4& bf, const GSVector4& af, GSVector4& rt, GSVector4& gt, GSVector4& bt)
{
	switch(tfx)
	{
	case TFX_MODULATE:
		rt = rt.mod2x(rf);
		gt = gt.mod2x(gf);
		bt = bt.mod2x(bf);
		rt = rt.clamp();
		gt = gt.clamp();
		bt = bt.clamp();
		break;
	case TFX_DECAL:
		break;
	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:
		rt = rt.mod2x(rf) + af;
		gt = gt.mod2x(gf) + af;
		bt = bt.mod2x(bf) + af;
		rt = rt.clamp();
		gt = gt.clamp();
		bt = bt.clamp();
		break;
	case TFX_NONE:
		rt = rf;
		gt = gf;
		bt = bf;
		break;
	default:
		__assume(0);
	}
}

void GSRasterizer::AlphaTFX(DWORD tfx, DWORD tcc, const GSVector4& af, GSVector4& at)
{
	switch(tfx)
	{
	case TFX_MODULATE: at = tcc ? at.mod2x(af).clamp() : af; break;
	case TFX_DECAL: break;
	case TFX_HIGHLIGHT: at = tcc ? (at + af).clamp() : af; break;
	case TFX_HIGHLIGHT2: if(!tcc) at = af; break;
	case TFX_NONE: at = af; break; 
	default: __assume(0);
	}
}

void GSRasterizer::Fog(const GSVector4& f, GSVector4& r, GSVector4& g, GSVector4& b)
{
	GSVector4 fc = m_slenv.fc;

	r = fc.xxxx().lerp(r, f);
	g = fc.yyyy().lerp(g, f);
	b = fc.zzzz().lerp(b, f);
}

bool GSRasterizer::TestZ(DWORD zpsm, DWORD ztst, const GSVector4i& zs, const GSVector4i& za, GSVector4i& test)
{
	if(ztst > 1)
	{
		GSVector4i zd = ReadZBufX(zpsm, za);

		GSVector4i zso = zs;
		GSVector4i zdo = zd;

		if(zpsm == 0)
		{
			GSVector4i o = GSVector4i::x80000000(zs);

			zso = zs - o;
			zdo = zd - o;
		}

		switch(ztst)
		{
		case ZTST_GEQUAL: test = zso < zdo; break;
		case ZTST_GREATER: test = zso <= zdo; break;
		default: __assume(0);
		}

		if(test.alltrue())
		{
			return false;
		}
	}
	else
	{
		test = GSVector4i::zero();
	}

	return true;
}

bool GSRasterizer::TestAlpha(DWORD atst, DWORD afail, const GSVector4& a, GSVector4i& fm, GSVector4i& zm, GSVector4i& test)
{
	if(atst != 1)
	{
		GSVector4i t;

		switch(atst)
		{
		case ATST_NEVER: t = GSVector4i::invzero(); break;
		case ATST_ALWAYS: t = GSVector4i::zero(); break;
		case ATST_LESS: 
		case ATST_LEQUAL: t = GSVector4i(a) > m_slenv.aref; break;
		case ATST_EQUAL: t = GSVector4i(a) != m_slenv.aref; break;
		case ATST_GEQUAL: 
		case ATST_GREATER: t = GSVector4i(a) < m_slenv.aref; break;
		case ATST_NOTEQUAL: t = GSVector4i(a) == m_slenv.aref; break;
		default: __assume(0);
		}

		switch(afail)
		{
		case AFAIL_KEEP:
			fm |= t;
			zm |= t;
			test |= t;
			if(test.alltrue()) return false;
			break;
		case AFAIL_FB_ONLY:
			zm |= t;
			break;
		case AFAIL_ZB_ONLY:
			fm |= t;
			break;
		case AFAIL_RGB_ONLY: 
			fm |= t & GSVector4i::xff000000(t);
			zm |= t;
			break;
		default: 
			__assume(0);
		}
	}

	return true;
}

DWORD GSRasterizer::ReadPixel32(DWORD* RESTRICT vm, DWORD addr)
{
	return vm[addr];
}

DWORD GSRasterizer::ReadPixel24(DWORD* RESTRICT vm, DWORD addr)
{
	return vm[addr] & 0x00ffffff;
}

DWORD GSRasterizer::ReadPixel16(WORD* RESTRICT vm, DWORD addr)
{
	return (DWORD)vm[addr];
}

void GSRasterizer::WritePixel32(DWORD* RESTRICT vm, DWORD addr, DWORD c) 
{
	vm[addr] = c;
}

void GSRasterizer::WritePixel24(DWORD* RESTRICT vm, DWORD addr, DWORD c) 
{
	vm[addr] = (vm[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSRasterizer::WritePixel16(WORD* RESTRICT vm, DWORD addr, DWORD c) 
{
	vm[addr] = (WORD)c;
}

GSVector4i GSRasterizer::ReadFrameX(int psm, const GSVector4i& addr) const
{
	DWORD* RESTRICT vm32 = (DWORD*)m_slenv.vm;
	WORD* RESTRICT vm16 = (WORD*)m_slenv.vm;

	GSVector4i c, r, g, b, a;

	switch(psm)
	{
	case 0:
		#if _M_SSE >= 0x401
		c = addr.gather32_32(vm32);
		#else
		c = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		break;
	case 1:
		#if _M_SSE >= 0x401
		c = addr.gather32_32(vm32);
		#else
		c = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		c = (c & GSVector4i::x00ffffff(addr)) | GSVector4i::x80000000(addr);
		break;
	case 2:
		#if _M_SSE >= 0x401
		c = addr.gather32_32(vm16);
		#else
		c = GSVector4i(
			ReadPixel16(vm16, addr.u32[0]),
			ReadPixel16(vm16, addr.u32[1]),
			ReadPixel16(vm16, addr.u32[2]),
			ReadPixel16(vm16, addr.u32[3]));
		#endif
		c = ((c & 0x8000) << 16) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3); 
		break;
	default: 
		ASSERT(0); 
		c = GSVector4i::zero();
	}
	
	return c;
}

GSVector4i GSRasterizer::ReadZBufX(int psm, const GSVector4i& addr) const
{
	DWORD* RESTRICT vm32 = (DWORD*)m_slenv.vm;
	WORD* RESTRICT vm16 = (WORD*)m_slenv.vm;

	GSVector4i z;

	switch(psm)
	{
	case 0: 
		#if _M_SSE >= 0x401
		z = addr.gather32_32(vm32);
		#else
		z = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		break;
	case 1: 
		#if _M_SSE >= 0x401
		z = addr.gather32_32(vm32);
		#else
		z = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		z = z & GSVector4i::x00ffffff(addr);
		break;
	case 2: 
		#if _M_SSE >= 0x401
		z = addr.gather32_32(vm16);
		#else
		z = GSVector4i(
			ReadPixel16(vm16, addr.u32[0]),
			ReadPixel16(vm16, addr.u32[1]),
			ReadPixel16(vm16, addr.u32[2]),
			ReadPixel16(vm16, addr.u32[3]));
		#endif
		break;
	default: 
		ASSERT(0); 
		z = GSVector4i::zero();
	}

	return z;
}

void GSRasterizer::WriteFrameAndZBufX(
	int fpsm, const GSVector4i& fa, const GSVector4i& fm, const GSVector4i& f, 
	int zpsm, const GSVector4i& za, const GSVector4i& zm, const GSVector4i& z, 
	int pixels)
{
	// FIXME: compiler problem or not enough xmm regs in x86 mode to store the address regs (fa, za)

	DWORD* RESTRICT vm32 = (DWORD*)m_slenv.vm;
	WORD* RESTRICT vm16 = (WORD*)m_slenv.vm;

	GSVector4i c = f;

	if(fpsm == 2)
	{
		GSVector4i rb = c & 0x00f800f8;
		GSVector4i ga = c & 0x8000f800;
		c = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);
	}

	#if _M_SSE >= 0x401

	if(fm.extract32<0>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[0], c.extract32<0>()); break;
		case 1: WritePixel24(vm32, fa.u32[0], c.extract32<0>()); break;
		case 2: WritePixel16(vm16, fa.u32[0], c.extract16<0 * 2>()); break;
		}
	}

	if(zm.extract32<0>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[0], z.extract32<0>()); break;
		case 1: WritePixel24(vm32, za.u32[0], z.extract32<0>()); break;
		case 2: WritePixel16(vm16, za.u32[0], z.extract16<0 * 2>()); break;
		}
	}

	if(pixels <= 1) return;

	if(fm.extract32<1>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[1], c.extract32<1>()); break;
		case 1: WritePixel24(vm32, fa.u32[1], c.extract32<1>()); break;
		case 2: WritePixel16(vm16, fa.u32[1], c.extract16<1 * 2>()); break;
		}
	}

	if(zm.extract32<1>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[1], z.extract32<1>()); break;
		case 1: WritePixel24(vm32, za.u32[1], z.extract32<1>()); break;
		case 2: WritePixel16(vm16, za.u32[1], z.extract16<1 * 2>()); break;
		}
	}

	if(pixels <= 2) return;

	if(fm.extract32<2>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[2], c.extract32<2>()); break;
		case 1: WritePixel24(vm32, fa.u32[2], c.extract32<2>()); break;
		case 2: WritePixel16(vm16, fa.u32[2], c.extract16<2 * 2>()); break;
		}
	}

	if(zm.extract32<2>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[2], z.extract32<2>()); break;
		case 1: WritePixel24(vm32, za.u32[2], z.extract32<2>()); break;
		case 2: WritePixel16(vm16, za.u32[2], z.extract16<2 * 2>()); break;
		}
	}

	if(pixels <= 3) return;

	if(fm.extract32<3>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[3], c.extract32<3>()); break;
		case 1: WritePixel24(vm32, fa.u32[3], c.extract32<3>()); break;
		case 2: WritePixel16(vm16, fa.u32[3], c.extract16<3 * 2>()); break;
		}
	}

	if(zm.extract32<3>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[3], z.extract32<3>()); break;
		case 1: WritePixel24(vm32, za.u32[3], z.extract32<3>()); break;
		case 2: WritePixel16(vm16, za.u32[3], z.extract16<3 * 2>()); break;
		}
	}

	#else

	int i = 0;

	do
	{
		if(fm.u32[i] != 0xffffffff)
		{
			switch(fpsm)
			{
			case 0: WritePixel32(vm32, fa.u32[i], c.u32[i]);  break;
			case 1: WritePixel24(vm32, fa.u32[i], c.u32[i]);  break;
			case 2: WritePixel16(vm16, fa.u32[i], c.u16[i * 2]);  break;
			}
		}

		if(zm.u32[i] != 0xffffffff) 
		{
			switch(zpsm)
			{
			case 0: WritePixel32(vm32, za.u32[i], z.u32[i]);  break;
			case 1: WritePixel24(vm32, za.u32[i], z.u32[i]);  break;
			case 2: WritePixel16(vm16, za.u32[i], z.u16[i * 2]);  break;
			}
		}
	}
	while(++i < pixels);

	#endif
}

//

GSRasterizerMT::GSRasterizerMT(GSState* state, int id, int threads, long* sync)
	: GSRasterizer(state, id, threads)
	, m_vertices(NULL)
	, m_count(0)
	, m_sync(sync)
	, m_exit(false)
	, m_ThreadId(0)
	, m_hThread(NULL)
{
	m_hThread = CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);
}

GSRasterizerMT::~GSRasterizerMT()
{
	if(m_hThread != NULL)
	{
		m_exit = true;

		if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
		{
			TerminateThread(m_hThread, 1);
		}

		CloseHandle(m_hThread);
	}
}

void GSRasterizerMT::BeginDraw(Vertex* vertices, int count, const GSTextureCacheSW::GSTexture* texture)
{
	m_vertices = vertices;
	m_count = count;
	m_texture = texture;

	InterlockedBitTestAndSet(m_sync, m_id);
}

DWORD WINAPI GSRasterizerMT::StaticThreadProc(LPVOID lpParam)
{
	return ((GSRasterizerMT*)lpParam)->ThreadProc();
}

DWORD GSRasterizerMT::ThreadProc()
{
	// _mm_setcsr(MXCSR);

	while(!m_exit)
	{
		if(*m_sync & (1 << m_id))
		{
			Draw(m_vertices, m_count, m_texture);

			InterlockedBitTestAndReset(m_sync, m_id);
		}
		else
		{
			_mm_pause();
		}
	}

	return 0;
}

