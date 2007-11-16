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

#pragma once

#include "GS.h"
#include "GSLocalMemory.h"
#include "GSTexture.h"

template <class T> class CSurfMap : public CAtlMap<DWORD, CComPtr<T> > {};

extern bool HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm);

class GSState;

class GSTextureCache : public CAtlList<GSTexture*>
{
protected:
	CInterfaceList<IDirect3DTexture9> m_pTexturePool;
	CInterfaceList<IDirect3DTexture9> m_pRenderTargetPool;

	HRESULT CreateRenderTarget(GSState* s, int w, int h, IDirect3DTexture9** pprt);
	HRESULT CreateTexture(GSState* s, GSTexture* t, DWORD psm, DWORD cpsm = PSM_PSMCT32);
	bool IsTextureInCache(IDirect3DTexture9* t);
	void RemoveOld(GSState* s);
	bool GetDirtyRect(GSState* s, GSTexture* pt, CRect& r);

	GSTexture* ConvertRT(GSState* s, GSTexture* pt);
	GSTexture* ConvertRTPitch(GSState* s, GSTexture* pt);
	GSTexture* ConvertRTWidthHeight(GSState* s, GSTexture* pt);

public:
	GSTextureCache();
	virtual ~GSTextureCache();

	void RemoveAll();

	bool Fetch(GSState* s, GSTextureBase& tb);
	bool FetchP(GSState* s, GSTextureBase& tb);
	bool FetchNP(GSState* s, GSTextureBase& tb);

	void IncAge(CSurfMap<IDirect3DTexture9>& pRTs);
	void ResetAge(DWORD TBP0);
	void InvalidateTexture(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);
	void InvalidateLocalMem(GSState* s, DWORD TBP0, DWORD BW, DWORD PSM, const CRect& r);
	void AddRT(GIFRegTEX0& TEX0, IDirect3DTexture9* pRT, GSScale scale);
};
