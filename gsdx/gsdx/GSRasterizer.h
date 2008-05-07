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

// texture cache size should be the size of the page (depends on psm, ttbl->pgs), but the min fetchable 4 bpp block size (32x16) is the fastest and 99% ok

#define TEXTURE_CACHE_WIDTH 5
#define TEXTURE_CACHE_HEIGHT 4

// FIXME: the fog effect in re4 needs even bigger cache (128x128) to leave no black lines at the edges (might be the bilinear filter)

class GSRasterizer
{
protected:
	typedef GSVertexSW Vertex;
	typedef GSVertexSW::Vector Vector;

private:
	struct ColumnOffset
	{
		__m128i addr[1024]; 
		DWORD hash;
	};

	struct ScanlineEnvironment
	{
		__m128i fm, zm;
		struct {__m128i min, max, mask;} t; // [u] x 4 [v] x 4
		__m128i datm;
		__m128i colclamp;
		__m128i fba;
		__m128i aref;
		GSVector4 afix;
		struct {GSVector4 r, g, b;} f;

		GSVector4 dz0123, dz;
		GSVector4 dt0123, dt;
		GSVector4 ds0123, ds;
		GSVector4 df0123, df;
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
			DWORD fpsm:3; // 0
			DWORD zpsm:2; // 3
			DWORD ztst:2; // 5
			DWORD tfx:3; // 7
			DWORD tcc:1; // 10
			DWORD fst:1; // 11
			DWORD ltf:1; // 12
			DWORD atst:3; // 13
			DWORD afail:2; // 16
			DWORD fge:1; // 18
			DWORD rfb:1; // 19
			DWORD date:1; // 20
			DWORD abe:2; // 21
			DWORD abea:2; // 23
			DWORD abeb:2; // 25
			DWORD abec:2; // 27
			DWORD abed:2; // 29
		};

		DWORD dw;

		operator DWORD() {return dw & 0x7fffffff;}
	};

	GSState* m_state;
	CRect m_scissor;
	DWORD* m_cache;
	DWORD m_page[(1024 / (1 << TEXTURE_CACHE_HEIGHT)) * (1024 / (1 << TEXTURE_CACHE_WIDTH))];
	DWORD m_pagehash;
	DWORD m_pagedirty;
	CSimpleMap<DWORD, ColumnOffset*> m_comap;
	ColumnOffset* m_fbco;
	ColumnOffset* m_zbco;
	ScanlineSelector m_sel;
	ScanlineEnvironment* m_slenv;
	bool m_solidrect;

	void SetupColumnOffset();
	void SetupScanline(const Vertex& dv);

	typedef void (GSRasterizer::*DrawScanlinePtr)(int top, int left, int right, const Vertex& v);

	DrawScanlinePtr m_ds[8][4][3]/*[3]*/, m_dsf;
	CAtlMap<DWORD, DrawScanlinePtr> m_dsmap, m_dsmap2;

	template<int iFPSM, int iZPSM, int iZTST>//, int iABE>
	void DrawScanline(int top, int left, int right, const Vertex& v);

	template<DWORD sel>
	void DrawScanline2(int top, int left, int right, const Vertex& v);

	void FetchTexture(int x, int y);

	__forceinline void FetchTexel(int x, int y)
	{
		int i = x >> TEXTURE_CACHE_WIDTH;
		int j = y & ~((1 << TEXTURE_CACHE_HEIGHT) - 1);
		int k = 10 - (TEXTURE_CACHE_WIDTH + TEXTURE_CACHE_HEIGHT);

		if(k > 0) i += j << k;
		else if(k < 0) i += j >> -k;
		else i += j;

		if(m_page[i] != m_pagehash)
		{
			m_page[i] = m_pagehash;
			m_pagedirty = true;

			FetchTexture(x, y);
		}
	}

	__forceinline DWORD ReadTexelNoFetch(int x, int y)
	{
		return m_cache[y * 1024 + x];
	}

	__forceinline DWORD ReadTexel(int x, int y)
	{
		FetchTexel(x, y);

		return ReadTexelNoFetch(x, y);
	}

	__forceinline __m128i Wrap(__m128i t)
	{
		__m128i clamp = _mm_min_epi16(_mm_max_epi16(t, m_slenv->t.min), m_slenv->t.max);
		__m128i repeat = _mm_or_si128(_mm_and_si128(t, m_slenv->t.min), m_slenv->t.max);

		return _mm_blendv_epi8(clamp, repeat, m_slenv->t.mask);
	}

	void DrawPoint(Vertex* v);
	void DrawLine(Vertex* v);
	void DrawTriangle(Vertex* v);
	void DrawSprite(Vertex* v);
	bool DrawSolidRect(int left, int top, int right, int bottom, const Vertex& v);

public:
	GSRasterizer(GSState* state);
	virtual ~GSRasterizer();

	void InvalidateTextureCache();
	int Draw(Vertex* v, int count);
};
/*
class GSRasterizerMT : public GSRasterizer
{
	Vertex* m_vertices;
	int m_count;

    DWORD m_ThreadId;
    HANDLE m_hThread;
	HANDLE m_hEventBeginDraw;
	HANDLE m_hEventEndDraw;
	HANDLE m_hEventExit;

	DWORD ThreadProc()
	{
		HANDLE events[] = {m_hEventBeginDraw, m_hEventExit};

		while(1)
		{
			switch(WaitForMultipleObjects(countof(events), events, FALSE, INFINITE))
			{
			case WAIT_OBJECT_0:
				Draw(m_vertices, m_count);
				SetEvent(m_hEventEndDraw);
				break;
			default:
				return 0;
			}
		}

		return 1;
	}

	static DWORD WINAPI StaticThreadProc(LPVOID lpParam)
	{
		return ((GSRasterizerMT*)lpParam)->ThreadProc();
	}

public:
	GSRasterizerMT(GSState* state)
		: GSRasterizer(state)
		, m_ThreadId(0)
		, m_hThread(NULL)
		, m_vertices(NULL)
		, m_count(0)
	{
		m_hThread = CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);

		m_hEventBeginDraw = CreateEvent(0, FALSE, FALSE, 0);
		m_hEventEndDraw = CreateEvent(0, FALSE, FALSE, 0);
		m_hEventExit = CreateEvent(0, FALSE, FALSE, 0);
	}

	virtual ~GSRasterizerMT()
	{
		if(m_hThread != NULL)
		{
			SetEvent(m_hEventExit);

			if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
			{
				TerminateThread(m_hThread, 1);
			}

			CloseHandle(m_hThread);

			m_ThreadId = 0;
		}

		CloseHandle(m_hEventBeginDraw);
		CloseHandle(m_hEventEndDraw);
		CloseHandle(m_hEventExit);
	}

	HANDLE BeginDraw(Vertex* vertices, int count)
	{
		m_vertices = vertices;
		m_count = count;

		SetEvent(m_hEventBeginDraw);

		SwitchToThread();

		return m_hEventEndDraw;
	}
};
*/