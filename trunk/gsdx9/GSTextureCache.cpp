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
	m_chksum = ~0;
	m_chksumsize.SetSize(0, 0);
	m_size = 0;
	memset(&m_desc, 0, sizeof(m_desc));
}

//

GSTextureCache::GSTextureCache()
{
}

HRESULT GSTextureCache::CreateTexture(GSState* s, int w, int h, GSTexture& t, DWORD PSM)
{
	if(t.m_pTexture) {ASSERT(0); return E_FAIL;}

	CInterfaceList<IDirect3DTexture9>* pTexturePool = NULL;
	int bpp = 0;
	D3DFORMAT format = D3DFMT_UNKNOWN;

	switch(PSM)
	{
	default:
	case PSM_PSMCT32:
	case PSM_PSMCT24:
		pTexturePool = &m_pTexturePool32;
		bpp = 32;
		format = D3DFMT_A8R8G8B8;
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		pTexturePool = &m_pTexturePool16;
		bpp = 16;
		format = D3DFMT_A1R5G5B5;
		break;
	case PSM_PSMT8:
	case PSM_PSMT4:
	case PSM_PSMT8H:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		pTexturePool = &m_pTexturePool8;
		bpp = 8;
		format = D3DFMT_L8;
		break;
	}

	t.m_size = w*h*bpp>>3;

	POSITION pos = pTexturePool->GetHeadPosition();
	while(pos)
	{
		CComPtr<IDirect3DTexture9> pTexture = pTexturePool->GetNext(pos);

		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		pTexture->GetLevelDesc(0, &desc);

		if(w == desc.Width && h == desc.Height && !IsTextureInCache(pTexture))
		{
			t.m_pTexture = pTexture;
			t.m_desc = desc;
			break;
		}
	}

	if(!t.m_pTexture)
	{
		while(pTexturePool->GetCount() > 10)
			pTexturePool->RemoveTail();

		if(FAILED(s->m_pD3DDev->CreateTexture(w, h, 1, 0, format, D3DPOOL_MANAGED, &t.m_pTexture, NULL)))
			return E_FAIL;

		t.m_pTexture->GetLevelDesc(0, &t.m_desc);

		pTexturePool->AddHead(t.m_pTexture);
	}

	if(bpp == 8)
	{
		if(FAILED(s->m_pD3DDev->CreateTexture(256, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t.m_pPalette, NULL)))
		{
			t.m_pTexture = NULL;
			return E_FAIL;
		}
	}

	return S_OK;
}

bool GSTextureCache::IsTextureInCache(IDirect3DTexture9* pTexture)
{
	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		const GSTexture& t = m_TextureCache.GetNext(pos);
		if(t.m_pTexture == pTexture)
			return true;
	}

	return false;
}

void GSTextureCache::RemoveOldTextures(GSState* s)
{
	DWORD size = 0;

	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		const GSTexture& t = m_TextureCache.GetNext(pos);
		size += t.m_size;
	}

	pos = m_TextureCache.GetTailPosition();
	while(pos && size > 48*1024*1024/*s->m_ddcaps.dwVidMemTotal*/)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 too many textures in cache (%d, %.2f MB)\n"), m_TextureCache.GetCount(), 1.0f*size/1024/1024);
#endif
		POSITION cur = pos;

		const GSTexture& t = m_TextureCache.GetPrev(pos);
		if(!t.m_fRT)
		{
			size -= t.m_size;
			m_TextureCache.RemoveAt(cur);
		}
	}
}

bool GSTextureCache::GetDirtySize(GSState* s, int& tw, int& th, GSTexture* pt)
{
	int tw0 = tw;
	int th0 = th;

//TODO: fix vf4evo
//return true;

	s->MaxTexUV(tw, th);

	CSize dirty = pt->m_dirty.GetDirtyRect(pt->m_TEX0).BottomRight();
	CSize valid = pt->m_valid;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 used %dx%d (%dx%d), valid %dx%d, dirty %dx%d\n"), 
		tw, th, tw0, th0, valid.cx, valid.cy, dirty.cx, dirty.cy);
#endif

	bool fNeededInValid = valid.cx >= tw && valid.cy >= th;

	if(fNeededInValid)
	{
		bool fDirty = dirty.cx > 0 || dirty.cy > 0;

		if(!fDirty) 
			return false;

		bool fDirtyInNeeded = tw >= dirty.cx && th >= dirty.cy;

		if(fDirtyInNeeded)
		{
			tw = dirty.cx;
			th = dirty.cy;
		}
		else
		{
			bool fDirtyInValid = valid.cx >= dirty.cx && valid.cy >= dirty.cy;

			if(fDirtyInValid)
			{
				tw = max(tw, dirty.cx);
				th = max(th, dirty.cy);
			}
			else
			{
				tw = max(valid.cx, dirty.cx);
				th = max(valid.cy, dirty.cy);
			}
		}
	}
	else
	{
		tw = max(tw, valid.cx);
		th = max(th, valid.cy);
	}

	return true;
}

void GSTextureCache::MoveToHead(GSTexture* pt)
{
	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		if(&m_TextureCache.GetNext(pos) == pt)
		{
			m_TextureCache.AddHead(*pt);
			m_TextureCache.RemoveAt(cur); 
			break;
		}
	}
}

bool GSTextureCache::Fetch(GSState* s, GSTexture& t)
{
	t = GSTexture();

	t.m_TEX0 = s->m_ctxt->TEX0;
	t.m_CLAMP = s->m_ctxt->CLAMP;
	t.m_TEXA = s->m_de.TEXA;

	ASSERT(t.m_TEX0.TW <= 10 && t.m_TEX0.TH <= 10);
	int tw = 1 << t.m_TEX0.TW, tw0 = tw;
	int th = 1 << t.m_TEX0.TH, th0 = th;

	int nPaletteEntries = PaletteEntries(t.m_TEX0.PSM);

	if(nPaletteEntries)
	{
		s->m_lm.setupCLUT(t.m_TEX0, t.m_TEXA);
		s->m_lm.getCLUT(t.m_clut, nPaletteEntries);
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05x %d (%d)\n"), tw, th, t.m_TEX0.TBP0, t.m_TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	GSTexture* pt = NULL;

	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos && !pt)
	{
		pt = &m_TextureCache.GetNext(pos);

		if(pt->m_TEX0.TBP0 == t.m_TEX0.TBP0)
		{
			if(pt->m_fRT)
			{
				lr = found;

				// FIXME: RT + 8h,4hl,4hh
				if(nPaletteEntries)
					return false;

				// FIXME: different RT res
			}
			else if(t.m_TEX0.PSM == pt->m_TEX0.PSM && t.m_TEX0.TW == pt->m_TEX0.TW && t.m_TEX0.TH == pt->m_TEX0.TH
			&& (!(t.m_CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(t.m_CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) || t.m_CLAMP.i64 == pt->m_CLAMP.i64)
			&& t.m_TEXA.TA0 == pt->m_TEXA.TA0 && t.m_TEXA.TA1 == pt->m_TEXA.TA1 && t.m_TEXA.AEM == pt->m_TEXA.AEM
			&& (!nPaletteEntries || t.m_TEX0.CPSM == pt->m_TEX0.CPSM && !memcmp(t.m_clut, pt->m_clut, nPaletteEntries*sizeof(t.m_clut[0]))))
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
		if(!SUCCEEDED(CreateTexture(s, tw0, th0, t, PSM_PSMCT32)))
			return false;

		RemoveOldTextures(s);

		pt = &m_TextureCache.GetAt(m_TextureCache.AddHead(t));

		lr = needsupdate;
	}

	if(lr == needsupdate)
	{
		if(!GetDirtySize(s, tw, th, pt))
		{
			lr = found;
		}
	}

	if(lr == needsupdate)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 updating texture %dx%d (%dx%d)\n"), tw, th, tw0, th0);
#endif
		RECT rlock = {0, 0, tw, th};

		D3DLOCKED_RECT r;
		if(FAILED(pt->m_pTexture->LockRect(0, &r, tw == tw0 && th == th0 ? NULL : &rlock, 0/*D3DLOCK_NO_DIRTY_UPDATE*/)))
			return(false);

		s->m_lm.ReadTexture(tw, th, (BYTE*)r.pBits, r.Pitch, t.m_TEX0, t.m_TEXA, t.m_CLAMP);
		s->m_stats.IncReads(tw*th*4);

		DWORD chksum = 0;

		BYTE* ptr = (BYTE*)r.pBits;
		for(int j = 0; j < th; j++, ptr += r.Pitch)
		{
			DWORD* p = (DWORD*)ptr;
			for(int i = 0; i < tw; i++, p++)
				chksum += *p;
		}

		pt->m_pTexture->UnlockRect(0);

		if(pt->m_chksumsize != CSize(tw, th) || pt->m_chksum != chksum)
		{
			pt->m_chksumsize = CSize(tw, th);
			pt->m_chksum = chksum;
			pt->m_pTexture->AddDirtyRect(&rlock);
			pt->m_pTexture->PreLoad();
			s->m_stats.IncTexWrite(tw*th*4);
		}
		else
		{
#ifdef DEBUG_LOG
			s->LOG(_T("*TC2 updating texture is not necessary!!! skipping AddDirtyRect\n"));
#endif
		}

		if(pt->m_valid.cx < tw) pt->m_valid.cx = tw;
		if(pt->m_valid.cy < th) pt->m_valid.cy = th;

		pt->m_dirty.RemoveAll();

		lr = found;

#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was updated, valid %dx%d\n"), pt->m_valid.cx, pt->m_valid.cy);
#endif
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		MoveToHead(pt);
		t = *pt;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchPal(GSState* s, GSTexture& t)
{
	t = GSTexture();

	t.m_TEX0 = s->m_ctxt->TEX0;
	t.m_CLAMP = s->m_ctxt->CLAMP;
	t.m_TEXA = s->m_de.TEXA;

	ASSERT(t.m_TEX0.TW <= 10 && t.m_TEX0.TH <= 10);
	int tw = 1 << t.m_TEX0.TW, tw0 = tw;
	int th = 1 << t.m_TEX0.TH, th0 = th;

	int nPaletteEntries = PaletteEntries(t.m_TEX0.PSM);

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05x %d (%d)\n"), tw, th, t.m_TEX0.TBP0, t.m_TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	GSTexture* pt = NULL;

	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos && !pt)
	{
		pt = &m_TextureCache.GetNext(pos);

		if(pt->m_TEX0.TBP0 == t.m_TEX0.TBP0)
		{
			if(pt->m_fRT)
			{
				lr = found;

				// FIXME: RT + 8h,4hl,4hh
				if(nPaletteEntries)
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
			else if(t.m_TEX0.PSM == pt->m_TEX0.PSM && t.m_TEX0.TW == pt->m_TEX0.TW && t.m_TEX0.TH == pt->m_TEX0.TH
			&& (!(t.m_CLAMP.WMS&2) && !(pt->m_CLAMP.WMS&2) && !(t.m_CLAMP.WMT&2) && !(pt->m_CLAMP.WMT&2) 
				|| t.m_CLAMP.i64 == pt->m_CLAMP.i64))
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
		if(!SUCCEEDED(CreateTexture(s, tw0, th0, t, t.m_TEX0.PSM)))
			return false;

		RemoveOldTextures(s);

		pt = &m_TextureCache.GetAt(m_TextureCache.AddHead(t));

		lr = needsupdate;
	}

	if(nPaletteEntries > 0 && pt->m_pPalette)
	{
		D3DLOCKED_RECT r;
		if(FAILED(pt->m_pPalette->LockRect(0, &r, NULL, 0)))
			return(false);
		s->m_lm.readCLUT(t.m_TEX0, t.m_TEXA, (DWORD*)r.pBits);
		pt->m_pPalette->UnlockRect(0);
		s->m_stats.IncTexWrite(256*4);
	}

	if(lr == needsupdate)
	{
		if(!GetDirtySize(s, tw, th, pt))
		{
			lr = found;
		}
	}

	if(lr == needsupdate)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 updating texture %dx%d (%dx%d)\n"), tw, th, tw0, th0);
#endif

		int bpp;

		switch(pt->m_TEX0.PSM)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			bpp = 32;
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			bpp = 16;
			break;
		case PSM_PSMT8:
		case PSM_PSMT4:
		case PSM_PSMT8H:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			bpp = 8;
			break;
		}

		RECT rlock = {0, 0, tw, th};

		D3DLOCKED_RECT r;
		if(FAILED(pt->m_pTexture->LockRect(0, &r, tw == tw0 && th == th0 ? NULL : &rlock, D3DLOCK_NO_DIRTY_UPDATE)))
			return(false);

		s->m_lm.ReadTextureP(tw, th, (BYTE*)r.pBits, r.Pitch, t.m_TEX0, t.m_CLAMP);
		s->m_stats.IncReads(tw*th*bpp>>3);

		DWORD chksum = 0;

		if(tw >= 4)
		{
			BYTE* ptr = (BYTE*)r.pBits;
			for(int j = 0, w = (tw>>2) * (bpp>>3); j < th; j++, ptr += r.Pitch)
				for(int i = 0; i < w; i++)
					chksum += ((DWORD*)ptr)[i];
		}

		pt->m_pTexture->UnlockRect(0);

		if(tw < 4 || pt->m_chksumsize != CSize(tw, th) || pt->m_chksum != chksum)
		{
			pt->m_chksumsize = CSize(tw, th);
			pt->m_chksum = chksum;
			pt->m_pTexture->AddDirtyRect(&rlock);
			pt->m_pTexture->PreLoad();
			s->m_stats.IncTexWrite(tw*th*bpp>>3);
		}
		else
		{
			//rlock.right = rlock.bottom = 1;
#ifdef DEBUG_LOG
			s->LOG(_T("*TC2 updating texture is not necessary!!! skipping AddDirtyRect\n"));
#endif
		}

		//pt->m_pTexture->AddDirtyRect(&rlock);
		//pt->m_pTexture->PreLoad();

		if(pt->m_valid.cx < tw) pt->m_valid.cx = tw;
		if(pt->m_valid.cy < th) pt->m_valid.cy = th;

		pt->m_dirty.RemoveAll();

		lr = found;

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


	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
		MoveToHead(pt);
		t = *pt;
		return true;
	}

	return false;
}

void GSTextureCache::IncAge(CSurfMap<IDirect3DTexture9>& pRTs)
{
	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture& t = m_TextureCache.GetNext(pos);
		t.m_nAge++;
		t.m_nVsyncs++;
		if(t.m_nAge > 10)
		{
			pRTs.RemoveKey(t.m_TEX0.TBP0);
			m_TextureCache.RemoveAt(cur);
		}
	}
}

void GSTextureCache::ResetAge(DWORD TBP0)
{
	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		GSTexture& t = m_TextureCache.GetNext(pos);
		if(t.m_TEX0.TBP0 == TBP0) t.m_nAge = 0;
	}
}

void GSTextureCache::RemoveAll()
{
	m_TextureCache.RemoveAll();
	m_pTexturePool32.RemoveAll();
	m_pTexturePool8.RemoveAll();
}

void GSTextureCache::InvalidateTexture(GSState* s, DWORD TBP0, DWORD PSM, CRect r)
{
#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 invalidate %05x (%dx%x - %dx%d)\n"), TBP0, r.left, r.top, r.right, r.bottom);
#endif

	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		const GSTexture& t = m_TextureCache.GetNext(pos);
		if(t.m_TEX0.TBP0 == TBP0 && t.m_fRT) 
			m_TextureCache.RemoveAt(cur);
	}

	pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		GSTexture& t = m_TextureCache.GetNext(pos);
		if(t.m_TEX0.TBP0 == TBP0)
			t.m_dirty.AddHead(GSDirtyRect(PSM, r));
	}
}

void GSTextureCache::InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, CRect r)
{
	CComPtr<IDirect3DTexture9> pRT;

	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		const GSTexture& t = m_TextureCache.GetNext(pos);
		if(t.m_TEX0.TBP0 == TBP0 && t.m_fRT) 
		{
			pRT = t.m_pTexture;
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
	POSITION pos = m_TextureCache.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		const GSTexture& t = m_TextureCache.GetNext(pos);
		if(t.m_TEX0.TBP0 == TBP0) 
			m_TextureCache.RemoveAt(cur);
	}

	GSTexture t;
	t.m_pTexture = pRT;
	t.m_pTexture->GetLevelDesc(0, &t.m_desc);
	t.m_TEX0.TBP0 = TBP0;
	t.m_scale = scale;
	// t.m_valid.SetSize();
	t.m_fRT = true;

	m_TextureCache.AddHead(t);
}
