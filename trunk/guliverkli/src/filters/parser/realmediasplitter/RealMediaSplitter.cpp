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
#include <Shlwapi.h>
#include <atlpath.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "..\..\..\DSUtil\DSUtil.h"
#include "RealMediaSplitter.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

template<typename T>
static void bswap(T& var)
{
	BYTE* s = (BYTE*)&var;
	for(BYTE* d = s + sizeof(var)-1; s < d; s++, d--)
		*s ^= *d, *d ^= *s, *s ^= *d;
}

void rvinfo::bswap()
{
	::bswap(dwSize);
	::bswap(w); ::bswap(h); ::bswap(bpp);
	::bswap(unk1); ::bswap(fps); 
	::bswap(type1); ::bswap(type2);
}

void rainfo::bswap()
{
	::bswap(version1);
	::bswap(version2);
	::bswap(header_size);
	::bswap(flavor);
	::bswap(coded_frame_size);
	::bswap(sub_packet_h);
	::bswap(frame_size);
	::bswap(sub_packet_size);
}

void rainfo4::bswap()
{
	__super::bswap();
	::bswap(sample_rate);
	::bswap(sample_size);
	::bswap(channels);
}

void rainfo5::bswap()
{
	__super::bswap();
	::bswap(sample_rate);
	::bswap(sample_size);
	::bswap(channels);
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

const AMOVIESETUP_MEDIATYPE sudPinTypesIn3[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut3[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_14_4},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_28_8},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_ATRC},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_COOK},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DNET},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_SIPR},
};

const AMOVIESETUP_PIN sudpPins3[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn3)/sizeof(sudPinTypesIn3[0]),	// Number of types
      sudPinTypesIn3			// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut3)/sizeof(sudPinTypesOut3[0]),	// Number of types
      sudPinTypesOut3		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter2[] =
{
	{&__uuidof(CRealVideoDecoder), L"RealVideo Decoder", MERIT_UNLIKELY, sizeof(sudpPins2)/sizeof(sudpPins2[0]), sudpPins2},
	{&__uuidof(CRealAudioDecoder), L"RealAudio Decoder", MERIT_UNLIKELY, sizeof(sudpPins3)/sizeof(sudpPins3[0]), sudpPins3},
};

////////////////////

CFactoryTemplate g_Templates[] =
{
	{L"RealMedia Source", &__uuidof(CRealMediaSourceFilter), CRealMediaSourceFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"RealMedia Splitter", &__uuidof(CRealMediaSplitterFilter), CRealMediaSplitterFilter::CreateInstance, NULL, &sudFilter[1]},
////////////////////
    {L"RealVideo Decoder", &__uuidof(CRealVideoDecoder), CRealVideoDecoder::CreateInstance, NULL, &sudFilter2[0]},
    {L"RealAudio Decoder", &__uuidof(CRealAudioDecoder), CRealAudioDecoder::CreateInstance, NULL, &sudFilter2[1]},
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
	, m_nOpenProgress(100)
	, m_fAbort(false)
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
		QI(IAMOpenProgress)
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

	m_rtStop = 10000i64*m_pFile->m_p.tDuration;

	POSITION pos = m_pFile->m_mps.GetHeadPosition();
	while(pos)
	{
		MediaProperies* pmp = m_pFile->m_mps.GetNext(pos);

		CStringW name;
		name.Format(L"Output %02d", pmp->stream);
		if(!pmp->name.IsEmpty()) name += L" (" + CStringW(pmp->name) + L")";

		CArray<CMediaType> mts;

		CMediaType mt;
		mt.SetSampleSize(max(pmp->maxPacketSize*256, 1));

		if(pmp->mime == "video/x-pn-realvideo")
		{
			mt.majortype = MEDIATYPE_Video;
			mt.formattype = FORMAT_VideoInfo;

			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pmp->typeSpecData.GetCount());
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(pvih + 1, pmp->typeSpecData.GetData(), pmp->typeSpecData.GetCount());

			rvinfo rvi = *(rvinfo*)pmp->typeSpecData.GetData();
			rvi.bswap();

			ASSERT(rvi.dwSize >= FIELD_OFFSET(rvinfo, w2));
			ASSERT(rvi.fcc1 == 'ODIV');

			mt.subtype = FOURCCMap(rvi.fcc2);
			if(rvi.fps > 0x10000) pvih->AvgTimePerFrame = REFERENCE_TIME(10000000i64 / ((float)rvi.fps/0x10000)); 
			pvih->dwBitRate = pmp->avgBitRate; 
			pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
			pvih->bmiHeader.biWidth = rvi.w;
			pvih->bmiHeader.biHeight = rvi.h;
			pvih->bmiHeader.biPlanes = 3;
			pvih->bmiHeader.biBitCount = rvi.bpp;
			pvih->bmiHeader.biCompression = rvi.fcc2;
			pvih->bmiHeader.biSizeImage = rvi.w*rvi.h*3/2;

			mts.Add(mt);
		}
		else if(pmp->mime == "audio/x-pn-realaudio")
		{
			mt.majortype = MEDIATYPE_Audio;
			mt.formattype = FORMAT_WaveFormatEx;

			WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmp->typeSpecData.GetCount());
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(pwfe + 1, pmp->typeSpecData.GetData(), pmp->typeSpecData.GetCount());

			union {
			DWORD fcc;
			char fccstr[5];
			};

			fcc = 0;
			fccstr[4] = 0;

			rainfo rai = *(rainfo*)pmp->typeSpecData.GetData();
			rai.bswap();

			if(rai.version2 == 4)
			{
				rainfo4 rai4 = *(rainfo4*)pmp->typeSpecData.GetData();
				rai4.bswap();
				pwfe->nChannels = rai4.channels;
				pwfe->wBitsPerSample = rai4.sample_size;
				pwfe->nSamplesPerSec = rai4.sample_rate;
				pwfe->nBlockAlign = rai4.frame_size;
				BYTE* p = (BYTE*)((rainfo4*)pmp->typeSpecData.GetData()+1);
				int len = *p++; p += len; len = *p++; ASSERT(len == 4);
				if(len == 4)
				fcc = MAKEFOURCC(p[0],p[1],p[2],p[3]);
			}
			else if(rai.version2 == 5)
			{
				rainfo5 rai5 = *(rainfo5*)pmp->typeSpecData.GetData();
				rai5.bswap();
				pwfe->nChannels = rai5.channels;
				pwfe->wBitsPerSample = rai5.sample_size;
				pwfe->nSamplesPerSec = rai5.sample_rate;
				pwfe->nBlockAlign = rai5.frame_size;
				fcc = rai5.fourcc3;
			}

			_strupr(fccstr);

			mt.subtype = FOURCCMap(fcc);

			bswap(fcc);

			switch(fcc)
			{
			case '14_4': pwfe->wFormatTag = WAVE_FORMAT_14_4; break;
			case '28_8': pwfe->wFormatTag = WAVE_FORMAT_28_8; break;
			case 'ATRC': pwfe->wFormatTag = WAVE_FORMAT_ATRC; break;
			case 'COOK': pwfe->wFormatTag = WAVE_FORMAT_COOK; break;
			case 'DNET': pwfe->wFormatTag = WAVE_FORMAT_DNET; break;
			case 'SIPR': pwfe->wFormatTag = WAVE_FORMAT_SIPR; break;
			}

			if(pwfe->wFormatTag)
			{
				mts.Add(mt);

				if(fcc == 'DNET')
				{
					mt.subtype = MEDIASUBTYPE_WAVE_DOLBY_AC3;
					pwfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3;
					mts.Add(mt);
				}

/*
				if(fcc == 'SIPR')
				{
					pwfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX)+4);
					pwfe->cbSize = 4;
					//pwfe->wBitsPerSample = 0;
					//pwfe->nAvgBytesPerSec = 1055;
					//pwfe->nBlockAlign = 19;
					memset(pwfe+1, 0, 4);
					WORD* p = (WORD*)(pwfe+1);
					*p = 1;
					mts.Add(mt);
				}
*/
			}
		}

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
	m_fFlushing = true;
	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverBeginFlush();
}

void CRealMediaSourceFilter::DeliverEndFlush()
{
	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverEndFlush();
	m_fFlushing = false;
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

	// reindex if needed

	if(m_pFile->m_irs.GetCount() == 0)
	{
		m_nOpenProgress = 0;
		m_pFile->m_p.tDuration = 0;

		int stream = m_pFile->GetMasterStream();

		UINT32 tLastStart = -1;
		UINT32 nPacket = 0;

		POSITION pos = m_pFile->m_dcs.GetHeadPosition(); 
		while(pos && !m_fAbort)
		{
			DataChunk* pdc = m_pFile->m_dcs.GetNext(pos);

			m_pFile->Seek(pdc->pos);

			for(UINT32 i = 0; i < pdc->nPackets && !m_fAbort; i++, nPacket++)
			{
				UINT64 filepos = m_pFile->GetPos();

				HRESULT hr;

				MediaPacketHeader mph;
				if(S_OK != (hr = m_pFile->Read(mph, false)))
					break;

				if(mph.stream == stream && (mph.flags&MediaPacketHeader::PN_KEYFRAME_FLAG) && tLastStart != mph.tStart)
				{
					m_pFile->m_p.tDuration = max(mph.tStart, m_pFile->m_p.tDuration);

					CAutoPtr<IndexRecord> pir(new IndexRecord());
					pir->tStart = mph.tStart;
					pir->ptrFilePos = (UINT32)filepos;
					pir->packet = nPacket;
					m_pFile->m_irs.AddTail(pir);

					tLastStart = mph.tStart;
				}

				m_nOpenProgress = m_pFile->GetPos()*100/m_pFile->GetLength();

				DWORD cmd;
				if(CheckRequest(&cmd))
				{
					if(cmd == CMD_EXIT) m_fAbort = true;
					else Reply(S_OK);
				}
			}
		}

		m_nOpenProgress = 100;

		if(m_fAbort) m_pFile->m_irs.RemoveAll();

		m_fAbort = false;
	}

	//

	m_eEndFlush.Set();
	m_fFlushing = false;

	bool fFirstRun = true;

	POSITION seekpos = NULL;
	UINT32 seekpacket = 0;
	UINT64 seekfilepos = 0;

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

		if(m_rtStart <= 0)
		{
			seekpos = m_pFile->m_dcs.GetHeadPosition(); 
			seekpacket = 0;
			seekfilepos = m_pFile->m_dcs.GetAt(seekpos)->pos;
		}
		else
		{
			seekpos = NULL; 

			POSITION pos = m_pFile->m_irs.GetTailPosition();
			while(pos && !seekpos)
			{
				IndexRecord* pir = m_pFile->m_irs.GetPrev(pos);
				if(pir->tStart <= m_rtStart/10000)
				{
					seekpacket = pir->packet;

					pos = m_pFile->m_dcs.GetTailPosition();
					while(pos && !seekpos)
					{
						POSITION tmp = pos;

						DataChunk* pdc = m_pFile->m_dcs.GetPrev(pos);

						if(pdc->pos <= pir->ptrFilePos)
						{
							seekpos = tmp;
							seekfilepos = pir->ptrFilePos;

							POSITION pos = m_pFile->m_dcs.GetHeadPosition();
							while(pos != seekpos)
							{
								seekpacket -= m_pFile->m_dcs.GetNext(pos)->nPackets;
							}
						}
					}

					// search the closest keyframe to the seek time (commented out 'cause rm seems to index all of its keyframes...)
/*
					if(seekpos)
					{
						DataChunk* pdc = m_pFile->m_dcs.GetAt(seekpos);

						m_pFile->Seek(seekfilepos);

						REFERENCE_TIME seektime = -1;
						UINT32 seekstream = -1;

						for(UINT32 i = seekpacket; i < pdc->nPackets; i++)
						{
							UINT64 filepos = m_pFile->GetPos();

							MediaPacketHeader mph;
							if(S_OK != m_pFile->Read(mph, false))
								break;

							if(seekstream == -1) seekstream = mph.stream;
							if(seekstream != mph.stream) continue;

							if(seektime == 10000i64*mph.tStart) continue;
							if(m_rtStart < 10000i64*mph.tStart) break;

							if((mph.flags&MediaPacketHeader::PN_KEYFRAME_FLAG))
							{
								seekpacket = i;
								seekfilepos = filepos;
								seektime = 10000i64*mph.tStart;
							}
						}
					}
*/
				}
			}

			if(!seekpos)
			{
				seekpos = m_pFile->m_dcs.GetHeadPosition(); 
				seekpacket = 0;
				seekfilepos = m_pFile->m_dcs.GetAt(seekpos)->pos;
			}
		}

		if(cmd != -1)
			Reply(S_OK);

		m_eEndFlush.Wait();

		m_bDiscontinuitySent.RemoveAll();
		m_pActivePins.RemoveAll();

		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos && !m_fFlushing)
		{
			CBaseOutputPin* pPin = m_pOutputs.GetNext(pos);
			if(pPin->IsConnected())
			{
				pPin->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
				m_pActivePins.AddTail(pPin);
			}
		}

		HRESULT hr = S_OK;

		pos = seekpos; 
		while(pos && SUCCEEDED(hr) && !CheckRequest(&cmd))
		{
			DataChunk* pdc = m_pFile->m_dcs.GetNext(pos);

			m_pFile->Seek(seekfilepos > 0 ? seekfilepos : pdc->pos);
//TRACE(_T("m_pFile->Seek(%I64d)\n"), m_pFile->GetPos());

			for(UINT32 i = seekpacket; i < pdc->nPackets && SUCCEEDED(hr) && !CheckRequest(&cmd); i++)
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

			seekpacket = 0;
			seekfilepos = 0;
//TRACE(_T("Exited deliver loop\n"));
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
/*
	if(m_fFlushing)
		return S_FALSE;
*/
	CRealMediaSplitterOutputPin* pPin = NULL;
	if(!m_mapTrackToPin.Lookup(b->TrackNumber, pPin) || !pPin 
	|| !pPin->IsConnected() || !m_pActivePins.Find(pPin))
		return S_FALSE;

	if(b->pData.GetCount() == 0)
		return S_FALSE;

	m_rtCurrent = b->rtStart;

	b->rtStart -= m_rtStart;
	b->rtStop -= m_rtStart;

	ASSERT(b->rtStart < b->rtStop);

	DWORD TrackNumber = b->TrackNumber;
	BOOL bDiscontinuity = !m_bDiscontinuitySent.Find(TrackNumber);

//TRACE(_T("pPin->DeliverBlock: TrackNumber (%d) %I64d, %I64d (disc=%d)\n"), (int)TrackNumber, b->rtStart, b->rtStop, bDiscontinuity);

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
		AM_SEEKING_CanGetDuration|
		AM_SEEKING_CanSeekAbsolute|
		AM_SEEKING_CanSeekForwards|
		AM_SEEKING_CanSeekBackwards, S_OK : E_POINTER;
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
//DbgLog((LOG_TRACE, 0, _T("m_rtNewStart=%I64d"), m_rtNewStart));
//DbgLog((LOG_TRACE, 0, _T("DeliverBeginFlush()")));
		DeliverBeginFlush();
//DbgLog((LOG_TRACE, 0, _T("CallWorker(CMD_SEEK)")));
		CallWorker(CMD_SEEK);
//DbgLog((LOG_TRACE, 0, _T("DeliverEndFlush()")));
		DeliverEndFlush();
//DbgLog((LOG_TRACE, 0, _T("Seeking ended")));
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
	m_rtStart = tStart;
	m_segments.Clear(); // FIXME: !!!!!!!!!!!!!!!!!!!!! this is not thread safe, can crash any time !!!!!!!!!!!!!!!!!!!!!!!!
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

HRESULT CRealMediaSplitterOutputPin::DeliverSegments(CSegments& segments)
{
	HRESULT hr;

	if(segments.GetCount() == 0)
	{
		segments.Clear();
		return S_OK;
	}

	CComPtr<IMediaSample> pSample;
	BYTE* pData;
	BYTE* pDataOrg;
	if(S_OK != (hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0))
	|| S_OK != (hr = pSample->GetPointer(&pData)) || !(pDataOrg = pData))
	{
		segments.Clear();
		return hr;
	}

	*pData++ = segments.fMerged ? 0 : segments.GetCount()-1;

	if(segments.fMerged)
	{
		*((DWORD*)pData) = 1; pData += 4;
		*((DWORD*)pData) = 0; pData += 4;
	}
	else
	{
		POSITION pos = segments.GetHeadPosition();
		while(pos)
		{
			segment* s = segments.GetNext(pos);
			*((DWORD*)pData) = 1; pData += 4;
			*((DWORD*)pData) = s->offset; pData += 4;
		}
	}

	DWORD len = 0, total = 0;
	POSITION pos = segments.GetHeadPosition();
	while(pos)
	{
		segment* s = segments.GetNext(pos);
		ASSERT(pSample->GetSize() >= (pData-pDataOrg) + s->offset + s->data.GetCount());
		memcpy(pData + s->offset, s->data.GetData(), s->data.GetCount());
		total = max(total, s->offset + s->data.GetCount());
		len += s->data.GetCount();
	}

	ASSERT(total == len);

	total += pData - pDataOrg;

	REFERENCE_TIME rtStart = segments.rtStart;
	REFERENCE_TIME rtStop = rtStart+1;

	if(S_OK != (hr = pSample->SetActualDataLength(total))
	|| S_OK != (hr = pSample->SetTime(&rtStart, &rtStop))
	|| S_OK != (hr = pSample->SetMediaTime(NULL, NULL))
	|| S_OK != (hr = pSample->SetDiscontinuity(segments.fDiscontinuity))
	|| S_OK != (hr = pSample->SetSyncPoint(segments.fSyncPoint))
	|| S_OK != (hr = pSample->SetPreroll(segments.rtStart < 0))
	|| S_OK != (hr = Deliver(pSample)))
		int empty = 0;

	segments.Clear();

	return S_OK;
}

DWORD CRealMediaSplitterOutputPin::ThreadProc()
{
	m_hrDeliver = S_OK;

	::SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	m_segments.Clear();

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
//TRACE(_T("CRMSplitterOutputPin::ThreadProc: %I64d (disc=%d)\n"), p->b->rtStart, p->bDiscontinuity);

				ASSERT(p->b->rtStart < p->b->rtStop);

				if(m_mt.subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
				{
					WORD* s = (WORD*)p->b->pData.GetData();
					WORD* e = s + p->b->pData.GetSize()/2;
					while(s < e) bswap(*s++);
				}
				
				if(m_mt.subtype == FOURCCMap('01VR') || m_mt.subtype == FOURCCMap('02VR')
				|| m_mt.subtype == FOURCCMap('03VR') || m_mt.subtype == FOURCCMap('04VR'))
				{
					int len = p->b->pData.GetCount();
					BYTE* pIn = p->b->pData.GetData(), * pInOrg = pIn;

					if(m_hrDeliver == S_OK && m_segments.rtStart != p->b->rtStart)
					{
//						ASSERT(m_segments.rtStart < p->b->rtStart);

//						TRACE(_T("WARNING: CRealMediaSplitterOutputPin::ThreadProc() sending incomplete segments\n"));

//TRACE(_T("sending not terminated segments\n"));
						if(S_OK != (hr = DeliverSegments(m_segments)))
						{
//TRACE(_T("S_OK != (hr = DeliverSegments(m_segments))\n"));
							CAutoLock cAutoLock(&m_csQueueLock);
							m_hrDeliver = hr;
							continue;
						}
					}

					if(!m_segments.fDiscontinuity && p->bDiscontinuity)
						m_segments.fDiscontinuity = true;
					m_segments.fSyncPoint = !!p->b->bSyncPoint;
					m_segments.rtStart = p->b->rtStart;

					while(m_hrDeliver == S_OK && pIn - pInOrg < len)
					{
						BYTE hdr = *pIn++, subseq = 0, seqnum = 0;
						DWORD packetlen = 0, packetoffset = 0;

						if((hdr&0xc0) == 0x40)
						{
							pIn++;
							packetlen = len - (pIn - pInOrg);
						}
						else
						{
							if((hdr&0x40) == 0)
								subseq = (*pIn++)&0x7f;

							#define GetWORD(var) \
								var = (var<<8)|(*pIn++); \
								var = (var<<8)|(*pIn++); \

							GetWORD(packetlen);
							if(packetlen&0x8000) m_segments.fMerged = true;
							if((packetlen&0x4000) == 0) {GetWORD(packetlen); packetlen &= 0x3fffffff;}
							else packetlen &= 0x3fff;

							GetWORD(packetoffset);
							if((packetoffset&0x4000) == 0) {GetWORD(packetoffset); packetoffset &= 0x3fffffff;}
							else packetoffset &= 0x3fff;

							#undef GetWORD

							if((hdr&0xc0) == 0xc0)
								m_segments.rtStart = 10000i64*packetoffset - m_rtStart, packetoffset = 0;
							else if((hdr&0xc0) == 0x80)
								packetoffset = packetlen - packetoffset;

							seqnum = *pIn++;
						}

                        int len2 = min(len - (pIn - pInOrg), packetlen - packetoffset);

						CAutoPtr<segment> s(new segment);
						s->offset = packetoffset;
						s->data.SetSize(len2);
#ifdef DEBUG
						s->rtStart = m_segments.rtStart;
#endif
						memcpy(s->data.GetData(), pIn, len2);
						m_segments.AddTail(s);

#ifdef DEBUG
						{
							POSITION pos = m_segments.GetHeadPosition();
							while(pos)
							{
								segment* s = m_segments.GetNext(pos);
								ASSERT(s->rtStart == m_segments.rtStart);
							}
						}
#endif


						pIn += len2;

						if((hdr&0x80) /*|| packetoffset+len2 >= packetlen*/)
						{
//TRACE(_T("sending terminated segments\n"));
							if(S_OK != (hr = DeliverSegments(m_segments)))
							{
//TRACE(_T("S_OK != (hr = DeliverSegments(m_segments))\n"));
								CAutoLock cAutoLock(&m_csQueueLock);
								m_hrDeliver = hr;
							}
						}

					}
				}
				else
				{
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

HRESULT CRMFile::Read(MediaPacketHeader& mph, bool fFull)
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
	len = max(len, 0);

	if(fFull)
	{
		mph.pData.SetSize(len);
		if(mph.len > 0 && S_OK != (hr = Read(mph.pData.GetData(), len)))
			return hr;
	}
	else
	{
		Seek(m_pos + len);
	}

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

		if(hdr.object_id == 0x2E7261FD) // '.ra+0xFD'
			return E_FAIL;

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
				int stream = GetMasterStream();
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
						if(ich.stream == stream) m_irs.AddTail(ir);
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

int CRMFile::GetMasterStream()
{
	int s1 = -1, s2 = -1;

	POSITION pos = m_mps.GetHeadPosition();
	while(pos)
	{
		MediaProperies* pmp = m_mps.GetNext(pos);
		if(pmp->mime == "video/x-pn-realvideo") {s1 = pmp->stream; break;}
		else if(pmp->mime == "audio/x-pn-realaudio" && s2 == -1) s2 = pmp->stream;
	}

	if(s1 == -1)
		s1 = s2;

	return s1;
}


////////////////////////////

//
// CRealVideoDecoder
//

CRealVideoDecoder::CRealVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(NAME("CRealVideoDecoder"), lpunk, __uuidof(this))
	, m_hDrvDll(NULL)
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

HRESULT CRealVideoDecoder::InitRV(const CMediaType* pmt)
{
	FreeRV();

	HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

	rvinfo rvi = *(rvinfo*)(pmt->Format() + sizeof(VIDEOINFOHEADER));
	rvi.bswap();

	#pragma pack(push, 1)
	struct {WORD unk1, w, h, unk3; DWORD unk2, subformat, unk5, format;} i =
		{11, rvi.w, rvi.h, 0, 0, rvi.type1, 1, rvi.type2};
	#pragma pack(pop)

	if(FAILED(hr = RVInit(&i, &m_dwCookie)))
		return hr;

	if(rvi.fcc2 <= '03VR' && rvi.type2 >= 0x20200002)
	{
		#pragma pack(push, 1)
		UINT32 cmsg24[6] = {rvi.w, rvi.h, rvi.w2*4, rvi.h2*4, rvi.w3*4, rvi.h3*4};
		struct {UINT32 data1; UINT32 data2; UINT32* dimensions;} cmsg_data = 
			{0x24, 1+((rvi.type1>>16)&7), cmsg24};;
		#pragma pack(pop)

		hr = RVCustomMessage(&cmsg_data, m_dwCookie);
	}

	return hr;
}

void CRealVideoDecoder::FreeRV()
{
	if(m_dwCookie)
	{
		RVFree(m_dwCookie);
		m_dwCookie = 0;
	}
}

HRESULT CRealVideoDecoder::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);
//DbgLog((LOG_TRACE, 0, _T("CRealVideoDecoder::Receive()")));

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;

	long len = pIn->GetActualDataLength();
	if(len <= 0) return S_OK; // nothing to do

	REFERENCE_TIME rtStart, rtStop;
	pIn->GetTime(&rtStart, &rtStop);

	rtStart += m_tStart;

//	TRACE(_T("in=%04x, start=%I64d, stop=%I64d\n"), len, rtStart, rtStop);

	#pragma pack(push, 1)
	struct {DWORD len, unk1, chunks; DWORD* extra; DWORD unk2, timestamp;} transform_in = 
		{len - (1+((*pDataIn)+1)*8), 0, *pDataIn, (DWORD*)(pDataIn+1), 0, (DWORD)(rtStart/10000)};
	struct {DWORD unk1, unk2, timestamp, w, h;} transform_out = 
		{0,0,0,0,0};
	#pragma pack(pop)

	if(m_fDropFrames && m_timestamp+1 == transform_in.timestamp)
	{
		m_timestamp = transform_in.timestamp;
		return S_OK;
	}

	hr = RVTransform(pDataIn + (1+((*pDataIn)+1)*8), (BYTE*)m_pI420FrameBuff, &transform_in, &transform_out, m_dwCookie);

	m_timestamp = transform_in.timestamp;

	if(FAILED(hr))
	{
		TRACE(_T("RV returned an error code!!!\n"));
//		return hr;
	}

	if(pIn->IsPreroll() == S_OK || rtStart < 0 || !(transform_out.unk1&1))
		return S_OK;

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
/*
CMediaType mt = m_pOutput->CurrentMediaType();
VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.Format();
if(vih->rcSource.right != transform_out.w || vih->rcSource.bottom != transform_out.h)
{
vih->rcSource.right = transform_out.w;
vih->rcSource.bottom = transform_out.h;
vih->rcTarget.right = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->bmiHeader.biWidth;
vih->rcTarget.bottom = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->bmiHeader.biHeight;
pOut->SetMediaType(&mt);
}
*/
	rtStart = 10000i64*transform_out.timestamp - m_tStart;
	rtStop = rtStart + 1;
	pOut->SetTime(&rtStart, /*NULL*/&rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
	pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);

	Copy(m_pI420FrameBuff, pDataOut, transform_out.w, transform_out.h);
DbgLog((LOG_TRACE, 0, _T("V: rtStart=%I64d, rtStop=%I64d, disc=%d, sync=%d"), 
	   rtStart, rtStop, pOut->IsDiscontinuity() == S_OK, pOut->IsSyncPoint() == S_OK));
	return m_pOutput->Deliver(pOut);
}

void CRealVideoDecoder::Copy(BYTE* pIn, BYTE* pOut, int w, int h)
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

		CStringList paths;
		CString olddll, newdll, oldpath, newpath;

		olddll.Format(_T("drv%c3260.dll"), (TCHAR)((mtIn->subtype.Data1>>16)&0xff));
		newdll = mtIn->subtype == FOURCCMap('02VR') ? _T("drv2.dll") : _T("drvc.dll");

		CRegKey key;
		TCHAR buff[MAX_PATH];
		ULONG len = sizeof(buff);
		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Software\\RealNetworks\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			oldpath = buff;
			TCHAR c = oldpath[oldpath.GetLength()-1];
			if(c != '\\' && c != '/') oldpath += '\\';
			key.Close();
		}
		len = sizeof(buff);
		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Helix\\HelixSDK\\10.0\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			newpath = buff;
			TCHAR c = newpath[newpath.GetLength()-1];
			if(c != '\\' && c != '/') newpath += '\\';
			key.Close();
		}
		if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Helix\\HelixSDK\\10.0\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			newpath = buff;
			TCHAR c = newpath[newpath.GetLength()-1];
			if(c != '\\' && c != '/') newpath += '\\';
			key.Close();
		}

		if(!newpath.IsEmpty()) paths.AddTail(newpath + newdll);
		if(!oldpath.IsEmpty()) paths.AddTail(oldpath + newdll);
		paths.AddTail(newdll); // default dll paths
		if(!newpath.IsEmpty()) paths.AddTail(newpath + olddll);
		if(!oldpath.IsEmpty()) paths.AddTail(oldpath + olddll);
		paths.AddTail(olddll); // default dll paths

		POSITION pos = paths.GetHeadPosition();
		while(pos && !(m_hDrvDll = LoadLibrary(paths.GetNext(pos))));

		if(m_hDrvDll)
		{
			RVCustomMessage = (PRVCustomMessage)GetProcAddress(m_hDrvDll, "RV20toYUV420CustomMessage");
			RVFree = (PRVFree)GetProcAddress(m_hDrvDll, "RV20toYUV420Free");
			RVHiveMessage = (PRVHiveMessage)GetProcAddress(m_hDrvDll, "RV20toYUV420HiveMessage");
			RVInit = (PRVInit)GetProcAddress(m_hDrvDll, "RV20toYUV420Init");
			RVTransform = (PRVTransform)GetProcAddress(m_hDrvDll, "RV20toYUV420Transform");

			if(!RVCustomMessage) RVCustomMessage = (PRVCustomMessage)GetProcAddress(m_hDrvDll, "RV40toYUV420CustomMessage");
			if(!RVFree) RVFree = (PRVFree)GetProcAddress(m_hDrvDll, "RV40toYUV420Free");
			if(!RVHiveMessage) RVHiveMessage = (PRVHiveMessage)GetProcAddress(m_hDrvDll, "RV40toYUV420HiveMessage");
			if(!RVInit) RVInit = (PRVInit)GetProcAddress(m_hDrvDll, "RV40toYUV420Init");
			if(!RVTransform) RVTransform = (PRVTransform)GetProcAddress(m_hDrvDll, "RV40toYUV420Transform");
		}

		if(!m_hDrvDll || !RVCustomMessage 
		|| !RVFree || !RVHiveMessage
		|| !RVInit || !RVTransform)
			return VFW_E_TYPE_NOT_ACCEPTED;

		if(FAILED(InitRV(mtIn)))
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CRealVideoDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return mtIn->majortype == MEDIATYPE_Video && (mtIn->subtype == FOURCCMap('02VR')
												|| mtIn->subtype == FOURCCMap('03VR')
												|| mtIn->subtype == FOURCCMap('04VR'))
		&& mtOut->majortype == MEDIATYPE_Video && (mtOut->subtype == FOURCCMap('21VY')
												|| mtOut->subtype == FOURCCMap('2YUY'))
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
iPosition = 1-iPosition;
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
	if(FAILED(InitRV(&m_pInput->CurrentMediaType())))
		return E_FAIL;

	BITMAPINFOHEADER& bih = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->bmiHeader;
	int size = bih.biWidth*bih.biHeight;
	m_pI420FrameBuff.Allocate(size*3/2);
	memset(m_pI420FrameBuff, 0, size);
	memset(m_pI420FrameBuff + size, 0x80, size/2);

	return __super::StartStreaming();
}

HRESULT CRealVideoDecoder::StopStreaming()
{
	m_pI420FrameBuff.Free();

	FreeRV();

	return __super::StopStreaming();
}

HRESULT CRealVideoDecoder::EndOfStream()
{
//DbgLog((LOG_TRACE, 0, _T("CRealVideoDecoder::EndOfStream()")));
	return __super::EndOfStream();
}

HRESULT CRealVideoDecoder::BeginFlush()
{
//DbgLog((LOG_TRACE, 0, _T("CRealVideoDecoder::BeginFlush()")));
	return __super::BeginFlush();
}

HRESULT CRealVideoDecoder::EndFlush()
{
//DbgLog((LOG_TRACE, 0, _T("CRealVideoDecoder::EndFlush()")));
	return __super::EndFlush();
}

HRESULT CRealVideoDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_timestamp = ~0;
	m_fDropFrames = false;

	DWORD tmp[2] = {20, 0};
	RVHiveMessage(tmp, m_dwCookie);

//DbgLog((LOG_TRACE, 0, _T("CRealVideoDecoder::NewSegment()")));
	m_tStart = tStart;
	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CRealVideoDecoder::AlterQuality(Quality q)
{
	if(q.Late > 500*10000i64) m_fDropFrames = true;
	if(q.Late <= 0) m_fDropFrames = false;
//	TRACE(_T("CRealVideoDecoder::AlterQuality: Type=%d, Proportion=%d, Late=%I64d, TimeStamp=%I64d\n"), q.Type, q.Proportion, q.Late, q.TimeStamp);
	return E_NOTIMPL;
}

/////////////////////////

//
// CRealAudioDecoder
//

CRealAudioDecoder::CRealAudioDecoder(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(NAME("CRealAudioDecoder"), lpunk, __uuidof(this))
	, m_hDrvDll(NULL)
	, m_dwCookie(0)
{
	if(phr) *phr = S_OK;
}

CRealAudioDecoder::~CRealAudioDecoder()
{
	if(m_hDrvDll) FreeLibrary(m_hDrvDll);
}

#ifdef REGISTER_FILTER
CUnknown* WINAPI CRealAudioDecoder::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CRealAudioDecoder(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}
#endif

HRESULT CRealAudioDecoder::InitRA(const CMediaType* pmt)
{
	FreeRA();

	HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

	if(RAOpenCodec2 && FAILED(hr = RAOpenCodec2(&m_dwCookie, m_dllpath))
	|| RAOpenCodec && FAILED(hr = RAOpenCodec(&m_dwCookie)))
		return hr;

	WAVEFORMATEX* pwfe = (WAVEFORMATEX*)pmt->Format();

	// someone might be doing cbSize = sizeof(WAVEFORMATEX), chances of 
	// cbSize being really sizeof(WAVEFORMATEX) is less than this, 
	// especially with our rm splitter ;)
	DWORD cbSize = pwfe->cbSize;
	if(cbSize == sizeof(WAVEFORMATEX)) {ASSERT(0); cbSize = 0;}

	if(pmt->FormatLength() <= sizeof(WAVEFORMATEX) + cbSize) // must have type_specific_data appended
		return hr;

	BYTE* tsd = pmt->Format() + sizeof(WAVEFORMATEX) + cbSize;
	BYTE* p = NULL;

	m_rai = *(rainfo*)tsd;
	m_rai.bswap();

	if(m_rai.version2 == 4)
	{
		p = (BYTE*)((rainfo4*)tsd+1);
		int len = *p++; p += len; len = *p++; p += len; 
		ASSERT(len == 4);		
	}
	else if(m_rai.version2 == 5)
	{
		p = (BYTE*)((rainfo5*)tsd+1);
	}
	else
	{
		return hr;
	}

	p += 3;
	if(m_rai.version2 == 5) p++;

	#pragma pack(push, 1)
	struct {DWORD freq; WORD bpsample, channels, quality; DWORD bpframe, packetsize, extralen; void* extra;} i =
		{pwfe->nSamplesPerSec, pwfe->wBitsPerSample, pwfe->nChannels, 100, 
		m_rai.sub_packet_size, m_rai.coded_frame_size, *(DWORD*)p, p + 4};
	#pragma pack(pop)

	if(FAILED(hr = RAInitDecoder(m_dwCookie, &i)))
		return hr;

	if(RASetPwd)
		RASetPwd(m_dwCookie, "Ardubancel Quazanga");

	if(FAILED(hr = RASetFlavor(m_dwCookie, m_rai.flavor)))
		return hr;

	return hr;
}

void CRealAudioDecoder::FreeRA()
{
	if(m_dwCookie)
	{
		RAFreeDecoder(m_dwCookie);
		RACloseCodec(m_dwCookie);
		m_dwCookie = 0;
	}
}

HRESULT CRealAudioDecoder::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;
	BYTE* pDataInOrg = pDataIn;

	long len = pIn->GetActualDataLength();
	if(len <= 0) return S_OK;

	REFERENCE_TIME rtStart, rtStop;
	pIn->GetTime(&rtStart, &rtStop);

	if(pIn->IsPreroll() == S_OK || rtStart < 0)
		return S_OK;

	//

	if(S_OK == pIn->IsSyncPoint())
	{
		m_bufflen = 0;
		m_rtBuffStart = rtStart;
		m_fBuffDiscontinuity = pIn->IsDiscontinuity() == S_OK;
	}

	memcpy(&m_buff[m_bufflen], pDataIn, len);
	m_bufflen += len;

	if(S_OK != pIn->IsSyncPoint())
	{
		int w = m_rai.coded_frame_size;
		int h = m_rai.sub_packet_h;
		int sps = m_rai.sub_packet_size;

		len = w*h;

		if(m_bufflen >= len)
		{
			ASSERT(m_bufflen == len);

			BYTE* src = m_buff;
			BYTE* dst = m_buff + len;

			if(sps > 0
			&& (m_pInput->CurrentMediaType().subtype == MEDIASUBTYPE_COOK
			|| m_pInput->CurrentMediaType().subtype == MEDIASUBTYPE_ATRC))
			{
				for(int y = 0; y < h; y++)
				{
					for(int x = 0, w2 = w / sps; x < w2; x++)
					{
						// TRACE(_T("--- %d, %d\n"), (h*x+((h+1)/2)*(y&1)+(y>>1)), sps*(h*x+((h+1)/2)*(y&1)+(y>>1)));
						memcpy(dst + sps*(h*x+((h+1)/2)*(y&1)+(y>>1)), src, sps);
						src += sps;
					}
				}

				src = m_buff + len;
				dst = m_buff + len*2;
			}
			else if(m_pInput->CurrentMediaType().subtype == MEDIASUBTYPE_SIPR)
			{
				// http://mplayerhq.hu/pipermail/mplayer-dev-eng/2002-August/010569.html

				static BYTE sipr_swaps[38][2]={
					{0,63},{1,22},{2,44},{3,90},{5,81},{7,31},{8,86},{9,58},{10,36},{12,68},
					{13,39},{14,73},{15,53},{16,69},{17,57},{19,88},{20,34},{21,71},{24,46},
					{25,94},{26,54},{28,75},{29,50},{32,70},{33,92},{35,74},{38,85},{40,56},
					{42,87},{43,65},{45,59},{48,79},{49,93},{51,89},{55,95},{61,76},{67,83},
					{77,80} };

				int bs=h*w*2/96; // nibbles per subpacket
				for(int n=0;n<38;n++){
					int i=bs*sipr_swaps[n][0];
					int o=bs*sipr_swaps[n][1];
				// swap nibbles of block 'i' with 'o'      TODO: optimize
				for(int j=0;j<bs;j++){
					int x=(i&1) ? (src[(i>>1)]>>4) : (src[(i>>1)]&15);
					int y=(o&1) ? (src[(o>>1)]>>4) : (src[(o>>1)]&15);
					if(o&1) src[(o>>1)]=(src[(o>>1)]&0x0F)|(x<<4);
						else  src[(o>>1)]=(src[(o>>1)]&0xF0)|x;
					if(i&1) src[(i>>1)]=(src[(i>>1)]&0x0F)|(y<<4);
						else  src[(i>>1)]=(src[(i>>1)]&0xF0)|y;
					++i;++o;
				}
				}
			}

			rtStart = m_rtBuffStart;

			for(; src < dst; src += w)
			{
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

				hr = RADecode(m_dwCookie, src, w, pDataOut, &len, -1);

				if(FAILED(hr))
				{
					TRACE(_T("RA returned an error code!!!\n"));
					continue;
//					return hr;
				}

				WAVEFORMATEX* pwfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

				rtStop = rtStart + 1000i64*len/pwfe->nAvgBytesPerSec*10000;
				pOut->SetTime(&rtStart, &rtStop);
				pOut->SetMediaTime(NULL, NULL);

				pOut->SetDiscontinuity(m_fBuffDiscontinuity); m_fBuffDiscontinuity = false;
				pOut->SetSyncPoint(TRUE);

				pOut->SetActualDataLength(len);

DbgLog((LOG_TRACE, 0, _T("A: rtStart=%I64d, rtStop=%I64d, disc=%d, sync=%d"), 
	   rtStart, rtStop, pOut->IsDiscontinuity() == S_OK, pOut->IsSyncPoint() == S_OK));

				if(S_OK != (hr = m_pOutput->Deliver(pOut)))
					return hr;

				rtStart = rtStop;
			}

			m_bufflen = 0;
		}
	}

	return S_OK;
}

HRESULT CRealAudioDecoder::CheckInputType(const CMediaType* mtIn)
{
	if(mtIn->majortype != MEDIATYPE_Audio 
	|| mtIn->subtype != MEDIASUBTYPE_14_4
	&& mtIn->subtype != MEDIASUBTYPE_28_8
	&& mtIn->subtype != MEDIASUBTYPE_ATRC
	&& mtIn->subtype != MEDIASUBTYPE_COOK
	&& mtIn->subtype != MEDIASUBTYPE_DNET
	&& mtIn->subtype != MEDIASUBTYPE_SIPR)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if(!m_pInput->IsConnected())
	{
		if(m_hDrvDll) {FreeLibrary(m_hDrvDll); m_hDrvDll = NULL;}

		CStringList paths;
		CString olddll, newdll, oldpath, newpath;

		olddll.Format(_T("%c%c%c%c3260.dll"), 
			(TCHAR)((mtIn->subtype.Data1>>0)&0xff),
			(TCHAR)((mtIn->subtype.Data1>>8)&0xff),
			(TCHAR)((mtIn->subtype.Data1>>16)&0xff),
			(TCHAR)((mtIn->subtype.Data1>>24)&0xff));

		newdll.Format(_T("%c%c%c%c.dll"), 
			(TCHAR)((mtIn->subtype.Data1>>0)&0xff),
			(TCHAR)((mtIn->subtype.Data1>>8)&0xff),
			(TCHAR)((mtIn->subtype.Data1>>16)&0xff),
			(TCHAR)((mtIn->subtype.Data1>>24)&0xff));

		CRegKey key;
		TCHAR buff[MAX_PATH];
		ULONG len = sizeof(buff);
		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Software\\RealNetworks\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			oldpath = buff;
			TCHAR c = oldpath[oldpath.GetLength()-1];
			if(c != '\\' && c != '/') oldpath += '\\';
			key.Close();
		}
		len = sizeof(buff);
		if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Helix\\HelixSDK\\10.0\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			newpath = buff;
			TCHAR c = newpath[newpath.GetLength()-1];
			if(c != '\\' && c != '/') newpath += '\\';
			key.Close();
		}
		if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Helix\\HelixSDK\\10.0\\Preferences\\DT_Codecs"), KEY_READ)
		&& ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && _tcslen(buff) > 0)
		{
			newpath = buff;
			TCHAR c = newpath[newpath.GetLength()-1];
			if(c != '\\' && c != '/') newpath += '\\';
			key.Close();
		}

		if(!newpath.IsEmpty()) paths.AddTail(newpath + newdll);
		if(!oldpath.IsEmpty()) paths.AddTail(oldpath + newdll);
		paths.AddTail(newdll); // default dll paths
		if(!newpath.IsEmpty()) paths.AddTail(newpath + olddll);
		if(!oldpath.IsEmpty()) paths.AddTail(oldpath + olddll);
		paths.AddTail(olddll); // default dll paths

		POSITION pos = paths.GetHeadPosition();
		while(pos && !(m_hDrvDll = LoadLibrary(paths.GetNext(pos))));

		if(m_hDrvDll)
		{
			RACloseCodec = (PCloseCodec)GetProcAddress(m_hDrvDll, "RACloseCodec");
			RADecode = (PDecode)GetProcAddress(m_hDrvDll, "RADecode");
			RAFlush = (PFlush)GetProcAddress(m_hDrvDll, "RAFlush");
			RAFreeDecoder = (PFreeDecoder)GetProcAddress(m_hDrvDll, "RAFreeDecoder");
			RAGetFlavorProperty = (PGetFlavorProperty)GetProcAddress(m_hDrvDll, "RAGetFlavorProperty");
			RAInitDecoder = (PInitDecoder)GetProcAddress(m_hDrvDll, "RAInitDecoder");
			RAOpenCodec = (POpenCodec)GetProcAddress(m_hDrvDll, "RAOpenCodec");
			RAOpenCodec2 = (POpenCodec2)GetProcAddress(m_hDrvDll, "RAOpenCodec2");
			RASetFlavor = (PSetFlavor)GetProcAddress(m_hDrvDll, "RASetFlavor");
			RASetDLLAccessPath = (PSetDLLAccessPath)GetProcAddress(m_hDrvDll, "RASetDLLAccessPath");
			RASetPwd = (PSetPwd)GetProcAddress(m_hDrvDll, "RASetPwd");
		}

		if(!m_hDrvDll || !RACloseCodec || !RADecode /*|| !RAFlush*/
		|| !RAFreeDecoder || !RAGetFlavorProperty || !RAInitDecoder 
		|| !(RAOpenCodec || RAOpenCodec2) || !RASetFlavor)
			return VFW_E_TYPE_NOT_ACCEPTED;

		if(m_hDrvDll)
		{
			char buff[MAX_PATH];
			GetModuleFileNameA(m_hDrvDll, buff, MAX_PATH);
			CPathA p(buff);
			p.RemoveFileSpec();
			p.AddBackslash();
			m_dllpath = p.m_strPath;
			if(RASetDLLAccessPath)
				RASetDLLAccessPath("DT_Codecs=" + m_dllpath);
		}

		if(FAILED(InitRA(mtIn)))
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CRealAudioDecoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return mtIn->majortype == MEDIATYPE_Audio && (mtIn->subtype == MEDIASUBTYPE_14_4
												|| mtIn->subtype == MEDIASUBTYPE_28_8
												|| mtIn->subtype == MEDIASUBTYPE_ATRC
												|| mtIn->subtype == MEDIASUBTYPE_COOK
												|| mtIn->subtype == MEDIASUBTYPE_DNET
												|| mtIn->subtype == MEDIASUBTYPE_SIPR)
		&& mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM												
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
	// TODO
}

HRESULT CRealAudioDecoder::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CComPtr<IMemAllocator> pAllocatorIn;
	m_pInput->GetAllocator(&pAllocatorIn);
	if(!pAllocatorIn) return E_UNEXPECTED;

	WAVEFORMATEX* pwfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

	// ok, maybe this is too much...
	pProperties->cBuffers = 8;
	pProperties->cbBuffer = pwfe->nChannels*pwfe->nSamplesPerSec*pwfe->wBitsPerSample>>3; // nAvgBytesPerSec;
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

HRESULT CRealAudioDecoder::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	*pmt = m_pInput->CurrentMediaType();
	pmt->subtype = MEDIASUBTYPE_PCM;
	WAVEFORMATEX* pwfe = (WAVEFORMATEX*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEX));

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > (pwfe->nChannels > 2 && pwfe->nChannels <= 6 ? 1 : 0)) return VFW_S_NO_MORE_ITEMS;

	pwfe->cbSize = 0;
	pwfe->wFormatTag = WAVE_FORMAT_PCM;
	pwfe->nBlockAlign = pwfe->nChannels*pwfe->wBitsPerSample>>3;
	pwfe->nAvgBytesPerSec = pwfe->nSamplesPerSec*pwfe->nBlockAlign;

	if(iPosition == 0 && pwfe->nChannels > 2 && pwfe->nChannels <= 6)
	{
		static DWORD chmask[] = 
		{
			KSAUDIO_SPEAKER_DIRECTOUT,
			KSAUDIO_SPEAKER_MONO,
			KSAUDIO_SPEAKER_STEREO,
			KSAUDIO_SPEAKER_STEREO|SPEAKER_FRONT_CENTER,
			KSAUDIO_SPEAKER_QUAD,
			KSAUDIO_SPEAKER_QUAD|SPEAKER_FRONT_CENTER,
			KSAUDIO_SPEAKER_5POINT1
		};
		
		WAVEFORMATEXTENSIBLE* pwfex = (WAVEFORMATEXTENSIBLE*)pmt->ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
		pwfex->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		pwfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		pwfex->dwChannelMask = chmask[pwfex->Format.nChannels];
		pwfex->Samples.wValidBitsPerSample = pwfex->Format.wBitsPerSample;
		pwfex->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	}

	return S_OK;
}

HRESULT CRealAudioDecoder::StartStreaming()
{
	int w = m_rai.coded_frame_size;
	int h = m_rai.sub_packet_h;
	int sps = m_rai.sub_packet_size;

	m_buff.Allocate(w*h*2);
	m_bufflen = 0;
	m_rtBuffStart = 0;

	return __super::StartStreaming();
}

HRESULT CRealAudioDecoder::StopStreaming()
{
	m_buff.Free();
	m_bufflen = 0;

	return __super::StopStreaming();
}

HRESULT CRealAudioDecoder::EndOfStream()
{
	return __super::EndOfStream();
}

HRESULT CRealAudioDecoder::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CRealAudioDecoder::EndFlush()
{
	return __super::EndFlush();
}

HRESULT CRealAudioDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_tStart = tStart;
	m_bufflen = 0;
	m_rtBuffStart = 0;
	return __super::NewSegment(tStart, tStop, dRate);
}
