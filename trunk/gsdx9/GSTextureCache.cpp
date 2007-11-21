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
#include "GSTextureCache.h"
#include "GSState.h"
#include "GSUtil.h"
#include "resource.h"

GSTextureCache::GSTextureCache(GSState* state)
	: m_state(state)
{
	m_nativeres = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE);
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();
}

void GSTextureCache::Create()
{
	if(m_state->m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
	{
		DWORD flags = D3DXSHADER_PARTIALPRECISION|D3DXSHADER_AVOID_FLOW_CONTROL;

		for(int i = 0; i < countof(m_ps); i++)
		{
			if(!m_ps[i])
			{
				CString main;
				main.Format(_T("main%d"), i);
				CompileShaderFromResource(m_state->m_dev, IDR_HLSL_TEXTURECACHE, main, _T("ps_3_0"), flags, &m_ps[i]);
			}
		}
	}

	// ps_2_0

	if(m_state->m_caps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
	{
		DWORD flags = D3DXSHADER_PARTIALPRECISION;

		for(int i = 0; i < countof(m_ps); i++)
		{
			if(!m_ps[i])
			{
				CString main;
				main.Format(_T("main%d"), i);
				CompileShaderFromResource(m_state->m_dev, IDR_HLSL_TEXTURECACHE, main, _T("ps_2_0"), flags, &m_ps[i]);
			}
		}
	}
}

void GSTextureCache::RemoveAll()
{
	while(m_rt.GetCount()) delete m_rt.RemoveHead();
	while(m_ds.GetCount()) delete m_ds.RemoveHead();
	while(m_tex.GetCount()) delete m_tex.RemoveHead();
	m_pool.RemoveAll();
}

GSTextureCache::GSRenderTarget* GSTextureCache::GetRenderTarget(const GIFRegTEX0& TEX0, int w, int h, bool fb)
{
	POSITION pos = m_tex.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = m_tex.GetNext(pos);

		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			m_tex.RemoveAt(cur);

			delete t;
		}
	}

	GSRenderTarget* rt = NULL;

	if(rt == NULL)
	{
		for(POSITION pos = m_rt.GetHeadPosition(); pos; m_rt.GetNext(pos))
		{
			GSRenderTarget* rt2 = m_rt.GetAt(pos);

			if(rt2->m_TEX0.TBP0 == TEX0.TBP0)
			{
				m_rt.MoveToHead(pos);

				rt = rt2;

				if(!fb) rt->m_TEX0 = TEX0;

				rt->Update();

				break;
			}
		}
	}

	if(rt == NULL && fb)
	{
		// HACK: try to find something close to the base pointer

		for(POSITION pos = m_rt.GetHeadPosition(); pos; m_rt.GetNext(pos))
		{
			GSRenderTarget* rt2 = m_rt.GetAt(pos);

			if(rt2->m_TEX0.TBP0 <= TEX0.TBP0 && TEX0.TBP0 < rt2->m_TEX0.TBP0 + 0xe00 && (!rt || rt2->m_TEX0.TBP0 >= rt->m_TEX0.TBP0))
			{
				rt = rt2;
			}
		}

		if(rt)
		{
			rt->Update();
		}
	}

	if(rt == NULL)
	{
		rt = new GSRenderTarget(this);

		rt->m_TEX0 = TEX0;

		if(!rt->Create(w, h))
		{
			delete rt;

			return NULL;
		}

		m_rt.AddHead(rt);
	}

	if(!m_nativeres)
	{
		rt->m_scale.x = (float)w / (m_state->m_regs.GetFramePos().cx + rt->m_TEX0.TBW * 64);
		rt->m_scale.y = (float)h / (m_state->m_regs.GetFramePos().cy + m_state->m_regs.GetDisplaySize().cy);
	}

	if(!fb)
	{
		rt->m_used = true;
	}

	return rt;
}

GSTextureCache::GSDepthStencil* GSTextureCache::GetDepthStencil(const GIFRegTEX0& TEX0, int w, int h)
{
	POSITION pos = m_tex.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = m_tex.GetNext(pos);

		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			m_tex.RemoveAt(cur);

			delete t;
		}
	}

	GSDepthStencil* ds = NULL;

	if(ds == NULL)
	{
		for(POSITION pos = m_ds.GetHeadPosition(); pos; m_ds.GetNext(pos))
		{
			GSDepthStencil* ds2 = m_ds.GetAt(pos);

			if(ds2->m_TEX0.TBP0 == TEX0.TBP0)
			{
				m_ds.MoveToHead(pos);

				ds = ds2;

				ds->m_TEX0 = TEX0;

				ds->Update();

				break;
			}
		}
	}

	if(ds == NULL)
	{
		ds = new GSDepthStencil(this);

		ds->m_TEX0 = TEX0;

		if(!ds->Create(w, h))
		{
			delete ds;

			return NULL;
		}

		m_ds.AddHead(ds);
	}

	return ds;
}

GSTextureCache::GSTexture* GSTextureCache::GetTextureNP()
{
	const GIFRegTEX0& TEX0 = m_state->m_context->TEX0;
	const GIFRegCLAMP& CLAMP = m_state->m_context->CLAMP;
	const GIFRegTEXA& TEXA = m_state->m_env.TEXA;

	DWORD clut[256];

	int pal = GSLocalMemory::m_psmtbl[TEX0.PSM].pal;

	if(pal > 0)
	{
		m_state->m_mem.SetupCLUT(TEX0, TEXA);
		m_state->m_mem.CopyCLUT32(clut, pal);
	}

	GSTexture* t = NULL;

	for(POSITION pos = m_tex.GetHeadPosition(); pos; m_tex.GetNext(pos))
	{
		t = m_tex.GetAt(pos);

		if(HasSharedBits(t->m_TEX0.TBP0, t->m_TEX0.PSM, TEX0.TBP0, TEX0.PSM))
		{
			if(TEX0.PSM == t->m_TEX0.PSM && TEX0.TBW == t->m_TEX0.TBW
			&& TEX0.TW == t->m_TEX0.TW && TEX0.TH == t->m_TEX0.TH
			&& (CLAMP.WMS != 3 && t->m_CLAMP.WMS != 3 && CLAMP.WMT != 3 && t->m_CLAMP.WMT != 3 || CLAMP.i64 == t->m_CLAMP.i64)
			// && TEXA.TA0 == t->m_TEXA.TA0 && TEXA.TA1 == t->m_TEXA.TA1 && TEXA.AEM == t->m_TEXA.AEM
			&& (pal == 0 || TEX0.CPSM == t->m_TEX0.CPSM && !memcmp(t->m_clut, clut, pal * sizeof(clut[0]))))
			{
				m_tex.MoveToHead(pos);

				break;
			}
		}

		t = NULL;
	}

	if(t == NULL)
	{
		for(POSITION pos = m_rt.GetHeadPosition(); pos; m_rt.GetNext(pos))
		{
			GSRenderTarget* rt = m_rt.GetAt(pos);

			if(rt->m_dirty.IsEmpty() && HasSharedBits(rt->m_TEX0.TBP0, rt->m_TEX0.PSM, TEX0.TBP0, TEX0.PSM))
			{
				t = new GSTexture(this);

				if(!t->Create(rt))
				{
					delete t;

					return NULL;
				}

				m_tex.AddHead(t);

				break;
			}
		}
	}

	if(t == NULL)
	{
		for(POSITION pos = m_ds.GetHeadPosition(); pos; m_ds.GetNext(pos))
		{
			GSDepthStencil* ds = m_ds.GetAt(pos);

			if(ds->m_dirty.IsEmpty() && HasSharedBits(ds->m_TEX0.TBP0, ds->m_TEX0.PSM, TEX0.TBP0, TEX0.PSM))
			{
				t = new GSTexture(this);

				if(!t->Create(ds))
				{
					delete t;

					return NULL;
				}

				m_tex.AddHead(t);

				break;
			}
		}
	}

	if(t == NULL)
	{
		t = new GSTexture(this);

		if(!t->Create())
		{
			delete t;

			return NULL;
		}

		m_tex.AddHead(t);
	}

	if(pal > 0)
	{
		int size = pal * sizeof(clut[0]);

		memcpy(t->m_clut, clut, size);

		if(t->m_palette) 
		{
			D3DLOCKED_RECT r;

			if(SUCCEEDED(t->m_palette->LockRect(0, &r, NULL, 0)))
			{
				memcpy(r.pBits, t->m_clut, size);

				t->m_palette->UnlockRect(0);
				
				m_state->m_perfmon.Put(GSPerfMon::Texture, size);
			}
		}
	}

	t->Update(&GSLocalMemory::ReadTextureNP);

	return t;
}

void GSTextureCache::InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
{
	POSITION pos = m_tex.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = m_tex.GetNext(pos);

		if(HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			if(BITBLTBUF.DBW == t->m_TEX0.TBW)
			{
				t->m_dirty.AddTail(GSDirtyRect(BITBLTBUF.DPSM, r));
			}
			else
			{
				m_tex.RemoveAt(cur);

				delete t;
			}
		}
	}

	pos = m_rt.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSRenderTarget* rt = m_rt.GetNext(pos);

		if(HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, rt->m_TEX0.TBP0, rt->m_TEX0.PSM))
		{
			if(BITBLTBUF.DPSM == PSM_PSMCT32 
			|| BITBLTBUF.DPSM == PSM_PSMCT24 
			|| BITBLTBUF.DPSM == PSM_PSMCT16 
			|| BITBLTBUF.DPSM == PSM_PSMCT16S
			|| BITBLTBUF.DPSM == PSM_PSMZ32 
			|| BITBLTBUF.DPSM == PSM_PSMZ24 
			|| BITBLTBUF.DPSM == PSM_PSMZ16 
			|| BITBLTBUF.DPSM == PSM_PSMZ16S)
			{
				rt->m_dirty.AddTail(GSDirtyRect(BITBLTBUF.DPSM, r));
				rt->m_TEX0.TBW = BITBLTBUF.DBW;
			}
			else
			{
				m_rt.RemoveAt(cur);

				delete rt;

				continue;
			}
		}

		if(HasSharedBits(BITBLTBUF.DPSM, rt->m_TEX0.PSM) && BITBLTBUF.DBP < rt->m_TEX0.TBP0)
		{
			DWORD rowsize = BITBLTBUF.DBW * 8192;
			DWORD offset = (rt->m_TEX0.TBP0 - BITBLTBUF.DBP) * 256;

			if(rowsize > 0 && offset % rowsize == 0)
			{
				int y = m_state->m_mem.m_psmtbl[BITBLTBUF.DPSM].pgs.cy * offset / rowsize;

				if(r.top >= y)
				{
					// TODO: do not add this rect above too
					rt->m_dirty.AddTail(GSDirtyRect(BITBLTBUF.DPSM, CRect(r.left, r.top - y, r.right, r.bottom - y)));
					rt->m_TEX0.TBW = BITBLTBUF.DBW;
					continue;
				}
			}
		}
	}

	// copypaste for ds

	pos = m_ds.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSDepthStencil* ds = m_ds.GetNext(pos);

		if(HasSharedBits(BITBLTBUF.DBP, BITBLTBUF.DPSM, ds->m_TEX0.TBP0, ds->m_TEX0.PSM))
		{
			if(BITBLTBUF.DPSM == PSM_PSMCT32 
			|| BITBLTBUF.DPSM == PSM_PSMCT24 
			|| BITBLTBUF.DPSM == PSM_PSMCT16 
			|| BITBLTBUF.DPSM == PSM_PSMCT16S
			|| BITBLTBUF.DPSM == PSM_PSMZ32 
			|| BITBLTBUF.DPSM == PSM_PSMZ24 
			|| BITBLTBUF.DPSM == PSM_PSMZ16 
			|| BITBLTBUF.DPSM == PSM_PSMZ16S)
			{
				ds->m_dirty.AddTail(GSDirtyRect(BITBLTBUF.DPSM, r));
				ds->m_TEX0.TBW = BITBLTBUF.DBW;
			}
			else
			{
				m_ds.RemoveAt(cur);

				delete ds;

				continue;
			}
		}

		if(HasSharedBits(BITBLTBUF.DPSM, ds->m_TEX0.PSM) && BITBLTBUF.DBP < ds->m_TEX0.TBP0)
		{
			DWORD rowsize = BITBLTBUF.DBW * 8192;
			DWORD offset = (ds->m_TEX0.TBP0 - BITBLTBUF.DBP) * 256;

			if(rowsize > 0 && offset % rowsize == 0)
			{
				int y = m_state->m_mem.m_psmtbl[BITBLTBUF.DPSM].pgs.cy * offset / rowsize;

				if(r.top >= y)
				{
					// TODO: do not add this rect above too
					ds->m_dirty.AddTail(GSDirtyRect(BITBLTBUF.DPSM, CRect(r.left, r.top - y, r.right, r.bottom - y)));
					ds->m_TEX0.TBW = BITBLTBUF.DBW;
					continue;
				}
			}
		}
	}
}

void GSTextureCache::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
{
	POSITION pos = m_rt.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSRenderTarget* rt = m_rt.GetNext(pos);

		if(HasSharedBits(BITBLTBUF.SBP, BITBLTBUF.SPSM, rt->m_TEX0.TBP0, rt->m_TEX0.PSM))
		{
			rt->Read(r);
			break;
		}
	}
}

void GSTextureCache::IncAge()
{
	RecycleByAge(m_tex);
	RecycleByAge(m_rt);
	RecycleByAge(m_ds);
}

template<class T> void GSTextureCache::RecycleByAge(CAtlList<T*>& l, int maxage)
{
	POSITION pos = l.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		T* t = l.GetNext(pos);

		if(++t->m_age >= maxage)
		{
			l.RemoveAt(cur);

			delete t;
		}
	}
}

void GSTextureCache::Recycle(IDirect3DSurface9* surface)
{
	if(surface)
	{
		m_pool.AddHead(surface);

		while(m_pool.GetCount() > 100)
		{			
			m_pool.RemoveTail();
		}
	}
}

HRESULT GSTextureCache::CreateRenderTarget(int w, int h, IDirect3DTexture9** ppt, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
{
	HRESULT hr;

	for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
	{
		CComPtr<IDirect3DSurface9> s = m_pool.GetAt(pos);

		CComPtr<IDirect3DTexture9> t;

		if(SUCCEEDED(s->GetContainer(__uuidof(IDirect3DTexture9), (void**)&t)))
		{
			D3DSURFACE_DESC desc;
			
			hr = t->GetLevelDesc(0, &desc);

			if(desc.Pool == D3DPOOL_DEFAULT && (desc.Usage & D3DUSAGE_RENDERTARGET) && desc.Width == w && desc.Height == h)
			{
				*ppt = t.Detach();

				m_pool.RemoveAt(pos);

				break;
			}
		}
	}

	if(*ppt == NULL)
	{
		hr = m_state->m_dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, ppt, NULL);

		TRACE(_T("CreateRenderTarget %d x %d\n"), w, h);
	}

	if(pps) (*ppt)->GetSurfaceLevel(0, pps);

	if(desc) (*ppt)->GetLevelDesc(0, desc);

	return hr;
}

HRESULT GSTextureCache::CreateDepthStencil(int w, int h, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
{
	HRESULT hr;

	for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
	{
		CComPtr<IDirect3DSurface9> s = m_pool.GetAt(pos);

		D3DSURFACE_DESC desc;
			
		hr = s->GetDesc(&desc);

		if(desc.Pool == D3DPOOL_DEFAULT && (desc.Usage & D3DUSAGE_DEPTHSTENCIL) && desc.Width == w && desc.Height == h)
		{
			*pps = s.Detach();

			m_pool.RemoveAt(pos);

			break;
		}
	}

	if(*pps == NULL)
	{
		hr = m_state->m_dev->CreateDepthStencilSurface(w, h, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, pps, NULL);

		TRACE(_T("CreateDepthStencilSurface %d x %d\n"), w, h);
	}

	if(desc) (*pps)->GetDesc(desc);

	return hr;
}

HRESULT GSTextureCache::CreateTexture(int w, int h, D3DFORMAT format, IDirect3DTexture9** ppt, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
{
	HRESULT hr;

	for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
	{
		CComPtr<IDirect3DSurface9> s = m_pool.GetAt(pos);

		CComPtr<IDirect3DTexture9> t;

		if(SUCCEEDED(s->GetContainer(__uuidof(IDirect3DTexture9), (void**)&t)))
		{
			D3DSURFACE_DESC desc;
			
			hr = t->GetLevelDesc(0, &desc);

			if(desc.Pool == D3DPOOL_MANAGED && !(desc.Usage & D3DUSAGE_RENDERTARGET) && desc.Width == w && desc.Height == h && desc.Format == format)
			{
				*ppt = t.Detach();

				m_pool.RemoveAt(pos);

				break;
			}
		}
	}

	if(*ppt == NULL)
	{
		hr = m_state->m_dev->CreateTexture(w, h, 1, 0, format, D3DPOOL_MANAGED, ppt, NULL);

		TRACE(_T("CreateTexture %d x %d\n"), w, h);
	}

	if(pps) (*ppt)->GetSurfaceLevel(0, pps);

	if(desc) (*ppt)->GetLevelDesc(0, desc);

	return hr;
}

HRESULT GSTextureCache::CreateOffscreenPlainSurface(int w, int h, D3DFORMAT format, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
{
	HRESULT hr;

	for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
	{
		CComPtr<IDirect3DSurface9> s = m_pool.GetAt(pos);

		D3DSURFACE_DESC desc;
			
		hr = s->GetDesc(&desc);

		if(desc.Pool == D3DPOOL_SYSTEMMEM && desc.Width == w && desc.Height == h && desc.Format == format)
		{
			*pps = s.Detach();

			m_pool.RemoveAt(pos);

			break;
		}
	}

	if(*pps == NULL)
	{
		hr = m_state->m_dev->CreateOffscreenPlainSurface(w, h, format, D3DPOOL_SYSTEMMEM, pps, NULL);

		TRACE(_T("CreateOffscreenPlainSurface %d x %d\n"), w, h);
	}

	if(desc) (*pps)->GetDesc(desc);

	return hr;
}

