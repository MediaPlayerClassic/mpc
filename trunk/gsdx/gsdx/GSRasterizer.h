/* 
 *	Copyright (C) 2007 Gabest
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
#include "GSVertexSW.h"
#include "GSAlignedClass.h"

// - texture cache size should be the size of the page (depends on psm, ttbl->pgs), 
// but the min fetchable 4 bpp block size (32x16) is the fastest and 99% ok
// - there should be only one page in the cache, addressing another is equal to a TEXFLUSH

#define TEXTURE_CACHE_WIDTH 5
#define TEXTURE_CACHE_HEIGHT 4

// FIXME: the fog effect in re4 needs even bigger cache (128x128) to leave no black lines at the edges (might be the bilinear filter)

class GSRasterizer : public GSAlignedClass<16>
{
protected:
	typedef GSVertexSW Vertex;

	GSState* m_state;
	int m_id;
	int m_threads;

	DWORD* m_texture;
	DWORD m_tw;

private:
	struct ColumnOffset
	{
		GSVector4i row[1024];
		int* col[4];
		DWORD hash;
	};

	__declspec(align(16)) struct ScanlineEnvironment
	{
		int steps;

		GSLocalMemory::readTexture rtx;

		GSVector4i* fbr;
		GSVector4i* zbr;
		int** fbc;
		int** zbc;

		GSVector4i fm, zm;
		struct {GSVector4i min, max, mask;} t; // [u] x 4 [v] x 4
		GSVector4i datm;
		GSVector4i colclamp;
		GSVector4i fba;
		GSVector4i aref;
		GSVector4 afix;
		struct {GSVector4 r, g, b;} f;

		GSVector4 dz0123, dz;
		GSVector4 df0123, df;
		GSVector4 dt0123, dt;
		GSVector4 ds0123, ds;
		GSVector4 dq0123, dq;
		GSVector4 dr0123, dr;
		GSVector4 dg0123, dg;
		GSVector4 db0123, db;
		GSVector4 da0123, da;
	};

	union ScanlineSelector
	{
		struct
		{
			DWORD fpsm:2; // 0
			DWORD zpsm:2; // 2
			DWORD ztst:2; // 4 (0: off, 1: write, 2: test (ge), 3: test (g))
			DWORD iip:1; // 6
			DWORD tfx:3; // 7
			DWORD tcc:1; // 10
			DWORD fst:1; // 11
			DWORD ltf:1; // 12
			DWORD atst:3; // 13
			DWORD afail:2; // 16
			DWORD fge:1; // 18
			DWORD date:1; // 19
			DWORD abea:2; // 20
			DWORD abeb:2; // 22
			DWORD abec:2; // 24
			DWORD abed:2; // 26
			DWORD pabe:1; // 28
			DWORD rfb:1; // 29
			DWORD wzb:1; // 30
		};

		struct
		{
			DWORD _pad1:20;
			DWORD abe:8;
			DWORD _pad2:4;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x7fffffff;}
	};

	CRect m_scissor;
	CRBMapC<DWORD, ColumnOffset*> m_comap;
	ColumnOffset* m_fbco;
	ColumnOffset* m_zbco;
	ScanlineSelector m_sel;
	ScanlineEnvironment m_slenv;
	bool m_solidrect;

	void SetupColumnOffset();

	template<bool pos, bool tex, bool col> 
	__forceinline void SetupScanline(const Vertex& dv);

	typedef void (GSRasterizer::*DrawScanlinePtr)(int top, int left, int right, const Vertex& v);

	DrawScanlinePtr m_ds[4][4][4][2], m_dsf;
	CRBMap<DWORD, DrawScanlinePtr> m_dsmap, m_dsmap2;

	template<DWORD fpsm, DWORD zpsm, DWORD ztst, DWORD iip>
	void DrawScanline(int top, int left, int right, const Vertex& v);

	void InitEx();

	template<DWORD sel> 
	void DrawScanlineEx(int top, int left, int right, const Vertex& v);

	__forceinline void ColorTFX(DWORD tfx, const GSVector4& rf, const GSVector4& gf, const GSVector4& bf, const GSVector4& af, GSVector4& rt, GSVector4& gt, GSVector4& bt);
	__forceinline void AlphaTFX(DWORD tfx, DWORD tcc, const GSVector4& af, GSVector4& at);
	__forceinline void Fog(const GSVector4& f, GSVector4& r, GSVector4& g, GSVector4& b);
	__forceinline bool TestZ(DWORD zpsm, DWORD ztst, const GSVector4i& zs, const GSVector4i& za, GSVector4i& test);
	__forceinline bool TestAlpha(DWORD atst, DWORD afail, const GSVector4& a, GSVector4i& fm, GSVector4i& zm, GSVector4i& test);

	__forceinline DWORD ReadTexel(int x, int y)
	{
		return m_texture[(y << m_tw) + x];
	}

	__forceinline GSVector4i Wrap(const GSVector4i& t)
	{
		GSVector4i clamp = t.sat_i16(m_slenv.t.min, m_slenv.t.max);
		GSVector4i repeat = (t & m_slenv.t.min) | m_slenv.t.max;

 		return clamp.blend8(repeat, m_slenv.t.mask);
	}

	void DrawPoint(Vertex* v);
	void DrawLine(Vertex* v);
	void DrawTriangle(Vertex* v);
	void DrawSprite(Vertex* v);
	bool DrawSolidRect(int left, int top, int right, int bottom, const Vertex& v);

	__forceinline void DrawTriangleSection(Vertex& l, const Vertex& dl, GSVector4& r, const GSVector4& dr, const GSVector4& b, const Vertex& dscan);

public:
	GSRasterizer(GSState* state, int id = 0, int threads = 0);
	virtual ~GSRasterizer();

	int Draw(Vertex* v, int count, DWORD* texture);
};

class GSRasterizerMT : public GSRasterizer
{
	Vertex* m_vertices;
	int m_count;
	long* m_sync;
	bool m_exit;
    DWORD m_ThreadId;
    HANDLE m_hThread;

	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);

	DWORD ThreadProc();

public:
	GSRasterizerMT(GSState* state, int id, int threads, long* sync);
	virtual ~GSRasterizerMT();

	void BeginDraw(Vertex* vertices, int count, DWORD* texture);
};