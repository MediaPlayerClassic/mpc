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

#include <Videoacc.h>

#include <initguid.h>
#include "DX9AllocatorPresenter.h"
#include <d3d9.h>
#include <Vmr9.h>
#include "..\..\SubPic\DX9SubPic.h"
#include "IQTVideoSurface.h"

//#include <d3dx9.h>

namespace DSObjects
{

class CDX9AllocatorPresenter
	: public ISubPicAllocatorPresenterImpl
{
protected:
	CSize m_ScreenSize;

	CComPtr<IDirect3D9> m_pD3D;
    CComPtr<IDirect3DDevice9> m_pD3DDev;
	CComPtr<IDirect3DTexture9> m_pVideoTexture;
	CComPtr<IDirect3DSurface9> m_pVideoSurface;

    virtual HRESULT CreateDevice();
	virtual HRESULT AllocSurfaces();
	virtual void DeleteSurfaces();

public:
	CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr);

	// ISubPicAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
	STDMETHODIMP_(bool) Paint(bool fAll);
};

class CVMR9AllocatorPresenter
	: public CDX9AllocatorPresenter
	, public IVMRSurfaceAllocator9
	, public IVMRImagePresenter9
	, public IVMRWindowlessControl9
{
protected:
	CComPtr<IVMRSurfaceAllocatorNotify9> m_pIVMRSurfAllocNotify;
	CInterfaceArray<IDirect3DSurface9> m_pSurfaces;

	HRESULT CreateDevice();
	void DeleteSurfaces();

public:
	CVMR9AllocatorPresenter(HWND hWnd, HRESULT& hr);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

    // IVMRSurfaceAllocator9
    STDMETHODIMP InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo* lpAllocInfo, DWORD* lpNumBuffers);
    STDMETHODIMP TerminateDevice(DWORD_PTR dwID);
    STDMETHODIMP GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9** lplpSurface);
    STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify);

    // IVMRImagePresenter9
    STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo* lpPresInfo);

	// IVMRWindowlessControl9
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
};

class CQT9AllocatorPresenter
	: public CDX9AllocatorPresenter
	, public IQTVideoSurface
{
	CComPtr<IDirect3DSurface9> m_pVideoSurfaceOff;

protected:
	 HRESULT AllocSurfaces();
	 void DeleteSurfaces();

public:
	CQT9AllocatorPresenter(HWND hWnd, HRESULT& hr);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IQTVideoSurface
	STDMETHODIMP BeginBlt(const BITMAP& bm);
	STDMETHODIMP DoBlt(const BITMAP& bm);
};

}
using namespace DSObjects;

//

HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP)
{
	CheckPointer(ppAP, E_POINTER);

	*ppAP = NULL;

	HRESULT hr;
	if(clsid == CLSID_VMR9AllocatorPresenter && !(*ppAP = new CVMR9AllocatorPresenter(hWnd, hr))
//	|| clsid == CLSID_RM9AllocatorPresenter && !(*ppAP = new CRM9AllocatorPresenter(hWnd, hr))
	|| clsid == CLSID_QT9AllocatorPresenter && !(*ppAP = new CQT9AllocatorPresenter(hWnd, hr)))
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

static HRESULT TextureBlt(CComPtr<IDirect3DTexture9> pTexture, CRect dst, CRect src)
{
	if(!pTexture)
		return E_POINTER;

	CComPtr<IDirect3DDevice9> pD3DDev;
	if(FAILED(pTexture->GetDevice(&pD3DDev)) || !pD3DDev)
		return E_FAIL;

	HRESULT hr;

    do
	{
		D3DSURFACE_DESC d3dsd;
		ZeroMemory(&d3dsd, sizeof(d3dsd));
		if(FAILED(pTexture->GetLevelDesc(0, &d3dsd)))
			break;

        float w = (float)d3dsd.Width;
        float h = (float)d3dsd.Height;

		struct
		{
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] =
		{
			{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h},
			{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
			{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h},
			{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
		};

		for(int i = 0; i < sizeof(pVertices)/sizeof(pVertices[0]); i++)
		{
			pVertices[i].x -= 0.5;
			pVertices[i].y -= 0.5;
		}

        hr = pD3DDev->SetTexture(0, pTexture);

        hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
    	hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 

		hr = pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        hr = pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        hr = pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		//

		if(FAILED(hr = pD3DDev->BeginScene()))
			break;

        hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		hr = pD3DDev->EndScene();

        //

		pD3DDev->SetTexture(0, NULL);

		return S_OK;
    }
	while(0);

    return E_FAIL;
}

// CDX9AllocatorPresenter

CDX9AllocatorPresenter::CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: ISubPicAllocatorPresenterImpl(hWnd)
	, m_ScreenSize(0, 0)
{
    if(!IsWindow(m_hWnd))
    {
        hr = E_INVALIDARG;
        return;
    }

	m_pD3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));
	if(!m_pD3D)
	{
		hr = E_FAIL;
		return;
	}

	GetWindowRect(m_hWnd, &m_WindowRect);

	hr = CreateDevice();
}

HRESULT CDX9AllocatorPresenter::CreateDevice()
{
    m_pD3DDev = NULL;

	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof(d3ddm));
	if(FAILED(m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
		return E_UNEXPECTED;

	m_ScreenSize.SetSize(d3ddm.Width, d3ddm.Height);

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.Windowed = TRUE;
    pp.hDeviceWindow = m_hWnd;
    pp.SwapEffect = D3DSWAPEFFECT_COPY;
	pp.Flags = D3DPRESENTFLAG_VIDEO;
	pp.BackBufferWidth = d3ddm.Width;
	pp.BackBufferHeight = d3ddm.Height;

	HRESULT hr = m_pD3D->CreateDevice(
						D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
						&pp, &m_pD3DDev);
	if(FAILED(hr))
		return hr;

	//

	CComPtr<ISubPicProvider> pSubPicProvider;
	if(m_pSubPicQueue) m_pSubPicQueue->GetSubPicProvider(&pSubPicProvider);

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

	if(m_pAllocator)
	{
		m_pAllocator->ChangeDevice(m_pD3DDev);
	}
	else
	{
		m_pAllocator = new CDX9SubPicAllocator(m_pD3DDev, size);
		if(!m_pAllocator)
			return E_FAIL;
	}

	hr = S_OK;
	m_pSubPicQueue = AfxGetAppSettings().nSPCSize > 0 
		? (ISubPicQueue*)new CSubPicQueue(AfxGetAppSettings().nSPCSize, m_pAllocator, &hr)
		: (ISubPicQueue*)new CSubPicQueueNoThread(m_pAllocator, &hr);
	if(!m_pSubPicQueue || FAILED(hr))
		return E_FAIL;

	if(pSubPicProvider) m_pSubPicQueue->SetSubPicProvider(pSubPicProvider);

	return S_OK;
} 

HRESULT CDX9AllocatorPresenter::AllocSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pVideoTexture = NULL;
	m_pVideoSurface = NULL;

	HRESULT hr;

	if(AfxGetAppSettings().fVMRTexture)
	{
		if(FAILED(hr = m_pD3DDev->CreateTexture(
			m_NativeVideoSize.cx, m_NativeVideoSize.cy, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8/*D3DFMT_A8R8G8B8*/, 
			D3DPOOL_DEFAULT, &m_pVideoTexture, NULL)))
			return hr;

		if(FAILED(hr = m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface)))
			return hr;

		if(!AfxGetAppSettings().fVMR3D) m_pVideoTexture = NULL;
	}
	else
	{
		if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
			m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_A8R8G8B8, 
			D3DPOOL_DEFAULT, &m_pVideoSurface, NULL)))
			return hr;
	}

	hr = m_pD3DDev->ColorFill(m_pVideoSurface, NULL, 0);

	return S_OK;
}

void CDX9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pVideoTexture = NULL;
	m_pVideoSurface = NULL;
}

// ISubPicAllocatorPresenter

STDMETHODIMP CDX9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::Paint(bool fAll)
{
	CAutoLock cAutoLock(this);

	if(m_WindowRect.right <= m_WindowRect.left || m_WindowRect.bottom <= m_WindowRect.top
	|| m_NativeVideoSize.cx <= 0 || m_NativeVideoSize.cy <= 0
	|| !m_pVideoSurface)
		return(false);

	HRESULT hr;

	CRect rSrcVid(CPoint(0, 0), m_NativeVideoSize);
	CRect rDstVid(m_VideoRect);

	CRect rSrcPri(CPoint(0, 0), m_WindowRect.Size());
	CRect rDstPri(m_WindowRect);

	if(fAll)
	{
		// clear the backbuffer

		hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

		// paint the video on the backbuffer

		if(!rDstVid.IsRectEmpty())
		{
			if(m_pVideoTexture)
			{
				hr = TextureBlt(m_pVideoTexture, rDstVid, rSrcVid);
			}
			else
			{
				CComPtr<IDirect3DSurface9> pBackBuffer;
				if(SUCCEEDED(hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
					hr = m_pD3DDev->StretchRect(m_pVideoSurface, rSrcVid, pBackBuffer, rDstVid, D3DTEXF_LINEAR);
			}
		}

		// paint the text on the backbuffer

		AlphaBltSubPic(rSrcPri.Size());
	}

	hr = m_pD3DDev->Present(rSrcPri, rDstPri, NULL, NULL);

	if(hr == D3DERR_DEVICELOST)
	{
		if(m_pD3DDev->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) 
		{
			DeleteSurfaces();
			if(FAILED(hr = CreateDevice()) || FAILED(hr = AllocSurfaces()))
				return(false);
		}

		hr = S_OK;
	}

	return(true);
}

//
// CVMR9AllocatorPresenter
//

#define MY_USER_ID 0x6ABE51

CVMR9AllocatorPresenter::CVMR9AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: CDX9AllocatorPresenter(hWnd, hr)
{
}

STDMETHODIMP CVMR9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IVMRSurfaceAllocator9)
		QI(IVMRImagePresenter9)
		QI(IVMRWindowlessControl9)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVMR9AllocatorPresenter::CreateDevice()
{
	HRESULT hr = __super::CreateDevice();
	if(FAILED(hr)) return hr;

	if(m_pIVMRSurfAllocNotify)
	{
		HMONITOR hMonitor = m_pD3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
		if(FAILED(hr = m_pIVMRSurfAllocNotify->ChangeD3DDevice(m_pD3DDev, hMonitor)))
			return(false);
	}

	return hr;
}

void CVMR9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pSurfaces.RemoveAll();

	return __super::DeleteSurfaces();
}

// ISubPicAllocatorPresenter

class COuterVMR9
	: public CUnknown
	, public IVideoWindow
	, public IBasicVideo2
	, public IVMRWindowlessControl
{
	CComPtr<IUnknown> m_pVMR;

public:

	COuterVMR9(const TCHAR* pName, LPUNKNOWN pUnk) : CUnknown(pName, pUnk)
	{
		m_pVMR.CoCreateInstance(CLSID_VideoMixingRenderer9, GetOwner());
	}

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		HRESULT hr = m_pVMR ? m_pVMR->QueryInterface(riid, ppv) : E_NOINTERFACE;

		if(m_pVMR && FAILED(hr))
		{
			if(riid == __uuidof(IVideoWindow))
				return GetInterface((IVideoWindow*)this, ppv);
			if(riid == __uuidof(IBasicVideo))
				return GetInterface((IBasicVideo*)this, ppv);
			if(riid == __uuidof(IBasicVideo2))
				return GetInterface((IBasicVideo2*)this, ppv);
/*			if(riid == __uuidof(IVMRWindowlessControl))
				return GetInterface((IVMRWindowlessControl*)this, ppv);
*/		}

		return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
	}

	// IVMRWindowlessControl

	STDMETHODIMP GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			return pWC9->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
	STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;}
    STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			return pWC9->GetVideoPosition(lpSRCRect, lpDSTRect);
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			*lpAspectRatioMode = VMR_ARMODE_NONE;
			return S_OK;
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
	STDMETHODIMP SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
	STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
	STDMETHODIMP DisplayModeChanged() {return E_NOTIMPL;}
	STDMETHODIMP GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
	STDMETHODIMP SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}
	STDMETHODIMP GetBorderColor(COLORREF* lpClr) {return E_NOTIMPL;}
	STDMETHODIMP SetColorKey(COLORREF Clr) {return E_NOTIMPL;}
	STDMETHODIMP GetColorKey(COLORREF* lpClr) {return E_NOTIMPL;}

	// IVideoWindow
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {return E_NOTIMPL;}
    STDMETHODIMP put_Caption(BSTR strCaption) {return E_NOTIMPL;}
    STDMETHODIMP get_Caption(BSTR* strCaption) {return E_NOTIMPL;}
	STDMETHODIMP put_WindowStyle(long WindowStyle) {return E_NOTIMPL;}
	STDMETHODIMP get_WindowStyle(long* WindowStyle) {return E_NOTIMPL;}
	STDMETHODIMP put_WindowStyleEx(long WindowStyleEx) {return E_NOTIMPL;}
	STDMETHODIMP get_WindowStyleEx(long* WindowStyleEx) {return E_NOTIMPL;}
	STDMETHODIMP put_AutoShow(long AutoShow) {return E_NOTIMPL;}
	STDMETHODIMP get_AutoShow(long* AutoShow) {return E_NOTIMPL;}
	STDMETHODIMP put_WindowState(long WindowState) {return E_NOTIMPL;}
	STDMETHODIMP get_WindowState(long* WindowState) {return E_NOTIMPL;}
	STDMETHODIMP put_BackgroundPalette(long BackgroundPalette) {return E_NOTIMPL;}
	STDMETHODIMP get_BackgroundPalette(long* pBackgroundPalette) {return E_NOTIMPL;}
	STDMETHODIMP put_Visible(long Visible) {return E_NOTIMPL;}
	STDMETHODIMP get_Visible(long* pVisible) {return E_NOTIMPL;}
	STDMETHODIMP put_Left(long Left) {return E_NOTIMPL;}
	STDMETHODIMP get_Left(long* pLeft) {return E_NOTIMPL;}
	STDMETHODIMP put_Width(long Width) {return E_NOTIMPL;}
	STDMETHODIMP get_Width(long* pWidth)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pWidth = d.Width();
			return hr;
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP put_Top(long Top) {return E_NOTIMPL;}
	STDMETHODIMP get_Top(long* pTop) {return E_NOTIMPL;}
	STDMETHODIMP put_Height(long Height) {return E_NOTIMPL;}
	STDMETHODIMP get_Height(long* pHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pHeight = d.Height();
			return hr;
		}

		return E_NOTIMPL;
	}
	STDMETHODIMP put_Owner(OAHWND Owner) {return E_NOTIMPL;}
	STDMETHODIMP get_Owner(OAHWND* Owner) {return E_NOTIMPL;}
	STDMETHODIMP put_MessageDrain(OAHWND Drain) {return E_NOTIMPL;}
	STDMETHODIMP get_MessageDrain(OAHWND* Drain) {return E_NOTIMPL;}
	STDMETHODIMP get_BorderColor(long* Color) {return E_NOTIMPL;}
	STDMETHODIMP put_BorderColor(long Color) {return E_NOTIMPL;}
	STDMETHODIMP get_FullScreenMode(long* FullScreenMode) {return E_NOTIMPL;}
	STDMETHODIMP put_FullScreenMode(long FullScreenMode) {return E_NOTIMPL;}
    STDMETHODIMP SetWindowForeground(long Focus) {return E_NOTIMPL;}
    STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam) {return E_NOTIMPL;}
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
	STDMETHODIMP GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetMinIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetMaxIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP GetRestorePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
	STDMETHODIMP HideCursor(long HideCursor) {return E_NOTIMPL;}
	STDMETHODIMP IsCursorHidden(long* CursorHidden) {return E_NOTIMPL;}

	// IBasicVideo2
    STDMETHODIMP get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame) {return E_NOTIMPL;}
    STDMETHODIMP get_BitRate(long* pBitRate) {return E_NOTIMPL;}
    STDMETHODIMP get_BitErrorRate(long* pBitErrorRate) {return E_NOTIMPL;}
    STDMETHODIMP get_VideoWidth(long* pVideoWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_VideoHeight(long* pVideoHeight) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceLeft(long SourceLeft) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceLeft(long* pSourceLeft) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceWidth(long SourceWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceWidth(long* pSourceWidth) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceTop(long SourceTop) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceTop(long* pSourceTop) {return E_NOTIMPL;}
    STDMETHODIMP put_SourceHeight(long SourceHeight) {return E_NOTIMPL;}
    STDMETHODIMP get_SourceHeight(long* pSourceHeight) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationLeft(long DestinationLeft) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationLeft(long* pDestinationLeft) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationWidth(long DestinationWidth) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationWidth(long* pDestinationWidth) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationTop(long DestinationTop) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationTop(long* pDestinationTop) {return E_NOTIMPL;}
    STDMETHODIMP put_DestinationHeight(long DestinationHeight) {return E_NOTIMPL;}
    STDMETHODIMP get_DestinationHeight(long* pDestinationHeight) {return E_NOTIMPL;}
    STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
    STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
	{
		// DVD Nav. bug workaround fix
		{
			*pLeft = *pTop = 0;
			return GetVideoSize(pWidth, pHeight);
		}
/*
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pLeft = s.left;
			*pTop = s.top;
			*pWidth = s.Width();
			*pHeight = s.Height();
			return hr;
		}
*/
		return E_NOTIMPL;
	}
    STDMETHODIMP SetDefaultSourcePosition() {return E_NOTIMPL;}
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
    STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			CRect s, d;
			HRESULT hr = pWC9->GetVideoPosition(&s, &d);
			*pLeft = d.left;
			*pTop = d.top;
			*pWidth = d.Width();
			*pHeight = d.Height();
			return hr;
		}

		return E_NOTIMPL;
	}
    STDMETHODIMP SetDefaultDestinationPosition() {return E_NOTIMPL;}
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			LONG aw, ah;
//			return pWC9->GetNativeVideoSize(pWidth, pHeight, &aw, &ah);
			// DVD Nav. bug workaround fix
			HRESULT hr = pWC9->GetNativeVideoSize(pWidth, pHeight, &aw, &ah);
			*pWidth = *pHeight * aw / ah;
			return hr;
		}

		return E_NOTIMPL;
	}
    STDMETHODIMP GetVideoPaletteEntries(long StartIndex, long Entries, long* pRetrieved, long* pPalette) {return E_NOTIMPL;}
    STDMETHODIMP GetCurrentImage(long* pBufferSize, long* pDIBImage) {return E_NOTIMPL;}
    STDMETHODIMP IsUsingDefaultSource() {return E_NOTIMPL;}
    STDMETHODIMP IsUsingDefaultDestination() {return E_NOTIMPL;}

	STDMETHODIMP GetPreferredAspectRatio(long* plAspectX, long* plAspectY)
	{
		if(CComQIPtr<IVMRWindowlessControl9> pWC9 = m_pVMR)
		{
			LONG w, h;
			return pWC9->GetNativeVideoSize(&w, &h, plAspectX, plAspectY);
		}

		return E_NOTIMPL;
	}
};

#include "MacrovisionKicker.h"
STDMETHODIMP CVMR9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

	*ppRenderer = NULL;

	HRESULT hr;

	do
	{
		CMacrovisionKicker* pMK = new CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;
		pMK->SetInner((IUnknown*)(INonDelegatingUnknown*)new COuterVMR9(NAME("COuterVMR9"), pUnk));
		CComQIPtr<IBaseFilter> pBF = pUnk;
/*
		CComQIPtr<IBaseFilter> pBF = (IUnknown*)(INonDelegatingUnknown*)new COuterVMR9(NAME("COuterVMR9"), NULL);
		if(!pBF) pBF.CoCreateInstance(CLSID_VideoMixingRenderer9);
*/
		CComQIPtr<IVMRFilterConfig9> pConfig = pBF;
		if(!pConfig)
			break;

		if(FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless)))
			break;
//pConfig->SetNumberOfStreams(1);

		CComQIPtr<IVMRSurfaceAllocatorNotify9> pSAN = pBF;
		if(!pSAN)
			break;

		if(FAILED(hr = pSAN->AdviseSurfaceAllocator(MY_USER_ID, static_cast<IVMRSurfaceAllocator9*>(this)))
		|| FAILED(hr = AdviseNotify(pSAN)))
			break;

		*ppRenderer = (IUnknown*)pBF.Detach();

		return S_OK;
	}
	while(0);

    return E_FAIL;
}

// IVMRSurfaceAllocator9

STDMETHODIMP CVMR9AllocatorPresenter::InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo* lpAllocInfo, DWORD* lpNumBuffers)
{
	if(!lpAllocInfo || !lpNumBuffers)
		return E_POINTER;

	if(!m_pIVMRSurfAllocNotify)
		return E_FAIL;

	DeleteSurfaces();
	m_pSurfaces.SetCount(*lpNumBuffers);

    HRESULT hr;

	hr = m_pIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_pSurfaces[0]);
	if(FAILED(hr))
		return hr;

	m_NativeVideoSize = CSize(lpAllocInfo->dwWidth, abs((int)lpAllocInfo->dwHeight));
	m_AspectRatio = m_NativeVideoSize;
	int arx = lpAllocInfo->szAspectRatio.cx, ary = lpAllocInfo->szAspectRatio.cy;
	if(arx > 0 && ary > 0) m_AspectRatio.SetSize(arx, ary);

	if(FAILED(hr = AllocSurfaces()))
		return hr;

	// test if the colorspace is acceptable
	if(FAILED(hr = m_pD3DDev->StretchRect(m_pSurfaces[0], NULL, m_pVideoSurface, NULL, D3DTEXF_NONE)))
	{
		DeleteSurfaces();
		return E_FAIL;
	}

	hr = m_pD3DDev->ColorFill(m_pVideoSurface, NULL, 0);

	return hr;
}

STDMETHODIMP CVMR9AllocatorPresenter::TerminateDevice(DWORD_PTR dwUserID)
{
    DeleteSurfaces();
    return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9** lplpSurface)
{
    if(!lplpSurface)
		return E_POINTER;

	if(SurfaceIndex >= m_pSurfaces.GetCount()) 
        return E_FAIL;

    CAutoLock cAutoLock(this);

	(*lplpSurface = m_pSurfaces[SurfaceIndex])->AddRef();

	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify)
{
    CAutoLock cAutoLock(this);
	
	m_pIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;

	HRESULT hr;
    HMONITOR hMonitor = m_pD3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
    if(FAILED(hr = m_pIVMRSurfAllocNotify->SetD3DDevice(m_pD3DDev, hMonitor)))
		return hr;

    return S_OK;
}

// IVMRImagePresenter9

STDMETHODIMP CVMR9AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
    CAutoLock cAutoLock(this);

    ASSERT(m_pD3DDev);

	return m_pD3DDev ? S_OK : E_FAIL;
}

STDMETHODIMP CVMR9AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
	return S_OK;
}

STDMETHODIMP CVMR9AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo* lpPresInfo)
{
    HRESULT hr;

	{
		if(!m_pIVMRSurfAllocNotify)
			return E_FAIL;

		D3DDEVICE_CREATION_PARAMETERS Parameters;
		if(FAILED(m_pD3DDev->GetCreationParameters(&Parameters)))
		{
			ASSERT(0);
			return E_FAIL;
		}

		HMONITOR hCurMonitor = m_pD3D->GetAdapterMonitor(Parameters.AdapterOrdinal);
		HMONITOR hMonitor = m_pD3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);

		if(hMonitor != hCurMonitor)
		{
			ASSERT(0);
		}
	}



/*
    // if we are in the middle of the display change
    if(NeedToHandleDisplayChange())
    {
        // NOTE: this piece of code is left as a user exercise.  
        // The D3DDevice here needs to be switched
        // to the device that is using another adapter
    }
*/
	{
		if(!lpPresInfo || !lpPresInfo->lpSurf)
			return E_POINTER;

		CAutoLock cAutoLock(this);

		hr = m_pD3DDev->StretchRect(lpPresInfo->lpSurf, NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);

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

    return hr;
}

// IVMRWindowlessControl9
//
// It is only implemented (partially) for the dvd navigator's 
// menu handling, which needs to know a few things about the 
// location of our window.

STDMETHODIMP CVMR9AllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
	if(lpWidth) *lpWidth = m_NativeVideoSize.cx;
	if(lpHeight) *lpHeight = m_NativeVideoSize.cy;
	if(lpARWidth) *lpARWidth = m_AspectRatio.cx;
	if(lpARHeight) *lpARHeight = m_AspectRatio.cy;
	return S_OK;
}
STDMETHODIMP CVMR9AllocatorPresenter::GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) {return E_NOTIMPL;} // we have our own method for this
STDMETHODIMP CVMR9AllocatorPresenter::GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
{
	CopyRect(lpSRCRect, CRect(CPoint(0, 0), m_NativeVideoSize));
	CopyRect(lpDSTRect, &m_VideoRect);
	return S_OK;
}
STDMETHODIMP CVMR9AllocatorPresenter::GetAspectRatioMode(DWORD* lpAspectRatioMode)
{
	if(lpAspectRatioMode) *lpAspectRatioMode = AM_ARMODE_STRETCHED;
	return S_OK;
}
STDMETHODIMP CVMR9AllocatorPresenter::SetAspectRatioMode(DWORD AspectRatioMode) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::SetVideoClippingWindow(HWND hwnd) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::RepaintVideo(HWND hwnd, HDC hdc) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::DisplayModeChanged() {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::GetCurrentImage(BYTE** lpDib) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::SetBorderColor(COLORREF Clr) {return E_NOTIMPL;}
STDMETHODIMP CVMR9AllocatorPresenter::GetBorderColor(COLORREF* lpClr)
{
	if(lpClr) *lpClr = 0;
	return S_OK;
}

//
// CQT9AllocatorPresenter
//

CQT9AllocatorPresenter::CQT9AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: CDX9AllocatorPresenter(hWnd, hr)
{
}

STDMETHODIMP CQT9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IQTVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CQT9AllocatorPresenter::AllocSurfaces()
{
	HRESULT hr;

	m_pVideoSurfaceOff = NULL;

	if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
		m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_X8R8G8B8, 
		D3DPOOL_DEFAULT, &m_pVideoSurfaceOff, NULL)))
		return hr;

	return __super::AllocSurfaces();
}

void CQT9AllocatorPresenter::DeleteSurfaces()
{
	m_pVideoSurfaceOff = NULL;

	__super::DeleteSurfaces();
}

// IQTVideoSurface

STDMETHODIMP CQT9AllocatorPresenter::BeginBlt(const BITMAP& bm)
{
    CAutoLock cAutoLock(this);

	DeleteSurfaces();

	m_NativeVideoSize = m_AspectRatio = CSize(bm.bmWidth, abs(bm.bmHeight));

	HRESULT hr;
	if(FAILED(hr = AllocSurfaces()))
		return hr;

	return S_OK;
}

STDMETHODIMP CQT9AllocatorPresenter::DoBlt(const BITMAP& bm)
{
	if(!m_pVideoSurface || !m_pVideoSurfaceOff)
		return E_FAIL;

	bool fOk = false;

	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if(FAILED(m_pVideoSurfaceOff->GetDesc(&d3dsd)))
		return E_FAIL;

	int bppin = bm.bmBitsPixel;

	int w = bm.bmWidth;
	int h = abs(bm.bmHeight);

	if((bppin == 16 || bppin == 24 || bppin == 32) && w == d3dsd.Width && h == d3dsd.Height)
	{
		D3DLOCKED_RECT r;
		if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, NULL, 0)))
		{
			int pitchIn = bm.bmWidthBytes;
			int pitchOut = r.Pitch;
			int pitchMin = min(pitchIn, pitchOut);

			BYTE* pDataIn = (BYTE*)bm.bmBits;
			BYTE* pDataOut = (BYTE*)r.pBits;

			if(bm.bmHeight < 0)
			{
				pDataIn += (h-1)*pitchIn;
				pitchIn = -pitchIn;
			}

			for(int y = 0; y < h; y++, pDataIn += pitchIn, pDataOut += pitchOut)
			{
				if(bppin == 32)
				{
					memcpy(pDataOut, pDataIn, pitchMin);
				}
				else if(bppin == 24)
				{
					BYTE* pIn = pDataIn;
					DWORD* pOut = (DWORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = *((DWORD*)pIn)&0xffffff;
						pIn += 3;
					}
				}
				else if(bppin == 16)
				{
					WORD* pIn = (WORD*)pDataIn;
					DWORD* pOut = (DWORD*)pDataOut;
					for(int x = 0; x < w; x++)
					{
						*pOut++ = ((*pIn&0xf800)<<8)|((*pIn&0x07e0)<<5)|((*pIn&0x001f)<<3);
						pIn++;
					}
				}
			}

			m_pVideoSurfaceOff->UnlockRect();

			fOk = true;
		}
	}

	if(!fOk)
	{
		m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurfaceOff->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this color format is not supported"));

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x404040);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurfaceOff->ReleaseDC(hDC);
		}
	}

	m_pD3DDev->StretchRect(m_pVideoSurfaceOff, NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);

	Paint(true);

	return S_OK;
}
