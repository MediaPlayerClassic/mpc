/* 
 *	Copyright (C) 2003 Gabest
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
#include <math.h>
#include <time.h>
#include "DirectVobSubUIDs.h"
#include "DirectVobSubFilter.h"
#include "DirectVobSubInputPin.h"
#include "DirectVobSubOutputPin.h"
#include "TextInputPin.h"
#include "DirectVobSubPropPage.h"
#include "VSFilter.h"
#include "systray.h"
#include "..\..\..\DSUtil\MediaTypes.h"
#include "..\..\..\SubPic\MemSubPic.h"
#include "..\..\..\..\include\Ogg\OggDS.h"
#include "..\..\..\..\include\matroska\matroska.h"

///////////////////////////////////////////////////////////////////////////

/*removeme*/
bool g_RegOK = true;//false; // doen't work with the dvd graph builder
#include "valami.cpp"

////////////////////////////////////////////////////////////////////////////
//
// Constructor
//

CDirectVobSubFilter::CDirectVobSubFilter(TCHAR* tszName, LPUNKNOWN punk, HRESULT* phr, const GUID& guid)
	: CTransformFilter(tszName, punk, guid)
	, m_Allocator(this, phr)
	, m_fUsingOwnAllocator(false)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_wIn = 0;
	memset(&m_bihIn, 0, sizeof(BITMAPINFOHEADER));
	memset(&m_bihOut, 0, sizeof(BITMAPINFOHEADER));
	m_sizeSub.SetSize(0, 0);

	m_nSubtitleId = -1;

	m_fMSMpeg4Fix = false;
	m_fDivxPlusFix = false;

	m_hdc = 0;
	m_hbm = 0;
	m_hfont = 0;

	m_fps = 25;

/* ResX2 */
	m_fResX2Active = false;

	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), _T("Hint"), _T("The first three are fixed, but you can add more up to ten entries."));
	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), _T("Path0"), _T("."));
	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), _T("Path1"), _T("c:\\subtitles"));
	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), _T("Path2"), _T(".\\subtitles"));

	m_fLoading = true;

	m_hSystrayThread = 0;
	m_tbid.hSystrayWnd = NULL;
	m_tbid.graph = NULL;
	m_tbid.fRunOnce = false;
	m_tbid.fShowIcon = (theApp.m_AppName.Find(_T("zplayer"), 0) < 0 || !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ENABLEZPICON), 0));

	HRESULT hr = S_OK;
	m_pInput = new CDirectVobSubInputPin(this, &hr);
	ASSERT(SUCCEEDED(hr));
	m_pOutput = new CDirectVobSubOutputPin(this, &hr);
    ASSERT(SUCCEEDED(hr));
	if(!m_pInput || !m_pOutput)
	{
		delete m_pInput, m_pInput = NULL;
		delete m_pOutput, m_pOutput = NULL;
		return;
	}

	hr = S_OK;
	m_pTextInput.Add(new CTextInputPin(this, m_pLock, &m_csSubLock, &hr));
	ASSERT(SUCCEEDED(hr));

	CAMThread::Create();
	m_frd.EndThreadEvent.Create(0, FALSE, FALSE, 0);
	m_frd.RefreshEvent.Create(0, FALSE, FALSE, 0);
}

CDirectVobSubFilter::~CDirectVobSubFilter()
{
	CAutoLock cAutoLock(&m_csQueueLock);
	if(m_pSubPicQueue) m_pSubPicQueue->Invalidate();
	m_pSubPicQueue = NULL;

	if(m_hfont) {DeleteObject(m_hfont); m_hfont = 0;}
	if(m_hbm) {DeleteObject(m_hbm); m_hbm = 0;}
	if(m_hdc) {DeleteObject(m_hdc); m_hdc = 0;}

	for(int i = 0; i < m_pTextInput.GetSize(); i++) 
		delete m_pTextInput[i];

	m_frd.EndThreadEvent.Set();
	CAMThread::Close();
}

CUnknown* CDirectVobSubFilter::CreateInstance(LPUNKNOWN punk, HRESULT* phr)
{
    CUnknown* pUnk = new CDirectVobSubFilter(NAME("DirectVobSub"), punk, phr, CLSID_DirectVobSubFilter);
	if(pUnk == NULL) *phr = E_OUTOFMEMORY;
    return pUnk;
}

STDMETHODIMP CDirectVobSubFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return 
		QI(IDirectVobSub)
		QI(IDirectVobSub2)
		QI(ISpecifyPropertyPages)
		QI(IPersistStream)
		QI(IAMStreamSelect)
		CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDirectVobSubFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	if(pGraph)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		if(!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SEENDIVXWARNING), 0))
		{
			unsigned __int64 ver = GetFileVersion(_T("divx_c32.ax"));
			if(((ver >> 48)&0xffff) == 4 && ((ver >> 32)&0xffff) == 2)
			{
				DWORD dwVersion = GetVersion();
				DWORD dwWindowsMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
				DWORD dwWindowsMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
				
				if(dwVersion < 0x80000000 && dwWindowsMajorVersion >= 5)
				{
					AfxMessageBox(IDS_DIVX_WARNING);
					theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SEENDIVXWARNING), 1);
				}
			}
		}

		/*removeme*/
		if(!g_RegOK)
		{
			DllRegisterServer();
			g_RegOK = true;
		}
	}
	else
	{
		if(m_hSystrayThread)
		{
			SendMessage(m_tbid.hSystrayWnd, WM_CLOSE, 0, 0);

			if(WaitForSingleObject(m_hSystrayThread, 10000) != WAIT_OBJECT_0)
			{
				DbgLog((LOG_TRACE, 0, _T("CALL THE AMBULANCE!!!")));
				TerminateThread(m_hSystrayThread, (DWORD)-1);
			}

			m_hSystrayThread = 0;
		}
	}

	return CTransformFilter::JoinFilterGraph(pGraph, pName);
}

HRESULT CDirectVobSubFilter::StartStreaming()
{
	m_fLoading = false;

	if(m_hfont) {DeleteObject(m_hfont); m_hfont = NULL;}
	if(m_hbm) {DeleteObject(m_hbm); m_hbm = NULL;}
	if(m_hdc) {DeleteDC(m_hdc); m_hdc = NULL;}

	if(!m_hdc)
	{
		m_hdc = CreateCompatibleDC(NULL);

		struct {BITMAPINFOHEADER bih; DWORD mask[3];} b = 
		{
			{sizeof(BITMAPINFOHEADER), m_sizeSub.cx, -abs(m_sizeSub.cy), 1, 32, BI_BITFIELDS, 0, 0, 0, 0, 0},
			0xFF0000, 0x00FF00, 0x0000FF
		};
		
		m_hbm = CreateDIBSection(m_hdc, (BITMAPINFO*)&b, DIB_RGB_COLORS, NULL, NULL, 0);

		BITMAP bm;
		GetObject(m_hbm, sizeof(bm), &bm);
		memsetd(bm.bmBits, 0xFF000000, bm.bmHeight*bm.bmWidthBytes);
	}

	if(!m_hfont)
	{
		CAutoLock cAutolock(&m_propsLock);

		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfOutPrecision = OUT_CHARACTER_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = ANTIALIASED_QUALITY;
		HDC hdc = GetDC(NULL);
		lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(NULL, hdc);
		lf.lfWeight = FW_BOLD;
		_tcscpy(lf.lfFaceName, _T("Arial"));
		m_hfont = CreateFontIndirect(&lf);
	}

	InvalidateSubtitle();

	m_tbid.fRunOnce = true;

	CComPtr<IBaseFilter> pFilter;
	m_fDivxPlusFix = SUCCEEDED(m_pGraph->FindFilterByName(L"HPlus YUV Video Renderer", &pFilter));

	put_MediaFPS(m_fMediaFPSEnabled, m_MediaFPS);

	return CTransformFilter::StartStreaming();
}

HRESULT CDirectVobSubFilter::StopStreaming()
{
	InvalidateSubtitle();

	if(m_hfont) {DeleteObject(m_hfont); m_hfont = NULL;}
	if(m_hbm) {DeleteObject(m_hbm); m_hbm = NULL;}
	if(m_hdc) {DeleteDC(m_hdc); m_hdc = NULL;}

	return CTransformFilter::StopStreaming();
}

HRESULT CDirectVobSubFilter::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	if(dir == PINDIR_INPUT)
	{
	}
	else if(dir == PINDIR_OUTPUT)
	{
		/*removeme*/
		if(HmGyanusVagyTeNekem(pPin)) return(E_FAIL);
	}

	return CTransformFilter::CheckConnect(dir, pPin);
}

HRESULT CDirectVobSubFilter::CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin)
{
	if(dir == PINDIR_INPUT)
	{
		CComPtr<IBaseFilter> pFilter;

		// needed when we have a decoder with a version number of 3.x
		if(SUCCEEDED(m_pGraph->FindFilterByName(L"DivX MPEG-4 DVD Video Decompressor ", &pFilter)) 
		|| SUCCEEDED(m_pGraph->FindFilterByName(L"Microcrap MPEG-4 Video Decompressor", &pFilter))
		|| (SUCCEEDED(m_pGraph->FindFilterByName(L"Microsoft MPEG-4 Video Decompressor", &pFilter)) 
			&& (GetFileVersion(_T("mpg4ds32.ax")) >> 48) <= 3))
		{
			m_fMSMpeg4Fix = true;
		}

		m_wIn = m_bihIn.biWidth;
	}
	else if(dir == PINDIR_OUTPUT)
	{
		if(!m_hSystrayThread)
		{
			m_tbid.graph = m_pGraph;
			m_tbid.dvs = static_cast<IDirectVobSub*>(this);

			DWORD tid;
			m_hSystrayThread = CreateThread(0, 0, SystrayThreadProc, &m_tbid, 0, &tid);
		}

		m_sizeSub.SetSize(m_bihIn.biWidth, m_bihIn.biHeight);
		m_fResX2Active = AdjustFrameSize(m_sizeSub);

		InitSubPicQueue();
	}

	return CTransformFilter::CompleteConnect(dir, pReceivePin);
}

void CDirectVobSubFilter::InitSubPicQueue()
{
	CAutoLock cAutoLock(&m_csQueueLock);

	m_pSubPicQueue = NULL;

	m_pTempPicBuff.Free();
	m_pTempPicBuff.Allocate(4*m_sizeSub.cx*m_sizeSub.cy*(m_fResX2Active?4:1));

	const GUID& subtype = m_pOutput->CurrentMediaType().subtype;

	m_spd.type = -1;
	if(subtype == MEDIASUBTYPE_YV12) m_spd.type = MSP_YV12;
	else if(subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV) m_spd.type = MSP_IYUV;
	else if(subtype == MEDIASUBTYPE_YUY2) m_spd.type = MSP_YUY2;
	else if(subtype == MEDIASUBTYPE_RGB32) m_spd.type = MSP_RGB32;
	else if(subtype == MEDIASUBTYPE_RGB24) m_spd.type = MSP_RGB24;
	else if(subtype == MEDIASUBTYPE_RGB565) m_spd.type = MSP_RGB16;
	else if(subtype == MEDIASUBTYPE_RGB555) m_spd.type = MSP_RGB15;
	m_spd.w = m_sizeSub.cx;
	m_spd.h = m_sizeSub.cy;
	m_spd.bpp = (m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV) 
		? 8 : m_bihOut.biBitCount;
	m_spd.pitch = m_spd.w*m_spd.bpp>>3;
	m_spd.bits = (void*)m_pTempPicBuff;

	CComPtr<ISubPicAllocator> pSubPicAllocator = 
		new CMemSubPicAllocator(m_spd.type, m_sizeSub);

	CSize video(m_bihIn.biWidth, m_bihIn.biHeight);
	CSize window(m_bihIn.biWidth, m_bihIn.biHeight);
	if(AdjustFrameSize(window)) video += video;
	pSubPicAllocator->SetCurSize(window);
	pSubPicAllocator->SetCurVidRect(CRect(CPoint((window.cx - video.cx)/2, (window.cy - video.cy)/2), video));

	HRESULT hr = S_OK;
	m_pSubPicQueue = m_fDoPreBuffering 
		? (ISubPicQueue*)new CSubPicQueue(10, pSubPicAllocator, &hr)
		: (ISubPicQueue*)new CSubPicQueueNoThread(pSubPicAllocator, &hr);

	if(FAILED(hr)) m_pSubPicQueue = NULL;

	UpdateSubtitle(false);
}

HRESULT CDirectVobSubFilter::BreakConnect(PIN_DIRECTION dir)
{
	if(dir == PINDIR_INPUT)
	{
	}
	else if(dir == PINDIR_OUTPUT)
	{
		// not really needed, but may free up a little memory
		CAutoLock cAutoLock(&m_csQueueLock);
		m_pSubPicQueue = NULL;
	}

	return CTransformFilter::BreakConnect(dir);
}

CBasePin* CDirectVobSubFilter::GetPin(int n)
{
	if(n >= 0 && n < 2)
		return (n == 0) ? (CBasePin*)m_pInput : (n == 1) ? (CBasePin*)m_pOutput : NULL;

	n -= 2;

	if(n >= 0 && n < m_pTextInput.GetSize())
		return m_pTextInput[n]; 

	n -= m_pTextInput.GetSize();

	return NULL;
}

int CDirectVobSubFilter::GetPinCount()
{
	return (m_pInput?1:0) + (m_pOutput?1:0) + m_pTextInput.GetSize();
}

REFERENCE_TIME CDirectVobSubFilter::CalcCurrentTime()
{
	REFERENCE_TIME rt = m_pSubClock ? m_pSubClock->GetTime() : m_tPrev;
	return (rt - 10000i64*m_SubtitleDelay) * m_SubtitleSpeedMul / m_SubtitleSpeedDiv; // no, it won't overflow if we use normal parameters (__int64 is enough for about 2000 hours if we multiply it by the max: 65536 as m_SubtitleSpeedMul)
}

HRESULT CDirectVobSubFilter::Receive(IMediaSample* pSample)
{
	__asm emms; // just for safety

	return CTransformFilter::Receive(pSample);
}

HRESULT CDirectVobSubFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
#ifdef DEBUG
clock_t startt = clock();
#endif

	CRefTime tStart, tEnd;
	if(SUCCEEDED(pIn->GetTime(&tStart.m_time, &tEnd.m_time)))
	{
		double dRate = m_pInput->CurrentRate();
		m_tPrev = m_pInput->CurrentStartTime() + dRate*tStart;
		REFERENCE_TIME rtAvgTimePerFrame = tEnd-tStart;
		if(CComQIPtr<ISubClock2> pSC2 = m_pSubClock)
		{
			REFERENCE_TIME rt;
			 if(S_OK == pSC2->GetAvgTimePerFrame(&rt))
				 rtAvgTimePerFrame = rt;
		}
		m_fps = 10000000.0/rtAvgTimePerFrame / dRate;
	}

//TRACE(_T("m_tPrev = %I64d\n"), m_tPrev);

	CAutoLock cAutoLock(&m_csQueueLock);

	if(m_pSubPicQueue)
	{
		m_pSubPicQueue->SetTime(CalcCurrentTime());
		m_pSubPicQueue->SetFPS(m_fps);
	}

	bool fSkipFrame = false;

	{
		AM_MEDIA_TYPE* pmt = NULL;
		if(S_OK == pOut->GetMediaType((AM_MEDIA_TYPE**)&pmt) && pmt)
		{
			CMediaType mt(*pmt);
			DeleteMediaType(pmt), pmt = NULL;

			if(mt != m_pOutput->CurrentMediaType())
			{
TRACE(_T("m_pOutput->SetMediaType(&mt)\n"));
				m_pOutput->SetMediaType(&mt);
				fSkipFrame = true;

				InitSubPicQueue();
			}
		}
	}

	if(!fSkipFrame)
	{
		AM_MEDIA_TYPE* pmt = NULL;
		if(S_OK == pIn->GetMediaType((AM_MEDIA_TYPE**)&pmt) && pmt)
		{
			CMediaType mt(*pmt);
			DeleteMediaType(pmt), pmt = NULL;
DbgLog((LOG_TRACE, 0, _T("Transform: pIn->GetMediaType")));
			if(mt != m_pInput->CurrentMediaType())
			{
				// this is only for sending aspect ratio or palette changes 
				// downstream
				//
				// other format changes would already work with the VMR, but
				// not with the older renderers, so we must not change anything 
				// serious here which would affect the color space or the 
				// picture buffer size (we can be safe about these, by default
				// the input pin is rejecting ReceiveConnection when running)

				m_pInput->SetMediaType(&mt);
				ConvertMediaTypeInputToOutput(&mt);
                pOut->SetMediaType(&mt);
			}
		}
	}

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;
	if(FAILED(pIn->GetPointer(&pDataIn)) || !pDataIn
	|| FAILED(pOut->GetPointer(&pDataOut)) || !pDataOut)
		return S_FALSE;

	CMediaType& mt = m_pOutput->CurrentMediaType();

	bool fYV12 = (mt.subtype == MEDIASUBTYPE_YV12 || mt.subtype == MEDIASUBTYPE_I420 || mt.subtype == MEDIASUBTYPE_IYUV);
	int bpp = fYV12 ? 8 : m_bihOut.biBitCount;
	DWORD black = fYV12 ? 0x10101010 : (m_bihOut.biCompression == '2YUY') ? 0x80108010 : 0;

	if(fSkipFrame || m_sizeSub.cy != abs(m_bihOut.biHeight)
	|| m_pInput->CurrentMediaType().subtype != m_pOutput->CurrentMediaType().subtype)
	{
		if(fYV12)
		{
			DWORD size = m_bihOut.biWidth*abs(m_bihOut.biHeight);
			memset(pDataOut, 0x10, size);
			memset(pDataOut+size, 0x80, size>>1);
		}
		else
		{
			memsetd(pDataOut, black, m_bihOut.biSizeImage);
		}
	}
	else
	{
		{
			CSize sub = m_sizeSub, in = CSize(m_bihIn.biWidth, m_bihIn.biHeight);
			if(FAILED(Copy((BYTE*)m_pTempPicBuff, pDataIn, sub, in, m_wIn, bpp, mt.subtype, black, m_fResX2Active))) 
				return E_FAIL;

			if(fYV12)
			{
				BYTE* pSubV = (BYTE*)m_pTempPicBuff + (sub.cx*bpp>>3)*sub.cy;
				BYTE* pInV = pDataIn + (in.cx*bpp>>3)*in.cy;
				sub.cx >>= 1; sub.cy >>= 1; in.cx >>= 1; in.cy >>= 1;
				BYTE* pSubU = pSubV + (sub.cx*bpp>>3)*sub.cy;
				BYTE* pInU = pInV + (in.cx*bpp>>3)*in.cy;
				if(FAILED(Copy(pSubV, pInV, sub, in, m_wIn>>1, bpp, mt.subtype, 0x80808080, m_fResX2Active)))
					return E_FAIL;
				if(FAILED(Copy(pSubU, pInU, sub, in, m_wIn>>1, bpp, mt.subtype, 0x80808080, m_fResX2Active)))
					return E_FAIL;
			}
		}

		bool fFlip = m_bihOut.biHeight < 0 && m_bihOut.biCompression <= 3; // flip if we are copying rgb and the signs aren't matching (we only check the output height since input is always > 0)
		if(m_fFlipPicture) fFlip = !fFlip;
		if(m_fMSMpeg4Fix) fFlip = !fFlip;
//		if(m_fDivxPlusFix) fFlip = !fFlip;

		bool fFlipSub = (m_bihOut.biHeight > 0 && m_bihOut.biCompression <= 3); // flip unless the dst bitmap is also a flipped rgb
		if(m_fFlipSubtitles) fFlipSub = !fFlipSub;
//		if(m_fDivxPlusFix) fFlipSub = !fFlipSub;

		SubPicDesc spd = m_spd;

		if(m_pSubPicQueue)
		{
			CComPtr<ISubPic> pSubPic;
			if(SUCCEEDED(m_pSubPicQueue->LookupSubPic(CalcCurrentTime(), &pSubPic)) && pSubPic)
			{
				CRect r;
				pSubPic->GetDirtyRect(r);

				if(fFlip ^ fFlipSub)
					spd.h = -spd.h;

				pSubPic->AlphaBlt(r, r, &spd);
			}
		}

		{
			BYTE* pIn = (BYTE*)spd.bits;
			BYTE* pOut = pDataOut;
			int h = abs(spd.h), 
				pitchIn = spd.pitch, 
				pitchOut = m_bihOut.biWidth * bpp >> 3, 
				pitchMin = min(pitchIn, pitchOut);
			if(fFlip) {pIn += (h-1)*pitchIn; pitchIn = -pitchIn;}
			for(int h2 = h; h2-- > 0; pIn += pitchIn, pOut += pitchOut)
				memcpy_accel(pOut, pIn, pitchMin);
			pitchIn = abs(pitchIn);

			if(fYV12)
			{
				BYTE* pInV = (BYTE*)spd.bits + pitchIn*h;
				BYTE* pOutV = pDataOut + pitchOut*h;
				h >>= 1; pitchIn >>= 1; pitchOut >>= 1; pitchMin = min(pitchIn, pitchOut);
				BYTE* pInU = pInV + pitchIn*h;
				BYTE* pOutU = pOutV + pitchOut*h;
				if(fFlip) {pInV += (h-1)*pitchIn; pInU += (h-1)*pitchIn; pitchIn = -pitchIn;}
				for(int h2 = h; h2-- > 0; pInV += pitchIn, pOutV += pitchOut)
					memcpy_accel(pOutV, pInV, pitchMin);
				for(int h2 = h; h2-- > 0; pInU += pitchIn, pOutU += pitchOut)
					memcpy_accel(pOutU, pInU, pitchMin);
				pitchIn = abs(pitchIn);
			}
		}
	}

	PrintMessages(pDataOut);

#ifdef DEBUG
clock_t dtt = clock() - startt;
if(dtt > 20) DbgLog((LOG_TRACE, 0, _T("Transform: %d"), dtt));
#endif

	return NOERROR;
}

HRESULT CDirectVobSubFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_tPrev = tStart;

	// TODO: check if we really need to do this
//	InvalidateSubtitle();

    return CTransformFilter::NewSegment(tStart, tStop, dRate);
}

HRESULT CDirectVobSubFilter::CheckInputType(const CMediaType* mtIn)
{
	if(CheckOutputType(mtIn))
		return VFW_E_TYPE_NOT_ACCEPTED;

	BITMAPINFOHEADER bih;
	ExtractBIH(mtIn, &bih);

	return bih.biHeight > 0
		? NOERROR 
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDirectVobSubFilter::CheckOutputType(const CMediaType* mtOut)
{
	if(mtOut->majortype != MEDIATYPE_Video
	|| !(mtOut->formattype == FORMAT_VideoInfo || mtOut->formattype == FORMAT_VideoInfo2))
		return E_FAIL;

	return (mtOut->subtype == MEDIASUBTYPE_YUY2
		 || mtOut->subtype == MEDIASUBTYPE_YV12
		 || mtOut->subtype == MEDIASUBTYPE_I420
		 || mtOut->subtype == MEDIASUBTYPE_IYUV
		 || mtOut->subtype == MEDIASUBTYPE_RGB555
		 || mtOut->subtype == MEDIASUBTYPE_RGB565
		 || mtOut->subtype == MEDIASUBTYPE_RGB24
		 || mtOut->subtype == MEDIASUBTYPE_RGB32
		 || mtOut->subtype == MEDIASUBTYPE_ARGB32)
		 ? NOERROR : E_FAIL;
}

HRESULT CDirectVobSubFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	BITMAPINFOHEADER bihIn;
	ExtractBIH(mtIn, &bihIn);

	BITMAPINFOHEADER bihOut;
	ExtractBIH(mtOut, &bihOut);

	if(m_pInput && m_pInput->IsConnected())
	{
		if(GetCLSID(m_pInput->GetConnected()) == CLSID_AVIDec)
		{
			if(bihIn.biBitCount != bihOut.biBitCount)
				return VFW_E_TYPE_NOT_ACCEPTED;

			if(bihIn.biCompression != bihOut.biCompression
			&& (bihIn.biCompression > 3 || bihOut.biCompression > 3))
				return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return (CheckInputType(mtIn) == NOERROR && CheckOutputType(mtOut) == NOERROR)
			? NOERROR : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDirectVobSubFilter::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_bihOut.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties, &Actual);
    if(FAILED(hr)) return hr;

    ASSERT(Actual.cBuffers == 1);

	// TODO: check if the renderer has the allocator, otherwise it won't be able to dynamically change the format

    return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR);
}

HRESULT CDirectVobSubFilter::GetMediaType(int iPosition, CMediaType* pMediaType)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	bool fVIH2 = !!(iPosition&1);
	iPosition >>= 1;

	if(m_pInput->CurrentMediaType().formattype == FORMAT_VideoInfo2) fVIH2 = !fVIH2;

    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition >= VIHSIZE) return VFW_S_NO_MORE_ITEMS;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	BYTE* pData = NULL;
	UINT nSize;
	if(theApp.GetProfileBinary(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_COLORFORMATS), &pData, &nSize) && pData)
	{
		iPosition = pData[iPosition];
		delete [] pData;
	}

	bool fFound = false;

	BeginEnumMediaTypes(m_pInput->GetConnected(), pEMT, pmt)
	{
		const CMediaType mt(*pmt);
		if(S_OK == CheckInputType(&mt))
		{
			*pMediaType = mt;
			fFound = true;
			break;
		}
	}
	EndEnumMediaTypes(pmt)

	return fFound ? ConvertMediaTypeInputToOutput(pMediaType, iPosition, fVIH2) : VFW_S_NO_MORE_ITEMS;
}

bool CDirectVobSubFilter::AdjustFrameSize(CSize& s)
{
	int horizontal, vertical, resx2, resx2minw, resx2minh;
	get_ExtendPicture(&horizontal, &vertical, &resx2, &resx2minw, &resx2minh);

	bool fRet;
	if(fRet = (resx2 == 1) || (resx2 == 2 && s.cx*s.cy <= resx2minw*resx2minh))
	{
		s.cx <<= 1; 
		s.cy <<= 1;
	}

	int h;
	switch(vertical&0x7f)
	{
	case 1:
		h = s.cx * 9 / 16;
		if(s.cy < h || !!(vertical&0x80)) s.cy = (h + 3) & ~3;
		break;
	case 2:
		h = s.cx * 3 / 4;
		if(s.cy < h || !!(vertical&0x80)) s.cy = (h + 3) & ~3;
		break;
	case 3:
		h = 480;
		if(s.cy < h || !!(vertical&0x80)) s.cy = (h + 3) & ~3;
		break;
	case 4:
		h = 576;
		if(s.cy < h || !!(vertical&0x80)) s.cy = (h + 3) & ~3;
		break;
	}

	if(horizontal == 1)
	{
		s.cx = (s.cx + 31) & ~31;
		s.cy = (s.cy + 1) & ~1;
	}

	return(fRet);
}

HRESULT CDirectVobSubFilter::ConvertMediaTypeInputToOutput(CMediaType* pmt, int iVIHTemplate, bool fVIH2)
{
    if(iVIHTemplate >= VIHSIZE) 
		return VFW_S_NO_MORE_ITEMS;

	if(iVIHTemplate < 0)
	{
		if(IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo))
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->Format();

			for(int i = 0; i < VIHSIZE; i++)
			{
				if(*vihs[i].subtype == pmt->subtype
				&& vihs[i].vih.bmiHeader.biCompression == vih->bmiHeader.biCompression
				&& vihs[i].vih.bmiHeader.biBitCount == vih->bmiHeader.biBitCount)
				{
					iVIHTemplate = i;
					fVIH2 = false;
					break;
				}
			}
		}
		else if(IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo2))
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->Format();

			for(int i = 0; i < VIHSIZE; i++)
			{
				if(*vih2s[i].subtype == pmt->subtype
				&& vih2s[i].vih.bmiHeader.biCompression == vih->bmiHeader.biCompression
				&& vih2s[i].vih.bmiHeader.biBitCount == vih->bmiHeader.biBitCount)
				{
					iVIHTemplate = i;
					fVIH2 = true;
					break;
				}
			}
		}

		if(iVIHTemplate < 0)
		{
			return VFW_S_NO_MORE_ITEMS;
		}
	}

	CSize s, so;
    DWORD dwBitRate, dwBitErrorRate;
	REFERENCE_TIME AvgTimePerFrame;
	DWORD dwInterlaceFlags = 0, dwCopyProtectFlags = 0;
	DWORD dwPictAspectRatioX = 0, dwPictAspectRatioY = 0;
	DWORD dwControlFlags = 0;

	if(IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo))
	{
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->Format();
		s.cx = so.cx = dwPictAspectRatioX = vih->bmiHeader.biWidth;
		s.cy = so.cy = dwPictAspectRatioY = vih->bmiHeader.biHeight;
		dwBitRate = vih->dwBitRate;
		dwBitErrorRate = vih->dwBitErrorRate;
		AvgTimePerFrame = vih->AvgTimePerFrame;
	}
	else if(IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo2))
	{
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->Format();
		s.cx = so.cx = vih->bmiHeader.biWidth;
		s.cy = so.cy = vih->bmiHeader.biHeight;
		dwBitRate = vih->dwBitRate;
		dwBitErrorRate = vih->dwBitErrorRate;
		AvgTimePerFrame = vih->AvgTimePerFrame;
		dwInterlaceFlags = vih->dwInterlaceFlags;
		dwCopyProtectFlags = vih->dwCopyProtectFlags & (~AMCOPYPROTECT_RestrictDuplication); // hehe
		dwPictAspectRatioX = vih->dwPictAspectRatioX;
		dwPictAspectRatioY = vih->dwPictAspectRatioY;
		dwControlFlags = vih->dwControlFlags;
	}
	else
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	ASSERT(s.cy >= 0);

	AdjustFrameSize(s);

	if(!fVIH2)
	{
		pmt->SetFormatType(&FORMAT_VideoInfo);
		pmt->SetSubtype(vihs[iVIHTemplate].subtype);

		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->ReallocFormatBuffer(vihs[iVIHTemplate].size);
		if(!vih) return E_FAIL;

		memcpy(vih, &vihs[iVIHTemplate], pmt->FormatLength());

		vih->bmiHeader.biWidth = s.cx;
		vih->bmiHeader.biHeight = s.cy;
		vih->bmiHeader.biSizeImage = s.cx*s.cy*vih->bmiHeader.biBitCount>>3;
		vih->AvgTimePerFrame = AvgTimePerFrame;
		vih->dwBitErrorRate = dwBitErrorRate;
		vih->dwBitRate = dwBitRate;

		pmt->SetSampleSize(vih->bmiHeader.biSizeImage);
	}
	else
	{
		pmt->SetFormatType(&FORMAT_VideoInfo2);
		pmt->SetSubtype(vih2s[iVIHTemplate].subtype);

		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->ReallocFormatBuffer(vih2s[iVIHTemplate].size);
		if(!vih) return E_FAIL;

		memcpy(vih, &vih2s[iVIHTemplate], pmt->FormatLength());

		vih->bmiHeader.biWidth = s.cx;
		vih->bmiHeader.biHeight = s.cy;
		vih->bmiHeader.biSizeImage = s.cx*s.cy*vih->bmiHeader.biBitCount>>3;
		vih->AvgTimePerFrame = AvgTimePerFrame;
		vih->dwBitErrorRate = dwBitErrorRate;
		vih->dwBitRate = dwBitRate;
		vih->dwInterlaceFlags = dwInterlaceFlags;
		vih->dwCopyProtectFlags = dwCopyProtectFlags;
		vih->dwPictAspectRatioX = dwPictAspectRatioX*s.cx/so.cx;
		vih->dwPictAspectRatioY = dwPictAspectRatioY*s.cy/so.cy;
		vih->dwControlFlags = dwControlFlags;

		pmt->SetSampleSize(vih->bmiHeader.biSizeImage);
	}

	return NOERROR;
}

HRESULT CDirectVobSubFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	if(dir == PINDIR_INPUT)
	{
		CAutoLock cAutoLock(&m_csReceive);

		ASSERT(*pmt == m_pInput->CurrentMediaType());

//		if(!m_pOutput->IsConnected())
			m_Allocator.NotifyMediaType(*pmt/*m_pInput->CurrentMediaType()*/);

		ExtractBIH(pmt, &m_bihIn);

DbgLog((LOG_TRACE, 0, _T("SetMediaType PINDIR_INPUT, lines: %d"), m_bihIn.biHeight));
		REFERENCE_TIME atpf = 
			(pmt->formattype == FORMAT_VideoInfo) ? ((VIDEOINFOHEADER*)pmt->Format())->AvgTimePerFrame
			: (pmt->formattype == FORMAT_VideoInfo2) ? ((VIDEOINFOHEADER2*)pmt->Format())->AvgTimePerFrame
			: 0;

		m_fps = atpf ? 10000000.0 / atpf : 25;
	}
	else if(dir == PINDIR_OUTPUT)
	{
		if(m_fUsingOwnAllocator && m_pInput->IsConnected())
		{
			CComPtr<IPin> pIn = m_pInput->GetConnected();

			BeginEnumMediaTypes(pIn, pEMT, pmt2)
			{
				if(pmt->majortype == pmt2->majortype && pmt->subtype == pmt2->subtype && SUCCEEDED(pIn->QueryAccept(pmt2))) 
				{
					BITMAPINFOHEADER bih, bih2;
					ExtractBIH(pmt, &bih);
					ExtractBIH(pmt2, &bih2);
					CSize s(bih2.biWidth, abs(bih2.biHeight));
					AdjustFrameSize(s);
					if(s.cy != abs(bih.biHeight))
						continue;

					const CMediaType mt(*pmt2);
					if(m_pInput->CurrentMediaType() == mt || SUCCEEDED(m_pInput->SetMediaType(&mt)))
						break;
				}
			}
			EndEnumMediaTypes(pmt2)
		}

		ExtractBIH(pmt, &m_bihOut);
	}

	return CTransformFilter::SetMediaType(dir, pmt);
}

STDMETHODIMP CDirectVobSubFilter::Count(DWORD* pcStreams)
{
	if(!pcStreams) return E_POINTER;

	*pcStreams = 0;

	int nLangs = 0;
	if(SUCCEEDED(get_LanguageCount(&nLangs)))
		(*pcStreams) += nLangs;

	(*pcStreams) += 2; // enable ... disable

	(*pcStreams) += 2; // normal flipped

	return S_OK;
}

#define MAXPREFLANGS 5

int CDirectVobSubFilter::FindPreferedLanguage(bool fHideToo)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int nLangs;
	get_LanguageCount(&nLangs);

	if(nLangs <= 0) return(0);

	for(int i = 0; i < MAXPREFLANGS; i++)
	{
		CString tmp;
		tmp.Format(IDS_RL_LANG, i);

		CString lang = theApp.GetProfileString(ResStr(IDS_R_PREFLANGS), tmp);
		
		if(!lang.IsEmpty())
		{
			for(int ret = 0; ret < nLangs; ret++)
			{
				CString l;
				WCHAR* pName = NULL;
				get_LanguageName(ret, &pName);
				l = pName;
				CoTaskMemFree(pName);

				if(!l.CompareNoCase(lang)) return(ret);
			}
		}
	}

	return(0);
}

void CDirectVobSubFilter::UpdatePreferedLanguages(CString l)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString langs[MAXPREFLANGS+1];

	int i = 0, j = 0, k = -1;
	for(; i < MAXPREFLANGS; i++)
	{
		CString tmp;
		tmp.Format(IDS_RL_LANG, i);

		langs[j] = theApp.GetProfileString(ResStr(IDS_R_PREFLANGS), tmp);

		if(!langs[j].IsEmpty()) 
		{
			if(!langs[j].CompareNoCase(l)) k = j;
			j++;
		}
	}

	if(k == -1)
	{
		langs[k = j] = l;
		j++;
	}

	// move the selected to the top of the list

	while(k > 0)
	{
		CString tmp = langs[k]; langs[k] = langs[k-1]; langs[k-1] = tmp;
		k--;
	}

	// move "Hide subtitles" to the last position if it wasn't our selection

	CString hidesubs;
	hidesubs.LoadString(IDS_M_HIDESUBTITLES);

	for(k = 1; k < j; k++)
	{
		if(!langs[k].CompareNoCase(hidesubs)) break;
	}

	while(k < j-1)
	{
		CString tmp = langs[k]; langs[k] = langs[k+1]; langs[k+1] = tmp;
		k++;
	}

	for(i = 0; i < j; i++)
	{
		CString tmp;
		tmp.Format(IDS_RL_LANG, i);

		theApp.WriteProfileString(ResStr(IDS_R_PREFLANGS), tmp, langs[i]);
	}
}

STDMETHODIMP CDirectVobSubFilter::Enable(long lIndex, DWORD dwFlags)
{
	if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
		return E_NOTIMPL;

	int nLangs = 0;
	get_LanguageCount(&nLangs);

	if(!(lIndex >= 0 && lIndex < nLangs+2+2)) 
		return E_INVALIDARG;

	int i = lIndex-1;

	if(i == -1 && !m_fLoading) // we need this because when loading something stupid media player pushes the first stream it founds, which is "enable" in our case
	{
		put_HideSubtitles(false);
	}
	else if(i >= 0 && i < nLangs)
	{
		put_HideSubtitles(false);
		put_SelectedLanguage(i);

		WCHAR* pName = NULL;
		if(SUCCEEDED(get_LanguageName(i, &pName)))
		{
			UpdatePreferedLanguages(CString(pName));
			if(pName) CoTaskMemFree(pName);
		}
	}
	else if(i == nLangs && !m_fLoading)
	{
		put_HideSubtitles(true);
	}
	else if((i == nLangs+1 || i == nLangs+2) && !m_fLoading)
	{
		put_Flip(i == nLangs+2, m_fFlipSubtitles);
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSubFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int nLangs = 0;
	get_LanguageCount(&nLangs);

	if(!(lIndex >= 0 && lIndex < nLangs+2+2)) 
		return E_INVALIDARG;

	int i = lIndex-1;

	if(ppmt) *ppmt = CreateMediaType(&m_pInput->CurrentMediaType());

	if(pdwFlags)
	{
		*pdwFlags = 0;

		if(i == -1 && !m_fHideSubtitles
		|| i >= 0 && i < nLangs && i == m_iSelectedLanguage
		|| i == nLangs && m_fHideSubtitles
		|| i == nLangs+1 && !m_fFlipPicture
		|| i == nLangs+2 && m_fFlipPicture)
		{
			*pdwFlags |= AMSTREAMSELECTINFO_ENABLED;
		}
	}

	if(plcid) *plcid = 0;

	if(pdwGroup) *pdwGroup = 0x648E51;

	if(ppszName)
	{
		*ppszName = NULL;

		CStringW str;
		if(i == -1) str = ResStr(IDS_M_SHOWSUBTITLES);
		else if(i >= 0 && i < nLangs) get_LanguageName(i, ppszName);
		else if(i == nLangs) str = ResStr(IDS_M_HIDESUBTITLES);
		else if(i == nLangs+1) {str = ResStr(IDS_M_ORIGINALPICTURE); if(pdwGroup) (*pdwGroup)++;}
		else if(i == nLangs+2) {str = ResStr(IDS_M_FLIPPEDPICTURE); if(pdwGroup) (*pdwGroup)++;}

		if(!str.IsEmpty())
		{
			*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
			if(*ppszName == NULL) return S_FALSE;
			wcscpy(*ppszName, str);
		}
	}

	if(ppObject) *ppObject = NULL;

	if(ppUnk) *ppUnk = NULL;

	return S_OK;
}

STDMETHODIMP CDirectVobSubFilter::GetClassID(CLSID* pClsid)
{
    if(pClsid == NULL) return E_POINTER;
	*pClsid = m_clsid;
    return NOERROR;
}

STDMETHODIMP CDirectVobSubFilter::GetPages(CAUUID* pPages)
{
	pPages->cElems = 7;
    pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID)*pPages->cElems);

	if(pPages->pElems == NULL) return E_OUTOFMEMORY;

	int i = 0;
    pPages->pElems[i++] = CLSID_DVSMainPPage;
    pPages->pElems[i++] = CLSID_DVSGeneralPPage;
    pPages->pElems[i++] = CLSID_DVSMiscPPage;
    pPages->pElems[i++] = CLSID_DVSTimingPPage;
    pPages->pElems[i++] = CLSID_DVSColorPPage;
    pPages->pElems[i++] = CLSID_DVSPathsPPage;
    pPages->pElems[i++] = CLSID_DVSAboutPPage;

    return NOERROR;
}

// IDirectVobSub

STDMETHODIMP CDirectVobSubFilter::put_FileName(WCHAR* fn)
{
	HRESULT hr = CDirectVobSub::put_FileName(fn);

	if(hr == S_OK && !Open()) 
	{
		m_FileName.Empty();
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_LanguageCount(int* nLangs)
{
	HRESULT hr = CDirectVobSub::get_LanguageCount(nLangs);

	if(hr == NOERROR && nLangs)
	{
        CAutoLock cAutolock(&m_csQueueLock);

		*nLangs = 0;
		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(pos) (*nLangs) += m_pSubStreams.GetNext(pos)->GetStreamCount();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_LanguageName(int iLanguage, WCHAR** ppName)
{
	HRESULT hr = CDirectVobSub::get_LanguageName(iLanguage, ppName);

	if(!ppName) return E_POINTER;

	if(hr == NOERROR)
	{
        CAutoLock cAutolock(&m_csQueueLock);

		hr = E_INVALIDARG;

		int i = iLanguage;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(i >= 0 && pos)
		{
			CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

			if(i < pSubStream->GetStreamCount())
			{
				pSubStream->GetStreamInfo(i, ppName, NULL);
				hr = NOERROR;
				break;
			}

			i -= pSubStream->GetStreamCount();
		}
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_SelectedLanguage(int iSelected)
{
	HRESULT hr = CDirectVobSub::put_SelectedLanguage(iSelected);

	if(hr == NOERROR)
	{
		UpdateSubtitle(false);
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_HideSubtitles(bool fHideSubtitles)
{
	HRESULT hr = CDirectVobSub::put_HideSubtitles(fHideSubtitles);

	if(hr == NOERROR)
	{
		UpdateSubtitle(false);
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_PreBuffering(bool fDoPreBuffering)
{
	HRESULT hr = CDirectVobSub::put_PreBuffering(fDoPreBuffering);

	if(hr == NOERROR)
	{
		InitSubPicQueue();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_Placement(bool fOverridePlacement, int xperc, int yperc)
{
	HRESULT hr = CDirectVobSub::put_Placement(fOverridePlacement, xperc, yperc);

	if(hr == NOERROR)
	{
		UpdateSubtitle(true);
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_VobSubSettings(bool fBuffer, bool fOnlyShowForcedSubs, bool fReserved)
{
	HRESULT hr = CDirectVobSub::put_VobSubSettings(fBuffer, fOnlyShowForcedSubs, fReserved);

	if(hr == NOERROR)
	{
//		UpdateSubtitle(false);
		InvalidateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_TextSettings(void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer)
{
	HRESULT hr = CDirectVobSub::put_TextSettings(lf, lflen, color, fShadow, fOutline, fAdvancedRenderer);
	
	if(hr == NOERROR)
	{
//		UpdateSubtitle(true);
		InvalidateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_SubtitleTiming(int delay, int speedmul, int speeddiv)
{
	HRESULT hr = CDirectVobSub::put_SubtitleTiming(delay, speedmul, speeddiv);

	if(hr == NOERROR)
	{
		InvalidateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_MediaFPS(bool* fEnabled, double* fps)
{
	HRESULT hr = CDirectVobSub::get_MediaFPS(fEnabled, fps);

	CComQIPtr<IMediaSeeking> pMS = m_pGraph;
	double rate;
	if(pMS && SUCCEEDED(pMS->GetRate(&rate)))
	{
		m_MediaFPS = rate * m_fps;
		if(fps) *fps = m_MediaFPS;
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_MediaFPS(bool fEnabled, double fps)
{
	HRESULT hr = CDirectVobSub::put_MediaFPS(fEnabled, fps);

	CComQIPtr<IMediaSeeking> pMS = m_pGraph;
	if(pMS)
	{
		if(hr == NOERROR)
		{
			hr = pMS->SetRate(m_fMediaFPSEnabled ? m_MediaFPS / m_fps : 1.0);
		}

		double dRate;
		if(SUCCEEDED(pMS->GetRate(&dRate)))
			m_MediaFPS = dRate * m_fps;
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_ZoomRect(NORMALIZEDRECT* rect)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDirectVobSubFilter::put_ZoomRect(NORMALIZEDRECT* rect)
{
	return E_NOTIMPL;
}

// IDirectVobSub2

STDMETHODIMP CDirectVobSubFilter::put_TextSettings(STSStyle* pDefStyle)
{
	HRESULT hr = CDirectVobSub::put_TextSettings(pDefStyle);
	
	if(hr == NOERROR)
	{
		UpdateSubtitle(true);
	}

	return hr;
}

// IDirectVobSubFilterColor

STDMETHODIMP CDirectVobSubFilter::get_ColorFormat(int* iPosition)
{
	if(!m_pOutput || !m_pOutput->IsConnected() || !iPosition) return E_FAIL;
	
	BITMAPINFOHEADER bih, bih2;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bih);
	const GUID& subtype = m_pOutput->CurrentMediaType().subtype;

	*iPosition = 0;
	CMediaType mt;
	while(SUCCEEDED(GetMediaType((*iPosition)<<1, &mt)))
	{
		ExtractBIH(&mt, &bih2);
		if(mt.subtype == subtype 
		&& bih2.biBitCount == bih.biBitCount 
		&& bih2.biCompression == bih.biCompression)
		{
			return S_OK;
		}
		
		(*iPosition)++;
	}
	
	return E_FAIL;
}

HRESULT CDirectVobSubFilter::ChangeMediaType(int iPosition)
{
	if(!m_pOutput || !m_pOutput->IsConnected() || m_State == State_Paused) return E_FAIL;

	CAutoLock cAutoLock(&m_csReceive);

	CComPtr<IPin> pPin = m_pOutput->GetConnected();
	if(!pPin) return E_FAIL;

	CComQIPtr<IPinConnection> pPinConnection = pPin;
	CComQIPtr<IMemInputPin> pMemImputPin = pPin;
	if(!pPinConnection || !pMemImputPin) return E_FAIL;

	CComPtr<IMemAllocator> pAllocator;
	if(FAILED(pMemImputPin->GetAllocator(&pAllocator))) return E_FAIL;

	CMediaType orgmt;
	orgmt = m_pOutput->CurrentMediaType();

	HRESULT hr;

	CMediaType mt;
	GetMediaType(iPosition<<1, &mt);

	if(SUCCEEDED(hr = pPinConnection->DynamicQueryAccept(&mt)))
	{
		if(SUCCEEDED(hr = pPin->ReceiveConnection(m_pOutput, &mt)))
		{
			// this shouldn't be needed, but the old renderer won't attach 
			// the new media type to the next sample, despite the fact that 
			// it has just accepted it...
			m_pOutput->SetMediaType(&mt); 

			return(NOERROR);
		}
	}

	return(E_FAIL);
}

STDMETHODIMP CDirectVobSubFilter::put_ColorFormat(int iPosition)
{
	if(!m_pOutput || !m_pOutput->IsConnected() /*|| !m_fVMRFilter*/) return E_FAIL;

	return ChangeMediaType(iPosition);
}

STDMETHODIMP CDirectVobSubFilter::HasConfigDialog(int iSelected)
{
	int nLangs;
	if(FAILED(get_LanguageCount(&nLangs))) return E_FAIL;
	return E_FAIL;
	// TODO: temporally disabled since we don't have a new textsub/vobsub editor dlg for dvs yet
//	return(nLangs >= 0 && iSelected < nLangs ? S_OK : E_FAIL);
}

STDMETHODIMP CDirectVobSubFilter::ShowConfigDialog(int iSelected, HWND hWndParent)
{
	// TODO: temporally disabled since we don't have a new textsub/vobsub editor dlg for dvs yet
	return(E_FAIL);
}

////////////////////////////////////////////////////////////////////////

STDMETHODIMP CDirectVobSubFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
    CheckPointer(pInfo, E_POINTER);
    ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	HRESULT hr = E_FAIL;

	if(get_Forced())
	{
        wcscpy(pInfo->achName, L"DirectVobSub (forced auto-loading version)");
		if(pInfo->pGraph = m_pGraph) m_pGraph->AddRef();
		hr = NOERROR;
	}
	else
	{
		hr = CTransformFilter::QueryFilterInfo(pInfo);
	}

	return(hr);
}

///////////////////////////////////////////////////////////////////////////

CDirectVobSubFilter2::CDirectVobSubFilter2(TCHAR* tszName, LPUNKNOWN punk, HRESULT* phr, const GUID& guid) :
	CDirectVobSubFilter(tszName, punk, phr, guid)
{
}

CUnknown* CDirectVobSubFilter2::CreateInstance(LPUNKNOWN punk, HRESULT* phr)
{
	CUnknown* pUnk = new CDirectVobSubFilter2(NAME("DirectVobSub (auto-loading version)"), punk, phr, CLSID_DirectVobSubFilter2);
	if(pUnk == NULL) *phr = E_OUTOFMEMORY;
	return pUnk;
}

HRESULT CDirectVobSubFilter2::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	CPinInfo pi;
	if(FAILED(pPin->QueryPinInfo(&pi))) return E_FAIL;

	if(CComQIPtr<IDirectVobSub>(pi.pFilter)) return E_FAIL;

	if(dir == PINDIR_INPUT)
	{
		CFilterInfo fi;
		if(SUCCEEDED(pi.pFilter->QueryFilterInfo(&fi))
		&& !wcsnicmp(fi.achName, L"Overlay Mixer", 13))
			return(E_FAIL);
	}
	else
	{
	}

	return CDirectVobSubFilter::CheckConnect(dir, pPin);
}

HRESULT CDirectVobSubFilter2::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	if(pGraph)
	{
		BeginEnumFilters(pGraph, pEF, pBF)
		{
			if(pBF != (IBaseFilter*)this && CComQIPtr<IDirectVobSub>(pBF))
				return E_FAIL;
		}
		EndEnumFilters

		// don't look... we will do some serious graph hacking again...
		//
		// we will add dvs2 to the filter graph cache
		// - if the main app has already added some kind of renderer or overlay mixer (anything which accepts video on its input)
		// and 
		// - if we have a reason to auto-load (we don't want to make any trouble when there is no need :)
		//
		// This whole workaround is needed because the video stream will always be connect 
		// to the pre-added filters first, no matter how high merit we have.

		if(!get_Forced())
		{
			BeginEnumFilters(pGraph, pEF, pBF)
			{
				if(CComQIPtr<IDirectVobSub>(pBF))
					continue;

				CComPtr<IPin> pInPin = GetFirstPin(pBF, PINDIR_INPUT);
				CComPtr<IPin> pOutPin = GetFirstPin(pBF, PINDIR_OUTPUT);

				if(!pInPin)
					continue;

				CComPtr<IPin> pPin;
				if(pInPin && SUCCEEDED(pInPin->ConnectedTo(&pPin))
				|| pOutPin && SUCCEEDED(pOutPin->ConnectedTo(&pPin)))
					continue;

				if(pOutPin && GetFilterName(pBF) == _T("Overlay Mixer")) 
					continue;

				bool fVideoInputPin = false;

				do
				{
					BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER), 384, 288, 1, 16, '2YUY', 384*288*2, 0, 0, 0, 0};

					CMediaType cmt;
					cmt.majortype = MEDIATYPE_Video;
					cmt.subtype = MEDIASUBTYPE_YUY2;
					cmt.formattype = FORMAT_VideoInfo;
					cmt.pUnk = NULL;
					cmt.bFixedSizeSamples = TRUE;
					cmt.bTemporalCompression = TRUE;
					cmt.lSampleSize = 384*288*2;
					VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)cmt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
					memset(vih, 0, sizeof(VIDEOINFOHEADER));
					memcpy(&vih->bmiHeader, &bih, sizeof(bih));
					vih->AvgTimePerFrame = 400000;

					if(SUCCEEDED(pInPin->QueryAccept(&cmt))) 
					{
						fVideoInputPin = true;
						break;
					}

					VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)cmt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
					memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
					memcpy(&vih2->bmiHeader, &bih, sizeof(bih));
					vih2->AvgTimePerFrame = 400000;
					vih2->dwPictAspectRatioX = 384;
					vih2->dwPictAspectRatioY = 288;

					if(SUCCEEDED(pInPin->QueryAccept(&cmt)))
					{
						fVideoInputPin = true;
						break;
					}
				}
				while(false);

				if(fVideoInputPin)
				{
					CComPtr<IBaseFilter> pDVS;
					if(ShouldWeAutoload(pGraph) && SUCCEEDED(pDVS.CoCreateInstance(CLSID_DirectVobSubFilter2)))
					{
						CComQIPtr<IDirectVobSub2>(pDVS)->put_Forced(true);
						CComQIPtr<IGraphConfig>(pGraph)->AddFilterToCache(pDVS);
					}

					break;
				}
			}
			EndEnumFilters
		}
	}
	else
	{
	}

	return CDirectVobSubFilter::JoinFilterGraph(pGraph, pName);
}

HRESULT CDirectVobSubFilter2::CheckInputType(const CMediaType* mtIn)
{
    HRESULT hr = CDirectVobSubFilter::CheckInputType(mtIn);

	if(FAILED(hr) || m_pInput->IsConnected()) return hr;

	if(!ShouldWeAutoload(m_pGraph)) return VFW_E_TYPE_NOT_ACCEPTED;

	GetRidOfInternalScriptRenderer();

	return NOERROR;
}

bool CDirectVobSubFilter2::ShouldWeAutoload(IFilterGraph* pGraph)
{
	TCHAR blacklistedapps[][32] = 
	{
		_T("WM8EUTIL."), // wmp8 encoder's dummy renderer releases the outputted media sample after calling Receive on its input pin (yes, even when dvobsub isn't registered at all)
		_T("explorer."), // as some users reported thumbnail preview loads dvobsub, I've never experienced this yet...
		_T("producer."), // this is real's producer
	};

	for(int i = 0; i < countof(blacklistedapps); i++)
	{
		if(theApp.m_AppName.Find(blacklistedapps[i]) >= 0) 
			return(false);
	}

	int level;
	bool m_fExternalLoad, m_fWebLoad, m_fEmbeddedLoad;
	get_LoadSettings(&level, &m_fExternalLoad, &m_fWebLoad, &m_fEmbeddedLoad);

	if(level < 0 || level >= 2) return(false);

	bool fRet = false;

	if(level == 1)
		fRet = m_fExternalLoad = m_fWebLoad = m_fEmbeddedLoad = true;

	// find text stream on known splitters

	if(!fRet && m_fEmbeddedLoad)
	{
		CComPtr<IBaseFilter> pBF;
		if((pBF = FindFilter(CLSID_OggSplitter, pGraph)) || (pBF = FindFilter(CLSID_AviSplitter, pGraph))
		|| (pBF = FindFilter(GUIDFromCString("{34293064-02F2-41D5-9D75-CC5967ACA1AB}"), pGraph)) // matroska demux
		|| (pBF = FindFilter(GUIDFromCString("{0A68C3B5-9164-4a54-AFAF-995B2FF0E0D4}"), pGraph)) // matroska source
		|| (pBF = FindFilter(GUIDFromCString("{149D2E01-C32E-4939-80F6-C07B81015A7A}"), pGraph))) // matroska splitter
		{
			BeginEnumPins(pBF, pEP, pPin)
			{
				BeginEnumMediaTypes(pPin, pEM, pmt)
				{
					if(pmt->majortype == MEDIATYPE_Text || pmt->majortype == MEDIATYPE_Subtitle)
					{
						fRet = true;
						break;
					}
				}
				EndEnumMediaTypes(pmt)
				if(fRet) break;
			}
			EndEnumFilters
		}
	}

	// find file name

	CStringW fn;

	BeginEnumFilters(pGraph, pEF, pBF)
	{
		if(CComQIPtr<IFileSourceFilter> pFSF = pBF)
		{
			LPOLESTR fnw = NULL;
			if(!pFSF || FAILED(pFSF->GetCurFile(&fnw, NULL)) || !fnw)
				continue;
			fn = CString(fnw);
			CoTaskMemFree(fnw);
			break;
		}
	}
	EndEnumFilters

	if((m_fExternalLoad || m_fWebLoad) && (m_fWebLoad || !(wcsstr(fn, L"http://") || wcsstr(fn, L"mms://"))))
	{
		bool fTemp = m_fHideSubtitles;
		fRet = !fn.IsEmpty() && SUCCEEDED(put_FileName((LPWSTR)(LPCWSTR)fn))
			|| SUCCEEDED(put_FileName(L"c:\\tmp.srt")) 
			|| fRet;
		if(fTemp) m_fHideSubtitles = true;
	}

	return(fRet);
}

void CDirectVobSubFilter2::GetRidOfInternalScriptRenderer()
{
	while(CComPtr<IBaseFilter> pBF = FindFilter(L"{48025243-2D39-11CE-875D-00608CB78066}", m_pGraph))
	{
		BeginEnumPins(pBF, pEP, pPin)
		{
			PIN_DIRECTION dir;
			CComPtr<IPin> pPinTo;

			if(SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT 
			&& SUCCEEDED(pPin->ConnectedTo(&pPinTo)))
			{
				m_pGraph->Disconnect(pPinTo);
				m_pGraph->Disconnect(pPin);
				m_pGraph->ConnectDirect(pPinTo, GetPin(2 + m_pTextInput.GetSize()-1), NULL);
			}
		}
		EndEnumPins

		if(FAILED(m_pGraph->RemoveFilter(pBF)))
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////

bool CDirectVobSubFilter::Open()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutolock(&m_csQueueLock);

	m_pSubStreams.RemoveAll();

	m_frd.files.RemoveAll();

	CStringArray paths;

	for(int i = 0; i < 10; i++)
	{
		CString tmp;
		tmp.Format(IDS_RP_PATH, i);
		CString path = theApp.GetProfileString(ResStr(IDS_R_DEFTEXTPATHES), tmp);
		if(!path.IsEmpty()) paths.Add(path);
	}

	SubFiles ret;
	GetSubFileNames(m_FileName, paths, ret);

	for(int i = 0; i < ret.GetSize(); i++)
	{
		if(m_frd.files.Find(ret[i].fn))
			continue;

		CComPtr<ISubStream> pSubStream;

		if(!pSubStream)
		{
			CAutoPtr<CVobSubFile> pVSF(new CVobSubFile(&m_csSubLock));
			if(pVSF && pVSF->Open(ret[i].fn) && pVSF->GetStreamCount() > 0)
			{
				pSubStream = pVSF.Detach();
				m_frd.files.AddTail(ret[i].fn.Left(ret[i].fn.GetLength()-4) + _T(".sub"));
			}
		}

		if(!pSubStream)
		{
			CAutoPtr<CRenderedTextSubtitle> pRTS(new CRenderedTextSubtitle(&m_csSubLock));
			if(pRTS && pRTS->Open(ret[i].fn, DEFAULT_CHARSET) && pRTS->GetStreamCount() > 0)
			{
				pSubStream = pRTS.Detach();
				m_frd.files.AddTail(ret[i].fn + _T(".style"));
			}
		}
	    
		if(pSubStream)
		{
			m_pSubStreams.AddTail(pSubStream);
			m_frd.files.AddTail(ret[i].fn);
		}
	}

	for(int i = 0; i < m_pTextInput.GetSize(); i++)
	{
		if(m_pTextInput[i]->IsConnected())
			m_pSubStreams.AddTail(m_pTextInput[i]->GetSubStream());
	}

	if(S_FALSE == put_SelectedLanguage(FindPreferedLanguage()))
        UpdateSubtitle(false); // make sure pSubPicProvider of our queue gets updated even if the stream number hasn't changed

	m_frd.RefreshEvent.Set();

	return(m_pSubStreams.GetCount() > 0);
}

void CDirectVobSubFilter::UpdateSubtitle(bool fApplyDefStyle)
{
	CAutoLock cAutolock(&m_csQueueLock);

	if(!m_pSubPicQueue) return;

	InvalidateSubtitle();

	CComPtr<ISubStream> pSubStream;

	if(!m_fHideSubtitles)
	{
		int i = m_iSelectedLanguage;

		for(POSITION pos = m_pSubStreams.GetHeadPosition(); i >= 0 && pos; pSubStream = NULL)
		{
			pSubStream = m_pSubStreams.GetNext(pos);

			if(i < pSubStream->GetStreamCount()) 
			{
				CAutoLock cAutoLock(&m_csSubLock);
				pSubStream->SetStream(i);
				break;
			}

			i -= pSubStream->GetStreamCount();
		}
	}

	SetSubtitle(pSubStream, fApplyDefStyle);
}

void CDirectVobSubFilter::SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle)
{
    CAutoLock cAutolock(&m_csQueueLock);

	if(pSubStream)
	{
		CAutoLock cAutolock(&m_csSubLock);

		CLSID clsid;
		pSubStream->GetClassID(&clsid);

		if(clsid == __uuidof(CVobSubFile))
		{
			CVobSubSettings* pVSS = (CVobSubFile*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSS->SetAlignment(m_fOverridePlacement, m_PlacementXperc, m_PlacementYperc, 1, 1);
				pVSS->m_fOnlyShowForcedSubs = m_fOnlyShowForcedVobSubs;
			}
		}
		else if(clsid == __uuidof(CVobSubStream))
		{
			CVobSubSettings* pVSS = (CVobSubStream*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSS->SetAlignment(m_fOverridePlacement, m_PlacementXperc, m_PlacementYperc, 1, 1);
				pVSS->m_fOnlyShowForcedSubs = m_fOnlyShowForcedVobSubs;
			}
		}
		else if(clsid == __uuidof(CRenderedTextSubtitle))
		{
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

			if(fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle)
			{
				STSStyle s = m_defStyle;

				if(m_fOverridePlacement)
				{
					s.scrAlignment = 2;
					int w = pRTS->m_dstScreenSize.cx;
					int h = pRTS->m_dstScreenSize.cy;
					int mw = w - s.marginRect.left - s.marginRect.right;
					s.marginRect.bottom = h - MulDiv(h, m_PlacementYperc, 100);
					s.marginRect.left = MulDiv(w, m_PlacementXperc, 100) - mw/2;
					s.marginRect.right = w - (s.marginRect.left + mw);
				}

				pRTS->SetDefaultStyle(s);
			}

			pRTS->Deinit();
		}
	}

	if(!fApplyDefStyle)
	{
		int i = 0;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(pos)
		{
			CComPtr<ISubStream> pSubStream2 = m_pSubStreams.GetNext(pos);

			if(pSubStream == pSubStream2)
			{
				m_iSelectedLanguage = i + pSubStream2->GetStream();
				break;
			}

			i += pSubStream2->GetStreamCount();
		}
	}

	m_nSubtitleId = (DWORD_PTR)pSubStream;

	if(m_pSubPicQueue)
		m_pSubPicQueue->SetSubPicProvider(CComQIPtr<ISubPicProvider>(pSubStream));
}

void CDirectVobSubFilter::InvalidateSubtitle(REFERENCE_TIME rtInvalidate, DWORD_PTR nSubtitleId)
{
    CAutoLock cAutolock(&m_csQueueLock);

	if(m_pSubPicQueue)
	{
		if(nSubtitleId == -1 || nSubtitleId == m_nSubtitleId)
			m_pSubPicQueue->Invalidate(rtInvalidate);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

void CDirectVobSubFilter::AddSubStream(ISubStream* pSubStream)
{
	CAutoLock cAutoLock(&m_csQueueLock);

	POSITION pos = m_pSubStreams.Find(pSubStream);
	if(!pos) m_pSubStreams.AddTail(pSubStream);

	int len = m_pTextInput.GetSize();
	for(int i = 0; i < m_pTextInput.GetSize(); i++)
		if(m_pTextInput[i]->IsConnected()) len--;

	if(len == 0)
	{
		HRESULT hr = S_OK;
		m_pTextInput.Add(new CTextInputPin(this, m_pLock, &m_csSubLock, &hr));
	}
}

void CDirectVobSubFilter::RemoveSubStream(ISubStream* pSubStream)
{
	CAutoLock cAutoLock(&m_csQueueLock);

	POSITION pos = m_pSubStreams.Find(pSubStream);
	if(pos) m_pSubStreams.RemoveAt(pos);
}

void CDirectVobSubFilter::Post_EC_OLE_EVENT(CString str, DWORD_PTR nSubtitleId)
{
	if(nSubtitleId != -1 && nSubtitleId != m_nSubtitleId)
		return;

	CComQIPtr<IMediaEventSink> pMES = m_pGraph;
	if(!pMES) return;

	CComBSTR bstr1("Text"), bstr2(" ");

	str.Trim();
	if(!str.IsEmpty()) bstr2 = CStringA(str);

	pMES->Notify(EC_OLE_EVENT, (LONG_PTR)bstr1.Detach(), (LONG_PTR)bstr2.Detach());
}

////////////////////////////////////////////////////////////////

void CDirectVobSubFilter::SetupFRD(CStringArray& paths, CArray<HANDLE>& handles)
{
    CAutoLock cAutolock(&m_csSubLock);

	for(int i = 2; i < handles.GetSize(); i++)
	{
		FindCloseChangeNotification(handles[i]);
	}

	paths.RemoveAll();
	handles.RemoveAll();

	handles.Add(m_frd.EndThreadEvent);
	handles.Add(m_frd.RefreshEvent);

	m_frd.mtime.SetSize(m_frd.files.GetSize());

	POSITION pos = m_frd.files.GetHeadPosition();
	for(int i = 0; pos; i++)
	{
		CString fn = m_frd.files.GetNext(pos);

		CFileStatus status;
		if(CFile::GetStatus(fn, status)) 
			m_frd.mtime[i] = status.m_mtime;

		fn.Replace('\\', '/');
		fn = fn.Left(fn.ReverseFind('/')+1);

		bool fFound = false;

		for(int j = 0; !fFound && j < paths.GetSize(); j++)
		{
			if(paths[j] == fn) fFound = true;
		}

		if(!fFound)
		{
			paths.Add(fn);

			HANDLE h = FindFirstChangeNotification(fn, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE); 
			if(h != INVALID_HANDLE_VALUE) handles.Add(h);
		}
	}
}

DWORD CDirectVobSubFilter::ThreadProc()
{	
	SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST/*THREAD_PRIORITY_BELOW_NORMAL*/);

	CStringArray paths;
	CArray<HANDLE> handles;

	SetupFRD(paths, handles);

	while(1)
	{ 
		DWORD idx = WaitForMultipleObjects(handles.GetSize(), handles.GetData(), FALSE, INFINITE);

		if(idx == (WAIT_OBJECT_0 + 0)) // m_frd.hEndThreadEvent
		{
			break;
		}
		if(idx == (WAIT_OBJECT_0 + 1)) // m_frd.hRefreshEvent
		{
			SetupFRD(paths, handles);
		}
		else if(idx >= (WAIT_OBJECT_0 + 2) && idx < (WAIT_OBJECT_0 + handles.GetSize()))
		{
			bool fLocked = true;
			IsSubtitleReloaderLocked(&fLocked);
			if(fLocked) continue;

			if(FindNextChangeNotification(handles[idx - WAIT_OBJECT_0]) == FALSE) 
				break;

			int j = 0;

			POSITION pos = m_frd.files.GetHeadPosition();
			for(int i = 0; pos && j == 0; i++)
			{
				CString fn = m_frd.files.GetNext(pos);

				CFileStatus status;
				if(CFile::GetStatus(fn, status) && m_frd.mtime[i] != status.m_mtime) 
				{
					for(j = 0; j < 10; j++)
					{
						if(FILE* f = _tfopen(fn, _T("rb+"))) 
						{
							fclose(f);
							j = 0;
							break;
						}
						else
						{
							Sleep(100);
							j++;
						}
					}
				}
			}

			if(j > 0)
			{
				SetupFRD(paths, handles);
			}
			else
			{
				Sleep(500);

				POSITION pos = m_frd.files.GetHeadPosition();
				for(int i = 0; pos; i++)
				{
					CFileStatus status;
					if(CFile::GetStatus(m_frd.files.GetNext(pos), status) 
						&& m_frd.mtime[i] != status.m_mtime) 
					{
						Open();
						SetupFRD(paths, handles);
						break;
					}
				}
			}
		}
		else 
		{
			break;
		}
	}

	for(int i = 2; i < handles.GetSize(); i++)
	{
		FindCloseChangeNotification(handles[i]);
	}

	return 0;
}
