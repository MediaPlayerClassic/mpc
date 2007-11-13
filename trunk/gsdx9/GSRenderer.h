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

#include "GSState.h"

template <class VERTEX> class GSRenderer : public GSState
{
protected:
	VERTEX* m_pVertices;
	int m_nMaxVertices, m_nVertices;
	GSVertexList<VERTEX> m_vl;

	void ResetState()
	{
		m_nVertices = 0;
		m_vl.RemoveAll();

		__super::ResetState();
	}

	void VertexKick(bool skip)
	{
		while(m_vl.GetCount() >= minPrimVertexTable[m_pPRIM->PRIM])
		{
			if(m_nVertices + 6 > m_nMaxVertices)
			{
				m_nMaxVertices <<= 1;

				VERTEX* pVertices = (VERTEX*)_aligned_malloc(sizeof(VERTEX) * m_nMaxVertices, 16);
				memcpy(pVertices, m_pVertices, sizeof(VERTEX) * m_nVertices);
				_aligned_free(m_pVertices);
				m_pVertices = pVertices;
			}

			m_nVertices += DrawingKick(skip);
		}
	}

	void NewPrim()
	{
		m_vl.RemoveAll();
	}

	void FlushPrim() 
	{
		m_nVertices = 0;
	}

public:
	GSRenderer()
		: m_nMaxVertices(256)
	{
		m_pVertices = (VERTEX*)_aligned_malloc(sizeof(VERTEX) * m_nMaxVertices, 16);
	}

	virtual ~GSRenderer()
	{
		_aligned_free(m_pVertices);
	}
};