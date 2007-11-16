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
#include "GSTexture.h"
#include "GSHash.h"
#include "GSState.h"

GSTextureBase::GSTextureBase()
{
	m_scale = GSScale(1, 1);
	m_rt = false;
	memset(&m_desc, 0, sizeof(m_desc));
}

GSTexture::GSTexture()
{
	m_TEX0.TBP0 = ~0;
	m_valid = CRect(0, 0, 0, 0);
	m_dwHash = ~0;
	m_nHashDiff = 0;
	m_rcHash = CRect(0, 0, 0, 0);
	m_bytes = 0;
	m_age = 0;
	m_vsync = 0;
	m_bpp = 0;
}

DWORD GSTexture::Hash()
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

static bool IsRectInRect(const CRect& inner, const CRect& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right && outer.top <= inner.top && inner.bottom <= outer.bottom;
}

static bool IsRectInRectH(const CRect& inner, const CRect& outer)
{
	return outer.top <= inner.top && inner.bottom <= outer.bottom;
}

static bool IsRectInRectV(const CRect& inner, const CRect& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right;
}

bool GSTexture::GetDirtyRect(GSState* s, CRect& r)
{
	int w = 1 << m_TEX0.TW;
	int h = 1 << m_TEX0.TH;

	r.SetRect(0, 0, w, h);

	// FIXME: kyo's left hand after being selected for player one (PS2-SNK_Vs_Capcom_SVC_Chaos_PAL_CDFull.iso)
	// return true;

	s->MinMaxUV(w, h, r);

	CRect dirty = m_dirty.GetDirtyRect(m_TEX0);
	CRect valid = m_valid;

	if(IsRectInRect(r, valid))
	{
		if(dirty.IsRectEmpty()) return false;
		else if(IsRectInRect(dirty, r)) r = dirty;
		else if(IsRectInRect(dirty, valid)) r |= dirty;
		else r = valid | dirty;
	}
	else
	{
		if(IsRectInRectH(r, valid) && (r.left >= valid.left || r.right <= valid.right))
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
	}

	return true;
}

HRESULT GSTexture::Update(GSState* s, GSLocalMemory::readTexture rt)
{
	CRect r;

	if(!GetDirtyRect(s, r))
	{
		return S_OK;
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

		if((m_nHashDiff & limit) && m_nHashDiff >= limit && m_rcHash == m_valid) // predicted to be dirty
		{
			m_nHashDiff++;
		}
		else
		{
			DWORD dwHash = Hash();

			if(m_rcHash != m_valid)
			{
				m_nHashDiff = 0;
				m_rcHash = m_valid;
				m_dwHash = dwHash;
			}
			else
			{
				if(m_dwHash != dwHash)
				{
					m_nHashDiff++;
					m_dwHash = dwHash;
				}
				else
				{
					if(m_nHashDiff < limit) r.SetRect(0, 0, 1, 1);
					// else t->m_dwHash is not reliable, must update
					m_nHashDiff = 0;
				}
			}
		}

		m_texture->AddDirtyRect(&r);
		
		m_texture->PreLoad();

		s->m_perfmon.Put(GSPerfMon::Texture, r.Width() * r.Height() * m_bpp >> 3);
	}

	return S_OK;
}
