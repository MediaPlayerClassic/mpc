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

#include "GSVertexSW.h"

interface IDrawScanline
{
public:
	typedef GSVertexSW Vertex;

	virtual void SetupDraw(Vertex* vertices, int count, const void* texture) = 0;
	virtual void DrawScanline(int top, int left, int right, const Vertex& v, const Vertex& dv) = 0;
	virtual void FillRect(const GSVector4i& r, const Vertex& v) = 0;
};

class GSRasterizer
{
protected:
	typedef GSVertexSW Vertex;

	IDrawScanline* m_ds;
	int m_id;
	int m_threads;
	int m_pixels;

	void DrawTriangleTop(Vertex* v, const GSVector4i& scissor);
	void DrawTriangleBottom(Vertex* v, const GSVector4i& scissor);
	void DrawTriangleTopBottom(Vertex* v, const GSVector4i& scissor);

	__forceinline void DrawTriangleSection(int top, int bottom, Vertex& l, const Vertex& dl, GSVector4& r, const GSVector4& dr, const Vertex& dscan, const GSVector4i& scissor);

public:
	GSRasterizer(IDrawScanline* ds, int id = 0, int threads = 0);

	void DrawPoint(Vertex* v, const GSVector4i& scissor);
	void DrawLine(Vertex* v, const GSVector4i& scissor);
	void DrawTriangle(Vertex* v, const GSVector4i& scissor);
	void DrawSprite(Vertex* v, const GSVector4i& scissor, bool solid);

	int GetPixels() {int pixels = m_pixels; m_pixels = 0; return pixels;}
};

interface IDrawAsync
{
public:
	virtual int DrawAsync(GSRasterizer* r) = 0;
};

class GSRasterizerMT : public GSRasterizer
{
	IDrawAsync* m_da;
	long* m_sync;
	bool m_exit;
    DWORD m_ThreadId;
    HANDLE m_hThread;

	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);

	DWORD ThreadProc();

public:
	GSRasterizerMT(IDrawScanline* ds, int id, int threads, IDrawAsync* da, long* sync);
	virtual ~GSRasterizerMT();

	void Run();
};
