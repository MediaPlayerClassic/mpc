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

#include "stdafx.h"
#include "GSTextureCache.h"
#include "GSState.h"
#include "GSUtil.h"

GSTextureCache::GSTexture::GSTexture(GSTextureCache* tc) 
	: GSSurface(tc)
{
	m_valid = CRect(0, 0, 0, 0);
	m_bpp = 0;
}

GSTextureCache::GSTexture::~GSTexture()
{
	if(m_palette)
	{
		CComPtr<IDirect3DSurface9> surface;

		m_palette->GetSurfaceLevel(0, &surface);

		m_tc->Recycle(surface);
	}
}

bool GSTextureCache::GSTexture::Create()
{
	HRESULT hr;

	m_TEX0 = m_tc->m_state->m_context->TEX0;
	m_CLAMP = m_tc->m_state->m_context->CLAMP;

	DWORD psm = m_TEX0.PSM;

	switch(psm)
	{
	case PSM_PSMT8:
	case PSM_PSMT8H:
	case PSM_PSMT4:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		psm = m_TEX0.CPSM;
		break;
	}

	D3DFORMAT format;

	switch(psm)
	{
	default:
		ASSERT(0);
	case PSM_PSMCT32:
		m_bpp = 32;
		format = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT24:
		m_bpp = 32;
		format = D3DFMT_X8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		m_bpp = 16;
		format = D3DFMT_A1R5G5B5;
		break;
	}

	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	m_size = w * h * m_bpp >> 3;

	hr = m_tc->CreateTexture(w, h, format, &m_texture, &m_surface, &m_desc);

	return SUCCEEDED(hr);
}

bool GSTextureCache::GSTexture::Create(GSRenderTarget* rt)
{
	rt->Update();

	HRESULT hr;

	m_scale = rt->m_scale;
	m_size = rt->m_size;
	m_TEX0 = m_tc->m_state->m_context->TEX0;
	m_CLAMP = m_tc->m_state->m_context->CLAMP;

	int tw = 1 << m_TEX0.TW;
	int th = 1 << m_TEX0.TH;
	int tp = m_TEX0.TW << 6;

	hr = m_tc->CreateRenderTarget(rt->m_desc.Width, rt->m_desc.Height, &m_texture, &m_surface, &m_desc);

	// pitch conversion

	if(rt->m_TEX0.TBW != m_TEX0.TBW) // && rt->m_TEX0.PSM == m_TEX0.PSM
	{
		// sfex3 uses this trick (bw: 10 -> 5, wraps the right side below the left)

		// ASSERT(rt->m_TEX0.TBW > m_TEX0.TBW); // otherwise scale.x need to be reduced to make the larger texture fit (TODO)

		int bw = 64;
		int bh = m_TEX0.PSM == PSM_PSMCT32 || m_TEX0.PSM == PSM_PSMCT24 ? 32 : 64;

		int sw = rt->m_TEX0.TBW << 6;

		int dw = m_TEX0.TBW << 6;
		int dh = 1 << m_TEX0.TH;

		for(int dy = 0; dy < dh; dy += bh)
		{
			for(int dx = 0; dx < dw; dx += bw)
			{
				int o = dy * dw / bh + dx;

				int sx = o % sw;
				int sy = o / sw;

				CRect src, dst;

				src.left = (LONG)(m_scale.x * sx + 0.5f);
				src.top = (LONG)(m_scale.y * sy + 0.5f);
				src.right = (LONG)(m_scale.x * (sx + bw) + 0.5f);
				src.bottom = (LONG)(m_scale.y * (sy + bh) + 0.5f);

				dst.left = (LONG)(m_scale.x * dx + 0.5f);
				dst.top = (LONG)(m_scale.y * dy + 0.5f);
				dst.right = (LONG)(m_scale.x * (dx + bw) + 0.5f);
				dst.bottom = (LONG)(m_scale.y * (dy + bh) + 0.5f);

				hr = m_tc->m_state->m_dev->StretchRect(rt->m_surface, src, m_surface, dst, D3DTEXF_POINT);

				// TODO: this is quite a lot of StretchRect call, do it with one DrawPrimUP
			}
		}
	}
	else if(tw < tp)
	{
		// FIXME: timesplitters blurs the render target by blending itself over a couple of times

		if(tw == 256 && th == 128 && tp == 512 && (m_TEX0.TBP0 == 0 || m_TEX0.TBP0 == 0x00e00))
		{
			return false;
		}

	/*

		CRect src(0, 0, m_desc.Width * tw / tp, m_desc.Height);
		CRect dst(0, 0, m_desc.Width, m_desc.Height);

D3DXSaveSurfaceToFile(_T("c:\\1.bmp"), D3DXIFF_BMP, rt->m_surface, NULL, NULL);

		hr = m_tc->m_state->m_dev->StretchRect(rt->m_surface, src, m_surface, dst, D3DTEXF_LINEAR);

D3DXSaveSurfaceToFile(_T("c:\\2.bmp"), D3DXIFF_BMP, m_surface, NULL, NULL);

		m_scale.x = 1;
*/

		hr = m_tc->m_state->m_dev->StretchRect(rt->m_surface, NULL, m_surface, NULL, D3DTEXF_LINEAR);
	}
	else
	{
		hr = m_tc->m_state->m_dev->StretchRect(rt->m_surface, NULL, m_surface, NULL, D3DTEXF_LINEAR);
	}

	// width/height conversion

	int w = m_scale.x * tw;
	int h = m_scale.y * th;

	if(w != m_desc.Width || h != m_desc.Height)
	{
		CRect dst(0, 0, w, h);
		
		if(w > m_desc.Width) 
		{
			float scale = m_scale.x;
			m_scale.x = (float)m_desc.Width / tw;
			dst.right = m_desc.Width * m_scale.x / scale;
			w = m_desc.Width;
		}
		
		if(h > m_desc.Height) 
		{
			float scale = m_scale.y;
			m_scale.y = (float)m_desc.Height / th;
			dst.bottom = m_desc.Height * m_scale.y / scale;
			h = m_desc.Height;
		}

		CRect src(0, 0, w, h);

		CComPtr<IDirect3DTexture9> texture;
		CComPtr<IDirect3DSurface9> surface;

		hr = m_tc->CreateRenderTarget(w, h, &texture, &surface, &m_desc);

		hr = m_tc->m_state->m_dev->StretchRect(m_surface, src, surface, dst, src == dst ? D3DTEXF_POINT : D3DTEXF_LINEAR);

		m_tc->Recycle(m_surface);

		m_texture = texture;
		m_surface = surface;
	}

	switch(m_TEX0.PSM)
	{
	case PSM_PSMCT24:
		m_desc.Format = D3DFMT_X8R8G8B8;
		break;
	case PSM_PSMT8H:
		hr = m_tc->CreateTexture(256, 1, m_TEX0.CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5, &m_palette);
		break;
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		ASSERT(0); // TODO
		break;
	}

	return true;
}

bool GSTextureCache::GSTexture::Create(GSDepthStencil* ds)
{
	// hmmmm, lockable ds formats don't have stencil...

	return false;
}

void GSTextureCache::GSTexture::Update(GSLocalMemory::readTexture rt)
{
	__super::Update();

	if(IsRenderTarget())
	{
		return;
	}

	CRect r;

	if(!GetDirtyRect(r))
	{
		return;
	}

	HRESULT hr;

	D3DLOCKED_RECT lr;

	if(SUCCEEDED(hr = m_texture->LockRect(0, &lr, &r, 0))) 
	{
		GSState* s = m_tc->m_state;

		(s->m_mem.*rt)(r, (BYTE*)lr.pBits, lr.Pitch, s->m_context->TEX0, s->m_env.TEXA, s->m_context->CLAMP);

		m_texture->UnlockRect(0);

		s->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * m_bpp >> 3);

		CRect r2 = m_valid & r;

		if(!r2.IsRectEmpty())
		{
			s->m_perfmon.Put(GSPerfMon::Unswizzle2, r2.Width() * r2.Height() * m_bpp >> 3);
		}

		m_valid |= r;
		m_dirty.RemoveAll();

		s->m_perfmon.Put(GSPerfMon::Texture, r.Width() * r.Height() * m_bpp >> 3);
	}
}

bool GSTextureCache::GSTexture::GetDirtyRect(CRect& r)
{
	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	r.SetRect(0, 0, w, h);

	// FIXME: kyo's left hand after being selected for player one (PS2-SNK_Vs_Capcom_SVC_Chaos_PAL_CDFull.iso)
	// return true;

	m_tc->m_state->MinMaxUV(w, h, r);

	CRect dirty = m_dirty.GetDirtyRect(m_TEX0);
	CRect valid = m_valid;

	dirty &= CRect(0, 0, m_desc.Width, m_desc.Height);

	if(IsRectInRect(r, valid))
	{
		if(dirty.IsRectEmpty()) return false;
		else if(IsRectInRect(dirty, r)) r = dirty;
		else if(IsRectInRect(dirty, valid)) r |= dirty;
		else r = valid & dirty;
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
