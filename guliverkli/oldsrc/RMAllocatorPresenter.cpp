#include "stdafx.h"
#include "mplayerc.h"
#include <atlbase.h>
#include <atlcoll.h>
#include <ddraw.h>
#include <d3d.h>
#include "RMAllocatorPresenter.h"
#include "..\..\SubPic\DX7SubPic.h"
#include "..\..\..\include\RealMedia\pntypes.h"
#include "..\..\..\include\RealMedia\pnwintyp.h"
#include "..\..\..\include\RealMedia\pncom.h"
#include "..\..\..\include\RealMedia\rmavsurf.h"
#include "..\..\DSUtil\DSUtil.h"

namespace DSObjects
{

class CRMAllocatorPresenter
	: public ISubPicAllocatorPresenterImpl
	, public IRMAVideoSurface
{
	CSize m_ScreenSize;

	CComPtr<IDirectDraw7> m_pDD;
	CComQIPtr<IDirect3D7, &IID_IDirect3D7> m_pD3D;
    CComPtr<IDirect3DDevice7> m_pD3DDev;

	CComPtr<IDirectDrawSurface7> m_pPrimary, m_pBackBuffer, m_pVideoSurfaceCompat, m_pVideoSurfaceYUY2;

    HRESULT CreateDevice();
	void DeleteSurfaces();
	HRESULT AllocateSurfaces(CSize size);

    RMABitmapInfoHeader m_bitmapInfo;
    RMABitmapInfoHeader m_lastBitmapInfo;

public:
	CRMAllocatorPresenter(HWND hWnd, HRESULT& hr);
	virtual ~CRMAllocatorPresenter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
	STDMETHODIMP_(bool) Paint(bool fAll);

	// IRMAVideoSurface
    STDMETHODIMP Blt(UCHAR*	/*IN*/ pImageData, RMABitmapInfoHeader* /*IN*/ pBitmapInfo, REF(PNxRect) /*IN*/ inDestRect, REF(PNxRect) /*IN*/ inSrcRect);
	STDMETHODIMP BeginOptimizedBlt(RMABitmapInfoHeader* /*IN*/ pBitmapInfo);
	STDMETHODIMP OptimizedBlt(UCHAR* /*IN*/ pImageBits, REF(PNxRect) /*IN*/ rDestRect, REF(PNxRect) /*IN*/ rSrcRect);
	STDMETHODIMP EndOptimizedBlt();
	STDMETHODIMP GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType);
    STDMETHODIMP GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType);
};

}
using namespace DSObjects;

static HRESULT AllocDX7Surface(IDirectDraw7* pDD, CSize size, DWORD compression, int bpp, IDirectDrawSurface7** pSurface)
{
	if(!pDD || !pSurface || size.cx <= 0 || size.cy <= 0)
		return E_POINTER;

	*pSurface = NULL;

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	ddsd.dwFlags = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY;
	ddsd.dwWidth = size.cx;
	ddsd.dwHeight = size.cy;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

	if(compression >= 0x1000)
	{
		ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		ddsd.ddpfPixelFormat.dwFourCC = compression;
	}
	else if((compression == 0 || compression == 3) && (bpp == 16 || bpp == 24 || bpp == 32))
	{
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask	= (bpp == 16) ? 0xF000 : 0xFF000000;
		ddsd.ddpfPixelFormat.dwRBitMask			= (bpp == 16) ? 0x0F00 : 0x00FF0000;
		ddsd.ddpfPixelFormat.dwGBitMask			= (bpp == 16) ? 0x00F0 : 0x0000FF00;
		ddsd.ddpfPixelFormat.dwBBitMask			= (bpp == 16) ? 0x000F : 0x000000FF;
	}

	return pDD->CreateSurface(&ddsd, pSurface, NULL);
}

#define MY_USER_ID 0x6ABE51

// CRMAllocatorPresenter

CRMAllocatorPresenter::CRMAllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: ISubPicAllocatorPresenterImpl(hWnd)
	, m_ScreenSize(0, 0)
{
    if(FAILED(hr = DirectDrawCreateEx(NULL, (VOID**)&m_pDD, IID_IDirectDraw7, NULL))
	|| FAILED(hr = m_pDD->SetCooperativeLevel(AfxGetMainWnd()->GetSafeHwnd(), DDSCL_NORMAL)))
		return;

	if(!(m_pD3D = m_pDD))
	{
		hr = E_NOINTERFACE;
		return;
	}

	GetWindowRect(m_hWnd, &m_WindowRect);

	hr = CreateDevice();
}

CRMAllocatorPresenter::~CRMAllocatorPresenter()
{
}

STDMETHODIMP CRMAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI2(IRMAVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRMAllocatorPresenter::CreateDevice()
{
    m_pD3DDev = NULL;
	m_pAllocator = NULL;
	m_pSubPicQueue = NULL;

	m_pPrimary = NULL;
	m_pBackBuffer = NULL;
	m_pVideoSurfaceCompat = NULL;
	m_pVideoSurfaceYUY2 = NULL;

    DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
    if(FAILED(m_pDD->GetDisplayMode(&ddsd))
	|| ddsd.ddpfPixelFormat.dwRGBBitCount <= 8)
		return DDERR_INVALIDMODE;

	m_ScreenSize.SetSize(ddsd.dwWidth, ddsd.dwHeight);

	HRESULT hr;

	// m_pPrimary

	INITDDSTRUCT(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if(FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pPrimary, NULL)))
        return hr;

	CComPtr<IDirectDrawClipper> pcClipper;
    if(FAILED(hr = m_pDD->CreateClipper(0, &pcClipper, NULL)))
        return hr;
	pcClipper->SetHWnd(0, m_hWnd);
	m_pPrimary->SetClipper(pcClipper);

	// m_pBackBuffer

	INITDDSTRUCT(ddsd);
    ddsd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
	ddsd.dwWidth = m_ScreenSize.cx;
	ddsd.dwHeight = m_ScreenSize.cy;
	if(FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pBackBuffer, NULL)))
        return hr;

	DDBLTFX fx;
	INITDDSTRUCT(fx);
	fx.dwFillColor = 0;
	m_pBackBuffer->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);

	pcClipper = NULL;
    if(FAILED(hr = m_pDD->CreateClipper(0, &pcClipper, NULL)))
		return hr;
    BYTE rgnDataBuffer[1024];
	HRGN hrgn = CreateRectRgn(0, 0, ddsd.dwWidth, ddsd.dwHeight);
	GetRegionData(hrgn, sizeof(rgnDataBuffer), (RGNDATA*)rgnDataBuffer);
	DeleteObject(hrgn);
	pcClipper->SetClipList((RGNDATA*)rgnDataBuffer, 0);
	m_pBackBuffer->SetClipper(pcClipper);

	// m_pD3DDev

	if(FAILED(hr = m_pD3D->CreateDevice(IID_IDirect3DHALDevice, m_pBackBuffer, &m_pD3DDev))) // this seems to fail if the desktop size is too large (width or height >2048)
		return hr;

	//

	CSize size;
	switch(AfxGetAppSettings().nSPCMaxRes)
	{
	case 0: default: size = m_ScreenSize; break;
	case 1: size.SetSize(1024, 768); break;
	case 2: size.SetSize(800, 600); break;
	case 3: size.SetSize(640, 480); break;
	case 4: size.SetSize(512, 384); break;
	case 5: size.SetSize(384, 288); break;
	}

	m_pAllocator = new CDX7SubPicAllocator(m_pD3DDev, size);
	if(!m_pAllocator)
		return E_FAIL;

	hr = S_OK;
	m_pSubPicQueue = AfxGetAppSettings().nSPCSize > 0 
		? (ISubPicQueue*)new CSubPicQueue(AfxGetAppSettings().nSPCSize, m_pAllocator, &hr)
		: (ISubPicQueue*)new CSubPicQueueNoThread(m_pAllocator, &hr);
	if(!m_pSubPicQueue || FAILED(hr))
		return E_FAIL;

	return S_OK;
}

HRESULT CRMAllocatorPresenter::AllocateSurfaces(CSize size)
{
    CAutoLock cAutoLock(this);

	DeleteSurfaces();

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(!m_pBackBuffer || FAILED(m_pBackBuffer->GetSurfaceDesc(&ddsd)))
		return E_FAIL;

	AllocDX7Surface(m_pDD, size, BI_RGB, ddsd.ddpfPixelFormat.dwRGBBitCount, &m_pVideoSurfaceCompat);
	AllocDX7Surface(m_pDD, size, '2YUY', 16, &m_pVideoSurfaceYUY2);
	if(FAILED(m_pVideoSurfaceCompat->Blt(NULL, m_pVideoSurfaceYUY2, NULL, DDBLT_WAIT, NULL)))
		m_pVideoSurfaceYUY2 = NULL;

	DDBLTFX fx;
	INITDDSTRUCT(fx);
	fx.dwFillColor = 0;
	m_pVideoSurfaceCompat->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
	if(m_pVideoSurfaceYUY2)
	{
		fx.dwFillColor = 0x80108010;
		m_pVideoSurfaceYUY2->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
	}

	m_NativeVideoSize = m_AspectRatio = size;

	return S_OK;
}

void CRMAllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pVideoSurfaceCompat = NULL;
	m_pVideoSurfaceYUY2 = NULL;
}

// ISubPicAllocatorPresenter

STDMETHODIMP CRMAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(bool) CRMAllocatorPresenter::Paint(bool fAll)
{
	CAutoLock cAutoLock(this);

	if(m_WindowRect.right <= m_WindowRect.left || m_WindowRect.bottom <= m_WindowRect.top
	|| m_NativeVideoSize.cx <= 0 || m_NativeVideoSize.cy <= 0
	|| !m_pPrimary || !m_pBackBuffer)
		return(false);

	HRESULT hr;

	CRect rSrcVid(CPoint(0, 0), m_NativeVideoSize);
	CRect rDstVid(m_VideoRect);

	CRect rSrcPri(CPoint(0, 0), m_WindowRect.Size());
	CRect rDstPri(m_WindowRect);
	MapWindowRect(m_hWnd, HWND_DESKTOP, &rDstPri);

	if(fAll)
	{
		// clear the backbuffer

		CRect rl(0, 0, rDstVid.left, rSrcPri.bottom);
		CRect rr(rDstVid.right, 0, rSrcPri.right, rSrcPri.bottom);
		CRect rt(0, 0, rSrcPri.right, rDstVid.top);
		CRect rb(0, rDstVid.bottom, rSrcPri.right, rSrcPri.bottom);

		DDBLTFX fx;
		INITDDSTRUCT(fx);
		fx.dwFillColor = 0;
		if(!rl.IsRectEmpty()) hr = m_pBackBuffer->Blt(&rl, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
		if(!rr.IsRectEmpty()) hr = m_pBackBuffer->Blt(&rr, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
		if(!rt.IsRectEmpty()) hr = m_pBackBuffer->Blt(&rt, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
		if(!rb.IsRectEmpty()) hr = m_pBackBuffer->Blt(&rb, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);

		// paint the video on the backbuffer

		hr = m_pBackBuffer->Blt(rDstVid, m_pVideoSurfaceCompat, rSrcVid, DDBLT_WAIT, NULL);

		// paint the text on the backbuffer

		CComPtr<ISubPic> pSubPic;
		if(m_pSubPicQueue->LookupSubPic(m_rtNow, &pSubPic))
		{
			SubPicDesc spd;
			pSubPic->GetDesc(spd);

			CRect r;
			pSubPic->GetDirtyRect(r);

			r.DeflateRect(1, 1); // FIXME

			CRect rDstText(rSrcPri);
			rDstText.SetRect(
				r.left * rSrcPri.Width() / spd.w,
				r.top * rSrcPri.Height() / spd.h,
				r.right * rSrcPri.Width() / spd.w,
				r.bottom * rSrcPri.Height() / spd.h);

			pSubPic->AlphaBlt(r, rDstText);
		}
	}

	// wait vsync

	m_pDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);

	// blt to the primary surface

	hr = m_pPrimary->Blt(rDstPri, m_pBackBuffer, rSrcPri, DDBLT_WAIT, NULL);

	if(hr == DDERR_SURFACELOST)
	{
		// a display change doesn't seem to make this return DDERR_WRONGMODE always
		hr = DDERR_WRONGMODE; // m_pDD->TestCooperativeLevel();

		if(hr == DDERR_WRONGMODE) 
		{
			DeleteSurfaces();
			if(FAILED(CreateDevice()))
				return(false);

			if(FAILED(AllocateSurfaces(m_NativeVideoSize)))
				return(false);
		}

		hr = S_OK;
	}

	return(true);
}

// IRMAVideoSurface

STDMETHODIMP CRMAllocatorPresenter::Blt(UCHAR* /*IN*/ pImageData, RMABitmapInfoHeader* /*IN*/ pBitmapInfo, REF(PNxRect) /*IN*/ inDestRect, REF(PNxRect) /*IN*/ inSrcRect)
{
	CRect src((RECT*)&inSrcRect), dst((RECT*)&inDestRect), src2(CPoint(0,0), src.Size());

	if(!m_pVideoSurfaceCompat)
		return E_FAIL;

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(FAILED(m_pVideoSurfaceCompat->GetSurfaceDesc(&ddsd)))
		return E_FAIL;

	bool fRGB = !!(ddsd.ddpfPixelFormat.dwFlags&DDPF_RGB);
	bool fFourCC = !!(ddsd.ddpfPixelFormat.dwFlags&DDPF_FOURCC);

	DWORD fccin = pBitmapInfo->biCompression;
	DWORD fccout = ddsd.ddpfPixelFormat.dwFourCC;

	int bppin = pBitmapInfo->biBitCount;
	int bppout = ddsd.ddpfPixelFormat.dwRGBBitCount;

	int h = min(src.Height(), dst.Height());

	if(src.left >= src.right || src.top >= src.bottom 
	|| ((fccin == 0 || fccin == 3) && src.Size() != dst.Size())
	|| (fccin >= 0x1000 && ((fccin != '2YUY' && fccin != '024I' && fccin != '21VY') || !m_pVideoSurfaceYUY2 || src.Width() > dst.Width() || src.Height() > dst.Height())))
	{
		DDBLTFX fx;
		INITDDSTRUCT(fx);
		fx.dwFillColor = (ddsd.ddpfPixelFormat.dwFlags&DDPF_RGB) ? 0 : 0x80008000;
		m_pVideoSurfaceCompat->Blt((RECT*)&dst, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurfaceCompat->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this color format or source/destination rectangle is not supported"), fccin);

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x202020);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurfaceCompat->ReleaseDC(hDC);
			
			Paint(true);
		}
	}
	else if(fccin == '2YUY' && m_pVideoSurfaceYUY2)
	{
		INITDDSTRUCT(ddsd);
		if(SUCCEEDED(m_pVideoSurfaceYUY2->Lock(src2, &ddsd, DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, NULL)))
		{
			int pitchIn = pBitmapInfo->biWidth*2;
			int pitchOut = ddsd.lPitch;

			BYTE* pDataIn = (BYTE*)pImageData + src.top*pitchIn + src.left*2;
			BYTE* pDataOut = (BYTE*)ddsd.lpSurface;

			for(int y = 0, h = src.Height(), w = src.Width(); y < h; y++, pDataIn += pitchIn, pDataOut += pitchOut)
				memcpy(pDataOut, pDataIn, w*2);

			m_pVideoSurfaceYUY2->Unlock(src2);

			m_pVideoSurfaceCompat->Blt(dst, m_pVideoSurfaceYUY2, src2, DDBLT_WAIT, NULL);

			Paint(true);
		}
	}
	else if((fccin == '024I' || fccin == '21VY') && m_pVideoSurfaceYUY2)
	{
		ASSERT((src.Width()&1) == 0);
		ASSERT((src.Height()&1) == 0);

		INITDDSTRUCT(ddsd);
		if(SUCCEEDED(m_pVideoSurfaceYUY2->Lock(src2, &ddsd, DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, NULL)))
		{
			int pitchIn = pBitmapInfo->biWidth;
			int pitchInUV = pitchIn>>1;
			int pitchOut = ddsd.lPitch;

			BYTE* pDataIn = (BYTE*)pImageData + src.top*pitchIn + src.left;
			BYTE* pDataInU = pDataIn + pitchIn*pBitmapInfo->biHeight;
			BYTE* pDataInV = pDataInU + pitchInUV*pBitmapInfo->biHeight/2;
			BYTE* pDataOut = (BYTE*)ddsd.lpSurface;

			if(fccin == '21VY') {BYTE* p = pDataInU; pDataInU = pDataInV; pDataInV = p;}

			for(int y = 0, h = src.Height(); y < h; y+=2, pDataIn += pitchIn*2, pDataInU += pitchInUV, pDataInV += pitchInUV, pDataOut += pitchOut*2)
			{
				BYTE* pIn = (BYTE*)pDataIn;
				BYTE* pInU = (BYTE*)pDataInU;
				BYTE* pInV = (BYTE*)pDataInV;
				WORD* pOut = (WORD*)pDataOut;

				for(int x = 0, w = src.Width(); x < w; x+=2)
				{
					*pOut++ = (*pInU++<<8)|*pIn++;
					*pOut++ = (*pInV++<<8)|*pIn++;
				}

				pIn = (BYTE*)pDataIn + pitchIn;
				pInU = (BYTE*)pDataInU;
				pInV = (BYTE*)pDataInV;
				pOut = (WORD*)(pDataOut + pitchOut);

				if(y < h-2)
				{
					for(int x = 0, w = src.Width(); x < w; x+=2, pInU++, pInV++)
					{
						*pOut++ = (((pInU[0]+pInU[pitchInUV])>>1)<<8)|*pIn++;
						*pOut++ = (((pInV[0]+pInV[pitchInUV])>>1)<<8)|*pIn++;
					}
				}
				else
				{
					for(int x = 0, w = src.Width(); x < w; x+=2)
					{
						*pOut++ = (*pInU++<<8)|*pIn++;
						*pOut++ = (*pInV++<<8)|*pIn++;
					}
				}
			}

			m_pVideoSurfaceYUY2->Unlock(src2);

			m_pVideoSurfaceCompat->Blt(dst, m_pVideoSurfaceYUY2, src2, DDBLT_WAIT, NULL);

			Paint(true);
		}
	}
	else if((fccin == 0 || fccin == 3) && (bppin == 16 || bppin == 24 || bppin == 32))
	{
		INITDDSTRUCT(ddsd);
		if(SUCCEEDED(m_pVideoSurfaceCompat->Lock(dst, &ddsd, DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, NULL)))
		{
			int pitchIn = pBitmapInfo->biWidth*bppin>>3;
			int pitchOut = ddsd.lPitch;
			int pitchMin = min(pitchIn, pitchOut);

			int w = min(pitchIn*8/bppin, pitchOut*8/bppout);

			BYTE* pDataIn = (BYTE*)pImageData + src.top*pitchIn + ((src.left*bppin)>>3);
			BYTE* pDataOut = (BYTE*)ddsd.lpSurface;

			if(fccin == 0)
			{
				pDataIn += (h-1)*pitchIn;
				pitchIn = -pitchIn;
			}

			for(int y = 0; y < h; y++, pDataIn += pitchIn, pDataOut += pitchOut)
			{
				if(bppin == bppout)
				{
					memcpy(pDataOut, pDataIn, pitchMin);
				}
				else if(bppin == 16 && bppout == 32)
				{
					WORD* pIn = (WORD*)pDataIn;
					DWORD* pOut = (DWORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = ((*pIn&0xf800)<<8)|((*pIn&0x07e0)<<5)|(*pIn&0x001f);
						pIn++;
					}
				}
				else if(bppin == 32 && bppout == 16)
				{
					DWORD* pIn = (DWORD*)pDataIn;
					WORD* pOut = (WORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = ((*pIn>>8)&0xf800)|((*pIn>>5)&0x07e0)|((*pIn>>3)&0x001f);
						pIn++;
					}
				}
				else if(bppin == 24 && bppout == 16)
				{
					BYTE* pIn = pDataIn;
					WORD* pOut = (WORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = ((*((DWORD*)pIn)>>8)&0xf800)|((*((DWORD*)pIn)>>5)&0x07e0)|((*((DWORD*)pIn)>>3)&0x001f);
						pIn += 3;
					}
				}
				else if(bppin == 24 && bppout == 32)
				{
					BYTE* pIn = pDataIn;
					DWORD* pOut = (DWORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = *((DWORD*)pIn)&0xffffff;
						pIn += 3;
					}
				}
			}

			m_pVideoSurfaceCompat->Unlock(dst);
		}
		
		Paint(true);
	}

	return PNR_OK;
}

STDMETHODIMP CRMAllocatorPresenter::BeginOptimizedBlt(RMABitmapInfoHeader* /*IN*/ pBitmapInfo)
{
	AllocateSurfaces(CSize(pBitmapInfo->biWidth, abs(pBitmapInfo->biHeight)));

	return PNR_NOTIMPL;
}

STDMETHODIMP CRMAllocatorPresenter::OptimizedBlt(UCHAR* /*IN*/ pImageBits, REF(PNxRect) /*IN*/ rDestRect, REF(PNxRect) /*IN*/ rSrcRect)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRMAllocatorPresenter::EndOptimizedBlt()
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRMAllocatorPresenter::GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRMAllocatorPresenter::GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType)
{
//	ulType = RMA_YUY2;
	ulType = RMA_I420;
	return PNR_OK;
}

//

HRESULT CreateRMAP(HWND hWnd, ISubPicAllocatorPresenter** pRMAP)
{
	if(!pRMAP)
		return E_POINTER;

	HRESULT hr;
	if(!(*pRMAP = new CRMAllocatorPresenter(hWnd, hr)))
		return E_OUTOFMEMORY;

	(*pRMAP)->AddRef();

	if(FAILED(hr))
	{
		(*pRMAP)->Release();
		*pRMAP = NULL;
	}

	return hr;
}
