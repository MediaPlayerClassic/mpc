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

#include "GSRenderer.h"

#define D3DFVF_HWVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1)

#pragma pack(push, 1)
__declspec(align(16)) union HWVERTEX
{
	struct
	{
		float x, y, z, rhw;
		D3DCOLOR color, fog;
		float tu, tv;
	};
	
	__m128i xmm[2];

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
	HWVERTEX& operator = (HWVERTEX& v)
	{
		xmm[0] = v.xmm[0];
		xmm[1] = v.xmm[1];
		return *this;
	}
#endif
};
#pragma pack(pop)

class GSRendererHW : public GSRenderer<HWVERTEX>
{
protected:
	CSurfMap<IDirect3DTexture9> m_pRenderTargets;
	CSurfMap<IDirect3DSurface9> m_pDepthStencils;
	CMap<DWORD, DWORD, CGSWnd*, CGSWnd*> m_pRenderWnds;

	GSTextureCache m_tc;

	void SetupTexture(const GSTextureBase& t, float tsx, float tsy);
	void SetupAlphaBlend();
	void SetupColorMask();
	void SetupZBuffer();
	void SetupAlphaTest();
	void SetupScissor(scale_t& s);

	void Reset();
	void VertexKick(bool fSkip);
	int DrawingKick(bool fSkip);
	void FlushPrim();
	void Flip();
	void EndFrame();
	void InvalidateTexture(DWORD TBP0, DWORD PSM, CRect r);
	void InvalidateLocalMem(DWORD BP, DWORD TBP0, DWORD PSM, CRect r);
	void MinMaxUV(int w, int h, CRect& r);

	D3DPRIMITIVETYPE m_primtype;

public:
	GSRendererHW(HWND hWnd, HRESULT& hr);
	~GSRendererHW();

	HRESULT ResetDevice(bool fForceWindowed = false);

	void LOGVERTEX(HWVERTEX& v, LPCTSTR type)
	{
		int tw = 1, th = 1;
		if(m_pPRIM->TME) {tw = 1<<m_ctxt->TEX0.TW; th = 1<<m_ctxt->TEX0.TH;}
		LOG2(_T("\t %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%f, %f) (%f, %f)\n"), 
			type,
			v.x, v.y, v.z, v.rhw, 
			v.color, v.tu, v.tv,
			v.tu*tw, v.tv*th);
	}
};