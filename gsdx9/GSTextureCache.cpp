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

GSTextureBase::GSTextureBase()
{
	m_scale = scale_t(1, 1);
	m_fRT = false;
	memset(&m_desc, 0, sizeof(m_desc));
}

GSTexture::GSTexture()
{
	m_TEX0.TBP0 = ~0;
	m_rcValid = CRect(0, 0, 0, 0);
	m_dwHash = ~0;
	m_nHashDiff = m_nHashSame = 0;
	m_rcHash = CRect(0, 0, 0, 0);
	m_nBytes = 0;
	m_nAge = 0;
	m_nVsyncs = 0;
}

//

GSTextureCache::GSTextureCache()
{
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();
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

	pt->m_nBytes = w*h*bpp>>3;

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
	DWORD nBytes = 0;

	POSITION pos = GetHeadPosition();
	while(pos) nBytes += GetNext(pos)->m_nBytes;

	pos = GetTailPosition();
	while(pos && nBytes > 96*1024*1024/*s->m_ddcaps.dwVidMemTotal*/)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 too many textures in cache (%d, %.2f MB)\n"), GetCount(), 1.0f*nBytes/1024/1024);
#endif
		POSITION cur = pos;

		GSTexture* pt = GetPrev(pos);
		if(!pt->m_fRT)
		{
			nBytes -= pt->m_nBytes;
			RemoveAt(cur);
			delete pt;
		}
	}
}

static bool RectInRect(const RECT& inner, const RECT& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right
		&& outer.top <= inner.top && inner.bottom <= outer.bottom;
}

static bool RectInRectH(const RECT& inner, const RECT& outer)
{
	return outer.top <= inner.top && inner.bottom <= outer.bottom;
}

static bool RectInRectV(const RECT& inner, const RECT& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right;
}

bool GSTextureCache::GetDirtyRect(GSState* s, GSTexture* pt, CRect& r)
{
	int w = 1 << pt->m_TEX0.TW;
	int h = 1 << pt->m_TEX0.TH;

	r.SetRect(0, 0, w, h);

// FIXME: kyo's left hand after being selected for player one (PS2-SNK_Vs_Capcom_SVC_Chaos_PAL_CDFull.iso)
// return true;

	s->MinMaxUV(w, h, r);

	CRect rcDirty = pt->m_rcDirty.GetDirtyRect(pt->m_TEX0);
	CRect rcValid = pt->m_rcValid;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 used %d,%d-%d,%d (%dx%d), valid %d,%d-%d,%d, dirty %d,%d-%d,%d\n"), r, w, h, rcValid, rcDirty);
#endif

	if(RectInRect(r, rcValid))
	{
		if(rcDirty.IsRectEmpty()) return false;
		else if(RectInRect(rcDirty, r)) r = rcDirty;
		else if(RectInRect(rcDirty, rcValid)) r |= rcDirty;
		else r = rcValid | rcDirty;
	}
	else
	{
		if(RectInRectH(r, rcValid) && (r.left >= rcValid.left || r.right <= rcValid.right))
		{
			r.top = rcValid.top;
			r.bottom = rcValid.bottom;
			if(r.left < rcValid.left) r.right = rcValid.left;
			else /*if(r.right > rcValid.right)*/ r.left = rcValid.right;
		}
		else if(RectInRectV(r, rcValid) && (r.top >= rcValid.top || r.bottom <= rcValid.bottom))
		{
			r.left = rcValid.left;
			r.right = rcValid.right;
			if(r.top < rcValid.top) r.bottom = rcValid.top;
			else /*if(r.bottom > rcValid.bottom)*/ r.top = rcValid.bottom;
		}
		else
		{
			r |= rcValid;
		}
	}

	return true;
}

static DWORD s_crctable[256] = 
{
	0x00000000l, 0x90910101l, 0x91210201l, 0x01b00300l,
	0x92410401l, 0x02d00500l, 0x03600600l, 0x93f10701l,
	0x94810801l, 0x04100900l, 0x05a00a00l, 0x95310b01l,
	0x06c00c00l, 0x96510d01l, 0x97e10e01l, 0x07700f00l,
	0x99011001l, 0x09901100l, 0x08201200l, 0x98b11301l,
	0x0b401400l, 0x9bd11501l, 0x9a611601l, 0x0af01700l,
	0x0d801800l, 0x9d111901l, 0x9ca11a01l, 0x0c301b00l,
	0x9fc11c01l, 0x0f501d00l, 0x0ee01e00l, 0x9e711f01l,
	0x82012001l, 0x12902100l, 0x13202200l, 0x83b12301l,
	0x10402400l, 0x80d12501l, 0x81612601l, 0x11f02700l,
	0x16802800l, 0x86112901l, 0x87a12a01l, 0x17302b00l,
	0x84c12c01l, 0x14502d00l, 0x15e02e00l, 0x85712f01l,
	0x1b003000l, 0x8b913101l, 0x8a213201l, 0x1ab03300l,
	0x89413401l, 0x19d03500l, 0x18603600l, 0x88f13701l,
	0x8f813801l, 0x1f103900l, 0x1ea03a00l, 0x8e313b01l,
	0x1dc03c00l, 0x8d513d01l, 0x8ce13e01l, 0x1c703f00l,
	0xb4014001l, 0x24904100l, 0x25204200l, 0xb5b14301l,
	0x26404400l, 0xb6d14501l, 0xb7614601l, 0x27f04700l,
	0x20804800l, 0xb0114901l, 0xb1a14a01l, 0x21304b00l,
	0xb2c14c01l, 0x22504d00l, 0x23e04e00l, 0xb3714f01l,
	0x2d005000l, 0xbd915101l, 0xbc215201l, 0x2cb05300l,
	0xbf415401l, 0x2fd05500l, 0x2e605600l, 0xbef15701l,
	0xb9815801l, 0x29105900l, 0x28a05a00l, 0xb8315b01l,
	0x2bc05c00l, 0xbb515d01l, 0xbae15e01l, 0x2a705f00l,
	0x36006000l, 0xa6916101l, 0xa7216201l, 0x37b06300l,
	0xa4416401l, 0x34d06500l, 0x35606600l, 0xa5f16701l,
	0xa2816801l, 0x32106900l, 0x33a06a00l, 0xa3316b01l,
	0x30c06c00l, 0xa0516d01l, 0xa1e16e01l, 0x31706f00l,
	0xaf017001l, 0x3f907100l, 0x3e207200l, 0xaeb17301l,
	0x3d407400l, 0xadd17501l, 0xac617601l, 0x3cf07700l,
	0x3b807800l, 0xab117901l, 0xaaa17a01l, 0x3a307b00l,
	0xa9c17c01l, 0x39507d00l, 0x38e07e00l, 0xa8717f01l,
	0xd8018001l, 0x48908100l, 0x49208200l, 0xd9b18301l,
	0x4a408400l, 0xdad18501l, 0xdb618601l, 0x4bf08700l,
	0x4c808800l, 0xdc118901l, 0xdda18a01l, 0x4d308b00l,
	0xdec18c01l, 0x4e508d00l, 0x4fe08e00l, 0xdf718f01l,
	0x41009000l, 0xd1919101l, 0xd0219201l, 0x40b09300l,
	0xd3419401l, 0x43d09500l, 0x42609600l, 0xd2f19701l,
	0xd5819801l, 0x45109900l, 0x44a09a00l, 0xd4319b01l,
	0x47c09c00l, 0xd7519d01l, 0xd6e19e01l, 0x46709f00l,
	0x5a00a000l, 0xca91a101l, 0xcb21a201l, 0x5bb0a300l,
	0xc841a401l, 0x58d0a500l, 0x5960a600l, 0xc9f1a701l,
	0xce81a801l, 0x5e10a900l, 0x5fa0aa00l, 0xcf31ab01l,
	0x5cc0ac00l, 0xcc51ad01l, 0xcde1ae01l, 0x5d70af00l,
	0xc301b001l, 0x5390b100l, 0x5220b200l, 0xc2b1b301l,
	0x5140b400l, 0xc1d1b501l, 0xc061b601l, 0x50f0b700l,
	0x5780b800l, 0xc711b901l, 0xc6a1ba01l, 0x5630bb00l,
	0xc5c1bc01l, 0x5550bd00l, 0x54e0be00l, 0xc471bf01l,
	0x6c00c000l, 0xfc91c101l, 0xfd21c201l, 0x6db0c300l,
	0xfe41c401l, 0x6ed0c500l, 0x6f60c600l, 0xfff1c701l,
	0xf881c801l, 0x6810c900l, 0x69a0ca00l, 0xf931cb01l,
	0x6ac0cc00l, 0xfa51cd01l, 0xfbe1ce01l, 0x6b70cf00l,
	0xf501d001l, 0x6590d100l, 0x6420d200l, 0xf4b1d301l,
	0x6740d400l, 0xf7d1d501l, 0xf661d601l, 0x66f0d700l,
	0x6180d800l, 0xf111d901l, 0xf0a1da01l, 0x6030db00l,
	0xf3c1dc01l, 0x6350dd00l, 0x62e0de00l, 0xf271df01l,
	0xee01e001l, 0x7e90e100l, 0x7f20e200l, 0xefb1e301l,
	0x7c40e400l, 0xecd1e501l, 0xed61e601l, 0x7df0e700l,
	0x7a80e800l, 0xea11e901l, 0xeba1ea01l, 0x7b30eb00l,
	0xe8c1ec01l, 0x7850ed00l, 0x79e0ee00l, 0xe971ef01l,
	0x7700f000l, 0xe791f101l, 0xe621f201l, 0x76b0f300l,
	0xe541f401l, 0x75d0f500l, 0x7460f600l, 0xe4f1f701l,
	0xe381f801l, 0x7310f900l, 0x72a0fa00l, 0xe231fb01l,
	0x71c0fc00l, 0xe151fd01l, 0xe0e1fe01l, 0x7070ff00l
};

static DWORD s_crc(const CRect& r, int pitch, BYTE* p)
{
	DWORD hash = 0;

	for(int j = r.top; j < r.bottom; j++, p += pitch)
	{
		for(int i = r.left, x = 0; i < r.right; i++, x += 4)
		{
			hash = s_crctable[(hash ^ p[x+0]) & 0xff] ^ (hash >> 8); 
			hash = s_crctable[(hash ^ p[x+1]) & 0xff] ^ (hash >> 8); 
			hash = s_crctable[(hash ^ p[x+2]) & 0xff] ^ (hash >> 8); 
			hash = s_crctable[(hash ^ p[x+3]) & 0xff] ^ (hash >> 8); 
		}
	}

	return hash;
}

DWORD GSTextureCache::HashTexture(const CRect& r, int pitch, void* bits)
{
	// TODO: make the hash more unique

	BYTE* p = (BYTE*)bits;
	DWORD hash = r.left + r.right + r.top + r.bottom + pitch + *(BYTE*)bits;

	if(r.Width()*r.Height() <= 32*32)
	{
		if(r.Width() > 0)
		{
			hash += s_crc(r, pitch, p);
		}
	}
	else
	{
#if defined(_M_AMD64) || _M_IX86_FP >= 2
		if(r.Width() >= 4 && !((DWORD_PTR)bits & 0xf))
		{
			__m128i hash128 = _mm_setzero_si128();
			for(int j = r.top; j < r.bottom; j++, p += pitch)
			{
				hash128 = _mm_add_epi32(hash128, _mm_set1_epi32(j));
				for(int i = r.left, x = 0; i < r.right; i += 4, x++)
				{
					hash128 = _mm_shuffle_epi32(hash128, 0x93);
					////hash128 = _mm_shufflelo_epi16(hash128, 0x93);
					//hash128 = _mm_shufflehi_epi16(hash128, 0x93);
					hash128 = _mm_add_epi32(hash128, _mm_load_si128(&((__m128i*)p)[x]));
				}
			}
			hash128 = _mm_add_epi32(hash128, _mm_srli_si128(hash128, 8));
			hash128 = _mm_add_epi32(hash128, _mm_srli_si128(hash128, 4));
			hash += _mm_cvtsi128_si32(hash128);
		}
		else 
#endif
		if(r.Width() > 0)
		{
			for(int j = r.top; j < r.bottom; j++, p += pitch)
			{
				hash += j;
				for(int i = r.left, x = 0; i < r.right; i++, x++)
				{
					hash += ((DWORD*)p)[x];
				}
			}
		}
	}

	return hash;
}

HRESULT GSTextureCache::UpdateTexture(GSState* s, GSTexture* pt, GSLocalMemory::readTexture rt)
{
	CRect r;
	if(!GetDirtyRect(s, pt, r))
		return S_OK;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 updating texture %d,%d-%d,%d (%dx%d)\n"), r.left, r.top, r.right, r.bottom, 1 << pt->m_TEX0.TW, 1 << pt->m_TEX0.TH);
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

	D3DLOCKED_RECT lr;
	if(FAILED(pt->m_pTexture->LockRect(0, &lr, &r, D3DLOCK_NO_DIRTY_UPDATE))) {ASSERT(0); return E_FAIL;}
	(s->m_lm.*rt)(r, (BYTE*)lr.pBits, lr.Pitch, s->m_ctxt->TEX0, s->m_de.TEXA, s->m_ctxt->CLAMP);
	s->m_perfmon.IncCounter(GSPerfMon::c_unswizzle, r.Width()*r.Height()*bpp>>3);
	pt->m_pTexture->UnlockRect(0);

	pt->m_rcValid |= r;
	pt->m_rcDirty.RemoveAll();

	const static DWORD limit = 7;

	if((pt->m_nHashDiff & limit) && pt->m_nHashDiff >= limit && pt->m_rcHash == pt->m_rcValid) // predicted to be dirty
	{
		pt->m_nHashDiff++;
	}
	else
	{
		if(FAILED(pt->m_pTexture->LockRect(0, &lr, &pt->m_rcValid, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY))) {ASSERT(0); return E_FAIL;}
		DWORD dwHash = HashTexture(
			CRect((pt->m_rcValid.left>>2)*(bpp>>3), pt->m_rcValid.top, (pt->m_rcValid.right>>2)*(bpp>>3), pt->m_rcValid.bottom), 
			lr.Pitch, lr.pBits);
		pt->m_pTexture->UnlockRect(0);

		if(pt->m_rcHash != pt->m_rcValid)
		{
			pt->m_nHashDiff = 0;
			pt->m_nHashSame = 0;
			pt->m_rcHash = pt->m_rcValid;
			pt->m_dwHash = dwHash;
		}
		else
		{
			if(pt->m_dwHash != dwHash)
			{
				pt->m_nHashDiff++;
				pt->m_nHashSame = 0;
				pt->m_dwHash = dwHash;
			}
			else
			{
				if(pt->m_nHashDiff < limit) r.SetRect(0, 0, 1, 1);
				// else pt->m_dwHash is not reliable, must update
				pt->m_nHashDiff = 0;
				pt->m_nHashSame++;
			}
		}
	}

	pt->m_pTexture->AddDirtyRect(&r);
	pt->m_pTexture->PreLoad();
	s->m_perfmon.IncCounter(GSPerfMon::c_texture, r.Width()*r.Height()*bpp>>3);

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 texture was updated, valid %d,%dx%d,%d\n"), pt->m_rcValid);
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
	s->LOG(_T("*TC2 Fetch %dx%d %05I64x %I64d (%d)\n"), 
		1 << s->m_ctxt->TEX0.TW, 1 << s->m_ctxt->TEX0.TH, 
		s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		POSITION cur = pos;
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

		if(lr != notfound) {MoveToHead(cur); break;}

		pt = NULL;
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
		t = *pt;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchP(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05I64x %I64d (%d)\n"), 
		1 << s->m_ctxt->TEX0.TW, 1 << s->m_ctxt->TEX0.TH, 
		s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, PaletteEntries(s->m_ctxt->TEX0.PSM));
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		POSITION cur = pos;
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

		if(lr != notfound) {MoveToHead(cur); break;}
		
		pt = NULL;
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
		s->m_perfmon.IncCounter(GSPerfMon::c_texture, 256*4);
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
		t = *pt;
		return true;
	}

	return false;
}

bool GSTextureCache::FetchNP(GSState* s, GSTextureBase& t)
{
	GSTexture* pt = NULL;

	int nPaletteEntries = PaletteEntries(s->m_ctxt->TEX0.PSM);

	DWORD clut[256];

	if(nPaletteEntries)
	{
		s->m_lm.SetupCLUT(s->m_ctxt->TEX0, s->m_de.TEXA);
		s->m_lm.CopyCLUT32(clut, nPaletteEntries);
	}

#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 Fetch %dx%d %05I64x %I64d (%d)\n"), 
		1 << s->m_ctxt->TEX0.TW, 1 << s->m_ctxt->TEX0.TH, 
		s->m_ctxt->TEX0.TBP0, s->m_ctxt->TEX0.PSM, nPaletteEntries);
#endif

	enum lookupresult {notfound, needsupdate, found} lr = notfound;

	POSITION pos = GetHeadPosition();
	while(pos && !pt)
	{
		POSITION cur = pos;
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
			// && s->m_de.TEXA.TA0 == pt->m_TEXA.TA0 && s->m_de.TEXA.TA1 == pt->m_TEXA.TA1 && s->m_de.TEXA.AEM == pt->m_TEXA.AEM
			&& (!nPaletteEntries || s->m_ctxt->TEX0.CPSM == pt->m_TEX0.CPSM && !memcmp(pt->m_clut, clut, nPaletteEntries*sizeof(clut[0]))))
			{
				lr = needsupdate;
			}
		}

		if(lr != notfound) {MoveToHead(cur); break;}

		pt = NULL;
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

		DWORD psm = s->m_ctxt->TEX0.PSM;

		switch(psm)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			psm = s->m_ctxt->TEX0.CPSM;
			break;
		}

		if(!SUCCEEDED(CreateTexture(s, pt, psm)))
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
		UpdateTexture(s, pt, &GSLocalMemory::ReadTextureNP);

		lr = found;
	}

	if(lr == found)
	{
#ifdef DEBUG_LOG
		s->LOG(_T("*TC2 texture was found, age %d -> 0\n"), pt->m_nAge);
#endif
		pt->m_nAge = 0;
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

void GSTextureCache::InvalidateTexture(GSState* s, DWORD TBP0, DWORD PSM, const CRect& r)
{
#ifdef DEBUG_LOG
	s->LOG(_T("*TC2 invalidate %05x (%d,%d-%d,%d)\n"), TBP0, r.left, r.top, r.right, r.bottom);
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
			pt->m_rcDirty.AddHead(GSDirtyRect(PSM, r));
	}
}

void GSTextureCache::InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, const CRect& r)
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
	pt->m_TEX0.TBP0 = TBP0;
	pt->m_pTexture = pRT;
	pt->m_pTexture->GetLevelDesc(0, &pt->m_desc);
	pt->m_scale = scale;
	pt->m_fRT = true;

	AddHead(pt);
}
