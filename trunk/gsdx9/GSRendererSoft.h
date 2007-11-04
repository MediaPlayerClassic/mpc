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
	int DrawingKick(bool skip);
	void FlushPrim();
	void Flip();
	void EndFrame();

	enum {PRIM_NONE, PRIM_SPRITE, PRIM_TRIANGLE, PRIM_LINE, PRIM_POINT} m_primtype;

	DWORD m_faddr_x0, m_faddr;
	DWORD m_zaddr_x0, m_zaddr;
	int* m_faddr_ro;
	int* m_zaddr_ro;
	int m_fx, m_fy;
	void RowInit(int x, int y);
	void RowStep();

	void DrawPoint(Vertex* v);
	void DrawLine(Vertex* v);
	void DrawTriangle(Vertex* v);
	void DrawSprite(Vertex* v);
	bool DrawFilledRect(int left, int top, int right, int bottom, const Vertex& v);

	template <int iZTST, int iATST> 
	void DrawVertex(const Vertex& v);

	typedef void (GSRendererSoft<Vertex>::*DrawVertexPtr)(const Vertex& v);
	DrawVertexPtr m_dv[4][8], m_pDrawVertex;

	template <int iLOD, bool bLCM, bool bTCC, int iTFX>
	void DrawVertexTFX(typename Vertex::Vector& Cf, const Vertex& v);

	typedef void (GSRendererSoft<Vertex>::*DrawVertexTFXPtr)(typename Vertex::Vector& Cf, const Vertex& v);
	DrawVertexTFXPtr m_dvtfx[4][2][2][4], m_pDrawVertexTFX;

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
	virtual ~GSRendererSoft();

	HRESULT ResetDevice(bool fForceWindowed = false);
};

class GSRendererSoftFP : public GSRendererSoft<GSSoftVertexFP>
{
protected:
	void VertexKick(bool skip);

public:
	GSRendererSoftFP(HWND hWnd, HRESULT& hr);
};
/*
class GSRendererSoftFX : public GSRendererSoft<GSSoftVertexFX>
{
protected:
	void VertexKick(bool skip);

public:
	GSRendererSoftFX(HWND hWnd, HRESULT& hr);
};
*/