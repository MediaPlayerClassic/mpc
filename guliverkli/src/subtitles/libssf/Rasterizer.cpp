/* 
 *	Copyright (C) 2003-2006 Gabest
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
 *  Based on the rasterizer of virtualdub's subtitler plugin
 */

#include "stdafx.h"
#include <vector>
#include <algorithm>
#include "Rasterizer.h"
#include "Glyph.h"

namespace ssf
{
	template<class T> T mymax(T a, T b) {return a > b ? a : b;}
	template<class T> T mymin(T a, T b) {return a < b ? a : b;}

	Rasterizer::Rasterizer()
	{
		mpOverlayBuffer = NULL;
		mOverlayWidth = mOverlayHeight = 0;
		mPathOffsetX = mPathOffsetY = 0;
		mOffsetX = mOffsetY = 0;
	}

	Rasterizer::~Rasterizer()
	{
		_TrashOverlay();
	}

	void Rasterizer::_TrashOverlay()
	{
		if(mpOverlayBuffer) delete [] mpOverlayBuffer;
		mpOverlayBuffer = NULL;
	}

	void Rasterizer::_ReallocEdgeBuffer(int edges)
	{
		mEdgeHeapSize = edges;
		mpEdgeBuffer = (Edge*)realloc(mpEdgeBuffer, sizeof(Edge)*edges);
	}

	void Rasterizer::_EvaluateBezier(const CPoint& p0, const CPoint& p1, const CPoint& p2, const CPoint& p3)
	{
		if(abs(p0.x + p2.x - p1.x*2) +
		   abs(p0.y + p2.y - p1.y*2) +
		   abs(p1.x + p3.x - p2.x*2) +
		   abs(p1.y + p3.y - p2.y*2) <= 8)
		{
			_EvaluateLine(p0, p3);
		}
		else
		{
			CPoint p01, p12, p23, p012, p123, p0123;

			p01.x = (p0.x + p1.x + 1) >> 1;
			p01.y = (p0.y + p1.y + 1) >> 1;
			p12.x = (p1.x + p2.x + 1) >> 1;
			p12.y = (p1.y + p2.y + 1) >> 1;
			p23.x = (p2.x + p3.x + 1) >> 1;
			p23.y = (p2.y + p3.y + 1) >> 1;
			p012.x = (p01.x + p12.x + 1) >> 1;
			p012.y = (p01.y + p12.y + 1) >> 1;
			p123.x = (p12.x + p23.x + 1) >> 1;
			p123.y = (p12.y + p23.y + 1) >> 1;
			p0123.x = (p012.x + p123.x + 1) >> 1;
			p0123.y = (p012.y + p123.y + 1) >> 1;

			_EvaluateBezier(p0, p01, p012, p0123);
			_EvaluateBezier(p0123, p123, p23, p3);
		}
	}

	void Rasterizer::_EvaluateLine(CPoint p0, CPoint p1)
	{
		if(lastp != p0)
		{
			_EvaluateLine(lastp, p0);
		}

		if(!fFirstSet)
		{
			firstp = p0; 
			fFirstSet = true;
		}

		lastp = p1;

		if(p1.y > p0.y)	// down
		{
			int xacc = p0.x << 5;

			// prestep p0.y down

			int dy = p1.y - p0.y;
			int y = ((p0.y + 3)&~7) + 4;
			int iy = y >> 3;

			p1.y = (p1.y - 5) >> 3;

			if(iy <= p1.y)
			{
				int invslope = ((p1.x - p0.x) << 8) / dy;

				while(mEdgeNext + p1.y + 1 - iy > mEdgeHeapSize)
					_ReallocEdgeBuffer(mEdgeHeapSize*2);

				xacc += (invslope * (y - p0.y)) >> 3;

				while(iy <= p1.y)
				{
					int ix = (xacc + 128) >> 8;

					mpEdgeBuffer[mEdgeNext].next = mpScanBuffer[iy];
					mpEdgeBuffer[mEdgeNext].posandflag = ix*2 + 1;

					mpScanBuffer[iy] = mEdgeNext++;

					++iy;
					xacc += invslope;
				}
			}
		}
		else if(p1.y < p0.y) // up
		{
			int xacc = p1.x << 5;

			// prestep p1.y down

			int dy = p0.y - p1.y;
			int y = ((p1.y + 3)&~7) + 4;
			int iy = y >> 3;

			p0.y = (p0.y - 5) >> 3;

			if(iy <= p0.y)
			{
				int invslope = ((p0.x - p1.x) << 8) / dy;

				while(mEdgeNext + p0.y + 1 - iy > mEdgeHeapSize)
					_ReallocEdgeBuffer(mEdgeHeapSize*2);

				xacc += (invslope * (y - p1.y)) >> 3;

				while(iy <= p0.y)
				{
					int ix = (xacc + 128) >> 8;

					mpEdgeBuffer[mEdgeNext].next = mpScanBuffer[iy];
					mpEdgeBuffer[mEdgeNext].posandflag = ix*2;

					mpScanBuffer[iy] = mEdgeNext++;

					++iy;
					xacc += invslope;
				}
			}
		}
	}

	bool Rasterizer::ScanConvert(GlyphPath& path, const CRect& bbox)
	{
		// Drop any outlines we may have.

		mOutline.RemoveAll();
		mWideOutline.RemoveAll();

		if(path.types.IsEmpty() || path.points.IsEmpty() || bbox.IsRectEmpty())
		{
			mPathOffsetX = mPathOffsetY = 0;
			mWidth = mHeight = 0;
			return 0;
		}

		int minx = (bbox.left >> 3) & ~7;
		int miny = (bbox.top >> 3) & ~7;
		int maxx = (bbox.right + 7) >> 3;
		int maxy = (bbox.bottom + 7) >> 3;

		path.MovePoints(CPoint(-minx*8, -miny*8));

		if(minx > maxx || miny > maxy)
		{
			mWidth = mHeight = 0;
			mPathOffsetX = mPathOffsetY = 0;
			return true;
		}

		mWidth = maxx + 1 - minx;
		mHeight = maxy + 1 - miny;

		mPathOffsetX = minx;
		mPathOffsetY = miny;

		// Initialize edge buffer.  We use edge 0 as a sentinel.

		mEdgeNext = 1;
		mEdgeHeapSize = 0x10000;
		mpEdgeBuffer = (Edge*)malloc(sizeof(Edge)*mEdgeHeapSize);

		// Initialize scanline list.

		mpScanBuffer = new unsigned int[mHeight];
		memset(mpScanBuffer, 0, mHeight*sizeof(unsigned int));

		// Scan convert the outline.  Yuck, Bezier curves....

		// Unfortunately, Windows 95/98 GDI has a bad habit of giving us text
		// paths with all but the first figure left open, so we can't rely
		// on the PT_CLOSEFIGURE flag being used appropriately.

		fFirstSet = false;
		firstp.x = firstp.y = 0;
		lastp.x = lastp.y = 0;

		int lastmoveto = -1;

		BYTE* type = path.types.GetData();
		POINT* pt = path.points.GetData();

		for(size_t i = 0, j = path.types.GetCount(); i < j; i++)
		{
			switch(type[i] & ~PT_CLOSEFIGURE)
			{
			case PT_MOVETO:
				if(lastmoveto >= 0 && firstp != lastp) _EvaluateLine(lastp, firstp);
				lastmoveto = i;
				fFirstSet = false;
				lastp = pt[i];
				break;
			case PT_LINETO:
				if(j - (i-1) >= 2) _EvaluateLine(pt[i-1], pt[i]);
				break;
			case PT_BEZIERTO:
				if(j - (i-1) >= 4) _EvaluateBezier(pt[i-1], pt[i], pt[i+1], pt[i+2]);
				i += 2;
				break;
			}
		}

		if(lastmoveto >= 0 && firstp != lastp) _EvaluateLine(lastp, firstp);

		// Convert the edges to spans.  We couldn't do this before because some of
		// the regions may have winding numbers >+1 and it would have been a pain
		// to try to adjust the spans on the fly.  We use one heap to detangle
		// a scanline's worth of edges from the singly-linked lists, and another
		// to collect the actual scans.

		std::vector<int> heap;

		mOutline.SetCount(0, mEdgeNext / 2);

		for(int y = 0; y < mHeight; y++)
		{
			int count = 0;

			// Detangle scanline into edge heap.

			for(unsigned int ptr = mpScanBuffer[y]; ptr; ptr = mpEdgeBuffer[ptr].next)
			{
				heap.push_back(mpEdgeBuffer[ptr].posandflag);
			}

			// Sort edge heap.  Note that we conveniently made the opening edges
			// one more than closing edges at the same spot, so we won't have any
			// problems with abutting spans.

			std::sort(heap.begin(), heap.end());

			// Process edges and add spans.  Since we only check for a non-zero
			// winding number, it doesn't matter which way the outlines go!

			std::vector<int>::iterator itX1 = heap.begin();
			std::vector<int>::iterator itX2 = heap.end();

			int x1, x2;

			for(; itX1 != itX2; ++itX1)
			{
				int x = *itX1;

				if(!count) 
				{
					x1 = x >> 1;
				}

				if(x&1) ++count;
				else --count;

				if(!count)
				{
					x2 = x >> 1;

					if(x2 > x1)
					{
						Span s(x1, y, x2, y);
						s.first += 0x4000000040000000i64;
						s.second += 0x4000000040000000i64;
						mOutline.Add(s);
					}
				}
			}

			heap.clear();
		}

		// Dump the edge and scan buffers, since we no longer need them.

		free(mpEdgeBuffer);
		delete [] mpScanBuffer;

		// All done!

		return true;
	}

	void Rasterizer::_OverlapRegion(Array<Span>& dst, Array<Span>& src, int dx, int dy)
	{
		mWideOutlineTmp.Move(dst);

		Span* a = mWideOutlineTmp.GetData();
		Span* ae = a + mWideOutlineTmp.GetCount();
		Span* b = src.GetData();
		Span* be = b + src.GetCount();

		Span o(0, dy, 0, dy);
		o.first -= dx;
		o.second += dx;

		while(a != ae && b != be)
		{
			Span x;

			if(b->first + o.first < a->first)
			{
				// B span is earlier.  Use it.

				x.first = b->first + o.first;
				x.second = b->second + o.second;

				b++;

				// B spans don't overlap, so begin merge loop with A first.

				for(;;)
				{
					// If we run out of A spans or the A span doesn't overlap,
					// then the next B span can't either (because B spans don't
					// overlap) and we exit.

					if(a == ae || a->first > x.second)
						break;

					do {x.second = mymax(x.second, a->second);}
					while(++a != ae && a->first <= x.second);

					// If we run out of B spans or the B span doesn't overlap,
					// then the next A span can't either (because A spans don't
					// overlap) and we exit.

					if(b == be || b->first + o.first > x.second)
						break;

					do {x.second = mymax(x.second, b->second + o.second);}
					while(++b != be && b->first + o.first <= x.second);
				}
			}
			else
			{
				// A span is earlier.  Use it.

				x = *a;

				a++;

				// A spans don't overlap, so begin merge loop with B first.

				for(;;)
				{
					// If we run out of B spans or the B span doesn't overlap,
					// then the next A span can't either (because A spans don't
					// overlap) and we exit.

					if(b == be || b->first + o.first > x.second)
						break;

					do {x.second = mymax(x.second, b->second + o.second);}
					while(++b != be && b->first + o.first <= x.second);

					// If we run out of A spans or the A span doesn't overlap,
					// then the next B span can't either (because B spans don't
					// overlap) and we exit.

					if(a == ae || a->first > x.second)
						break;

					do {x.second = mymax(x.second, a->second);}
					while(++a != ae && a->first <= x.second);
				}
			}

			// Flush span.

			dst.Add(x);
		}

		// Copy over leftover spans.

		dst.Append(a, ae - a);

		for(; b != be; b++)
		{
			dst.Add(Span(b->first + o.first, b->second + o.second));
		}
	}

	bool Rasterizer::CreateWidenedRegion(int r)
	{
		if(r < 0) r = 0;

		for(int y = -r; y <= r; ++y)
		{
			int x = (int)(0.5f + sqrt(float(r*r - y*y)));

			_OverlapRegion(mWideOutline, mOutline, x, y);
		}

		mWideBorder = r;

		return true;
	}

	bool Rasterizer::Rasterize(int xsub, int ysub)
	{
		_TrashOverlay();

		if(!mWidth || !mHeight)
		{
			mOverlayWidth = mOverlayHeight = 0;
			return true;
		}

		xsub &= 7;
		ysub &= 7;

		int width = mWidth + xsub;
		int height = mHeight + ysub;

		mOffsetX = mPathOffsetX - xsub;
		mOffsetY = mPathOffsetY - ysub;

		int border = ((mWideBorder + 7) & ~7) + 32;

		if(!mWideOutline.IsEmpty())
		{
			width += 2*border;
			height += 2*border;

			xsub += border;
			ysub += border;

			mOffsetX -= border;
			mOffsetY -= border;
		}

		mOverlayWidth = ((width + 7) >> 3) + 1;
		mOverlayHeight = ((height + 7) >> 3) + 1;

		mpOverlayBuffer = new BYTE[2 * mOverlayWidth * mOverlayHeight];
		memset(mpOverlayBuffer, 0, 2 * mOverlayWidth * mOverlayHeight);

		Array<Span>* pOutline[2] = {&mOutline, &mWideOutline};

		for(int i = 0; i < countof(pOutline); i++)
		{
			const Span* s = pOutline[i]->GetData();

			for(size_t j = 0, k = pOutline[i]->GetCount(); j < k; j++)
			{
				int y = s[j].y1 - 0x40000000 + ysub;
				int x1 = s[j].x1 - 0x40000000 + xsub;
				int x2 = s[j].x2 - 0x40000000 + xsub;

				if(x2 > x1)
				{
					int first = x1 >> 3;
					int last = (x2-1) >> 3;

					BYTE* dst = mpOverlayBuffer + 2*(mOverlayWidth * (y >> 3) + first) + i;

					if(first == last)
					{
						*dst += x2 - x1;
					}
					else
					{
						*dst += ((first+1) << 3) - x1;
						dst += 2;

						while(++first < last)
						{
							*dst += 8;
							dst += 2;
						}

						*dst += x2 - (last << 3);
					}
				}
			}
		}

		return true;
	}

	void Rasterizer::Blur(float n, int plane)
	{
		if(n <= 0 || !mOverlayWidth || !mOverlayHeight || !mpOverlayBuffer)
			return;

		int w = mOverlayWidth;
		int h = mOverlayHeight;
		int pitch = w*2;
		BYTE* q0 = new BYTE[w*h];

		for(int pass = 0, limit = (int)(n + 0.5); pass < n; pass++)
		{
			BYTE* p = mpOverlayBuffer + plane;
			BYTE* q = q0;

			for(int y = 0; y < h; y++, p += pitch, q += w)
			{
				q[0] = (2*p[0] + p[2]) >> 2;
				int x = 0;
				for(x = 1; x < w-1; x++)
					q[x] = (p[(x-1)*2] + 2*p[x*2] + p[(x+1)*2]) >> 2;
				q[x] = (p[(x-1)*2] + 2*p[x*2]) >> 2;
			}

			p = mpOverlayBuffer + plane;
			q = q0;

			for(int x = 0; x < w; x++, p += 2, q++)
			{
				p[0] = (2*q[0] + q[w]) >> 2;
				int y = 0, yp, yq;
				for(y = 1, yp = y*pitch, yq = y*w; y < h-1; y++, yp += pitch, yq += w)
					p[yp] = (q[yq-w] + 2*q[yq] + q[yq+w]) >> 2;
				p[yp] = (q[yq-w] + 2*q[yq]) >> 2;
			}
		}

		delete [] q0;
	}

	void Rasterizer::Reuse(Rasterizer& r)
	{
		mWidth = r.mWidth;
		mHeight = r.mHeight;
		mPathOffsetX = r.mPathOffsetX;
		mPathOffsetY = r.mPathOffsetY;
		mWideBorder = r.mWideBorder;
		mOutline.Move(r.mOutline);
		mWideOutline.Move(r.mWideOutline);
	}

	///////////////////////////////////////////////////////////////////////////

	#define pixmix(s) {																 \
		int a = (((s)*(color>>24))>>6)&0xff;										 \
		int ia = 256-a;																 \
																					 \
		dst[wt] = ((((dst[wt]&0x00ff00ff)*ia + (color&0x00ff00ff)*a)&0xff00ff00)>>8) \
				| ((((dst[wt]&0x0000ff00)*ia + (color&0x0000ff00)*a)&0x00ff0000)>>8) \
				| ((((dst[wt]>>8)&0x00ff0000)*ia)&0xff000000);						 \
		} \

	static __forceinline void pixmix_sse2(DWORD* dst, DWORD color, DWORD alpha)
	{
		alpha = ((alpha * (color>>24)) >> 6) & 0xff;
		color &= 0xffffff;

		__m128i zero = _mm_setzero_si128();
		__m128i a = _mm_set1_epi32((alpha << 16) | (0x100 - alpha));
		__m128i d = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*dst), zero);
		__m128i s = _mm_unpacklo_epi8(_mm_cvtsi32_si128(color), zero);
		__m128i r = _mm_unpacklo_epi16(d, s);

		r = _mm_madd_epi16(r, a);
		r = _mm_srli_epi32(r, 8);
		r = _mm_packs_epi32(r, r);
		r = _mm_packus_epi16(r, r);

		*dst = (DWORD)_mm_cvtsi128_si32(r);
	}

	CRect Rasterizer::Draw(const SubPicDesc& spd, const CRect& clip, int xsub, int ysub, const DWORD* switchpts, bool fBody, bool fBorder)
	{
		CRect bbox(0, 0, 0, 0);

		if(!switchpts || !fBody && !fBorder) return bbox;

		// clip

		CRect r(0, 0, spd.w, spd.h);
		r &= clip;

		int x = (xsub + mOffsetX + 4) >> 3;
		int y = (ysub + mOffsetY + 4) >> 3;
		int w = mOverlayWidth;
		int h = mOverlayHeight;
		int xo = 0, yo = 0;

		if(x < r.left) {xo = r.left - x; w -= r.left - x; x = r.left;}
		if(y < r.top) {yo = r.top - y; h -= r.top - y; y = r.top;}
		if(x+w > r.right) w = r.right - x;
		if(y+h > r.bottom) h = r.bottom - y;

		if(w <= 0 || h <= 0) return bbox;

		bbox.SetRect(x, y, x + w, y + h);
		bbox &= CRect(0, 0, spd.w, spd.h);

		// draw

		const BYTE* src = mpOverlayBuffer + 2*(mOverlayWidth * yo + xo);
		const BYTE* s = fBorder ? (src+1) : src;
		DWORD* dst = (DWORD*)((BYTE*)spd.bits + spd.pitch * y) + x;

		DWORD color = switchpts[0];

		bool fSSE2 = !!(g_cpuid.m_flags & CCpuID::sse2);

		while(h--)
		{
			if(switchpts[1] == 0xffffffff)
			{
				if(1) // fBody)
				{
					if(fSSE2) for(int wt=0; wt<w; ++wt) pixmix_sse2(&dst[wt], color, s[wt*2]);
					else for(int wt=0; wt<w; ++wt) pixmix(s[wt*2]);
				}
				else
				{
					if(fSSE2) for(int wt=0; wt<w; ++wt) pixmix_sse2(&dst[wt], color, src[wt*2+1] - src[wt*2]);
					else for(int wt=0; wt<w; ++wt) pixmix(src[wt*2+1] - src[wt*2]);
				}
			}
			else
			{
				const DWORD* sw = switchpts;

				if(1) // fBody)
				{
					if(fSSE2) 
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];}
						pixmix_sse2(&dst[wt], color, s[wt*2]);
					}
					else
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];}
						pixmix(s[wt*2]);
					}
				}
				else
				{
					if(fSSE2) 
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];} 
						pixmix_sse2(&dst[wt], color, src[wt*2+1] - src[wt*2]);
					}
					else
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];} 
						pixmix(src[wt*2+1] - src[wt*2]);
					}
				}
			}

			src += 2*mOverlayWidth;
			s += 2*mOverlayWidth;
			dst = (DWORD*)((BYTE*)dst + spd.pitch);
		}

		return bbox;
	}
}