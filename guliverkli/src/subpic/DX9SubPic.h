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

class CDX9SubPicAllocator : public ISubPicAllocatorImpl
{
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CSize m_maxsize;

	bool Alloc(bool fStatic, ISubPic** ppSubPic);

public:
	CDX9SubPicAllocator(IDirect3DDevice9* pD3DDev, SIZE maxsize);
};
