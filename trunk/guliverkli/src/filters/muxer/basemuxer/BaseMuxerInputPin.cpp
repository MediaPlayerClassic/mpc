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
#include <mmreg.h>
#include "BaseMuxer.h"
#include "..\..\..\DSUtil\DSUtil.h"
#include "..\..\..\..\include\moreuuids.h"

#include <initguid.h>
#include "..\..\..\..\include\ogg\OggDS.h"

//
// CBaseMuxerInputPin
//

CBaseMuxerInputPin::CBaseMuxerInputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseInputPin(NAME("CBaseMuxerInputPin"), pFilter, pLock, phr, pName)
	, m_rtDuration(0)
{
	static int s_iID = 0;
	m_iID = s_iID++;
}

CBaseMuxerInputPin::~CBaseMuxerInputPin()
{
}

STDMETHODIMP CBaseMuxerInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool CBaseMuxerInputPin::IsSubtitleStream()
{
	return m_mt.majortype == MEDIATYPE_Text || m_mt.majortype == MEDIATYPE_Subtitle; // TODO
}

HRESULT CBaseMuxerInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		|| pmt->majortype == MEDIATYPE_Audio && pmt->formattype != FORMAT_VorbisFormat
		|| pmt->majortype == MEDIATYPE_Text && pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_None
		|| pmt->majortype == MEDIATYPE_Subtitle
		? S_OK
		: E_INVALIDARG;
}

HRESULT CBaseMuxerInputPin::BreakConnect()
{
	HRESULT hr = __super::BreakConnect();
	if(FAILED(hr)) return hr;

	RemoveAll();

	// TODO: remove extra disconnected pins, leave one

	return hr;
}

HRESULT CBaseMuxerInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr = __super::CompleteConnect(pReceivePin);
	if(FAILED(hr)) return hr;

	// duration

	m_rtDuration = 0;
	CComQIPtr<IMediaSeeking> pMS;
	if((pMS = GetFilterFromPin(pReceivePin)) || (pMS = pReceivePin))
		pMS->GetDuration(&m_rtDuration);

	// properties

	for(CComPtr<IPin> pPin = pReceivePin; pPin; pPin = GetUpStreamPin(GetFilterFromPin(pPin)))
	{
		if(CComQIPtr<IDSMPropertyBag> pPB = pPin)
		{
			ULONG cProperties = 0;
			if(FAILED(pPB->CountProperties(&cProperties)) || cProperties == 0)
				continue;

			for(ULONG iProperty = 0; iProperty < cProperties; iProperty++)
			{
				PROPBAG2 PropBag;
				memset(&PropBag, 0, sizeof(PropBag));
				ULONG cPropertiesReturned = 0;
				if(FAILED(pPB->GetPropertyInfo(iProperty, 1, &PropBag, &cPropertiesReturned)))
					continue;

				HRESULT hr;
				CComVariant var;
				if(SUCCEEDED(pPB->Read(1, &PropBag, NULL, &var, &hr)) && SUCCEEDED(hr))
					SetProperty(PropBag.pstrName, &var);

				CoTaskMemFree(PropBag.pstrName);
			}
		}
	}

	((CBaseMuxerFilter*)m_pFilter)->AddInput();

	return S_OK;
}

HRESULT CBaseMuxerInputPin::Active()
{
	m_rtMaxStart = _I64_MIN;
	m_fEOS = false;
	return __super::Active();
}

STDMETHODIMP CBaseMuxerInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CBaseMuxerInputPin::Receive(IMediaSample* pSample)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr = __super::Receive(pSample);
	if(FAILED(hr)) return hr;

	if(m_mt.formattype == FORMAT_WaveFormatEx 
	&& (((WAVEFORMATEX*)m_mt.pbFormat)->wFormatTag == WAVE_FORMAT_PCM
	|| ((WAVEFORMATEX*)m_mt.pbFormat)->wFormatTag == WAVE_FORMAT_MPEGLAYER3))
		pSample->SetSyncPoint(TRUE); // HACK: some capture filters don't set this

	REFERENCE_TIME rtStart = _I64_MAX, rtStop;
	if(SUCCEEDED(pSample->GetTime(&rtStart, &rtStop)))
	{
		rtStart += m_tStart; 
		rtStop += m_tStart;
		pSample->SetTime(&rtStart, &rtStop);

		if(S_OK == pSample->IsSyncPoint() && rtStart < m_rtMaxStart)
			pSample->SetSyncPoint(FALSE);

		m_rtMaxStart = max(m_rtMaxStart,  rtStart);
	}
	else
	{
		pSample->SetSyncPoint(FALSE);
	}

	((CBaseMuxerFilter*)m_pFilter)->Receive(pSample, this);

	return S_OK;
}

STDMETHODIMP CBaseMuxerInputPin::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr = __super::EndOfStream();
	if(FAILED(hr)) return hr;

	((CBaseMuxerFilter*)m_pFilter)->Receive(NULL, this);

	m_fEOS = true;

	return hr;
}

