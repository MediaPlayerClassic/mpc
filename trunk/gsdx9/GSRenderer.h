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

template <class VERTEX>
class GSRenderer : public GSState
{
protected:
	GSVertexList<VERTEX> m_vl;

	VERTEX* m_pVertices;
	int m_nMaxVertices, m_nVertices, m_nPrims;

	void Reset()
	{
		m_nVertices = m_nPrims = 0;
		m_vl.RemoveAll();
		__super::Reset();
	}

	void VertexKick(bool fSkip)
	{
		LOG((_T("VertexKick(%d)\n"), fSkip));

		static const int vmin[8] = {1, 2, 2, 3, 3, 3, 2, 1};
		while(m_vl.GetCount() >= vmin[m_de.PRIM.PRIM])
		{
			if(m_nVertices+6 > m_nMaxVertices)
			{
				VERTEX* pVertices = (VERTEX*)_aligned_malloc(sizeof(VERTEX) * (m_nMaxVertices <<= 1), 16);
				memcpy(pVertices, m_pVertices, m_nVertices*sizeof(VERTEX));
				_aligned_free(m_pVertices);
				m_pVertices = pVertices;
			}

			DrawingKick(fSkip);
		}
	}

	void NewPrim()
	{
		m_vl.RemoveAll();
	}

	void FlushPrim()
	{
		m_PRIM = 7;
		m_nVertices = 0;
	}

public:
	GSRenderer(int w, int h, HWND hWnd, HRESULT& hr)
		: GSState(w, h, hWnd, hr)
	{
		m_pVertices = (VERTEX*)_aligned_malloc(sizeof(VERTEX) * (m_nMaxVertices = 256), 16);
		Reset();
	}

	~GSRenderer()
	{
		_aligned_free(m_pVertices);
	}
};