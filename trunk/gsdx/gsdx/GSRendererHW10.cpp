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

#include "stdafx.h"
#include "GSRendererHW10.h"
#include "GSTextureCache10.h"
#include "resource.h"

extern int s_n;
extern bool s_dump;
extern bool s_save;
extern bool s_savez;

GSRendererHW10::GSRendererHW10(BYTE* base, bool mt, void (*irq)(), int nloophack, int interlace, int aspectratio, int filter, bool vsync)
	: GSRendererHW<GSDevice10>(base, mt, irq, nloophack, interlace, aspectratio, filter, vsync, true)
{
	m_tc = new GSTextureCache10(this, !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE));

	if(!AfxGetApp()->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE))
	{
		m_width = AfxGetApp()->GetProfileInt(_T("Settings"), _T("resx"), 1024);
		m_height = AfxGetApp()->GetProfileInt(_T("Settings"), _T("resy"), 1024);
	}
}

bool GSRendererHW10::Create(LPCTSTR title)
{
	if(!__super::Create(title))
		return false;

	if(!m_tfx.Create(&m_dev))
		return false;

	//

	D3D10_DEPTH_STENCIL_DESC dsd;

	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = false;
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 1;
	dsd.StencilWriteMask = 1;
	dsd.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;

	m_dev->CreateDepthStencilState(&dsd, &m_date.dss);

	D3D10_BLEND_DESC bd;

	memset(&bd, 0, sizeof(bd));

	m_dev->CreateBlendState(&bd, &m_date.bs);

	//

	return true;
}

void GSRendererHW10::DrawingKick(bool skip)
{
	GSVertexHW* v = &m_vertices[m_count];
	int nv = 0;

	switch(PRIM->PRIM)
	{
	case GS_POINTLIST:
		m_vl.RemoveAt(0, v[0]);
		nv = 1;
		break;
	case GS_LINELIST:
		m_vl.RemoveAt(0, v[0]);
		m_vl.RemoveAt(0, v[1]);
		nv = 2;
		break;
	case GS_LINESTRIP:
		m_vl.RemoveAt(0, v[0]);
		m_vl.GetAt(0, v[1]);
		nv = 2;
		break;
	case GS_TRIANGLELIST:
		m_vl.RemoveAt(0, v[0]);
		m_vl.RemoveAt(0, v[1]);
		m_vl.RemoveAt(0, v[2]);
		nv = 3;
		break;
	case GS_TRIANGLESTRIP:
		m_vl.RemoveAt(0, v[0]);
		m_vl.GetAt(0, v[1]);
		m_vl.GetAt(1, v[2]);
		nv = 3;
		break;
	case GS_TRIANGLEFAN:
		m_vl.GetAt(0, v[0]);
		m_vl.RemoveAt(1, v[1]);
		m_vl.GetAt(1, v[2]);
		nv = 3;
		break;
	case GS_SPRITE:
		m_vl.RemoveAt(0, v[0]);
		m_vl.RemoveAt(0, v[1]);
		nv = 2;
		break;
	default:
		m_vl.RemoveAll();
		ASSERT(0);
		return;
	}

	if(skip)
	{
		return;
	}

	float sx0 = m_context->scissor.x0;
	float sy0 = m_context->scissor.y0;
	float sx1 = m_context->scissor.x1;
	float sy1 = m_context->scissor.y1;

	switch(nv)
	{
	case 1:
		if(v[0].p.x < sx0
		|| v[0].p.x > sx1
		|| v[0].p.y < sy0
		|| v[0].p.y > sy1)
			return;
		break;
	case 2:
		if(v[0].p.x < sx0 && v[1].p.x < sx0
		|| v[0].p.x > sx1 && v[1].p.x > sx1
		|| v[0].p.y < sy0 && v[1].p.y < sy0
		|| v[0].p.y > sy1 && v[1].p.y > sy1)
			return;
		break;
	case 3:
		if(v[0].p.x < sx0 && v[1].p.x < sx0 && v[2].p.x < sx0
		|| v[0].p.x > sx1 && v[1].p.x > sx1 && v[2].p.x > sx1
		|| v[0].p.y < sy0 && v[1].p.y < sy0 && v[2].p.y < sy0
		|| v[0].p.y > sy1 && v[1].p.y > sy1 && v[2].p.y > sy1)
			return;
		break;
	default:
		__assume(0);
	}

	m_count += nv;

	// costs a few fps, but fixes RR's shadows (or anything which paints overlapping shapes with date)
/*
	if(m_context->TEST.DATE)
	{
		Flush();
	}

	if(m_env.COLCLAMP.CLAMP == 0)
	{
		Flush();
	}
*/
}

void GSRendererHW10::Draw()
{
	if(DetectBadFrame(m_skip))
	{
		return;
	}

	//

	GIFRegTEX0 TEX0;

	// rt

	TEX0.TBP0 = m_context->FRAME.Block();
	TEX0.TBW = m_context->FRAME.FBW;
	TEX0.PSM = m_context->FRAME.PSM;

	GSTextureCache<GSDevice10>::GSRenderTarget* rt = m_tc->GetRenderTarget(TEX0, m_width, m_height);

	// ds

	TEX0.TBP0 = m_context->ZBUF.Block();
	TEX0.TBW = m_context->FRAME.FBW;
	TEX0.PSM = m_context->ZBUF.PSM;

	GSTextureCache<GSDevice10>::GSDepthStencil* ds = m_tc->GetDepthStencil(TEX0, m_width, m_height);

	// tex

	GSTextureCache<GSDevice10>::GSTexture* tex = NULL;

	if(PRIM->TME)
	{
		tex = m_tc->GetTexture();

		if(!tex) return;
	}

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_tex_%05x_%d_%d%d_%02x_%02x_%02x_%02x.dds"), 
		s_n++, m_perfmon.GetFrame(), (int)m_context->TEX0.TBP0, (int)m_context->TEX0.PSM,
		(int)m_context->CLAMP.WMS, (int)m_context->CLAMP.WMT, 
		(int)m_context->CLAMP.MINU, (int)m_context->CLAMP.MAXU, 
		(int)m_context->CLAMP.MINV, (int)m_context->CLAMP.MAXV);
	if(PRIM->TME) if(s_save) tex->m_texture.Save(str, true);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rt0_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
	if(s_save) rt->m_texture.Save(str);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rz0_%05x_%d.bmp"), s_n-1, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);
	if(s_savez) m_dev.SaveToFileD32S8X24(ds->m_texture, str); // TODO
}

	//

	int prim = PRIM->PRIM;
	int prims = 0;

	if(!OverrideInput(prim, tex))
	{
		return;
	}

	D3D10_PRIMITIVE_TOPOLOGY topology;

	switch(prim)
	{
	case GS_POINTLIST:
		topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		prims = m_count;
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
	case GS_SPRITE:
		topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
		prims = m_count / 2;
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
		topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		prims = m_count / 3;
		break;
	default:
		__assume(0);
	}

	m_perfmon.Put(GSPerfMon::Prim, prims);
	m_perfmon.Put(GSPerfMon::Draw, 1);

	// date

	SetupDATE(rt, ds);

	// color wrap

	GSTexture10 rt2;
/*
	if(m_env.COLCLAMP.CLAMP == 0) // color wrap is a crime against humanity!
	{
		m_dev.CreateRenderTarget(rt2, rt->m_texture.GetWidth(), rt->m_texture.GetHeight(), DXGI_FORMAT_R16G16B16A16_FLOAT);

		GSVector4 dr(0, 0, rt->m_texture.GetWidth(), rt->m_texture.GetHeight());

		m_dev.StretchRect(rt->m_texture, rt2, dr, false);
	}
*/
	//

	m_dev.BeginScene();

	// om

	GSTextureFX10::OMDepthStencilSelector om_dssel;

	om_dssel.zte = m_context->TEST.ZTE;
	om_dssel.ztst = m_context->TEST.ZTST;
	om_dssel.zwe = !m_context->ZBUF.ZMSK;
	om_dssel.date = m_context->TEST.DATE;

	GSTextureFX10::OMBlendSelector om_bsel;

	om_bsel.abe = PRIM->ABE || (PRIM->PRIM == 1 || PRIM->PRIM == 2) && PRIM->AA1;
	om_bsel.a = m_context->ALPHA.A;
	om_bsel.b = m_context->ALPHA.B;
	om_bsel.c = m_context->ALPHA.C;
	om_bsel.d = m_context->ALPHA.D;
	om_bsel.wr = (m_context->FRAME.FBMSK & 0x000000ff) != 0x000000ff;
	om_bsel.wg = (m_context->FRAME.FBMSK & 0x0000ff00) != 0x0000ff00;
	om_bsel.wb = (m_context->FRAME.FBMSK & 0x00ff0000) != 0x00ff0000;
	om_bsel.wa = (m_context->FRAME.FBMSK & 0xff000000) != 0xff000000;

	float factor = (float)(int)m_context->ALPHA.FIX / 0x80;

	m_tfx.SetupOM(om_dssel, om_bsel, factor, rt2 ? rt2 : rt->m_texture, ds->m_texture);

	// ia

	m_tfx.SetupIA(m_vertices, m_count, topology);

	// vs

	GSTextureFX10::VSConstantBuffer vs_cb;

	float sx = 2.0f * rt->m_scale.x / (rt->m_texture.GetWidth() * 16);
	float sy = 2.0f * rt->m_scale.y / (rt->m_texture.GetHeight() * 16);
	float ox = (float)(int)m_context->XYOFFSET.OFX;
	float oy = (float)(int)m_context->XYOFFSET.OFY;

	vs_cb.VertexScale = GSVector4(sx, -sy, 1.0f / UINT_MAX, 0);
	vs_cb.VertexOffset = GSVector4(ox * sx + 1, -(oy * sy + 1), 0, -1);
	vs_cb.TextureScale = GSVector2(1.0f, 1.0f);

	if(PRIM->TME && PRIM->FST)
	{
		vs_cb.TextureScale.x = 1.0f / (16 << m_context->TEX0.TW);
		vs_cb.TextureScale.y = 1.0f / (16 << m_context->TEX0.TH);
	}

	m_tfx.SetupVS(&vs_cb);

	// gs

	GSTextureFX10::GSSelector gs_sel;

	gs_sel.iip = PRIM->IIP;

	switch(prim)
	{
	case GS_POINTLIST:
		gs_sel.prim = 0;
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
		gs_sel.prim = 1;
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
		gs_sel.prim = 2;
		break;
	case GS_SPRITE:
		gs_sel.prim = 3;
		break;
	default:
		__assume(0);
	}	

	m_tfx.SetupGS(gs_sel);

	// ps

	GSTextureFX10::PSSelector ps_sel;

	ps_sel.fst = PRIM->FST;
	ps_sel.wms = m_context->CLAMP.WMS;
	ps_sel.wmt = m_context->CLAMP.WMT;
	ps_sel.bpp = 0;
	ps_sel.aem = m_env.TEXA.AEM;
	ps_sel.tfx = m_context->TEX0.TFX;
	ps_sel.tcc = m_context->TEX0.TCC;
	ps_sel.ate = m_context->TEST.ATE;
	ps_sel.atst = m_context->TEST.ATST;
	ps_sel.fog = PRIM->FGE;
	ps_sel.clr1 = om_bsel.abe && om_bsel.a == 1 && om_bsel.b == 2 && om_bsel.d == 1;
	ps_sel.fba = m_context->FBA.FBA;
	ps_sel.aout = m_context->FRAME.PSM == PSM_PSMCT16 || m_context->FRAME.PSM == PSM_PSMCT16S || (m_context->FRAME.FBMSK & 0xff000000) == 0x7f000000 ? 1 : 0;

	GSTextureFX10::PSSamplerSelector ps_ssel;

	ps_ssel.min = m_filter == 2 ? (m_context->TEX1.MMIN & 1) : m_filter;
	ps_ssel.mag = m_filter == 2 ? (m_context->TEX1.MMAG & 1) : m_filter;
	ps_ssel.tau = 0;
	ps_ssel.tav = 0;

	GSTextureFX10::PSConstantBuffer ps_cb;

	ps_cb.FogColor = GSVector4((float)(int)m_env.FOGCOL.FCR / 255, (float)(int)m_env.FOGCOL.FCG / 255, (float)(int)m_env.FOGCOL.FCB / 255, 0);
	ps_cb.TA0 = (float)(int)m_env.TEXA.TA0 / 255;
	ps_cb.TA1 = (float)(int)m_env.TEXA.TA1 / 255;
	ps_cb.AREF = (float)(int)m_context->TEST.AREF / 255;

	if(m_context->TEST.ATST == 2 || m_context->TEST.ATST == 5)
	{
		ps_cb.AREF -= 0.9f/256;
	}
	else if(m_context->TEST.ATST == 3 || m_context->TEST.ATST == 6)
	{
		ps_cb.AREF += 0.9f/256;
	}

	if(tex)
	{
		ps_sel.bpp = tex->m_bpp2;

		switch(m_context->CLAMP.WMS)
		{
		case 0: 
			ps_ssel.tau = 1; 
			break;
		case 1: 
			ps_ssel.tau = 0; 
			break;
		case 2: 
			ps_cb.MINU = ((float)(int)m_context->CLAMP.MINU + 0.5f) / (1 << m_context->TEX0.TW);
			ps_cb.MAXU = ((float)(int)m_context->CLAMP.MAXU) / (1 << m_context->TEX0.TW);
			ps_ssel.tau = 0; 
			break;
		case 3: 
			ps_cb.UMSK = m_context->CLAMP.MINU;
			ps_cb.UFIX = m_context->CLAMP.MAXU;
			ps_ssel.tau = 1; 
			break;
		default: 
			__assume(0);
		}

		switch(m_context->CLAMP.WMT)
		{
		case 0: 
			ps_ssel.tav = 1; 
			break;
		case 1: 
			ps_ssel.tav = 0; 
			break;
		case 2: 
			ps_cb.MINV = ((float)(int)m_context->CLAMP.MINV + 0.5f) / (1 << m_context->TEX0.TH);
			ps_cb.MAXV = ((float)(int)m_context->CLAMP.MAXV) / (1 << m_context->TEX0.TH);
			ps_ssel.tav = 0; 
			break;
		case 3: 
			ps_cb.VMSK = m_context->CLAMP.MINV;
			ps_cb.VFIX = m_context->CLAMP.MAXV;
			ps_ssel.tav = 1; 
			break;
		default: 
			__assume(0);
		}

		float w = (float)tex->m_texture.GetWidth();
		float h = (float)tex->m_texture.GetHeight();

		ps_cb.WH = GSVector2(w, h);
		ps_cb.rWrH = GSVector2(1.0f / w, 1.0f / h);

		m_tfx.SetupPS(ps_sel, &ps_cb, ps_ssel, tex->m_texture, tex->m_palette);
	}
	else
	{
		ps_sel.tfx = 4;

		m_tfx.SetupPS(ps_sel, &ps_cb, ps_ssel, NULL, NULL);
	}

	// rs

	int w = rt->m_texture.GetWidth();
	int h = rt->m_texture.GetHeight();

	CRect scissor(
		(int)(rt->m_scale.x * (m_context->SCISSOR.SCAX0)),
		(int)(rt->m_scale.y * (m_context->SCISSOR.SCAY0)), 
		(int)(rt->m_scale.x * (m_context->SCISSOR.SCAX1 + 1)),
		(int)(rt->m_scale.y * (m_context->SCISSOR.SCAY1 + 1)));

	scissor &= CRect(0, 0, w, h);

	m_tfx.SetupRS(w, h, scissor);

	// draw

	if(m_context->TEST.DoFirstPass())
	{
		m_dev.DrawPrimitive();
	}

	if(m_context->TEST.DoSecondPass())
	{
		ASSERT(!m_env.PABE.PABE);

		static const DWORD iatst[] = {1, 0, 5, 6, 7, 2, 3, 4};

		ps_sel.atst = iatst[ps_sel.atst];

		m_tfx.UpdatePS(ps_sel, &ps_cb, ps_ssel);

		bool z = om_dssel.zwe;
		bool r = om_bsel.wr;
		bool g = om_bsel.wg;
		bool b = om_bsel.wb;
		bool a = om_bsel.wa;

		switch(m_context->TEST.AFAIL)
		{
		case 0: z = r = g = b = a = false; break; // none
		case 1: z = false; break; // rgba
		case 2: r = g = b = a = false; break; // z
		case 3: z = a = false; break; // rgb
		default: __assume(0);
		}

		if(z || r || g || b || a)
		{
			om_dssel.zwe = z;
			om_bsel.wr = r;
			om_bsel.wg = g;
			om_bsel.wb = b;
			om_bsel.wa = a;

			m_tfx.UpdateOM(om_dssel, om_bsel, factor);

			m_dev.DrawPrimitive();
		}
	}

	m_dev.EndScene();
/*
	if(rt2)
	{
		GSVector4 sr(0, 0, 1, 1);
		GSVector4 dr(0, 0, rt->m_texture.GetWidth(), rt->m_texture.GetHeight());

		m_dev.StretchRect(rt2, sr, rt->m_texture, dr, m_dev.m_convert.ps[4], false);

		m_dev.Recycle(rt2);
	}
*/
if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rt1_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
	if(s_save) rt->m_texture.Save(str);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rz1_%05x_%d.bmp"), s_n-1, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);
	if(s_savez) m_dev.SaveToFileD32S8X24(ds->m_texture, str); // TODO
}

}

void GSRendererHW10::SetupDATE(GSTextureCache<GSDevice10>::GSRenderTarget* rt, GSTextureCache<GSDevice10>::GSDepthStencil* ds)
{
	if(!m_context->TEST.DATE) return; // || (::GetAsyncKeyState(VK_CONTROL)&0x80000000)

	// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows, rumble roses shadows

	int w = rt->m_texture.GetWidth();
	int h = rt->m_texture.GetHeight();

	float sx = 2.0f * rt->m_scale.x / (w * 16);
	float sy = 2.0f * rt->m_scale.y / (h * 16);
	
	float ox = (float)(int)m_context->XYOFFSET.OFX;
	float oy = (float)(int)m_context->XYOFFSET.OFY;

	GSVector4 mm;

	MinMaxXY(mm);

	mm.x = (mm.x - ox) * sx - 1;
	mm.y = (mm.y - oy) * sy - 1;
	mm.z = (mm.z - ox) * sx - 1;
	mm.w = (mm.w - oy) * sy - 1;

	if(mm.x < -1) mm.x = -1;
	if(mm.y < -1) mm.y = -1;
	if(mm.z > +1) mm.z = +1;
	if(mm.w > +1) mm.w = +1;

	GSVector4 uv;

	uv.x = (mm.x + 1) / 2;
	uv.y = (mm.y + 1) / 2;
	uv.z = (mm.z + 1) / 2;
	uv.w = (mm.w + 1) / 2;

	//

	m_dev.BeginScene();

	// om

	GSTexture10 tmp;

	m_dev.CreateRenderTarget(tmp, rt->m_texture.GetWidth(), rt->m_texture.GetHeight());

	m_dev.OMSetRenderTargets(tmp, ds->m_texture);
	m_dev.OMSetDepthStencilState(m_date.dss, 1);
	m_dev.OMSetBlendState(m_date.bs, 0);

	m_dev->ClearDepthStencilView(ds->m_texture, D3D10_CLEAR_STENCIL, 0, 0);

	// ia

	GSVertexPT1 vertices[] =
	{
		{GSVector4(mm.x, -mm.y), GSVector2(uv.x, uv.y)},
		{GSVector4(mm.z, -mm.y), GSVector2(uv.z, uv.y)},
		{GSVector4(mm.x, -mm.w), GSVector2(uv.x, uv.w)},
		{GSVector4(mm.z, -mm.w), GSVector2(uv.z, uv.w)},
	};

	m_dev.IASetVertexBuffer(m_dev.m_convert.vb, 4, vertices);
	m_dev.IASetInputLayout(m_dev.m_convert.il);
	m_dev.IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// vs

	m_dev.VSSetShader(m_dev.m_convert.vs, NULL);

	// gs

	m_dev.GSSetShader(NULL);

	// ps

	m_dev.PSSetShaderResources(rt->m_texture, NULL);
	m_dev.PSSetShader(m_dev.m_convert.ps[m_context->TEST.DATM ? 2 : 3], NULL);
	m_dev.PSSetSamplerState(m_dev.m_convert.pt);

	// rs

	m_dev.RSSet(tmp.GetWidth(), tmp.GetHeight());

	// set

	m_dev.DrawPrimitive();

	//

	m_dev.EndScene();

	m_dev.Recycle(tmp);
}

bool GSRendererHW10::OverrideInput(int& prim, GSTextureCache<GSDevice10>::GSTexture* tex)
{
	#pragma region ffxii pal video conversion

	if(m_crc == 0x78da0252 || m_crc == 0xc1274668 || m_crc == 0xdc2a467e || m_crc == 0xca284668)
	{
		static DWORD* video = NULL;
		static bool ok = false;

		if(prim == GS_POINTLIST && m_count >= 448*448 && m_count <= 448*512)
		{
			// incoming pixels are stored in columns, one column is 16x512, total res 448x512 or 448x454

			if(!video) video = new DWORD[512*512];

			for(int x = 0, i = 0, rows = m_count / 448; x < 448; x += 16)
			{
				DWORD* dst = &video[x];

				for(int y = 0; y < rows; y++, dst += 512)
				{
					for(int j = 0; j < 16; j++, i++)
					{
						dst[j] = m_vertices[i].c0;
					}
				}
			}

			ok = true;

			return false;
		}
		else if(prim == GS_LINELIST && m_count == 512*2 && ok)
		{
			// normally, this step would copy the video onto screen with 512 texture mapped horizontal lines,
			// but we use the stored video data to create a new texture, and replace the lines with two triangles

			ok = false;

			m_dev.Recycle(tex->m_texture);
			m_dev.Recycle(tex->m_palette);

			m_dev.CreateTexture(tex->m_texture, 512, 512);

			tex->m_texture.Update(CRect(0, 0, 448, 512), video, 512*4);

			m_vertices[0] = m_vertices[0];
			m_vertices[1] = m_vertices[m_count - 1];

			prim = GS_SPRITE;
			m_count = 2;

			return true;
		}
	}

	#pragma endregion

	return true;
}

