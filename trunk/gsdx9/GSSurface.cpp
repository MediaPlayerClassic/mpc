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

#include "stdafx.h"
#include "GSTextureCache.h"

GSTextureCache::GSSurface::GSSurface(GSTextureCache* tc)
	: m_tc(tc)
{
	memset(&m_desc, 0, sizeof(m_desc));
	m_scale = GSScale(1, 1);
	m_age = 0;
	m_size = 0;
	m_TEX0.TBP0 = ~0;
}

GSTextureCache::GSSurface::~GSSurface()
{
	m_tc->Recycle(m_surface);
}

bool GSTextureCache::GSSurface::IsRenderTarget() const 
{
	return !!(m_desc.Usage & D3DUSAGE_RENDERTARGET);
}

bool GSTextureCache::GSSurface::IsDepthStencil() const 
{
	return !!(m_desc.Usage & D3DUSAGE_DEPTHSTENCIL);
}

void GSTextureCache::GSSurface::Update()
{
	m_age = 0;
}