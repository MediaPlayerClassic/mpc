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
#include <atlbase.h>
#include <ks.h>
#include <ksmedia.h>
#include "libmpeg2.h"
#include "Mpeg2DecFilter.h"

#include "..\..\..\decss\CSSauth.h"
#include "..\..\..\decss\CSSscramble.h"
#include "..\..\..\DSUtil\DSUtil.h"
#include "..\..\..\DSUtil\MediaTypes.h"
#include "..\..\..\DSUtil\vd.h"

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

static void memcpy_mmx(void* dst, const void* src, size_t len)
{
    __asm 
    {
        mov     esi, dword ptr [src]
        mov     edi, dword ptr [dst]
        mov     ecx, len
        shr     ecx, 6
align 8
CopyLoop:
        movq    mm0, qword ptr[esi]
        movq    mm1, qword ptr[esi+8*1]
        movq    mm2, qword ptr[esi+8*2]
        movq    mm3, qword ptr[esi+8*3]
        movq    mm4, qword ptr[esi+8*4]
        movq    mm5, qword ptr[esi+8*5]
        movq    mm6, qword ptr[esi+8*6]
        movq    mm7, qword ptr[esi+8*7]
        movq    qword ptr[edi], mm0
        movq    qword ptr[edi+8*1], mm1
        movq    qword ptr[edi+8*2], mm2
        movq    qword ptr[edi+8*3], mm3
        movq    qword ptr[edi+8*4], mm4
        movq    qword ptr[edi+8*5], mm5
        movq    qword ptr[edi+8*6], mm6
        movq    qword ptr[edi+8*7], mm7
        add     esi, 64
        add     edi, 64
        loop CopyLoop
        mov     ecx, len
        and     ecx, 63
        cmp     ecx, 0
        je EndCopyLoop
align 8
CopyLoop2:
        mov dl, byte ptr[esi] 
        mov byte ptr[edi], dl
        inc esi
        inc edi
        dec ecx
        jne CopyLoop2
EndCopyLoop:
		emms
    }
}

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
}

CMpeg2DecFilter::~CMpeg2DecFilter()
{
	delete m_pSubpicInput;
	delete m_pClosedCaptionOutput;
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

		switch(state)
		{
		case STATE_BUFFER:
			if(len == 0) len = -1;
			else {m_dec->mpeg2_buffer(pDataIn, pDataIn + len); len = 0;}
			break;
		case STATE_INVALID:
			{int i = 0;}
			break;
		case STATE_GOP:
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
			m_AvgTimePerFrame = 10i64 * m_dec->m_info.m_sequence->frame_period / 27;
			break;
		case STATE_PICTURE:
			{
				TCHAR frametype[] = {'?','I', 'P', 'B', 'D'};
				TRACE(_T("[%c]\n"), frametype[m_dec->m_picture->flags&PIC_MASK_CODING_TYPE]);

				m_dec->m_picture->rtStart = rtStart;
				rtStart = _I64_MIN;
			}
			break;
		case STATE_SLICE:
		case STATE_END:
			if(m_dec->m_info.m_display_picture)
			{
				//

				m_fb.rtStart = m_dec->m_info.m_display_picture->rtStart;

				if(m_fb.rtStart != _I64_MIN)
				{
					// HACK: ntsc is wierd...
					if(m_fb.rtFrame > 0 && m_dec->m_info.m_sequence->frame_period == 900900)
					{
						int dt = (int)((m_fb.rtStart - m_fb.rtOffset) / m_fb.rtFrame);
						if(m_AvgTimePerFrame == 333666 && abs(dt-333666) > 10000 && abs(dt-417083) < 10000)
							m_AvgTimePerFrame = 417083;
//						else if(m_AvgTimePerFrame == 417083 && abs(dt-417083) > 10000 && abs(dt-333666) < 10000)
//							m_AvgTimePerFrame = 333666;
					}

					m_fb.rtOffset = m_fb.rtStart;
					m_fb.rtFrame = 0;
				}
				else
				{
					m_fb.rtStart = m_fb.rtOffset + m_fb.rtFrame*m_AvgTimePerFrame;
				}

				m_fb.rtStop = m_fb.rtStart + m_AvgTimePerFrame;
				m_fb.rtFrame++;

				//

				m_fb.flags = m_dec->m_info.m_display_picture->flags;

				//

				int w = m_dec->m_info.m_sequence->width;
				int h = m_dec->m_info.m_sequence->height;
				int pw = m_dec->m_info.m_sequence->picture_width;
				int ph = m_dec->m_info.m_sequence->picture_height;

				if(m_fb.w != w || m_fb.h != h)
					m_fb.alloc(w, h, pw, ph);
				if(m_fb.pw != pw || m_fb.ph != ph)
					m_fb.pw = pw, m_fb.ph = ph;

				// TODO: find out when it is really needed to deinterlace
				if(m_fb.flags & PIC_FLAG_PROGRESSIVE_FRAME)
				{
					memcpy_mmx(m_fb.buf[0], m_dec->m_info.m_display_fbuf->buf[0], m_fb.w*m_fb.h);
					memcpy_mmx(m_fb.buf[1], m_dec->m_info.m_display_fbuf->buf[1], m_fb.w*m_fb.h/4);
					memcpy_mmx(m_fb.buf[2], m_dec->m_info.m_display_fbuf->buf[2], m_fb.w*m_fb.h/4);
				}
				else
				{
					DeinterlaceBlend(m_fb.buf[0], m_dec->m_info.m_display_fbuf->buf[0], m_fb.pw, m_fb.ph, m_fb.w);
					DeinterlaceBlend(m_fb.buf[1], m_dec->m_info.m_display_fbuf->buf[1], m_fb.pw/2, m_fb.ph/2, m_fb.w/2);
					DeinterlaceBlend(m_fb.buf[2], m_dec->m_info.m_display_fbuf->buf[2], m_fb.pw/2, m_fb.ph/2, m_fb.w/2);
				}

				// TODO: add all kinds of nice postprocessing here :P

				//

				hr = Deliver(false);
				if(FAILED(hr)) return hr;
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

	TCHAR frametype[] = {'?','I', 'P', 'B', 'D'};
	TRACE(_T("m_rtFrame: %I64d, %I64d - %I64d [%c] (%dx%d %dx%d)\n"), 
		m_fb.rtFrame, m_fb.rtStart, m_fb.rtStop, 
		frametype[m_fb.flags&PIC_MASK_CODING_TYPE],
		m_fb.w, m_fb.h, m_fb.pw, m_fb.ph);

	if((m_fb.flags&PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I)
		m_fWaitForKeyFrame = false;

	if(m_fb.rtStart < 0 || m_fWaitForKeyFrame)
		return S_OK;

	HRESULT hr;

	hr = ReconnectOutput(m_fb.pw, m_fb.ph);
	if(FAILED(hr)) return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))
	|| FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	pOut->SetTime(&m_fb.rtStart, &m_fb.rtStop);
	pOut->SetMediaTime(NULL, NULL);

//	pOut->SetDiscontinuity();
	pOut->SetSyncPoint(TRUE);

	BYTE** buf = &m_fb.buf[0];

	if(m_pSubpicInput->HasAnythingToRender(m_fb.rtStart))
	{
		memcpy_mmx(m_fb.buf[3], m_fb.buf[0], m_fb.w*m_fb.h);
		memcpy_mmx(m_fb.buf[4], m_fb.buf[1], m_fb.w*m_fb.h/4);
		memcpy_mmx(m_fb.buf[5], m_fb.buf[2], m_fb.w*m_fb.h/4);

		buf = &m_fb.buf[3];

		m_pSubpicInput->RenderSubpics(m_fb.rtStart, buf, m_fb.w, m_fb.h);
	}

	Copy(pDataOut, buf, m_fb.pw, m_fb.ph, m_fb.w);

	if(FAILED(hr = m_pOutput->Deliver(pOut)))
		return hr;

	return S_OK;
}

static void __declspec(naked) yuvtoyuy2row(
	BYTE* pDataOut, BYTE* pDataIn, BYTE* pDataInU, BYTE* pDataInV, DWORD width)
{
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi, [esp+20] // pDataOut
		mov		ebp, [esp+24] // pDataIn
		mov		ebx, [esp+28] // pDataInU
		mov		esi, [esp+32] // pDataInV
		mov		ecx, [esp+36] // width
		shr		ecx, 3

yuvtoyuy2row_loop:

		movd		mm0, [ebx]
		punpcklbw	mm0, [esi]

		movq		mm1, [ebp]
		movq		mm2, mm1
		punpcklbw	mm1, mm0
		punpckhbw	mm2, mm0

		movq		[edi], mm1
		movq		[edi+8], mm2

		add		ebp, 8
		add		ebx, 4
		add		esi, 4
        add		edi, 16

		loop	yuvtoyuy2row_loop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void __declspec(naked) yuvtoyuy2row_avg(
	BYTE* pDataOut, BYTE* pDataIn, BYTE* pDataInU, BYTE* pDataInV, DWORD pitchInUV, DWORD width)
{
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		eax, 0x7f7f7f7f
		movd	mm6, eax
		movd	mm7, eax
		punpckldq	mm7, mm6

		mov		edi, [esp+20] // pDataOut
		mov		ebp, [esp+24] // pDataIn
		mov		ebx, [esp+28] // pDataInU
		mov		esi, [esp+32] // pDataInV
		mov		eax, [esp+36] // pitchInUV
		mov		ecx, [esp+40] // width
		shr		ecx, 3

yuvtoyuy2row_avg_loop:

		movd		mm0, [ebx]
		punpcklbw	mm0, [esi]
		movq		mm1, mm0

		movd		mm2, [ebx + eax]
		punpcklbw	mm2, [esi + eax]
		movq		mm3, mm2

		// (x+y)>>1 == (x&y)+((x^y)>>1)

		pand		mm0, mm2
		pxor		mm1, mm3
		psrlq		mm1, 1
		pand		mm1, mm7
		paddb		mm0, mm1

		movq		mm1, [ebp]
		movq		mm2, mm1
		punpcklbw	mm1, mm0
		punpckhbw	mm2, mm0

		movq		[edi], mm1
		movq		[edi+8], mm2

		add		ebp, 8
		add		ebx, 4
		add		esi, 4
        add		edi, 16

		loop	yuvtoyuy2row_avg_loop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

void CMpeg2DecFilter::Copy(BYTE* pOut, BYTE** ppIn, DWORD w, DWORD h, DWORD pitchIn)
{
	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	DWORD pitchInUV = pitchIn>>1;
	BYTE* pIn = ppIn[0];
	BYTE* pInU = ppIn[1];
	BYTE* pInV = ppIn[2];

	if(bihOut.biCompression == '2YUY')
	{
		int pitchOut = bihOut.biWidth*2;

		for(DWORD y = 0; y < h; y+=2, pIn += pitchIn*2, pInU += pitchInUV, pInV += pitchInUV, pOut += pitchOut*2)
		{
			BYTE* pDataIn = pIn;
			BYTE* pDataInU = pInU;
			BYTE* pDataInV = pInV;
			WORD* pDataOut = (WORD*)pOut;
/*
			for(DWORD x = 0; x < w; x+=2)
			{
				*pDataOut++ = (*pDataInU++<<8)|*pDataIn++;
				*pDataOut++ = (*pDataInV++<<8)|*pDataIn++;
			}
*/
			yuvtoyuy2row((BYTE*)pDataOut, pDataIn, pDataInU, pDataInV, w);

			pDataIn = pIn + pitchIn;
			pDataInU = pInU;
			pDataInV = pInV;
			pDataOut = (WORD*)(pOut + pitchOut);
// 
			if(y < h-2)
			{
/*
				for(DWORD x = 0; x < w; x+=2, pDataInU++, pDataInV++)
				{
					*pDataOut++ = (((pDataInU[0]+pDataInU[pitchInUV])>>1)<<8)|*pDataIn++;
					*pDataOut++ = (((pDataInV[0]+pDataInV[pitchInUV])>>1)<<8)|*pDataIn++;
				}
*/
				yuvtoyuy2row_avg((BYTE*)pDataOut, pDataIn, pDataInU, pDataInV, pitchInUV, w);
			}
			else
			{
/*
				for(DWORD x = 0; x < w; x+=2)
				{
					*pDataOut++ = (*pDataInU++<<8)|*pDataIn++;
					*pDataOut++ = (*pDataInV++<<8)|*pDataIn++;
				}
*/
				yuvtoyuy2row((BYTE*)pDataOut, pDataIn, pDataInU, pDataInV, w);
			}
		}

		__asm emms;
	}
	else if(bihOut.biCompression == '21VY' || bihOut.biCompression == 'I420' || bihOut.biCompression == 'VUYI')
	{
		DWORD pitchOut = bihOut.biWidth;

		for(DWORD y = 0; y < h; y++, pIn += pitchIn, pOut += pitchOut)
		{
			memcpy_mmx(pOut, pIn, min(pitchIn, pitchOut));
		}

		pitchIn >>= 1;
		pitchOut >>= 1;

		pIn = bihOut.biCompression == '21VY' ? pInV : pInU;

		for(DWORD y = 0; y < h; y+=2, pIn += pitchIn, pOut += pitchOut)
		{
			memcpy_mmx(pOut, pIn, min(pitchIn, pitchOut));
		}

		pIn = bihOut.biCompression == '21VY' ? pInU : pInV;

		for(DWORD y = 0; y < h; y+=2, pIn += pitchIn, pOut += pitchOut)
		{
			memcpy_mmx(pOut, pIn, min(pitchIn, pitchOut));
		}
	}
	else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
	{
		int pitchOut = bihOut.biWidth*bihOut.biBitCount>>3;

		if(bihOut.biHeight > 0)
		{
			pOut += pitchOut*(h-1);
			pitchOut = -pitchOut;
		}

		if(!BitBltFromI420(pOut, pitchOut, pIn, pInU, pInV, w, h, bihOut.biBitCount, pitchIn))
		{
			for(DWORD y = 0; y < h; y++, pIn += pitchIn, pOut += pitchOut)
				memset(pOut, 0, pitchOut);
		}
	}
}

void CMpeg2DecFilter::ResetMpeg2Decoder()
{
	CAutoLock cAutoLock(&m_csReceive);

	for(int i = 0; i < sizeof(m_dec->m_pictures)/sizeof(m_dec->m_pictures[0]); i++)
	{
		m_dec->m_pictures[i].rtStart = m_dec->m_pictures[i].rtStop = _I64_MIN+1;
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

	m_dec->mpeg2_buffer(pSequenceHeader, pSequenceHeader + cbSequenceHeader);

	m_fWaitForKeyFrame = true;
}

HRESULT CMpeg2DecFilter::ReconnectOutput(int w, int h)
{
	CMediaType& mtOut = m_pOutput->CurrentMediaType();
	VIDEOINFOHEADER2* vihout = ((VIDEOINFOHEADER2*)mtOut.Format());

	DWORD arx = vihout->dwPictAspectRatioX;
	DWORD ary = vihout->dwPictAspectRatioY;

	CMediaType& mtIn = m_pInput->CurrentMediaType();
	if(mtIn.formattype == FORMAT_MPEG2_VIDEO)
	{
		arx = ((MPEG2VIDEOINFO*)mtIn.Format())->hdr.dwPictAspectRatioX;
		ary = ((MPEG2VIDEOINFO*)mtIn.Format())->hdr.dwPictAspectRatioY;
	}

	HRESULT hr = S_OK;

	if(vihout->bmiHeader.biWidth < w || abs(vihout->bmiHeader.biHeight) != h
	|| vihout->dwPictAspectRatioX != arx || vihout->dwPictAspectRatioY != ary)
	{
		CMediaType mt = mtOut;

		vihout = (VIDEOINFOHEADER2*)mt.Format();
		vihout->bmiHeader.biWidth = w;
		vihout->bmiHeader.biHeight = h;
		vihout->bmiHeader.biSizeImage = w*h*vihout->bmiHeader.biBitCount>>3;
//		vihout->bmiHeader.biXPelsPerMeter = w*ary;
//		vihout->bmiHeader.biYPelsPerMeter = h*arx;
		vihout->dwPictAspectRatioX = arx;
		vihout->dwPictAspectRatioY = ary;

		CComPtr<IPin> pPin = m_pOutput->GetConnected();

		// only the overlay mixer needs this, the vmr doesn't seem so
		CComQIPtr<IMemInputPin> pMemPin = pPin;
		CComPtr<IMemAllocator> pAlloc;
		hr = pMemPin->GetAllocator(&pAlloc);
		hr = pAlloc->Decommit();
		ALLOCATOR_PROPERTIES props;
		hr = pAlloc->GetProperties(&props);
		props.cbBuffer = vihout->bmiHeader.biSizeImage;
		hr = pAlloc->Commit();
		if(FAILED(hr)) return hr;

		hr = pPin->ReceiveConnection(m_pOutput, &mt);
		if(FAILED(hr)) return hr;

		NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(w, h), 0);
		
		m_pOutput->SetMediaType(&mt);
	}

	return hr;
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
	return SUCCEEDED(mtIn)
		&& mtOut->majortype == MEDIATYPE_Video && (mtOut->subtype == MEDIASUBTYPE_YUY2
												|| mtOut->subtype == MEDIASUBTYPE_YV12
												|| mtOut->subtype == MEDIASUBTYPE_I420
												|| mtOut->subtype == MEDIASUBTYPE_IYUV
												|| mtOut->subtype == MEDIASUBTYPE_ARGB32
												|| mtOut->subtype == MEDIASUBTYPE_RGB32
												|| mtOut->subtype == MEDIASUBTYPE_RGB24
												|| mtOut->subtype == MEDIASUBTYPE_RGB565
												|| mtOut->subtype == MEDIASUBTYPE_RGB555)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::CheckOutputMediaType(const CMediaType& mtOut)
{
	CMediaType& mt = m_pInput->CurrentMediaType();

	int w = 0, h = 0;

	if(mt.formattype == FORMAT_MPEGVideo)
	{
		w = ((MPEG1VIDEOINFO*)mt.Format())->hdr.bmiHeader.biWidth;
		h = ((MPEG1VIDEOINFO*)mt.Format())->hdr.bmiHeader.biHeight;
	}
	else if(mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		w = ((MPEG2VIDEOINFO*)mt.Format())->hdr.bmiHeader.biWidth;
		h = ((MPEG2VIDEOINFO*)mt.Format())->hdr.bmiHeader.biHeight;
	}

	return mtOut.formattype == FORMAT_VideoInfo2 && h == abs(((VIDEOINFOHEADER2*)mtOut.Format())->bmiHeader.biHeight)
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

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= sizeof(fmts)/sizeof(fmts[0])) return VFW_S_NO_MORE_ITEMS;

	CMediaType& mt = m_pInput->CurrentMediaType();

	BITMAPINFOHEADER bih;
	ExtractBIH(&mt, &bih);

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = *fmts[iPosition].subtype;

	BITMAPINFOHEADER bihOut;
	memset(&bihOut, 0, sizeof(bihOut));
	bihOut.biSize = sizeof(bihOut);
	bihOut.biWidth = bih.biWidth;
	bihOut.biHeight = bih.biHeight;
	bihOut.biPlanes = fmts[iPosition].biPlanes;
	bihOut.biBitCount = fmts[iPosition].biBitCount;
	bihOut.biCompression = fmts[iPosition].biCompression;
	bihOut.biSizeImage = bih.biWidth*bih.biHeight*bihOut.biBitCount>>3;

	pmt->formattype = FORMAT_VideoInfo2;
	VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
	memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
	vih2->bmiHeader = bihOut;
	vih2->AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
	vih2->dwBitRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitRate;
	vih2->dwBitErrorRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate;

	if(mt.formattype == FORMAT_MPEGVideo)
	{
		vih2->dwPictAspectRatioX = bih.biWidth*((MPEG1VIDEOINFO*)mt.Format())->hdr.bmiHeader.biYPelsPerMeter;
		vih2->dwPictAspectRatioY = bih.biHeight*((MPEG1VIDEOINFO*)mt.Format())->hdr.bmiHeader.biXPelsPerMeter;
	}
	else if(mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		vih2->dwPictAspectRatioX = ((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioX;
		vih2->dwPictAspectRatioY = ((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioY;
	}

	CorrectMediaType(pmt);

	return S_OK;
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
	return E_NOTIMPL;
}

//
// CMpeg2DecInputPin
//

CMpeg2DecInputPin::CMpeg2DecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CTransformInputPin(NAME("CMpeg2DecInputPin"), pFilter, phr, pName)
{
	m_varient = -1;
	memset(m_Challenge, 0, sizeof(m_Challenge));
	memset(m_KeyCheck, 0, sizeof(m_KeyCheck));
	memset(m_DiscKey, 0, sizeof(m_DiscKey));
	memset(m_TitleKey, 0, sizeof(m_TitleKey));
}

STDMETHODIMP CMpeg2DecInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IKsPropertySet)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

// IMemInputPin

STDMETHODIMP CMpeg2DecInputPin::Receive(IMediaSample* pSample)
{
	if(m_mt.majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && pSample->GetActualDataLength() == 2048)
	{
		BYTE* pBuffer = NULL;
		if(SUCCEEDED(pSample->GetPointer(&pBuffer)) && (pBuffer[0x14]&0x30))
		{
			CSSdescramble(pBuffer, m_TitleKey);
			pBuffer[0x14] &= ~0x30;

			if(CComQIPtr<IMediaSample2> pMS2 = pSample)
			{
				AM_SAMPLE2_PROPERTIES props;
				memset(&props, 0, sizeof(props));
				if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))
				&& (props.dwTypeSpecificFlags & AM_UseNewCSSKey))
				{
					props.dwTypeSpecificFlags &= ~AM_UseNewCSSKey;
					pMS2->SetProperties(sizeof(props), (BYTE*)&props);
				}
			}
		}
	}

	HRESULT hr = Transform(pSample);

	return 
		hr == S_OK ? __super::Receive(pSample) :
		hr == S_FALSE ? S_OK : hr;
}

// IKsPropertySet

STDMETHODIMP CMpeg2DecInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_CopyProt)
		return E_NOTIMPL;

	switch(Id)
	{
	case AM_PROPERTY_COPY_MACROVISION:
		break;
	case AM_PROPERTY_DVDCOPY_CHLG_KEY: // 3. auth: receive drive nonce word, also store and encrypt the buskey made up of the two nonce words
		{
			AM_DVDCOPY_CHLGKEY* pChlgKey = (AM_DVDCOPY_CHLGKEY*)pPropertyData;
			for(int i = 0; i < 10; i++)
				m_Challenge[i] = pChlgKey->ChlgKey[9-i];

			CSSkey2(m_varient, m_Challenge, &m_Key[5]);

			CSSbuskey(m_varient, m_Key, m_KeyCheck);
		}
		break;
	case AM_PROPERTY_DVDCOPY_DISC_KEY: // 5. receive the disckey
		{
			AM_DVDCOPY_DISCKEY* pDiscKey = (AM_DVDCOPY_DISCKEY*)pPropertyData; // pDiscKey->DiscKey holds the disckey encrypted with itself and the 408 disckeys encrypted with the playerkeys

			bool fSuccess = false;

			for(int j = 0; j < g_nPlayerKeys; j++)
			{
				for(int k = 1; k < 409; k++)
				{
					BYTE DiscKey[6];
					for(int i = 0; i < 5; i++)
						DiscKey[i] = pDiscKey->DiscKey[k*5+i] ^ m_KeyCheck[4-i];
					DiscKey[5] = 0;

					CSSdisckey(DiscKey, g_PlayerKeys[j]);

					BYTE Hash[6];
					for(int i = 0; i < 5; i++)
						Hash[i] = pDiscKey->DiscKey[i] ^ m_KeyCheck[4-i];
					Hash[5] = 0;

					CSSdisckey(Hash, DiscKey);

					if(!memcmp(Hash, DiscKey, 6))
					{
						memcpy(m_DiscKey, DiscKey, 6);
						j = g_nPlayerKeys;
						fSuccess = true;
						break;
					}
				}
			}

			if(!fSuccess)
				return E_FAIL;
		}
		break;
	case AM_PROPERTY_DVDCOPY_DVD_KEY1: // 2. auth: receive our drive-encrypted nonce word and decrypt it for verification
		{
			AM_DVDCOPY_BUSKEY* pKey1 = (AM_DVDCOPY_BUSKEY*)pPropertyData;
			for(int i = 0; i < 5; i++)
				m_Key[i] =  pKey1->BusKey[4-i];

			m_varient = -1;

			for(int i = 31; i >= 0; i--)
			{
				CSSkey1(i, m_Challenge, m_KeyCheck);

				if(memcmp(m_KeyCheck, &m_Key[0], 5) == 0)
					m_varient = i;
			}
		}
		break;
	case AM_PROPERTY_DVDCOPY_REGION:
		break;
	case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
		break;
	case AM_PROPERTY_DVDCOPY_TITLE_KEY: // 6. receive the title key and decrypt it with the disc key
		{
			AM_DVDCOPY_TITLEKEY* pTitleKey = (AM_DVDCOPY_TITLEKEY*)pPropertyData;
			for(int i = 0; i < 5; i++)
				m_TitleKey[i] = pTitleKey->TitleKey[i] ^ m_KeyCheck[4-i];
			m_TitleKey[5] = 0;
			CSStitlekey(m_TitleKey, m_DiscKey);
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

STDMETHODIMP CMpeg2DecInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_CopyProt)
		return E_NOTIMPL;

	switch(Id)
	{
	case AM_PROPERTY_DVDCOPY_CHLG_KEY: // 1. auth: send our nonce word
		{
			AM_DVDCOPY_CHLGKEY* pChlgKey = (AM_DVDCOPY_CHLGKEY*)pPropertyData;
			for(int i = 0; i < 10; i++)
				pChlgKey->ChlgKey[i] = 9 - (m_Challenge[i] = i);
			*pBytesReturned = sizeof(AM_DVDCOPY_CHLGKEY);
		}
		break;
	case AM_PROPERTY_DVDCOPY_DEC_KEY2: // 4. auth: send back the encrypted drive nonce word to finish the authentication
		{
			AM_DVDCOPY_BUSKEY* pKey2 = (AM_DVDCOPY_BUSKEY*)pPropertyData;
			for(int i = 0; i < 5; i++)
				pKey2->BusKey[4-i] = m_Key[5+i];
			*pBytesReturned = sizeof(AM_DVDCOPY_BUSKEY);
		}
		break;
	case AM_PROPERTY_DVDCOPY_REGION:
		{
			DVD_REGION* pRegion = (DVD_REGION*)pPropertyData;
			pRegion->RegionData = 0;
			pRegion->SystemRegion = 0;
			*pBytesReturned = sizeof(DVD_REGION);
		}
		break;
	case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
		{
			AM_DVDCOPY_SET_COPY_STATE* pState = (AM_DVDCOPY_SET_COPY_STATE*)pPropertyData;
			pState->DVDCopyState = AM_DVDCOPYSTATE_AUTHENTICATION_REQUIRED;
			*pBytesReturned = sizeof(AM_DVDCOPY_SET_COPY_STATE);
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

STDMETHODIMP CMpeg2DecInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_CopyProt)
		return E_NOTIMPL;

	switch(Id)
	{
	case AM_PROPERTY_COPY_MACROVISION:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_CHLG_KEY:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_DEC_KEY2:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_PROPERTY_DVDCOPY_DISC_KEY:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_DVD_KEY1:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_REGION:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDCOPY_TITLE_KEY:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
	
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

    if(packetsize > sp->pData.GetSize()-4 || datasize > packetsize)
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

static __inline void DrawPixels(BYTE** yuv, CPoint pt, CRect& rc, int pitch, int len, BYTE color, 
								AM_PROPERTY_SPHLI& sphli, AM_DVD_YUV* sppal,
								AM_COLCON* colcon_hli = NULL)
{
	if(pt.y < rc.top || pt.y >= rc.bottom) return;
	if(pt.x < rc.left) {len -= rc.left - pt.x; pt.x = rc.left;}
	if(pt.x + len > rc.right) len = rc.right - pt.x;
	if(len <= 0 || pt.x >= rc.right) return;

	BYTE contrast;

	if(colcon_hli) switch(color)
	{
	case 0: color = colcon_hli->backcol; contrast = colcon_hli->backcon; break;
	case 1: color = colcon_hli->patcol; contrast = colcon_hli->patcon; break;
	case 2: color = colcon_hli->emph1col; contrast = colcon_hli->emph1con; break;
	case 3: color = colcon_hli->emph2col; contrast = colcon_hli->emph2con; break;
	default: ASSERT(0); return;
	}
	else switch(color)
	{
	case 0: color = sphli.ColCon.backcol; contrast = sphli.ColCon.backcon; break;
	case 1: color = sphli.ColCon.patcol; contrast = sphli.ColCon.patcon; break;
	case 2: color = sphli.ColCon.emph1col; contrast = sphli.ColCon.emph1con; break;
	case 3: color = sphli.ColCon.emph2col; contrast = sphli.ColCon.emph2con; break;
	default: ASSERT(0); return;
	}

	if(contrast == 0) return;

	BYTE* p = &yuv[0][pt.y*pitch + pt.x];
	BYTE c = sppal[color].Y;
	for(int i = 0; i < len; i++) 
//		*p++ = (*p*(15-contrast) + c*contrast)>>4;
		*p++ -= (*p - c) * contrast >> 4;

	if(pt.y&1) return; // since U/V is half res there is no need to overwrite the same line again

	len = (len + 1) / 2;
	pt.x = (pt.x + 1) / 2;
	pt.y = (pt.y /*+ 1*/) / 2; // only paint the upper field always, don't round it
	pitch /= 2;

	// U/V is exchanged? wierd but looks true when comparing the outputted colors from other decoders

	p = &yuv[1][pt.y*pitch + pt.x];
	c = sppal[color].V;
	for(int i = 0; i < len; i++) 
//		*p++ = (BYTE)(((((int)*p-0x80)*(15-contrast) + ((int)c-0x80)*contrast) >> 4) + 0x80);
		*p++ -= (*p - c) * contrast >> 4;

	p = &yuv[2][pt.y*pitch + pt.x];
	c = sppal[color].U;
	for(int i = 0; i < len; i++) 
		*p++ = (BYTE)(((((int)*p-0x80)*(15-contrast) + ((int)c-0x80)*contrast) >> 4) + 0x80);
//		*p++ -= (*p - c) * contrast >> 4;

	// Neighter of the blending formulas are accurate (">>4" should be "/15").
	// Even though the second one is a bit worse, since we are scaling the difference only,
	// the error is still not noticable.
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
	CRect rcclip(rc);

	if(sphli_hli)
	{
		rcclip &= CRect(CPoint(sphli_hli->StartX, sphli_hli->StartY), CPoint(sphli_hli->StopX, sphli_hli->StopY));
		if(rcclip.IsRectEmpty()) return;
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
			DrawPixels(p, pt, rcclip, w, code >> 2, (BYTE)(code & 3), sphli, m_sppal, sphli_hli ? &sphli_hli->ColCon : NULL);
			if((pt.x += code >> 2) < rc.right) continue;
		}

		DrawPixels(p, pt, rcclip, w, rc.right - pt.x, (BYTE)(code & 3), sphli, m_sppal, sphli_hli ? &sphli_hli->ColCon : NULL);

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
		if(sp->rtStart <= rt && rt < sp->rtStop && (/*sp->sphli ||*/ sp->fForced || m_spon))
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
		// remove subpics with undefined end time
		POSITION pos = m_sps.GetHeadPosition();
		while(pos)
		{
			POSITION cur = pos;
			sp_t* sp = m_sps.GetNext(pos);
			if(sp->rtStop == _I64_MAX) m_sps.RemoveAt(cur);
		}

		CAutoPtr<sp_t> p(new sp_t());
		p->rtStart = rtStart;
		p->rtStop = _I64_MAX;
		p->pData.SetSize(len);
		memcpy(p->pData.GetData(), pDataIn, len);

		if(m_sphli && p->rtStart == PTS2RT(m_sphli->StartPTM))
			p->sphli = m_sphli;

		m_sps.AddTail(p);
	}

	if(!m_sps.IsEmpty())
	{
		AM_PROPERTY_SPHLI sphli;
		DWORD offset[2];
		if(DecodeSubpic(m_sps.GetTail(), sphli, offset[0], offset[1]))
			DbgLog((LOG_TRACE, 0, _T("transform: %I64d - %I64d"), m_sps.GetTail()->rtStart/10000, m_sps.GetTail()->rtStop/10000));
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

			POSITION pos = m_sps.GetHeadPosition();
			while(pos)
			{
				sp_t* sp = m_sps.GetNext(pos);
				if(sp->rtStart <= PTS2RT(pSPHLI->StartPTM) && PTS2RT(pSPHLI->StartPTM) < sp->rtStop)
				{
					fRefresh = true;
					sp->sphli.Free();
					if(pSPHLI->HLISS == 0) break;
					sp->sphli.Attach(new AM_PROPERTY_SPHLI());
					memcpy(sp->sphli.m_p, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
				}
			}

			if(!fRefresh && pSPHLI->HLISS) // save it for later, a subpic might be late for this hli
			{
				m_sphli.Attach(new AM_PROPERTY_SPHLI());
				memcpy(m_sphli.m_p, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
			}

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

STDMETHODIMP CSubpicInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
		return __super::Get(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength, pBytesReturned);

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
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
