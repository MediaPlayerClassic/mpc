/* 
 *	Copyright (C) 2003-2004 Gabest
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
#include <d3dx9.h>
#include <Vmr9.h>
#include "..\..\SubPic\DX9SubPic.h"
#include "..\..\..\include\RealMedia\pntypes.h"
#include "..\..\..\include\RealMedia\pnwintyp.h"
#include "..\..\..\include\RealMedia\pncom.h"
#include "..\..\..\include\RealMedia\rmavsurf.h"
#include "IQTVideoSurface.h"

bool IsVMR9InGraph(IFilterGraph* pFG)
{
	BeginEnumFilters(pFG, pEF, pBF)
		if(CComQIPtr<IVMRWindowlessControl9>(pBF)) return(true);
	EndEnumFilters
	return(false);
}

namespace DSObjects
{

class CDX9AllocatorPresenter
	: public ISubPicAllocatorPresenterImpl
{
protected:
	CSize m_ScreenSize;
	bool m_fVMRSyncFix;

	CComPtr<IDirect3D9> m_pD3D;
    CComPtr<IDirect3DDevice9> m_pD3DDev;
	CComPtr<IDirect3DTexture9> m_pVideoTexture;
	CComPtr<IDirect3DSurface9> m_pVideoSurface;
	CComPtr<IDirect3DPixelShader9> m_pPixelShader;
	D3DTEXTUREFILTERTYPE m_Filter;

	virtual HRESULT CreateDevice();
	virtual HRESULT AllocSurfaces();
	virtual void DeleteSurfaces();

public:
	CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr);

	// ISubPicAllocatorPresenter
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
	STDMETHODIMP_(bool) Paint(bool fAll);
	STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
	STDMETHODIMP SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget, LPSTR err, int errlen);
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

class CRM9AllocatorPresenter
	: public CDX9AllocatorPresenter
	, public IRMAVideoSurface
{
	CComPtr<IDirect3DSurface9> m_pVideoSurfaceOff;
	CComPtr<IDirect3DSurface9> m_pVideoSurfaceYUY2;

    RMABitmapInfoHeader m_bitmapInfo;
    RMABitmapInfoHeader m_lastBitmapInfo;

protected:
	HRESULT AllocSurfaces();
	void DeleteSurfaces();

public:
	CRM9AllocatorPresenter(HWND hWnd, HRESULT& hr);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IRMAVideoSurface
    STDMETHODIMP Blt(UCHAR*	pImageData, RMABitmapInfoHeader* pBitmapInfo, REF(PNxRect) inDestRect, REF(PNxRect) inSrcRect);
	STDMETHODIMP BeginOptimizedBlt(RMABitmapInfoHeader* pBitmapInfo);
	STDMETHODIMP OptimizedBlt(UCHAR* pImageBits, REF(PNxRect) rDestRect, REF(PNxRect) rSrcRect);
	STDMETHODIMP EndOptimizedBlt();
	STDMETHODIMP GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) ulType);
    STDMETHODIMP GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) ulType);
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
	|| clsid == CLSID_RM9AllocatorPresenter && !(*ppAP = new CRM9AllocatorPresenter(hWnd, hr))
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

static HRESULT TextureBlt(CComPtr<IDirect3DTexture9> pTexture, Vector dst[4], CRect src)
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
			{(float)dst[0].x, (float)dst[0].y, (float)dst[0].z, 1.0f/(float)dst[0].z, (float)src.left / w, (float)src.top / h},
			{(float)dst[1].x, (float)dst[1].y, (float)dst[1].z, 1.0f/(float)dst[1].z, (float)src.right / w, (float)src.top / h},
			{(float)dst[2].x, (float)dst[2].y, (float)dst[2].z, 1.0f/(float)dst[2].z, (float)src.left / w, (float)src.bottom / h},
			{(float)dst[3].x, (float)dst[3].y, (float)dst[3].z, 1.0f/(float)dst[3].z, (float)src.right / w, (float)src.bottom / h},
		};

		for(int i = 0; i < countof(pVertices); i++)
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
	if(!m_pD3D) m_pD3D.Attach(Direct3DCreate9(D3D9b_SDK_VERSION));

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

	if(m_fVMRSyncFix = AfxGetMyApp()->m_s.fVMRSyncFix)
		pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	HRESULT hr = m_pD3D->CreateDevice(
						D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, //D3DCREATE_MANAGED 
						&pp, &m_pD3DDev);
	if(FAILED(hr))
		return hr;

	//

	m_Filter = D3DTEXF_NONE;

	D3DCAPS9 caps;
    ZeroMemory(&caps, sizeof(caps));
	m_pD3DDev->GetDeviceCaps(&caps);
	if((caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MINFLINEAR)
	&& (caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MAGFLINEAR))
		m_Filter = D3DTEXF_LINEAR;

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

	AppSettings& s = AfxGetAppSettings();

	m_pVideoTexture = NULL;
	m_pVideoSurface = NULL;

	HRESULT hr;

	if(s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE2D || s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D)
	{
		if(FAILED(hr = m_pD3DDev->CreateTexture(
			m_NativeVideoSize.cx, m_NativeVideoSize.cy, 1, D3DUSAGE_RENDERTARGET, /*D3DFMT_X8R8G8B8*/D3DFMT_A8R8G8B8, 
			D3DPOOL_DEFAULT, &m_pVideoTexture, NULL)))
			return hr;

		if(FAILED(hr = m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface)))
			return hr;

		if(s.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE2D) 
			m_pVideoTexture = NULL;
	}
	else
	{
		if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
			m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_X8R8G8B8/*D3DFMT_A8R8G8B8*/, 
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

static bool ClipToSurface(IDirect3DSurface9* pSurface, CRect& s, CRect& d)   
{   
	D3DSURFACE_DESC d3dsd;   
	ZeroMemory(&d3dsd, sizeof(d3dsd));   
	if(FAILED(pSurface->GetDesc(&d3dsd)))   
		return(false);   

	int w = d3dsd.Width, h = d3dsd.Height;   
	int sw = s.Width(), sh = s.Height();   
	int dw = d.Width(), dh = d.Height();   

	if(d.left >= w || d.right < 0 || d.top >= h || d.bottom < 0   
	|| sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)   
	{   
		s.SetRectEmpty();   
		d.SetRectEmpty();   
		return(true);   
	}   

	if(d.right > w) {s.right -= (d.right-w)*sw/dw; d.right = w;}   
	if(d.bottom > h) {s.bottom -= (d.bottom-h)*sh/dh; d.bottom = h;}   
	if(d.left < 0) {s.left += (0-d.left)*sw/dw; d.left = 0;}   
	if(d.top < 0) {s.top += (0-d.top)*sh/dh; d.top = 0;}   

	return(true);   
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

	CComPtr<IDirect3DSurface9> pBackBuffer;
	m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

	if(fAll)
	{
		// clear the backbuffer

		hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

		// paint the video on the backbuffer

		if(!rDstVid.IsRectEmpty())
		{
			if(m_pVideoTexture)
			{
				if(m_pPixelShader)
				{
					static __int64 i = 0, j = clock();
					float fConstData[] = {(float)m_NativeVideoSize.cx, (float)m_NativeVideoSize.cy, (float)(i++), (float)(clock()-j)};
					hr = m_pD3DDev->SetPixelShaderConstantF(0, fConstData, countof(fConstData)/4);
				}

				Vector v[4];
				Transform(rDstVid, v);
				hr = TextureBlt(m_pVideoTexture, v, rSrcVid);
			}
			else
			{
				if(pBackBuffer)
				{
					ClipToSurface(pBackBuffer, rSrcVid, rDstVid); // grrr
					// IMPORTANT: rSrcVid has to be aligned on mod2 for yuy2->rgb conversion with StretchRect!!!
					rSrcVid.left &= ~1; rSrcVid.right &= ~1;
					rSrcVid.top &= ~1; rSrcVid.bottom &= ~1;
					hr = m_pD3DDev->StretchRect(m_pVideoSurface, rSrcVid, pBackBuffer, rDstVid, m_Filter);
				}
			}
		}

		// paint the text on the backbuffer

		AlphaBltSubPic(rSrcPri.Size());
	}

	if(m_fVMRSyncFix)
	{
		D3DLOCKED_RECT lr;
		if(SUCCEEDED(pBackBuffer->LockRect(&lr, NULL, 0)))
			pBackBuffer->UnlockRect();
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

STDMETHODIMP CDX9AllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	CheckPointer(size, E_POINTER);

	HRESULT hr;

	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	m_pVideoSurface->GetDesc(&desc);

	DWORD required = sizeof(BITMAPINFOHEADER) + (desc.Width * desc.Height * 32 >> 3);
	if(!lpDib) {*size = required; return S_OK;}
	if(*size < required) return E_OUTOFMEMORY;
	*size = required;

	CComPtr<IDirect3DSurface9> pSurface = m_pVideoSurface;
	D3DLOCKED_RECT r;
	if(FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY)))
	{
		pSurface = NULL;
		if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pSurface, NULL))
		|| FAILED(hr = m_pD3DDev->GetRenderTargetData(m_pVideoSurface, pSurface))
		|| FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY)))
			return hr;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)lpDib;
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = desc.Width;
	bih->biHeight = desc.Height;
	bih->biBitCount = 32;
	bih->biPlanes = 1;
	bih->biSizeImage = bih->biWidth * bih->biHeight * bih->biBitCount >> 3;

	BitBltFromRGBToRGB(
		bih->biWidth, bih->biHeight, 
		(BYTE*)(bih + 1), bih->biWidth*bih->biBitCount>>3, bih->biBitCount,
		(BYTE*)r.pBits + r.Pitch*(desc.Height-1), -(int)r.Pitch, 32);

	pSurface->UnlockRect();

	return S_OK;
}

STDMETHODIMP CDX9AllocatorPresenter::SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget, LPSTR err, int errlen)
{
	CAutoLock cAutoLock(this);

	if(err && errlen > 0) *err = 0;

	m_pPixelShader = NULL;
	m_pD3DDev->SetPixelShader(NULL);

	if(!pSrcData || !pTarget) 
		return E_INVALIDARG;

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
	HRESULT hr = D3DXCompileShader(pSrcData, strlen(pSrcData), NULL, NULL, "main", pTarget, 0, &pShader, &pErrorMsgs, NULL);
	if(FAILED(hr))
	{
		if(err && errlen > 0)
		{
			if(pErrorMsgs)
			{
				int len = pErrorMsgs->GetBufferSize();
				memcpy(err, pErrorMsgs->GetBufferPointer(), min(errlen, len));
			}
			else
			{
				char* msg = "Unknown compiler error";
				strncpy(err, msg, min(strlen(msg), errlen));
			}
		}

		return hr;
	}

	hr = m_pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &m_pPixelShader);
	if(FAILED(hr)) return hr;

	hr = m_pD3DDev->SetPixelShader(m_pPixelShader);
	if(FAILED(hr)) return hr;

	Paint(true);

	return S_OK;
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

	// StretchRect's yv12 -> rgb conversion looks horribly bright compared to the result of yuy2 -> rgb
	if(lpAllocInfo->Format == '21VY' || lpAllocInfo->Format == '024Y')
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
// CRM9AllocatorPresenter
//

CRM9AllocatorPresenter::CRM9AllocatorPresenter(HWND hWnd, HRESULT& hr) 
	: CDX9AllocatorPresenter(hWnd, hr)
{
}

STDMETHODIMP CRM9AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI2(IRMAVideoSurface)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRM9AllocatorPresenter::AllocSurfaces()
{
    CAutoLock cAutoLock(this);

	m_pVideoSurfaceOff = NULL;
	m_pVideoSurfaceYUY2 = NULL;

	HRESULT hr;

	if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
		m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_X8R8G8B8, 
		D3DPOOL_DEFAULT, &m_pVideoSurfaceOff, NULL)))
		return hr;

	m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0);

	if(FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
		m_NativeVideoSize.cx, m_NativeVideoSize.cy, D3DFMT_YUY2, 
		D3DPOOL_DEFAULT, &m_pVideoSurfaceYUY2, NULL)))
		m_pVideoSurfaceYUY2 = NULL;

	if(m_pVideoSurfaceYUY2)
	{
		m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0x80108010);
	}

	return __super::AllocSurfaces();
}

void CRM9AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);
	m_pVideoSurfaceOff = NULL;
	m_pVideoSurfaceYUY2 = NULL;
	__super::DeleteSurfaces();
}

// IRMAVideoSurface

STDMETHODIMP CRM9AllocatorPresenter::Blt(UCHAR* pImageData, RMABitmapInfoHeader* pBitmapInfo, REF(PNxRect) inDestRect, REF(PNxRect) inSrcRect)
{
	if(!m_pVideoSurface || !m_pVideoSurfaceOff)
		return E_FAIL;

	bool fRGB = false;
	bool fYUY2 = false;

	CRect src((RECT*)&inSrcRect), dst((RECT*)&inDestRect), src2(CPoint(0,0), src.Size());
	if(src.Width() > dst.Width() || src.Height() > dst.Height())
		return E_FAIL;

	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if(FAILED(m_pVideoSurfaceOff->GetDesc(&d3dsd)))
		return E_FAIL;

	int dbpp = 
		d3dsd.Format == D3DFMT_R8G8B8 || d3dsd.Format == D3DFMT_X8R8G8B8 || d3dsd.Format == D3DFMT_A8R8G8B8 ? 32 : 
		d3dsd.Format == D3DFMT_R5G6B5 ? 16 : 0;

	if(pBitmapInfo->biCompression == '024I')
	{
		DWORD pitch = pBitmapInfo->biWidth;
		DWORD size = pitch*abs(pBitmapInfo->biHeight);

		BYTE* y = pImageData					+ src.top*pitch + src.left;
		BYTE* u = pImageData + size				+ src.top*(pitch/2) + src.left/2;
		BYTE* v = pImageData + size + size/4	+ src.top*(pitch/2) + src.left/2;

		if(m_pVideoSurfaceYUY2)
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceYUY2->LockRect(&r, src2, 0)))
			{
				BitBltFromI420ToYUY2(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, y, u, v, pitch);
				m_pVideoSurfaceYUY2->UnlockRect();
				fYUY2 = true;
			}
		}
		else
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, src2, 0)))
			{
				BitBltFromI420ToRGB(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, dbpp, y, u, v, pitch);
				m_pVideoSurfaceOff->UnlockRect();
				fRGB = true;
			}
		}
	}
	else if(pBitmapInfo->biCompression == '2YUY')
	{
		DWORD w = pBitmapInfo->biWidth;
		DWORD h = abs(pBitmapInfo->biHeight);
		DWORD pitch = pBitmapInfo->biWidth*2;

		BYTE* yvyu = pImageData + src.top*pitch + src.left*2;

		if(m_pVideoSurfaceYUY2)
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceYUY2->LockRect(&r, src2, 0)))
			{
				BitBltFromYUY2ToYUY2(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, yvyu, pitch);
				m_pVideoSurfaceYUY2->UnlockRect();
				fYUY2 = true;
			}
		}
		else
		{
			D3DLOCKED_RECT r;
			if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, src2, 0)))
			{
				BitBltFromYUY2ToRGB(src.Width(), src.Height(), (BYTE*)r.pBits, r.Pitch, dbpp, yvyu, pitch);
				m_pVideoSurfaceOff->UnlockRect();
				fRGB = true;
			}
		}
	}
	else if(pBitmapInfo->biCompression == 0 || pBitmapInfo->biCompression == 3
		 || pBitmapInfo->biCompression == 'BGRA')
	{
		DWORD w = pBitmapInfo->biWidth;
		DWORD h = abs(pBitmapInfo->biHeight);
		DWORD pitch = pBitmapInfo->biWidth*pBitmapInfo->biBitCount>>3;

		BYTE* rgb = pImageData + src.top*pitch + src.left*(pBitmapInfo->biBitCount>>3);

		D3DLOCKED_RECT r;
		if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, src2, 0)))
		{
			BYTE* pBits = (BYTE*)r.pBits;
			if(pBitmapInfo->biHeight > 0) {pBits += r.Pitch*(src.Height()-1); r.Pitch = -r.Pitch;}
			BitBltFromRGBToRGB(src.Width(), src.Height(), pBits, r.Pitch, dbpp, rgb, pitch, pBitmapInfo->biBitCount);
			m_pVideoSurfaceOff->UnlockRect();
			fRGB = true;
		}
	}

	if(!fRGB && !fYUY2)
	{
		m_pD3DDev->ColorFill(m_pVideoSurfaceOff, NULL, 0);

		HDC hDC;
		if(SUCCEEDED(m_pVideoSurfaceOff->GetDC(&hDC)))
		{
			CString str;
			str.Format(_T("Sorry, this format is not supported"));

			SetBkColor(hDC, 0);
			SetTextColor(hDC, 0x404040);
			TextOut(hDC, 10, 10, str, str.GetLength());

			m_pVideoSurfaceOff->ReleaseDC(hDC);

			fRGB = true;
		}
	}

	HRESULT hr;
	
	if(fRGB)
		hr = m_pD3DDev->StretchRect(m_pVideoSurfaceOff, src2, m_pVideoSurface, dst, D3DTEXF_NONE);
	if(fYUY2)
		hr = m_pD3DDev->StretchRect(m_pVideoSurfaceYUY2, src2, m_pVideoSurface, dst, D3DTEXF_NONE);

	Paint(true);

	return PNR_OK;
}

STDMETHODIMP CRM9AllocatorPresenter::BeginOptimizedBlt(RMABitmapInfoHeader* pBitmapInfo)
{
    CAutoLock cAutoLock(this);
	DeleteSurfaces();
	m_NativeVideoSize = m_AspectRatio = CSize(pBitmapInfo->biWidth, abs(pBitmapInfo->biHeight));
	if(FAILED(AllocSurfaces())) return E_FAIL;
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::OptimizedBlt(UCHAR* pImageBits, REF(PNxRect) rDestRect, REF(PNxRect) rSrcRect)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::EndOptimizedBlt()
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::GetOptimizedFormat(REF(RMA_COMPRESSION_TYPE) ulType)
{
	return PNR_NOTIMPL;
}

STDMETHODIMP CRM9AllocatorPresenter::GetPreferredFormat(REF(RMA_COMPRESSION_TYPE) ulType)
{
	ulType = RMA_I420;
	return PNR_OK;
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
	if(FAILED(AllocSurfaces())) return E_FAIL;
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

	int w = bm.bmWidth;
	int h = abs(bm.bmHeight);
	int bpp = bm.bmBitsPixel;
	int dbpp = 
		d3dsd.Format == D3DFMT_R8G8B8 || d3dsd.Format == D3DFMT_X8R8G8B8 || d3dsd.Format == D3DFMT_A8R8G8B8 ? 32 : 
		d3dsd.Format == D3DFMT_R5G6B5 ? 16 : 0;

	if((bpp == 16 || bpp == 24 || bpp == 32) && w == d3dsd.Width && h == d3dsd.Height)
	{
		D3DLOCKED_RECT r;
		if(SUCCEEDED(m_pVideoSurfaceOff->LockRect(&r, NULL, 0)))
		{
			BitBltFromRGBToRGB(
				w, h,
				(BYTE*)r.pBits, r.Pitch, dbpp,
				(BYTE*)bm.bmBits, bm.bmWidthBytes, bm.bmBitsPixel);
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
