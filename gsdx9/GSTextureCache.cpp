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

//

GSTextureCache::GSTextureCache()
{
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();
}

void GSTextureCache::RemoveAll()
{
	while(GetCount()) delete RemoveHead();
	m_pTexturePool.RemoveAll();
	m_pRenderTargetPool.RemoveAll();
}

HRESULT GSTextureCache::CreateRenderTarget(GSState* s, int w, int h, IDirect3DTexture9** pprt)
{
	ASSERT(pprt && *pprt == NULL);

	HRESULT hr;

	POSITION pos = m_pRenderTargetPool.GetHeadPosition();

	while(pos)
	{
		IDirect3DTexture9* rt = m_pRenderTargetPool.GetNext(pos);

		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		rt->GetLevelDesc(0, &desc);

		if(desc.Width == w && desc.Height == h && !IsTextureInCache(rt))
		{
			(*pprt = rt)->AddRef();
			return S_OK;
		}
	}

	hr = s->m_dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, pprt, NULL);

	if(FAILED(hr)) return hr;
TRACE(_T("3. CreateTexture(%d, %d)\n"), w, h);

	m_pRenderTargetPool.AddHead(*pprt);

	while(m_pRenderTargetPool.GetCount() > 3)
	{
		m_pRenderTargetPool.RemoveTail();
	}

	return S_OK;
}

HRESULT GSTextureCache::CreateTexture(GSState* s, GSTexture* t, DWORD psm, DWORD cpsm)
{
	if(!t || t->m_texture) {ASSERT(0); return E_FAIL;}

	int w = 1 << t->m_TEX0.TW;
	int h = 1 << t->m_TEX0.TH;

	D3DFORMAT fmt = D3DFMT_UNKNOWN;
	D3DFORMAT palfmt = D3DFMT_UNKNOWN;

	t->m_bpp = 0;

	switch(psm)
	{
	default:
	case PSM_PSMCT32:
		t->m_bpp = 32;
		fmt = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT24:
		t->m_bpp = 32;
		fmt = D3DFMT_X8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		t->m_bpp = 16;
		fmt = D3DFMT_A1R5G5B5;
		break;
	case PSM_PSMT8:
	case PSM_PSMT4:
	case PSM_PSMT8H:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		t->m_bpp = 8;
		fmt = D3DFMT_L8;
		palfmt = cpsm == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5;
		break;
	}

	t->m_bytes = w * h * t->m_bpp >> 3;

	POSITION pos = m_pTexturePool.GetHeadPosition();

	while(pos)
	{
		IDirect3DTexture9* texture = m_pTexturePool.GetNext(pos);

		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		texture->GetLevelDesc(0, &desc);

		if(w == desc.Width && h == desc.Height && fmt == desc.Format && !IsTextureInCache(texture))
		{
			t->m_texture = texture;
			t->m_desc = desc;
			break;
		}
	}

	if(!t->m_texture)
	{
		while(m_pTexturePool.GetCount() > 20)
		{
			m_pTexturePool.RemoveTail();
		}

		if(FAILED(s->m_dev->CreateTexture(w, h, 1, 0, fmt, D3DPOOL_MANAGED, &t->m_texture, NULL)))
		{
			return E_FAIL;
		}
TRACE(_T("4. CreateTexture(%d, %d)\n"), w, h);

		t->m_texture->GetLevelDesc(0, &t->m_desc);

		m_pTexturePool.AddHead(t->m_texture);
	}

	if(t->m_bpp == 8)
	{
		if(FAILED(s->m_dev->CreateTexture(256, 1, 1, 0, palfmt, D3DPOOL_MANAGED, &t->m_palette, NULL)))
		{
			t->m_texture = NULL;
			return E_FAIL;
		}
	}

	RemoveOld(s);

	AddHead(t);

	return S_OK;
}

bool GSTextureCache::IsTextureInCache(IDirect3DTexture9* t)
{
	POSITION pos = GetHeadPosition();

	while(pos)
	{
		if(GetNext(pos)->m_texture == t)
		{
			return true;
		}
	}

	return false;
}

void GSTextureCache::RemoveOld(GSState* s)
{
	DWORD bytes = 0;

	POSITION pos = GetHeadPosition();
	
	while(pos)
	{
		bytes += GetNext(pos)->m_bytes;
	}

	pos = GetTailPosition();

	while(pos && bytes > 96*1024*1024/*s->m_ddcaps.dwVidMemTotal*/)
	{
		POSITION cur = pos;

		GSTexture* t = GetPrev(pos);

		if(!t->m_rt)
		{
			bytes -= t->m_bytes;
			RemoveAt(cur);
			delete t;
		}
	}
}

GSTexture* GSTextureCache::ConvertRTPitch(GSState* s, GSTexture* t)
{
	if(t->m_TEX0.TBW == s->m_context->TEX0.TBW)
	{
		return t;
	}

	// sfex3 uses this trick (bw: 10 -> 5, wraps the right side below the left)

	ASSERT(t->m_TEX0.TBW > s->m_context->TEX0.TBW); // otherwise scale.x need to be reduced to make the larger texture fit (TODO)

	int bw = 64;
	int bh = s->m_context->TEX0.PSM == PSM_PSMCT32 || s->m_context->TEX0.PSM == PSM_PSMCT24 ? 32 : 64;

	int sw = t->m_TEX0.TBW << 6;

	int dw = s->m_context->TEX0.TBW << 6;
	int dh = 1 << s->m_context->TEX0.TH;

	HRESULT hr;

	CComPtr<IDirect3DTexture9> pRT;

	if(FAILED(hr = CreateRenderTarget(s, t->m_desc.Width, t->m_desc.Height, &pRT)))
	{
		return NULL;
	}

	CComPtr<IDirect3DSurface9> pSrc, pDst;

	hr = pRT->GetSurfaceLevel(0, &pSrc);
	if(FAILED(hr)) return NULL;

	hr = t->m_texture->GetSurfaceLevel(0, &pDst);
	if(FAILED(hr)) return NULL;

	hr = s->m_dev->StretchRect(pDst, NULL, pSrc, NULL, D3DTEXF_POINT);
	if(FAILED(hr)) return NULL;

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

			hr = s->m_dev->StretchRect(pSrc, src, pDst, dst, D3DTEXF_POINT);

			// TODO: this is quite a lot of StretchRect call, do it with one DrawPrimUP
		}
	}

	t->m_TEX0.TW = s->m_context->TEX0.TW;
	t->m_TEX0.TBW = s->m_context->TEX0.TBW;

	return t;
}

GSTexture* GSTextureCache::ConvertRTWidthHeight(GSState* s, GSTexture* t)
{
	int tw = t->m_scale.x * (1 << s->m_context->TEX0.TW);
	int th = t->m_scale.y * (1 << s->m_context->TEX0.TH);

	int rw = t->m_desc.Width;
	int rh = t->m_desc.Height;

	if(tw != rw || th != rh)
	{
		CRect dst(0, 0, tw, th);
		
		if(tw > rw)
		{
			float scale = t->m_scale.x;
			t->m_scale.x = (float)rw / (1 << s->m_context->TEX0.TW);
			dst.right = rw * t->m_scale.x / scale;
			tw = rw;
		}
		
		if(th > rh)
		{
			float scale = t->m_scale.y;
			t->m_scale.y = (float)rh / (1 << s->m_context->TEX0.TH);
			dst.bottom = rh * t->m_scale.y / scale;
			th = rh;
		}

		CRect src(0, 0, tw, th);

		HRESULT hr;

		CComPtr<IDirect3DTexture9> rt;

		hr = CreateRenderTarget(s, tw, th, &rt);
		if(FAILED(hr)) return NULL;

		CComPtr<IDirect3DSurface9> pSrc, pDst;

		hr = t->m_texture->GetSurfaceLevel(0, &pSrc);
		if(FAILED(hr)) return NULL;

		hr = rt->GetSurfaceLevel(0, &pDst);
		if(FAILED(hr)) return NULL;

		hr = s->m_dev->StretchRect(pSrc, src, pDst, dst, src == dst ? D3DTEXF_POINT : D3DTEXF_LINEAR);
		if(FAILED(hr)) return NULL;

		t->m_texture = rt;
	}

	return t;
}

GSTexture* GSTextureCache::ConvertRT(GSState* s, GSTexture* t)
{
	ASSERT(t->m_rt);

	// FIXME: RT + 8h,4hl,4hh

	if(s->m_context->TEX0.PSM == PSM_PSMT8H)
	{
		if(!t->m_palette)
		{
			if(FAILED(s->m_dev->CreateTexture(256, 1, 1, 0, s->m_context->TEX0.CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &t->m_palette, NULL)))
			{
				return NULL;
			}
		}
	}
	else if(GSLocalMemory::m_psmtbl[s->m_context->TEX0.PSM].pal)
	{
		return NULL;
	}

	t = ConvertRTPitch(s, t);

	t = ConvertRTWidthHeight(s, t);

	return t;
}

bool GSTextureCache::Fetch(GSState* s, GSTextureBase& tb)
{
	GSTexture* t = NULL;

	int nPaletteEntries = GSLocalMemory::m_psmtbl[s->m_context->TEX0.PSM].pal;

	DWORD clut[256];

	if(nPaletteEntries)
	{
		s->m_mem.SetupCLUT32(s->m_context->TEX0, s->m_env.TEXA);
		s->m_mem.CopyCLUT32(clut, nPaletteEntries);
	}

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();

	while(pos && !t)
	{
		POSITION cur = pos;
		
		t = GetNext(pos);

		if(HasSharedBits(t->m_TEX0.TBP0, t->m_TEX0.PSM, s->m_context->TEX0.TBP0, s->m_context->TEX0.PSM))
		{
			if(t->m_rt)
			{
				lr = found;

				if(!(t = ConvertRT(s, t)))
				{
					return false;
				}
			}
			else if(s->m_context->TEX0.PSM == t->m_TEX0.PSM && t->m_TEX0.TBW == s->m_context->TEX0.TBW
			&& s->m_context->TEX0.TW == t->m_TEX0.TW && s->m_context->TEX0.TH == t->m_TEX0.TH
			&& (!(s->m_context->CLAMP.WMS & 2) && !(t->m_CLAMP.WMS & 2) && !(s->m_context->CLAMP.WMT & 2) && !(t->m_CLAMP.WMT & 2) || s->m_context->CLAMP.i64 == t->m_CLAMP.i64)
			&& s->m_env.TEXA.TA0 == t->m_TEXA.TA0 && s->m_env.TEXA.TA1 == t->m_TEXA.TA1 && s->m_env.TEXA.AEM == t->m_TEXA.AEM
			&& (!nPaletteEntries || s->m_context->TEX0.CPSM == t->m_TEX0.CPSM && !memcmp(t->m_clut, clut, nPaletteEntries * sizeof(clut[0]))))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}

		t = NULL;
	}

	if(lr == notfound)
	{
		t = new GSTexture();

		t->m_TEX0 = s->m_context->TEX0;
		t->m_CLAMP = s->m_context->CLAMP;
		t->m_TEXA = s->m_env.TEXA;

		if(!SUCCEEDED(CreateTexture(s, t, PSM_PSMCT32)))
		{
			delete t;
			return false;
		}

		lr = needsupdate;
	}

	ASSERT(t);

	if(t && nPaletteEntries)
	{
		memcpy(t->m_clut, clut, nPaletteEntries * sizeof(clut[0]));
	}

	if(lr == needsupdate)
	{
		t->Update(s, &GSLocalMemory::ReadTexture);

		lr = found;
	}

	if(lr == found)
	{
		t->m_age = 0;
		tb = *t;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchP(GSState* s, GSTextureBase& tb)
{
	GSTexture* t = NULL;

	int nPaletteEntries = GSLocalMemory::m_psmtbl[s->m_context->TEX0.PSM].pal;

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();

	while(pos && !t)
	{
		POSITION cur = pos;

		t = GetNext(pos);

		if(HasSharedBits(t->m_TEX0.TBP0, t->m_TEX0.PSM, s->m_context->TEX0.TBP0, s->m_context->TEX0.PSM))
		{
			if(t->m_rt)
			{
				lr = found;

				if(!(t = ConvertRT(s, t)))
				{
					return false;
				}
			}
			else if(s->m_context->TEX0.PSM == t->m_TEX0.PSM && t->m_TEX0.TBW == s->m_context->TEX0.TBW
			&& s->m_context->TEX0.TW == t->m_TEX0.TW && s->m_context->TEX0.TH == t->m_TEX0.TH
			&& (!(s->m_context->CLAMP.WMS&2) && !(t->m_CLAMP.WMS&2) && !(s->m_context->CLAMP.WMT&2) && !(t->m_CLAMP.WMT&2) || s->m_context->CLAMP.i64 == t->m_CLAMP.i64))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}
		
		t = NULL;
	}

	if(lr == notfound)
	{
		t = new GSTexture();

		t->m_TEX0 = s->m_context->TEX0;
		t->m_CLAMP = s->m_context->CLAMP;
		// t->m_TEXA = s->m_env.TEXA;

		if(!SUCCEEDED(CreateTexture(s, t, s->m_context->TEX0.PSM, PSM_PSMCT32)))
		{
			delete t;
			return false;
		}

		lr = needsupdate;
	}

	ASSERT(t);

	if(t && t->m_palette) 
	{
		D3DLOCKED_RECT r;
		if(FAILED(t->m_palette->LockRect(0, &r, NULL, 0)))
			return false;
		s->m_mem.ReadCLUT32(s->m_context->TEX0, s->m_env.TEXA, (DWORD*)r.pBits);
		t->m_palette->UnlockRect(0);
		s->m_perfmon.Put(GSPerfMon::Texture, 256*4);
	}

	if(lr == needsupdate)
	{
		t->Update(s, &GSLocalMemory::ReadTextureP);

		lr = found;
	}

	if(lr == found)
	{
		t->m_age = 0;
		tb = *t;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchNP(GSState* s, GSTextureBase& tb)
{
	GSTexture* t = NULL;

	int nPaletteEntries = GSLocalMemory::m_psmtbl[s->m_context->TEX0.PSM].pal;

	DWORD clut[256];

	if(nPaletteEntries)
	{
		s->m_mem.SetupCLUT(s->m_context->TEX0, s->m_env.TEXA);
		s->m_mem.CopyCLUT32(clut, nPaletteEntries);
	}

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();

	while(pos && !t)
	{
		POSITION cur = pos;

		t = GetNext(pos);

		if(HasSharedBits(t->m_TEX0.TBP0, t->m_TEX0.PSM, s->m_context->TEX0.TBP0, s->m_context->TEX0.PSM))
		{
			if(t->m_rt)
			{
				lr = found;

				if(!(t = ConvertRT(s, t)))
				{
					return false;
				}
			}
			else if(s->m_context->TEX0.PSM == t->m_TEX0.PSM && t->m_TEX0.TBW == s->m_context->TEX0.TBW
			&& s->m_context->TEX0.TW == t->m_TEX0.TW && s->m_context->TEX0.TH == t->m_TEX0.TH
			&& (!(s->m_context->CLAMP.WMS & 2) && !(t->m_CLAMP.WMS & 2) && !(s->m_context->CLAMP.WMT & 2) && !(t->m_CLAMP.WMT & 2) || s->m_context->CLAMP.i64 == t->m_CLAMP.i64)
			// && s->m_env.TEXA.TA0 == t->m_TEXA.TA0 && s->m_env.TEXA.TA1 == t->m_TEXA.TA1 && s->m_env.TEXA.AEM == t->m_TEXA.AEM
			&& (!nPaletteEntries || s->m_context->TEX0.CPSM == t->m_TEX0.CPSM && !memcmp(t->m_clut, clut, nPaletteEntries * sizeof(clut[0]))))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}

		t = NULL;
	}

	if(lr == notfound)
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

		if(!SUCCEEDED(CreateTexture(s, t, psm)))
		{
			delete t;
			return false;
		}

		lr = needsupdate;
	}

	ASSERT(t);

	if(t && nPaletteEntries)
	{
		memcpy(t->m_clut, clut, nPaletteEntries * sizeof(clut[0]));
	}

	if(lr == needsupdate)
	{
		t->Update(s, &GSLocalMemory::ReadTextureNP);

		lr = found;
	}

	if(lr == found)
	{
		t->m_age = 0;
		tb = *t;
		return true;
	}

	return false;
}

void GSTextureCache::IncAge(CSurfMap<IDirect3DTexture9>& pRTs)
{
	POSITION pos = GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = GetNext(pos);

		t->m_age++;
		t->m_vsync++;

		if(t->m_age > 10 && (!t->m_rt || pRTs.GetCount() > 3))
		{
			pRTs.RemoveKey(t->m_TEX0.TBP0);
			RemoveAt(cur);
			delete t;
		}
	}
}

void GSTextureCache::ResetAge(DWORD TBP0)
{
	POSITION pos = GetHeadPosition();

	while(pos)
	{
		GSTexture* t = GetNext(pos);
		
		if(t->m_TEX0.TBP0 == TBP0)
		{
			t->m_age = 0;
		}
	}
}

void GSTextureCache::InvalidateTexture(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r)
{
	GIFRegTEX0 TEX0;

	TEX0.TBP0 = BITBLTBUF.DBP;
	TEX0.TBW = BITBLTBUF.DBW;
	TEX0.PSM = BITBLTBUF.DPSM;
	TEX0.TCC = 0;

	POSITION pos = GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;
		
		GSTexture* t = GetNext(pos);

		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM)) 
		{
			if(TEX0.TBW != t->m_TEX0.TBW)
			{
				// if TEX0.TBW != t->m_TEX0.TBW then this render target is more likely to 
				// be discarded by the game (means it doesn't want to transfer an image over 
				// another pre-rendered image) and can be refetched from local mem safely.

				RemoveAt(cur);
				delete t;
			}
			else if(t->m_rt) 
			{
				// TEX0.TBW = t->m_TEX0.TBW;
				TEX0.PSM = t->m_TEX0.PSM;

				if(TEX0.PSM == PSM_PSMCT32 || TEX0.PSM == PSM_PSMCT24 || TEX0.PSM == PSM_PSMCT16 || TEX0.PSM == PSM_PSMCT16S) 
				{
//					t->m_dirty.AddHead(GSDirtyRect(PSM, r));

					TRACE(_T("%d,%d - %d,%d (%08x)\n"), r.left, r.top, r.right, r.bottom, TEX0.TBP0);

					HRESULT hr;

					int tw = (r.Width() + 3) & ~3;
					int th = r.Height();

					CComPtr<IDirect3DTexture9> pSrc;
					
					hr = s->m_dev->CreateTexture(tw, th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pSrc, NULL);
TRACE(_T("5. CreateTexture(%d, %d)\n"), tw, th);

					D3DLOCKED_RECT lr;

					if(pSrc && SUCCEEDED(pSrc->LockRect(0, &lr, NULL, 0)))
					{
						GIFRegTEXA TEXA;

						TEXA.AEM = 1;
						TEXA.TA0 = 0;
						TEXA.TA1 = 0x80;

						GIFRegCLAMP CLAMP;

						CLAMP.WMS = 0;
						CLAMP.WMT = 0;

						s->m_mem.ReadTexture(r, (BYTE*)lr.pBits, lr.Pitch, TEX0, TEXA, CLAMP);
						
						s->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * 4);

						pSrc->UnlockRect(0);

						float left = t->m_scale.x * r.left;
						float top = t->m_scale.y * r.top;
						float right = t->m_scale.x * r.right;
						float bottom = t->m_scale.y * r.bottom;

						//

						CComPtr<IDirect3DSurface9> pRTSurf;
						hr = t->m_texture->GetSurfaceLevel(0, &pRTSurf);

						hr = s->m_dev->SetRenderTarget(0, pRTSurf);
						hr = s->m_dev->SetDepthStencilSurface(NULL);

						hr = s->m_dev->SetTexture(0, pSrc);
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
							{(float)right, (float)top, 0.5f, 2.0f, 1.0f * r.Width() / tw, 0},
							{(float)left, (float)bottom, 0.5f, 2.0f, 0, 1},
							{(float)right, (float)bottom, 0.5f, 2.0f, 1.0f * r.Width() / tw, 1},
						};

						hr = s->m_dev->BeginScene();
						hr = s->m_dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
						hr = s->m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
						hr = s->m_dev->EndScene();
					}
				}
				else
				{
					RemoveAt(cur);
					delete t;
				}
			}
			else
			{
				t->m_dirty.AddHead(GSDirtyRect(TEX0.PSM, r));
			}
		}
	}
}

void GSTextureCache::InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, const CRect& r)
{
	CComPtr<IDirect3DTexture9> pRT;

	POSITION pos = GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = GetNext(pos);

		if(t->m_TEX0.TBP0 == TBP0 && t->m_rt) 
		{
			pRT = t->m_texture;
			break;
		}
	}

	if(!pRT) return;

	// TODO: add resizing
/*
	HRESULT hr;

	D3DSURFACE_DESC desc;
	hr = pRT->GetLevelDesc(0, &desc);
	if(FAILED(hr)) return;

	CComPtr<IDirect3DSurface9> pVidMem;
	hr = pRT->GetSurfaceLevel(0, &pVidMem);
	if(FAILED(hr)) return;

	CComPtr<IDirect3DSurface9> pSysMem;
	hr = s->m_dev->CreateOffscreenPlainSurface(desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSysMem, NULL);
	if(FAILED(hr)) return;

	hr = s->m_dev->GetRenderTargetData(pVidMem, pSysMem);
	if(FAILED(hr)) return;

	D3DLOCKED_RECT lr;
	hr = pSysMem->LockRect(&lr, &r, D3DLOCK_READONLY|D3DLOCK_NO_DIRTY_UPDATE);
	if(SUCCEEDED(hr))
	{
		BYTE* p = (BYTE*)lr.pBits;

		if(0 && r.left == 0 && r.top == 0 && PSM == PSM_PSMCT32)
		{
		}
		else
		{
			GSLocalMemory::writeFrame wf = s->m_mem.GetWriteFrame(PSM);

			for(int y = r.top; y < r.bottom; y++, p += lr.Pitch)
			{
				for(int x = r.left; x < r.right; x++)
				{
					(s->m_mem.*wf)(x, y, ((DWORD*)p)[x], TBP0, BW);
				}
			}
		}

		pSysMem->UnlockRect();
	}
	*/
}

void GSTextureCache::AddRT(GIFRegTEX0& TEX0, IDirect3DTexture9* pRT, GSScale scale)
{
	POSITION pos = GetHeadPosition();

	while(pos)
	{
		POSITION cur = pos;

		GSTexture* t = GetNext(pos);

		if(HasSharedBits(TEX0.TBP0, TEX0.PSM, t->m_TEX0.TBP0, t->m_TEX0.PSM))
		{
			RemoveAt(cur);
			delete t;
		}
	}

	GSTexture* t = new GSTexture();

	t->m_TEX0 = TEX0;
	t->m_texture = pRT;
	t->m_texture->GetLevelDesc(0, &t->m_desc);
	t->m_scale = scale;
	t->m_rt = true;
	t->m_bpp = 32;

	AddHead(t);
}
