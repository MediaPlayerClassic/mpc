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

#include "ISubPic.h"

// CDX9SubPic

class CDX9SubPic : public ISubPicImpl
{
	CComPtr<IDirect3DSurface9> m_pSurface;

protected:
	STDMETHODIMP_(void*) GetObject(); // returns IDirect3DTexture9*

public:
	CDX9SubPic(IDirect3DSurface9* pSurface);

	// ISubPic
	STDMETHODIMP GetDesc(SubPicDesc& spd);
	STDMETHODIMP CopyTo(ISubPic* pSubPic);
	STDMETHODIMP ClearDirtyRect(DWORD color);
	STDMETHODIMP Lock(SubPicDesc& spd);
	STDMETHODIMP Unlock(RECT* pDirtyRect);
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget);
};

// CDX9SubPicAllocator

class CDX9SubPicAllocator : public ISubPicAllocatorImpl, public CCritSec
{
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CSize m_maxsize;

	bool Alloc(bool fStatic, ISubPic** ppSubPic);

public:
	CDX9SubPicAllocator(IDirect3DDevice9* pD3DDev, SIZE maxsize, bool fPow2Textures);

	// ISubPicAllocator
	STDMETHODIMP ChangeDevice(IUnknown* pDev);
};
