/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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
#include "mplayerc.h"
#include <atlbase.h>
#include <atlcoll.h>
#include "..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "DX7AllocatorPresenter.h"
#include <ddraw.h>
#include <d3d.h>
#include "..\..\SubPic\DX7SubPic.h"
#include "..\..\..\include\RealMedia\pntypes.h"
#include "..\..\..\include\RealMedia\pnwintyp.h"
#include "..\..\..\include\RealMedia\pncom.h"
#include "..\..\..\include\RealMedia\rmavsurf.h"
#include "IQTVideoSurface.h"

namespace DSObjects
{

class CDX7AllocatorPresenter
	: public ISubPicAllocatorPresenterImpl
{
protected:
	CSize m_ScreenSize;

	CComPtr<IDirectDraw7> m_pDD;
	CComQIPtr<IDirect3D7, &IID_IDirect3D7> m_pD3D;
    CComPtr<IDirect3DDevice7> m_pD3DDev;

	CComPtr<IDirectDrawSurface7> m_pPrimary, m_pBackBuffer, m_pVideoSurface;

    virtual HRESULT CreateDevice();
	virtual void DeleteSurfaces();
	virtual bool OnDeviceLost();

public:
	CDX7AllocatorPresenter(HWND hWnd, HRESULT& hr);
	virtual ~CDX7AllocatorPresenter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
	STDMETHODIMP_(bool) Paint(bool fAll);
};

class CVMR7AllocatorPresenter
	: public CDX7AllocatorPresenter
	, public IVMRSurfaceAllocator
	, public IVMRImagePresenter
	, public IVMRWindowlessControl
{
	CComPtr<IVMRSurfaceAllocator> m_pSA;
	CComPtr<IVMRSurfaceAllocatorNotify> m_pIVMRSurfAllocNotify;

	void DeleteSurfaces();
	bool OnDeviceLost();

public:
	CVMR7AllocatorPresenter(HWND hWnd, HRESULT& hr);
	virtual ~CVMR7AllocatorPresenter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

	// IVMRSurfaceAllocator
    STDMETHODIMP AllocateSurface(DWORD_PTR dwUserID, VMRALLOCATIONINFO* lpAllocInfo, DWORD* lpdwBuffer, LPDIRECTDRAWSURFACE7* lplpSurface);
    STDMETHODIMP FreeSurface(DWORD_PTR dwUserID);
    STDMETHODIMP PrepareSurface(DWORD_PTR dwUserID, IDirectDrawSurface7* lpSurface, DWORD dwSurfaceFlags);
    STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify);

	// IVMRImagePresenter
    STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO* lpPresInfo);

	// IVMRWindowlessControl
	STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
	STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight);
	STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight);
	STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
    STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect);
	STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode);
	STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode);
	STDMETHODIMP SetVideoClippingWindow(HWND hwnd);
	STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc);
	STDMETHODIMP DisplayModeChanged();
	STDMETHODIMP GetCurrentImage(BYTE** lpDib);
	STDMETHODIMP SetBorderColor(COLORREF Clr);
	STDMETHODIMP GetBorderColor(COLORREF* lpClr);
	STDMETHODIMP SetColorKey(COLORREF Clr);
	STDMETHODIMP GetColorKey(COLORREF* lpClr);
};

class CRM7AllocatorPresenter
	: public CDX7AllocatorPresenter
	, public IRMAVideoSurface
{
	CComPtr<IDirectDrawSurface7> m_pVideoSurfaceYUY2;

	void DeleteSurfaces();
	bool OnDeviceLost();

	HRESULT AllocateSurfaces(CSize size);
    RMABitmapInfoHeader m_bitmapInfo;
    RMABitmapInfoHeader m_lastBitmapInfo;

public:
	CRM7AllocatorPresenter(HWND hWnd, HRESULT& hr);
	virtual ~CRM7AllocatorPresenter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IRMAVideoSurface
    STDMETHODIMP Blt(UCHAR*	/*IN*/ pImageData, RMABitmapInfoHeader* /*IN*/ pBitmapInfo, REF(PNxRect) /*IN*/ inDestRect, REF(PNxRect) /*IN*/ inSrcRect);
	STDMETHODIMP BeginOptimizedBlt(RMABitmapInfoHeader* /*IN*/ pBitmapInfo);
	STDMETHODIMP OptimizedBlt(UCHAR* /*IN*/ pImageBits, REF(PNxRect) /*IN*/ rDestRect, REF(PNxRect) /*IN*/ rSrcRect);
	STDMETHODIMP EndOptimizedBlt();
	STDMETHODIMP GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType);
    STDMETHODIMP GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType);
};

class CQT7AllocatorPresenter
	: public CDX7AllocatorPresenter
	, public IQTVideoSurface
{
	bool OnDeviceLost();

	HRESULT AllocateSurfaces(CSize size);

public:
	CQT7AllocatorPresenter(HWND hWnd, HRESULT& hr);
	virtual ~CQT7AllocatorPresenter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IQTVideoSurface
	STDMETHODIMP BeginBlt(const BITMAP& bm);
	STDMETHODIMP DoBlt(const BITMAP& bm);
};

}
using namespace DSObjects;

//

HRESULT CreateAP7(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP)
{
	CheckPointer(ppAP, E_POINTER);

	*ppAP = NULL;

	HRESULT hr;
	if(clsid == CLSID_VMR7AllocatorPresenter && !(*ppAP = new CVMR7AllocatorPresenter(hWnd, hr))
	|| clsid == CLSID_RM7AllocatorPresenter && !(*ppAP = new CRM7AllocatorPresenter(hWnd, hr))
	|| clsid == CLSID_QT7AllocatorPresenter && !(*ppAP = new CQT7AllocatorPresenter(hWnd, hr)))
		return E_OUTOFMEMORY;

	if(*ppAP == NULL)
		return E_FAIL;

	(*ppAP)->AddRef();

	if(FAILED(hr))
	{
		(*ppAP)->Release();
		*ppAP = NULL;
	}

	return hr;
}

//
// CDX7AllocatorPresenter
//

CDX7AllocatorPresenter::CDX7AllocatorPresenter(HWND hWnd, HRESULT& hr) 
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

CDX7AllocatorPresenter::~CDX7AllocatorPresenter()
{
}

STDMETHODIMP CDX7AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDX7AllocatorPresenter::CreateDevice()
{
    m_pD3DDev = NULL;
	m_pAllocator = NULL;
	m_pSubPicQueue = NULL;

	m_pPrimary = NULL;
	m_pBackBuffer = NULL;

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
	if(!m_pAllocator || FAILED(hr))
		return E_FAIL;

	hr = S_OK;
	m_pSubPicQueue = AfxGetAppSettings().nSPCSize > 0 
		? (ISubPicQueue*)new CSubPicQueue(AfxGetAppSettings().nSPCSize, m_pAllocator, &hr)
		: (ISubPicQueue*)new CSubPicQueueNoThread(m_pAllocator, &hr);
	if(!m_pSubPicQueue || FAILED(hr))
		return E_FAIL;

	return S_OK;
}

void CDX7AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pVideoSurface = NULL;
}

bool CDX7AllocatorPresenter::OnDeviceLost()
{
	// a display change doesn't seem to return DDERR_WRONGMODE always
	HRESULT hr = DDERR_WRONGMODE; // m_pDD->TestCooperativeLevel();

	if(hr == DDERR_WRONGMODE) 
	{
		DeleteSurfaces();
		if(SUCCEEDED(CreateDevice()))
			return(true);
	}

	return(false);
}

// ISubPicAllocatorPresenter

STDMETHODIMP CDX7AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(bool) CDX7AllocatorPresenter::Paint(bool fAll)
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

		if(m_pVideoSurface && !rDstVid.IsRectEmpty())
			hr = m_pBackBuffer->Blt(rDstVid, m_pVideoSurface, rSrcVid, DDBLT_WAIT, NULL);

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
		OnDeviceLost();

		hr = S_OK;
	}

	return(true);
}

//
// CVMR7AllocatorPresenter
//

#define MY_USER_ID 0x6ABE51

CVMR7AllocatorPresenter::CVMR7AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: CDX7AllocatorPresenter(hWnd, hr)
{
    if(FAILED(hr))
		return;

	if(FAILED(hr = m_pSA.CoCreateInstance(CLSID_AllocPresenter)))
	{
		hr = E_FAIL;
		return;
	}
}

CVMR7AllocatorPresenter::~CVMR7AllocatorPresenter()
{
}

STDMETHODIMP CVMR7AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IVMRSurfaceAllocator)
		QI(IVMRImagePresenter)
		QI(IVMRWindowlessControl)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CVMR7AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

	// IMPORTANT: __super::DeleteSurfaces() also releases m_pVideoSurface, 
	// but m_pSA->FreeSurface(MY_USER_ID) seems to be deleting it directly (!), 
	// so it wouldn't be wise to keep the pointer any longer.
	m_pVideoSurface = NULL;
	m_pSA->FreeSurface(MY_USER_ID);

	__super::DeleteSurfaces();
}

bool CVMR7AllocatorPresenter::OnDeviceLost()
{
	if(!__super::OnDeviceLost())
		return(false);

	HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	if(FAILED(m_pIVMRSurfAllocNotify->ChangeDDrawDevice(m_pDD, hMonitor)))
		return(false);

	return(true);
}

// ISubPicAllocatorPresenter

STDMETHODIMP CVMR7AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

	*ppRenderer = NULL;

	HRESULT hr;

	do
	{
		CComPtr<IBaseFilter> pBF;

		if(FAILED(hr = pBF.CoCreateInstance(CLSID_VideoMixingRenderer)))
			break;

		CComQIPtr<IVMRFilterConfig> pConfig = pBF;
		if(!pConfig)
			break;

		if(FAILED(hr = pConfig->SetRenderingMode(VMRMode_Renderless)))
			break;

		CComQIPtr<IVMRSurfaceAllocatorNotify> pSAN = pBF;
		if(!pSAN)
			break;

		if(FAILED(hr = pSAN->AdviseSurfaceAllocator(MY_USER_ID, static_cast<IVMRSurfaceAllocator*>(this)))
		|| FAILED(hr = AdviseNotify(pSAN)))
			break;

		*ppRenderer = (IUnknown*)pBF.Detach();

		return S_OK;
	}
	while(0);

    return E_FAIL;
}

// IVMRSurfaceAllocator

STDMETHODIMP CVMR7AllocatorPresenter::AllocateSurface(DWORD_PTR dwUserID, VMRALLOCATIONINFO* lpAllocInfo, DWORD* lpdwBuffer, LPDIRECTDRAWSURFACE7* lplpSurface)
{
	if(!lpAllocInfo || !lpdwBuffer || !lplpSurface)
		return E_POINTER;

	if(!m_pIVMRSurfAllocNotify)
		return E_FAIL;

	HRESULT hr;

    DeleteSurfaces();

	// HACK: yv12 will fail to blt onto the backbuffer anyway, but if we first
	// allocate it and then let our FreeSurface callback call m_pSA->FreeSurface,
	// then that might stall for about 30 seconds because of some unknown buggy code 
	// behind <ddraw surface>->Release()

	if(lpAllocInfo->lpHdr->biBitCount < 16)
		return E_FAIL;

	hr = m_pSA->AllocateSurface(dwUserID, lpAllocInfo, lpdwBuffer, lplpSurface);
	if(FAILED(hr))
		return hr;

	{
		hr = m_pBackBuffer->Blt(NULL, *lplpSurface, NULL, DDBLT_WAIT, NULL);
		if(FAILED(hr))
			return hr;

		DDBLTFX fx;
		INITDDSTRUCT(fx);
		fx.dwFillColor = 0;
		m_pBackBuffer->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
	}

	m_NativeVideoSize = CSize(abs(lpAllocInfo->lpHdr->biWidth), abs(lpAllocInfo->lpHdr->biHeight));
	m_AspectRatio = m_NativeVideoSize;
	int arx = lpAllocInfo->szAspectRatio.cx, ary = lpAllocInfo->szAspectRatio.cy;
	if(arx > 0 && ary > 0) m_AspectRatio.SetSize(arx, ary);

	return hr;
}

STDMETHODIMP CVMR7AllocatorPresenter::FreeSurface(DWORD_PTR dwUserID)
{
    DeleteSurfaces();
	return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::PrepareSurface(DWORD_PTR dwUserID, IDirectDrawSurface7* lpSurface, DWORD dwSurfaceFlags)
{
    if(!lpSurface)
		return E_POINTER;

	// FIXME: sometimes the msmpeg4/divx3/wmv decoder wants to reuse our 
	// surface (expects it to point to the same mem every time), and to avoid 
	// problems we can't cal m_pSA->PrepareSurface (flips? clears?).
	return S_OK; 
/*
	return m_pSA->PrepareSurface(dwUserID, lpSurface, dwSurfaceFlags);
*/
}

STDMETHODIMP CVMR7AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify)
{
    CAutoLock cAutoLock(this);
	
	m_pIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;

	HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	if(FAILED(m_pIVMRSurfAllocNotify->SetDDrawDevice(m_pDD, hMonitor)))
		return E_FAIL;

	return m_pSA->AdviseNotify(lpIVMRSurfAllocNotify);
}

// IVMRImagePresenter

STDMETHODIMP CVMR7AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
    CAutoLock cAutoLock(this);

    ASSERT(m_pD3DDev);

	return m_pD3DDev ? S_OK : E_FAIL;
}

STDMETHODIMP CVMR7AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
	return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO* lpPresInfo)
{
    HRESULT hr;

	{
		if(!lpPresInfo || !lpPresInfo->lpSurf)
			return E_POINTER;

		CAutoLock cAutoLock(this);

		m_pVideoSurface = lpPresInfo->lpSurf;

		m_fps = 10000000.0 / (lpPresInfo->rtEnd - lpPresInfo->rtStart);
		if(m_pSubPicQueue)
			m_pSubPicQueue->SetFPS(m_fps);

		CSize VideoSize = m_NativeVideoSize;
		int arx = lpPresInfo->szAspectRatio.cx, ary = lpPresInfo->szAspectRatio.cy;
		if(arx > 0 && ary > 0) VideoSize.cx = VideoSize.cy*arx/ary;
		if(VideoSize != GetVideoSize())
		{
			m_AspectRatio.SetSize(arx, ary);
			AfxGetApp()->m_pMainWnd->PostMessage(WM_REARRANGERENDERLESS);
		}

		Paint(true);

		hr = S_OK;
	}

	return S_OK;
}

// IVMRWindowlessControl
//
// It is only implemented (partially) for the dvd navigator's 
// menu handling, which needs to know a few things about the 
// location of our window.

STDMETHODIMP CVMR7AllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	CSize vs = m_NativeVideoSize, ar = m_AspectRatio;
	// DVD Nav. bug workaround fix
	vs.cx = vs.cy * ar.cx / ar.cy;
	if(lpWidth) *lpWidth = vs.cx;
	if(lpHeight) *lpHeight = vs.cy;
	if(lpARWidth) *lpARWidth = ar.cx;
	if(lpARHeight) *lpARHeight = ar.cy;
	return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;} // we have our own method for this

STDMETHODIMP CVMR7AllocatorPresenter::GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
{
	CopyRect(lpSRCRect, CRect(CPoint(0, 0), m_NativeVideoSize));
	CopyRect(lpDSTRect, &m_VideoRect);
	// DVD Nav. bug workaround fix
	GetNativeVideoSize(&lpSRCRect->right, &lpSRCRect->bottom, NULL, NULL);
	return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetAspectRatioMode(DWORD* lpAspectRatioMode)
{
	if(lpAspectRatioMode) *lpAspectRatioMode = AM_ARMODE_STRETCHED;

	return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::DisplayModeChanged() {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}

STDMETHODIMP CVMR7AllocatorPresenter::GetBorderColor(COLORREF* lpClr)
{
	if(lpClr) *lpClr = 0;
	return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetColorKey(COLORREF Clr) {return E_NOTIMPL;}
STDMETHODIMP CVMR7AllocatorPresenter::GetColorKey(COLORREF* lpClr) {return E_NOTIMPL;}

//

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
	else if((compression == 0 || compression == 3) && (bpp == 15 || bpp == 16 || bpp == 24 || bpp == 32))
	{
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = max(bpp, 16);
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask	= (bpp == 16) ? 0x0000 : (bpp == 15) ? 0x8000 : 0xFF000000;
		ddsd.ddpfPixelFormat.dwRBitMask			= (bpp == 16) ? 0xf800 : (bpp == 15) ? 0x7c00 : 0x00FF0000;
		ddsd.ddpfPixelFormat.dwGBitMask			= (bpp == 16) ? 0x07e0 : (bpp == 15) ? 0x03e0 : 0x0000FF00;
		ddsd.ddpfPixelFormat.dwBBitMask			= (bpp == 16) ? 0x001F : (bpp == 15) ? 0x001F : 0x000000FF;
	}

	return pDD->CreateSurface(&ddsd, pSurface, NULL);
}

//
// CRM7AllocatorPresenter
//

CRM7AllocatorPresenter::CRM7AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: CDX7AllocatorPresenter(hWnd, hr)
{
    if(FAILED(hr))
		return;
}

CRM7AllocatorPresenter::~CRM7AllocatorPresenter()
{
}

STDMETHODIMP CRM7AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI2(IRMAVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRM7AllocatorPresenter::AllocateSurfaces(CSize size)
{
    CAutoLock cAutoLock(this);

	DeleteSurfaces();

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(!m_pBackBuffer || FAILED(m_pBackBuffer->GetSurfaceDesc(&ddsd)))
		return E_FAIL;

	AllocDX7Surface(m_pDD, size, BI_RGB, ddsd.ddpfPixelFormat.dwRGBBitCount, &m_pVideoSurface);
	AllocDX7Surface(m_pDD, size, '2YUY', 16, &m_pVideoSurfaceYUY2);
	if(FAILED(m_pVideoSurface->Blt(NULL, m_pVideoSurfaceYUY2, NULL, DDBLT_WAIT, NULL)))
		m_pVideoSurfaceYUY2 = NULL;

	DDBLTFX fx;
	INITDDSTRUCT(fx);
	fx.dwFillColor = 0;
	m_pVideoSurface->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
	if(m_pVideoSurfaceYUY2)
	{
		fx.dwFillColor = 0x80108010;
		m_pVideoSurfaceYUY2->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);
	}

	m_NativeVideoSize = m_AspectRatio = size;

	return S_OK;
}

void CRM7AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pVideoSurfaceYUY2 = NULL;

	__super::DeleteSurfaces();
}

bool CRM7AllocatorPresenter::OnDeviceLost()
{
	if(!__super::OnDeviceLost())
		return(false);

	if(FAILED(AllocateSurfaces(m_NativeVideoSize)))
		return(false);

	return(true);
}

// IRMAVideoSurface

STDMETHODIMP CRM7AllocatorPresenter::Blt(UCHAR* /*IN*/ pImageData, RMABitmapInfoHeader* /*IN*/ pBitmapInfo, REF(PNxRect) /*IN*/ inDestRect, REF(PNxRect) /*IN*/ inSrcRect)
{
	CRect src((RECT*)&inSrcRect), dst((RECT*)&inDestRect), src2(CPoint(0,0), src.Size());

	if(!m_pVideoSurface)
		return E_FAIL;

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(FAILED(m_pVideoSurface->GetSurfaceDesc(&ddsd)))
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
		m_pVideoSurface->Blt((RECT*)&dst, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurface->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this color format or source/destination rectangle is not supported"));

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x202020);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurface->ReleaseDC(hDC);
			
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

			m_pVideoSurface->Blt(dst, m_pVideoSurfaceYUY2, src2, DDBLT_WAIT, NULL);

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

			m_pVideoSurface->Blt(dst, m_pVideoSurfaceYUY2, src2, DDBLT_WAIT, NULL);

			Paint(true);
		}
	}
	else if((fccin == 0 || fccin == 3) && (bppin == 16 || bppin == 24 || bppin == 32))
	{
		INITDDSTRUCT(ddsd);
		if(SUCCEEDED(m_pVideoSurface->Lock(dst, &ddsd, DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, NULL)))
		{
			int pitchIn = pBitmapInfo->biWidth*bppin>>3;
			int pitchOut = ddsd.lPitch;
			int pitchMin = min(pitchIn, pitchOut);

			int w = min(pitchIn*8/bppin, pitchOut*8/bppout);

			BYTE* pDataIn = (BYTE*)pImageData + src.top*pitchIn + ((src.left*bppin)>>3);
			BYTE* pDataOut = (BYTE*)ddsd.lpSurface;

			if(fccin == 0)
			{
				// TODO: test this!!!
//				pDataIn = (BYTE*)pImageData + (pBitmapInfo->biHeight-1)*pitchIn - src.top*pitchIn + ((src.left*bppin)>>3);
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
						*pOut++ = (WORD)(((*pIn>>8)&0xf800)|((*pIn>>5)&0x07e0)|((*pIn>>3)&0x001f));
						pIn++;
					}
				}
				else if(bppin == 24 && bppout == 16)
				{
					BYTE* pIn = pDataIn;
					WORD* pOut = (WORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = (WORD)(((*((DWORD*)pIn)>>8)&0xf800)|((*((DWORD*)pIn)>>5)&0x07e0)|((*((DWORD*)pIn)>>3)&0x001f));
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

			m_pVideoSurface->Unlock(dst);
		}
		
		Paint(true);
	}

	return PNR_OK;
}

STDMETHODIMP CRM7AllocatorPresenter::BeginOptimizedBlt(RMABitmapInfoHeader* /*IN*/ pBitmapInfo)
{
	AllocateSurfaces(CSize(pBitmapInfo->biWidth, abs(pBitmapInfo->biHeight)));

	return PNR_NOTIMPL;
}

STDMETHODIMP CRM7AllocatorPresenter::OptimizedBlt(UCHAR* /*IN*/ pImageBits, REF(PNxRect) /*IN*/ rDestRect, REF(PNxRect) /*IN*/ rSrcRect)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM7AllocatorPresenter::EndOptimizedBlt()
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM7AllocatorPresenter::GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM7AllocatorPresenter::GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) /*OUT*/ ulType)
{
//	ulType = RMA_YUY2;
	ulType = RMA_I420;
	return PNR_OK;
}

//
// CQT7AllocatorPresenter
//

CQT7AllocatorPresenter::CQT7AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: CDX7AllocatorPresenter(hWnd, hr)
{
    if(FAILED(hr))
		return;
}

CQT7AllocatorPresenter::~CQT7AllocatorPresenter()
{
}

STDMETHODIMP CQT7AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IQTVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CQT7AllocatorPresenter::AllocateSurfaces(CSize size)
{
    CAutoLock cAutoLock(this);

	DeleteSurfaces();

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(!m_pBackBuffer || FAILED(m_pBackBuffer->GetSurfaceDesc(&ddsd)))
		return E_FAIL;

	AllocDX7Surface(m_pDD, size, BI_RGB, ddsd.ddpfPixelFormat.dwRGBBitCount, &m_pVideoSurface);

	DDBLTFX fx;
	INITDDSTRUCT(fx);
	fx.dwFillColor = 0;
	m_pVideoSurface->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);

	m_NativeVideoSize = m_AspectRatio = size;

	return S_OK;
}

bool CQT7AllocatorPresenter::OnDeviceLost()
{
	if(!__super::OnDeviceLost())
		return(false);

	if(FAILED(AllocateSurfaces(m_NativeVideoSize)))
		return(false);

	return(true);
}

// IQTVideoSurface

STDMETHODIMP CQT7AllocatorPresenter::BeginBlt(const BITMAP& bm)
{
	return AllocateSurfaces(CSize(bm.bmWidth, abs(bm.bmHeight)));
}

STDMETHODIMP CQT7AllocatorPresenter::DoBlt(const BITMAP& bm)
{
	if(!m_pVideoSurface)
		return E_FAIL;

	DDSURFACEDESC2 ddsd;
	INITDDSTRUCT(ddsd);
	if(FAILED(m_pVideoSurface->GetSurfaceDesc(&ddsd)))
		return E_FAIL;

	int bppin = bm.bmBitsPixel;
	int bppout = ddsd.ddpfPixelFormat.dwRGBBitCount;

	int w = bm.bmWidth;
	int h = abs(bm.bmHeight);

	if((bppin == 16 || bppin == 24 || bppin == 32) && w == ddsd.dwWidth && h == ddsd.dwHeight)
	{
		INITDDSTRUCT(ddsd);
		if(SUCCEEDED(m_pVideoSurface->Lock(NULL, &ddsd, DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR|DDLOCK_WRITEONLY, NULL)))
		{
			int pitchIn = bm.bmWidthBytes;
			int pitchOut = ddsd.lPitch;
			int pitchMin = min(pitchIn, pitchOut);

			BYTE* pDataIn = (BYTE*)bm.bmBits;
			BYTE* pDataOut = (BYTE*)ddsd.lpSurface;

			if(bm.bmHeight < 0)
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
						*pOut++ = (WORD)(((*pIn>>8)&0xf800)|((*pIn>>5)&0x07e0)|((*pIn>>3)&0x001f));
						pIn++;
					}
				}
				else if(bppin == 24 && bppout == 16)
				{
					BYTE* pIn = pDataIn;
					WORD* pOut = (WORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = (WORD)(((*((DWORD*)pIn)>>8)&0xf800)|((*((DWORD*)pIn)>>5)&0x07e0)|((*((DWORD*)pIn)>>3)&0x001f));
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

			m_pVideoSurface->Unlock(NULL);
		}
		
		Paint(true);
	}
	else
	{
		DDBLTFX fx;
		INITDDSTRUCT(fx);
		fx.dwFillColor = (ddsd.ddpfPixelFormat.dwFlags&DDPF_RGB) ? 0 : 0x80008000;
		m_pVideoSurface->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &fx);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurface->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this color format is not supported"));

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x202020);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurface->ReleaseDC(hDC);
			
			Paint(true);
		}
	}

	return S_OK;
}
