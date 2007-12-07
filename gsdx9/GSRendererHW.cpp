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
#include "GSUtil.h"
#include "resource.h"

inline BYTE SCALE_ALPHA(BYTE a) 
{
	return (((a)&0x80)?0xff:((a)<<1));
}

static const D3DVERTEXELEMENT9 s_vertexdecl[] =
{
	{0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
	{0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
	{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()
};

GSRendererHW::GSRendererHW()
	: m_tc(this)
	, m_width(1024)
	, m_height(1024)
	, m_hVertexShaderParams("g_params")
{
	if(!AfxGetApp()->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE))
	{
		m_width = AfxGetApp()->GetProfileInt(_T("Settings"), _T("resx"), 1024);
		m_height = AfxGetApp()->GetProfileInt(_T("Settings"), _T("resy"), 1024);
	}
}

GSRendererHW::~GSRendererHW()
{
}

bool GSRendererHW::Create(LPCTSTR title)
{
	if(!__super::Create(title))
		return false;

	HRESULT hr;

	if(!IsDepthFormatOk(m_d3d, D3DFMT_D24S8, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8))
	{
		return false;
	}

	// vs

	hr = m_dev->CreateVertexDeclaration(s_vertexdecl, &m_pVertexDeclaration);

	DWORD flags = 0;
	LPCTSTR target = NULL;

	if(m_caps.VertexShaderVersion >= D3DVS_VERSION(3, 0)) target = _T("vs_3_0");
	else if(m_caps.VertexShaderVersion >= D3DVS_VERSION(2, 0)) target = _T("vs_2_0");
	else return false;

	hr = CompileShaderFromResource(m_dev, IDR_TFX_FX, _T("vs_main"), target, flags, &m_pVertexShader, &m_pVertexShaderConstantTable);

	if(FAILED(hr)) return false;

	m_hVertexShaderParams = m_pVertexShaderConstantTable->GetConstantByName(NULL, m_hVertexShaderParams);

	// ps
/*
	// TODO: no need to precompile these shaders, could be created on-demand during the game

//	CompileTFX(_T("c:\\ps20_tfx"), _T("ps_2_0"), D3DXSHADER_PARTIALPRECISION);
//	CompileTFX(_T("c:\\ps30_tfx"), _T("ps_3_0"), D3DXSHADER_PARTIALPRECISION|D3DXSHADER_AVOID_FLOW_CONTROL);

	const DWORD** ps_tfx = NULL;

	if(m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
	{
		ps_tfx = ps_3_0_tfx;
	}
	else if(m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
	{
		ps_tfx = ps_2_0_tfx;
	}
	else 
	{
		return false;
	}

	for(int i = 0; i < 0x500; i++)
	{
		int tfx = (i >> 8) & 7;
		int bpp = (i >> 6) & 3;
		int tcc = (i >> 5) & 1;
		int aem = (i >> 4) & 1;
		int fog = (i >> 3) & 1;
		int rt = (i >> 2) & 1;
		int fst = (i >> 1) & 1;
		int clamp = (i >> 0) & 1;

		hr = m_dev->CreatePixelShader(ps_tfx[i], &m_pPixelShader[tfx][bpp][tcc][aem][fog][rt][fst][clamp]);

		if(FAILED(hr)) return false;
	}
*/
	//

	m_tc.Create();

	return true;
}

void GSRendererHW::ResetState()
{
	m_tc.RemoveAll();

	__super::ResetState();
}

HRESULT GSRendererHW::ResetDevice(bool fForceWindowed)
{
	m_tc.RemoveAll();

	return __super::ResetDevice(fForceWindowed);
}

static const float s_one_over_log_2pow32 = 1.0f / (log(2.0f)*32);

void GSRendererHW::VertexKick(bool skip)
{
	GSVertexHW& v = m_vl.AddTail();

	v.x = (float)m_v.XYZ.X;
	v.y = (float)m_v.XYZ.Y;
	//if(m_v.XYZ.Z && m_v.XYZ.Z < 0x100) m_v.XYZ.Z = 0x100;
	//v.z = 1.0f * (m_v.XYZ.Z>>8)/(UINT_MAX>>8);
	v.z = log(1.0f + m_v.XYZ.Z) * s_one_over_log_2pow32;
	//v.z = (float)m_v.XYZ.Z / UINT_MAX;

	v.color = m_v.RGBAQ.ai32[0];

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.w = 1.0f;
			v.tu = (float)(int)m_v.UV.U;
			v.tv = (float)(int)m_v.UV.V;
		}
		else
		{
			v.w = m_v.RGBAQ.Q;
			v.tu = m_v.ST.S;
			v.tv = m_v.ST.T;
		}
	}
	else
	{
		v.w = 1.0f;
		v.tu = 0;
		v.tv = 0;
	}

	v.fog = m_v.FOG.ai32[1];

	__super::VertexKick(skip);
}

void GSRendererHW::DrawingKick(bool skip)
{
	GSVertexHW* v = &m_pVertices[m_nVertices];
	int nv = 0;

	switch(m_pPRIM->PRIM)
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
		// ASSERT(v[0].z == v[1].z);
		v[0].z = v[1].z;
		v[0].w = v[1].w;
		v[2] = v[1];
		v[3] = v[1];
		v[1].y = v[0].y;
		v[1].tv = v[0].tv;
		v[2].x = v[0].x;
		v[2].tu = v[0].tu;
		v[4] = v[1];
		v[5] = v[2];
		nv = 6;
		break;
	default:
		//ASSERT(0);
		m_vl.RemoveAll();
		// return;
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
		if(v[0].x < sx0
		|| v[0].x > sx1
		|| v[0].y < sy0
		|| v[0].y > sy1)
			return;
		break;
	case 2:
		if(v[0].x < sx0 && v[1].x < sx0
		|| v[0].x > sx1 && v[1].x > sx1
		|| v[0].y < sy0 && v[1].y < sy0
		|| v[0].y > sy1 && v[1].y > sy1)
			return;
		break;
	case 3:
		if(v[0].x < sx0 && v[1].x < sx0 && v[2].x < sx0
		|| v[0].x > sx1 && v[1].x > sx1 && v[2].x > sx1
		|| v[0].y < sy0 && v[1].y < sy0 && v[2].y < sy0
		|| v[0].y > sy1 && v[1].y > sy1 && v[2].y > sy1)
			return;
		break;
	case 6:
		if(v[0].x < sx0 && v[3].x < sx0
		|| v[0].x > sx1 && v[3].x > sx1
		|| v[0].y < sy0 && v[3].y < sy0
		|| v[0].y > sy1 && v[3].y > sy1)
			return;
		break;
	default:
		__assume(0);
	}

	if(!m_pPRIM->IIP)
	{
		v[0].color = v[nv - 1].color;

		if(m_pPRIM->PRIM == 6)
		{
			v[3].color = v[5].color;
		}
	}

	m_nVertices += nv;
}

int s_n = 0;
bool s_dump = false;
bool s_save = true;

void GSRendererHW::FlushPrim()
{
	FlushPrimInternal();

	__super::FlushPrim();
}

void GSRendererHW::Flip()
{
	FlipInfo src[2];

	for(int i = 0; i < countof(src); i++)
	{
		if(!m_regs.IsEnabled(i))
		{
			continue;
		}

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = m_regs.pDISPFB[i]->Block();
		TEX0.TBW = m_regs.pDISPFB[i]->FBW;
		TEX0.PSM = m_regs.pDISPFB[i]->PSM;

		if(GSTextureCache::GSRenderTarget* rt = m_tc.GetRenderTarget(TEX0, m_width, m_height, true))
		{
			src[i].tex = rt->m_texture;
			src[i].desc = rt->m_desc;
			src[i].scale = rt->m_scale;
/*
CString str;

str.Format(_T("%d %05x %d | %d %d | %d x %d | %.2f %.2f\n"), 
	i, m_regs.pDISPFB[i]->Block(), m_regs.pDISPFB[i]->FBW * 64,
	m_regs.pDISPFB[i]->DBX, m_regs.pDISPFB[i]->DBY, 
	m_regs.GetDisplaySize(i).cx, m_regs.GetDisplaySize(i).cy, 				
	rt->m_scale.x, rt->m_scale.y);

TRACE(_T("%s"), str);
*/
if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_fr%d_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), i, (int)TEX0.TBP0, (int)TEX0.PSM);
	if(s_save) ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, rt->m_texture, NULL);
}

// s_dump = m_perfmon.GetFrame() >= 5000;

		}
	}

	FinishFlip(src);

	m_tc.IncAge();
}

void GSRendererHW::InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r)
{
	TRACE(_T("[%d] InvalidateTexture %d,%d - %d,%d %05x\n"), (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.DBP);

	m_tc.InvalidateTexture(BITBLTBUF, &r);
}

void GSRendererHW::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r)
{
	TRACE(_T("[%d] InvalidateLocalMem %d,%d - %d,%d %05x\n"), (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.SBP);

	m_tc.InvalidateLocalMem(BITBLTBUF, &r);
}

void GSRendererHW::MinMaxUV(int w, int h, CRect& r)
{
	r.SetRect(0, 0, w, h);

	uvmm_t uv;
	CSize bsm;

	if(m_context->CLAMP.WMS < 3 || m_context->CLAMP.WMT < 3)
	{
		uv.umin = uv.vmin = 0;
		uv.umax = uv.vmax = 1;

		if(m_pPRIM->FST)
		{
			UVMinMax(m_nVertices, (vertex_t*)m_pVertices, &uv);

			uv.umin *= 1.0f / (16 << m_context->TEX0.TW);
			uv.umax *= 1.0f / (16 << m_context->TEX0.TW);
			uv.vmin *= 1.0f / (16 << m_context->TEX0.TH);
			uv.vmax *= 1.0f / (16 << m_context->TEX0.TH);
		}
		else
		{
			// FIXME

			if(m_nVertices > 0)// && m_nVertices < 100)
			{
				uv.umin = uv.vmin = +1e10;
				uv.umax = uv.vmax = -1e10;

				for(int i = 0, j = m_nVertices; i < j; i++)
				{
					float w = 1.0f / m_pVertices[i].w;
					float u = m_pVertices[i].tu * w;
					if(uv.umax < u) uv.umax = u;
					if(uv.umin > u) uv.umin = u;
					float v = m_pVertices[i].tv * w;
					if(uv.vmax < v) uv.vmax = v;
					if(uv.vmin > v) uv.vmin = v;
				}
			}
		}

		CSize bs = GSLocalMemory::m_psmtbl[m_context->TEX0.PSM].bs;

		bsm.SetSize(bs.cx-1, bs.cy-1);
	}

	if(m_context->CLAMP.WMS < 3)
	{
		if(m_context->CLAMP.WMS == 0)
		{
			float fmin = floor(uv.umin);
			float fmax = floor(uv.umax);

			if(fmin != fmax) {uv.umin = 0; uv.umax = 1.0f;}
			else {uv.umin -= fmin; uv.umax -= fmax;}

			// FIXME: 
			if(uv.umin == 0 && uv.umax != 1.0f) uv.umax = 1.0f;
		}
		else if(m_context->CLAMP.WMS == 1)
		{
			if(uv.umin < 0) uv.umin = 0;
			else if(uv.umin > 1.0f) uv.umin = 1.0f;
			if(uv.umax < 0) uv.umax = 0;
			else if(uv.umax > 1.0f) uv.umax = 1.0f;
			if(uv.umin > uv.umax) uv.umin = uv.umax;
		}
		else if(m_context->CLAMP.WMS == 2)
		{
			float minu = 1.0f * m_context->CLAMP.MINU / w;
			float maxu = 1.0f * m_context->CLAMP.MAXU / w;
			if(uv.umin < minu) uv.umin = minu;
			else if(uv.umin > maxu) uv.umin = maxu;
			if(uv.umax < minu) uv.umax = minu;
			else if(uv.umax > maxu) uv.umax = maxu;
			if(uv.umin > uv.umax) uv.umin = uv.umax;
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

			// FIXME: 
			if(uv.vmin == 0 && uv.vmax != 1.0f) uv.vmax = 1.0f;
		}
		else if(m_context->CLAMP.WMT == 1)
		{
			if(uv.vmin < 0) uv.vmin = 0;
			else if(uv.vmin > 1.0f) uv.vmin = 1.0f;
			if(uv.vmax < 0) uv.vmax = 0;
			else if(uv.vmax > 1.0f) uv.vmax = 1.0f;
			if(uv.vmin > uv.vmax) uv.vmin = uv.vmax;
		}
		else if(m_context->CLAMP.WMT == 2)
		{
			float minv = 1.0f * m_context->CLAMP.MINV / h;
			float maxv = 1.0f * m_context->CLAMP.MAXV / h;
			if(uv.vmin < minv) uv.vmin = minv;
			else if(uv.vmin > maxv) uv.vmin = maxv;
			if(uv.vmax < minv) uv.vmax = minv;
			else if(uv.vmax > maxv) uv.vmax = maxv;
			if(uv.vmin > uv.vmax) uv.vmin = uv.vmax;
		}
		
		r.top = max((int)(uv.vmin * h) & ~bsm.cy, 0);
		r.bottom = min(((int)(uv.vmax * h) + bsm.cy + 1) & ~bsm.cy, h);
	}

	//ASSERT(r.left <= r.right);
	//ASSERT(r.top <= r.bottom);
}

void GSRendererHW::FlushPrimInternal()
{
	HRESULT hr;

	if(m_nVertices == 0)
	{
		return;
	}
/*
if(s_n >= 5500)
{
	// DebugBreak();
	s_save = true;
}
*/
	if(m_pPRIM->TME)
	{
		if(HasSharedBits(m_context->TEX0.TBP0, m_context->TEX0.PSM, m_context->FRAME.Block(), m_context->FRAME.PSM))
		{
			return;
		}

		// FIXME: depth textures (bully, mgs3s1 intro)

		if(m_context->TEX0.PSM == PSM_PSMZ32 || m_context->TEX0.PSM == PSM_PSMZ24
		|| m_context->TEX0.PSM == PSM_PSMZ16 || m_context->TEX0.PSM == PSM_PSMZ16S)
		{
			return;
		}

	}

	D3DPRIMITIVETYPE prim;

	int nPrims = 0;

	switch(m_pPRIM->PRIM)
	{
	case GS_POINTLIST:
		prim = D3DPT_POINTLIST;
		nPrims = m_nVertices;
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
		prim = D3DPT_LINELIST;
		nPrims = m_nVertices / 2; 
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
	case GS_SPRITE:
		prim = D3DPT_TRIANGLELIST;
		nPrims = m_nVertices / 3; 
		break;
	default:
		__assume(0);
	}

	m_perfmon.Put(GSPerfMon::Prim, nPrims);

	m_perfmon.Put(GSPerfMon::Draw, 1);

/**/
TRACE(_T("[%d] FlushPrim f %05x (%d) z %05x (%d %d %d %d) t %05x (%d) p %d\n"), 
	  (int)m_perfmon.GetFrame(), 
	  (int)m_context->FRAME.Block(), 
	  (int)m_context->FRAME.PSM, 
	  (int)m_context->ZBUF.Block(), 
	  (int)m_context->ZBUF.PSM, 
	  m_context->TEST.ZTE, 
	  m_context->TEST.ZTST, 
	  m_context->ZBUF.ZMSK, 
	  m_pPRIM->TME ? (int)m_context->TEX0.TBP0 : 0xfffff, 
	  m_pPRIM->TME ? (int)m_context->TEX0.PSM : 0xff, 
	  nPrims);

/**/
	// rt + ds

	GSTextureCache::GSRenderTarget* rt = NULL;
	GSTextureCache::GSDepthStencil* ds = NULL;

	GIFRegTEX0 TEX0;

	TEX0.TBP0 = m_context->FRAME.Block();
	TEX0.TBW = m_context->FRAME.FBW;
	TEX0.PSM = m_context->FRAME.PSM;

	rt = m_tc.GetRenderTarget(TEX0, m_width, m_height);

	TEX0.TBP0 = m_context->ZBUF.Block();
	TEX0.TBW = m_context->FRAME.FBW;
	TEX0.PSM = m_context->ZBUF.PSM;

	ds = m_tc.GetDepthStencil(TEX0, m_width, m_height);

	// tex

	GSTextureCache::GSTexture* tex = NULL;

	if(m_pPRIM->TME && !(tex = m_tc.GetTextureNP()))
	{
		return;
	}

	//

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_tex_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), (int)m_context->TEX0.TBP0, (int)m_context->TEX0.PSM);
	if(m_pPRIM->TME) if(s_save) ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, tex->m_texture, NULL);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rt0_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
	if(s_save) ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, rt->m_texture, NULL);
}

	if(!SetupHacks(prim, nPrims, tex))
	{
		return;
	}

	SetupDestinationAlphaTest(rt, ds);

	hr = m_dev->BeginScene();

	hr = m_dev->SetRenderTarget(0, rt->m_surface);
	hr = m_dev->SetDepthStencilSurface(ds->m_surface);

	hr = m_dev->SetRenderState(D3DRS_SHADEMODE, m_pPRIM->IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT);

	SetupVertexShader(rt);

	SetupTexture(tex);

	SetupAlphaBlend();

	SetupColorMask();

	SetupZBuffer();

	SetupAlphaTest();

	SetupScissor(rt->m_scale);

	SetupFrameBufferAlpha();

	if(!m_context->TEST.ATE || m_context->TEST.ATST != 0)
	{
		hr = m_dev->DrawPrimitiveUP(prim, nPrims, m_pVertices, sizeof(GSVertexHW));
	}

	if(m_context->TEST.ATE && m_context->TEST.ATST != 1 && m_context->TEST.AFAIL)
	{
		ASSERT(!m_env.PABE.PABE);

		static const DWORD iafunc[] = {D3DCMP_ALWAYS, D3DCMP_NEVER, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL};

		hr = m_dev->SetRenderState(D3DRS_ALPHAFUNC, iafunc[m_context->TEST.ATST]);
		hr = m_dev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_context->TEST.AREF));

		DWORD mask = 0;
		bool zwrite = false;

		hr = m_dev->GetRenderState(D3DRS_COLORWRITEENABLE, &mask);

		switch(m_context->TEST.AFAIL)
		{
		case 0: mask = 0; break; // keep
		case 1: break; // fbuf
		case 2: mask = 0; zwrite = !m_context->ZBUF.ZMSK; break; // zbuf
		case 3: mask &= ~D3DCOLORWRITEENABLE_ALPHA; break; // fbuf w/o alpha
		default: __assume(0);
		}

		hr = m_dev->SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
		hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

		if(mask || zwrite)
		{
			hr = m_dev->DrawPrimitiveUP(prim, nPrims, m_pVertices, sizeof(GSVertexHW));
		}
	}

	hr = m_dev->EndScene();

	UpdateFrameBufferAlpha(rt);

	hr = m_dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rt1_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
	if(s_save) ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, rt->m_texture, NULL);
}

}

void GSRendererHW::SetupVertexShader(const GSTextureCache::GSRenderTarget* rt)
{
	HRESULT hr;

	hr = m_dev->SetVertexDeclaration(m_pVertexDeclaration);

	hr = m_dev->SetVertexShader(m_pVertexShader);

	float sx = 2.0f * rt->m_scale.x / (rt->m_desc.Width * 16);
	float sy = 2.0f * rt->m_scale.y / (rt->m_desc.Height * 16);
	float ox = (float)m_context->XYOFFSET.OFX;
	float oy = (float)m_context->XYOFFSET.OFY;

	D3DXVECTOR4 params[] = 
	{
		D3DXVECTOR4(sx, -sy, 1, 0),
		D3DXVECTOR4(ox * sx + 1, -(oy * sy + 1), 0, -1),
		D3DXVECTOR4(1.0f, 1.0f, 0, 0),
	};

	if(m_pPRIM->TME && m_pPRIM->FST)
	{
		params[2][0] = 1.0f / (16 << m_context->TEX0.TW);
		params[2][1] = 1.0f / (16 << m_context->TEX0.TH);
	}

	hr = m_pVertexShaderConstantTable->SetVectorArray(m_dev, m_hVertexShaderParams, params, countof(params));
}

void GSRendererHW::SetupTexture(const GSTextureCache::GSTexture* t)
{
	ASSERT(m_env.COLCLAMP.CLAMP == 1);

	HRESULT hr;

	int tw = 0, th = 0;
	float rw = 0, rh = 0;
	float cl = -4096, ct = -4096, cr = +4096, cb = +4096;

	int tfx = m_context->TEX0.TFX;
	int bpp = 0;
	int tcc = m_context->TEX0.TCC;
	int aem = m_env.TEXA.AEM;
	int fog = m_pPRIM->FGE;
	int rt = t && t->IsRenderTarget();
	int fst = m_pPRIM->FST;
	int clamp = 0;

	if(m_pPRIM->TME && t && t->m_texture)
	{
		switch(t->m_desc.Format)
		{
		case D3DFMT_A8R8G8B8:
			bpp = 0;
			if(t->m_palette && m_context->TEX0.PSM == PSM_PSMT8H) bpp = 3;
			break;
		case D3DFMT_X8R8G8B8:
			bpp = 1;
			break;
		case D3DFMT_A1R5G5B5:
			bpp = 2;
			break;
		default: 
			ASSERT(0);
			break;
		}

		if(m_context->CLAMP.WMS == 2)
		{
			cl = (float)m_context->CLAMP.MINU / (1 << m_context->TEX0.TW);
			cr = (float)m_context->CLAMP.MAXU / (1 << m_context->TEX0.TW);
			clamp = 1;
		}

		if(m_context->CLAMP.WMT == 2)
		{
			ct = (float)m_context->CLAMP.MINV / (1 << m_context->TEX0.TH);
			cb = (float)m_context->CLAMP.MAXV / (1 << m_context->TEX0.TH);
			clamp = 1;
		}

		hr = m_dev->SetTexture(0, t->m_texture);
		hr = m_dev->SetTexture(1, t->m_palette);

		tw = t->m_desc.Width;
		th = t->m_desc.Height;
		rw = 1.0f / tw;
		rh = 1.0f / th;

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

		hr = m_dev->SetSamplerState(0, D3DSAMP_ADDRESSU, u);
		hr = m_dev->SetSamplerState(0, D3DSAMP_ADDRESSV, v);

		DWORD magf = 0;
		DWORD minf = 0;

		switch(m_filter)
		{
		case 0:
			magf = D3DTEXF_POINT;
			minf = D3DTEXF_POINT;
			break;
		case 1:
			magf = t->m_palette ? D3DTEXF_POINT : D3DTEXF_LINEAR;
			minf = t->m_palette ? D3DTEXF_POINT : D3DTEXF_LINEAR;
			break;
		default:
			magf = (m_context->TEX1.MMAG & 1) == 0 || t->m_palette ? D3DTEXF_POINT : D3DTEXF_LINEAR;
			minf = (m_context->TEX1.MMIN & 1) == 0 || t->m_palette ? D3DTEXF_POINT : D3DTEXF_LINEAR;
			break;
		}

		hr = m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, magf);
		hr = m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, minf);
		hr = m_dev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		hr = m_dev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	}
	else
	{
		tfx = 4;

		hr = m_dev->SetTexture(0, NULL);
		hr = m_dev->SetTexture(1, NULL);
	}

	float fConstData[][4] = 
	{
		{min(2.0f * m_env.TEXA.TA0 / 255, 1), min(2.0f * m_env.TEXA.TA1 / 255, 1), 0, 0},
		{(float)m_env.FOGCOL.FCB / 255, (float)m_env.FOGCOL.FCG / 255, (float)m_env.FOGCOL.FCR / 255 , 0},
		{(float)tw, (float)th, 0, 0},
		{rw, rh, 0, 0},
		{rw, 0, 0, 0},
		{0, rh, 0, 0},
		{cl, ct, 0, 0},
		{cr, cb, 0, 0},
	};

	hr = m_dev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

	if(!m_pPixelShader[tfx][bpp][tcc][aem][fog][rt][fst][clamp])
	{
		DWORD flags = 0;//D3DXSHADER_PARTIALPRECISION;
		LPCTSTR target = _T("ps_2_0");

		if(m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0)) 
		{
			target = _T("ps_3_0");
			flags |= D3DXSHADER_AVOID_FLOW_CONTROL;
		}

		CompileTFX(m_dev, &m_pPixelShader[tfx][bpp][tcc][aem][fog][rt][fst][clamp], target, flags, tfx, bpp, tcc, aem, fog, rt, fst, clamp);
	}

	hr = m_dev->SetPixelShader(m_pPixelShader[tfx][bpp][tcc][aem][fog][rt][fst][clamp]);
}

void GSRendererHW::SetupAlphaBlend()
{
	// ASSERT(!m_env.PABE.PABE); // bios

//static __int64 s_frame = 0;
//if(m_env.PABE.PABE && s_frame != m_perfmon.GetFrame()) {printf("PABE\n"); s_frame = m_perfmon.GetFrame();}

	HRESULT hr;

	bool ABE = m_pPRIM->ABE || (m_pPRIM->PRIM == 1 || m_pPRIM->PRIM == 2) && m_pPRIM->AA1; // FIXME

	hr = m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, ABE);

	if(!ABE) return;

	// (A:Cs/Cd/0 - B:Cs/Cd/0) * C:As/Ad/FIX + D:Cs/Cd/0

	BYTE FIX = SCALE_ALPHA(m_context->ALPHA.FIX);

	hr = m_dev->SetRenderState(D3DRS_BLENDFACTOR, (0x010101*FIX) | (FIX<<24));

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
		{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// * 1201: (Cd - 0)*As + Cd ==> Cd*(1 + As)  // ffxii main menu background glow effect
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

	hr = m_dev->SetRenderState(D3DRS_BLENDOP, blendmap[i].op);
	hr = m_dev->SetRenderState(D3DRS_SRCBLEND, blendmap[i].src);
	hr = m_dev->SetRenderState(D3DRS_DESTBLEND, blendmap[i].dst);

	if(blendmap[i].bogus)
	{
		ASSERT(0);

		if(m_context->ALPHA.A == 0)
		{
			hr = m_dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		}
		else
		{
			hr = m_dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		}
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
	hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

	// ASSERT(m_context->FRAME.FBMSK == 0); // wild arms (also 8H+pal on RT...)
}

void GSRendererHW::SetupZBuffer()
{
	HRESULT hr;

	if(m_context->TEST.ZTE && m_context->TEST.ZTST == 1 && m_context->ZBUF.ZMSK)
	{
		hr = m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		return;
	}

	hr = m_dev->SetRenderState(D3DRS_ZENABLE, m_context->TEST.ZTE);
	hr = m_dev->SetRenderState(D3DRS_ZWRITEENABLE, !m_context->ZBUF.ZMSK);

	if(m_context->TEST.ZTE)
	{
		static const DWORD zfunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_GREATEREQUAL, D3DCMP_GREATER};

		hr = m_dev->SetRenderState(D3DRS_ZFUNC, zfunc[m_context->TEST.ZTST]);
	}
}

void GSRendererHW::SetupAlphaTest()
{
	HRESULT hr = m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, m_context->TEST.ATE);

	if(m_context->TEST.ATE)
	{
		static const DWORD afunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL};

		hr = m_dev->SetRenderState(D3DRS_ALPHAFUNC, afunc[m_context->TEST.ATST]);
		hr = m_dev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_context->TEST.AREF));
	}
}

void GSRendererHW::SetupDestinationAlphaTest(const GSTextureCache::GSRenderTarget* rt, const GSTextureCache::GSDepthStencil* ds)
{
	// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows

	HRESULT hr;
	
	hr = m_dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	if(m_context->TEST.DATE) // need to render each prim one-by-one for this
	{
//static __int64 s_frame = 0;
//if(s_frame != m_perfmon.GetFrame()) {printf("DATE\n"); s_frame = m_perfmon.GetFrame();}
		CComPtr<IDirect3DTexture9> texture;
		CComPtr<IDirect3DSurface9> surface;

		hr = m_tc.CreateRenderTarget(rt->m_desc.Width, rt->m_desc.Height, &texture, &surface);

		hr = m_dev->SetRenderTarget(0, surface);
		hr = m_dev->SetDepthStencilSurface(ds->m_surface);

		hr = m_dev->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, 0);

		hr = m_dev->SetTexture(0, rt->m_texture);
		hr = m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		hr = m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		hr = m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		hr = m_dev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		hr = m_dev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
		hr = m_dev->SetRenderState(D3DRS_STENCILREF, 1);
		hr = m_dev->SetRenderState(D3DRS_STENCILMASK, 1);
		hr = m_dev->SetRenderState(D3DRS_STENCILWRITEMASK, 1);	
		hr = m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		hr = m_dev->SetRenderState(D3DRS_ALPHAFUNC, m_context->TEST.DATM ? D3DCMP_EQUAL : D3DCMP_LESS);
		hr = m_dev->SetRenderState(D3DRS_ALPHAREF, 0xff);
		hr = m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);

		hr = m_dev->SetVertexShader(NULL);
		hr = m_dev->SetPixelShader(m_tc.m_ps[0]);

		struct
		{
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] =
		{
			{0, 0, 0.5f, 2.0f, 0, 0},
			{(float)rt->m_desc.Width, 0, 0.5f, 2.0f, 1, 0},
			{0, (float)rt->m_desc.Height, 0.5f, 2.0f, 0, 1},
			{(float)rt->m_desc.Width, (float)rt->m_desc.Height, 0.5f, 2.0f, 1, 1},
		};

		hr = m_dev->BeginScene();
		hr = m_dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_dev->EndScene();

		hr = m_dev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		hr = m_dev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		hr = m_dev->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		hr = m_dev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);

		m_tc.Recycle(surface);
	}
}

void GSRendererHW::SetupScissor(const GSScale& scale)
{
	HRESULT hr = m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	CRect r(
		(int)(scale.x * m_context->SCISSOR.SCAX0),
		(int)(scale.y * m_context->SCISSOR.SCAY0), 
		(int)(scale.x * (m_context->SCISSOR.SCAX1 + 1)),
		(int)(scale.y * (m_context->SCISSOR.SCAY1 + 1)));

	r &= CRect(0, 0, m_width, m_height);

	hr = m_dev->SetScissorRect(r);
}

void GSRendererHW::SetupFrameBufferAlpha()
{
	HRESULT hr;
	
	if(m_context->FBA.FBA=0)
	{
//static __int64 s_frame = 0;
//if(s_frame != m_perfmon.GetFrame()) {printf("FBA\n"); s_frame = m_perfmon.GetFrame();}
		if(m_context->TEST.DATE)
		{
			ASSERT(0); // can't do both at the same time

			return;
		}
/*
		if(m_context->TEST.ATE && m_context->TEST.ATST != 1 && m_context->TEST.AFAIL == 0)
		{
			// if not all pixels will reach the stencil test then clear it

			hr = m_dev->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 0, 0);
		}
*/
		hr = m_dev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		hr = m_dev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		hr = m_dev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
		hr = m_dev->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_ZERO);
		hr = m_dev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_ZERO);
		hr = m_dev->SetRenderState(D3DRS_STENCILREF, 2);
		hr = m_dev->SetRenderState(D3DRS_STENCILMASK, 2);
		hr = m_dev->SetRenderState(D3DRS_STENCILWRITEMASK, 2);	
	}
}

void GSRendererHW::UpdateFrameBufferAlpha(const GSTextureCache::GSRenderTarget* rt)
{
	HRESULT hr;
	
	if(m_context->FBA.FBA)
	{
		if(m_context->TEST.DATE)
		{
			ASSERT(0); // can't do both at the same time

			return;
		}

		hr = m_dev->SetTexture(0, NULL);
		hr = m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		hr = m_dev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		hr = m_dev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
		hr = m_dev->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_ZERO);
		hr = m_dev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_ZERO);
		hr = m_dev->SetRenderState(D3DRS_STENCILREF, 2);
		hr = m_dev->SetRenderState(D3DRS_STENCILMASK, 2);
		hr = m_dev->SetRenderState(D3DRS_STENCILWRITEMASK, 2);	
		hr = m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

		hr = m_dev->SetVertexShader(NULL);
		hr = m_dev->SetPixelShader(m_tc.m_ps[2]);

		struct
		{
			float x, y, z, rhw;
		}
		pVertices[] =
		{
			{0, 0, 0.5f, 2.0f},
			{(float)rt->m_desc.Width, 0, 0.5f, 2.0f},
			{0, (float)rt->m_desc.Height, 0.5f, 2.0f},
			{(float)rt->m_desc.Width, (float)rt->m_desc.Height, 0.5f, 2.0f},
		};

		hr = m_dev->BeginScene();
		hr = m_dev->SetFVF(D3DFVF_XYZRHW);
		hr = m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = m_dev->EndScene();
	}
/*
	if(m_context->FRAME.PSM == PSM_PSMCT16 || m_context->FRAME.PSM == PSM_PSMCT16S)
	{
		CComPtr<IDirect3DTexture9> texture;
		CComPtr<IDirect3DSurface9> surface;

		hr = m_tc.CreateRenderTarget(rt->m_desc.Width, rt->m_desc.Height, &texture, &surface);

		hr = m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		hr = m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		hr = m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		hr = m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

		hr = m_dev->SetVertexShader(NULL);

		struct
		{
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] =
		{
			{0, 0, 0.5f, 2.0f, 0, 0},
			{(float)rt->m_desc.Width, 0, 0.5f, 2.0f, 1, 0},
			{0, (float)rt->m_desc.Height, 0.5f, 2.0f, 0, 1},
			{(float)rt->m_desc.Width, (float)rt->m_desc.Height, 0.5f, 2.0f, 1, 1},
		};

		hr = m_dev->BeginScene();

		hr = m_dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

		hr = m_dev->SetRenderTarget(0, surface);
		hr = m_dev->SetTexture(0, rt->m_texture);
		hr = m_dev->SetPixelShader(m_tc.m_ps[3]);

		hr = m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		hr = m_dev->SetRenderTarget(0, rt->m_surface);
		hr = m_dev->SetTexture(0, texture);
		hr = m_dev->SetPixelShader(m_tc.m_ps[0]);

		hr = m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		hr = m_dev->EndScene();

		m_tc.Recycle(surface);
	}
*/
}

bool GSRendererHW::SetupHacks(D3DPRIMITIVETYPE& prim, int& count, GSTextureCache::GSTexture* tex)
{
	#pragma region ffxii pal video conversion

	if(m_crc == 0x78DA0252 || m_crc == 0xC1274668 || m_crc == 0xDC2A467E)
	{
		static DWORD* video = NULL;
		static bool ok = false;

		if(prim == D3DPT_POINTLIST && count >= 448*448 && count <= 448*512)
		{
			// incoming pixels are stored in columns, one column is 16x512, total res 448x512 or 448x454

			if(!video) video = new DWORD[512*512];

			int i = 0;

			int rows = count / 448;

			for(int x = 0; x < 448; x += 16)
			{
				DWORD* dst = &video[x];

				for(int y = 0; y < rows; y++, dst += 512)
				{
					for(int j = 0; j < 16; j++, i++)
					{
						dst[j] = m_pVertices[i].color;
					}
				}
			}

			ok = true;

			return false;
		}
		else if(prim == D3DPT_LINELIST && count == 512 && ok)
		{
			// normally, this step would copy the video onto screen with 512 texture mapped horizontal lines,
			// but we use the stored video data to create a new texture, and replace the lines with two triangles

			ok = false;

			m_tc.Recycle(tex->m_surface);

			tex->m_surface = NULL;
			tex->m_texture = NULL;

			if(SUCCEEDED(m_tc.CreateTexture(512, 512, D3DFMT_A8R8G8B8, &tex->m_texture, &tex->m_surface, &tex->m_desc)))
			{
				D3DLOCKED_RECT lr;
				
				if(SUCCEEDED(tex->m_surface->LockRect(&lr, NULL, 0)))
				{
					BYTE* bits = (BYTE*)lr.pBits;

					for(int i = 0; i < 512; i++, bits += lr.Pitch)
					{
						memcpy(bits, &video[i*512], 448*4);
					}

					tex->m_surface->UnlockRect();
				}
			}

			m_nVertices = 6;

			m_pVertices[0] = m_pVertices[0];
			m_pVertices[1] = m_pVertices[1];
			m_pVertices[2] = m_pVertices[m_nVertices - 2];
			m_pVertices[3] = m_pVertices[1];
			m_pVertices[4] = m_pVertices[2];
			m_pVertices[5] = m_pVertices[m_nVertices - 1];

			prim = D3DPT_TRIANGLELIST;
			count = 2;

			return true;
		}
	}

	#pragma endregion

	return true;
}
