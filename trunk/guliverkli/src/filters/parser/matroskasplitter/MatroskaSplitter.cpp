#include "StdAfx.h"
#include <mmreg.h>
#include "MatroskaSplitter.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>

#include "..\..\..\..\include\ogg\OggDS.h"

// {1AC0BEBD-4D2B-45ad-BCEB-F2C41C5E3788}
DEFINE_GUID(MEDIASUBTYPE_Matroska, 
0x1ac0bebd, 0x4d2b, 0x45ad, 0xbc, 0xeb, 0xf2, 0xc4, 0x1c, 0x5e, 0x37, 0x88);

#define NBUFFERS 100

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
	, m_dRate(1.0)
{
	if(phr) *phr = S_OK;

	m_pInput.Attach(new CMatroskaSplitterInputPin(NAME("CMatroskaSplitterInputPin"), this, this, phr));
}

CMatroskaSplitterFilter::~CMatroskaSplitterFilter()
{
	CAutoLock cAutoLock(this);

	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();
}

STDMETHODIMP CMatroskaSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IMediaSeeking)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CMatroskaSplitterFilter::SendVorbisHeaderSample()
{
	HRESULT hr;

	POSITION pos = m_mapTrackToTrackEntry.GetStartPosition();
	while(pos)
	{
		UINT64 TrackNumber = 0;
		TrackEntry* pTE = NULL;
		m_mapTrackToTrackEntry.GetNextAssoc(pos, TrackNumber, pTE);

		CMatroskaSplitterOutputPin* pPin = NULL;
		m_mapTrackToPin.Lookup(TrackNumber, pPin);

		if(!(pTE && pPin && pPin->IsConnected()))
			continue;

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

			hr = S_OK;

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

			if(FAILED(hr))
				TRACE(_T("ERROR: Vorbis initialization failed for stream %I64d\n"), TrackNumber);
		}
	}
}

void CMatroskaSplitterFilter::SendFakeTextSample()
{
	HRESULT hr;

	POSITION pos = m_mapTrackToTrackEntry.GetStartPosition();
	while(pos)
	{
		UINT64 TrackNumber = 0;
		TrackEntry* pTE = NULL;
		m_mapTrackToTrackEntry.GetNextAssoc(pos, TrackNumber, pTE);

		CMatroskaSplitterOutputPin* pPin = NULL;
		m_mapTrackToPin.Lookup(TrackNumber, pPin);

		if(!(pTE && pPin && pPin->IsConnected()))
			continue;

		if(CStringA(pTE->CodecID) == "S_TEXT/ASCII")
		{
			hr = S_OK;

			do
			{
				CComPtr<IMediaSample> pSample;
				BYTE* pData;
				if(FAILED(hr = pPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0))
				|| FAILED(hr = pSample->GetPointer(&pData)))
					break;

				strcpy((char*)pData, " ");

				REFERENCE_TIME rt = 0;

				if(FAILED(hr = pSample->SetActualDataLength(strlen((const char*)pData)))
				|| FAILED(hr = pSample->SetMediaTime(&rt, &rt))
				|| FAILED(hr = pPin->Deliver(pSample)))
					break;
			}
			while(0);
		}
	}
}

DWORD CMatroskaSplitterFilter::ThreadProc()
{
	CMatroskaNode Root(m_pFile);
	CAutoPtr<CMatroskaNode> pSegment, pCluster;
	if(!m_pFile
	|| !(pSegment = Root.Child(0x18538067))
	|| !(pCluster = pSegment->Child(0x1F43B675)))
	{
		while(1)
		{
			DWORD cmd = GetRequest();
			if(cmd == CMD_EXIT) m_hThread = NULL;
			Reply(S_OK);
			if(cmd == CMD_EXIT) return 0;
		}
	}

	SendVorbisHeaderSample(); // HACK: init vorbis decoder with the headers

	int LastBlockNumber = 0;

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

				POSITION pos = m_pOutputs.GetHeadPosition();
				while(pos) 
				{
					CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
//					pPin->DeliverBeginFlush();
					pPin->DeliverEndFlush();
				}

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

							if(m_rtStart < (REFERENCE_TIME)(pCuePoint->CueTime*m_pFile->m_segment.SegmentInfo.TimeCodeScale/100)) continue;

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

								bool fFoundKeyFrame = false, fPassedCueTime = false;

								Cluster c;
								c.ParseTimeCode(pCluster);

								if(CAutoPtr<CMatroskaNode> pBlocks = pCluster->Child(0xA0))
								{
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

								if(fFoundKeyFrame)
									pos1 = pos2 = pos3 = NULL;
							}
						}
					}
				}

				m_bDiscontinuitySent.RemoveAll();

				// fall through
			}
		case CMD_RUN:

			REFERENCE_TIME rtSegmentStart = m_rtStart;
			REFERENCE_TIME rtSegmentStop = m_rtStop;

			Reply(S_OK);

			m_pActivePins.RemoveAll();

			POSITION pos = m_pOutputs.GetHeadPosition();
			while(pos)
			{
				CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
				if(pPin->IsConnected())
				{
					pPin->DeliverNewSegment(rtSegmentStart, rtSegmentStop, 1.0f);
					m_pActivePins.AddTail(pPin);
				}
			}

			SendFakeTextSample(); // HACK: the internal script command renderer tends to freeze without one sample sent at the beginning

			HRESULT hr = S_OK;

			CBlockNode Blocks;

			do
			{
				Cluster c;
				c.ParseTimeCode(pCluster);

				CAutoPtr<CMatroskaNode> pBlocks = pCluster->Child(0xA0);
				if(!pBlocks) continue; 

				do
				{
					POSITION last = Blocks.GetTailPosition();

					bool fNext = true;
					while(LastBlockNumber > 0 && (fNext = pBlocks->Next(true))) LastBlockNumber--;
					if(!fNext) break;
					if(LastBlockNumber > 0) continue;

					Blocks.Parse(pBlocks, true);

					POSITION last2 = Blocks.GetTailPosition();
					while(last2 != last)
					{
						Block* b = Blocks.GetPrev(last2);
						b->TimeCode.Set(c.TimeCode + b->TimeCode);
					}

					POSITION pos = Blocks.GetHeadPosition();
					while(pos && !m_fSeeking && SUCCEEDED(hr) && !CheckRequest(&cmd))
					{
						POSITION next = pos;

						Block* b = Blocks.GetNext(next);

						if(b->BlockDuration == Block::INVALIDDURATION)
						{
							pos = next;
							while(pos && !m_fSeeking)
							{
								Block* b2 = Blocks.GetNext(pos);

								if(b2->TrackNumber == b->TrackNumber)
								{
									b->BlockDuration.Set(b2->TimeCode - b->TimeCode);
									break;
								}
							}
						}
						else
						{
							hr = DeliverBlock(b);
							Blocks.RemoveAt(pos);
						}

						pos = next;
					}
				}
				while(pBlocks->Next(true) && !m_fSeeking && SUCCEEDED(hr) && !CheckRequest(&cmd));
			}
			while(pCluster->Next(true) && !m_fSeeking && SUCCEEDED(hr) && !CheckRequest(&cmd));

			pos = Blocks.GetHeadPosition();
			while(pos && !m_fSeeking && SUCCEEDED(hr) && !CheckRequest(&cmd))
			{
				Block* b = Blocks.GetNext(pos);
				b->BlockDuration.Set((INT64)m_pFile->m_segment.SegmentInfo.Duration - b->TimeCode);
				hr = DeliverBlock(b);
			}

			pos = m_pActivePins.GetHeadPosition();
			while(pos && !m_fSeeking && !CheckRequest(&cmd))
				m_pActivePins.GetNext(pos)->DeliverEndOfStream();
		}
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	m_hThread = NULL;
	return 0;
}

HRESULT CMatroskaSplitterFilter::DeliverBlock(Block* b)
{
	HRESULT hr = S_FALSE;

	CMatroskaSplitterOutputPin* pPin = NULL;
	if(!m_mapTrackToPin.Lookup(b->TrackNumber, pPin) || !pPin 
	|| !pPin->IsConnected() || !m_pActivePins.Find(pPin))
		return S_FALSE;

	TrackEntry* pTE = NULL;
	if(!m_mapTrackToTrackEntry.Lookup(b->TrackNumber, pTE) || !pTE)
		return S_FALSE;

	ASSERT(b->BlockData.GetCount() > 0 && b->BlockDuration > 0);

	if(b->BlockData.GetCount() == 0 || b->BlockDuration == 0)
		return S_FALSE;

	REFERENCE_TIME 
		rtStart = (b->TimeCode)*m_pFile->m_segment.SegmentInfo.TimeCodeScale/100 - m_rtStart, 
		rtStop = (b->TimeCode + b->BlockDuration)*m_pFile->m_segment.SegmentInfo.TimeCodeScale/100 - m_rtStart,
		rtDelta = (rtStop - rtStart) / b->BlockData.GetCount();

	ASSERT(rtStart < rtStop);

	m_rtCurrent = m_rtStart + rtStart;

	BOOL bDiscontinuity = !m_bDiscontinuitySent.Find(b->TrackNumber);

//ASSERT(bDiscontinuity || b->TrackNumber != 1 || rtStart >= 0 || b->ReferenceBlock != 0);

	POSITION pos = b->BlockData.GetHeadPosition();
	while(pos && !m_fSeeking)
	{
		CBinary* pBlockData = b->BlockData.GetNext(pos);

		CComPtr<IMediaSample> pSample;
		BYTE* pData;
		if(FAILED(hr = pPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0))
		|| FAILED(hr = pSample->GetPointer(&pData))
		|| (hr = memcpy(pData, pBlockData->GetData(), pBlockData->GetSize()) ? S_OK : E_FAIL)
		|| FAILED(hr = pSample->SetActualDataLength((long)pBlockData->GetSize()))
		|| FAILED(hr = pSample->SetTime(&rtStart, &rtStop))
		|| FAILED(hr = pSample->SetMediaTime(NULL, NULL))
		|| FAILED(hr = pSample->SetDiscontinuity(bDiscontinuity))
		|| FAILED(hr = pSample->SetSyncPoint(b->ReferenceBlock == 0))
		|| FAILED(hr = pSample->SetPreroll(rtStart < 0))
		|| FAILED(hr = pPin->Deliver(pSample)))
		{
			if(POSITION pos = m_pActivePins.Find(pPin))
				m_pActivePins.RemoveAt(pos);

			if(!m_pActivePins.IsEmpty()) // only die when all pins are down
				hr = S_OK;

			return hr;
		}

		rtStart += rtDelta;
		rtStop += rtDelta;

		m_rtCurrent += rtDelta;

		if(bDiscontinuity)
			m_bDiscontinuitySent.AddTail(b->TrackNumber);
	}
    
	return hr;
}

HRESULT CMatroskaSplitterFilter::BreakConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CMatroskaSplitterInputPin* pIn = (CMatroskaSplitterInputPin*)pPin;
/*
		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos) m_pOutputs.GetNext(pos)->Disconnect();
		m_pOutputs.RemoveAll();
*/
		m_pFile.Free();
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

HRESULT CMatroskaSplitterFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CMatroskaSplitterInputPin* pIn = (CMatroskaSplitterInputPin*)pPin;

		HRESULT hr;

		CComPtr<IAsyncReader> pAsyncReader;
		if(FAILED(hr = pIn->GetAsyncReader(&pAsyncReader)))
			return hr;

		hr = E_FAIL;
		m_pFile.Attach(new CMatroskaFile(pAsyncReader, hr));
		if(!m_pFile) return E_OUTOFMEMORY;
		if(FAILED(hr)) {m_pFile.Free(); return hr;}

		POSITION pos;

		pos = m_pFile->m_segment.Tracks.GetHeadPosition();
		while(pos)
		{
			Track* pT = m_pFile->m_segment.Tracks.GetNext(pos);

			POSITION pos2 = pT->TrackEntries.GetHeadPosition();
			while(pos2)
			{
				TrackEntry* pTE = pT->TrackEntries.GetNext(pos2);

				UINT64 maxlen = 0;

				CStringA CodecID(pTE->CodecID);

				CStringW Name;
				Name.Format(L"Output %I64d", (UINT64)pTE->TrackNumber);

				CMediaType mt;

				if(pTE->TrackType == TrackEntry::TypeVideo)
				{
					Name.Format(L"Video %I64d", (UINT64)pTE->TrackNumber);

					mt.majortype = MEDIATYPE_Video;
					mt.formattype = FORMAT_VideoInfo;
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.GetCount() - sizeof(BITMAPINFOHEADER));
					memset(pvih, 0, sizeof(VIDEOINFOHEADER));

					if(CodecID == "V_MS/VFW/FOURCC")
					{
						BITMAPINFOHEADER* pbmi = (BITMAPINFOHEADER*)(BYTE*)pTE->CodecPrivate;

						mt.subtype = FOURCCMap(pbmi->biCompression);
						memcpy(&pvih->bmiHeader, pbmi, pTE->CodecPrivate.GetCount());

						switch(pbmi->biCompression)
						{
						case BI_RGB: case BI_BITFIELDS: mt.subtype = 
									pbmi->biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
									pbmi->biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
									pbmi->biBitCount == 32 ? MEDIASUBTYPE_RGB32 :
									MEDIASUBTYPE_NULL;
									break;
						case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
						case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
						}

						maxlen = pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4;
					}
					else if(CodecID == "V_UNCOMPRESSED")
					{
					}
				}
				else if(pTE->TrackType == TrackEntry::TypeAudio)
				{
					Name.Format(L"Audio %I64d", (UINT64)pTE->TrackNumber);

					mt.majortype = MEDIATYPE_Audio;
					mt.formattype = FORMAT_WaveFormatEx;

					WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
					memset(pwfe, 0, sizeof(WAVEFORMATEX));
					pwfe->nChannels = (WORD)pTE->a.Channels;
					pwfe->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
					pwfe->wBitsPerSample = (WORD)pTE->a.BitDepth;
					pwfe->nBlockAlign = (WORD)((pwfe->nChannels * pwfe->wBitsPerSample) / 8);
					pwfe->nAvgBytesPerSec = pwfe->nSamplesPerSec * pwfe->nBlockAlign;

					maxlen = pwfe->nChannels*pwfe->nSamplesPerSec*32>>3;

					if(CodecID == "A_VORBIS")
					{
						mt.subtype = MEDIASUBTYPE_Vorbis;
						mt.formattype = FORMAT_VorbisFormat;
						VORBISFORMAT* pvf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
						memset(pvf, 0, sizeof(VORBISFORMAT));
						pvf->nChannels = (WORD)pTE->a.Channels;
						pvf->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
						pvf->nMinBitsPerSec = pvf->nMaxBitsPerSec = pvf->nAvgBitsPerSec = -1;

						maxlen = pvf->nChannels*pvf->nSamplesPerSec*32>>3;
						maxlen = max(maxlen, pTE->CodecPrivate.GetSize());
					}
					else if(CodecID == "A_MPEG/L3")
					{
						mt.subtype = FOURCCMap(pwfe->wFormatTag = 0x55);
					}
					else if(CodecID == "A_AC3")
					{
						mt.subtype = FOURCCMap(pwfe->wFormatTag = 0x2000);
					}
					else if(CodecID == "A_DTS")
					{
						mt.subtype = FOURCCMap(pwfe->wFormatTag = 0x2001);
					}
					else if(CodecID == "A_MS/ACM")
					{
						pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(pTE->CodecPrivate.GetCount());
						memcpy(pwfe, (WAVEFORMATEX*)(BYTE*)pTE->CodecPrivate, pTE->CodecPrivate.GetCount());
						mt.subtype = FOURCCMap(pwfe->wFormatTag);

						maxlen = pwfe->nChannels*pwfe->nSamplesPerSec*32>>3;
					}
					else if(CodecID == "A_PCM/INT/LIT")
					{
						mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_PCM);
					}
					else if(CodecID == "A_PCM/FLOAT/IEEE")
					{
						mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_IEEE_FLOAT);
					}
					else if(CodecID.Find("A_AAC/") == 0)
					{
						mt.subtype = FOURCCMap(pwfe->wFormatTag = 0xff);
					}
					else
					{
						mt.majortype = MEDIATYPE_NULL;
					}
				}
				else if(pTE->TrackType == TrackEntry::TypeSubtitle)
				{
					Name.Format(L"Subtitle %I64d", (UINT64)pTE->TrackNumber);

					if(CodecID == "S_TEXT/ASCII")
					{
						mt.majortype = MEDIATYPE_Text;
						mt.subtype = MEDIASUBTYPE_NULL;
						mt.formattype = FORMAT_None;

						maxlen = 0x10000;
					}
				}

				if(mt.majortype == MEDIATYPE_NULL)
				{
					TRACE(_T("CMatroskaSplitterFilter: Unsupported TrackType %s (%I64d)\n"), CString(CodecID), (UINT64)pTE->TrackType);
					continue;
				}

				ASSERT(maxlen > 0);
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

		// TODO: don't do this, instead seek to start-stop right before or after starting
		if(m_pFile)
		{
			Info& info = m_pFile->m_segment.SegmentInfo;
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

	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	Info& i = m_pFile->m_segment.SegmentInfo;
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

	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) 
	{
		CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
		pPin->DeliverBeginFlush();
//		pPin->DeliverEndFlush();
	}

	if(pCurrent)
	switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: m_rtStart = *pCurrent; break;
	case AM_SEEKING_RelativePositioning: m_rtStart = m_rtCurrent + *pCurrent; break;
	case AM_SEEKING_IncrementalPositioning: m_rtStart = m_rtCurrent + *pCurrent; break;
	}

	if(pStop)
	switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: m_rtStop = *pStop; break;
	case AM_SEEKING_RelativePositioning: m_rtStop += *pStop; break;
	case AM_SEEKING_IncrementalPositioning: m_rtStop = m_rtCurrent + *pStop; break;
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
STDMETHODIMP CMatroskaSplitterFilter::SetRate(double dRate)
{
	return E_NOTIMPL;
}
STDMETHODIMP CMatroskaSplitterFilter::GetRate(double* pdRate) {return pdRate ? *pdRate = m_dRate, S_OK : E_POINTER;}
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

	if(FAILED(hr = ((CMatroskaSplitterFilter*)m_pFilter)->BreakConnect(PINDIR_INPUT, this)))
		return hr;

	m_pAsyncReader.Release();

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

	pProperties->cBuffers = NBUFFERS;
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

		m_pOutputQueue.Attach(new COutputQueue(m_Connected, &hr, FALSE, TRUE, 1, FALSE, NBUFFERS));
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

#define MapDeliverCall(pin, queue)					\
HRESULT CMatroskaSplitterOutputPin::Deliver##pin	\
{													\
		if(!m_pOutputQueue) return NOERROR;			\
		m_pOutputQueue->##queue;					\
		return NOERROR;								\
}													\

MapDeliverCall(EndOfStream(), EOS())
MapDeliverCall(BeginFlush(), BeginFlush())
MapDeliverCall(EndFlush(), EndFlush())
MapDeliverCall(NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate), NewSegment(tStart, tStop, dRate))
