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
#include "GSTextureCache.h"

#include "GSRendererHW.h"

//

GSDirtyRect::GSDirtyRect(DWORD PSM, CRect r)
{
	m_PSM = PSM;
	m_rcDirty = r;
}

CRect GSDirtyRect::GetDirtyRect(const GIFRegTEX0& TEX0)
{
	CRect rcDirty = m_rcDirty;

	CSize src = GSLocalMemory::GetBlockSize(m_PSM);
	rcDirty.left = (rcDirty.left) & ~(src.cx-1);
	rcDirty.right = (rcDirty.right + (src.cx-1) /* + 1 */) & ~(src.cx-1);
	rcDirty.top = (rcDirty.top) & ~(src.cy-1);
	rcDirty.bottom = (rcDirty.bottom + (src.cy-1) /* + 1 */) & ~(src.cy-1);

	if(m_PSM != TEX0.PSM)
	{
		CSize dst = GSLocalMemory::GetBlockSize(TEX0.PSM);
		rcDirty.left = MulDiv(m_rcDirty.left, dst.cx, src.cx);
		rcDirty.right = MulDiv(m_rcDirty.right, dst.cx, src.cx);
		rcDirty.top = MulDiv(m_rcDirty.top, dst.cy, src.cy);
		rcDirty.bottom = MulDiv(m_rcDirty.bottom, dst.cy, src.cy);
	}

	rcDirty &= CRect(0, 0, 1<<TEX0.TW, 1<<TEX0.TH);

	return rcDirty;
}

void GSDirtyRectList::operator = (const GSDirtyRectList& l)
{
	RemoveAll();
	POSITION pos = l.GetHeadPosition();
	while(pos) AddTail(l.GetNext(pos));
}

CRect GSDirtyRectList::GetDirtyRect(const GIFRegTEX0& TEX0)
{
	if(IsEmpty()) return CRect(0, 0, 0, 0);
	CRect r(INT_MAX, INT_MAX, 0, 0);
	POSITION pos = GetHeadPosition();
	while(pos) r |= GetNext(pos).GetDirtyRect(TEX0);
	return r;
}

//

GSTexture::GSTexture()
{
	m_TEX0.TBP0 = ~0;
	m_fRT = false;
	m_scale = scale_t(1, 1);
	m_valid = CSize(0, 0);
	m_nAge = 0;
	m_nVsyncs = 0;
	m_hash = ~0;
	m_hashsize.SetSize(0, 0);
	m_size = 0;
	memset(&m_desc, 0, sizeof(m_desc));
}

//

GSTextureCache::GSTextureCache()
{
}

HRESULT GSTextureCache::CreateTexture(GSState* s, GSTexture* pt, DWORD PSM, DWORD CPSM)
{
	if(!pt || pt->m_pTexture) {ASSERT(0); return E_FAIL;}

	int w = 1 << pt->m_TEX0.TW;
	int h = 1 << pt->m_TEX0.TH;

	int bpp = 0;
	D3DFORMAT fmt = D3DFMT_UNKNOWN;
	D3DFORMAT palfmt = D3DFMT_UNKNOWN;

	switch(PSM)
	{
	default:
	case PSM_PSMCT32:
		bpp = 32;
		fmt = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT24:
		bpp = 32;
		fmt = D3DFMT_X8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		bpp = 16;
		fmt = D3DFMT_A1R5G5B5;
		break;
	case PSM_PSMT8:
	case PSM_PSMT4:
	case PSM_PSMT8H:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		bpp = 8;
		fmt = D3DFMT_L8;
		palfmt = CPSM == PSM_PSMCT32 ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5;
		break;
	}

	pt->m_size = w*h*bpp>>3;

	POSITION pos = m_pTexturePool.GetHeadPosition();
	while(pos)
	{
		CComPtr<IDirect3DTexture9> pTexture = m_pTexturePool.GetNext(pos);

		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		pTexture->GetLevelDesc(0, &desc);

		if(w == desc.Width && h == desc.Height && fmt == desc.Format && !IsTextureInCache(pTexture))
		{
			pt->m_pTexture = pTexture;
			pt->m_desc = desc;
			break;
		}
	}

	if(!pt->m_pTexture)
	{
		while(m_pTexturePool.GetCount() > 20)
			m_pTexturePool.RemoveTail();

		if(FAILED(s->m_pD3DDev->CreateTexture(w, h, 1, 0, fmt, D3DPOOL_MANAGED, &pt->m_pTexture, NULL)))
			return E_FAIL;

		pt->m_pTexture->GetLevelDesc(0, &pt->m_desc);

		m_pTexturePool.AddHead(pt->m_pTexture);
	}

	if(bpp == 8)
	{
		if(FAILED(s->m_pD3DDev->CreateTexture(256, 1, 1, 0, palfmt, D3DPOOL_MANAGED, &pt->m_pPalette, NULL)))
		{
			pt->m_pTexture = NULL;
			return E_FAIL;
		}
	}

	return S_OK;
}

bool GSTextureCache::IsTextureInCache(IDirect3DTexture9* pTexture)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		if(GetNext(pos)->m_pTexture == pTexture)
			return true;
	}

	return false;
}

void GSTextureCache::RemoveOldTextures(GSState* s)
{
	DWORD size = 0;

	POSITION pos = GetHeadPosition();
	while(pos) size += GetNext(pos)->m_size;

	pos = GetTailPosition();
	while(pos && size > 48*1024*1024/*s->m_ddcaps.dwVidMemTotal*/)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 too many textures in cache (%d, %.2f MB)\n"), GetCount(), 1.0f*size/1024/1024);
#endif
		POSITION cur = pos;

		GSTexture* pt = GetPrev(pos);
		if(!pt->m_fRT)
		{
			size -= pt->m_size;
			RemoveAt(cur);
			delete pt;
		}
	}
}

bool GSTextureCache::GetDirtySize(GSState* s, GSTexture* pt, int& w, int& h)
{
	int w0 = w;
	int h0 = h;

//TODO: fix vf4evo
//return true;

	s->MaxTexUV(w, h);

	CSize dirty = pt->m_dirty.GetDirtyRect(pt->m_TEX0).BottomRight();
	CSize valid = pt->m_valid;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 used %dx%d (%dx%d), valid %dx%d, dirty %dx%d\n"), 
		w, h, w0, h0, valid.cx, valid.cy, dirty.cx, dirty.cy);
#endif

	bool fNeededInValid = valid.cx >= w && valid.cy >= h;

	if(fNeededInValid)
	{
		bool fDirty = dirty.cx > 0 || dirty.cy > 0;

		if(!fDirty) 
			return false;

		bool fDirtyInNeeded = w >= dirty.cx && h >= dirty.cy;

		if(fDirtyInNeeded)
		{
			w = dirty.cx;
			h = dirty.cy;
		}
		else
		{
			bool fDirtyInValid = valid.cx >= dirty.cx && valid.cy >= dirty.cy;

			if(fDirtyInValid)
			{
				w = max(w, dirty.cx);
				h = max(h, dirty.cy);
			}
			else
			{
				w = max(valid.cx, dirty.cx);
				h = max(valid.cy, dirty.cy);
			}
		}
	}
	else
	{
		w = max(w, valid.cx);
		h = max(h, valid.cy);
	}

	return true;
}

DWORD GSTextureCache::HashTexture(int w, int h, int pitch, void* bits)
{
	// TODO: make this faster with sse2

	DWORD chksum = w + h + pitch + *(BYTE*)bits;

	if(w >= 1)
	{
		BYTE* p = (BYTE*)bits;
		for(int j = 0; j < h; j++, p += pitch)
			for(int i = 0; i < w; i++)
				chksum += ((DWORD*)p)[i];
	}

	return chksum;
}

HRESULT GSTextureCache::UpdateTexture(GSState* s, GSTexture* pt, GSLocalMemory::readTexture rt)
{
	int w = 1 << pt->m_TEX0.TW;
	int h = 1 << pt->m_TEX0.TH;

	if(!GetDirtySize(s, pt, w, h))
		return S_OK;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 updating texture %dx%d (%dx%d)\n"), tw, th, tw0, th0);
#endif

	int bpp = 0;

	switch(pt->m_desc.Format)
	{
	case D3DFMT_A8R8G8B8: bpp = 32; break;
	case D3DFMT_X8R8G8B8: bpp = 32; break;
	case D3DFMT_A1R5G5B5: bpp = 16; break;
	case D3DFMT_L8: bpp = 8; break;
	default: ASSERT(0); return E_FAIL;
	}

	RECT rlock = {0, 0, w, h};

	D3DLOCKED_RECT r;
	if(FAILED(pt->m_pTexture->LockRect(0, &r, w == pt->m_desc.Width && h == pt->m_desc.Height ? NULL : &rlock, D3DLOCK_NO_DIRTY_UPDATE)))
	{
		ASSERT(0);
		return E_FAIL;
	}

	(s->m_lm.*rt)(w, h, (BYTE*)r.pBits, r.Pitch, s->m_ctxt->TEX0, s->m_de.TEXA, s->m_ctxt->CLAMP);
	s->m_stats.IncReads(w*h*bpp>>3);

	DWORD hash = HashTexture((w>>2)*(bpp>>3), h, r.Pitch, r.pBits);

	pt->m_pTexture->UnlockRect(0);

	if(pt->m_hashsize != CSize(w, h) || pt->m_hash != hash)
	{
		pt->m_hashsize = CSize(w, h);
		pt->m_hash = hash;
		pt->m_pTexture->AddDirtyRect(&rlock);
		pt->m_pTexture->PreLoad();
		s->m_stats.IncTexWrite(w*h*bpp>>3);
	}
	else
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 updating texture is not necessary!!! skipping AddDirtyRect\n"));
#endif
	}

	if(pt->m_valid.cx < w) pt->m_valid.cx = w;
	if(pt->m_valid.cy < h) pt->m_valid.cy = h;

	pt->m_dirty.RemoveAll();

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 texture was updated, valid %dx%d\n"), pt->m_valid.cx, pt->m_valid.cy);
#endif

 #ifdef DEBUG_SAVETEXTURES   
// if(pt->m_TEX0.TBP0 == 0x02722 && pt->m_TEX0.PSM == 20 && pt->m_TEX0.TW == 8 && pt->m_TEX0.TH == 6)   
 {   
    CString fn;   
    fn.Format(_T("c:\\%08I64x_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d-%I64d_%I64d-%I64d.bmp"),   
            pt->m_TEX0.TBP0, pt->m_TEX0.PSM, pt->m_TEX0.TBW,   
            pt->m_TEX0.TW, pt->m_TEX0.TH,   
            pt->m_CLAMP.WMS, pt->m_CLAMP.WMT, pt->m_CLAMP.MINU, pt->m_CLAMP.MAXU, pt->m_CLAMP.MINV, pt->m_CLAMP.MAXV);   
    D3DXSaveTextureToFile(fn, D3DXIFF_BMP, pt->m_pTexture, NULL);   
 }   
 #endif 

	return S_OK;
}

bool GSTextureCache::Fetch(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

	int nPaletteEntries = PaletteEntries(s->m_ctxt->TEX0.PSM);

	DWORD clut[256];

	if(nPaletteEntries)
	{
		s->m_lm.SetupCLUT32(s->m_ctxt->TEX0, s->m_de.TEXA);
		s->m_lm.CopyCLUT32(clut, nPaletteEntries);
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05x %d (%d)\n"), tw, th, s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		pt = GetNext(pos);

		if(pt->m_TEX0.TBP0 == s->m_ctxt->TEX0.TBP0)
		{
			if(pt->m_fRT)
			{
				lr = found;

				// FIXME: RT + 8h,4hl,4hh
				if(nPaletteEntries)
					return false;

				// FIXME: different RT res
			}
			else if(s->m_ctxt->TEX0.PSM == pt->m_TEX0.PSM && s->m_ctxt->TEX0.TW == pt->m_TEX0.TW && s->m_ctxt->TEX0.TH == pt->m_TEX0.TH
			&& (!(s->m_ctxt->CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(s->m_ctxt->CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) || s->m_ctxt->CLAMP.i64 == pt->m_CLAMP.i64)
			&& s->m_de.TEXA.TA0 == pt->m_TEXA.TA0 && s->m_de.TEXA.TA1 == pt->m_TEXA.TA1 && s->m_de.TEXA.AEM == pt->m_TEXA.AEM
			&& (!nPaletteEntries || s->m_ctxt->TEX0.CPSM == pt->m_TEX0.CPSM && !memcmp(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]))))
			{
				lr = needsupdate;
			}
		}

		if(lr == notfound) pt = NULL;
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 lr = %s\n"), lr == found ? _T("found") : lr == needsupdate ? _T("needsupdate") : _T("notfound"));
#endif

	if(lr == notfound)
	{
		pt = new GSTexture();

		pt->m_TEX0 = s->m_ctxt->TEX0;
		pt->m_CLAMP = s->m_ctxt->CLAMP;
		pt->m_TEXA = s->m_de.TEXA;

		if(!SUCCEEDED(CreateTexture(s, pt, PSM_PSMCT32)))
		{
			delete pt;
			return false;
		}

		RemoveOldTextures(s);

		AddHead(pt);

		lr = needsupdate;
	}

	ASSERT(pt);

	if(pt && nPaletteEntries)
	{
		memcpy(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]));
	}

	if(lr == needsupdate)
	{
		UpdateTexture(s, pt, &GSLocalMemory::ReadTexture);

		lr = found;
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		if(POSITION pos = Find(pt))
			RemoveAt(pos);
		AddHead(pt);

		t = *pt;

		return true;
	}

	return false;
}

bool GSTextureCache::FetchP(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05x %d (%d)\n"), tw, th, s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, PaletteEntries(s->m_ctxt->TEX0.PSM));
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		pt = GetNext(pos);

		if(pt->m_TEX0.TBP0 == s->m_ctxt->TEX0.TBP0)
		{
			if(pt->m_fRT)
			{
				lr = found;

				// FIXME: RT + 8h,4hl,4hh
				if(PaletteEntries(s->m_ctxt->TEX0.PSM))
					return false;
/*
				if(nPaletteEntries && !pt->m_pPalette) // yuck!
				{
					if(FAILED(s->m_pD3DDev->CreateTexture(256, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pt->m_pPalette, NULL)))
						return false;
				}

				// FIXME: different RT res
*/
			}
			else if(s->m_ctxt->TEX0.PSM == pt->m_TEX0.PSM && s->m_ctxt->TEX0.TW == pt->m_TEX0.TW && s->m_ctxt->TEX0.TH == pt->m_TEX0.TH
			&& (!(s->m_ctxt->CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(s->m_ctxt->CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) || s->m_ctxt->CLAMP.i64 == pt->m_CLAMP.i64))
			{
				lr = needsupdate;
			}
		}

		if(lr == notfound) pt = NULL;
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 lr = %s\n"), lr == found ? _T("found") : lr == needsupdate ? _T("needsupdate") : _T("notfound"));
#endif

	if(lr == notfound)
	{
		pt = new GSTexture();

		pt->m_TEX0 = s->m_ctxt->TEX0;
		pt->m_CLAMP = s->m_ctxt->CLAMP;
		// pt->m_TEXA = s->m_de.TEXA;

		if(!SUCCEEDED(CreateTexture(s, pt, s->m_ctxt->TEX0.PSM, PSM_PSMCT32)))
		{
			delete pt;
			return false;
		}

		RemoveOldTextures(s);

		AddHead(pt);

		lr = needsupdate;
	}

	ASSERT(pt);

	if(pt && pt->m_pPalette)
	{
		D3DLOCKED_RECT r;
		if(FAILED(pt->m_pPalette->LockRect(0, &r, NULL, 0)))
			return false;
		s->m_lm.ReadCLUT32(s->m_ctxt->TEX0, s->m_de.TEXA, (DWORD*)r.pBits);
		pt->m_pPalette->UnlockRect(0);
		s->m_stats.IncTexWrite(256*4);
	}

	if(lr == needsupdate)
	{
		UpdateTexture(s, pt, &GSLocalMemory::ReadTextureP);

		lr = found;
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		if(POSITION pos = Find(pt))
			RemoveAt(pos);
		AddHead(pt);

		t = *pt;

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
		GSTexture* pt = GetNext(pos);
		pt->m_nAge++;
		pt->m_nVsyncs++;
		if(pt->m_nAge > 10)
		{
			pRTs.RemoveKey(pt->m_TEX0.TBP0);
			RemoveAt(cur);
			delete pt;
		}
	}
}

void GSTextureCache::ResetAge(DWORD TBP0)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0) pt->m_nAge = 0;
	}
}

void GSTextureCache::RemoveAll()
{
	while(GetCount()) delete RemoveHead();
	m_pTexturePool.RemoveAll();
}

void GSTextureCache::InvalidateTexture(GSState* s, DWORD TBP0, DWORD PSM, CRect r)
{
#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 invalidate %05x (%dx%x - %dx%d)\n"), TBP0, r.left, r.top, r.right, r.bottom);
#endif

	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0 && pt->m_fRT) 
		{
			RemoveAt(cur);
			delete pt;
		}
	}

	pos = GetHeadPosition();
	while(pos)
	{
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0)
			pt->m_dirty.AddHead(GSDirtyRect(PSM, r));
	}
}

void GSTextureCache::InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, CRect r)
{
	CComPtr<IDirect3DTexture9> pRT;

	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0 && pt->m_fRT) 
		{
			pRT = pt->m_pTexture;
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
	hr = s->m_pD3DDev->CreateOffscreenPlainSurface(desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSysMem, NULL);
	if(FAILED(hr)) return;

	hr = s->m_pD3DDev->GetRenderTargetData(pVidMem, pSysMem);
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
			GSLocalMemory::writeFrame wf = s->m_lm.GetWriteFrame(PSM);

			for(int y = r.top; y < r.bottom; y++, p += lr.Pitch)
			{
				for(int x = r.left; x < r.right; x++)
				{
					(s->m_lm.*wf)(x, y, ((DWORD*)p)[x], TBP0, BW);
				}
			}
		}

		pSysMem->UnlockRect();
	}
	*/
}

void GSTextureCache::AddRT(DWORD TBP0, IDirect3DTexture9* pRT, scale_t scale)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture* pt = GetNext(pos);
		if(pt->m_TEX0.TBP0 == TBP0)
		{
			RemoveAt(cur);
			delete pt;
		}
	}

	GSTexture* pt = new GSTexture();
	pt->m_pTexture = pRT;
	pt->m_pTexture->GetLevelDesc(0, &pt->m_desc);
	pt->m_TEX0.TBP0 = TBP0;
	pt->m_scale = scale;
	// pt->m_valid.SetSize();
	pt->m_fRT = true;

	AddHead(pt);
}
