/* 
 *	Copyright (C) 2003-2005 Gabest
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
	m_pOutput.Attach(new CBaseMuxerOutputPin(NAME("CBaseMuxerOutputPin"), this, this, phr));
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
	
	CBaseMuxerInputPin* pPin = NULL;
	if(FAILED(CreateInput(name, &pPin)) || !pPin) {ASSERT(0); return;}

	CAutoPtr<CBaseMuxerInputPin> pAutoPtrPin(pPin);
	m_pInputs.AddTail(pAutoPtrPin);
}

HRESULT CBaseMuxerFilter::CreateInput(CStringW name, CBaseMuxerInputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	HRESULT hr = S_OK;
	*ppPin = new CBaseMuxerInputPin(name, this, this, &hr);
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

			if(m_pOutput)
			if(CComQIPtr<IStream> pStream = m_pOutput->GetConnected())
			{
				CComPtr<IBitStream> pBitStream = new CBitStream(pStream);
				m_pBitStreams.AddTail(pBitStream);

				// TODO: figure out a way to smuggle socket based streams into m_pBitStreams
			}

			Reply(S_OK);

			MuxInit();

//			TRACE(_T("WriteHeader\n"));
			pos = m_pBitStreams.GetHeadPosition();
			while(pos) MuxHeader(m_pBitStreams.GetNext(pos));

			while(!CheckRequest(NULL) && m_pActivePins.GetCount())
			{
				if(m_State == State_Paused) {Sleep(10); continue;}

				CAutoPtr<MuxerPacket> pPacket = GetPacket();
				if(!pPacket) {Sleep(1); continue;}

				if(pPacket->IsTimeValid())
					m_rtCurrent = pPacket->rtStart;

				if(pPacket->IsEOS())
					m_pActivePins.RemoveAt(m_pActivePins.Find(pPacket->pPin));
/*
				TRACE(_T("WritePacket pPin=%x, size=%d, s%d e%d b%d, rt=(%I64d-%I64d)\n"), 
					pPacket->pPin->GetID(),
					pPacket->pData.GetSize(),
					!!(pPacket->flags & MuxerPacket::syncpoint),
					!!(pPacket->flags & MuxerPacket::eos), 
					!!(pPacket->flags & MuxerPacket::bogus), 
					pPacket->rtStart/10000, pPacket->rtStop/10000);
*/
				pos = m_pBitStreams.GetHeadPosition();
				while(pos) MuxPacket(m_pBitStreams.GetNext(pos), pPacket);
			}

//			TRACE(_T("WriteFooter\n"));
			pos = m_pBitStreams.GetHeadPosition();
			while(pos) MuxFooter(m_pBitStreams.GetNext(pos));

			m_pOutput->DeliverEndOfStream();

			m_pActivePins.RemoveAll();
			m_pPins.RemoveAll();

			m_pBitStreams.RemoveAll();

			break;
		}
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
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
	return m_pInputs.GetCount() + (m_pOutput ? 1 : 0);
}

CBasePin* CBaseMuxerFilter::GetPin(int n)
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
STDMETHODIMP CBaseMuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::SetRate(double dRate) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetRate(double* pdRate) {return E_NOTIMPL;}
STDMETHODIMP CBaseMuxerFilter::GetPreroll(LONGLONG* pllPreroll) {return E_NOTIMPL;}
