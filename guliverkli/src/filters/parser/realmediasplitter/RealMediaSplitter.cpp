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

#include "StdAfx.h"
#include <mmreg.h>
#include "..\..\..\DSUtil\DSUtil.h"
#include "RealMediaSplitter.h"

#include <initguid.h>

// {3701A20D-BB90-4df7-A56F-F78B5CB04EC5}
DEFINE_GUID(MEDIASUBTYPE_RealMedia, 
0x3701a20d, 0xbb90, 0x4df7, 0xa5, 0x6f, 0xf7, 0x8b, 0x5c, 0xb0, 0x4e, 0xc5);

// {C03655F6-EF66-4266-ADC6-8B510759D977}
DEFINE_GUID(MEDIATYPE_RealMediaPackets, 
0xc03655f6, 0xef66, 0x4266, 0xad, 0xc6, 0x8b, 0x51, 0x7, 0x59, 0xd9, 0x77);

// {A733AE3E-7656-4b95-A243-31E94370E04D}
DEFINE_GUID(FORMAT_RealMediaInfo, 
0xa733ae3e, 0x7656, 0x4b95, 0xa2, 0x43, 0x31, 0xe9, 0x43, 0x70, 0xe0, 0x4d);

template<typename T>
static void bswap(T& var)
{
	BYTE* s = (BYTE*)&var;
	for(BYTE* d = s + sizeof(var)-1; s < d; s++, d--)
		*s ^= *d, *d ^= *s, *s ^= *d;
}

using namespace RealMedia;

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
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
      sudPinTypesIn         // Pin information
    },
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{ &__uuidof(CRealMediaSourceFilter)	// Filter CLSID
    , L"RealMedia Source"					// String name
    , MERIT_UNLIKELY						// Filter merit
    , 0										// Number of pins
	, NULL},								// Pin information
	{ &__uuidof(CRealMediaSplitterFilter)	// Filter CLSID
    , L"RealMedia Splitter"					// String name
    , MERIT_UNLIKELY						// Filter merit
    , sizeof(sudpPins)/sizeof(sudpPins[0])	// Number of pins
	, sudpPins},							// Pin information
};

/////////////////////

const AMOVIESETUP_MEDIATYPE sudPinTypesIn2[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut2[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YV12},
};

const AMOVIESETUP_PIN sudpPins2[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn2)/sizeof(sudPinTypesIn2[0]),	// Number of types
      sudPinTypesIn2			// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut2)/sizeof(sudPinTypesOut2[0]),	// Number of types
      sudPinTypesOut2		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter2[] =
{
	{&__uuidof(CRealVideoDecoder), L"RealVideo Decoder", MERIT_UNLIKELY, sizeof(sudpPins2)/sizeof(sudpPins2[0]), sudpPins2}
};

////////////////////

CFactoryTemplate g_Templates[] =
{
	{L"RealMedia Source", &__uuidof(CRealMediaSourceFilter), CRealMediaSourceFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"RealMedia Splitter", &__uuidof(CRealMediaSplitterFilter), CRealMediaSplitterFilter::CreateInstance, NULL, &sudFilter[1]},
////////////////////
    {L"RealVideo Decoder", &__uuidof(CRealVideoDecoder), CRealVideoDecoder::CreateInstance, NULL, &sudFilter2[0]},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), CStringFromGUID(MEDIASUBTYPE_RealMedia), 
		_T("0"), _T("0,4,,2E524D46")); // .RMF

	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), CStringFromGUID(MEDIASUBTYPE_RealMedia), 
		_T("Source Filter"), CStringFromGUID(CLSID_AsyncReader));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".rm"), 
		_T("Source Filter"), CStringFromGUID(__uuidof(CRealMediaSourceFilter)));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".ra"), 
		_T("Source Filter"), CStringFromGUID(__uuidof(CRealMediaSourceFilter)));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".ram"), 
		_T("Source Filter"), CStringFromGUID(__uuidof(CRealMediaSourceFilter)));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".rmvb"), 
		_T("Source Filter"), CStringFromGUID(__uuidof(CRealMediaSourceFilter)));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), CStringFromGUID(MEDIASUBTYPE_RealMedia));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".rm"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".ra"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".ram"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".rmvb"));

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI CRealMediaSourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CRealMediaSourceFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CUnknown* WINAPI CRealMediaSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CRealMediaSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// CRealMediaSourceFilter
//

CRealMediaSourceFilter::CRealMediaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(NAME("CRealMediaSourceFilter"), pUnk, this, __uuidof(this))
	, m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
	, m_dRate(1.0)
/*	, m_nOpenProgress(100)
	, m_fAbort(false)*/
{
	if(phr) *phr = S_OK;
}

CRealMediaSourceFilter::~CRealMediaSourceFilter()
{
	CAutoLock cAutoLock(this);

	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();
}

STDMETHODIMP CRealMediaSourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	if(m_pInput && riid == __uuidof(IFileSourceFilter)) 
		return E_NOINTERFACE;

	return 
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
//		QI(IAMOpenProgress)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRealMediaSourceFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	if(m_pOutputs.GetCount() > 0) return VFW_E_ALREADY_CONNECTED;

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_mapTrackToPin.RemoveAll();

	m_pFile.Attach(new CRMFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = 0;

	POSITION pos = m_pFile->m_mps.GetHeadPosition();
	while(pos)
	{
		MediaProperies* pmp = m_pFile->m_mps.GetNext(pos);

		CStringW name;
		name.Format(L"Output %02d", pmp->stream);
		if(!pmp->name.IsEmpty()) name += L" (" + CStringW(pmp->name) + L")";

		CArray<CMediaType> mts;

		CMediaType mt;
		mt.SetSampleSize(max(pmp->maxPacketSize*2, 1));

		if(pmp->mime == "video/x-pn-realvideo")
		{
			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pmp->typeSpecData.GetCount());
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), pmp->typeSpecData.GetData(), pmp->typeSpecData.GetCount());

			struct rvinfo* rvi = (struct rvinfo*)(mt.Format() + sizeof(VIDEOINFOHEADER));
			bswap(rvi->dwSize);
			bswap(rvi->w); bswap(rvi->h); bswap(rvi->bpp);
			bswap(rvi->unk1); bswap(rvi->fps); 
			bswap(rvi->type1); bswap(rvi->type2);

			ASSERT(rvi->dwSize >= FIELD_OFFSET(struct rvinfo, w2));
			ASSERT(rvi->fcc1 == 'ODIV');

			mt.majortype = MEDIATYPE_Video;
			mt.subtype = FOURCCMap(rvi->fcc2);
			mt.formattype = FORMAT_VideoInfo;
			if(rvi->fps > 0x10000) pvih->AvgTimePerFrame = 10000000i64 / ((float)rvi->fps/0x10000); 
			pvih->dwBitRate = pmp->avgBitRate; 
			pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
			pvih->bmiHeader.biWidth = rvi->w;
			pvih->bmiHeader.biHeight = rvi->h;
			pvih->bmiHeader.biPlanes = 3;
			pvih->bmiHeader.biBitCount = rvi->bpp;
			pvih->bmiHeader.biCompression = rvi->fcc2;
			pvih->bmiHeader.biSizeImage = rvi->w*rvi->h*3/2;

			mts.Add(mt);
		}
/*		else if(pmp->mime == "video/x-pn-realaudio")
		{
		}
*/
		if(mts.IsEmpty())
		{
			TRACE(_T("Unsupported RealMedia stream (%d): %s\n"), pmp->stream, CString(pmp->mime));
			continue;
		}

		HRESULT hr;
		CAutoPtr<CRealMediaSplitterOutputPin> pPinOut(new CRealMediaSplitterOutputPin(mts, name, this, this, &hr));
		if(!pPinOut) continue;

		m_mapTrackToPin[(DWORD)pmp->stream] = pPinOut;

		m_pOutputs.AddTail(pPinOut);

		m_rtStop = max(m_rtStop, 10000i64*(pmp->tStart+pmp->tDuration-pmp->tPreroll));
	}

	m_rtNewStop = m_rtStop;

	return S_OK;
}

void CRealMediaSourceFilter::DeliverBeginFlush()
{
	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverBeginFlush();
}

void CRealMediaSourceFilter::DeliverEndFlush()
{
	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverEndFlush();
	m_eEndFlush.Set();
}

DWORD CRealMediaSourceFilter::ThreadProc()
{
	if(!m_pFile)
	{
		while(1)
		{
			DWORD cmd = GetRequest();
			if(cmd == CMD_EXIT) CAMThread::m_hThread = NULL;
			Reply(S_OK);
			if(cmd == CMD_EXIT) return 0;
		}
	}

	//

	m_eEndFlush.Set();

	bool fFirstRun = true;

	while(1)
	{
		DWORD cmd = fFirstRun ? -1 : GetRequest();

		fFirstRun = false;

		if(cmd == CMD_EXIT)
		{
			CAMThread::m_hThread = NULL;
			Reply(S_OK);
			return 0;
		}

		m_rtStart = m_rtNewStart;
		m_rtStop = m_rtNewStop;
/*
		if(m_rtStart == 0)
		{
			pCluster = pSegment->Child(0x1F43B675);
		}
		else
		{
			QWORD lastCueClusterPosition = -1;

			POSITION pos1 = m_pFile->m_segment.Cues.GetHeadPosition();
			while(pos1)
			{
				Cue* pCue = m_pFile->m_segment.Cues.GetNext(pos1);
				POSITION pos2 = pCue->CuePoints.GetTailPosition();
				while(pos2)
				{
					CuePoint* pCuePoint = pCue->CuePoints.GetPrev(pos2);

					if(m_rtStart < (REFERENCE_TIME)(pCuePoint->CueTime*m_pFile->m_segment.SegmentInfo.TimeCodeScale/100))
						continue;

					POSITION pos3 = pCuePoint->CueTrackPositions.GetHeadPosition();
					while(pos3)
					{
						CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetNext(pos3);

						if(lastCueClusterPosition == pCueTrackPositions->CueClusterPosition)
							continue;

						lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

						pCluster->SeekTo(pSegment->m_start + pCueTrackPositions->CueClusterPosition);
						pCluster->Parse();

						int BlockNumber = LastBlockNumber = 0;
						bool fFoundKeyFrame = false;

						{
							Cluster c;
							c.ParseTimeCode(pCluster);

							if(CAutoPtr<CMatroskaNode> pBlocks = pCluster->Child(0xA0))
							{
								bool fPassedCueTime = false;

								do
								{
									CBlockNode blocks;
									blocks.Parse(pBlocks, false);
									POSITION pos4 = blocks.GetHeadPosition();
									while(!fPassedCueTime && pos4)
									{
										Block* b = blocks.GetNext(pos4);
										if((REFERENCE_TIME)((c.TimeCode+b->TimeCode)*m_pFile->m_segment.SegmentInfo.TimeCodeScale/100) > m_rtStart) 
										{
											fPassedCueTime = true;
										}
										else if(b->TrackNumber == pCueTrackPositions->CueTrack && b->ReferenceBlock == 0)
										{
											fFoundKeyFrame = true;
											LastBlockNumber = BlockNumber;
										}
									}

									BlockNumber++;
								}
								while(!fPassedCueTime && pBlocks->Next(true));
							}
						}

						if(fFoundKeyFrame)
							pos1 = pos2 = pos3 = NULL;
					}
				}
			}

			if(lastCueClusterPosition == -1)
			{
				pCluster = pSegment->Child(0x1F43B675);
			}
		}
*/
		if(cmd != -1)
			Reply(S_OK);

		m_eEndFlush.Wait();

		m_bDiscontinuitySent.RemoveAll();
		m_pActivePins.RemoveAll();

		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos)
		{
			CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
			if(pPin->IsConnected())
			{
				pPin->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
				m_pActivePins.AddTail(pPin);
			}
		}

		HRESULT hr = S_OK;

		// TODO: after having implemented the seeking, set "pos" in 
		// the seeking code above and start from the right packet

		pos = m_pFile->m_dcs.GetHeadPosition(); 
		while(pos && SUCCEEDED(hr) && !CheckRequest(&cmd))
		{
			DataChunk* pdc = m_pFile->m_dcs.GetNext(pos);

			m_pFile->Seek(pdc->pos);

			for(int i = 0; i < pdc->nPackets && SUCCEEDED(hr) && !CheckRequest(&cmd); i++)
			{
				MediaPacketHeader mph;
				if(S_OK != (hr = m_pFile->Read(mph)))
					break;

				CAutoPtr<RMBlock> b(new RMBlock);
				b->TrackNumber = mph.stream;
				b->bSyncPoint = !!(mph.flags&MediaPacketHeader::PN_KEYFRAME_FLAG);
				b->rtStart = 10000i64*(mph.tStart /*- preload + start*/);
				b->rtStop = b->rtStart+1;
				b->pData.Copy(mph.pData); // yea, I know...

				hr = DeliverBlock(b);
			}
		}

		pos = m_pActivePins.GetHeadPosition();
		while(pos && !CheckRequest(&cmd))
			m_pActivePins.GetNext(pos)->DeliverEndOfStream();
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
}

HRESULT CRealMediaSourceFilter::DeliverBlock(CAutoPtr<RMBlock> b)
{
	HRESULT hr = S_FALSE;

	CRealMediaSplitterOutputPin* pPin = NULL;
	if(!m_mapTrackToPin.Lookup(b->TrackNumber, pPin) || !pPin 
	|| !pPin->IsConnected() || !m_pActivePins.Find(pPin))
		return S_FALSE;

	if(b->pData.GetCount() == 0)
		return S_FALSE;

	REFERENCE_TIME 
		rtStart = b->rtStart - m_rtStart, 
		rtStop = b->rtStop - m_rtStart;

	ASSERT(rtStart < rtStop);

	m_rtCurrent = m_rtStart + rtStart;

	DWORD TrackNumber = b->TrackNumber;
	BOOL bDiscontinuity = !m_bDiscontinuitySent.Find(TrackNumber);

//TRACE(_T("pPin->DeliverBlock: TrackNumber (%d) %I64d, %I64d\n"), (int)TrackNumber, rtStart, rtStop);

	hr = pPin->DeliverBlock(b, bDiscontinuity);

	if(S_OK != hr)
	{
		if(POSITION pos = m_pActivePins.Find(pPin))
			m_pActivePins.RemoveAt(pos);

		if(!m_pActivePins.IsEmpty()) // only die when all pins are down
			hr = S_OK;

		return hr;
	}

	if(bDiscontinuity)
		m_bDiscontinuitySent.AddTail(TrackNumber);

	return hr;
}

HRESULT CRealMediaSourceFilter::BreakConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CRealMediaSplitterInputPin* pIn = (CRealMediaSplitterInputPin*)pPin;

		// TODO: do something here!!!
/*
		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos) m_pOutputs.GetNext(pos)->Disconnect();
		m_pOutputs.RemoveAll();
*/
//		m_pFile.Free();
	}
	else if(dir == PINDIR_OUTPUT)
	{
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT CRealMediaSourceFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CRealMediaSplitterInputPin* pIn = (CRealMediaSplitterInputPin*)pPin;

		HRESULT hr;

		CComPtr<IAsyncReader> pAsyncReader;
		if(FAILED(hr = pIn->GetAsyncReader(&pAsyncReader))
		|| FAILED(hr = CreateOutputs(pAsyncReader)))
			return hr;
	}
	else if(dir == PINDIR_OUTPUT)
	{
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

int CRealMediaSourceFilter::GetPinCount()
{
	return (m_pInput ? 1 : 0) + m_pOutputs.GetCount();
}

CBasePin* CRealMediaSourceFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n >= 0 && n < (int)m_pOutputs.GetCount())
	{
		if(POSITION pos = m_pOutputs.FindIndex(n))
			return m_pOutputs.GetAt(pos);
	}

	if(n == m_pOutputs.GetCount() && m_pInput)
	{
		return m_pInput;
	}

	return NULL;
}

STDMETHODIMP CRealMediaSourceFilter::Stop()
{
	CAutoLock cAutoLock(this);

	DeliverBeginFlush();
	CallWorker(CMD_EXIT);
	DeliverEndFlush();

	HRESULT hr;
	if(FAILED(hr = __super::Stop()))
		return hr;

	return S_OK;
}

STDMETHODIMP CRealMediaSourceFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr;
	if(FAILED(hr = __super::Pause()))
		return hr;

	if(fs == State_Stopped)
	{
		Create();
	}

	return S_OK;
}

STDMETHODIMP CRealMediaSourceFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;
	if(FAILED(hr = __super::Run(tStart)))
		return hr;

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CRealMediaSourceFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CheckPointer(pszFileName, E_POINTER);

	HRESULT hr;
	CComPtr<IAsyncReader> pAsyncReader = (IAsyncReader*)new CFileReader(CString(pszFileName), hr);
	if(FAILED(hr)) return hr;

	if(FAILED(hr = CreateOutputs(pAsyncReader)))
		return hr;

	m_fn = pszFileName;

	return S_OK;
}

STDMETHODIMP CRealMediaSourceFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
		return E_OUTOFMEMORY;
	wcscpy(*ppszFileName, m_fn);
	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CRealMediaSourceFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = 
		AM_SEEKING_CanGetStopPos|
		AM_SEEKING_CanGetDuration/*|
		AM_SEEKING_CanSeekAbsolute|
		AM_SEEKING_CanSeekForwards|
		AM_SEEKING_CanSeekBackwards*/, S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaSourceFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;
	GetCapabilities(&caps);

	DWORD caps2 = caps & *pCapabilities;

	return caps2 == 0 ? E_FAIL : caps2 == *pCapabilities ? S_OK : S_FALSE;
}
STDMETHODIMP CRealMediaSourceFilter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CRealMediaSourceFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CRealMediaSourceFilter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CRealMediaSourceFilter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CRealMediaSourceFilter::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
STDMETHODIMP CRealMediaSourceFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	*pDuration = 10000i64*m_pFile->m_p.tDuration;

	return S_OK;
}
STDMETHODIMP CRealMediaSourceFilter::GetStopPosition(LONGLONG* pStop) {return GetDuration(pStop);}
STDMETHODIMP CRealMediaSourceFilter::GetCurrentPosition(LONGLONG* pCurrent) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaSourceFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaSourceFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
return E_NOTIMPL;
	CAutoLock cAutoLock(this);

	if(!pCurrent && !pStop
	|| (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
		return S_OK;

	REFERENCE_TIME 
		rtCurrent = m_rtCurrent,
		rtStop = m_rtStop;

	if(pCurrent)
	switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
	case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
	case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
	}

	if(pStop)
	switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
	case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
	case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
	}

	if(m_rtCurrent == rtCurrent && m_rtStop == rtStop)
		return S_OK;

	m_rtNewStart = m_rtCurrent = rtCurrent;
	m_rtNewStop = rtStop;

	if(ThreadExists())
	{
		DeliverBeginFlush();
		CallWorker(CMD_SEEK);
		DeliverEndFlush();		
	}

	return S_OK;
}
STDMETHODIMP CRealMediaSourceFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) *pCurrent = m_rtCurrent;
	if(pStop) *pStop = m_rtStop;
	return S_OK;
}
STDMETHODIMP CRealMediaSourceFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest) *pEarliest = 0;
	return GetDuration(pLatest);
}
STDMETHODIMP CRealMediaSourceFilter::SetRate(double dRate) {return dRate == 1.0 ? S_OK : E_INVALIDARG;}
STDMETHODIMP CRealMediaSourceFilter::GetRate(double* pdRate) {return pdRate ? *pdRate = m_dRate, S_OK : E_POINTER;}
STDMETHODIMP CRealMediaSourceFilter::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}
/*
// IAMOpenProgress

STDMETHODIMP CRealMediaSourceFilter::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	CheckPointer(pllTotal, E_POINTER);
	CheckPointer(pllCurrent, E_POINTER);

	*pllTotal = 100;
	*pllCurrent = m_nOpenProgress;

	return S_OK;
}

STDMETHODIMP CRealMediaSourceFilter::AbortOperation()
{
	m_fAbort = true;
	return S_OK;
}
*/
//
// CRealMediaSplitterFilter
//

CRealMediaSplitterFilter::CRealMediaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRealMediaSourceFilter(pUnk, phr)
{
	m_pInput.Attach(new CRealMediaSplitterInputPin(NAME("CRealMediaSplitterInputPin"), this, this, phr));
}

//
// CRealMediaSplitterInputPin
//

CRealMediaSplitterInputPin::CRealMediaSplitterInputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBasePin(pName, pFilter, pLock, phr, L"Input", PINDIR_INPUT)
{
}

CRealMediaSplitterInputPin::~CRealMediaSplitterInputPin()
{
}

HRESULT CRealMediaSplitterInputPin::GetAsyncReader(IAsyncReader** ppAsyncReader)
{
	CheckPointer(ppAsyncReader, E_POINTER);
	*ppAsyncReader = NULL;
	CheckPointer(m_pAsyncReader, VFW_E_NOT_CONNECTED);
	(*ppAsyncReader = m_pAsyncReader)->AddRef();
	return S_OK;
}

STDMETHODIMP CRealMediaSplitterInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRealMediaSplitterInputPin::CheckMediaType(const CMediaType* pmt)
{
	return S_OK;
/*
	return pmt->majortype == MEDIATYPE_Stream
		? S_OK
		: E_INVALIDARG;
*/
}

HRESULT CRealMediaSplitterInputPin::CheckConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CheckConnect(pPin)))
		return hr;

	if(CComQIPtr<IAsyncReader> pAsyncReader = pPin)
	{
		DWORD dw;
		hr = S_OK == pAsyncReader->SyncRead(0, 4, (BYTE*)&dw) && dw == 'FMR.'
			? S_OK 
			: E_FAIL;
	}
	else
	{
		hr = E_NOINTERFACE;
	}

	return hr;
}

HRESULT CRealMediaSplitterInputPin::BreakConnect()
{
	HRESULT hr;

	if(FAILED(hr = __super::BreakConnect()))
		return hr;

	if(FAILED(hr = ((CRealMediaSourceFilter*)m_pFilter)->BreakConnect(PINDIR_INPUT, this)))
		return hr;

	m_pAsyncReader.Release();

	return S_OK;
}

HRESULT CRealMediaSplitterInputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CompleteConnect(pPin)))
		return hr;

	CheckPointer(pPin, E_POINTER);
	m_pAsyncReader = pPin;
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	if(FAILED(hr = ((CRealMediaSourceFilter*)m_pFilter)->CompleteConnect(PINDIR_INPUT, this)))
		return hr;

	return S_OK;
}

STDMETHODIMP CRealMediaSplitterInputPin::BeginFlush()
{
	return E_UNEXPECTED;
}

STDMETHODIMP CRealMediaSplitterInputPin::EndFlush()
{
	return E_UNEXPECTED;
}

//
// CRealMediaSplitterOutputPin
//

CRealMediaSplitterOutputPin::CRealMediaSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(NAME("CRealMediaSplitterOutputPin"), pFilter, pLock, phr, pName)
{
	m_mts.Copy(mts);
}

CRealMediaSplitterOutputPin::~CRealMediaSplitterOutputPin()
{
}

STDMETHODIMP CRealMediaSplitterOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		riid == __uuidof(IMediaSeeking) ? m_pFilter->QueryInterface(riid, ppv) : 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CRealMediaSplitterOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    HRESULT hr = NOERROR;

	pProperties->cBuffers = MAXBUFFERS;
	pProperties->cbBuffer = m_mt.GetSampleSize();

    ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
    ASSERT(Actual.cBuffers == pProperties->cBuffers);

    return NOERROR;
}

HRESULT CRealMediaSplitterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	for(int i = 0; i < m_mts.GetCount(); i++)
	{
		if(pmt->majortype == m_mts[i].majortype && pmt->subtype == m_mts[i].subtype)
		{
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

HRESULT CRealMediaSplitterOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= m_mts.GetCount()) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mts[iPosition];

	return S_OK;
}

STDMETHODIMP CRealMediaSplitterOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}

//

HRESULT CRealMediaSplitterOutputPin::Active()
{
    CAutoLock cAutoLock(m_pLock);

	if(m_Connected) 
		Create();

	return __super::Active();
}

HRESULT CRealMediaSplitterOutputPin::Inactive()
{
    CAutoLock cAutoLock(m_pLock);

	if(ThreadExists())
		CallWorker(CMD_EXIT);

	return __super::Inactive();
}

void CRealMediaSplitterOutputPin::DontGoWild()
{
	int cnt = 0;
	do
	{
		if(cnt > MAXPACKETS) Sleep(1);
		CAutoLock cAutoLock(&m_csQueueLock);
		cnt = m_packets.GetCount();
	}
	while(S_OK == m_hrDeliver && cnt > MAXPACKETS);
}

HRESULT CRealMediaSplitterOutputPin::DeliverEndOfStream()
{
	if(!ThreadExists()) return S_FALSE;

	DontGoWild();
	if(S_OK != m_hrDeliver) return m_hrDeliver;

	CAutoLock cAutoLock(&m_csQueueLock);
	CAutoPtr<packet> p(new packet());
	p->type = EOS;
	m_packets.AddHead(p);

	return m_hrDeliver;
}

HRESULT CRealMediaSplitterOutputPin::DeliverBeginFlush()
{
	CAutoLock cAutoLock(&m_csQueueLock);
	m_packets.RemoveAll();
	m_hrDeliver = S_FALSE;
	HRESULT hr = IsConnected() ? GetConnected()->BeginFlush() : S_OK;
	return hr;
}

HRESULT CRealMediaSplitterOutputPin::DeliverEndFlush()
{
	if(!ThreadExists()) return S_FALSE;
	HRESULT hr = IsConnected() ? GetConnected()->EndFlush() : S_OK;
	m_hrDeliver = S_OK;
	return hr;
}

HRESULT CRealMediaSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if(!ThreadExists()) return S_FALSE;
	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CRealMediaSplitterOutputPin::DeliverBlock(CAutoPtr<RMBlock> b, BOOL bDiscontinuity)
{
	if(!ThreadExists()) return S_FALSE;

	DontGoWild();
	if(S_OK != m_hrDeliver) return m_hrDeliver;

	CAutoLock cAutoLock(&m_csQueueLock);
	CAutoPtr<packet> p(new packet());
	p->type = BLOCK;
	p->b = b;
	p->bDiscontinuity = bDiscontinuity;
	m_packets.AddHead(p);

	return m_hrDeliver;
}

DWORD CRealMediaSplitterOutputPin::ThreadProc()
{
	m_hrDeliver = S_OK;

	::SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	while(1)
	{
		Sleep(1);

		DWORD cmd;
		if(CheckRequest(&cmd))
		{
			m_hThread = NULL;
			cmd = GetRequest();
			Reply(S_OK);
			ASSERT(cmd == CMD_EXIT);
			return 0;
		}

		int cnt = 0;
		do
		{
			CAutoPtr<packet> p;

			{
				CAutoLock cAutoLock(&m_csQueueLock);
				cnt = m_packets.GetCount();
				if(cnt > 0) {p = m_packets.RemoveTail(); cnt--;}
			}

			if(S_OK != m_hrDeliver) 
				continue;

			if(p && p->type == EOS)
			{
				HRESULT hr = GetConnected()->EndOfStream();
				if(hr != S_OK)
				{
					CAutoLock cAutoLock(&m_csQueueLock);
					m_hrDeliver = hr;
				}
			}
			else if(p && p->type == BLOCK)
			{
				HRESULT hr = S_FALSE;
				
				ASSERT(p->b->rtStart < p->b->rtStop);

				CComPtr<IMediaSample> pSample;
				BYTE* pData;
				if(S_OK != (hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0))
				|| S_OK != (hr = pSample->GetPointer(&pData))
				|| (hr = (memcpy(pData, p->b->pData.GetData(), p->b->pData.GetCount()) ? S_OK : E_FAIL))
				|| S_OK != (hr = pSample->SetActualDataLength(p->b->pData.GetCount()))
				|| S_OK != (hr = pSample->SetTime(&p->b->rtStart, &p->b->rtStop))
				|| S_OK != (hr = pSample->SetMediaTime(NULL, NULL))
				|| S_OK != (hr = pSample->SetDiscontinuity(p->bDiscontinuity))
				|| S_OK != (hr = pSample->SetSyncPoint(p->b->bSyncPoint))
				|| S_OK != (hr = pSample->SetPreroll(p->b->rtStart < 0))
				|| S_OK != (hr = Deliver(pSample)))
				{
					CAutoLock cAutoLock(&m_csQueueLock);
					m_hrDeliver = hr;
				}
			}
		}
		while(cnt > 0);
	}
}

//
// CFileReader
//

CRealMediaSourceFilter::CFileReader::CFileReader(CString fn, HRESULT& hr) : CUnknown(NAME(""), NULL, &hr)
{
	hr = m_file.Open(fn, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary) ? S_OK : E_FAIL;
}

STDMETHODIMP CRealMediaSourceFilter::CFileReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IAsyncReader)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CRealMediaSourceFilter::CFileReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	if(llPosition != m_file.Seek(llPosition, CFile::begin)) return E_FAIL;
	if((UINT)lLength < m_file.Read(pBuffer, lLength)) return S_FALSE;
	return S_OK;
}

STDMETHODIMP CRealMediaSourceFilter::CFileReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	if(pTotal) *pTotal = m_file.GetLength();
	if(pAvailable) *pAvailable = m_file.GetLength();
	return S_OK;
}

//
// CRMFile
//

CRMFile::CRMFile(IAsyncReader* pReader, HRESULT& hr) 
	: m_pReader(pReader)
	, m_pos(0), m_len(0)
{
	LONGLONG total = 0, available;
	pReader->Length(&total, &available);
	m_len = total;

	hr = Init();
}

HRESULT CRMFile::Read(BYTE* pData, LONG len)
{
	HRESULT hr = m_pReader->SyncRead(m_pos, len, pData);
	m_pos += len;
	return hr;
}

template<typename T> 
HRESULT CRMFile::Read(T& var)
{
	HRESULT hr = Read((BYTE*)&var, sizeof(var));
	bswap(var);
	return hr;
}

HRESULT CRMFile::Read(ChunkHdr& hdr)
{
	memset(&hdr, 0, sizeof(hdr));
	HRESULT hr;
	if(S_OK != (hr = Read(hdr.object_id))
	|| S_OK != (hr = Read(hdr.size))
	|| S_OK != (hr = Read(hdr.object_version)))
		return hr;
	return S_OK;
}

HRESULT CRMFile::Read(MediaPacketHeader& mph)
{
	memset(&mph, 0, FIELD_OFFSET(MediaPacketHeader, pData));

	HRESULT hr;

	UINT16 object_version;
	if(S_OK != (hr = Read(object_version))) return hr;
	if(object_version != 0) return S_OK;

	UINT8 flags;
	if(S_OK != (hr = Read(mph.len))
	|| S_OK != (hr = Read(mph.stream))
	|| S_OK != (hr = Read(mph.tStart))
	|| S_OK != (hr = Read(mph.reserved))
	|| S_OK != (hr = Read(flags)))
		return hr;
	mph.flags = (MediaPacketHeader::flag_t)flags;
	LONG len = mph.len;
	len -= sizeof(object_version);
	len -= FIELD_OFFSET(MediaPacketHeader, flags);
	len -= sizeof(flags);
	ASSERT(len >= 0);
	mph.pData.SetSize(len);
	if(mph.len > 0 && S_OK != (hr = Read(mph.pData.GetData(), len)))
		return hr;

	return S_OK;
}


HRESULT CRMFile::Init()
{
	if(!m_pReader) return E_UNEXPECTED;

	HRESULT hr;

	ChunkHdr hdr;
	while(m_pos < m_len && S_OK == (hr = Read(hdr)))
	{
		UINT64 pos = m_pos - sizeof(hdr);

		if(pos + hdr.size > m_len && hdr.object_id != 'DATA') // truncated?
			break;

		if(hdr.object_version == 0)
		{
			switch(hdr.object_id)
			{
			case '.RMF':
				if(S_OK != (hr = Read(m_fh.version))) return hr;
				if(S_OK != (hr = Read(m_fh.nHeaders))) return hr;
				break;
			case 'CONT':
				UINT16 slen;
				if(S_OK != (hr = Read(slen))) return hr;
				if(slen > 0 && S_OK != (hr = Read((BYTE*)m_cd.title.GetBufferSetLength(slen), slen))) return hr;
				if(S_OK != (hr = Read(slen))) return hr;
				if(slen > 0 && S_OK != (hr = Read((BYTE*)m_cd.author.GetBufferSetLength(slen), slen))) return hr;
				if(S_OK != (hr = Read(slen))) return hr;
				if(slen > 0 && S_OK != (hr = Read((BYTE*)m_cd.copyright.GetBufferSetLength(slen), slen))) return hr;
				if(S_OK != (hr = Read(slen))) return hr;
				if(slen > 0 && S_OK != (hr = Read((BYTE*)m_cd.comment.GetBufferSetLength(slen), slen))) return hr;
				break;
			case 'PROP':
				if(S_OK != (hr = Read(m_p.maxBitRate))) return hr;
				if(S_OK != (hr = Read(m_p.avgBitRate))) return hr;
				if(S_OK != (hr = Read(m_p.maxPacketSize))) return hr;
				if(S_OK != (hr = Read(m_p.avgPacketSize))) return hr;
				if(S_OK != (hr = Read(m_p.nPackets))) return hr;
				if(S_OK != (hr = Read(m_p.tDuration))) return hr;
				if(S_OK != (hr = Read(m_p.tPreroll))) return hr;
				if(S_OK != (hr = Read(m_p.ptrIndex))) return hr;
				if(S_OK != (hr = Read(m_p.ptrData))) return hr;
				if(S_OK != (hr = Read(m_p.nStreams))) return hr;
				UINT16 flags;
				if(S_OK != (hr = Read(flags))) return hr;
				m_p.flags = (Properies::flags_t)flags;
				break;
			case 'MDPR':
				{
				CAutoPtr<MediaProperies> mp(new MediaProperies);
				if(S_OK != (hr = Read(mp->stream))) return hr;
				if(S_OK != (hr = Read(mp->maxBitRate))) return hr;
				if(S_OK != (hr = Read(mp->avgBitRate))) return hr;
				if(S_OK != (hr = Read(mp->maxPacketSize))) return hr;
				if(S_OK != (hr = Read(mp->avgPacketSize))) return hr;
				if(S_OK != (hr = Read(mp->tStart))) return hr;
				if(S_OK != (hr = Read(mp->tPreroll))) return hr;
				if(S_OK != (hr = Read(mp->tDuration))) return hr;
				UINT8 slen;
				if(S_OK != (hr = Read(slen))) return hr;
				if(slen > 0 && S_OK != (hr = Read((BYTE*)mp->name.GetBufferSetLength(slen), slen))) return hr;
				if(S_OK != (hr = Read(slen))) return hr;
				if(slen > 0 && S_OK != (hr = Read((BYTE*)mp->mime.GetBufferSetLength(slen), slen))) return hr;
				UINT32 tsdlen;
				if(S_OK != (hr = Read(tsdlen))) return hr;
				mp->typeSpecData.SetSize(tsdlen);
				if(tsdlen > 0 && S_OK != (hr = Read(mp->typeSpecData.GetData(), tsdlen))) return hr;
				m_mps.AddTail(mp);
				break;
				}
			case 'DATA':
				{
				CAutoPtr<DataChunk> dc(new DataChunk);
				if(S_OK != (hr = Read(dc->nPackets))) return hr;
				if(S_OK != (hr = Read(dc->ptrNext))) return hr;
				dc->pos = m_pos;
				m_dcs.AddTail(dc);
				break;
				}
			case 'INDX':
				{
				IndexChunkHeader ich;
				if(S_OK != (hr = Read(ich.nIndices))) return hr;
				if(S_OK != (hr = Read(ich.stream))) return hr;
				if(S_OK != (hr = Read(ich.ptrNext))) return hr;
				while(ich.nIndices-- > 0)
				{
					UINT16 object_version;
					if(S_OK != (hr = Read(object_version))) return hr;
					if(object_version == 0)
					{
						CAutoPtr<IndexRecord> ir(new IndexRecord);
						if(S_OK != (hr = Read(ir->tStart))) return hr;
						if(S_OK != (hr = Read(ir->ptrFilePos))) return hr;
						if(S_OK != (hr = Read(ir->packet))) return hr;
						m_irs.AddTail(ir);
					}
				}
				break;
				}
			}
		}

		ASSERT(hdr.object_id == 'DATA' 
			|| m_pos == pos + hdr.size 
			|| m_pos == pos + sizeof(hdr));

		pos += hdr.size;
		if(pos > m_pos) 
			Seek(pos);
	}

	return S_OK;
}

////////////////////////////

//
// CRealVideoDecoder
//

CRealVideoDecoder::CRealVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(NAME("CRealVideoDecoder"), lpunk, __uuidof(this))
	, m_dwCookie(0)
{
	if(phr) *phr = S_OK;
}

CRealVideoDecoder::~CRealVideoDecoder()
{
	if(m_hDrvDll) FreeLibrary(m_hDrvDll);
}

#ifdef REGISTER_FILTER
CUnknown* WINAPI CRealVideoDecoder::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CRealVideoDecoder(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}
#endif

HRESULT CRealVideoDecoder::Receive(IMediaSample* pIn)
{
    HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	//

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;
	BYTE* pDataInOrg = pDataIn;

	long len = pIn->GetActualDataLength();
	if(len <= 0) return S_OK; // nothing to do

	REFERENCE_TIME rtStart, rtStop;
	pIn->GetTime(&rtStart, &rtStop);

	rtStart += m_tStart;

	TRACE(_T("in=%04x, start=%I64d, stop=%I64d\n"), len, rtStart, rtStop);

	if(m_rtLast < rtStart)
	{
		if(S_OK != (hr = Decode(false)))
			return hr;
	}

	m_rtLast = rtStart;

	BYTE hdr = *pDataIn++, subseq = 0, seqnum = 0;
	DWORD packetoffset = 0;

	m_packetlen = 0;

	if((hdr&0xc0) == 0x40)
	{
		pDataIn++;
	}
	else
	{
		if((hdr&0x40) == 0)
			subseq = (*pDataIn++)&0x7f;

		#define GetWORD(var) \
			var = (var<<8)|(*pDataIn++); \
			var = (var<<8)|(*pDataIn++); \

		GetWORD(m_packetlen);
		if((m_packetlen&0xc000) == 0) {GetWORD(m_packetlen);}
		else m_packetlen &= 0x3fff;

		GetWORD(packetoffset);
		if((packetoffset&0xc000) == 0) {GetWORD(packetoffset);}
		else packetoffset &= 0x3fff;

		if((hdr&0xc0) == 0x80)
			packetoffset = m_packetlen - packetoffset;

		seqnum = *pDataIn++;
	}

	if(subseq == 0 && (hdr&0xc0) == 0xc0)
	{
		TRACE(_T("!@#$%^&*: packetoffset=%04x for a starting packet!!!\n"), packetoffset);
		packetoffset = 0;
	}

	TRACE(_T("hdr=%02x, subseq=%d, seqnum=%d, packetlen=%04x, packetoffset=%04x\n"), hdr, subseq, seqnum, m_packetlen, packetoffset);

	len = min(len - (pDataIn - pDataInOrg), m_packetlen);

	CAutoPtr<chunk> c(new chunk);
	c->offset = packetoffset;
	c->data.SetSize(len);
	memcpy(c->data.GetData(), pDataIn, len);
	m_data.AddTail(c);

	if((hdr&0x80) /*|| packetoffset+len >= packetlen*/)
	{
		if(S_OK != (hr = Decode(pIn->IsPreroll() == S_OK)))
			return hr;
	}

	return S_OK;
}

HRESULT CRealVideoDecoder::Decode(bool fPreLoad)
{
	if(m_data.IsEmpty() || m_packetlen == 0)
		return S_OK;

	HRESULT hr;

    CComPtr<IMediaSample> pOut;
	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0)))
		return hr;

	BYTE* pDataOut = NULL;
	if(FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	CAutoVectorPtr<BYTE> p;
	p.Allocate(m_packetlen);

	CArray<DWORD> extra;

	POSITION pos = m_data.GetHeadPosition();
	while(pos)
	{
		chunk* c = m_data.GetNext(pos);
		memcpy((BYTE*)p + c->offset, c->data.GetData(), min(m_packetlen - c->offset, c->data.GetSize()));
		extra.Add(1);
		extra.Add(c->offset);
	}

	#pragma pack(push, 1)
	struct {DWORD len, unk1, chunks; DWORD* extra; DWORD unk2, timestamp;} transform_in = 
		{m_packetlen, 0, extra.GetCount()/2-1, extra.GetData(), 0, (DWORD)(m_rtLast/10000)};
	struct {DWORD unk1, unk2, timestamp, w, h;} transform_out = 
		{0,0,0,0,0};
	#pragma pack(pop)

	hr = RV20toYUV420Transform(p, (BYTE*)m_pI420FrameBuff, &transform_in, &transform_out, m_dwCookie);

	m_data.RemoveAll();

	if(FAILED(hr))
	{
		TRACE(_T("RV returned an error code!!!\n"));
//		return hr;
	}

	REFERENCE_TIME rtStart = 10000i64*transform_out.timestamp - m_tStart;
	REFERENCE_TIME rtStop = rtStart + 1;
	pOut->SetTime(&rtStart, NULL/*&rtStop*/);

	Copy(m_pI420FrameBuff, pDataOut, transform_out.w, transform_out.h);

	return fPreLoad ? S_OK : m_pOutput->Deliver(pOut);
}

void CRealVideoDecoder::Copy(BYTE* pIn, BYTE* pOut, DWORD w, DWORD h)
{
	BITMAPINFOHEADER& bihIn = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->bmiHeader;
	BITMAPINFOHEADER& bihOut = ((VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format())->bmiHeader;

	int pitchIn = w;
	int pitchInUV = pitchIn>>1;
	BYTE* pInU = pIn + pitchIn*h;
	BYTE* pInV = pInU + pitchInUV*h/2;

	if(bihOut.biCompression == '2YUY')
	{
		int pitchOut = bihOut.biWidth*2;

		for(int y = 0; y < h; y+=2, pIn += pitchIn*2, pInU += pitchInUV, pInV += pitchInUV, pOut += pitchOut*2)
		{
			BYTE* pDataIn = pIn;
			BYTE* pDataInU = pInU;
			BYTE* pDataInV = pInV;
			WORD* pDataOut = (WORD*)pOut;

			for(int x = 0; x < w; x+=2)
			{
				*pDataOut++ = (*pDataInU++<<8)|*pDataIn++;
				*pDataOut++ = (*pDataInV++<<8)|*pDataIn++;
			}

			pDataIn = pIn + pitchIn;
			pDataInU = pInU;
			pDataInV = pInV;
			pDataOut = (WORD*)(pOut + pitchOut);

			if(y < h-2)
			{
				for(int x = 0; x < w; x+=2, pDataInU++, pDataInV++)
				{
					*pDataOut++ = (((pDataInU[0]+pDataInU[pitchInUV])>>1)<<8)|*pDataIn++;
					*pDataOut++ = (((pDataInV[0]+pDataInV[pitchInUV])>>1)<<8)|*pDataIn++;
				}
			}
			else
			{
				for(int x = 0; x < w; x+=2)
				{
					*pDataOut++ = (*pDataInU++<<8)|*pDataIn++;
					*pDataOut++ = (*pDataInV++<<8)|*pDataIn++;
				}
			}
		}
	}
	else if(bihOut.biCompression == '21VY')
	{
		int pitchOut = bihOut.biWidth;

		for(int y = 0; y < h; y++, pIn += pitchIn, pOut += pitchOut)
		{
			memcpy(pOut, pIn, min(pitchIn, pitchOut));
		}

		pitchIn >>= 1;
		pitchOut >>= 1;

		pIn = pInV;

		for(int y = 0; y < h; y+=2, pIn += pitchIn, pOut += pitchOut)
		{
			memcpy(pOut, pIn, min(pitchIn, pitchOut));
		}

		pIn = pInU;

		for(int y = 0; y < h; y+=2, pIn += pitchIn, pOut += pitchOut)
		{
			memcpy(pOut, pIn, min(pitchIn, pitchOut));
		}
	}
}


HRESULT CRealVideoDecoder::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->majortype != MEDIATYPE_Video 
	|| mtIn->subtype != FOURCCMap('02VR') 
	&& mtIn->subtype != FOURCCMap('03VR') 
	&& mtIn->subtype != FOURCCMap('04VR'))
		return VFW_E_TYPE_NOT_ACCEPTED;

	if(!m_pInput->IsConnected())
	{
		if(m_hDrvDll) {FreeLibrary(m_hDrvDll); m_hDrvDll = NULL;}

		CString fn;
		fn.Format(_T("drv%c3260.dll"), (TCHAR)((mtIn->subtype.Data1>>16)&0xff));

		CStringList paths;
		paths.AddTail(fn);

		CRegKey key;
		TCHAR buff[MAX_PATH];
		ULONG len = sizeof(buff);
		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Software\\RealNetworks\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			CString path(buff);
			TCHAR c = path[path.GetLength()-1];
			if(c != '\\' && c != '/') path += '\\';
			paths.AddTail(path + fn);
		}

		POSITION pos = paths.GetHeadPosition();
		while(pos && !(m_hDrvDll = LoadLibrary(paths.GetNext(pos))));

		if(m_hDrvDll)
		{
			RV20toYUV420CustomMessage = (PRV20toYUV420CustomMessage)GetProcAddress(m_hDrvDll, "RV20toYUV420CustomMessage");
			RV20toYUV420Free = (PRV20toYUV420Free)GetProcAddress(m_hDrvDll, "RV20toYUV420Free");
			RV20toYUV420HiveMessage = (PRV20toYUV420HiveMessage)GetProcAddress(m_hDrvDll, "RV20toYUV420HiveMessage");
			RV20toYUV420Init = (PRV20toYUV420Init)GetProcAddress(m_hDrvDll, "RV20toYUV420Init");
			RV20toYUV420Transform = (PRV20toYUV420Transform)GetProcAddress(m_hDrvDll, "RV20toYUV420Transform");
		}

		if(!m_hDrvDll || !RV20toYUV420CustomMessage 
		|| !RV20toYUV420Free || !RV20toYUV420HiveMessage
		|| !RV20toYUV420Init || !RV20toYUV420Transform)
			return VFW_E_TYPE_NOT_ACCEPTED;

		struct rvinfo* rvi = (struct rvinfo*)(mtIn->Format() + sizeof(VIDEOINFOHEADER));
		struct rv_init_t i = {11, rvi->w, rvi->h, 0, 0, rvi->type1, 1, rvi->type2};

		if(FAILED(RV20toYUV420Init(&i, &m_dwCookie)))
			return VFW_E_TYPE_NOT_ACCEPTED;

		if(rvi->fcc2 <= '03VR' && rvi->type2 >= 0x20200002)
		{
			UINT32 cmsg24[6] = {rvi->w, rvi->h, rvi->w2*4, rvi->h2*4, rvi->w3*4, rvi->h3*4};
			cmsg_data_t cmsg_data = {0x24, 1+((rvi->type1>>16)&7), cmsg24};
			HRESULT hr = RV20toYUV420CustomMessage(&cmsg_data, m_dwCookie);
			hr = hr;
		}
	}

/*
	if(rvi->fcc2 <= 0x30335652 && rvi->type2 >= 0x20200002)
	{
		UINT32 cmsg24[4] = {rvi->w, rvi->h, rvi->type1, rvi->type2};
	    cmsg_data_t cmsg_data={0x24,1+((extrahdr[0]>>16)&7), &cmsg24[0]};
	}
	if((sh->format<=0x30335652) && (extrahdr[1]>=0x20200002)){
	    uint32_t cmsg24[4]={sh->disp_w,sh->disp_h,((unsigned short *)extrahdr)[4],((unsigned short *)extrahdr)[5]};
	    cmsg_data_t cmsg_data={0x24,1+((extrahdr[0]>>16)&7), &cmsg24[0]};

		(*wrvyuv_custom_message)(&cmsg_data,sh->context);
*/

	return S_OK;
}

HRESULT CRealVideoDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return mtIn->majortype == MEDIATYPE_Video && (mtIn->subtype == FOURCCMap('02VR') || mtIn->subtype == FOURCCMap('03VR') || mtIn->subtype == FOURCCMap('04VR'))
		&& mtOut->majortype == MEDIATYPE_Video && (mtOut->subtype == FOURCCMap('21VY') || mtOut->subtype == FOURCCMap('2YUY'))
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CRealVideoDecoder::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CComPtr<IMemAllocator> pAllocatorIn;
	m_pInput->GetAllocator(&pAllocatorIn);
	if(!pAllocatorIn) return E_UNEXPECTED;

	BITMAPINFOHEADER& bih = ((VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format())->bmiHeader;

	pProperties->cBuffers = 2;
	pProperties->cbBuffer = bih.biSizeImage;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR);
}

HRESULT CRealVideoDecoder::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 1) return VFW_S_NO_MORE_ITEMS;

	BITMAPINFOHEADER& bih = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->bmiHeader;

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = iPosition == 0 ? MEDIASUBTYPE_YUY2 : MEDIASUBTYPE_YV12;
	pmt->formattype = FORMAT_VideoInfo;
	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	memset(vih, 0, sizeof(VIDEOINFOHEADER));
	vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
	vih->bmiHeader.biWidth = bih.biWidth;
	vih->bmiHeader.biHeight = bih.biHeight;
	vih->bmiHeader.biPlanes = iPosition == 0 ? 1 : 3;
	vih->bmiHeader.biBitCount = iPosition == 0 ? 16 : 12;
	vih->bmiHeader.biCompression = iPosition == 0 ? '2YUY' : '21VY';
	vih->bmiHeader.biSizeImage = bih.biWidth*bih.biHeight*vih->bmiHeader.biBitCount>>3;

	return S_OK;
}

HRESULT CRealVideoDecoder::StartStreaming()
{
	if(m_dwCookie)
	{
		RV20toYUV420Free(m_dwCookie);
		m_dwCookie = 0;
	}

	struct rvinfo* rvi = (struct rvinfo*)(m_pInput->CurrentMediaType().Format() + sizeof(VIDEOINFOHEADER));
	struct rv_init_t i = {11, rvi->w, rvi->h, 0, 0, rvi->type1, 1, rvi->type2};

	if(FAILED(RV20toYUV420Init(&i, &m_dwCookie)))
		return E_FAIL;

	m_packetlen = 0;
	m_rtLast = 0;
	m_data.RemoveAll();

	BITMAPINFOHEADER& bih = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->bmiHeader;
	m_pI420FrameBuff.Allocate(bih.biWidth*bih.biHeight*3/2);

	return __super::StartStreaming();
}

HRESULT CRealVideoDecoder::StopStreaming()
{
	m_pI420FrameBuff.Free();

	if(m_dwCookie)
	{
		RV20toYUV420Free(m_dwCookie);
		m_dwCookie = 0;
	}

	return __super::StopStreaming();
}

HRESULT CRealVideoDecoder::EndOfStream()
{
	return __super::EndOfStream();
}

HRESULT CRealVideoDecoder::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CRealVideoDecoder::EndFlush()
{
	m_packetlen = 0;
	m_rtLast = 0;
	m_data.RemoveAll();

	return __super::EndFlush();
}

HRESULT CRealVideoDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_tStart = tStart;
	return __super::NewSegment(tStart, tStop, dRate);
}

