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

#include "GSState.h"

class GSStateSoft : public GSState
{
	GSVertexList<GSSoftVertex> m_vl;

	enum {D3DPT_SPRITE = 0};

	void Reset();
	void VertexKick(bool fSkip);
	void DrawingKick(bool fSkip);
	void NewPrim();
	void FlushPrim();
	void Flip();
	void EndFrame();
	void InvalidateTexture(DWORD TBP0);

	enum {PRIM_NONE, PRIM_SPRITE, PRIM_TRIANGLE, PRIM_LINE, PRIM_POINT} m_primtype;
	GSSoftVertex* m_pVertices;
	int m_nMaxVertices, m_nVertices, m_nPrims;

	void DrawPoint(GSSoftVertex* v, CRect& scissor);
	void DrawLine(GSSoftVertex* v, CRect& scissor);
	void DrawTriangle(GSSoftVertex* v, CRect& scissor);
	void DrawSprite(GSSoftVertex* v, CRect& scissor);
	void DrawVertex(int x, int y, GSSoftVertex& v);

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
	CTexture* LookupTexture();

	BYTE m_clip[512+256+512];
	BYTE m_mask[512+256+512];
	BYTE* m_clamp;

public:
	GSStateSoft(HWND hWnd, HRESULT& hr);
	~GSStateSoft();

	void LOGVERTEX(GSSoftVertex& v, LPCTSTR type)
	{
		GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];
		int tw = 1, th = 1;
		if(m_de.PRIM.TME) {tw = 1<<ctxt->TEX0.TW; th = 1<<ctxt->TEX0.TH;}
		LOG2((_T("\t %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%f, %f) (%f, %f)\n"), 
			type,
			v.x, v.y, v.z, 1.0 / v.w, 
			(((BYTE)v.a)<<24)|(((BYTE)v.r)<<16)|(((BYTE)v.g)<<8)|(((BYTE)v.b)<<0), 
			v.u, v.v, v.u*tw, v.v*th));
	}
};
