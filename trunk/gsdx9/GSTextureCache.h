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
#include "GSScale.h"
#include "GSDirtyRect.h"

class GSState;

class GSTextureCache
{
public:
	class GSSurface
	{
	public:
		CComPtr<IDirect3DSurface9> m_surface;
		D3DSURFACE_DESC m_desc;
		GSScale m_scale;
		int m_age;
		int m_size;
		GIFRegTEX0 m_TEX0;

		GSSurface();
		virtual ~GSSurface();

		bool IsRenderTarget() const;
		bool IsDepthStencil() const;

		void Update(GSState* s);
	};

	class GSRenderTarget : public GSSurface
	{
	public:
		CComPtr<IDirect3DTexture9> m_texture;
		GSDirtyRectList m_dirty;

		GSRenderTarget();

		void Update(GSState* s, GSTextureCache* tc);
	};

	class GSDepthStencil : public GSSurface
	{
	public:
		GSDepthStencil();
	};	
	
	class GSTexture : public GSSurface
	{
		DWORD Hash();
		bool GetDirtyRect(GSState* s, CRect& r);

	public:
		CComPtr<IDirect3DTexture9> m_texture;
		CComPtr<IDirect3DTexture9> m_palette;
		GIFRegCLAMP m_CLAMP;
		GIFRegTEXA m_TEXA; // *
		DWORD m_clut[256]; // *
		GSDirtyRectList m_dirty;
		CRect m_valid;
		DWORD m_hash;
		DWORD m_hashdiff;
		CRect m_hashrect;
		int m_bpp;

		GSTexture();

		void Update(GSState* s, GSLocalMemory::readTexture rt);
	};

protected:
	CAtlList<GSRenderTarget*> m_rt;
	CAtlList<GSDepthStencil*> m_ds;
	CAtlList<GSTexture*> m_tex;
	CInterfaceList<IDirect3DSurface9> m_pool;

	template<class T> void RecycleByAge(CAtlList<T*>& l, int maxage = 10);
	template<class T> void RecycleBySize(CAtlList<T*>& l, int maxsize = 50*1024*1024);
	
	void Recycle(IDirect3DSurface9* surface);
	void Recycle(GSSurface* s, bool del = true);
	void Recycle(GSTexture* t, bool del = true);

	bool Create(GSState* s, GSRenderTarget* rt, int w, int h);
	bool Create(GSState* s, GSDepthStencil* ds, int w, int h);
	bool Create(GSState* s, GSTexture* t, DWORD psm, DWORD cpsm = PSM_PSMCT32);

	GSTexture* Convert(GSState* s, GSRenderTarget* rt);

	HRESULT CreateRenderTarget(GSState* s, int w, int h, IDirect3DTexture9** ppt, IDirect3DSurface9** pps = NULL, D3DSURFACE_DESC* desc = NULL);
	HRESULT CreateDepthStencil(GSState* s, int w, int h, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc = NULL);
	HRESULT CreateTexture(GSState* s, int w, int h, D3DFORMAT format, IDirect3DTexture9** ppt, IDirect3DSurface9** pps = NULL, D3DSURFACE_DESC* desc = NULL);

public:
	GSTextureCache();
	virtual ~GSTextureCache();

	void RemoveAll();

	GSRenderTarget* GetRenderTarget(GSState* s, const GIFRegTEX0& TEX0, int w, int h, bool fb = false);
	GSDepthStencil* GetDepthStencil(GSState* s, const GIFRegTEX0& TEX0, int w, int h);
	GSTexture* GetTextureNP(GSState* s);

	void InvalidateTexture(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);
	void InvalidateLocalMem(GSState* s, const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);

	void IncAge();
};
