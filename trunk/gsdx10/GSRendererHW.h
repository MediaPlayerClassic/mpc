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
#include "GSTextureFX.h"
#include "GSVertexHW.h"

class GSRendererHW : public GSRenderer<GSVertexHW>
{
protected:
	int m_width;
	int m_height;
	int m_skip;

	GSTextureCache m_tc;
	GSTextureFX m_tfx;

	void FlushPrimInternal();

	void VertexKick(bool skip);
	void DrawingKick(bool skip);
	void FlushPrim();
	void Flip();
	void InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r);
	void MinMaxUV(int w, int h, CRect& r);

	struct
	{
		CComPtr<ID3D10DepthStencilState> dss;
		CComPtr<ID3D10BlendState> bs;
	} m_date;

	void SetupDestinationAlphaTest(GSTextureCache::GSRenderTarget* rt, GSTextureCache::GSDepthStencil* ds);

	bool DetectBadFrame();

	bool OverrideInput(int& prim, int& count, GSTextureCache::GSTexture* tex);	

public:
	GSRendererHW();
	virtual ~GSRendererHW();

	bool Create(LPCTSTR title);
};