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
#include "subtitlesource.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\matroska\matroska.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Subtitle, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_Text, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB32},
};

const AMOVIESETUP_PIN sudOpPin[] =
{
	{
		L"Output",              // Pin string name
		FALSE,                  // Is it rendered
		TRUE,                   // Is it an output
		FALSE,                  // Can we have none
		FALSE,                  // Can we have many
		&CLSID_NULL,            // Connects to filter
		NULL,                   // Connects to pin
		sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
		sudPinTypesOut			// Pin details
	},
};

const AMOVIESETUP_FILTER sudFilter =
{
    &__uuidof(CSubtitleSource),	// Filter CLSID
    L"SubtitleSource",			// String name
    MERIT_UNLIKELY,			// Filter merit
    sizeof(sudOpPin)/sizeof(sudOpPin[0]), // Number of pins
    sudOpPin				// Pin information
};

CFactoryTemplate g_Templates[] =
{
	{L"SubtitleSource", &__uuidof(CSubtitleSource), CSubtitleSource::CreateInstance, NULL, &sudFilter}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	CString clsid = CStringFromGUID(__uuidof(CSubtitleSource));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".sub"), 
		_T("Source Filter"), clsid);

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".srt"), 
		_T("Source Filter"), clsid);

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".smi"), 
		_T("Source Filter"), clsid);

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".ssa"), 
		_T("Source Filter"), clsid);

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".ass"), 
		_T("Source Filter"), clsid);

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".xss"), 
		_T("Source Filter"), clsid);

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".usf"), 
		_T("Source Filter"), clsid);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".sub"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".srt"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".smi"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".ssa"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".ass"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".xss"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".usf"));

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CSubtitleSource
//

CUnknown* WINAPI CSubtitleSource::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CSubtitleSource(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CSubtitleSource::CSubtitleSource(LPUNKNOWN lpunk, HRESULT* phr)
	: CSource(NAME("CSubtitleSource"), lpunk, __uuidof(this))
{
}

CSubtitleSource::~CSubtitleSource()
{
}

STDMETHODIMP CSubtitleSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IFileSourceFilter)
		QI(IAMFilterMiscFlags)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CSubtitleSource::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt) 
{
	if(GetPinCount() > 0)
		return VFW_E_ALREADY_CONNECTED;

	HRESULT hr = S_OK;
	if(!(new CSubtitleStream(pszFileName, this, &hr)))
		return E_OUTOFMEMORY;

	if(FAILED(hr))
		return hr;

	m_fn = pszFileName;

	return S_OK;
}

STDMETHODIMP CSubtitleSource::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	if(!ppszFileName) return E_POINTER;
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
		return E_OUTOFMEMORY;

	wcscpy(*ppszFileName, m_fn);

	return S_OK;
}

// IAMFilterMiscFlags

ULONG CSubtitleSource::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

// CSubtitleStream

CSubtitleStream::CSubtitleStream(const WCHAR* wfn, CSubtitleSource* pParent, HRESULT* phr) 
	: CSourceStream(NAME("SubtitleStream"), phr, pParent, L"Output")
	, CSourceSeeking(NAME("SubtitleStream"), (IPin*)this, phr, &m_cSharedState)
	, m_bDiscontinuity(FALSE), m_bFlushing(FALSE)
	, m_nPosition(0)
	, m_rts(NULL)
{
	CAutoLock cAutoLock(&m_cSharedState);

	CString fn(wfn);

	if(!m_rts.Open(fn, DEFAULT_CHARSET))
	{
		if(phr) *phr = E_FAIL;
		return;
	}

	m_rts.CreateDefaultStyle(DEFAULT_CHARSET);
	m_rts.ConvertToTimeBased(25);
	m_rts.Sort();

	m_rtDuration = 0;
	for(int i = 0, cnt = m_rts.GetCount(); i < cnt; i++)
		m_rtDuration = max(m_rtDuration, 10000i64*m_rts[i].end);

	m_rtStop = m_rtDuration;

	if(phr) *phr = m_rtDuration > 0 ? S_OK : E_FAIL;
}

CSubtitleStream::~CSubtitleStream()
{
	CAutoLock cAutoLock(&m_cSharedState);
}

STDMETHODIMP CSubtitleStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return (riid == IID_IMediaSeeking) ? CSourceSeeking::NonDelegatingQueryInterface(riid, ppv) //GetInterface((IMediaSeeking*)this, ppv)
		: CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

void CSubtitleStream::UpdateFromSeek()
{
	if(ThreadExists())
	{
		// next time around the loop, the worker thread will
		// pick up the position change.
		// We need to flush all the existing data - we must do that here
		// as our thread will probably be blocked in GetBuffer otherwise
	    
		m_bFlushing = TRUE;

		DeliverBeginFlush();
		// make sure we have stopped pushing
		Stop();
		// complete the flush
		DeliverEndFlush();

        m_bFlushing = FALSE;

		// restart
		Run();
	}
}

HRESULT CSubtitleStream::SetRate(double dRate)
{
	if(dRate <= 0)
		return E_INVALIDARG;

	{
		CAutoLock lock(CSourceSeeking::m_pLock);
		m_dRateSeeking = dRate;
	}

	UpdateFromSeek();

	return S_OK;
}

HRESULT CSubtitleStream::OnThreadStartPlay()
{
    m_bDiscontinuity = TRUE;
    return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}

HRESULT CSubtitleStream::ChangeStart()
{
    {
        CAutoLock lock(CSourceSeeking::m_pLock);

		if(m_mt.majortype == MEDIATYPE_Video)
		{
			int m_nSegments = 0;
			if(!m_rts.SearchSubs((int)(m_rtStart/10000), 25, &m_nPosition, &m_nSegments))
				m_nPosition = m_nSegments;
		}
		else
		{
			m_nPosition = m_rts.SearchSub((int)(m_rtStart/10000), 25);
		}
    }

    UpdateFromSeek();

    return S_OK;
}

HRESULT CSubtitleStream::ChangeStop()
{
/*
    {
        CAutoLock lock(CSourceSeeking::m_pLock);
        if(m_rtPosition < m_rtStop)
			return S_OK;
    }
*/
    // We're already past the new stop time -- better flush the graph.
    UpdateFromSeek();

    return S_OK;
}

HRESULT CSubtitleStream::OnThreadCreate()
{
    CAutoLock cAutoLockShared(&m_cSharedState);

	if(m_mt.majortype == MEDIATYPE_Video)
	{
		int m_nSegments = 0;
		if(!m_rts.SearchSubs((int)(m_rtStart/10000), 25, &m_nPosition, &m_nSegments))
			m_nPosition = m_nSegments;
	}
	else
	{
		m_nPosition = m_rts.SearchSub((int)(m_rtStart/10000), 25);
	}

    return CSourceStream::OnThreadCreate();
}

HRESULT CSubtitleStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
//    CAutoLock cAutoLock(m_pFilter->pStateLock());

    ASSERT(pAlloc);
    ASSERT(pProperties);

    HRESULT hr = NOERROR;

	if(m_mt.majortype == MEDIATYPE_Video)
	{
		pProperties->cBuffers = 1;
		pProperties->cbBuffer = ((VIDEOINFOHEADER*)m_mt.pbFormat)->bmiHeader.biSizeImage;
	}
	else
	{
		pProperties->cBuffers = 1;
		pProperties->cbBuffer = 0x10000;
	}

    ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
    ASSERT(Actual.cBuffers == pProperties->cBuffers);

    return NOERROR;
}

HRESULT CSubtitleStream::FillBuffer(IMediaSample* pSample)
{
	HRESULT hr;

	{
		CAutoLock cAutoLockShared(&m_cSharedState);

		BYTE* pData = NULL;
		if(FAILED(hr = pSample->GetPointer(&pData)) || !pData)
			return S_FALSE;

		AM_MEDIA_TYPE* pmt;
		if(SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt)
		{
			CMediaType mt(*pmt);
			SetMediaType(&mt);
			DeleteMediaType(pmt);
		}

		int len = 0;
		REFERENCE_TIME rtStart, rtStop;

		if(m_mt.majortype == MEDIATYPE_Video)
		{
			const STSSegment* stss = m_rts.GetSegment(m_nPosition);
			if(!stss) return S_FALSE;

			memset(pData, 0, pSample->GetSize());

			BITMAPINFOHEADER& bmi = ((VIDEOINFOHEADER*)m_mt.pbFormat)->bmiHeader;

			SubPicDesc spd;
			spd.w = 640;
			spd.h = 480;
			spd.bpp = 32;
			spd.pitch = bmi.biWidth*4;
			spd.bits = pData;
			RECT bbox;
			m_rts.Render(spd, 10000i64*(stss->start+stss->end)/2, 25, bbox);

			rtStart = (REFERENCE_TIME)((10000i64*stss->start - m_rtStart) / m_dRateSeeking);
			rtStop = (REFERENCE_TIME)((10000i64*stss->end - m_rtStart) / m_dRateSeeking);

			pSample->SetSyncPoint(TRUE);
		}
		else
		{
			if(m_nPosition >= m_rts.GetCount())
				return S_FALSE;

			STSEntry& stse = m_rts[m_nPosition];

			if(stse.start >= m_rtStop/10000)
				return S_FALSE;

			if(m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_RAWASS)
			{
				CStringA str = UTF16To8(m_rts.GetStrW(m_nPosition, true));
				memcpy((char*)pData, str, len = str.GetLength()+1);
			}
			else if(m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_UTF8)
			{
				CStringA str = UTF16To8(m_rts.GetStrW(m_nPosition, false));
				memcpy((char*)pData, str, len = str.GetLength()+1);
			}
	/*		else if(m_mt.majortype == MEDIATYPE_Subtitle && (m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS))
			{
				CStringA str = UTF16To8(m_rts.GetStrW(m_nPosition, true));
				memcpy((char*)pData, str, len = str.GetLength()+1);
				// TODO: Reconstruct and store "Dialogue: ..." line.
			}
	*/		else if(m_mt.majortype == MEDIATYPE_Text && m_mt.subtype == MEDIASUBTYPE_NULL)
			{
				CStringA str = m_rts.GetStrA(m_nPosition, false);
				memcpy((char*)pData, str, len = str.GetLength()+1);
			}
			else
			{
				return S_FALSE;
			}

			rtStart = (REFERENCE_TIME)((10000i64*stse.start - m_rtStart) / m_dRateSeeking);
			rtStop = (REFERENCE_TIME)((10000i64*stse.end - m_rtStart) / m_dRateSeeking);
		}

		pSample->SetTime(&rtStart, &rtStop);
		pSample->SetActualDataLength(len);

		m_nPosition++;
	}

	pSample->SetSyncPoint(TRUE);

	if(m_bDiscontinuity) 
    {
		pSample->SetDiscontinuity(TRUE);
		m_bDiscontinuity = FALSE;
	}

	return S_OK;
}

HRESULT CSubtitleStream::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0) return E_INVALIDARG;

	pmt->InitMediaType();

	SUBTITLEINFO* psi = (SUBTITLEINFO*)pmt->AllocFormatBuffer(sizeof(SUBTITLEINFO));
	memset(psi, 0, sizeof(pmt->FormatLength()));

	if(iPosition == 0)
	{
		pmt->SetType(&MEDIATYPE_Subtitle);
		pmt->SetSubtype(&MEDIASUBTYPE_RAWASS);
		pmt->SetFormatType(&FORMAT_SubtitleInfo);
		strcpy(psi->IsoLang, "eng");
	}
	else if(iPosition == 1)
	{
		pmt->SetType(&MEDIATYPE_Subtitle);
		pmt->SetSubtype(&MEDIASUBTYPE_UTF8);
		pmt->SetFormatType(&FORMAT_SubtitleInfo);
		strcpy(psi->IsoLang, "eng");
	}
	else if(iPosition == 2)
	{
		pmt->SetType(&MEDIATYPE_Video);
		pmt->SetSubtype(&MEDIASUBTYPE_RGB32);
		pmt->SetFormatType(&FORMAT_VideoInfo);
		VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memset(pvih, 0, sizeof(pmt->FormatLength()));
		pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
		pvih->bmiHeader.biWidth = 640;
		pvih->bmiHeader.biHeight = 480;
		pvih->bmiHeader.biBitCount = 32;
		pvih->bmiHeader.biCompression = BI_RGB;
		pvih->bmiHeader.biPlanes = 1;
		pvih->bmiHeader.biSizeImage = pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*pvih->bmiHeader.biBitCount>>3;
	}
/*
	else if(iPosition == 1)
	{
		pmt->SetType(&MEDIATYPE_Subtitle);
		pmt->SetSubtype(&MEDIASUBTYPE_SSA);
		pmt->SetFormatType(&FORMAT_SubtitleInfo);
	}
	else if(iPosition == 2)
	{
		pmt->SetType(&MEDIATYPE_Subtitle);
		pmt->SetSubtype(&MEDIASUBTYPE_ASS);
		pmt->SetFormatType(&FORMAT_SubtitleInfo);
	}
	else if(iPosition == 3)
	{
		pmt->SetType(&MEDIATYPE_Subtitle);
		pmt->SetSubtype(&MEDIASUBTYPE_USF);
		pmt->SetFormatType(&FORMAT_SubtitleInfo);
	}
	else if(iPosition == 4)
	{
		pmt->SetType(&MEDIATYPE_Text);
		pmt->SetSubtype(&MEDIASUBTYPE_NULL);
		pmt->SetFormatType(&FORMAT_None);
		pmt->ResetFormatBuffer();
	}
*/
	else
	{
		return VFW_S_NO_MORE_ITEMS;
	}

    return NOERROR;
}

HRESULT CSubtitleStream::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->majortype == MEDIATYPE_Subtitle || pmt->majortype == MEDIATYPE_Text
	|| pmt->majortype == MEDIATYPE_Video && pmt->subtype == MEDIASUBTYPE_RGB32)
		return S_OK;

	return E_INVALIDARG;
}

STDMETHODIMP CSubtitleStream::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}
