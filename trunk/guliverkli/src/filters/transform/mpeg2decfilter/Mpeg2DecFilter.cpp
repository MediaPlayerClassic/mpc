/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  Mpeg2DecFilter.ax is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  Mpeg2DecFilter.ax is distributed in the hope that it will be useful,
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
#include <atlbase.h>
#include <ks.h>
#include <ksmedia.h>
#include "libmpeg2.h"
#include "Mpeg2DecFilter.h"

#include "..\..\..\DSUtil\DSUtil.h"
#include "..\..\..\DSUtil\MediaTypes.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"
#include "..\..\..\..\include\matroska\matroska.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_IYUV},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn)/sizeof(sudPinTypesIn[0]), // Number of types
      sudPinTypesIn		// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
      sudPinTypesOut		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMpeg2DecFilter), L"Mpeg2Dec Filter", 0x40000002/*MERIT_PREFERRED*//*MERIT_DO_NOT_USE*/, sizeof(sudpPins)/sizeof(sudpPins[0]), sudpPins},
};

CFactoryTemplate g_Templates[] =
{
    {L"Mpeg2Dec Filter", &__uuidof(CMpeg2DecFilter), CMpeg2DecFilter::CreateInstance, NULL, &sudFilter[0]},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//

#include "..\..\..\..\include\detours\detours.h"

DETOUR_TRAMPOLINE(BOOL WINAPI Real_IsDebuggerPresent(), IsDebuggerPresent);
BOOL WINAPI Mine_IsDebuggerPresent()
{
	TRACE(_T("Oops, somebody was trying to be naughty! (called IsDebuggerPresent)\n")); 
	return FALSE;
}

DETOUR_TRAMPOLINE(LONG WINAPI Real_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam), ChangeDisplaySettingsExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam), ChangeDisplaySettingsExW);
LONG WINAPI Mine_ChangeDisplaySettingsEx(LONG ret, DWORD dwFlags, LPVOID lParam)
{
	if(dwFlags&CDS_VIDEOPARAMETERS)
	{
		VIDEOPARAMETERS* vp = (VIDEOPARAMETERS*)lParam;

		if(vp->Guid == GUIDFromCString(_T("{02C62061-1097-11d1-920F-00A024DF156E}"))
		&& (vp->dwFlags&VP_FLAGS_COPYPROTECT))
		{
			if(vp->dwCommand == VP_COMMAND_GET)
			{
				if((vp->dwTVStandard&VP_TV_STANDARD_WIN_VGA) && vp->dwTVStandard != VP_TV_STANDARD_WIN_VGA)
				{
					TRACE(_T("Ooops, tv-out enabled? macrovision checks suck..."));
					vp->dwTVStandard = VP_TV_STANDARD_WIN_VGA;
				}
			}
			else if(vp->dwCommand == VP_COMMAND_SET)
			{
				TRACE(_T("Ooops, as I already told ya, no need for any macrovision bs here"));
				return 0;
			}
		}
	}

	return ret;
}
LONG WINAPI Mine_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(Real_ChangeDisplaySettingsExA(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), dwFlags, lParam);
}
LONG WINAPI Mine_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(Real_ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), dwFlags, lParam);
}

bool fDetourInited = false;

//

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if(!fDetourInited)
	{
		DetourFunctionWithTrampoline((PBYTE)Real_IsDebuggerPresent, (PBYTE)Mine_IsDebuggerPresent);
		DetourFunctionWithTrampoline((PBYTE)Real_ChangeDisplaySettingsExA, (PBYTE)Mine_ChangeDisplaySettingsExA);
		DetourFunctionWithTrampoline((PBYTE)Real_ChangeDisplaySettingsExW, (PBYTE)Mine_ChangeDisplaySettingsExW);
		fDetourInited = true;
	}

	return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CMpeg2DecFilter
//

CUnknown* WINAPI CMpeg2DecFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMpeg2DecFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CMpeg2DecFilter::CMpeg2DecFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CMpeg2DecFilter"), lpunk, __uuidof(this))
	, m_fWaitForKeyFrame(true)
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CMpeg2DecInputPin(this, phr, L"Video"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CMpeg2DecOutputPin(this, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}

	if(!(m_pSubpicInput = new CSubpicInputPin(this, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pClosedCaptionOutput = new CClosedCaptionOutputPin(this, m_pLock, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	SetDeinterlaceMethod(DIAuto);
	SetBrightness(0.0);
	SetContrast(1.0);
	SetHue(0.0);
	SetSaturation(1.0);
	EnableForcedSubtitles(true);
	EnablePlanarYUV(true);

	m_rate.Rate = 10000;
	m_rate.StartTime = 0;
}

CMpeg2DecFilter::~CMpeg2DecFilter()
{
	delete m_pSubpicInput;
	delete m_pClosedCaptionOutput;
}

STDMETHODIMP CMpeg2DecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMpeg2DecFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CMpeg2DecFilter::GetPinCount()
{
	return 4;
}

CBasePin* CMpeg2DecFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_pInput;
	case 1: return m_pOutput;
	case 2: return m_pSubpicInput;
	case 3: return m_pClosedCaptionOutput;
	}
	return NULL;
}

HRESULT CMpeg2DecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_pClosedCaptionOutput->EndOfStream();
	return __super::EndOfStream();
}

HRESULT CMpeg2DecFilter::BeginFlush()
{
	m_pClosedCaptionOutput->DeliverBeginFlush();
	return __super::BeginFlush();
}

HRESULT CMpeg2DecFilter::EndFlush()
{
	m_pClosedCaptionOutput->DeliverEndFlush();
	return __super::EndFlush();
}

HRESULT CMpeg2DecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_pClosedCaptionOutput->DeliverNewSegment(tStart, tStop, dRate);
	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMpeg2DecFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);

		ResetMpeg2Decoder();
	}

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;

	long len = pIn->GetActualDataLength();

	if(len <= 0) return S_OK;

	if(*(DWORD*)pDataIn == 0xBA010000) // MEDIATYPE_*_PACK
	{
		len -= 14; pDataIn += 14;
		if(int stuffing = (pDataIn[-1]&7)) {len -= stuffing; pDataIn += stuffing;}
	}

	if(len <= 0) return S_OK;

	if(*(DWORD*)pDataIn == 0xBB010000)
	{
		len -= 4; pDataIn += 4;
		int hdrlen = ((pDataIn[0]<<8)|pDataIn[1]) + 2;
		len -= hdrlen; pDataIn += hdrlen;
	}

	if(len <= 0) return S_OK;

	if((*(DWORD*)pDataIn&0xE0FFFFFF) == 0xE0010000)
	{
		if(m_pInput->CurrentMediaType().subtype == MEDIASUBTYPE_MPEG1Packet)
		{
			len -= 4+2+12; pDataIn += 4+2+12;
		}
		else if(m_pInput->CurrentMediaType().subtype == MEDIASUBTYPE_MPEG2_VIDEO)
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1; pDataIn += *pDataIn+1;
		}
	}

	if(len <= 0) return S_OK;

	if(pIn->IsDiscontinuity() == S_OK)
	{
		ResetMpeg2Decoder();
	}

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);
	if(FAILED(hr)) rtStart = rtStop = _I64_MIN;

	while(len >= 0)
	{
		mpeg2_state_t state = m_dec->mpeg2_parse();

		__asm emms; // this one is missing somewhere in the precompiled mmx obj files

		switch(state)
		{
		case STATE_BUFFER:
			if(len == 0) len = -1;
			else {m_dec->mpeg2_buffer(pDataIn, pDataIn + len); len = 0;}
			break;
		case STATE_INVALID:
			TRACE(_T("STATE_INVALID\n"));
//			if(m_fWaitForKeyFrame)
//				ResetMpeg2Decoder();
			break;
		case STATE_GOP:
			TRACE(_T("STATE_GOP\n"));
			if(m_dec->m_info.m_user_data_len > 4 && *(DWORD*)m_dec->m_info.m_user_data == 0xf8014343
			&& m_pClosedCaptionOutput->IsConnected())
			{
				CComPtr<IMediaSample> pSample;
				m_pClosedCaptionOutput->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
				BYTE* pData = NULL;
				pSample->GetPointer(&pData);
				*(DWORD*)pData = 0xb2010000;
				memcpy(pData + 4, m_dec->m_info.m_user_data, m_dec->m_info.m_user_data_len);
				pSample->SetActualDataLength(m_dec->m_info.m_user_data_len + 4);
				m_pClosedCaptionOutput->Deliver(pSample);
			}
			break;
		case STATE_SEQUENCE:
			TRACE(_T("STATE_SEQUENCE\n"));
			m_AvgTimePerFrame = 10i64 * m_dec->m_info.m_sequence->frame_period / 27;
			if(m_AvgTimePerFrame == 0) m_AvgTimePerFrame = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->AvgTimePerFrame;
			break;
		case STATE_PICTURE:
/*			{
			TCHAR frametype[] = {'?','I', 'P', 'B', 'D'};
			TRACE(_T("STATE_PICTURE %010I64d [%c]\n"), rtStart, frametype[m_dec->m_picture->flags&PIC_MASK_CODING_TYPE]);
			}
*/			m_dec->m_picture->rtStart = rtStart;
			rtStart = _I64_MIN;
			m_dec->m_picture->fDelivered = false;
			break;
		case STATE_SLICE:
		case STATE_END:
			{
				mpeg2_picture_t* picture = m_dec->m_info.m_display_picture;
				mpeg2_picture_t* picture_2nd = m_dec->m_info.m_display_picture_2nd;
				mpeg2_fbuf_t* fbuf = m_dec->m_info.m_display_fbuf;

				if(picture && fbuf)
				{
					ASSERT(!picture->fDelivered);

					// start - end

					m_fb.rtStart = picture->rtStart;
					if(m_fb.rtStart == _I64_MIN) m_fb.rtStart = m_fb.rtStop;
					m_fb.rtStop = m_fb.rtStart + m_AvgTimePerFrame * picture->nb_fields / (picture_2nd ? 1 : 2);

					// flags

					if(!(m_dec->m_info.m_sequence->flags&SEQ_FLAG_PROGRESSIVE_SEQUENCE)
					&& (picture->flags&PIC_FLAG_PROGRESSIVE_FRAME))
					{
						if(!m_fFilm
						&& (picture->flags&PIC_FLAG_REPEAT_FIRST_FIELD)
						&& !(m_fb.flags&PIC_FLAG_REPEAT_FIRST_FIELD))
						{
							TRACE(_T("m_fFilm = true\n"));
							m_fFilm = true;
						}
						else if(m_fFilm
						&& !(picture->flags&PIC_FLAG_REPEAT_FIRST_FIELD)
						&& !(m_fb.flags&PIC_FLAG_REPEAT_FIRST_FIELD))
						{
							TRACE(_T("m_fFilm = false\n"));
							m_fFilm = false;
						}
					}

					m_fb.flags = picture->flags;

ASSERT(!(m_fb.flags&PIC_FLAG_SKIP));

					// frame buffer

					int w = m_dec->m_info.m_sequence->picture_width;
					int h = m_dec->m_info.m_sequence->picture_height;
					int pitch = m_dec->m_info.m_sequence->width;

					if(m_fb.w != w || m_fb.h != h || m_fb.pitch != pitch)
						m_fb.alloc(w, h, pitch);

					// deinterlace

					ditype di = GetDeinterlaceMethod();

					if(di == DIAuto || di != DIWeave && di != DIBlend)
					{
						if(!!(m_dec->m_info.m_sequence->flags&SEQ_FLAG_PROGRESSIVE_SEQUENCE))
							di = DIWeave; // hurray!
						else if(m_fFilm)
							di = DIWeave; // we are lucky
						else if(!(m_fb.flags&PIC_FLAG_PROGRESSIVE_FRAME))
							di = DIBlend; // ok, clear thing
						else
							// big trouble here, the progressive_frame bit is not reliable :'(
							// frames without temporal field diffs can be only detected when ntsc 
							// uses the repeat field flag (signaled with m_fFilm), if it's not set 
							// or we have pal then we might end up blending the fields unnecessarily...
							di = DIBlend;
							// TODO: find out if the pic is really interlaced by analysing it
					}

					if(di == DIWeave)
					{
						memcpy_accel(m_fb.buf[0], fbuf->buf[0], pitch*h);
						memcpy_accel(m_fb.buf[1], fbuf->buf[1], pitch*h/4);
						memcpy_accel(m_fb.buf[2], fbuf->buf[2], pitch*h/4);
					}
					else if(di == DIBlend)
					{
						DeinterlaceBlend(m_fb.buf[0], fbuf->buf[0], w, h, pitch);
						DeinterlaceBlend(m_fb.buf[1], fbuf->buf[1], w/2, h/2, pitch/2);
						DeinterlaceBlend(m_fb.buf[2], fbuf->buf[2], w/2, h/2, pitch/2);
					}

					// postproc

					ApplyBrContHueSat(m_fb.buf[0], m_fb.buf[1], m_fb.buf[2], w, h, pitch);
/*
					// TODO: add all kinds of nice postprocessing here :P

					// simple lame deblocking, don't use it
					{
						int w = m_fb.w, h = m_fb.h, p = m_fb.pw;

						for(int y = 5; y <= h-6; y += 8)
						{
							BYTE* a[6];
							for(int j = 0; j < 6; j++)
								a[j] = m_fb.buf[0] + p*(y+j);

							for(int x = 0; x < w; x++)
							{
								int dif = a[3][x] - a[2][x];
								if(abs(dif) > 16) continue;
								a[0][x] = a[0][x] + (dif>>3);
								a[1][x] = a[1][x] + (dif>>2);
								a[2][x] = a[2][x] + (dif>>1);
								a[3][x] = a[3][x] - (dif>>1);
								a[4][x] = a[4][x] - (dif>>2);
								a[5][x] = a[5][x] - (dif>>3);
							}
						}
					}

					{
						int w = m_fb.w, h = m_fb.h, p = m_fb.pw;

						for(int x = 5; x <= w-6; x += 8)
						{
							BYTE* a = m_fb.buf[0] + x;

							for(int y = 0; y < h; y++, a+=p)
							{
								int dif = a[3] - a[2];
								if(abs(dif) > 16) continue;
								a[0] = a[0] + (dif>>3);
								a[1] = a[1] + (dif>>2);
								a[2] = a[2] + (dif>>1);
								a[3] = a[3] - (dif>>1);
								a[4] = a[4] - (dif>>2);
								a[5] = a[5] - (dif>>3);
							}
						}
					}
*/
					//

					picture->fDelivered = true;

					if(FAILED(hr = Deliver(false)))
						return hr;
				}
			}
			break;
		default:
		    break;
		}
    }

	return S_OK;
}

HRESULT CMpeg2DecFilter::Deliver(bool fRepeatLast)
{
	CAutoLock cAutoLock(&m_csReceive);

	if((m_fb.flags&PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I)
		m_fWaitForKeyFrame = false;

	TCHAR frametype[] = {'?','I', 'P', 'B', 'D'};
//	TRACE(_T("%010I64d - %010I64d [%c] [prsq %d prfr %d tff %d rff %d nb_fields %d ref %d] (%dx%d/%dx%d)\n"), 
	TRACE(_T("%010I64d - %010I64d [%c] [prsq %d prfr %d tff %d rff %d] (%dx%d %d) (preroll %d)\n"), 
		m_fb.rtStart, m_fb.rtStop,
		frametype[m_fb.flags&PIC_MASK_CODING_TYPE],
		!!(m_dec->m_info.m_sequence->flags&SEQ_FLAG_PROGRESSIVE_SEQUENCE),
		!!(m_fb.flags&PIC_FLAG_PROGRESSIVE_FRAME),
		!!(m_fb.flags&PIC_FLAG_TOP_FIELD_FIRST),
		!!(m_fb.flags&PIC_FLAG_REPEAT_FIRST_FIELD),
//		m_dec->m_info.m_display_picture->nb_fields,
//		m_dec->m_info.m_display_picture->temporal_reference,
		m_fb.w, m_fb.h, m_fb.pitch,
		!!(m_fb.rtStart < 0 || m_fWaitForKeyFrame));

	if(m_fb.rtStart < 0 || m_fWaitForKeyFrame)
		return S_OK;

	HRESULT hr;

	if(FAILED(hr = ReconnectOutput(m_fb.w, m_fb.h)))
		return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))
	|| FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	{
		CMpeg2DecInputPin* pPin = (CMpeg2DecInputPin*)m_pInput;
		CAutoLock cAutoLock(&pPin->m_csRateLock);
		if(m_rate.Rate != pPin->m_ratechange.Rate)
		{
			m_rate.Rate = pPin->m_ratechange.Rate;
			m_rate.StartTime = m_fb.rtStart;
		}
	}

	REFERENCE_TIME rtStart = m_fb.rtStart;
	REFERENCE_TIME rtStop = m_fb.rtStop;
	rtStart = m_rate.StartTime + (rtStart - m_rate.StartTime) * m_rate.Rate / 10000;
	rtStop = m_rate.StartTime + (rtStop - m_rate.StartTime) * m_rate.Rate / 10000;

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetDiscontinuity(FALSE);
	pOut->SetSyncPoint(TRUE);

	// FIXME: hell knows why but without this the overlay mixer starts very skippy
	// (don't enable this for other renderers, the old for example will go crazy if you do)
	if(GetCLSID(GetFilterFromPin(m_pOutput->GetConnected())) == CLSID_OverlayMixer)
		pOut->SetDiscontinuity(TRUE);

	BYTE** buf = &m_fb.buf[0];

	if(m_pSubpicInput->HasAnythingToRender(m_fb.rtStart))
	{
		memcpy_accel(m_fb.buf[3], m_fb.buf[0], m_fb.pitch*m_fb.h);
		memcpy_accel(m_fb.buf[4], m_fb.buf[1], m_fb.pitch*m_fb.h/4);
		memcpy_accel(m_fb.buf[5], m_fb.buf[2], m_fb.pitch*m_fb.h/4);

		buf = &m_fb.buf[3];

		m_pSubpicInput->RenderSubpics(m_fb.rtStart, buf, m_fb.pitch, m_fb.h);
	}

	Copy(pDataOut, buf, m_fb.w, m_fb.h, m_fb.pitch);

	if(FAILED(hr = m_pOutput->Deliver(pOut)))
		return hr;

	return S_OK;
}

void CMpeg2DecFilter::Copy(BYTE* pOut, BYTE** ppIn, DWORD w, DWORD h, DWORD pitchIn)
{
	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	BYTE* pIn = ppIn[0];
	BYTE* pInU = ppIn[1];
	BYTE* pInV = ppIn[2];

	w = (w+7)&~7;
	ASSERT(w <= pitchIn);

	if(bihOut.biCompression == '2YUY')
	{
		BitBltFromI420ToYUY2(pOut, bihOut.biWidth*2, pIn, pInU, pInV, w, h, pitchIn);
	}
	else if(bihOut.biCompression == '21VY' || bihOut.biCompression == 'I420' || bihOut.biCompression == 'VUYI')
	{
		DWORD pitchOut = bihOut.biWidth;

		for(DWORD y = 0; y < h; y++, pIn += pitchIn, pOut += pitchOut)
			memcpy_accel(pOut, pIn, min(pitchIn, pitchOut));

		pitchIn >>= 1;
		pitchOut >>= 1;

		pIn = bihOut.biCompression == '21VY' ? pInV : pInU;

		for(DWORD y = 0; y < h; y+=2, pIn += pitchIn, pOut += pitchOut)
			memcpy_accel(pOut, pIn, min(pitchIn, pitchOut));

		pIn = bihOut.biCompression == '21VY' ? pInU : pInV;

		for(DWORD y = 0; y < h; y+=2, pIn += pitchIn, pOut += pitchOut)
			memcpy_accel(pOut, pIn, min(pitchIn, pitchOut));
	}
	else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
	{
		int pitchOut = bihOut.biWidth*bihOut.biBitCount>>3;

		if(bihOut.biHeight > 0)
		{
			pOut += pitchOut*(h-1);
			pitchOut = -pitchOut;
		}

		if(!BitBltFromI420ToRGB(pOut, pitchOut, pIn, pInU, pInV, w, h, bihOut.biBitCount, pitchIn))
		{
			for(DWORD y = 0; y < h; y++, pIn += pitchIn, pOut += pitchOut)
				memset(pOut, 0, pitchOut);
		}
	}
}

void CMpeg2DecFilter::ResetMpeg2Decoder()
{
	CAutoLock cAutoLock(&m_csReceive);
TRACE(_T("ResetMpeg2Decoder()\n"));

	for(int i = 0; i < sizeof(m_dec->m_pictures)/sizeof(m_dec->m_pictures[0]); i++)
	{
		m_dec->m_pictures[i].rtStart = m_dec->m_pictures[i].rtStop = _I64_MIN+1;
		m_dec->m_pictures[i].fDelivered = false;
		m_dec->m_pictures[i].flags &= ~PIC_MASK_CODING_TYPE;
	}

	CMediaType& mt = m_pInput->CurrentMediaType();

	BYTE* pSequenceHeader = NULL;
	DWORD cbSequenceHeader = 0;

	if(mt.formattype == FORMAT_MPEGVideo)
	{
		pSequenceHeader = ((MPEG1VIDEOINFO*)mt.Format())->bSequenceHeader;
		cbSequenceHeader = ((MPEG1VIDEOINFO*)mt.Format())->cbSequenceHeader;
	}
	else if(mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		pSequenceHeader = (BYTE*)((MPEG2VIDEOINFO*)mt.Format())->dwSequenceHeader;
		cbSequenceHeader = ((MPEG2VIDEOINFO*)mt.Format())->cbSequenceHeader;
	}

	m_dec->mpeg2_close();
	m_dec->mpeg2_init();

	m_dec->mpeg2_buffer(pSequenceHeader, pSequenceHeader + cbSequenceHeader);

	m_fWaitForKeyFrame = true;

	m_fFilm = false;
	m_fb.flags = 0;
}

HRESULT CMpeg2DecFilter::ReconnectOutput(int w, int h)
{
	CMediaType& mt = m_pOutput->CurrentMediaType();
	DWORD wout = 0, hout = 0, arxout = 0, aryout = 0;
	ExtractDim(&mt, wout, hout, arxout, aryout);

	if(w > m_win || h != m_hin)
	{
		TRACE(_T("CMpeg2DecFilter (ERROR): input and real video dimensions do not match (%dx%d %dx%d)"), w, h, m_win, m_hin);
		ASSERT(0);
		return E_FAIL;
	}

	bool fForceReconnection = false;
	if(w != m_win || h != m_hin)
	{
		fForceReconnection = true;
		m_win = w;
		m_hin = h;
	}

	HRESULT hr = S_OK;

	if(fForceReconnection || m_win > wout || m_hin != hout || m_arxin != arxout && arxout || m_aryin != aryout && aryout)
	{
		BITMAPINFOHEADER* bmi = NULL;

		if(mt.formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.Format();
			SetRect(&vih->rcSource, 0, 0, m_win, m_hin);
			SetRect(&vih->rcTarget, 0, 0, m_win, m_hin);
			bmi = &vih->bmiHeader;
			bmi->biXPelsPerMeter = m_win * m_aryin;
			bmi->biYPelsPerMeter = m_hin * m_arxin;
		}
		else if(mt.formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.Format();
			SetRect(&vih->rcSource, 0, 0, m_win, m_hin);
			SetRect(&vih->rcTarget, 0, 0, m_win, m_hin);
			bmi = &vih->bmiHeader;
			vih->dwPictAspectRatioX = m_arxin;
			vih->dwPictAspectRatioY = m_aryin;
		}

		if(fForceReconnection || m_win > wout || m_hin != hout)
		{
			bmi->biWidth = m_win;
			bmi->biHeight = m_hin;
			bmi->biSizeImage = m_win*m_hin*bmi->biBitCount>>3;

			if(FAILED(hr = m_pOutput->GetConnected()->ReceiveConnection(m_pOutput, &mt)))
				return hr;

			// some renderers don't send this
			NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_win, m_hin), 0);
		}
	}

	return hr;
}

HRESULT CMpeg2DecFilter::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	if(dir == PINDIR_OUTPUT)
	{
		if(GetCLSID(GetFilterFromPin(m_pInput)) == CLSID_DVDNavigator)
		{
			// one of these needed for dynamic format changes

			CLSID clsid = GetCLSID(GetFilterFromPin(pPin));
			if(clsid != CLSID_OverlayMixer
			/*&& clsid != CLSID_OverlayMixer2*/
			&& clsid != CLSID_VideoMixingRenderer 
			&& clsid != CLSID_VideoMixingRenderer9)
				return E_FAIL;
		}
	}

	return __super::CheckConnect(dir, pPin);
}

HRESULT CMpeg2DecFilter::CheckInputType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG1Packet
			|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG1Payload)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Video && (mtOut->subtype == MEDIASUBTYPE_YV12 && IsPlanarYUVEnabled()
												|| mtOut->subtype == MEDIASUBTYPE_I420 && IsPlanarYUVEnabled()
												|| mtOut->subtype == MEDIASUBTYPE_IYUV && IsPlanarYUVEnabled()
												|| mtOut->subtype == MEDIASUBTYPE_YUY2
												|| mtOut->subtype == MEDIASUBTYPE_ARGB32
												|| mtOut->subtype == MEDIASUBTYPE_RGB32
												|| mtOut->subtype == MEDIASUBTYPE_RGB24
												|| mtOut->subtype == MEDIASUBTYPE_RGB565
												/*|| mtOut->subtype == MEDIASUBTYPE_RGB555*/)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::CheckOutputMediaType(const CMediaType& mtOut)
{
	DWORD wout = 0, hout = 0, arxout = 0, aryout = 0;
	return ExtractDim(&mtOut, wout, hout, arxout, aryout)
		&& m_hin == abs((int)hout)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bih);

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = bih.biSizeImage;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

HRESULT CMpeg2DecFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	struct {const GUID* subtype; WORD biPlanes, biBitCount; DWORD biCompression;} fmts[] =
	{
		{&MEDIASUBTYPE_YV12, 3, 12, '21VY'},
		{&MEDIASUBTYPE_I420, 3, 12, '024I'},
		{&MEDIASUBTYPE_IYUV, 3, 12, 'VUYI'},
		{&MEDIASUBTYPE_YUY2, 1, 16, '2YUY'},
		{&MEDIASUBTYPE_ARGB32, 1, 32, BI_RGB},
		{&MEDIASUBTYPE_RGB32, 1, 32, BI_RGB},
		{&MEDIASUBTYPE_RGB24, 1, 24, BI_RGB},
		{&MEDIASUBTYPE_RGB565, 1, 16, BI_RGB},
		{&MEDIASUBTYPE_RGB555, 1, 16, BI_RGB},
		{&MEDIASUBTYPE_ARGB32, 1, 32, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB32, 1, 32, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB24, 1, 24, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB565, 1, 16, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB555, 1, 16, BI_BITFIELDS},
	};

	// this will make sure we won't connect to the old renderer in dvd mode
	// that renderer can't switch the format dynamically
	if(GetCLSID(GetFilterFromPin(m_pInput->GetConnected())) == CLSID_DVDNavigator)
		iPosition = iPosition*2;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= 2*sizeof(fmts)/sizeof(fmts[0])) return VFW_S_NO_MORE_ITEMS;

	CMediaType& mt = m_pInput->CurrentMediaType();

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = *fmts[iPosition/2].subtype;

	BITMAPINFOHEADER bihOut;
	memset(&bihOut, 0, sizeof(bihOut));
	bihOut.biSize = sizeof(bihOut);
	bihOut.biWidth = m_win;
	bihOut.biHeight = m_hin;
	bihOut.biPlanes = fmts[iPosition/2].biPlanes;
	bihOut.biBitCount = fmts[iPosition/2].biBitCount;
	bihOut.biCompression = fmts[iPosition/2].biCompression;
	bihOut.biSizeImage = m_win*m_hin*bihOut.biBitCount>>3;

	if(iPosition&1)
	{
		pmt->formattype = FORMAT_VideoInfo;
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memset(vih, 0, sizeof(VIDEOINFOHEADER));
		vih->bmiHeader = bihOut;
		vih->bmiHeader.biXPelsPerMeter = vih->bmiHeader.biWidth * m_aryin;
		vih->bmiHeader.biYPelsPerMeter = vih->bmiHeader.biHeight * m_arxin;
	}
	else
	{
		pmt->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih, 0, sizeof(VIDEOINFOHEADER2));
		vih->bmiHeader = bihOut;
		vih->dwPictAspectRatioX = m_arxin;
		vih->dwPictAspectRatioY = m_aryin;
	}

	// these fields have the same field offset in all four structs
	((VIDEOINFOHEADER*)pmt->Format())->AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitRate;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitErrorRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate;

	CorrectMediaType(pmt);

	return S_OK;
}

HRESULT CMpeg2DecFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	if(dir == PINDIR_INPUT)
	{
		m_win = m_hin = m_arxin = m_aryin = 0;
		ExtractDim(pmt, m_win, m_hin, m_arxin, m_aryin);
	}

	return __super::SetMediaType(dir, pmt);
}

HRESULT CMpeg2DecFilter::StartStreaming()
{
	HRESULT hr = __super::StartStreaming();
	if(FAILED(hr)) return hr;

	m_dec.Attach(new CMpeg2Dec());
	if(!m_dec) return E_OUTOFMEMORY;

	ResetMpeg2Decoder();

	return S_OK;
}

HRESULT CMpeg2DecFilter::StopStreaming()
{
	m_dec.Free();

	return __super::StopStreaming();
}

HRESULT CMpeg2DecFilter::AlterQuality(Quality q)
{
//	if(q.Late > 500*10000i64) m_fDropFrames = true;
//	if(q.Late <= 0) m_fDropFrames = false;
//	TRACE(_T("CMpeg2DecFilter::AlterQuality: Type=%d, Proportion=%d, Late=%I64d, TimeStamp=%I64d\n"), q.Type, q.Proportion, q.Late, q.TimeStamp);
	return S_OK;
}

// IMpeg2DecFilter

STDMETHODIMP CMpeg2DecFilter::SetDeinterlaceMethod(ditype di)
{
	CAutoLock cAutoLock(&m_csProps);
	m_di = di;
	return S_OK;
}

STDMETHODIMP_(ditype) CMpeg2DecFilter::GetDeinterlaceMethod()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_di;
}

void CMpeg2DecFilter::CalcBrCont(BYTE* YTbl, double bright, double cont)
{
	int Cont = (int)(cont * 512);
	int Bright = (int)bright;

	for(int i = 0; i < 256; i++)
	{
		int y = ((Cont * (i - 16)) >> 9) + Bright + 16;
		YTbl[i] = min(max(y, 0), 255);
//		YTbl[i] = min(max(y, 16), 235);
	}
}

void CMpeg2DecFilter::CalcHueSat(BYTE* UTbl, BYTE* VTbl, double hue, double sat)
{
	int Sat = (int)(sat * 512);
	double Hue = (hue * 3.1415926) / 180.0;
	int Sin = (int)(sin(Hue) * 4096);
	int Cos = (int)(cos(Hue) * 4096);

	for(int y = 0; y < 256; y++)
	{
		for(int x = 0; x < 256; x++)
		{
			int u = x - 128; 
			int v = y - 128;
			int ux = (u * Cos + v * Sin) >> 12;
			v = (v * Cos - u * Sin) >> 12;
			u = ((ux * Sat) >> 9) + 128;
			v = ((v * Sat) >> 9) + 128;
			u = min(max(u, 16), 235);
			v = min(max(v, 16), 235);
			UTbl[(y << 8) | x] = u;
			VTbl[(y << 8) | x] = v;
		}
	}
}

void CMpeg2DecFilter::ApplyBrContHueSat(BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int pitch)
{
	CAutoLock cAutoLock(&m_csProps);

	double EPSILON = 1e-4;

	if(fabs(m_bright-0.0) > EPSILON || fabs(m_cont-1.0) > EPSILON)
	{
		for(int size = pitch*h; size > 0; size--)
		{
			*srcy++ = m_YTbl[*srcy];
		}
	}

	pitch /= 2;
	w /= 2;
	h /= 2;

	if(fabs(m_hue-0.0) > EPSILON || fabs(m_sat-1.0) > EPSILON)
	{
		for(int size = pitch*h; size > 0; size--)
		{
			WORD uv = (*srcv<<8)|*srcu;
			*srcu++ = m_UTbl[uv];
			*srcv++ = m_VTbl[uv];
		}
	}
}

STDMETHODIMP CMpeg2DecFilter::SetBrightness(double bright)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcBrCont(m_YTbl, m_bright = bright, m_cont);
	return S_OK;
}

STDMETHODIMP CMpeg2DecFilter::SetContrast(double cont)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcBrCont(m_YTbl, m_bright, m_cont = cont);
	return S_OK;
}

STDMETHODIMP CMpeg2DecFilter::SetHue(double hue)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcHueSat(m_UTbl, m_VTbl, m_hue = hue, m_sat);
	return S_OK;
}

STDMETHODIMP CMpeg2DecFilter::SetSaturation(double sat)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcHueSat(m_UTbl, m_VTbl, m_hue, m_sat = sat);
	return S_OK;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetBrightness()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bright;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetContrast()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_cont;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetHue()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_hue;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetSaturation()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_sat;
}

STDMETHODIMP CMpeg2DecFilter::EnableForcedSubtitles(bool fEnable)
{
	CAutoLock cAutoLock(&m_csProps);
	m_fForcedSubs = fEnable;
	return S_OK;
}

STDMETHODIMP_(bool) CMpeg2DecFilter::IsForcedSubtitlesEnabled()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fForcedSubs;
}

STDMETHODIMP CMpeg2DecFilter::EnablePlanarYUV(bool fEnable)
{
	CAutoLock cAutoLock(&m_csProps);
	m_fPlanarYUV = fEnable;
	return S_OK;
}

STDMETHODIMP_(bool) CMpeg2DecFilter::IsPlanarYUVEnabled()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fPlanarYUV;
}

//
// CMpeg2DecInputPin
//

CMpeg2DecInputPin::CMpeg2DecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMpeg2DecInputPin"), pFilter, phr, pName)
{
	m_CorrectTS = 0;
	m_ratechange.Rate = 10000;
	m_ratechange.StartTime = _I64_MAX;
}

// IKsPropertySet

STDMETHODIMP CMpeg2DecInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
		return __super::Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	switch(Id)
	{
	case AM_RATE_SimpleRateChange:
		{
			AM_SimpleRateChange* p = (AM_SimpleRateChange*)pPropertyData;
			if(!m_CorrectTS) return E_PROP_ID_UNSUPPORTED;
			CAutoLock cAutoLock(&m_csRateLock);
			m_ratechange = *p;
			DbgLog((LOG_TRACE, 0, _T("StartTime=%I64d, Rate=%d"), p->StartTime, p->Rate));
		}
		break;
	case AM_RATE_UseRateVersion:
		{
			WORD* p = (WORD*)pPropertyData;
			if(*p > 0x0101) return E_PROP_ID_UNSUPPORTED;
		}
		break;
	case AM_RATE_CorrectTS:
		{
			LONG* p = (LONG*)pPropertyData;
			m_CorrectTS = *p;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	switch(Id)
	{
	case AM_RATE_ChangeRate:
		{
			AM_DVD_ChangeRate* p = (AM_DVD_ChangeRate*)pPropertyData;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
*/
	return S_OK;
}

STDMETHODIMP CMpeg2DecInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
		return __super::Get(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength, pBytesReturned);

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	switch(Id)
	{
	case AM_RATE_SimpleRateChange:
		{
			AM_SimpleRateChange* p = (AM_SimpleRateChange*)pPropertyData;
			return E_PROP_ID_UNSUPPORTED;
		}
		break;
	case AM_RATE_MaxFullDataRate:
		{
			AM_MaxFullDataRate* p = (AM_MaxFullDataRate*)pPropertyData;
			*p = 8*10000;
			*pBytesReturned = sizeof(AM_MaxFullDataRate);
		}
		break;
	case AM_RATE_QueryFullFrameRate:
		{
			AM_QueryRate* p = (AM_QueryRate*)pPropertyData;
			p->lMaxForwardFullFrame = 8*10000;
			p->lMaxReverseFullFrame = 8*10000;
			*pBytesReturned = sizeof(AM_QueryRate);
		}
		break;
	case AM_RATE_QueryLastRateSegPTS:
		{
			REFERENCE_TIME* p = (REFERENCE_TIME*)pPropertyData;
			return E_PROP_ID_UNSUPPORTED;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	switch(Id)
	{
	case AM_RATE_FullDataRateMax:
		{
			AM_MaxFullDataRate* p = (AM_MaxFullDataRate*)pPropertyData;
		}
		break;
	case AM_RATE_ReverseDecode:
		{
			LONG* p = (LONG*)pPropertyData;
		}
		break;
	case AM_RATE_DecoderPosition:
		{
			AM_DVD_DecoderPosition* p = (AM_DVD_DecoderPosition*)pPropertyData;
		}
		break;
	case AM_RATE_DecoderVersion:
		{
			LONG* p = (LONG*)pPropertyData;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
*/
	return S_OK;
}

STDMETHODIMP CMpeg2DecInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
		return __super::QuerySupported(PropSet, Id, pTypeSupport);

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	switch(Id)
	{
	case AM_RATE_SimpleRateChange:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_RATE_MaxFullDataRate:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_UseRateVersion:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_RATE_QueryFullFrameRate:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_QueryLastRateSegPTS:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_CorrectTS:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	switch(Id)
	{
	case AM_RATE_ChangeRate:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_RATE_FullDataRateMax:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_ReverseDecode:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_DecoderPosition:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_DecoderVersion:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
*/
	return S_OK;
}

//
// CSubpicInputPin
//

#define PTS2RT(pts) (10000i64*pts/90)

CSubpicInputPin::CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr) 
	: CMpeg2DecInputPin(pFilter, phr, L"SubPicture")
	, m_spon(TRUE)
{
}

HRESULT CSubpicInputPin::CheckMediaType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CSubpicInputPin::SetMediaType(const CMediaType* mtIn)
{
	return CBasePin::SetMediaType(mtIn);
}

bool CSubpicInputPin::DecodeSubpic(sp_t* sp, AM_PROPERTY_SPHLI& sphli, DWORD& offset1, DWORD& offset2)
{
	memset(&sphli, 0, sizeof(sphli));

	sp->fForced = false;

	BYTE* p = sp->pData.GetData();

	WORD packetsize = (p[0]<<8)|p[1];
	WORD datasize = (p[2]<<8)|p[3];

    if(packetsize > sp->pData.GetSize() || datasize > packetsize)
		return(false);

	int i, next = datasize;

	#define GetWORD (p[i]<<8)|p[i+1]; i += 2

	do
	{
		i = next;

		int pts = GetWORD;
		next = GetWORD;

		if(next > packetsize || next < datasize)
			return(false);

		for(bool fBreak = false; !fBreak; )
		{
			int len = 0;

			switch(p[i])
			{
				case 0x00: len = 0; break;
				case 0x01: len = 0; break;
				case 0x02: len = 0; break;
				case 0x03: len = 2; break;
				case 0x04: len = 2; break;
				case 0x05: len = 6; break;
				case 0x06: len = 4; break;
				default: len = 0; break;
			}

			if(i+len >= packetsize)
			{
				TRACE(_T("Warning: Wrong subpicture parameter block ending\n"));
				break;
			}

			switch(p[i++])
			{
				case 0x00: // forced start displaying
					sp->fForced = true;
					break;
				case 0x01: // normal start displaying
					sp->fForced = false;
					break;
				case 0x02: // stop displaying
					sp->rtStop = sp->rtStart + 1024*PTS2RT(pts);
					break;
				case 0x03:
					sphli.ColCon.emph2col = p[i]>>4;
					sphli.ColCon.emph1col = p[i]&0xf;
					sphli.ColCon.patcol = p[i+1]>>4;
					sphli.ColCon.backcol = p[i+1]&0xf;
					i += 2;
					break;
				case 0x04:
					sphli.ColCon.emph2con = p[i]>>4;
					sphli.ColCon.emph1con = p[i]&0xf;
					sphli.ColCon.patcon = p[i+1]>>4;
					sphli.ColCon.backcon = p[i+1]&0xf;
					i += 2;
					break;
				case 0x05:
					sphli.StartX = (p[i]<<4) + (p[i+1]>>4);
					sphli.StopX = ((p[i+1]&0x0f)<<8) + p[i+2]+1;
					sphli.StartY = (p[i+3]<<4) + (p[i+4]>>4);
					sphli.StopY = ((p[i+4]&0x0f)<<8) + p[i+5]+1;
					i += 6;
					break;
				case 0x06:
					offset1 = GetWORD;
					offset2 = GetWORD;
					break;
				case 0xff: // end of ctrlblk
					fBreak = true;
					continue;
				default: // skip this ctrlblk
					fBreak = true;
					break;
			}
		}
	}
	while(i <= next && i < packetsize);

	return(true);
}

static __inline BYTE GetNibble(BYTE* p, DWORD* offset, int& nField, int& fAligned)
{
	BYTE ret = (p[offset[nField]] >> (fAligned << 2)) & 0x0f;
	offset[nField] += 1-fAligned;
	fAligned = !fAligned;
    return ret;
}

static __inline void DrawPixel(BYTE** yuv, CPoint pt, int pitch, BYTE color, BYTE contrast, AM_DVD_YUV* sppal)
{
	if(contrast == 0) return;

	BYTE* p = &yuv[0][pt.y*pitch + pt.x];
//	*p = (*p*(15-contrast) + sppal[color].Y*contrast)>>4;
	*p -= (*p - sppal[color].Y) * contrast >> 4;

	if(pt.y&1) return; // since U/V is half res there is no need to overwrite the same line again

	pt.x = (pt.x + 1) / 2;
	pt.y = (pt.y /*+ 1*/) / 2; // only paint the upper field always, don't round it
	pitch /= 2;

	// U/V is exchanged? wierd but looks true when comparing the outputted colors from other decoders

	p = &yuv[1][pt.y*pitch + pt.x];
//	*p = (BYTE)(((((int)*p-0x80)*(15-contrast) + ((int)sppal[color].V-0x80)*contrast) >> 4) + 0x80);
	*p -= (*p - sppal[color].V) * contrast >> 4;

	p = &yuv[2][pt.y*pitch + pt.x];
//	*p = (BYTE)(((((int)*p-0x80)*(15-contrast) + ((int)sppal[color].U-0x80)*contrast) >> 4) + 0x80);
	*p -= (*p - sppal[color].U) * contrast >> 4;

	// Neighter of the blending formulas are accurate (">>4" should be "/15").
	// Even though the second one is a bit worse, since we are scaling the difference only,
	// the error is still not noticable.
}

static __inline void DrawPixels(BYTE** yuv, CPoint pt, int pitch, int len, BYTE color, 
								AM_PROPERTY_SPHLI& sphli, CRect& rc,
								AM_PROPERTY_SPHLI* sphli_hli, CRect& rchli,
								AM_DVD_YUV* sppal)
{
    if(pt.y < rc.top || pt.y >= rc.bottom) return;
	if(pt.x < rc.left) {len -= rc.left - pt.x; pt.x = rc.left;}
	if(pt.x + len > rc.right) len = rc.right - pt.x;
	if(len <= 0 || pt.x >= rc.right) return;

	BYTE contrast = 0, color_hli, contrast_hli = 0;

	if(sphli_hli) switch(color)
	{
	case 0: color_hli = sphli_hli->ColCon.backcol; contrast_hli = sphli_hli->ColCon.backcon; break;
	case 1: color_hli = sphli_hli->ColCon.patcol; contrast_hli = sphli_hli->ColCon.patcon; break;
	case 2: color_hli = sphli_hli->ColCon.emph1col; contrast_hli = sphli_hli->ColCon.emph1con; break;
	case 3: color_hli = sphli_hli->ColCon.emph2col; contrast_hli = sphli_hli->ColCon.emph2con; break;
	default: ASSERT(0); return;
	}
	
	switch(color)
	{
	case 0: color = sphli.ColCon.backcol; contrast = sphli.ColCon.backcon; break;
	case 1: color = sphli.ColCon.patcol; contrast = sphli.ColCon.patcon; break;
	case 2: color = sphli.ColCon.emph1col; contrast = sphli.ColCon.emph1con; break;
	case 3: color = sphli.ColCon.emph2col; contrast = sphli.ColCon.emph2con; break;
	default: ASSERT(0); return;
	}

	if(contrast == 0)
	{
		if(contrast_hli == 0)
			return;

		if(rchli.IsRectEmpty())
			return;

		if(pt.y < rchli.top || pt.y >= rchli.bottom 
		|| pt.x+len < rchli.left || pt.x >= rchli.right)
			return;
	}

	while(len-- > 0)
	{
		bool hli = sphli_hli && rchli.PtInRect(pt);
		DrawPixel(yuv, pt, pitch, hli ? color_hli : color, hli ? contrast_hli : contrast, sppal);
		pt.x++;
	}
}

void CSubpicInputPin::RenderSubpic(sp_t* sp, BYTE** p, int w, int h, AM_PROPERTY_SPHLI* sphli_hli)
{
	AM_PROPERTY_SPHLI sphli;
	DWORD offset[2];
	if(!DecodeSubpic(sp, sphli, offset[0], offset[1]))
		return;

	BYTE* pData = sp->pData.GetData();
	CPoint pt(sphli.StartX, sphli.StartY);
	CRect rc(pt, CPoint(sphli.StopX, sphli.StopY));
	CRect rchli(0,0,0,0);

	if(sphli_hli)
	{
		rchli = rc & CRect(CPoint(sphli_hli->StartX, sphli_hli->StartY), CPoint(sphli_hli->StopX, sphli_hli->StopY));
	}

	int nField = 0;
	int fAligned = 1;

	DWORD end[2] = {offset[1], (pData[2]<<8)|pData[3]};

	while((nField == 0 && offset[0] < end[0]) || (nField == 1 && offset[1] < end[1]))
	{
		DWORD code;

		if((code = GetNibble(pData, offset, nField, fAligned)) >= 0x4
		|| (code = (code << 4) | GetNibble(pData, offset, nField, fAligned)) >= 0x10
		|| (code = (code << 4) | GetNibble(pData, offset, nField, fAligned)) >= 0x40
		|| (code = (code << 4) | GetNibble(pData, offset, nField, fAligned)) >= 0x100)
		{
			DrawPixels(p, pt, w, code >> 2, (BYTE)(code & 3), sphli, rc, sphli_hli, rchli, m_sppal);
			if((pt.x += code >> 2) < rc.right) continue;
		}

		DrawPixels(p, pt, w, rc.right - pt.x, (BYTE)(code & 3), sphli, rc, sphli_hli, rchli, m_sppal);

		if(!fAligned) GetNibble(pData, offset, nField, fAligned); // align to byte

		pt.x = rc.left;
		pt.y++;
		nField = 1 - nField;
	}
}

bool CSubpicInputPin::HasAnythingToRender(REFERENCE_TIME rt)
{
	if(!IsConnected()) return(false);

	CAutoLock cAutoLock(&m_csReceive);

	POSITION pos = m_sps.GetHeadPosition();
	while(pos)
	{
		sp_t* sp = m_sps.GetNext(pos);
		if(sp->rtStart <= rt && rt < sp->rtStop && (/*sp->sphli ||*/ sp->fForced || m_spon))
		{
			return(true);
		}
	}

	return(false);
}

void CSubpicInputPin::RenderSubpics(REFERENCE_TIME rt, BYTE** p, int w, int h)
{
	CAutoLock cAutoLock(&m_csReceive);

	POSITION pos;

	// remove no longer needed things first
	pos = m_sps.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		sp_t* sp = m_sps.GetNext(pos);
		if(sp->rtStop <= rt) m_sps.RemoveAt(cur);
	}

	pos = m_sps.GetHeadPosition();
	while(pos)
	{
		sp_t* sp = m_sps.GetNext(pos);
		if(sp->rtStart <= rt && rt < sp->rtStop 
		&& (m_spon || sp->fForced && (((CMpeg2DecFilter*)m_pFilter)->IsForcedSubtitlesEnabled() || sp->sphli)))
		{
			RenderSubpic(sp, p, w, h, sp->sphli);
		}
	}
}

HRESULT CSubpicInputPin::Transform(IMediaSample* pSample)
{
	HRESULT hr;

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pSample->GetPointer(&pDataIn))) return hr;

	long len = pSample->GetActualDataLength();
	if(len <= 0) return S_FALSE;

	if(*(DWORD*)pDataIn == 0xBA010000) // MEDIATYPE_*_PACK
	{
		len -= 14; pDataIn += 14;
		if(int stuffing = (pDataIn[-1]&7)) {len -= stuffing; pDataIn += stuffing;}
	}

	if(len <= 0) return S_FALSE;

	if(*(DWORD*)pDataIn == 0xBD010000)
	{
		if(m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1+1; pDataIn += *pDataIn+1+1;
		}
	}

	if(len <= 0) return S_FALSE;

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME rtStart = 0, rtStop = 0;
	hr = pSample->GetTime(&rtStart, &rtStop);
	
	bool fRefresh = false;

	if(FAILED(hr))
	{
		if(!m_sps.IsEmpty())
		{
			sp_t* sp = m_sps.GetTail();
			sp->pData.SetSize(sp->pData.GetSize() + len);
			memcpy(sp->pData.GetData() + sp->pData.GetSize() - len, pDataIn, len);
		}
	}
	else
	{
		POSITION pos = m_sps.GetTailPosition();
		while(pos)
		{
			POSITION cur = pos;
			sp_t* sp = m_sps.GetPrev(pos);
			if(sp->rtStop == _I64_MAX)
			{
				sp->rtStop = rtStart;
				break;
			}
		}

		CAutoPtr<sp_t> p(new sp_t());
		p->rtStart = rtStart;
		p->rtStop = _I64_MAX;
		p->pData.SetSize(len);
		memcpy(p->pData.GetData(), pDataIn, len);

		if(m_sphli && p->rtStart == PTS2RT(m_sphli->StartPTM))
		{
			p->sphli = m_sphli;
			fRefresh = true;
		}

		m_sps.AddTail(p);
	}

	if(!m_sps.IsEmpty())
	{
		AM_PROPERTY_SPHLI sphli;
		DWORD offset[2];
		if(DecodeSubpic(m_sps.GetTail(), sphli, offset[0], offset[1]))
			DbgLog((LOG_TRACE, 0, _T("transform: %I64d - %I64d"), m_sps.GetTail()->rtStart/10000, m_sps.GetTail()->rtStop/10000));
	}

	if(fRefresh)
	{
//		((CMpeg2DecFilter*)m_pFilter)->Deliver(true);
	}

	return S_FALSE;
}

STDMETHODIMP CSubpicInputPin::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_sps.RemoveAll();
	return S_OK;
}

// IKsPropertySet

STDMETHODIMP CSubpicInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
		return __super::Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);

	bool fRefresh = false;

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		{
			CAutoLock cAutoLock(&m_csReceive);
			AM_PROPERTY_SPPAL* pSPPAL = (AM_PROPERTY_SPPAL*)pPropertyData;
			memcpy(m_sppal, pSPPAL->sppal, sizeof(AM_PROPERTY_SPPAL));

			DbgLog((LOG_TRACE, 0, _T("new palette")));
		}
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		{
			CAutoLock cAutoLock(&m_csReceive);

			AM_PROPERTY_SPHLI* pSPHLI = (AM_PROPERTY_SPHLI*)pPropertyData;

			m_sphli.Free();

			if(pSPHLI->HLISS)
			{
				POSITION pos = m_sps.GetHeadPosition();
				while(pos)
				{
					sp_t* sp = m_sps.GetNext(pos);
					if(sp->rtStart <= PTS2RT(pSPHLI->StartPTM) && PTS2RT(pSPHLI->StartPTM) < sp->rtStop)
					{
						fRefresh = true;
						sp->sphli.Free();
						sp->sphli.Attach(new AM_PROPERTY_SPHLI());
						memcpy(sp->sphli.m_p, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
					}
				}

				if(!fRefresh) // save it for later, a subpic might be late for this hli
				{
					m_sphli.Attach(new AM_PROPERTY_SPHLI());
					memcpy(m_sphli.m_p, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
				}
			}
			else
			{
				POSITION pos = m_sps.GetHeadPosition();
				while(pos)
				{
					sp_t* sp = m_sps.GetNext(pos);
					fRefresh = !!(sp->sphli.m_p);
					sp->sphli.Free();
				}
			}

			if(pSPHLI->HLISS)
			DbgLog((LOG_TRACE, 0, _T("hli: %I64d - %I64d, (%d,%d) - (%d,%d)"), 
				PTS2RT(pSPHLI->StartPTM)/10000, PTS2RT(pSPHLI->EndPTM)/10000,
				pSPHLI->StartX, pSPHLI->StartY, pSPHLI->StopX, pSPHLI->StopY));
		}
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		{
			CAutoLock cAutoLock(&m_csReceive);
			AM_PROPERTY_COMPOSIT_ON* pCompositOn = (AM_PROPERTY_COMPOSIT_ON*)pPropertyData;
			m_spon = *pCompositOn;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	if(fRefresh)
	{
		((CMpeg2DecFilter*)m_pFilter)->Deliver(true);
	}

	return S_OK;
}

STDMETHODIMP CSubpicInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
		return __super::QuerySupported(PropSet, Id, pTypeSupport);

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
	
	return S_OK;
}

//
// CMpeg2DecOutputPin
//

CMpeg2DecOutputPin::CMpeg2DecOutputPin(CTransformFilter* pTransformFilter, HRESULT* phr)
	: CTransformOutputPin(NAME("CMpeg2DecOutputPin"), pTransformFilter, phr, L"Output")
{
}

HRESULT CMpeg2DecOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	HRESULT hr = ((CMpeg2DecFilter*)m_pFilter)->CheckOutputMediaType(*mtOut);
	if(FAILED(hr)) return hr;
	return __super::CheckMediaType(mtOut);
}

//
// CClosedCaptionOutputPin
//

CClosedCaptionOutputPin::CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(NAME("CClosedCaptionOutputPin"), pFilter, pLock, phr, L"~CC")
{
}

HRESULT CClosedCaptionOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	return mtOut->majortype == MEDIATYPE_AUXLine21Data && mtOut->subtype == MEDIASUBTYPE_Line21_GOPPacket
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CClosedCaptionOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();
	pmt->majortype = MEDIATYPE_AUXLine21Data;
	pmt->subtype = MEDIASUBTYPE_Line21_GOPPacket;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CClosedCaptionOutputPin::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 2048;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

