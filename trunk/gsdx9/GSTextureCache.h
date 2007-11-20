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
	protected:
		GSTextureCache* m_tc;

	public:
		CComPtr<IDirect3DSurface9> m_surface;
		D3DSURFACE_DESC m_desc;
		GSScale m_scale;
		int m_age;
		int m_size;
		GIFRegTEX0 m_TEX0;

		GSSurface(GSTextureCache* tc);
		virtual ~GSSurface();

		bool IsRenderTarget() const;
		bool IsDepthStencil() const;

		void Update();
	};

	class GSRenderTarget : public GSSurface
	{
	public:
		CComPtr<IDirect3DTexture9> m_texture;
		GSDirtyRectList m_dirty;
		bool m_used;

		GSRenderTarget(GSTextureCache* tc);

		bool Create(int w, int h);
		void Update();
		void Read(CRect r);
	};

	class GSDepthStencil : public GSSurface
	{
	public:
		GSDepthStencil(GSTextureCache* tc);

		bool Create(int w, int h);
		void Update();
	};	
	
	class GSTexture : public GSSurface
	{
		DWORD Hash();
		bool GetDirtyRect(CRect& r);

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

		GSTexture(GSTextureCache* tc);
		virtual ~GSTexture();

		bool Create();
		bool Create(GSRenderTarget* rt);
		bool Create(GSDepthStencil* ds);
		void Update(GSLocalMemory::readTexture rt);
	};

protected:
	GSState* m_state;
	CAtlList<GSRenderTarget*> m_rt;
	CAtlList<GSDepthStencil*> m_ds;
	CAtlList<GSTexture*> m_tex;
	CInterfaceList<IDirect3DSurface9> m_pool;
	CComPtr<IDirect3DPixelShader9> m_ps[2];
	bool m_nativeres;

	template<class T> void RecycleByAge(CAtlList<T*>& l, int maxage = 10);
	
	void Recycle(IDirect3DSurface9* surface);

	HRESULT CreateRenderTarget(int w, int h, IDirect3DTexture9** ppt, IDirect3DSurface9** pps = NULL, D3DSURFACE_DESC* desc = NULL);
	HRESULT CreateDepthStencil(int w, int h, IDirect3DSurface9** pps, D3DSURFACE_DESC* desc = NULL);
	HRESULT CreateTexture(int w, int h, D3DFORMAT format, IDirect3DTexture9** ppt, IDirect3DSurface9** pps = NULL, D3DSURFACE_DESC* desc = NULL);
	HRESULT CreateOffscreenPlainSurface(int w, int h, D3DFORMAT format, IDirect3DSurface9** pps = NULL, D3DSURFACE_DESC* desc = NULL);

public:
	GSTextureCache(GSState* state);
	virtual ~GSTextureCache();

	void Create();
	void RemoveAll();

	GSRenderTarget* GetRenderTarget(const GIFRegTEX0& TEX0, int w, int h, bool fb = false);
	GSDepthStencil* GetDepthStencil(const GIFRegTEX0& TEX0, int w, int h);
	GSTexture* GetTextureNP();

	void InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const CRect& r);

	void IncAge();
};
