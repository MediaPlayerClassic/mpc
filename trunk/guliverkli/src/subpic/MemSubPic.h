#pragma once

#include "ISubPic.h"

enum {MSP_RGB32,MSP_RGB24,MSP_RGB16,MSP_RGB15,MSP_YUY2,MSP_YV12,MSP_IYUV,MSP_AYUV};

// CMemSubPic

class CMemSubPic : public ISubPicImpl
{
	SubPicDesc m_spd;

protected:
	STDMETHODIMP_(void*) GetObject(); // returns SubPicDesc*

public:
	CMemSubPic(SubPicDesc& spd);
	virtual ~CMemSubPic();

	// ISubPic
	STDMETHODIMP GetDesc(SubPicDesc& spd);
	STDMETHODIMP CopyTo(ISubPic* pSubPic);
	STDMETHODIMP ClearDirtyRect(DWORD color);
	STDMETHODIMP Lock(SubPicDesc& spd);
	STDMETHODIMP Unlock(RECT* pDirtyRect);
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget);
};

// CMemSubPicAllocator

class CMemSubPicAllocator : public ISubPicAllocatorImpl
{
	int m_type;
	CSize m_maxsize;

	bool Alloc(bool fStatic, ISubPic** ppSubPic);

public:
	CMemSubPicAllocator(int type, SIZE maxsize);
};

