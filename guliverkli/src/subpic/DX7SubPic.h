#pragma once

#include "ISubPic.h"

// CDX7SubPic

class CDX7SubPic : public ISubPicImpl
{
	CComPtr<IDirect3DDevice7> m_pD3DDev;
	CComPtr<IDirectDrawSurface7> m_pSurface;

protected:
	STDMETHODIMP_(void*) GetObject(); // returns IDirectDrawSurface7*

public:
	CDX7SubPic(IDirect3DDevice7* pD3DDev, IDirectDrawSurface7* pSurface);

	// ISubPic
	STDMETHODIMP GetDesc(SubPicDesc& spd);
	STDMETHODIMP CopyTo(ISubPic* pSubPic);
	STDMETHODIMP ClearDirtyRect(DWORD color);
	STDMETHODIMP Lock(SubPicDesc& spd);
	STDMETHODIMP Unlock(RECT* pDirtyRect);
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget);
};

// CDX7SubPicAllocator

class CDX7SubPicAllocator : public ISubPicAllocatorImpl
{
    CComPtr<IDirect3DDevice7> m_pD3DDev;
	CSize m_maxsize;

	bool Alloc(bool fStatic, ISubPic** ppSubPic);

public:
	CDX7SubPicAllocator(IDirect3DDevice7* pD3DDev, SIZE maxsize);
};
