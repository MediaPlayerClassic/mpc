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

#pragma once

#include "GS.h"

template <class T> class CSurfMap : public CMap<DWORD, DWORD, CComPtr<T>, CComPtr<T>& > {};

inline bool IsRenderTarget(IDirect3DTexture9* pTexture)
{
	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	return pTexture && S_OK == pTexture->GetLevelDesc(0, &desc) && (desc.Usage&D3DUSAGE_RENDERTARGET);
}

struct scale_t
{
	float x, y;
	struct scale_t() {x = y = 1;}
	struct scale_t(float x, float y) {this->x = x; this->y = y;}
	bool operator == (const struct scale_t& s) {return fabs(x - s.x) < 0.001 && fabs(y - s.y) < 0.001;}
};

class GSTexture
{
public:
	GIFRegTEX0 m_TEX0;
	GIFRegCLAMP m_CLAMP;
	GIFRegTEXA m_TEXA;
	scale_t m_scale;
	CComPtr<IDirect3DTexture9> m_pTexture;
	int m_age;

	class GSTexture();
	class GSTexture(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& m_scale, CComPtr<IDirect3DTexture9> pTexture);
};

class GSTextureCache : public CList<GSTexture>
{
public:
	GSTextureCache();
	void Add(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& m_scale, CComPtr<IDirect3DTexture9> pTexture);
	void Update(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& m_scale, CComPtr<IDirect3DTexture9> pTexture);
	POSITION Lookup(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, scale_t& m_scale, GSTexture& ret);
	POSITION Lookup(GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP, GIFRegTEXA& TEXA, GSTexture& ret);
	POSITION LookupByTBP(UINT32 TBP0, GSTexture& ret);
	POSITION LookupByCBP(UINT32 CBP, GSTexture& ret);
	void InvalidateByTBP(UINT32 TBP0);
	void InvalidateByCBP(UINT32 CBP);
	void IncAge(CSurfMap<IDirect3DTexture9>& pRTs);
	void ResetAge(UINT32 TBP0);
};