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

#include "GSRenderer.h"

template <class VERTEX>
class GSRendererSoft : public GSRenderer<VERTEX>
{
protected:
	void Reset();
	int DrawingKick(bool fSkip);
	void FlushPrim();
	void Flip();
	void EndFrame();
	void InvalidateTexture(DWORD TBP0);

	enum {PRIM_NONE, PRIM_SPRITE, PRIM_TRIANGLE, PRIM_LINE, PRIM_POINT} m_primtype;

	virtual void DrawPoint(VERTEX* v) = 0;
	virtual void DrawLine(VERTEX* v) = 0;
	virtual void DrawTriangle(VERTEX* v) = 0;
	virtual void DrawSprite(VERTEX* v) = 0;
	virtual void DrawVertex(int x, int y, VERTEX& v);
	virtual bool DrawFilledRect(int left, int top, int right, int bottom, VERTEX& v);

	class CTexture
	{
	public:
		GIFRegTEX0 m_TEX0;
		GIFRegTEXA m_TEXA;
		GIFRegTEXCLUT m_TEXCLUT;
		DWORD* m_pTexture, m_age;
		CTexture() {m_pTexture = NULL; m_age = 0;}
		~CTexture() {delete [] m_pTexture;}
	};

	CAutoPtrList<CTexture> m_tc;

	DWORD* m_pTexture;
	void SetTexture();
	bool LookupTexture(CTexture*& t);

	CRect m_scissor;
	virtual void SetScissor() = 0;

	BYTE m_clip[65536];
	BYTE m_mask[65536];
	BYTE* m_clamp;

public:
	GSRendererSoft(HWND hWnd, HRESULT& hr);
	~GSRendererSoft();

	void LOGVERTEX(GSSoftVertexFP& v, LPCTSTR type)
	{
		int tw = 1, th = 1;
		if(m_de.PRIM.TME) {tw = 1<<m_ctxt->TEX0.TW; th = 1<<m_ctxt->TEX0.TH;}
		LOG2((_T("\t %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%f, %f) (%f, %f)\n"), 
			type,
			v.x, v.y, v.z, v.q, 
			((BYTE)v.a << 24) | ((BYTE)v.r << 16) | ((BYTE)v.g << 8) | ((BYTE)v.b << 0),
			v.u, v.v, 
			v.u*tw, v.v*th));
	}

	void LOGVERTEX(GSSoftVertexFX& v, LPCTSTR type)
	{
		int tw = 1, th = 1;
		if(m_de.PRIM.TME) {tw = 1<<m_ctxt->TEX0.TW; th = 1<<m_ctxt->TEX0.TH;}
		LOG2((_T("\t %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%f, %f) (%f, %f)\n"), 
			type,
			(float)v.x/65536, (float)v.y/65536, (float)v.z/INT_MAX, (float)v.q/INT_MAX, 
			((v.a>>16)<<24) | ((v.r>>16)<<16) | ((v.g>>16)<<8) | ((v.b>>16)<<0),
			(float)v.u/INT_MAX, (float)v.v/INT_MAX, 
			(float)v.u/INT_MAX*tw, (float)v.v/INT_MAX*th));
	}
};

class GSRendererSoftFP : public GSRendererSoft<GSSoftVertexFP>
{
	typedef GSSoftVertexFP GSSoftVertex;

protected:
	void VertexKick(bool fSkip);

	void SetScissor();

	void DrawPoint(GSSoftVertex* v);
	void DrawLine(GSSoftVertex* v);
	void DrawTriangle(GSSoftVertex* v);
	void DrawSprite(GSSoftVertex* v);

public:
	GSRendererSoftFP(HWND hWnd, HRESULT& hr);
};

class GSRendererSoftFX : public GSRendererSoft<GSSoftVertexFX>
{
	typedef GSSoftVertexFX GSSoftVertex;

protected:
	void VertexKick(bool fSkip);

	void SetScissor();

	void DrawPoint(GSSoftVertex* v);
	void DrawLine(GSSoftVertex* v);
	void DrawTriangle(GSSoftVertex* v);
	void DrawSprite(GSSoftVertex* v);
	void DrawVertex(int x, int y, GSSoftVertex& v);

public:
	GSRendererSoftFX(HWND hWnd, HRESULT& hr);
};