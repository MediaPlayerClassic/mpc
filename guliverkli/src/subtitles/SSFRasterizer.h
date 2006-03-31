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
 */

#pragma once

#include <vector>
#include <atlcoll.h>
#include "..\SubPic\ISubPic.h"

// simple array class for simple types without constructors, and it doesn't free its reserves on SetCount(0)

template<class T>
class SSFArray
{
	T* m_pData;
	size_t m_nSize;
	size_t m_nMaxSize;
	size_t m_nGrowBy;

public:
	SSFArray() {m_pData = NULL; m_nSize = m_nMaxSize = 0; m_nGrowBy = 4096;}
	virtual ~SSFArray() {if(m_pData) _aligned_free(m_pData);}

	void SetCount(size_t nSize, size_t nGrowBy = 0)
	{
		if(nGrowBy > 0)
		{
			m_nGrowBy = nGrowBy;
		}

		if(nSize > m_nMaxSize)
		{
			m_nMaxSize = nSize + max(m_nGrowBy, m_nSize);
			size_t nBytes = m_nMaxSize * sizeof(T);
			m_pData = m_pData ? (T*)_aligned_realloc(m_pData, nBytes, 16) : (T*)_aligned_malloc(nBytes, 16);
		}

		m_nSize = nSize;
	}

	size_t GetCount() const {return m_nSize;}

	void RemoveAll() {m_nSize = 0;}
	bool IsEmpty() const {return m_nSize == 0;}

	T* GetData() {return m_pData;}

	void Add(const T& t)
	{
		size_t nPos = m_nSize;
		SetCount(m_nSize+1);
		m_pData[nPos] = t;
	}

	void Append(const SSFArray& a, size_t nGrowBy = 0)
	{
		Append(a.m_pData, a.m_nSize, nGrowBy);
	}

	void Append(const T* ptr, size_t nSize, size_t nGrowBy = 0)
	{
		if(!nSize) return;
		size_t nOldSize = m_nSize;
		SetCount(nOldSize + nSize);
		memcpy(m_pData + nOldSize, ptr, nSize * sizeof(T));
	}

	const T& operator [] (size_t i) const {return m_pData[i];}
	T& operator [] (size_t i) {return m_pData[i];}

	void Copy(const SSFArray& v)
	{
		SetCount(v.GetCount());
		memcpy(m_pData, v.m_pData, m_nSize * sizeof(T));
	}

	void Move(SSFArray& v)
	{
		Swap(v);
		v.SetCount(0);
	}

	void Swap(SSFArray& v)
	{
		T* pData = m_pData; m_pData = v.m_pData; v.m_pData = pData;
		size_t nSize = m_nSize; m_nSize = v.m_nSize; v.m_nSize = nSize;
		size_t nMaxSize = m_nMaxSize; m_nMaxSize = v.m_nMaxSize; v.m_nMaxSize = nMaxSize;
		size_t nGrowBy = m_nGrowBy; m_nGrowBy = v.m_nGrowBy; v.m_nGrowBy = nGrowBy;
	}
};

class SSFRasterizer
{
	bool fFirstSet;
	CPoint firstp, lastp;

private:
	int mWidth, mHeight;

	union tSpan
	{
		struct {unsigned int x1, y1, x2, y2;};
		struct {unsigned __int64 first, second;};
	};

	typedef SSFArray<tSpan> tSpanBuffer;

	tSpanBuffer mOutline, mWideOutline, mWideOutlineTmp;
	int mWideBorder;

	struct Edge {int next, posandflag;}* mpEdgeBuffer;
	unsigned int mEdgeHeapSize;
	unsigned int mEdgeNext;
	unsigned int* mpScanBuffer;

protected:
	BYTE* mpOverlayBuffer;
	int mOverlayWidth, mOverlayHeight;
	int mPathOffsetX, mPathOffsetY;
	int mOffsetX, mOffsetY;

private:
	void _TrashOverlay();
	void _ReallocEdgeBuffer(int edges);
	void _EvaluateBezier(const POINT* pt);
	void _EvaluateLine(const POINT* pt);
	void _EvaluateLine(int x0, int y0, int x1, int y1);
	void _OverlapRegion(tSpanBuffer& dst, tSpanBuffer& src, int dx, int dy);

public:
	SSFRasterizer();
	virtual ~SSFRasterizer();

	static void MovePoints(POINT* p, unsigned int n, int dx, int dy);

	bool ScanConvert(unsigned int npts, BYTE* types, POINT* pt);
	bool CreateWidenedRegion(int r);
	bool Rasterize(int xsub, int ysub);

	void Reuse(SSFRasterizer& r);

	CRect Draw(const SubPicDesc& spd, const CRect& clip, int xsub, int ysub, const DWORD* switchpts, bool fBody, bool fBorder);
};

