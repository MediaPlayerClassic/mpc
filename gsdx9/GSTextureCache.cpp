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

GSTexture::GSTexture() : m_scale(1, 1)
{
	m_TEX0.TBP0 = m_TEX0.CBP = m_TEX0.PSM = -1;
	m_CLAMP.WMS = m_CLAMP.WMT = 0; m_CLAMP.MINU = m_CLAMP.MINV = m_CLAMP.MAXU = m_CLAMP.MAXV = 0;
	m_TEXA.TA0 = m_TEXA.TA1 = m_TEXA.AEM = -1;
	m_age = 0;
}

GSTexture::GSTexture(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& scale, CComPtr<IDirect3DTexture9> pTexture)
	: m_TEX0(TEX0), m_CLAMP(CLAMP), m_TEXA(TEXA)
	, m_scale(scale), m_pTexture(pTexture)
	, m_age(0)
{
	ASSERT(pTexture);
}

//

GSTextureCache::GSTextureCache()
{
}

void GSTextureCache::Add(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& scale, CComPtr<IDirect3DTexture9> pTexture)
{
	InvalidateByTBP(TEX0.TBP0);
	InvalidateByCBP(TEX0.TBP0);
	AddHead(GSTexture(TEX0, CLAMP, TEXA, scale, pTexture));
}

void GSTextureCache::Update(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& scale, CComPtr<IDirect3DTexture9> pTexture)
{
	GSTexture t;
	if(Lookup(TEX0, CLAMP, TEXA, scale, t)) return;
	Add(TEX0, CLAMP, TEXA, scale, pTexture);
}

POSITION GSTextureCache::Lookup(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& scale, GSTexture& ret)
{
	for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
	{
		GSTexture& t = GetAt(pos);
		bool fRT = IsRenderTarget(t.m_pTexture);
		if(t.m_TEX0.TBP0 == TEX0.TBP0 && (TEX0.PSM <= PSM_PSMCT16S || t.m_TEX0.CBP == TEX0.CBP)
		&& (t.m_TEX0.PSM == TEX0.PSM || fRT && TEX0.PSM <= PSM_PSMCT16S)
		&& (t.m_TEX0.PSM == PSM_PSMCT32 || t.m_TEXA.TA0 == TEXA.TA0 && t.m_TEXA.TA1 == TEXA.TA1 && t.m_TEXA.AEM == TEXA.AEM)
		&& (fRT || !(t.m_CLAMP.WMS&2) && !(CLAMP.WMS&2) && !(t.m_CLAMP.WMT&2) && !(CLAMP.WMT&2) || t.m_CLAMP.i64 == CLAMP.i64)
		&& t.m_scale == scale)
		{
			t.m_age = 0;
			ret = t;
			return pos;
		}
	}
	return NULL;
}

POSITION GSTextureCache::Lookup(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, GSTexture& ret)
{
	for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
	{
		GSTexture& t = GetAt(pos);
		bool fRT = IsRenderTarget(t.m_pTexture);
		if(t.m_TEX0.TBP0 == TEX0.TBP0 && (TEX0.PSM <= PSM_PSMCT16S || t.m_TEX0.CBP == TEX0.CBP)
		&& (t.m_TEX0.PSM == TEX0.PSM || fRT && TEX0.PSM <= PSM_PSMCT16S)
		&& (fRT || !(t.m_CLAMP.WMS&2) && !(CLAMP.WMS&2) && !(t.m_CLAMP.WMT&2) && !(CLAMP.WMT&2) || t.m_CLAMP.i64 == CLAMP.i64)
		&& (t.m_TEXA.TA0 == TEXA.TA0 || t.m_TEXA.TA1 == TEXA.TA1 || t.m_TEXA.AEM == TEXA.AEM))
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
		if(t.m_TEX0.TBP0 == TBP0)
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
		if(t.m_TEX0.CBP == CBP)
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
		if(++t.m_age > 2)
		{
			TRACE(_T("Removing texture: %05x\n"), t.m_TEX0.TBP0);
			pRTs.RemoveKey(t.m_TEX0.TBP0);
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
		if(t.m_TEX0.TBP0 == TBP0) t.m_age = 0;
	}
}
