/* 
 *	Copyright (C) 2007-2009 Gabest
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
#include "GSRasterizer.h"
#include "GSAlignedClass.h"

union GSScanlineSelector
{
	struct
	{
		DWORD fpsm:2; // 0
		DWORD zpsm:2; // 2
		DWORD ztst:2; // 4 (0: off, 1: write, 2: test (ge), 3: test (g))
		DWORD atst:3; // 6
		DWORD afail:2; // 9
		DWORD iip:1; // 11
		DWORD tfx:3; // 12
		DWORD tcc:1; // 15
		DWORD fst:1; // 16
		DWORD ltf:1; // 17
		DWORD tlu:1; // 18
		DWORD fge:1; // 19
		DWORD date:1; // 20
		DWORD abea:2; // 21
		DWORD abeb:2; // 23
		DWORD abec:2; // 25
		DWORD abed:2; // 27
		DWORD pabe:1; // 29
		DWORD rfb:1; // 30
	};

	struct
	{
		DWORD _pad1:21;
		DWORD abe:8;
		DWORD _pad2:3;
	};

	DWORD dw;

	operator DWORD() {return dw & 0x7fffffff;}
};

__declspec(align(16)) struct GSScanlineEnvironment
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const DWORD* clut;
	DWORD tw;

	GSVector4i* fbr;
	GSVector4i* zbr;
	int** fbc;
	int** zbc;
	GSVector4i* fzbr;
	GSVector4i* fzbc;

	GSVector4i fm, zm;
	struct {GSVector4i min, max, mask;} t; // [u] x 4 [v] x 4
	GSVector4i datm;
	GSVector4i colclamp;
	GSVector4i fba;
	GSVector4i aref;
	GSVector4i afix, afix2;
	GSVector4i frb, fga;

	struct {GSVector4 z, s, t, q; GSVector4i f, rb, ga, _pad;} d[4];
	struct {GSVector4 z, stq; GSVector4i f, c;} d4;
	GSVector4i rb, ga;
};

__declspec(align(16)) struct GSScanlineParam
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const DWORD* clut;
	DWORD tw;

	GSLocalMemory::Offset* fbo;
	GSLocalMemory::Offset* zbo;
	GSLocalMemory::Offset4* fzbo;

	DWORD fm, zm;
};

class GSDrawScanline : public GSAlignedClass<16>, public IDrawScanline
{
	GSScanlineEnvironment m_env;

	class GSFunctionMap : public IDrawScanline::FunctionMap
	{
	public:
		DrawScanlinePtr f[4][4][4][2];

		virtual DrawScanlinePtr GetDefaultFunction(DWORD dw)
		{
			GSScanlineSelector sel;
			sel.dw = dw;
			return f[sel.fpsm][sel.zpsm][sel.ztst][sel.iip];
		}
	};
	
	GSFunctionMap m_ds;

	static const GSVector4 s_ps0123[4];
	static const GSVector4i s_test[9];

	void Init();

	__forceinline GSVector4i Wrap(const GSVector4i& t);

	__forceinline void SampleTexture(DWORD ztst, DWORD fst, DWORD ltf, DWORD tlu, const GSVector4i& test, const GSVector4& s, const GSVector4& t, const GSVector4& q, GSVector4i* c);
	__forceinline void ColorTFX(DWORD tfx, const GSVector4i& rbf, const GSVector4i& gaf, GSVector4i& rbt, GSVector4i& gat);
	__forceinline void AlphaTFX(DWORD tfx, DWORD tcc, const GSVector4i& gaf, GSVector4i& gat);
	__forceinline void Fog(DWORD fge, const GSVector4i& f, GSVector4i& rb, GSVector4i& ga);
	__forceinline bool TestZ(DWORD zpsm, DWORD ztst, const GSVector4i& zs, const GSVector4i& zd, GSVector4i& test);
	__forceinline bool TestAlpha(DWORD atst, DWORD afail, const GSVector4i& ga, GSVector4i& fm, GSVector4i& zm, GSVector4i& test);
	__forceinline bool TestDestAlpha(DWORD fpsm, DWORD date, const GSVector4i& d, GSVector4i& test);

	__forceinline static void WritePixel32(DWORD* RESTRICT vm, DWORD addr, DWORD c);
	__forceinline static void WritePixel24(DWORD* RESTRICT vm, DWORD addr, DWORD c);
	__forceinline static void WritePixel16(WORD* RESTRICT vm, DWORD addr, DWORD c);
	
	__forceinline GSVector4i ReadFrameX(int psm, const GSVector4i& addr) const;
	__forceinline GSVector4i ReadZBufX(int psm, const GSVector4i& addr) const;
	__forceinline void WriteFrameX(int fpsm, int rfb, GSVector4i* c, const GSVector4i& fd, const GSVector4i& fm, const GSVector4i& fza, int fzm);
	__forceinline void WriteZBufX(int zpsm, int ztst, const GSVector4i& z, const GSVector4i& zd, const GSVector4i& zm, const GSVector4i& fza, int fzm);

	void DrawSolidRect(const GSVector4i& r, const GSVertexSW& v);

	template<class T, bool masked> 
	void DrawSolidRectT(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m);

	template<class T, bool masked> 
	__forceinline void FillRect(const GSVector4i* row, int* col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m);

	template<class T, bool masked> 
	__forceinline void FillBlock(const GSVector4i* row, int* col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m);

	template<DWORD fpsm, DWORD zpsm, DWORD ztst, DWORD iip>
	void DrawScanline(int top, int left, int right, const GSVertexSW& v);

	template<DWORD sel>
	void DrawScanlineEx(int top, int left, int right, const GSVertexSW& v);

protected:
	GSState* m_state;
	int m_id;

public:
	GSDrawScanline(GSState* state, int id);
	virtual ~GSDrawScanline();

	// IDrawScanline

	void BeginDraw(const GSRasterizerData* data, Functions* f);
	void EndDraw(const GSRasterizerStats& stats);
	void SetupPrim(GS_PRIM_CLASS primclass, const GSVertexSW* vertices, const GSVertexSW& dscan);
	void PrintStats() {m_ds.PrintStats();}
};