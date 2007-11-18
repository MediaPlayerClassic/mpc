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
#include "GSHash.h"
#include "GSUtil.h"


GSTextureCache::GSTextureCache()
{
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();
}

void GSTextureCache::RemoveAll()
{
	while(m_tex.GetCount()) delete m_tex.RemoveHead();
	while(m_rt.GetCount()) delete m_rt.RemoveHead();
	while(m_ds.GetCount()) delete m_ds.RemoveHead();
	m_pool.RemoveAll();
}

GSTextureCache::GSRenderTarget* GSTextureCache::GetRenderTarget(GSState* s, const GIFRegTEX0& TEX0, int w, int h, bool fb)
{
	POSITION pos = m_tex.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = m_tex.GetNext(pos);

		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			m_tex.RemoveAt(cur);

			Recycle(t);
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

				rt->Update(s, this);

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
			rt->Update(s, this);
		}
	}

	if(rt == NULL)
	{
		rt = new GSRenderTarget();

		rt->m_TEX0 = TEX0;

		if(!Create(s, rt, w, h))
		{
			delete rt;

			return NULL;
		}
	}

	rt->m_scale.x = (float)w / (s->m_regs.GetFramePos().cx + rt->m_TEX0.TBW * 64);
	rt->m_scale.y = (float)h / (s->m_regs.GetFramePos().cy + s->m_regs.GetDisplaySize().cy);

	return rt;
}

GSTextureCache::GSDepthStencil* GSTextureCache::GetDepthStencil(GSState* s, const GIFRegTEX0& TEX0, int w, int h)
{
	POSITION pos = m_tex.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = m_tex.GetNext(pos);

		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			m_tex.RemoveAt(cur);

			Recycle(t);
		}
	}

	for(POSITION pos = m_ds.GetHeadPosition(); pos; m_ds.GetNext(pos))
	{
		GSDepthStencil* ds = m_ds.GetAt(pos);

		if(ds->m_TEX0.TBP0 == TEX0.TBP0)
		{
			m_ds.MoveToHead(pos);

			ds->m_TEX0 = TEX0;

			ds->Update(s);

			return ds;
		}
	}

	GSDepthStencil* ds = new GSDepthStencil();

	ds->m_TEX0 = TEX0;

	if(!Create(s, ds, w, h))
	{
		delete ds;

		return NULL;
	}

	return ds;
}

GSTextureCache::GSTexture* GSTextureCache::GetTextureNP(GSState* s)
{
	DWORD clut[256];

	int pal = GSLocalMemory::m_psmtbl[s->m_context->TEX0.PSM].pal;

	if(pal > 0)
	{
		s->m_mem.SetupCLUT(s->m_context->TEX0, s->m_env.TEXA);
		s->m_mem.CopyCLUT32(clut, pal);
	}

	GSTexture* t = NULL;

	for(POSITION pos = m_tex.GetHeadPosition(); pos; m_tex.GetNext(pos))
	{
		t = m_tex.GetAt(pos);

		if(HasSharedBits(t->m_TEX0.TBP0, t->m_TEX0.PSM, s->m_context->TEX0.TBP0, s->m_context->TEX0.PSM))
		{
			if(s->m_context->TEX0.PSM == t->m_TEX0.PSM && s->m_context->TEX0.TBW == t->m_TEX0.TBW
			&& s->m_context->TEX0.TW == t->m_TEX0.TW && s->m_context->TEX0.TH == t->m_TEX0.TH
			&& (!(s->m_context->CLAMP.WMS & 2) && !(t->m_CLAMP.WMS & 2) && !(s->m_context->CLAMP.WMT & 2) && !(t->m_CLAMP.WMT & 2) || s->m_context->CLAMP.i64 == t->m_CLAMP.i64)
			// && s->m_env.TEXA.TA0 == t->m_TEXA.TA0 && s->m_env.TEXA.TA1 == t->m_TEXA.TA1 && s->m_env.TEXA.AEM == t->m_TEXA.AEM
			&& (pal == 0 || s->m_context->TEX0.CPSM == t->m_TEX0.CPSM && !memcmp(t->m_clut, clut, pal * sizeof(clut[0]))))
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

			if(HasSharedBits(rt->m_TEX0.TBP0, rt->m_TEX0.PSM, s->m_context->TEX0.TBP0, s->m_context->TEX0.PSM))
			{
				rt->Update(s, this);

				t = Convert(s, rt);

				break;
			}
		}
	}

	if(t == NULL)
	{
		t = new GSTexture();

		t->m_TEX0 = s->m_context->TEX0;
		t->m_CLAMP = s->m_context->CLAMP;
		// t->m_TEXA = s->m_env.TEXA;

		DWORD psm = s->m_context->TEX0.PSM;

		switch(psm)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			psm = s->m_context->TEX0.CPSM;
			break;
		}

		if(!Create(s, t, psm))
		{
			delete t;

			return NULL;
		}
	}

	if(pal > 0)
	{
		memcpy(t->m_clut, clut, pal * sizeof(clut[0]));

		if(t->m_palette) 
		{
			D3DLOCKED_RECT r;

			if(SUCCEEDED(t->m_palette->LockRect(0, &r, NULL, 0)))
			{
				int size = pal * sizeof(t->m_clut[0]);
				memcpy(r.pBits, t->m_clut, size);
				t->m_palette->UnlockRect(0);
				s->m_perfmon.Put(GSPerfMon::Texture, size);
			}
		}
	}

	t->Update(s, &GSLocalMemory::ReadTextureNP);

	return t;
}

void GSTextureCache::InvalidateTexture(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
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
				Recycle(t);
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
			|| BITBLTBUF.DPSM == PSM_PSMCT16S)
			{
				rt->m_dirty.AddTail(GSDirtyRect(BITBLTBUF.DPSM, r));
				rt->m_TEX0.TBW = BITBLTBUF.DBW;
			}
			else
			{
				m_rt.RemoveAt(cur);
				Recycle(rt);
				continue;
			}
		}

		if(HasSharedBits(BITBLTBUF.DPSM, rt->m_TEX0.PSM) && BITBLTBUF.DBP < rt->m_TEX0.TBP0)
		{
			DWORD rowsize = BITBLTBUF.DBW * 8192;
			DWORD offset = (rt->m_TEX0.TBP0 - BITBLTBUF.DBP) * 256;

			if(rowsize > 0 && offset % rowsize == 0)
			{
				int y = s->m_mem.m_psmtbl[BITBLTBUF.DPSM].pgs.cy * offset / rowsize;

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
}

void GSTextureCache::InvalidateLocalMem(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
{
	// TODO: 
	// 1. search render target
	// 2. swizzle data back to local mem
}

void GSTextureCache::IncAge()
{
	RecycleByAge(m_tex);
	RecycleByAge(m_rt);
	RecycleByAge(m_ds);
}

bool GSTextureCache::Create(GSState* s, GSRenderTarget* rt, int w, int h)
{
	HRESULT hr;

	RecycleBySize(m_rt);

	rt->m_size = w * h * 2;

	hr = CreateRenderTarget(s, w, h, &rt->m_texture, &rt->m_surface, &rt->m_desc);
	
	if(FAILED(hr)) return false;

	hr = s->m_dev->SetRenderTarget(0, rt->m_surface);
	hr = s->m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	m_rt.AddHead(rt);

	return true;
}

bool GSTextureCache::Create(GSState* s, GSDepthStencil* ds, int w, int h)
{
	HRESULT hr;

	RecycleBySize(m_ds);

	D3DFORMAT format;

	if(IsDepthFormatOk(s->m_d3d, D3DFMT_D32, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8))
	{
		format = D3DFMT_D32;
		ds->m_size = w * h * 4;
	}
	else if(IsDepthFormatOk(s->m_d3d, D3DFMT_D24X8, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8))
	{
		format = D3DFMT_D24X8;
		ds->m_size = w * h * 4;
	}
	else
	{
		format = D3DFMT_D16;
		ds->m_size = w * h * 2;
	}

	hr = CreateDepthStencil(s, w, h, &ds->m_surface, &ds->m_desc);

	m_ds.AddHead(ds);

	return true;
}

bool GSTextureCache::Create(GSState* s, GSTexture* t, DWORD psm, DWORD cpsm)
{
	HRESULT hr;

	RecycleBySize(m_tex);

	D3DFORMAT format;

	switch(psm)
	{
	default:
	case PSM_PSMCT32:
		t->m_bpp = 32;
		format = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT24:
		t->m_bpp = 32;
		format = D3DFMT_X8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		t->m_bpp = 16;
		format = D3DFMT_A1R5G5B5;
		break;
	case PSM_PSMT8:
	case PSM_PSMT4:
	case PSM_PSMT8H:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		t->m_bpp = 8;
		format = D3DFMT_L8;
		break;
	}

	int w = 1 << t->m_TEX0.TW;
	int h = 1 << t->m_TEX0.TH;

	t->m_size = w * h * t->m_bpp >> 3;

	hr = CreateTexture(s, w, h, format, &t->m_texture, &t->m_surface, &t->m_desc);

	if(FAILED(hr)) return false;

	if(t->m_bpp == 8)
	{
		format = cpsm == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5;

		hr = CreateTexture(s, 256, 1, format, &t->m_palette);

		if(FAILED(hr)) return false;

	}

	m_tex.AddHead(t);

	return true;
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
			Recycle(t);
		}
	}
}

template<class T> void GSTextureCache::RecycleBySize(CAtlList<T*>& l, int maxsize)
{
	POSITION pos = l.GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		T* t = l.GetNext(pos);

		if(t->m_size >= maxsize)
		{
			l.RemoveAt(cur);
			Recycle(t);
		}
	}
}

void GSTextureCache::Recycle(IDirect3DSurface9* surface)
{
	m_pool.AddHead(surface);

	while(m_pool.GetCount() > 20) m_pool.RemoveTail();
}

void GSTextureCache::Recycle(GSSurface* s, bool del)
{
	Recycle(s->m_surface);

	if(del) delete s;
}

void GSTextureCache::Recycle(GSTexture* t, bool del)
{
	if(t->m_palette)
	{
		CComPtr<IDirect3DSurface9> surface;

		t->m_palette->GetSurfaceLevel(0, &surface);

		m_pool.AddHead(surface);
	}

	Recycle((GSSurface*)t, del);
}

GSTextureCache::GSTexture* GSTextureCache::Convert(GSState* s, GSRenderTarget* rt)
{
	HRESULT hr;

	GSTextureCache::GSTexture* t = new GSTexture();

	t->m_scale = rt->m_scale;
	t->m_size = rt->m_size;
	t->m_TEX0 = s->m_context->TEX0;

	hr = CreateRenderTarget(s, rt->m_desc.Width, rt->m_desc.Height, &t->m_texture, &t->m_surface, &t->m_desc);

	if(t->m_TEX0.PSM == PSM_PSMT8H)
	{
		hr = CreateTexture(s, 256, 1, t->m_TEX0.CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5, &t->m_palette);
	}

	// pitch conversion

	if(rt->m_TEX0.TBW != t->m_TEX0.TBW)
	{
		// sfex3 uses this trick (bw: 10 -> 5, wraps the right side below the left)

		ASSERT(rt->m_TEX0.TBW > t->m_TEX0.TBW); // otherwise scale.x need to be reduced to make the larger texture fit (TODO)

		int bw = 64;
		int bh = t->m_TEX0.PSM == PSM_PSMCT32 || t->m_TEX0.PSM == PSM_PSMCT24 ? 32 : 64;

		int sw = rt->m_TEX0.TBW << 6;

		int dw = t->m_TEX0.TBW << 6;
		int dh = 1 << t->m_TEX0.TH;

		for(int dy = 0; dy < dh; dy += bh)
		{
			for(int dx = 0; dx < dw; dx += bw)
			{
				int o = dy * dw / bh + dx;

				int sx = o % sw;
				int sy = o / sw;

				CRect src, dst;

				src.left = (LONG)(t->m_scale.x * sx + 0.5f);
				src.top = (LONG)(t->m_scale.y * sy + 0.5f);
				src.right = (LONG)(t->m_scale.x * (sx + bw) + 0.5f);
				src.bottom = (LONG)(t->m_scale.y * (sy + bh) + 0.5f);

				dst.left = (LONG)(t->m_scale.x * dx + 0.5f);
				dst.top = (LONG)(t->m_scale.y * dy + 0.5f);
				dst.right = (LONG)(t->m_scale.x * (dx + bw) + 0.5f);
				dst.bottom = (LONG)(t->m_scale.y * (dy + bh) + 0.5f);

				hr = s->m_dev->StretchRect(rt->m_surface, src, t->m_surface, dst, D3DTEXF_POINT);

				// TODO: this is quite a lot of StretchRect call, do it with one DrawPrimUP
			}
		}
	}
	else
	{
		hr = s->m_dev->StretchRect(rt->m_surface, NULL, t->m_surface, NULL, D3DTEXF_LINEAR);
	}

	// width/height conversion

	int w = t->m_scale.x * (1 << t->m_TEX0.TW);
	int h = t->m_scale.y * (1 << t->m_TEX0.TH);

	int rw = t->m_desc.Width;
	int rh = t->m_desc.Height;

	if(w != rw || h != rh)
	{
		CRect dst(0, 0, w, h);
		
		if(w > rw)
		{
			float scale = t->m_scale.x;
			t->m_scale.x = (float)rw / (1 << t->m_TEX0.TW);
			dst.right = rw * t->m_scale.x / scale;
			w = rw;
		}
		
		if(h > rh)
		{
			float scale = t->m_scale.y;
			t->m_scale.y = (float)rh / (1 << t->m_TEX0.TH);
			dst.bottom = rh * t->m_scale.y / scale;
			h = rh;
		}

		CRect src(0, 0, w, h);

		CComPtr<IDirect3DTexture9> texture;
		CComPtr<IDirect3DSurface9> surface;

		hr = CreateRenderTarget(s, w, h, &texture, &surface, &t->m_desc);

		hr = s->m_dev->StretchRect(t->m_surface, src, surface, dst, src == dst ? D3DTEXF_POINT : D3DTEXF_LINEAR);

		Recycle(t, false);

		t->m_texture = texture;
		t->m_surface = surface;
	}

	//

	m_tex.AddHead(t);

	return t;
}

HRESULT GSTextureCache::CreateRenderTarget(GSState* s, int w, int h, IDirect3DTexture9** ppt, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
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

			if((desc.Usage & D3DUSAGE_RENDERTARGET) && desc.Width == w && desc.Height == h)
			{
				*ppt = t.Detach();

				m_pool.RemoveAt(pos);

				break;
			}
		}
	}

	if(*ppt == NULL)
	{
		hr = s->m_dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, ppt, NULL);
	}

	if(pps) (*ppt)->GetSurfaceLevel(0, pps);

	if(desc) (*ppt)->GetLevelDesc(0, desc);

	return hr;
}

HRESULT GSTextureCache::CreateDepthStencil(GSState* s, int w, int h, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
{
	HRESULT hr;

	D3DFORMAT format = D3DFMT_D16;

	if(IsDepthFormatOk(s->m_d3d, D3DFMT_D32, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8))
	{
		format = D3DFMT_D32;
	}
	else if(IsDepthFormatOk(s->m_d3d, D3DFMT_D24X8, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8))
	{
		format = D3DFMT_D24X8;
	}

	for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
	{
		CComPtr<IDirect3DSurface9> s = m_pool.GetAt(pos);

		D3DSURFACE_DESC desc;
			
		hr = s->GetDesc(&desc);

		if((desc.Usage & D3DUSAGE_DEPTHSTENCIL) && desc.Width == w && desc.Height == h && desc.Format == format)
		{
			*pps = s.Detach();

			m_pool.RemoveAt(pos);

			break;
		}
	}

	if(*pps == NULL)
	{
		hr = s->m_dev->CreateDepthStencilSurface(w, h, format, D3DMULTISAMPLE_NONE, 0, FALSE, pps, NULL);
	}

	if(desc) (*pps)->GetDesc(desc);

	return hr;
}

HRESULT GSTextureCache::CreateTexture(GSState* s, int w, int h, D3DFORMAT format, IDirect3DTexture9** ppt, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc)
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

			if(!(desc.Usage & D3DUSAGE_RENDERTARGET) && desc.Width == w && desc.Height == h && desc.Format == format)
			{
				*ppt = t.Detach();

				m_pool.RemoveAt(pos);

				break;
			}
		}
	}

	if(*ppt == NULL)
	{
		hr = s->m_dev->CreateTexture(w, h, 1, 0, format, D3DPOOL_MANAGED, ppt, NULL);
	}

	if(pps) (*ppt)->GetSurfaceLevel(0, pps);

	if(desc) (*ppt)->GetLevelDesc(0, desc);

	return hr;
}

//

GSTextureCache::GSSurface::GSSurface()
{
	memset(&m_desc, 0, sizeof(m_desc));
	m_scale = GSScale(1, 1);
	m_age = 0;
	m_size = 0;
	m_TEX0.TBP0 = ~0;
}

GSTextureCache::GSSurface::~GSSurface()
{
}

bool GSTextureCache::GSSurface::IsRenderTarget() const 
{
	return !!(m_desc.Usage & D3DUSAGE_RENDERTARGET);
}

bool GSTextureCache::GSSurface::IsDepthStencil() const 
{
	return !!(m_desc.Usage & D3DUSAGE_DEPTHSTENCIL);
}

void GSTextureCache::GSSurface::Update(GSState* s)
{
	m_age = 0;
}

//

GSTextureCache::GSTexture::GSTexture() 
{
	m_valid = CRect(0, 0, 0, 0);
	m_hash = ~0;
	m_hashdiff = 0;
	m_hashrect = CRect(0, 0, 0, 0);
	m_bpp = 0;
}

void GSTextureCache::GSTexture::Update(GSState* s, GSLocalMemory::readTexture rt)
{
	__super::Update(s);

	if(IsRenderTarget())
	{
		return;
	}

	CRect r;

	if(!GetDirtyRect(s, r))
	{
		return;
	}

	HRESULT hr;

	D3DLOCKED_RECT lr;

	if(SUCCEEDED(hr = m_texture->LockRect(0, &lr, &r, D3DLOCK_NO_DIRTY_UPDATE))) 
	{
		(s->m_mem.*rt)(r, (BYTE*)lr.pBits, lr.Pitch, s->m_context->TEX0, s->m_env.TEXA, s->m_context->CLAMP);

		m_texture->UnlockRect(0);

		s->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * m_bpp >> 3);

		m_valid |= r;
		m_dirty.RemoveAll();

		const static DWORD limit = 7;

		if((m_hashdiff & limit) && m_hashdiff >= limit && m_hashrect == m_valid) // predicted to be dirty
		{
			m_hashdiff++;
		}
		else
		{
			DWORD hash = Hash();

			if(m_hashrect != m_valid)
			{
				m_hashdiff = 0;
				m_hashrect = m_valid;
				m_hash = hash;
			}
			else
			{
				if(m_hash != hash)
				{
					m_hashdiff++;
					m_hash = hash;
				}
				else
				{
					if(m_hashdiff < limit) r.SetRect(0, 0, 1, 1);
					// else t->m_hash is not reliable, must update
					m_hashdiff = 0;
				}
			}
		}

		m_texture->AddDirtyRect(&r);
		
		m_texture->PreLoad();

		s->m_perfmon.Put(GSPerfMon::Texture, r.Width() * r.Height() * m_bpp >> 3);
	}
}

DWORD GSTextureCache::GSTexture::Hash()
{
	// TODO: make the hash more unique

	ASSERT(m_bpp != 0);

	DWORD hash = 0;

	D3DLOCKED_RECT lr;

	if(SUCCEEDED(m_texture->LockRect(0, &lr, &m_valid, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY))) 
	{
		CRect r = CRect((m_valid.left >> 2) * (m_bpp >> 3), m_valid.top, (m_valid.right >> 2) * (m_bpp >> 3), m_valid.bottom);

		hash = (r.left << 8) + (r.right << 12) + (r.top << 16) + (r.bottom << 20) + (lr.Pitch << 24) + *(BYTE*)lr.pBits;

		if(r.Width() > 0)
		{
			int size = r.Width() * r.Height();

			/* if(size <= 8*8) return rand(); // :P
			else */	if(size <= 16*16) hash += hash_crc(r, lr.Pitch, (BYTE*)lr.pBits);
			else if(size <= 32*32) hash += hash_adler(r, lr.Pitch, (BYTE*)lr.pBits);
			else hash += hash_checksum(r, lr.Pitch, (BYTE*)lr.pBits);
		}

		m_texture->UnlockRect(0);
	}
	
	return hash;
}

bool GSTextureCache::GSTexture::GetDirtyRect(GSState* s, CRect& r)
{
	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	r.SetRect(0, 0, w, h);

	// FIXME: kyo's left hand after being selected for player one (PS2-SNK_Vs_Capcom_SVC_Chaos_PAL_CDFull.iso)
	// return true;

	s->MinMaxUV(w, h, r);

	CRect dirty = m_dirty.GetDirtyRect(m_TEX0);
	CRect valid = m_valid;

	dirty &= CRect(0, 0, m_desc.Width, m_desc.Height);

	if(IsRectInRect(r, valid))
	{
		if(dirty.IsRectEmpty()) return false;
		else if(IsRectInRect(dirty, r)) r = dirty;
		else if(IsRectInRect(dirty, valid)) r |= dirty;
		else r = valid | dirty;
	}
	else if(IsRectInRectH(r, valid) && (r.left >= valid.left || r.right <= valid.right))
	{
		r.top = valid.top;
		r.bottom = valid.bottom;
		if(r.left < valid.left) r.right = valid.left;
		else /*if(r.right > valid.right)*/ r.left = valid.right;
	}
	else if(IsRectInRectV(r, valid) && (r.top >= valid.top || r.bottom <= valid.bottom))
	{
		r.left = valid.left;
		r.right = valid.right;
		if(r.top < valid.top) r.bottom = valid.top;
		else /*if(r.bottom > valid.bottom)*/ r.top = valid.bottom;
	}
	else
	{
		r |= valid;
	}

	return true;
}

//

GSTextureCache::GSRenderTarget::GSRenderTarget()
{
}

extern int s_n;
extern bool s_dump;

void GSTextureCache::GSRenderTarget::Update(GSState* s, GSTextureCache* tc)
{
	__super::Update(s);

	CRect r = m_dirty.GetDirtyRect(m_TEX0); // FIXME: the union of the rects may update wrong parts of the render target too

	if(!r.IsRectEmpty())
	{
/*
	POSITION pos = m_dirty.GetHeadPosition();

	while(pos)
	{
		CRect r = m_dirty.GetNext(pos).GetDirtyRect(m_TEX0);
*/
		TRACE(_T("%d,%d - %d,%d (%08x)\n"), r.left, r.top, r.right, r.bottom, m_TEX0.TBP0);

		HRESULT hr;

		int w = (r.Width() + 3) & ~3;
		int h = r.Height();

		CComPtr<IDirect3DTexture9> texture;
		CComPtr<IDirect3DSurface9> surface;
		
		hr = tc->CreateTexture(s, w, h, D3DFMT_A8R8G8B8, &texture, &surface);

		D3DLOCKED_RECT lr;

		if(texture && SUCCEEDED(texture->LockRect(0, &lr, NULL, 0)))
		{
			GIFRegTEXA TEXA;

			TEXA.AEM = 1;
			TEXA.TA0 = 0;
			TEXA.TA1 = 0x80;

			GIFRegCLAMP CLAMP;

			CLAMP.WMS = 0;
			CLAMP.WMT = 0;

			s->m_mem.ReadTexture(r, (BYTE*)lr.pBits, lr.Pitch, m_TEX0, TEXA, CLAMP);
			
			s->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * 4);

			texture->UnlockRect(0);

			//

			float left = m_scale.x * r.left;
			float top = m_scale.y * r.top;
			float right = m_scale.x * r.right;
			float bottom = m_scale.y * r.bottom;

			hr = s->m_dev->SetRenderTarget(0, m_surface);
			hr = s->m_dev->SetDepthStencilSurface(NULL);

			hr = s->m_dev->SetTexture(0, texture);
			hr = s->m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			hr = s->m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			hr = s->m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
			hr = s->m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			hr = s->m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			hr = s->m_dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			hr = s->m_dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			hr = s->m_dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			hr = s->m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
			hr = s->m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

			hr = s->m_dev->SetPixelShader(NULL);

			struct
			{
				float x, y, z, rhw;
				float tu, tv;
			}
			pVertices[] =
			{
				{(float)left, (float)top, 0.5f, 2.0f, 0, 0},
				{(float)right, (float)top, 0.5f, 2.0f, 1, 0},
				{(float)left, (float)bottom, 0.5f, 2.0f, 0, 1},
				{(float)right, (float)bottom, 0.5f, 2.0f, 1, 1},
			};

			hr = s->m_dev->BeginScene();
			hr = s->m_dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
			hr = s->m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
			hr = s->m_dev->EndScene();

			tc->Recycle(surface);
		}
/**/
if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_dw_%05x.bmp"), s_n++, s->m_perfmon.GetFrame(), m_TEX0.TBP0);
	::D3DXSaveTextureToFile(str, D3DXIFF_BMP, m_texture, NULL);
}

	}

	m_dirty.RemoveAll();
}

//

GSTextureCache::GSDepthStencil::GSDepthStencil()
{
}
