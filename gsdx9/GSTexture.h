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

#include "GSScale.h"
#include "GSDirtyRect.h"

class GSState;

class GSTextureBase
{
public:
	CComPtr<IDirect3DTexture9> m_texture, m_palette;
	GSScale m_scale;
	bool m_rt;
	D3DSURFACE_DESC m_desc;

	GSTextureBase();
	virtual ~GSTextureBase() {}
};

class GSTexture : public GSTextureBase
{
	DWORD Hash();
	bool GetDirtyRect(GSState* s, CRect& r);

public:
	GIFRegCLAMP m_CLAMP;
	GIFRegTEX0 m_TEX0;
	GIFRegTEXA m_TEXA; // *
	DWORD m_clut[256]; // *
	GSDirtyRectList m_dirty;
	CRect m_valid;
	CRect m_rcHash;
	DWORD m_dwHash;
	DWORD m_nHashDiff;
	DWORD m_bytes;
	int m_age;
	int m_vsync;
	int m_bpp;

	GSTexture();

	HRESULT Update(GSState* s, GSLocalMemory::readTexture rt);
};
