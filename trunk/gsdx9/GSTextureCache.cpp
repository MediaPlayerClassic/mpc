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

GSTexture::GSTexture()
	: m_scale(1, 1), m_valid(0, 0)
{
	m_tex.TEX0.i64 = -1;
	m_tex.CLAMP.i64 = 0;
	m_tex.TEXA.i64 = -1;
	m_tex.TEXCLUT.i64 = -1;
	m_age = 0;
	m_fRT = false;
}

GSTexture::GSTexture(tex_t& tex, scale_t& scale, CComPtr<IDirect3DTexture9> pTexture, CSize valid)
	: m_tex(tex), m_scale(scale), m_pTexture(pTexture), m_age(0), m_valid(valid)
{
	ASSERT(pTexture);
	m_fRT = IsRenderTarget(m_pTexture);
}

//

GSTextureCache::GSTextureCache()
{
}

void GSTextureCache::Add(tex_t& tex, scale_t& scale, CComPtr<IDirect3DTexture9> pTexture, CSize valid)
{
	InvalidateByTBP(tex.TEX0.TBP0);
	InvalidateByCBP(tex.TEX0.TBP0);
	AddHead(GSTexture(tex, scale, pTexture, valid));
}

void GSTextureCache::Update(tex_t& tex, scale_t& scale, CComPtr<IDirect3DTexture9> pTexture)
{
	GSTexture t;
	if(Lookup(tex, scale, t)) return;
	Add(tex, scale, pTexture);
}

POSITION GSTextureCache::Lookup(tex_t& tex, scale_t& scale, GSTexture& ret)
{
	for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
	{
		GSTexture& t = GetAt(pos);

		if(t.m_tex.TEX0.TBP0 == tex.TEX0.TBP0 && t.m_fRT && t.m_scale == scale)
		{
			t.m_age = 0;
			ret = t;
			return pos;
		}

		if(t.m_tex.TEX0.TBP0 == tex.TEX0.TBP0 && t.m_tex.TEX0.PSM == tex.TEX0.PSM && (tex.TEX0.PSM <= PSM_PSMCT16S || t.m_tex.TEX0.CBP == tex.TEX0.CBP)
		&& t.m_tex.TEX0.TW == tex.TEX0.TW && t.m_tex.TEX0.TH == tex.TEX0.TH
		&& (!(t.m_tex.CLAMP.WMS&2) && !(tex.CLAMP.WMS&2) && !(t.m_tex.CLAMP.WMT&2) && !(tex.CLAMP.WMT&2) || t.m_tex.CLAMP.i64 == tex.CLAMP.i64)
		&& t.m_tex.TEXA.TA0 == tex.TEXA.TA0 && t.m_tex.TEXA.TA1 == tex.TEXA.TA1 && t.m_tex.TEXA.AEM == tex.TEXA.AEM
		&& (t.m_tex.TEX0.PSM <= PSM_PSMCT16S || t.m_tex.TEXCLUT.COU == tex.TEXCLUT.COU && t.m_tex.TEXCLUT.COV == tex.TEXCLUT.COV && t.m_tex.TEXCLUT.CBW == tex.TEXCLUT.CBW)
		&& t.m_scale == scale)
		{
			t.m_age = 0;
			ret = t;
			return pos;
		}
	}

	return NULL;
}

POSITION GSTextureCache::Lookup(tex_t& tex, GSTexture& ret)
{
	for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
	{
		GSTexture& t = GetAt(pos);

		if(t.m_tex.TEX0.TBP0 == tex.TEX0.TBP0 && t.m_fRT)
		{
			t.m_age = 0;
			ret = t;
			return pos;
		}

		if(t.m_tex.TEX0.TBP0 == tex.TEX0.TBP0 && t.m_tex.TEX0.PSM == tex.TEX0.PSM && (tex.TEX0.PSM <= PSM_PSMCT16S || t.m_tex.TEX0.CBP == tex.TEX0.CBP)
		&& t.m_tex.TEX0.TW == tex.TEX0.TW && t.m_tex.TEX0.TH == tex.TEX0.TH
		&& (!(t.m_tex.CLAMP.WMS&2) && !(tex.CLAMP.WMS&2) && !(t.m_tex.CLAMP.WMT&2) && !(tex.CLAMP.WMT&2) || t.m_tex.CLAMP.i64 == tex.CLAMP.i64)
		&& t.m_tex.TEXA.TA0 == tex.TEXA.TA0 && t.m_tex.TEXA.TA1 == tex.TEXA.TA1 && t.m_tex.TEXA.AEM == tex.TEXA.AEM
		&& (t.m_tex.TEX0.PSM <= PSM_PSMCT16S || t.m_tex.TEXCLUT.COU == tex.TEXCLUT.COU && t.m_tex.TEXCLUT.COV == tex.TEXCLUT.COV && t.m_tex.TEXCLUT.CBW == tex.TEXCLUT.CBW))
		{
			t.m_age = 0;
			ret = t;
			return pos;
		}
	}

	return NULL;
}

POSITION GSTextureCache::LookupByTBP(UINT32 TBP0, GSTexture& ret)
{
	for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
	{
		GSTexture& t = GetAt(pos);
		if(t.m_tex.TEX0.TBP0 == TBP0)
		{
			ret = t;
			return pos;
		}
	}

	return NULL;
}

POSITION GSTextureCache::LookupByCBP(UINT32 CBP, GSTexture& ret)
{
	for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
	{
		GSTexture& t = GetAt(pos);
		if(t.m_tex.TEX0.CBP == CBP)
		{
			ret = t;
			return pos;
		}
	}
	return NULL;
}

void GSTextureCache::InvalidateByTBP(UINT32 TBP0)
{
	GSTexture t;
	while(POSITION pos = LookupByTBP(TBP0, t))
		RemoveAt(pos);
}

void GSTextureCache::InvalidateByCBP(UINT CBP)
{
	if(CBP == 0) return;

	GSTexture t;
	while(POSITION pos = LookupByCBP(CBP, t))
		RemoveAt(pos);
}

void GSTextureCache::IncAge(CSurfMap<IDirect3DTexture9>& pRTs)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		GSTexture& t = GetNext(pos);
		if(++t.m_age > 3)
		{
			pRTs.RemoveKey(t.m_tex.TEX0.TBP0);
			RemoveAt(cur);
		}
	}
}

void GSTextureCache::ResetAge(UINT32 TBP0)
{
	POSITION pos = GetHeadPosition();
	while(pos)
	{
		GSTexture& t = GetNext(pos);
		if(t.m_tex.TEX0.TBP0 == TBP0) t.m_age = 0;
	}
}
