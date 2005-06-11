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

template <class Vertex>
class GSRendererSoft : public GSRenderer<Vertex>
{
protected:
	void Reset();
	int DrawingKick(bool fSkip);
	void FlushPrim();
	void Flip();
	void EndFrame();

	enum {PRIM_NONE, PRIM_SPRITE, PRIM_TRIANGLE, PRIM_LINE, PRIM_POINT} m_primtype;

	void DrawPoint(Vertex* v);
	void DrawLine(Vertex* v);
	void DrawTriangle(Vertex* v);
	void DrawSprite(Vertex* v);
	bool DrawFilledRect(int left, int top, int right, int bottom, const Vertex& v);

	virtual void DrawVertex(int x, int y, const Vertex& v);
	virtual void DrawVertexTFX(typename Vertex::Vector& Cf, const Vertex& v);

	CComPtr<IDirect3DTexture9> m_pRT[2];

	DWORD* m_pTexture;
	void SetupTexture();

	struct uv_wrap_t {union {struct {short min[8], max[8];}; struct {short and[8], or[8];};}; unsigned short mask[8];}* m_uv;

	CRect m_scissor;
	BYTE m_clip[65536];
	BYTE m_mask[65536];
	BYTE* m_clamp;

public:
	GSRendererSoft(HWND hWnd, HRESULT& hr);
	~GSRendererSoft();

	HRESULT ResetDevice(bool fForceWindowed = false);

	void LOGVERTEX(GSSoftVertexFP& v, LPCTSTR type)
	{
		int tw = 1, th = 1;
		if(m_de.PRIM.TME) {tw = 1<<m_ctxt->TEX0.TW; th = 1<<m_ctxt->TEX0.TH;}
		LOG2(_T("- %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%.3f, %.3f) (%.2f, %.2f)\n"), 
			type,
			v.p.x, v.p.y, v.p.z / UINT_MAX, v.t.q, 
			(DWORD)v.c,
			v.t.x, v.t.y, v.t.x*tw, v.t.y*th);
	}
/*
	void LOGVERTEX(GSSoftVertexFX& v, LPCTSTR type)
	{
		int tw = 1, th = 1;
		if(m_de.PRIM.TME) {tw = 1<<m_ctxt->TEX0.TW; th = 1<<m_ctxt->TEX0.TH;}
		LOG2(_T("- %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%f, %f) (%f, %f)\n"), 
			type,
			(float)v.x/65536, (float)v.y/65536, (float)v.z/INT_MAX, (float)v.q/INT_MAX, 
			((v.a>>16)<<24) | ((v.r>>16)<<16) | ((v.g>>16)<<8) | ((v.b>>16)<<0),
			(float)v.u/INT_MAX, (float)v.v/INT_MAX, 
			(float)v.u/INT_MAX*tw, (float)v.v/INT_MAX*th);
	}
*/
};

class GSRendererSoftFP : public GSRendererSoft<GSSoftVertexFP>
{
protected:
	void VertexKick(bool fSkip);

public:
	GSRendererSoftFP(HWND hWnd, HRESULT& hr);
};
/*
class GSRendererSoftFX : public GSRendererSoft<GSSoftVertexFX>
{
protected:
	void VertexKick(bool fSkip);
	//void DrawVertex(int x, int y, GSSoftVertexFX& v);

public:
	GSRendererSoftFX(HWND hWnd, HRESULT& hr);
};
*/