#include "StdAfx.h"
#include "MatroskaSplitter.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>

#include "..\..\..\..\include\ogg\OggDS.h"

// {1AC0BEBD-4D2B-45ad-BCEB-F2C41C5E3788}
DEFINE_GUID(MEDIASUBTYPE_Matroska, 
0x1ac0bebd, 0x4d2b, 0x45ad, 0xbc, 0xeb, 0xf2, 0xc4, 0x1c, 0x5e, 0x37, 0x88);

using namespace Matroska;

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

const AMOVIESETUP_FILTER sudFilter =
{
    &__uuidof(CMatroskaSplitterFilter),	// Filter CLSID
    L"Matroska Splitter",	// String name
    MERIT_UNLIKELY,			// Filter merit
    sizeof(sudpPins)/sizeof(sudpPins[0]),	// Number of pins
    sudpPins				// Pin information
};

CFactoryTemplate g_Templates[] =
{
	{ L"Matroska Splitter"
	, &__uuidof(CMatroskaSplitterFilter)
	, CMatroskaSplitterFilter::CreateInstance
	, NULL
	, &sudFilter}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), CStringFromGUID(MEDIASUBTYPE_Matroska), 
		_T("0"), _T("0,4,,1A45DFA3"));

	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), CStringFromGUID(MEDIASUBTYPE_Matroska), 
		_T("Source Filter"), CStringFromGUID(CLSID_AsyncReader));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".mkv"), 
		_T("Source Filter"), CStringFromGUID(CLSID_AsyncReader));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".mkv"), 
		_T("Media Type"), CStringFromGUID(MEDIATYPE_Stream));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".mkv"), 
		_T("Subtype"), CStringFromGUID(MEDIASUBTYPE_Matroska));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".mka"), 
		_T("Source Filter"), CStringFromGUID(CLSID_AsyncReader));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".mka"), 
		_T("Media Type"), CStringFromGUID(MEDIATYPE_Stream));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".mka"), 
		_T("Subtype"), CStringFromGUID(MEDIASUBTYPE_Matroska));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), CStringFromGUID(MEDIASUBTYPE_Matroska));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".mkv"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".mka"));

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CMatroskaSplitterFilter
//

CUnknown* WINAPI CMatroskaSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMatroskaSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CMatroskaSplitterFilter::CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(NAME("CMatroskaSplitterFilter"), pUnk, this, __uuidof(this))
	, m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
	, m_fSeeking(false)
{
	if(phr) *phr = S_OK;

	m_pInput = new CMatroskaSplitterInputPin(NAME("CMatroskaSplitterInputPin"), this, this, phr);
}

CMatroskaSplitterFilter::~CMatroskaSplitterFilter()
{
	CAutoLock cAutoLock(this);

	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();

	delete m_pInput;
}

STDMETHODIMP CMatroskaSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IMediaSeeking)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

DWORD CMatroskaSplitterFilter::ThreadProc()
{
	CMatroskaFile* pFile = NULL;
	if(m_pInput) pFile = m_pInput->GetFile();

	HRESULT hr;

	CList<UINT64> bDiscontinuitySent;

	POSITION pos = m_blocks.GetHeadPosition();

	while(1)
	{
		DWORD cmd = GetRequest();

		switch(cmd)
		{
		default:
		case CMD_EXIT: 
			m_hThread = NULL;
			Reply(S_OK);
			return 0;
		case CMD_SEEK:
			{
				m_fSeeking = false;

				pos = m_pOutputs.GetHeadPosition();
				while(pos) 
				{
					CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
//					pPin->DeliverBeginFlush();
					pPin->DeliverEndFlush();
				}

				UINT64 FirstTrackNumber = -1, FirstTrackType = -1;

				pos = m_mapTrackToTrackEntry.GetStartPosition();
				while(pos)
				{
					UINT64 TrackNumber = 0;
					TrackEntry* pTE = NULL;
					m_mapTrackToTrackEntry.GetNextAssoc(pos, TrackNumber, pTE);
					if(pTE->TrackType < FirstTrackType)
					{
						FirstTrackNumber = pTE->TrackNumber;
						FirstTrackType = pTE->TrackType;
					}
				}

				if(FirstTrackType != 1)
				{
					TRACE(_T("No video track found for seeking (FirstTrackNumber=%I64d, FirstTrackType=%I64d)"), 
						FirstTrackNumber,
						FirstTrackType);
				}

				pos = m_blocks.GetHeadPosition();
				while(pos)
				{
					block_t b = m_blocks.GetAt(pos);
					if(b.rtStart >= m_rtStart)
					{
						while(pos)
						{
							b = m_blocks.GetAt(pos);
							if(b.b->KeyFrame && b.b->TrackNumber == FirstTrackNumber || pos == m_blocks.GetHeadPosition()) 
								break;
							b = m_blocks.GetPrev(pos);
						}
						break;
					}
					m_blocks.GetNext(pos);
				}

				bDiscontinuitySent.RemoveAll();

				// fall through
			}
		case CMD_RUN:
			Reply(S_OK);

			REFERENCE_TIME rtSegmentStart = 0, rtSegmentStop = 0;

			if(pos)
			{
				block_t& b = m_blocks.GetAt(pos);
				rtSegmentStart = m_rtStart;
				rtSegmentStop = m_rtStop;
				POSITION pos = m_pOutputs.GetHeadPosition();
				while(pos) m_pOutputs.GetNext(pos)->DeliverNewSegment(rtSegmentStart, rtSegmentStop, 1.0f);
			}

			while(!CheckRequest(&cmd))
			{
				if(m_fSeeking)
				{
					Sleep(10);
					continue;
				}

				if(!pos)
				{
					Sleep(50);
					continue;
				}

				block_t& b = m_blocks.GetNext(pos);

				if(b.rtStart > m_rtStop)
					continue;

				m_rtCurrent = b.rtStart;

				CMatroskaSplitterOutputPin* pPin = NULL;
				if(!m_mapTrackToPin.Lookup(b.b->TrackNumber, pPin) || !pPin || !pPin->IsConnected())
					continue;

				TrackEntry* pTE = NULL;
				if(!m_mapTrackToTrackEntry.Lookup(b.b->TrackNumber, pTE) || !pTE)
					continue;

				do
				{
					if(b.fFirst)
					{
						if(CStringA(pTE->CodecID) == "A_VORBIS")
						{
							BYTE* p = (BYTE*)pTE->CodecPrivate;

							CList<long> sizes;
							for(BYTE n = *p++; n > 0; n--)
							{
								long size = 0;
								do {size += *p;} while(*p++ == 0xff);
								sizes.AddTail(size);
							}
							sizes.AddTail(pTE->CodecPrivate.GetSize() - (p - (BYTE*)pTE->CodecPrivate));

							POSITION pos = sizes.GetHeadPosition();
							while(pos)
							{
								long len = sizes.GetNext(pos);

								CComPtr<IMediaSample> pSample;
								BYTE* pData;
								if(FAILED(hr = pPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0))
								|| FAILED(hr = pSample->GetPointer(&pData)))
									break;

								memcpy(pData, p, len);
								p += len;

								if(FAILED(hr = pSample->SetActualDataLength(len))
								|| FAILED(hr = pPin->Deliver(pSample)))
									break;
							}
						}
					}

					if(FAILED(hr = pFile->SeekTo(b.b->BlockPos)))
						break;

					ASSERT(b.b->BlockLens.GetSize() > 0);

					if(b.b->BlockLens.GetSize() == 0)
						break;

					REFERENCE_TIME 
						rtStart = b.rtStart - rtSegmentStart, 
						rtDelta = (b.rtStop - b.rtStart) / b.b->BlockLens.GetSize(), 
						rtStop = rtStart + rtDelta;

					BOOL bDiscontinuity = !bDiscontinuitySent.Find(b.b->TrackNumber);

					POSITION pos = b.b->BlockLens.GetHeadPosition();
					while(pos)
					{
						QWORD len = b.b->BlockLens.GetNext(pos);

						CComPtr<IMediaSample> pSample;
						BYTE* pData;
						if(FAILED(hr = pPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0))
						|| FAILED(hr = pSample->GetPointer(&pData))
						|| FAILED(hr = pFile->Read(pData, len))
						|| FAILED(hr = pSample->SetActualDataLength((long)len))
						|| FAILED(hr = pSample->SetTime(&rtStart, &rtStop))
						|| FAILED(hr = pSample->SetMediaTime(NULL, NULL))
						|| FAILED(hr = pSample->SetDiscontinuity(bDiscontinuity))
						|| FAILED(hr = pSample->SetSyncPoint(b.b->KeyFrame))
						|| FAILED(hr = pSample->SetPreroll(m_rtCurrent < rtSegmentStart))
						|| FAILED(hr = pPin->Deliver(pSample)))
							break;

						rtStart += rtDelta;
						rtStop += rtDelta;

						m_rtCurrent += rtDelta;

						if(bDiscontinuity)
							bDiscontinuitySent.AddTail(b.b->TrackNumber);
					}

					if(FAILED(hr))
						break;
				}
				while(false);

				if(hr == VFW_E_WRONG_STATE || hr == VFW_E_NOT_COMMITTED)
				{
					// downstream being shut down?
					pos = NULL;
				}
				else if(FAILED(hr))
				{
					ASSERT(0);
					pos = NULL;
				}

				if(b.fLast)
				{
					pPin->DeliverEndOfStream();
				}
			}
		}
	}

	ASSERT(0); // we should only exit using the CMD_EXIT command

	m_hThread = NULL;
	return 0;
}

HRESULT CMatroskaSplitterFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CMatroskaSplitterInputPin* pIn = (CMatroskaSplitterInputPin*)pPin;

		CMatroskaFile* pFile = pIn->GetFile();
		ASSERT(pFile); // should never happen, we are already in CompleteConnect!

		CMap<UINT64, UINT64, UINT64, UINT64> mapTrackNumberToMaxBlockLen;

		m_blocks.RemoveAll();

		POSITION pos = pFile->m_segment.Clusters.GetHeadPosition();
		while(pos)
		{
			block_t b;
			b.c = pFile->m_segment.Clusters.GetNext(pos);

			REFERENCE_TIME rt = (UINT64)b.c->TimeCode*pFile->m_segment.SegmentInfo.TimeCodeScale/100;

			POSITION pos2 = b.c->Blocks.GetHeadPosition();
			while(pos2)
			{
				b.b = b.c->Blocks.GetNext(pos2);

				b.fFirst = b.fLast = true;

				b.rtStart = rt + (UINT64)b.b->TimeCode*pFile->m_segment.SegmentInfo.TimeCodeScale/100;
				REFERENCE_TIME rtDur = (UINT64)b.b->BlockDuration*pFile->m_segment.SegmentInfo.TimeCodeScale/100;
				b.rtStop = rtDur > 0 ? b.rtStart + rtDur : -1;

				POSITION pos3 = m_blocks.GetTailPosition();
				while(pos3)
				{
					block_t& bprev = m_blocks.GetPrev(pos3);
					if(bprev.b->TrackNumber == b.b->TrackNumber)
					{
						b.fFirst = false;
						bprev.fLast = false;
						if(bprev.rtStop == -1) bprev.rtStop = b.rtStart;
						break;
					}
				}

				UINT64 maxlen = 0;
				if(!mapTrackNumberToMaxBlockLen.Lookup(b.b->TrackNumber, maxlen) || maxlen < b.b->MaxBlockLen)
					mapTrackNumberToMaxBlockLen[b.b->TrackNumber] = b.b->MaxBlockLen;

				m_blocks.AddTail(b);
			}
		}

		pos = pFile->m_segment.Tracks.GetHeadPosition();
		while(pos)
		{
			Track* pT = pFile->m_segment.Tracks.GetNext(pos);

			POSITION pos2 = pT->TrackEntries.GetHeadPosition();
			while(pos2)
			{
				TrackEntry* pTE = pT->TrackEntries.GetNext(pos2);

				UINT64 maxlen = 0;
				if(!mapTrackNumberToMaxBlockLen.Lookup(pTE->TrackNumber, maxlen))
				{
					TRACE(_T("CMatroskaSplitterFilter: No max block length for track %I64d\n"), (UINT64)pTE->TrackType);
					continue;
				}

				CStringA CodecID(pTE->CodecID);

				CStringW Name;
				Name.Format(L"Output %I64d", (UINT64)pTE->TrackNumber);

				CMediaType mt;

				if(pTE->TrackType == TrackEntry::TypeVideo)
				{
					Name.Format(L"Video %I64d", (UINT64)pTE->TrackNumber);

					if(CodecID == "V_MS/VFW/FOURCC")
					{
						BITMAPINFOHEADER* pbmi = (BITMAPINFOHEADER*)(BYTE*)pTE->CodecPrivate;

						mt.majortype = MEDIATYPE_Video;
						mt.subtype = FOURCCMap(pbmi->biCompression);
						mt.formattype = FORMAT_VideoInfo;
						VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.GetCount() - sizeof(BITMAPINFOHEADER));
						memset(pvih, 0, sizeof(VIDEOINFOHEADER));
						memcpy(&pvih->bmiHeader, pbmi, pTE->CodecPrivate.GetCount());
					}
				}
				else if(pTE->TrackType == TrackEntry::TypeAudio)
				{
					Name.Format(L"Audio %I64d", (UINT64)pTE->TrackNumber);

					if(CodecID == "A_VORBIS")
					{
						mt.majortype = MEDIATYPE_Audio;
						mt.subtype = MEDIASUBTYPE_Vorbis;
						mt.formattype = FORMAT_VorbisFormat;
						VORBISFORMAT* pvf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
						memset(pvf, 0, sizeof(VORBISFORMAT));
						pvf->nChannels = (WORD)pTE->a.Channels;
						pvf->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
						pvf->nMinBitsPerSec = pvf->nMaxBitsPerSec = pvf->nAvgBitsPerSec = -1;

						maxlen = max(maxlen, pTE->CodecPrivate.GetSize());
					}
					else if(CodecID == "A_MPEG/L3")
					{
						mt.majortype = MEDIATYPE_Audio;
						mt.subtype = FOURCCMap(0x55);
						mt.formattype = FORMAT_WaveFormatEx;
						WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
						memset(pwfe, 0, sizeof(WAVEFORMATEX));
						pwfe->wFormatTag = mt.subtype.Data1;
						pwfe->nChannels = (WORD)pTE->a.Channels;
						pwfe->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
					}
				}
				else if(pTE->TrackType == TrackEntry::TypeSubtitle)
				{
					Name.Format(L"Subtitle %I64d", (UINT64)pTE->TrackNumber);
/*
					if(CodecID == "S_TEXT/ASCII")
					{
						mt.majortype = MEDIATYPE_Text;
						mt.subtype = MEDIASUBTYPE_NULL;
						mt.formattype = FORMAT_None;
					}
*/
				}

				if(mt.majortype == MEDIATYPE_NULL)
				{
					TRACE(_T("CMatroskaSplitterFilter: Unsupported TrackType %s (%I64d)\n"), CString(CodecID), (UINT64)pTE->TrackType);
					continue;
				}

				mt.SetSampleSize((ULONG)maxlen);

				HRESULT hr;
				CAutoPtr<CMatroskaSplitterOutputPin> pPinOut(new CMatroskaSplitterOutputPin(mt, Name, this, this, &hr));
				if(!pPinOut) continue;

				m_mapTrackToPin[(UINT64)pTE->TrackNumber] = pPinOut;
				m_mapTrackToTrackEntry[(UINT64)pTE->TrackNumber] = pTE;

				m_pOutputs.AddTail(pPinOut);
			}
		}
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

int CMatroskaSplitterFilter::GetPinCount()
{
	return 1 + m_pOutputs.GetCount();
}

CBasePin* CMatroskaSplitterFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n == 0) return m_pInput;
	else if(n > 0 && n < (int)(m_pOutputs.GetCount()+1))
	{
		if(POSITION pos = m_pOutputs.FindIndex(n-1))
			return m_pOutputs.GetAt(pos);
	}

	return NULL;
}

STDMETHODIMP CMatroskaSplitterFilter::Stop()
{
	CAutoLock cAutoLock(this);

	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverBeginFlush();

	CallWorker(CMD_EXIT);

	pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverEndFlush();

	HRESULT hr;
	
	if(FAILED(hr = __super::Stop()))
		return hr;

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr;

	if(FAILED(hr = __super::Pause()))
		return hr;

	if(fs == State_Stopped)
	{
		m_rtStart = m_rtStop = m_rtCurrent = 0;
		m_fSeeking = false;

		if(m_pInput && m_pInput->GetFile())
		{
			Info& info = m_pInput->GetFile()->m_segment.SegmentInfo;
			m_rtStop = (REFERENCE_TIME)(info.Duration*info.TimeCodeScale/100);
		}

		Create();
		CallWorker(CMD_RUN);
	}

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	if(FAILED(hr = __super::Run(tStart)))
		return hr;

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CMatroskaSplitterFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = 
		AM_SEEKING_CanGetStopPos|
		AM_SEEKING_CanGetDuration|
		AM_SEEKING_CanSeekAbsolute|
		AM_SEEKING_CanSeekForwards|
		AM_SEEKING_CanSeekBackwards, S_OK : E_POINTER;
}
STDMETHODIMP CMatroskaSplitterFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;
	GetCapabilities(&caps);

	DWORD caps2 = caps & *pCapabilities;

	return caps2 == 0 ? E_FAIL : caps2 == *pCapabilities ? S_OK : S_FALSE;
}
STDMETHODIMP CMatroskaSplitterFilter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CMatroskaSplitterFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CMatroskaSplitterFilter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CMatroskaSplitterFilter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CMatroskaSplitterFilter::SetTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CMatroskaSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pInput, VFW_E_NOT_CONNECTED);

	CMatroskaFile* pFile = m_pInput->GetFile();
	CheckPointer(pFile, VFW_E_NOT_CONNECTED);

	Info& i = pFile->m_segment.SegmentInfo;
	*pDuration = (UINT64)i.Duration * i.TimeCodeScale / 100;

	return S_OK;
}
STDMETHODIMP CMatroskaSplitterFilter::GetStopPosition(LONGLONG* pStop) {return GetDuration(pStop);}
STDMETHODIMP CMatroskaSplitterFilter::GetCurrentPosition(LONGLONG* pCurrent) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaSplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaSplitterFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	CAutoLock cAutoLock(this);

	if(!pCurrent && !pStop
	|| (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
		return S_OK;

	m_fSeeking = true;

	if(pCurrent)
	switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: m_rtStart = *pCurrent; break;
	case AM_SEEKING_RelativePositioning: m_rtStart = m_rtCurrent + *pCurrent; break;
	case AM_SEEKING_IncrementalPositioning: return E_INVALIDARG;
	}

	if(pStop)
	switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: m_rtStop = *pStop; break;
	case AM_SEEKING_RelativePositioning: m_rtStop += *pStop; break;
	case AM_SEEKING_IncrementalPositioning: m_rtStop = m_rtCurrent + *pStop; break;
	}

	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) 
	{
		CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
		pPin->DeliverBeginFlush();
//		pPin->DeliverEndFlush();
	}

	CallWorker(CMD_SEEK);

	return S_OK;
}
STDMETHODIMP CMatroskaSplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) *pCurrent = m_rtCurrent;
	if(pStop) *pStop = m_rtStop;
	return S_OK;
}
STDMETHODIMP CMatroskaSplitterFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaSplitterFilter::SetRate(double dRate) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaSplitterFilter::GetRate(double* pdRate) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaSplitterFilter::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}

//
// CMatroskaSplitterInputPin
//

CMatroskaSplitterInputPin::CMatroskaSplitterInputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBasePin(pName, pFilter, pLock, phr, L"Input", PINDIR_INPUT)
{
}

CMatroskaSplitterInputPin::~CMatroskaSplitterInputPin()
{
}

HRESULT CMatroskaSplitterInputPin::GetAsyncReader(IAsyncReader** ppAsyncReader)
{
	CheckPointer(ppAsyncReader, E_POINTER);
	*ppAsyncReader = NULL;
	CheckPointer(m_pAsyncReader, VFW_E_NOT_CONNECTED);
	(*ppAsyncReader = m_pAsyncReader)->AddRef();
	return S_OK;
}

STDMETHODIMP CMatroskaSplitterInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMatroskaSplitterInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream
		? S_OK
		: E_INVALIDARG;
}

HRESULT CMatroskaSplitterInputPin::CheckConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CheckConnect(pPin)))
		return hr;

	if(CComQIPtr<IAsyncReader> pAsyncReader = pPin)
	{
		DWORD dw;
		hr = S_OK == pAsyncReader->SyncRead(0, 4, (BYTE*)&dw) && dw == 0xA3DF451A 
			? S_OK 
			: E_FAIL;
	}
	else
	{
		hr = E_NOINTERFACE;
	}

	return hr;
}

HRESULT CMatroskaSplitterInputPin::BreakConnect()
{
	HRESULT hr;

	if(FAILED(hr = __super::BreakConnect()))
		return hr;

	m_pAsyncReader.Release();

	m_pFile.Free();

	return S_OK;
}

HRESULT CMatroskaSplitterInputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CompleteConnect(pPin)))
		return hr;

	CheckPointer(pPin, E_POINTER);
	m_pAsyncReader = pPin;
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	m_pFile.Attach(new CMatroskaFile(m_pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;

	if(S_OK != hr) {m_pFile.Free(); return E_FAIL;}

	if(FAILED(hr = ((CMatroskaSplitterFilter*)m_pFilter)->CompleteConnect(PINDIR_INPUT, this)))
		return hr;

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterInputPin::BeginFlush()
{
	return E_UNEXPECTED;
}

STDMETHODIMP CMatroskaSplitterInputPin::EndFlush()
{
	return E_UNEXPECTED;
}

//
// CMatroskaSplitterOutputPin
//

CMatroskaSplitterOutputPin::CMatroskaSplitterOutputPin(CMediaType& mt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(NAME("CMatroskaSplitterOutputPin"), pFilter, pLock, phr, pName)
	, m_mt(mt)
{
}

CMatroskaSplitterOutputPin::~CMatroskaSplitterOutputPin()
{
}

STDMETHODIMP CMatroskaSplitterOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMatroskaSplitterOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    HRESULT hr = NOERROR;

	pProperties->cBuffers = 100;
	pProperties->cbBuffer = m_mt.GetSampleSize();

    ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
    ASSERT(Actual.cBuffers == pProperties->cBuffers);

    return NOERROR;
}

HRESULT CMatroskaSplitterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == m_mt.majortype && pmt->subtype == m_mt.subtype
		? S_OK
		: E_INVALIDARG;
}

HRESULT CMatroskaSplitterOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mt;

	return S_OK;
}

HRESULT CMatroskaSplitterOutputPin::CheckConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CheckConnect(pPin)))
		return hr;

	return S_OK;
}

HRESULT CMatroskaSplitterOutputPin::BreakConnect()
{
	HRESULT hr;

	if(FAILED(hr = __super::BreakConnect()))
		return hr;

	return S_OK;
}

HRESULT CMatroskaSplitterOutputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CompleteConnect(pPin)))
		return hr;

	CheckPointer(pPin, E_POINTER);

	return S_OK;
}

STDMETHODIMP CMatroskaSplitterOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	TRACE(_T("Quality: Type=%d Proportion=%d Late=%I64d TimeStamp=%I64d\n"), q.Type, q.Proportion, q.Late, q.TimeStamp);
	return E_NOTIMPL;
}

//

HRESULT CMatroskaSplitterOutputPin::Active()
{
    CAutoLock cAutoLock(m_pLock);

	if(m_Connected && !m_pOutputQueue)
	{
		HRESULT hr = NOERROR;

		m_pOutputQueue.Attach(new COutputQueue(m_Connected, &hr, FALSE, TRUE));
		if(!m_pOutputQueue) hr = E_OUTOFMEMORY;

		if(FAILED(hr))
		{
			m_pOutputQueue.Free();
			return hr;
		}
	}

	return __super::Active();
}

HRESULT CMatroskaSplitterOutputPin::Inactive()
{
    CAutoLock cAutoLock(m_pLock);

	m_pOutputQueue.Free();

	return __super::Inactive();
}

HRESULT CMatroskaSplitterOutputPin::Deliver(IMediaSample* pSample)
{
	if(!m_pOutputQueue) return NOERROR;
	pSample->AddRef();
	return m_pOutputQueue->Receive(pSample);
}

#define MapDeliverCall(pin, queue)				\
HRESULT CMatroskaSplitterOutputPin::Deliver##pin	\
{												\
		if(!m_pOutputQueue) return NOERROR;		\
		m_pOutputQueue->##queue;				\
		return NOERROR;							\
}												\

MapDeliverCall(EndOfStream(), EOS())
MapDeliverCall(BeginFlush(), BeginFlush())
MapDeliverCall(EndFlush(), EndFlush())
MapDeliverCall(NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate), NewSegment(tStart, tStop, dRate))
