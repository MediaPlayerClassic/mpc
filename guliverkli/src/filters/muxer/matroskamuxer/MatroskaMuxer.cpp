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
#include "MatroskaMuxer.h"

#include <initguid.h>
#include "..\..\..\..\include\matroska\matroska.h"
#include "..\..\..\..\include\ogg\OggDS.h"
#include "..\..\..\..\include\moreuuids.h"

using namespace MatroskaWriter;

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_Matroska},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",				// Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      TRUE,                 // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      0,					// Number of types
      NULL					// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
      sudPinTypesOut        // Pin information
    },
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{ &__uuidof(CMatroskaMuxerFilter)		// Filter CLSID
    , L"Matroska Muxer"						// String name
    , MERIT_DO_NOT_USE						// Filter merit
    , sizeof(sudpPins)/sizeof(sudpPins[0])	// Number of pins
	, sudpPins}							// Pin information
};

CFactoryTemplate g_Templates[] =
{
	{ L"Matroska Muxer"
	, &__uuidof(CMatroskaMuxerFilter)
	, CMatroskaMuxerFilter::CreateInstance
	, NULL
	, &sudFilter[0]},
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

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI CMatroskaMuxerFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMatroskaMuxerFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// CMatroskaMuxerFilter
//

CMatroskaMuxerFilter::CMatroskaMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(NAME("CMatroskaMuxerFilter"), pUnk, this, __uuidof(this))
	, m_rtCurrent(0)
{
	if(phr) *phr = S_OK;

	m_pOutput.Attach(new CMatroskaMuxerOutputPin(NAME("CMatroskaMuxerOutputPin"), this, this, phr));

	AddInput();

	srand(clock());
}

CMatroskaMuxerFilter::~CMatroskaMuxerFilter()
{
	CAutoLock cAutoLock(this);
}

STDMETHODIMP CMatroskaMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
//		QI(IAMFilterMiscFlags)
		QI(IMediaSeeking)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

UINT CMatroskaMuxerFilter::GetTrackNumber(CBasePin* pPin)
{
	UINT nTrackNumber = 0;

	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos)
	{
		nTrackNumber++;
		if(m_pInputs.GetNext(pos) == pPin)
			return nTrackNumber;
	}

	return 0;
}

void CMatroskaMuxerFilter::AddInput()
{
	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos)
	{
		CBasePin* pPin = m_pInputs.GetNext(pos);
		if(!pPin->IsConnected()) return;
	}

	CStringW name;
	name.Format(L"Track %d", m_pInputs.GetCount()+1);
	
	HRESULT hr;
	CAutoPtr<CMatroskaMuxerInputPin> pPin(new CMatroskaMuxerInputPin(name, this, this, &hr));
	m_pInputs.AddTail(pPin);
}

int CMatroskaMuxerFilter::GetPinCount()
{
	return m_pInputs.GetCount() + (m_pOutput ? 1 : 0);
}

CBasePin* CMatroskaMuxerFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n >= 0 && n < (int)m_pInputs.GetCount())
	{
		if(POSITION pos = m_pInputs.FindIndex(n))
			return m_pInputs.GetAt(pos);
	}

	if(n == m_pInputs.GetCount() && m_pOutput)
	{
		return m_pOutput;
	}

	return NULL;
}

STDMETHODIMP CMatroskaMuxerFilter::Stop()
{
	CAutoLock cAutoLock(this);

	HRESULT hr;
	
	if(FAILED(hr = __super::Stop()))
		return hr;

	CallWorker(CMD_EXIT);

	return hr;
}

STDMETHODIMP CMatroskaMuxerFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr;

	if(FAILED(hr = __super::Pause()))
		return hr;

	if(fs == State_Stopped && m_pOutput)
	{
		CAMThread::Create();
		CallWorker(CMD_RUN);
	}

	return hr;
}

STDMETHODIMP CMatroskaMuxerFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	if(FAILED(hr = __super::Run(tStart)))
		return hr;

	return hr;
}

// IAMFilterMiscFlags

STDMETHODIMP_(ULONG) CMatroskaMuxerFilter::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

// IMediaSeeking

STDMETHODIMP CMatroskaMuxerFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = 
		AM_SEEKING_CanGetDuration|
		AM_SEEKING_CanGetCurrentPos, S_OK : E_POINTER;
}
STDMETHODIMP CMatroskaMuxerFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if(*pCapabilities == 0) return S_OK;
	DWORD caps;
	GetCapabilities(&caps);
	caps &= *pCapabilities;
	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}
STDMETHODIMP CMatroskaMuxerFilter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CMatroskaMuxerFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CMatroskaMuxerFilter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CMatroskaMuxerFilter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CMatroskaMuxerFilter::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
STDMETHODIMP CMatroskaMuxerFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	*pDuration = 0;
	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos) {REFERENCE_TIME rt = m_pInputs.GetNext(pos)->m_rtDur; if(rt > *pDuration) *pDuration = rt;}
	return S_OK;
}
STDMETHODIMP CMatroskaMuxerFilter::GetStopPosition(LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	CheckPointer(pCurrent, E_POINTER);
	*pCurrent = m_rtCurrent;
	return S_OK;
}
STDMETHODIMP CMatroskaMuxerFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::SetRate(double dRate) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::GetRate(double* pdRate) {return E_NOTIMPL;}
STDMETHODIMP CMatroskaMuxerFilter::GetPreroll(LONGLONG* pllPreroll) {return E_NOTIMPL;}

//

ULONGLONG GetStreamPosition(IStream* pStream)
{
	ULARGE_INTEGER pos = {0, 0};
	pStream->Seek(*(LARGE_INTEGER*)&pos, STREAM_SEEK_CUR, &pos);
	return pos.QuadPart;
}

ULONGLONG SetStreamPosition(IStream* pStream, ULONGLONG seekpos)
{
	LARGE_INTEGER pos;
	pos.QuadPart = seekpos;
	ULARGE_INTEGER posnew;
	posnew.QuadPart = GetStreamPosition(pStream);
	pStream->Seek(pos, STREAM_SEEK_SET, &posnew);
	return posnew.QuadPart;
}

DWORD CMatroskaMuxerFilter::ThreadProc()
{
	CComQIPtr<IStream> pStream;
	
	if(!m_pOutput || !(pStream = m_pOutput->GetConnected()))
	{
		while(1)
		{
			DWORD cmd = GetRequest();
			if(cmd == CMD_EXIT) CAMThread::m_hThread = NULL;
			Reply(S_OK);
			if(cmd == CMD_EXIT) return 0;
		}
	}

	REFERENCE_TIME rtDur = 0;
	GetDuration(&rtDur);

	SetStreamPosition(pStream, 0);

	ULARGE_INTEGER uli = {0};
	pStream->SetSize(uli);

	EBML hdr;
	hdr.DocType.Set("matroska");
	hdr.DocTypeVersion.Set(1);
	hdr.DocTypeReadVersion.Set(1);
	hdr.Write(pStream);

	Segment().Write(pStream);
	ULONGLONG segpos = GetStreamPosition(pStream);

	// TODO
	QWORD voidlen = 100;
	if(rtDur > 0) voidlen += int(1.0 * rtDur / MAXCLUSTERTIME / 10000 + 0.5) * 20;
	else voidlen += int(1.0 * 1000*60*60*24 / MAXCLUSTERTIME + 0.5) * 20; // when no duration is known, allocate for 24 hours (~340k)
	ULONGLONG voidpos = GetStreamPosition(pStream);
	{
		Void v(voidlen);
		voidlen = v.Size();
		v.Write(pStream);
	}

	// Meta Seek

	Seek seek;
	CAutoPtr<SeekHead> sh;

	// Segment Info

	sh.Attach(new SeekHead());
	sh->ID.Set(0x1549A966);
	sh->Position.Set(GetStreamPosition(pStream) - segpos);
	seek.SeekHeads.AddTail(sh);

	ULONGLONG infopos = GetStreamPosition(pStream);
	Info info;
	info.MuxingApp.Set("DirectShow Matroska Muxer");
	info.TimeCodeScale.Set(1000000);
	info.Duration.Set((float)rtDur / 10000);
	info.Write(pStream);

	// Tracks

	sh.Attach(new SeekHead());
	sh->ID.Set(0x1654AE6B);
	sh->Position.Set(GetStreamPosition(pStream) - segpos);
	seek.SeekHeads.AddTail(sh);

	UINT64 TrackNumber = 0;
/*
	CNode<Track> Tracks;
	CAutoPtr<Track> pT(new Track());
	POSITION pos = m_pInputs.GetHeadPosition();
	for(int i = 1; pos; i++)
	{
		CMatroskaMuxerInputPin* pPin = m_pInputs.GetNext(pos);
		if(!pPin->IsConnected()) continue;

		CAutoPtr<TrackEntry> pTE(new TrackEntry());
		*pTE = *pPin->GetTrackEntry();
		if(TrackNumber == 0 && pTE->TrackType == TrackEntry::TypeVideo) 
			TrackNumber = pTE->TrackNumber;
		pT->TrackEntries.AddTail(pTE);
	}
	Tracks.AddTail(pT);
	Tracks.Write(pStream);

	if(TrackNumber == 0) TrackNumber = 1;
*/
	bool fTracksWritten = false;

	//

	Cluster c;
	c.TimeCode.Set(0);

	while(1)
	{
		DWORD cmd = GetRequest();

		switch(cmd)
		{
		default:
		case CMD_EXIT:
			CAMThread::m_hThread = NULL;
			Reply(S_OK);
			return 0;

		case CMD_RUN:
			Reply(S_OK);

			Cue cue;
			ULONGLONG lastcueclusterpos = -1;
			INT64 lastcuetimecode = -1;
			UINT64 nBlocksInCueTrack = 0;

			while(!CheckRequest(NULL))
			{
				if(m_State == State_Paused)
				{
					Sleep(10);
					continue;
				}

				int nPinsGotSomething = 0, nPinsNeeded = m_pInputs.GetCount();
				CMatroskaMuxerInputPin* pPin = NULL;
				REFERENCE_TIME rtMin = _I64_MAX;

				POSITION pos = m_pInputs.GetHeadPosition();
				while(pos)
				{
					CMatroskaMuxerInputPin* pTmp = m_pInputs.GetNext(pos);

					CAutoLock cAutoLock(&pTmp->m_csQueue);

					if(pTmp->m_fECCompletSent || !pTmp->IsConnected() 
					|| pTmp->GetTrackEntry()->TrackType == TrackEntry::TypeSubtitle)
						nPinsNeeded--;
				}

				pos = m_pInputs.GetHeadPosition();
				while(pos)
				{
					CMatroskaMuxerInputPin* pTmp = m_pInputs.GetNext(pos);

					CAutoLock cAutoLock(&pTmp->m_csQueue);

					if(pTmp->m_blocks.GetCount() > 0)
					{
						nPinsGotSomething++;

						if(pTmp->m_blocks.GetCount() > 0)
						{
							REFERENCE_TIME rt = pTmp->m_blocks.GetHead()->Block.TimeCode;
							if(rt < rtMin) {rtMin = rt; pPin = pTmp;}
						}

						ASSERT(!pTmp->m_fECCompletSent);
					}
					else if(pTmp->m_fEndOfStreamReceived)
					{
//						NotifyEvent(EC_COMPLETE, 0, 0);
						pTmp->m_fEndOfStreamReceived = false;
						pTmp->m_fECCompletSent = true;
					}
				}

				if(nPinsNeeded == 0)
				{
					break;
				}

				if(nPinsNeeded > nPinsGotSomething)
				{
					Sleep(1);
					continue;
				}

				if(!fTracksWritten)
				{
					CNode<Track> Tracks;
					CAutoPtr<Track> pT(new Track());
					POSITION pos = m_pInputs.GetHeadPosition();
					for(int i = 1; pos; i++)
					{
						CMatroskaMuxerInputPin* pPin = m_pInputs.GetNext(pos);
						if(!pPin->IsConnected()) continue;

						CAutoPtr<TrackEntry> pTE(new TrackEntry());
						*pTE = *pPin->GetTrackEntry();
						if(TrackNumber == 0 && pTE->TrackType == TrackEntry::TypeVideo) 
							TrackNumber = pTE->TrackNumber;
						pT->TrackEntries.AddTail(pTE);
					}
					Tracks.AddTail(pT);
					Tracks.Write(pStream);

					if(TrackNumber == 0) TrackNumber = 1;

					fTracksWritten = true;
				}

				ASSERT(pPin);

				CAutoPtr<BlockGroup> b;

				{
					CAutoLock cAutoLock(&pPin->m_csQueue);
					b = pPin->m_blocks.RemoveHead();
				}

				if(b)
				{
/*
TRACE(_T("Muxing (%d): %I64d-%I64d dur=%I64d (c=%d, co=%dms), cnt=%d, ref=%d\n"), 
	GetTrackNumber(pPin), 
	(INT64)b->Block.TimeCode, (INT64)b->Block.TimeCodeStop, (UINT64)b->BlockDuration,
	(int)((b->Block.TimeCode)/MAXCLUSTERTIME), (int)(b->Block.TimeCode%MAXCLUSTERTIME),
	b->Block.BlockData.GetCount(), (int)b->ReferenceBlock);
*/
					if(b->Block.TimeCode < SHRT_MIN /*0*/) {ASSERT(0); continue;}

					if((INT64)(c.TimeCode + MAXCLUSTERTIME) < b->Block.TimeCode)
					{
						if(!c.BlockGroups.IsEmpty())
						{
							sh.Attach(new SeekHead());
							sh->ID.Set(0x1F43B675);
							sh->Position.Set(GetStreamPosition(pStream) - segpos);
							seek.SeekHeads.AddTail(sh);

							c.Write(pStream);
						}

						c.TimeCode.Set(c.TimeCode + MAXCLUSTERTIME);
						c.BlockGroups.RemoveAll();
						nBlocksInCueTrack = 0;
					}

					if(b->Block.TrackNumber == TrackNumber)
					{
						nBlocksInCueTrack++;
					}

					if(b->ReferenceBlock == 0 && b->Block.TrackNumber == TrackNumber) // TODO: test TrackNumber agains a video track instead of just the first one
					{
						ULONGLONG clusterpos = GetStreamPosition(pStream) - segpos;
						if(lastcueclusterpos != clusterpos || lastcuetimecode + 1000 < b->Block.TimeCode)
						{
							CAutoPtr<CueTrackPosition> ctp(new CueTrackPosition());
							ctp->CueTrack.Set(b->Block.TrackNumber);
							ctp->CueClusterPosition.Set(clusterpos);
							if(c.BlockGroups.GetCount() > 0) ctp->CueBlockNumber.Set(nBlocksInCueTrack);
							CAutoPtr<CuePoint> cp(new CuePoint());
							cp->CueTime.Set(b->Block.TimeCode);
							cp->CueTrackPositions.AddTail(ctp);
							cue.CuePoints.AddTail(cp);
							lastcueclusterpos = clusterpos;
							lastcuetimecode = b->Block.TimeCode;
						}
					}

					info.Duration.Set(max(info.Duration, (float)b->Block.TimeCodeStop));

					m_rtCurrent = b->Block.TimeCode*10000;

					b->Block.TimeCode -= c.TimeCode;
					c.BlockGroups.AddTail(b);
				}
			}

			if(!c.BlockGroups.IsEmpty())
			{
				sh.Attach(new SeekHead());
				sh->ID.Set(0x1F43B675);
				sh->Position.Set(GetStreamPosition(pStream) - segpos);
				seek.SeekHeads.AddTail(sh);

				c.Write(pStream);
			}

			ULONGLONG cuepos = 0;
			if(!cue.CuePoints.IsEmpty())
			{
				sh.Attach(new SeekHead());
				sh->ID.Set(0x1C53BB6B);
				sh->Position.Set(GetStreamPosition(pStream) - segpos);
				seek.SeekHeads.AddTail(sh);

				cue.Write(pStream);
			}

			SetStreamPosition(pStream, voidpos);
			int len = (int)(voidlen - seek.Size());
			ASSERT(len >= 0 && len != 1);
			seek.Write(pStream);

			if(len == 0)
			{
				// nothing to do
			}
			else if(len >= 2)
			{
				for(int i = 0; i < 8; i++)
				{
					if(len >= (1<<i*7)-2 && len <= (1<<(i+1)*7)-2)
					{
						Void(len-2-i).Write(pStream);
						break;
					}
				}
			}

			if(abs(m_rtCurrent - (REFERENCE_TIME)info.Duration*10000) > 10000000i64)
			{
				info.Duration.Set(m_rtCurrent / 10000 + 1);
			}

			SetStreamPosition(pStream, infopos);
			info.Write(pStream);

			// TODO: write some tags

			m_pOutput->DeliverEndOfStream();

			break;
		}
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
}

//
// CMatroskaMuxerInputPin
//

CMatroskaMuxerInputPin::CMatroskaMuxerInputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseInputPin(NAME("CMatroskaMuxerInputPin"), pFilter, pLock, phr, pName)
	, m_fActive(false)
	, m_fEndOfStreamReceived(false)
	, m_rtDur(0)
{
}

CMatroskaMuxerInputPin::~CMatroskaMuxerInputPin()
{
}

STDMETHODIMP CMatroskaMuxerInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMatroskaMuxerInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video && (pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_VideoInfo2)
		|| pmt->majortype == MEDIATYPE_Audio && (pmt->formattype == FORMAT_WaveFormatEx || pmt->formattype == FORMAT_VorbisFormat)
		|| pmt->majortype == MEDIATYPE_Audio && pmt->subtype == MEDIASUBTYPE_Vorbis && pmt->formattype == FORMAT_VorbisFormat
		|| pmt->majortype == MEDIATYPE_Audio && pmt->subtype == MEDIASUBTYPE_Vorbis2 && pmt->formattype == FORMAT_VorbisFormat2
		|| pmt->majortype == MEDIATYPE_Text && pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_None
		|| pmt->majortype == MEDIATYPE_Subtitle && pmt->formattype == FORMAT_SubtitleInfo
		? S_OK
		: E_INVALIDARG;
}

HRESULT CMatroskaMuxerInputPin::BreakConnect()
{
	HRESULT hr;

	if(FAILED(hr = __super::BreakConnect()))
		return hr;

	m_pTE.Free();

	return hr;
}

HRESULT CMatroskaMuxerInputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CompleteConnect(pPin)))
		return hr;

	m_rtDur = 0;
	CComQIPtr<IMediaSeeking> pMS;
	if((pMS = GetFilterFromPin(pPin)) || (pMS = pPin))
		pMS->GetDuration(&m_rtDur);

	m_pTE.Free();
	m_pTE.Attach(new TrackEntry());

	m_pTE->TrackUID.Set(rand());
	m_pTE->MinCache.Set(1);
	m_pTE->MaxCache.Set(1);
	m_pTE->TrackNumber.Set(((CMatroskaMuxerFilter*)m_pFilter)->GetTrackNumber(this));

	hr = E_FAIL;

	if(m_mt.majortype == MEDIATYPE_Video)
	{
		m_pTE->TrackType.Set(TrackEntry::TypeVideo);

		if(m_mt.formattype == FORMAT_VideoInfo)
		{
			m_pTE->CodecID.Set("V_MS/VFW/FOURCC");

			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader));
			memcpy(m_pTE->CodecPrivate, &vih->bmiHeader, m_pTE->CodecPrivate.GetSize());
			m_pTE->DefaultDuration.Set(vih->AvgTimePerFrame*100); 
			m_pTE->DescType = TrackEntry::DescVideo;
			m_pTE->v.PixelWidth.Set(vih->bmiHeader.biWidth);
			m_pTE->v.PixelHeight.Set(abs(vih->bmiHeader.biHeight));
			if(vih->AvgTimePerFrame > 0)
				m_pTE->v.FramePerSec.Set((float)(10000000.0 / vih->AvgTimePerFrame)); 

			hr = S_OK;
		}
		else if(m_mt.formattype == FORMAT_VideoInfo2)
		{
			m_pTE->CodecID.Set("V_MS/VFW/FOURCC");

			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat - FIELD_OFFSET(VIDEOINFOHEADER2, bmiHeader));
			memcpy(m_pTE->CodecPrivate, &vih->bmiHeader, m_pTE->CodecPrivate.GetSize());
			m_pTE->DefaultDuration.Set(vih->AvgTimePerFrame*100);
			m_pTE->DescType = TrackEntry::DescVideo;
			m_pTE->v.PixelWidth.Set(vih->bmiHeader.biWidth);
			m_pTE->v.PixelHeight.Set(abs(vih->bmiHeader.biHeight));
			m_pTE->v.DisplayWidth.Set(vih->dwPictAspectRatioX);
			m_pTE->v.DisplayHeight.Set(vih->dwPictAspectRatioY);
			if(vih->AvgTimePerFrame > 0)
				m_pTE->v.FramePerSec.Set((float)(10000000.0 / vih->AvgTimePerFrame)); 

			hr = S_OK;
		}
	}
	else if(m_mt.majortype == MEDIATYPE_Audio)
	{
		m_pTE->TrackType.Set(TrackEntry::TypeAudio);

		
		if(m_mt.formattype == FORMAT_WaveFormatEx 
		&& ((WAVEFORMATEX*)m_mt.pbFormat)->wFormatTag == WAVE_FORMAT_AAC
		&& m_mt.cbFormat == sizeof(WAVEFORMATEX)+2)
		{
			switch((*(m_mt.pbFormat + sizeof(WAVEFORMATEX)) >> 3) - 1)
			{
			default:
			case 0: m_pTE->CodecID.Set("A_AAC/MPEG2/MAIN"); break;
			case 1: m_pTE->CodecID.Set("A_AAC/MPEG2/LC"); break;
			case 2: m_pTE->CodecID.Set("A_AAC/MPEG2/SSR"); break;
			}

			WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.pbFormat;
			m_pTE->DescType = TrackEntry::DescAudio;
			m_pTE->a.SamplingFrequency.Set((float)wfe->nSamplesPerSec);
			m_pTE->a.Channels.Set(wfe->nChannels);
			m_pTE->a.BitDepth.Set(wfe->wBitsPerSample);

			hr = S_OK;
		}			
		else if(m_mt.formattype == FORMAT_WaveFormatEx)
		{
			m_pTE->CodecID.Set("A_MS/ACM");

			WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat);
			memcpy(m_pTE->CodecPrivate, wfe, m_pTE->CodecPrivate.GetSize());
			m_pTE->DescType = TrackEntry::DescAudio;
			m_pTE->a.SamplingFrequency.Set((float)wfe->nSamplesPerSec);
			m_pTE->a.Channels.Set(wfe->nChannels);
			m_pTE->a.BitDepth.Set(wfe->wBitsPerSample);

			hr = S_OK;
		}
		else if(m_mt.formattype == FORMAT_VorbisFormat)
		{
			m_pTE->CodecID.Set("A_VORBIS");

			VORBISFORMAT* pvf = (VORBISFORMAT*)m_mt.pbFormat;
			m_pTE->DescType = TrackEntry::DescAudio;
			m_pTE->a.SamplingFrequency.Set((float)pvf->nSamplesPerSec);
			m_pTE->a.Channels.Set(pvf->nChannels);

//			m_pTE->CodecPrivate.SetSize(5000); // TODO: fill this later

			hr = S_OK;
		}
		else if(m_mt.formattype == FORMAT_VorbisFormat2)
		{
			m_pTE->CodecID.Set("A_VORBIS");

			VORBISFORMAT2* pvf2 = (VORBISFORMAT2*)m_mt.pbFormat;
			m_pTE->DescType = TrackEntry::DescAudio;
			m_pTE->a.SamplingFrequency.Set((float)pvf2->SamplesPerSec);
			m_pTE->a.Channels.Set(pvf2->Channels);
			m_pTE->a.BitDepth.Set(pvf2->BitsPerSample);

			int len = 1;
			for(int i = 0; i < 2; i++) len += pvf2->HeaderSize[i]/255 + 1;
			for(int i = 0; i < 3; i++) len += pvf2->HeaderSize[i];
			m_pTE->CodecPrivate.SetSize(len);

			BYTE* src = (BYTE*)m_mt.pbFormat + sizeof(VORBISFORMAT2);
			BYTE* dst = m_pTE->CodecPrivate.GetData();

			*dst++ = 2;
			for(int i = 0; i < 2; i++)
				for(int len = pvf2->HeaderSize[i]; len >= 0; len -= 255)
					*dst++ = min(len, 255);

			memcpy(dst, src, pvf2->HeaderSize[0]); 
			dst += pvf2->HeaderSize[0]; 
			src += pvf2->HeaderSize[0];
			memcpy(dst, src, pvf2->HeaderSize[1]); 
			dst += pvf2->HeaderSize[1]; 
			src += pvf2->HeaderSize[1];
			memcpy(dst, src, pvf2->HeaderSize[2]); 
			dst += pvf2->HeaderSize[2]; 
			src += pvf2->HeaderSize[2];

			ASSERT(src <= m_mt.pbFormat + m_mt.cbFormat);
			ASSERT(dst <= m_pTE->CodecPrivate.GetData() + m_pTE->CodecPrivate.GetSize());

			hr = S_OK;
		}
	}
	else if(m_mt.majortype == MEDIATYPE_Text)
	{
		m_pTE->TrackType.Set(TrackEntry::TypeSubtitle);

		if(m_mt.formattype == FORMAT_None)
		{
			m_pTE->CodecID.Set("S_TEXT/ASCII");

			hr = S_OK;
		}
	}
	else if(m_mt.majortype == MEDIATYPE_Subtitle)
	{
		m_pTE->TrackType.Set(TrackEntry::TypeSubtitle);

		if(m_mt.subtype == MEDIASUBTYPE_UTF8 && m_mt.formattype == FORMAT_SubtitleInfo)
		{
			m_pTE->CodecID.Set("S_TEXT/UTF8");

			SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat);
			memcpy(m_pTE->CodecPrivate, psi, m_pTE->CodecPrivate.GetSize());

			hr = S_OK;
		}
		else if(m_mt.subtype == MEDIASUBTYPE_SSA && m_mt.formattype == FORMAT_SubtitleInfo)
		{
			m_pTE->CodecID.Set("S_SSA");

			SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat);
			memcpy(m_pTE->CodecPrivate, psi, m_pTE->CodecPrivate.GetSize());

			hr = S_OK;
		}
		else if(m_mt.subtype == MEDIASUBTYPE_ASS && m_mt.formattype == FORMAT_SubtitleInfo)
		{
			m_pTE->CodecID.Set("S_ASS");

			SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat);
			memcpy(m_pTE->CodecPrivate, psi, m_pTE->CodecPrivate.GetSize());

			hr = S_OK;
		}
		else if(m_mt.subtype == MEDIASUBTYPE_USF && m_mt.formattype == FORMAT_SubtitleInfo)
		{
			m_pTE->CodecID.Set("S_USF");

			SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
			m_pTE->CodecPrivate.SetSize(m_mt.cbFormat);
			memcpy(m_pTE->CodecPrivate, psi, m_pTE->CodecPrivate.GetSize());

			hr = S_OK;
		}
	}

	if(S_OK == hr)
	{
		((CMatroskaMuxerFilter*)m_pFilter)->AddInput();
	}

	return hr;
}

HRESULT CMatroskaMuxerInputPin::Active()
{
	m_fActive = true;
	m_fECCompletSent = false;
	m_rtLastStart = m_rtLastStop = -1;
	return __super::Active();
}

HRESULT CMatroskaMuxerInputPin::Inactive()
{
	m_fActive = false;
	CAutoLock cAutoLock(&m_csQueue);
	m_fEndOfStreamReceived = false;
	m_blocks.RemoveAll();
	m_pVorbisHdrs.RemoveAll();
	return __super::Inactive();
}

STDMETHODIMP CMatroskaMuxerInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CMatroskaMuxerInputPin::BeginFlush()
{
	return __super::BeginFlush();
}

STDMETHODIMP CMatroskaMuxerInputPin::EndFlush()
{
	return __super::EndFlush();
}

STDMETHODIMP CMatroskaMuxerInputPin::Receive(IMediaSample* pSample)
{
	if(m_fEndOfStreamReceived) {ASSERT(0); return S_FALSE;}

	CAutoLock cAutoLock(&m_csReceive);

	while(m_fActive)
	{
		{
			CAutoLock cAutoLock2(&m_csQueue);
			if(m_blocks.GetCount() < MAXBLOCKS)
				break;
		}

		Sleep(1);
	}

	if(!m_fActive) return S_FALSE;

	HRESULT hr;

	if(FAILED(hr = __super::Receive(pSample)))
		return hr;

	REFERENCE_TIME rtStart = -1, rtStop = -1;
	hr = pSample->GetTime(&rtStart, &rtStop);

	if(FAILED(hr) || rtStart == -1 || rtStop == -1)
	{
		CString err(_T("No timestamp was set on the sample!!!"));
		TRACE(err);
		m_pFilter->NotifyEvent(EC_ERRORABORT, VFW_E_SAMPLE_TIME_NOT_SET, 0);
		return VFW_E_SAMPLE_TIME_NOT_SET;
	}

	rtStart += m_tStart;
	rtStop += m_tStart;

	BYTE* pData = NULL;
	pSample->GetPointer(&pData);

	long len = pSample->GetActualDataLength();
/*
	TRACE(_T("Received (%d): %I64d-%I64d (c=%d, co=%dms), len=%d, d%d p%d s%d\n"), 
		((CMatroskaMuxerFilter*)m_pFilter)->GetTrackNumber(this), 
		rtStart, rtStop, (int)((rtStart/10000)/MAXCLUSTERTIME), (int)((rtStart/10000)%MAXCLUSTERTIME),
		len,
		pSample->IsDiscontinuity() == S_OK ? 1 : 0,
		pSample->IsPreroll() == S_OK ? 1 : 0,
		pSample->IsSyncPoint() == S_OK ? 1 : 0);
*/
	if(m_mt.subtype == MEDIASUBTYPE_Vorbis && m_pVorbisHdrs.GetCount() < 3)
	{
		CAutoPtr<CBinary> data(new CBinary(0));
		data->SetSize(len);
		memcpy(data->GetData(), pData, len);
		m_pVorbisHdrs.Add(data);

		if(m_pVorbisHdrs.GetCount() == 3)
		{
			int len = 1;
			for(int i = 0; i < 2; i++) len += m_pVorbisHdrs[i]->GetSize()/255 + 1;
			for(int i = 0; i < 3; i++) len += m_pVorbisHdrs[i]->GetSize();
			m_pTE->CodecPrivate.SetSize(len);

			BYTE* dst = m_pTE->CodecPrivate.GetData();

			*dst++ = 2;
			for(int i = 0; i < 2; i++)
				for(int len = m_pVorbisHdrs[i]->GetSize(); len >= 0; len -= 255)
					*dst++ = min(len, 255);

			for(int i = 0; i < 3; i++)
			{
				memcpy(dst, m_pVorbisHdrs[i]->GetData(), m_pVorbisHdrs[i]->GetSize());
				dst += m_pVorbisHdrs[i]->GetSize(); 
			}
		}

		return S_OK;
	}

	CAutoPtr<BlockGroup> b(new BlockGroup());

	if(S_OK != pSample->IsSyncPoint() && m_rtLastStart >= 0 && m_rtLastStart < rtStart)
	{
		b->ReferenceBlock.Set((m_rtLastStart - rtStart) / 10000);
	}

	b->Block.TrackNumber = ((CMatroskaMuxerFilter*)m_pFilter)->GetTrackNumber(this);

	b->Block.TimeCode = rtStart / 10000;
	b->Block.TimeCodeStop = rtStop / 10000;

	if(m_pTE->TrackType == TrackEntry::TypeSubtitle)
	{
		b->BlockDuration.Set((rtStop - rtStart) / 10000);
	}

	CAutoPtr<CBinary> data(new CBinary(0));
	data->SetSize(len);
	memcpy(data->GetData(), pData, len);
	b->Block.BlockData.AddTail(data);

	CAutoLock cAutoLock2(&m_csQueue);
	m_blocks.AddTail(b); // TODO: lacing for audio

	m_rtLastStart = rtStart;
	m_rtLastStop = rtStop;

	return S_OK;
}

STDMETHODIMP CMatroskaMuxerInputPin::EndOfStream()
{
	HRESULT hr;

	if(FAILED(hr = __super::EndOfStream()))
		return hr;

	CAutoLock cAutoLock(&m_csQueue);

	m_fEndOfStreamReceived = true;

	return hr;
}

//
// CMatroskaMuxerOutputPin
//

CMatroskaMuxerOutputPin::CMatroskaMuxerOutputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(pName, pFilter, pLock, phr, L"Output")
{
}

CMatroskaMuxerOutputPin::~CMatroskaMuxerOutputPin()
{
}

STDMETHODIMP CMatroskaMuxerOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMatroskaMuxerOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1;

    ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
    ASSERT(Actual.cBuffers == pProperties->cBuffers);

    return NOERROR;
}

HRESULT CMatroskaMuxerOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_Matroska
		? S_OK
		: E_INVALIDARG;
}

HRESULT CMatroskaMuxerOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->ResetFormatBuffer();
	pmt->InitMediaType();
	pmt->majortype = MEDIATYPE_Stream;
	pmt->subtype = MEDIASUBTYPE_Matroska;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

STDMETHODIMP CMatroskaMuxerOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}


