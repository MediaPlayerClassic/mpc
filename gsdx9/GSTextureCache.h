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

// TODO: get rid of this *PrivateData

[uuid("5D5EFE0E-5407-4BCF-855D-C46CBCD075FA")]
struct scale_t
{
	float x, y;
	struct scale_t() {x = y = 1;}
	struct scale_t(float x, float y) {this->x = x; this->y = y;}
	struct scale_t(IDirect3DResource9* p) {Get(p);}
	bool operator == (const struct scale_t& s) {return fabs(x - s.x) < 0.001 && fabs(y - s.y) < 0.001;}
	void Set(IDirect3DResource9* p) {p->SetPrivateData(__uuidof(*this), this, sizeof(*this), 0);}
	void Get(IDirect3DResource9* p) {DWORD size = sizeof(*this); p->GetPrivateData(__uuidof(*this), this, &size);}
};

class GSDirtyRect
{
	DWORD m_PSM;
	CRect m_rcDirty;

public:
	GSDirtyRect() : m_PSM(PSM_PSMCT32), m_rcDirty(0, 0, 0, 0) {}
	GSDirtyRect(DWORD PSM, CRect r);
	CRect GetDirtyRect(const GIFRegTEX0& TEX0);
};

class GSDirtyRectList : public CAtlList<GSDirtyRect>
{
public:
	GSDirtyRectList() {}
	GSDirtyRectList(const GSDirtyRectList& l) {*this = l;}
	void operator = (const GSDirtyRectList& l);
	CRect GetDirtyRect(const GIFRegTEX0& TEX0);
};

struct GSTexture
{
	CComPtr<IDirect3DTexture9> m_pTexture, m_pPalette;
	GIFRegCLAMP m_CLAMP;
	GIFRegTEX0 m_TEX0;
	GIFRegTEXA m_TEXA;
	DWORD m_clut[256];
	scale_t m_scale;
	CSize m_valid;
	GSDirtyRectList m_dirty;
	bool m_fRT;
	int m_nAge, m_nVsyncs;
	CSize m_chksumsize;
	DWORD m_chksum;
	DWORD m_size;
	D3DSURFACE_DESC m_desc;

	GSTexture();
};

class GSState;

class GSTextureCache
{
protected:
	CInterfaceList<IDirect3DTexture9> m_pTexturePool8;
	CInterfaceList<IDirect3DTexture9> m_pTexturePool16;
	CInterfaceList<IDirect3DTexture9> m_pTexturePool32;

	typedef CAtlList<GSTexture> GSTextureList;
	GSTextureList m_TextureCache;

	HRESULT CreateTexture(GSState* s, int w, int h, GSTexture& t, DWORD PSM);
	bool IsTextureInCache(IDirect3DTexture9* pTexture);
	void RemoveOldTextures(GSState* s);
	bool GetDirtySize(GSState* s, int& tw, int& th, GSTexture* pt);
	void MoveToHead(GSTexture* pt);

public:
	GSTextureCache();

	bool Fetch(GSState* s, GSTexture& t);
	bool FetchPal(GSState* s, GSTexture& t);
	void IncAge(CSurfMap<IDirect3DTexture9>& pRTs);
	void ResetAge(DWORD TBP0);
	void RemoveAll();
	void InvalidateTexture(GSState* s, DWORD TBP0, DWORD PSM, CRect r);
	void InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, CRect r);
	void AddRT(DWORD TBP0, IDirect3DTexture9* pRT, scale_t scale);
};
