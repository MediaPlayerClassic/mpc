/* 
 *	Copyright (C) 2003-2006 Gabest
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
#include <aviriff.h>
#include "BaseMuxer.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

//
// CBaseMuxerFilter
//

CBaseMuxerFilter::CBaseMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(NAME("CBaseMuxerFilter"), pUnk, this, clsid)
	, m_rtCurrent(0)
{
	if(phr) *phr = S_OK;
	m_pOutput.Attach(new CBaseMuxerOutputPin(L"Output", this, this, phr));
	AddInput();
}

CBaseMuxerFilter::~CBaseMuxerFilter()
{
}

STDMETHODIMP CBaseMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(IMediaSeeking)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

//

void CBaseMuxerFilter::AddInput()
{
	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos)
	{
		CBasePin* pPin = m_pInputs.GetNext(pos);
		if(!pPin->IsConnected()) return;
	}

	CStringW name;

	name.Format(L"Input %d", m_pInputs.GetCount()+1);

	CBaseMuxerInputPin* pInputPin = NULL;
	if(FAILED(CreateInput(name, &pInputPin)) || !pInputPin) {ASSERT(0); return;}
	CAutoPtr<CBaseMuxerInputPin> pAutoPtrInputPin(pInputPin);

	name.Format(L"~Output %d", m_pOutputs.GetCount()+1);

	CBaseMuxerOutputPin* pOutputPin = NULL;
	if(FAILED(CreateOutput(name, &pOutputPin)) || !pOutputPin) {ASSERT(0); return;}
	CAutoPtr<CBaseMuxerOutputPin> pAutoPtrOutputPin(pOutputPin);

	pInputPin->SetRelatedPin(pOutputPin);
	pOutputPin->SetRelatedPin(pInputPin);

	m_pInputs.AddTail(pAutoPtrInputPin);
	m_pOutputs.AddTail(pAutoPtrOutputPin);
}

HRESULT CBaseMuxerFilter::CreateInput(CStringW name, CBaseMuxerInputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	HRESULT hr = S_OK;
	*ppPin = new CBaseMuxerInputPin(name, this, this, &hr);
	return hr;
}

HRESULT CBaseMuxerFilter::CreateOutput(CStringW name, CBaseMuxerOutputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	HRESULT hr = S_OK;
	*ppPin = new CBaseMuxerOutputPin(name, this, this, &hr);
	return hr;
}

//

DWORD CBaseMuxerFilter::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	POSITION pos;

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
			m_pActivePins.RemoveAll();
			m_pPins.RemoveAll();

			pos = m_pInputs.GetHeadPosition();
			while(pos)
			{
				CBaseMuxerInputPin* pPin = m_pInputs.GetNext(pos);
				if(pPin->IsConnected())
				{
					m_pActivePins.AddTail(pPin);
					m_pPins.AddTail(pPin);
				}
			}

			m_rtCurrent = 0;

			Reply(S_OK);

			MuxInit();

			try
			{
				MuxHeaderInternal();

				while(!CheckRequest(NULL) && m_pActivePins.GetCount())
				{
					if(m_State == State_Paused) {Sleep(10); continue;}

					CAutoPtr<MuxerPacket> pPacket = GetPacket();
					if(!pPacket) {Sleep(1); continue;}

					if(pPacket->IsTimeValid())
						m_rtCurrent = pPacket->rtStart;

					if(pPacket->IsEOS())
						m_pActivePins.RemoveAt(m_pActivePins.Find(pPacket->pPin));
					
					MuxPacketInternal(pPacket);
				}

				MuxFooterInternal();
			}
			catch(HRESULT hr)
			{
				CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_ERRORABORT, hr, 0);
			}

			m_pOutput->DeliverEndOfStream();

			pos = m_pOutputs.GetHeadPosition();
			while(pos) m_pOutputs.GetNext(pos)->DeliverEndOfStream();

			m_pActivePins.RemoveAll();
			m_pPins.RemoveAll();

			break;
		}
	}

	ASSERT(0); // this function should only return via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
}

void CBaseMuxerFilter::MuxHeaderInternal()
{
	TRACE(_T("MuxHeader\n"));

	if(CComQIPtr<IBitStream> pBitStream = m_pOutput->GetBitStream())
		MuxHeader(pBitStream);

	MuxHeader();

	//

	POSITION pos = m_pPins.GetHeadPosition();
	while(pos)
	{
		CBaseMuxerInputPin* pInput = m_pPins.GetNext(pos);
		
		CBaseMuxerOutputPin* pOutput = dynamic_cast<CBaseMuxerOutputPin*>(pInput->GetRelatedPin());
		if(!pOutput) continue;

		CComQIPtr<IBitStream> pBitStream = pOutput->GetBitStream();
		if(!pBitStream) continue;

		const CMediaType& mt = pInput->CurrentMediaType();

		//

		const BYTE utf8bom[3] = {0xef, 0xbb, 0xbf};

		if((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO)
		{
			MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.Format();

			for(DWORD i = 0; i < vih->cbSequenceHeader-2; i += 2)
			{
				pBitStream->BitWrite(0x00000001, 32);
				WORD size = (((BYTE*)vih->dwSequenceHeader)[i+0]<<8) | ((BYTE*)vih->dwSequenceHeader)[i+1];
				pBitStream->ByteWrite(&((BYTE*)vih->dwSequenceHeader)[i+2], size);
				i += size;
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_UTF8)
		{
			pBitStream->ByteWrite(utf8bom, sizeof(utf8bom));
		}
		else if(mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2)
		{
			SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();
			BYTE* p = (BYTE*)si + si->dwOffset;

			if(memcmp(utf8bom, p, 3) != 0) 
				pBitStream->ByteWrite(utf8bom, sizeof(utf8bom));;

			CStringA str((char*)p, mt.FormatLength() - (p - mt.Format()));
			pBitStream->StrWrite(str + '\n', true);

			if(str.Find("[Events]") < 0) 
				pBitStream->StrWrite("\n\n[Events]\n", true);
		}
		else if(mt.majortype == MEDIATYPE_Audio 
			&& (mt.subtype == MEDIASUBTYPE_PCM 
			|| mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE) 
			|| mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
			&& mt.formattype == FORMAT_WaveFormatEx)
		{
			pBitStream->BitWrite('RIFF', 32);
			pBitStream->BitWrite(0, 32); // file length - 8, set later
			pBitStream->BitWrite('WAVE', 32);

			pBitStream->BitWrite('fmt ', 32);
			pBitStream->ByteWrite(&mt.cbFormat, 4);
			pBitStream->ByteWrite(mt.pbFormat, mt.cbFormat);

			pBitStream->BitWrite('data', 32);
			pBitStream->BitWrite(0, 32); // data length, set later
		}
	}
}

void CBaseMuxerFilter::MuxPacketInternal(const MuxerPacket* pPacket)
{
	TRACE(_T("MuxPacket pPin=%x, size=%d, s%d e%d b%d, rt=(%I64d-%I64d)\n"), 
		pPacket->pPin->GetID(),
		pPacket->pData.GetSize(),
		!!(pPacket->flags & MuxerPacket::syncpoint),
		!!(pPacket->flags & MuxerPacket::eos), 
		!!(pPacket->flags & MuxerPacket::bogus), 
		pPacket->rtStart/10000, pPacket->rtStop/10000);

	if(CComQIPtr<IBitStream> pBitStream = m_pOutput->GetBitStream())
		MuxPacket(pBitStream, pPacket);

	MuxPacket(pPacket);

	//

	do
	{
		CBaseMuxerInputPin* pInput = pPacket->pPin;
		if(!pInput) break;

		CBaseMuxerOutputPin* pOutput = dynamic_cast<CBaseMuxerOutputPin*>(pInput->GetRelatedPin());
		if(!pOutput) break;

		CComQIPtr<IBitStream> pBitStream = pOutput->GetBitStream();
		if(!pBitStream) break;

		const CMediaType& mt = pInput->CurrentMediaType();

		//

		const BYTE* pData = pPacket->pData.GetData();
		const int DataSize = pPacket->pData.GetSize();

		if(mt.subtype == MEDIASUBTYPE_AAC && mt.formattype == FORMAT_WaveFormatEx)
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

			int profile = 0;

			int srate_idx = 11;
			if(92017 <= wfe->nSamplesPerSec) srate_idx = 0;
			else if(75132 <= wfe->nSamplesPerSec) srate_idx = 1;
			else if(55426 <= wfe->nSamplesPerSec) srate_idx = 2;
			else if(46009 <= wfe->nSamplesPerSec) srate_idx = 3;
			else if(37566 <= wfe->nSamplesPerSec) srate_idx = 4;
			else if(27713 <= wfe->nSamplesPerSec) srate_idx = 5;
			else if(23004 <= wfe->nSamplesPerSec) srate_idx = 6;
			else if(18783 <= wfe->nSamplesPerSec) srate_idx = 7;
			else if(13856 <= wfe->nSamplesPerSec) srate_idx = 8;
			else if(11502 <= wfe->nSamplesPerSec) srate_idx = 9;
			else if(9391 <= wfe->nSamplesPerSec) srate_idx = 10;

			int channels = wfe->nChannels;

			if(wfe->cbSize >= 2)
			{
				BYTE* p = (BYTE*)(wfe+1);
				profile = (p[0]>>3)-1;
				srate_idx = ((p[0]&7)<<1)|((p[1]&0x80)>>7);
				channels = (p[1]>>3)&15;
			}

			int len = (DataSize + 7) & 0x1fff;

			BYTE hdr[7] = {0xff, 0xf9};
			hdr[2] = (profile<<6) | (srate_idx<<2) | ((channels&4)>>2);
			hdr[3] = ((channels&3)<<6) | (len>>11);
			hdr[4] = (len>>3)&0xff;
			hdr[5] = ((len&7)<<5) | 0x1f;
			hdr[6] = 0xfc;

			pBitStream->ByteWrite(hdr, sizeof(hdr));
		}
		else if((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO)
		{
			const BYTE* p = pData;
			int i = DataSize;

			while(i >= 4)
			{
				DWORD len = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];

				i -= len + 4;
				p += len + 4;
			}

			if(i == 0)
			{
				p = pData;
				i = DataSize;

				while(i >= 4)
				{
					DWORD len = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];

					pBitStream->BitWrite(0x00000001, 32);

					p += 4; 
					i -= 4;

					if(len > i || len == 1) {len = i; ASSERT(0);}

					pBitStream->ByteWrite(p, len);

					p += len;
					i -= len;
				}

				break;
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_UTF8 || mt.majortype == MEDIATYPE_Text)
		{
			CStringA str((char*)pData, DataSize);
			str.Trim();
			if(str.IsEmpty()) break;

			DVD_HMSF_TIMECODE start = RT2HMSF(pPacket->rtStart, 25);
			DVD_HMSF_TIMECODE stop = RT2HMSF(pPacket->rtStop, 25);

			str.Format("%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n", 
				pPacket->index+1,
				start.bHours, start.bMinutes, start.bSeconds, (int)((pPacket->rtStart/10000)%1000), 
				stop.bHours, stop.bMinutes, stop.bSeconds, (int)((pPacket->rtStop/10000)%1000),
				CStringA(str));

			pBitStream->StrWrite(str, true);

			break;
		}
		else if(mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2)
		{
			CStringA str((char*)pData, DataSize);
			str.Trim();
			if(str.IsEmpty()) break;

			DVD_HMSF_TIMECODE start = RT2HMSF(pPacket->rtStart, 25);
			DVD_HMSF_TIMECODE stop = RT2HMSF(pPacket->rtStop, 25);

			int fields = mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

			CList<CStringA> sl;
			Explode(str, sl, ',', fields);
			if(sl.GetCount() < fields) break;

			CStringA readorder = sl.RemoveHead(); // TODO
			CStringA layer = sl.RemoveHead();
			CStringA style = sl.RemoveHead();
			CStringA actor = sl.RemoveHead();
			CStringA left = sl.RemoveHead();
			CStringA right = sl.RemoveHead();
			CStringA top = sl.RemoveHead();
			if(fields == 10) top += ',' + sl.RemoveHead(); // bottom
			CStringA effect = sl.RemoveHead();
			str = sl.RemoveHead();

			if(mt.subtype == MEDIASUBTYPE_SSA) layer = "Marked=0";

			str.Format("Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%s,%s,%s,%s,%s\n",
				layer,
				start.bHours, start.bMinutes, start.bSeconds, (int)((pPacket->rtStart/100000)%100), 
				stop.bHours, stop.bMinutes, stop.bSeconds, (int)((pPacket->rtStop/100000)%100),
				style, actor, left, right, top, effect, 
				CStringA(str));

			pBitStream->StrWrite(str, true);

			break;
		}
		// else // TODO: restore more streams (vorbis to ogg, vobsub to idx/sub)

		pBitStream->ByteWrite(pData, DataSize);
	}
	while(0);
}

void CBaseMuxerFilter::MuxFooterInternal()
{
	TRACE(_T("MuxFooter\n"));
				
	if(CComQIPtr<IBitStream> pBitStream = m_pOutput->GetBitStream())
		MuxFooter(pBitStream);

	MuxFooter();

	//

	POSITION pos = m_pPins.GetHeadPosition();
	while(pos)
	{
		CBaseMuxerInputPin* pInput = m_pPins.GetNext(pos);
		
		CBaseMuxerOutputPin* pOutput = dynamic_cast<CBaseMuxerOutputPin*>(pInput->GetRelatedPin());
		if(!pOutput) continue;

		CComQIPtr<IBitStream> pBitStream = pOutput->GetBitStream();
		if(!pBitStream) continue;

		const CMediaType& mt = pInput->CurrentMediaType();

		if(mt.majortype == MEDIATYPE_Audio 
			&& (mt.subtype == MEDIASUBTYPE_PCM 
			|| mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE) 
			|| mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
			&& mt.formattype == FORMAT_WaveFormatEx)
		{
			pBitStream->BitFlush();

			ASSERT(pBitStream->GetPos() <= 0xffffffff);
			UINT32 size = (UINT32)pBitStream->GetPos();

			size -= 8;
			pBitStream->Seek(4);
			pBitStream->ByteWrite(&size, 4);

			size -= sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength();
			pBitStream->Seek(sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength() + 4);
			pBitStream->ByteWrite(&size, 4);
		}
	}
}

CAutoPtr<MuxerPacket> CBaseMuxerFilter::GetPacket()
{
	REFERENCE_TIME rtMin = _I64_MAX;
	CBaseMuxerInputPin* pPinMin = NULL;
	int i = m_pActivePins.GetCount();

	POSITION pos = m_pActivePins.GetHeadPosition();
	while(pos)
	{
		CBaseMuxerInputPin* pPin = m_pActivePins.GetNext(pos);

		CAutoLock cAutoLock(&pPin->m_csQueue);
		if(!pPin->m_queue.GetCount()) continue;

		MuxerPacket* p = pPin->m_queue.GetHead();

		if(p->IsBogus() || !p->IsTimeValid() || p->IsEOS())
		{
			pPinMin = pPin;
			i = 0;
			break;
		}

		if(p->rtStart < rtMin)
		{
			rtMin = p->rtStart;
			pPinMin = pPin;
		}

		i--;
	}

	CAutoPtr<MuxerPacket> pPacket;

	if(pPinMin && i == 0)
	{
		pPacket = pPinMin->PopPacket();
	}
	else
	{
		pos = m_pActivePins.GetHeadPosition();
		while(pos) m_pActivePins.GetNext(pos)->m_evAcceptPacket.Set();
	}

	return pPacket;
}

//

int CBaseMuxerFilter::GetPinCount()
{
	return m_pInputs.GetCount() + (m_pOutput ? 1 : 0) + m_pOutputs.GetCount();
}

CBasePin* CBaseMuxerFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n >= 0 && n < (int)m_pInputs.GetCount())
	{
		if(POSITION pos = m_pInputs.FindIndex(n))
			return m_pInputs.GetAt(pos);
	}

	n -= m_pInputs.GetCount();

	if(n == 0 && m_pOutput)
	{
		return m_pOutput;
	}

	n--;

	if(n >= 0 && n < (int)m_pOutputs.GetCount())
	{
		if(POSITION pos = m_pOutputs.FindIndex(n))
			return m_pOutputs.GetAt(pos);
	}

	n -= m_pOutputs.GetCount();

	return NULL;
}

STDMETHODIMP CBaseMuxerFilter::Stop()
{
	CAutoLock cAutoLock(this);

	HRESULT hr = __super::Stop();
	if(FAILED(hr)) return hr;

	CallWorker(CMD_EXIT);

	return hr;
}

STDMETHODIMP CBaseMuxerFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr = __super::Pause();
	if(FAILED(hr)) return hr;

	if(fs == State_Stopped && m_pOutput)
	{
		CAMThread::Create();
		CallWorker(CMD_RUN);
	}

	return hr;
}

STDMETHODIMP CBaseMuxerFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr = __super::Run(tStart);
	if(FAILED(hr)) return hr;

	return hr;
}

// IMediaSeeking

STDMETHODIMP CBaseMuxerFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = AM_SEEKING_CanGetDuration|AM_SEEKING_CanGetCurrentPos, S_OK : E_POINTER;
}
STDMETHODIMP CBaseMuxerFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if(*pCapabilities == 0) return S_OK;
	DWORD caps;
	GetCapabilities(&caps);
	caps &= *pCapabilities;
	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}
STDMETHODIMP CBaseMuxerFilter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CBaseMuxerFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CBaseMuxerFilter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CBaseMuxerFilter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CBaseMuxerFilter::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
STDMETHODIMP CBaseMuxerFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	*pDuration = 0;
	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos) {REFERENCE_TIME rt = m_pInputs.GetNext(pos)->GetDuration(); if(rt > *pDuration) *pDuration = rt;}
	return S_OK;
}
STDMETHODIMP CBaseMuxerFilter::GetStopPosition(LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	CheckPointer(pCurrent, E_POINTER);
	*pCurrent = m_rtCurrent;
	return S_OK;
}
STDMETHODIMP CBaseMuxerFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	FILTER_STATE fs;

	if(SUCCEEDED(GetState(0, &fs)) && fs == State_Stopped)
	{
		POSITION pos = m_pInputs.GetHeadPosition();
		while(pos)
		{
			CBasePin* pPin = m_pInputs.GetNext(pos);
			CComQIPtr<IMediaSeeking> pMS = pPin->GetConnected();
			if(!pMS) pMS = GetFilterFromPin(pPin->GetConnected());
			if(pMS) pMS->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
		}

		return S_OK;
	}

	return VFW_E_WRONG_STATE;
}
STDMETHODIMP CBaseMuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::SetRate(double dRate) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetRate(double* pdRate) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetPreroll(LONGLONG* pllPreroll) {return E_NOTIMPL;}
