/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  MpaDecFilter.ax is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  MpaDecFilter.ax is distributed in the hope that it will be useful,
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
#include <mmreg.h>
#include "MpaDecFilter.h"

#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Payload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
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
	{&__uuidof(CMpaDecFilter), L"MPEG Audio Decoder (MAD)", 0x03680003, sizeof(sudpPins)/sizeof(sudpPins[0]), sudpPins},
};

CFactoryTemplate g_Templates[] =
{
    {L"MPEG Audio Decoder (MAD)", &__uuidof(CMpaDecFilter), CMpaDecFilter::CreateInstance, NULL, &sudFilter[0]},
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

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CMpaDecFilter
//

CUnknown* WINAPI CMpaDecFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMpaDecFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CMpaDecFilter::CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CMpaDecFilter"), lpunk, __uuidof(this))
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CMpaDecInputPin(this, phr, L"In"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CTransformOutputPin(NAME("CTransformOutputPin"), this, phr, L"Out"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}
}

CMpaDecFilter::~CMpaDecFilter()
{
}

STDMETHODIMP CMpaDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
//		QI()
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMpaDecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);
	return __super::EndOfStream();
}

HRESULT CMpaDecFilter::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMpaDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	return __super::EndFlush();
}

HRESULT CMpaDecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	return __super::NewSegment(tStart, tStop, dRate);
}

static inline int scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if(sample >= MAD_F_ONE) sample = MAD_F_ONE - 1;
	else if(sample < -MAD_F_ONE) sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

HRESULT CMpaDecFilter::Receive(IMediaSample* pIn)
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
		pmt = NULL;
	}

	const GUID& subtype = m_pInput->CurrentMediaType().subtype;

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

	if((*(DWORD*)pDataIn&0xE0FFFFFF) == 0xC0010000 || *(DWORD*)pDataIn == 0xBD010000)
	{
		if(subtype == MEDIASUBTYPE_MPEG1Packet)
		{
			len -= 4+2+7; pDataIn += 4+2+7; // is it always ..+7 ?
		}
		else if(subtype == MEDIASUBTYPE_MPEG2_AUDIO)
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1; pDataIn += *pDataIn+1;
		}
		else if(subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1; pDataIn += *pDataIn+1;
			len -= 7; pDataIn += 7;
		}
	}

	if(len <= 0) return S_OK;

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);

	if(pIn->IsDiscontinuity() == S_OK)
	{
		m_buff.RemoveAll();
		ASSERT(SUCCEEDED(hr)); // what to do if not?
		if(FAILED(hr)) return S_OK; // lets wait then...
		m_rtStart = rtStart;
	}

	if(subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		if(SUCCEEDED(hr))
			m_rtStart = rtStart;

		CComPtr<IMediaSample> pOut;
		BYTE* pDataOut = NULL;
		if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))
		|| FAILED(hr = pOut->GetPointer(&pDataOut)))
			return hr;

		if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
		{
			CMediaType mt = *pmt;
			m_pOutput->SetMediaType(&mt);
			DeleteMediaType(pmt);
			pmt = NULL;
		}

		WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
		WAVEFORMATEX* wfeout = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

		REFERENCE_TIME rtDur = 10000000i64*len/wfein->nAvgBytesPerSec;
		REFERENCE_TIME rtStart = m_rtStart;
		REFERENCE_TIME rtStop = m_rtStart + rtDur;
		pOut->SetTime(&rtStart, &rtStop);
		pOut->SetMediaTime(NULL, NULL);
		m_rtStart += rtDur;

		pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
		pOut->SetSyncPoint(TRUE);

		long lenout = len*wfeout->wBitsPerSample/wfein->wBitsPerSample;
		ASSERT(lenout*wfein->wBitsPerSample == len*wfeout->wBitsPerSample);
		pOut->SetActualDataLength(lenout);

		// TODO: when needed convert wfein->wBitsPerSample (20/24bps) to wfeout->wBitsPerSample (16bps)

		for(int i = 0; i < len; i+=2, pDataIn+=2, pDataOut+=2)
		{
			pDataOut[0] = pDataIn[1];
			pDataOut[1] = pDataIn[0];
		}

		if(FAILED(hr = m_pOutput->Deliver(pOut)))
			return hr;
	}
	else
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();

		if(SUCCEEDED(hr))
		{
			if(abs(m_rtStart - rtStart) > 1000000)
			{
				m_buff.RemoveAll();
				m_rtStart = rtStart;
			}
		}

		int tmp = m_buff.GetSize();
		m_buff.SetSize(m_buff.GetSize() + len);
		memcpy(m_buff.GetData() + tmp, pDataIn, len);
		len += tmp;

		mad_stream_buffer(&m_stream, m_buff.GetData(), m_buff.GetSize());

		while(1)
		{
			if(mad_frame_decode(&m_frame, &m_stream) == -1)
			{
				if(m_stream.error == MAD_ERROR_BUFLEN)
				{
					m_buff.SetSize(m_stream.bufend - m_stream.this_frame);
					memcpy(m_buff.GetData(), m_stream.this_frame, m_buff.GetSize());
					break;
				}

				TRACE(_T("*m_stream.error == %d\n"), m_stream.error);

				if(MAD_RECOVERABLE(m_stream.error))
				{
					continue;
				}

				return E_FAIL;
			}

			mad_synth_frame(&m_synth, &m_frame);

			unsigned int nSamplesPerSec = m_synth.pcm.samplerate;
			unsigned int nChannels = m_synth.pcm.channels;
			unsigned int nSamples  = m_synth.pcm.length;
			const mad_fixed_t* left_ch   = m_synth.pcm.samples[0];
			const mad_fixed_t* right_ch  = m_synth.pcm.samples[1];

			// ASSERT(wfe->nChannels == nChannels);
			// ASSERT(wfe->nSamplesPerSec == nSamplesPerSec);

			if(wfe->nChannels != nChannels || wfe->nSamplesPerSec != nSamplesPerSec)
			{
				TRACE(_T("wfe->nChannels (%d) != nChannels (%d) || wfe->nSamplesPerSec (%d) != nSamplesPerSec (%d)\n"), 
					wfe->nChannels, nChannels, wfe->nSamplesPerSec, nSamplesPerSec);
				continue;
			}

			CComPtr<IMediaSample> pOut;
			BYTE* pDataOut = NULL;
			if(S_OK != (hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))
			|| S_OK != (hr = pOut->GetPointer(&pDataOut)))
				return hr;

			if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
			{
				CMediaType mt = *pmt;
				m_pOutput->SetMediaType(&mt);
				DeleteMediaType(pmt);
				pmt = NULL;
			}

			REFERENCE_TIME rtDur = 10000000i64*m_frame.header.duration.seconds 
						+ 10000000i64*m_frame.header.duration.fraction/MAD_TIMER_RESOLUTION;
			REFERENCE_TIME rtStart = m_rtStart;
			REFERENCE_TIME rtStop = m_rtStart + rtDur;

			m_rtStart += rtDur;

			TRACE(_T("mpadec: %I64d - %I64d\n"), rtStart/10000, rtStop/10000);

			// The reason of this 200ms limit is that the the splitters (both avi splitters for example)
			// seek (1+nPins) times simply because that's the amount they receive the SetPosition call usually.
			// If the filter let the samples through for the first couple of 100ms then it would do that 
			// (1+nPins) times in a row and would make horrible noises.

			if(rtStart < 2000000)
				continue;
/*
			if(rtStop < 0)
				continue;

			if(rtStart < 0)
			{
				int nSamplesToSkip = -rtStart*nSamplesPerSec/10000000;
				nSamples -= nSamplesToSkip;
				left_ch += nSamplesToSkip;
				right_ch += nSamplesToSkip;
				rtStart = 0;
			}
*/
			pOut->SetTime(&rtStart, &rtStop);
			pOut->SetMediaTime(NULL, NULL);

			pOut->SetDiscontinuity(FALSE);
			pOut->SetSyncPoint(TRUE);

			pOut->SetActualDataLength(nChannels*nSamples*2);

			while(nSamples--)
			{
				*(short*)pDataOut = scale(*left_ch++); 
				pDataOut += 2;

				if(nChannels == 2)
				{
					*(short*)pDataOut = scale(*right_ch++);
					pDataOut += 2;
				}
			}

			if(S_OK != (hr = m_pOutput->Deliver(pOut)))
				return hr;
		}
	}

	return S_OK;
}

HRESULT CMpaDecFilter::CheckInputType(const CMediaType* mtIn)
{
	// TODO: remove this limitation
	if(mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();
		if(wfe->nChannels != 2 || wfe->wBitsPerSample != 16)
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return (mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MP3
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG1AudioPayload
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG1Payload
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG1Packet
			|| mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CMediaType& mt = m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	pProperties->cBuffers = 8;
	pProperties->cbBuffer = wfe->nChannels*wfe->nSamplesPerSec*4; // TODO
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

HRESULT CMpaDecFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CMediaType& mt = m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfein = (WAVEFORMATEX*)mt.Format();
/*
MPEG1WAVEFORMAT* mpeg1wfe = (MPEG1WAVEFORMAT*)wfein;
MPEGLAYER3WAVEFORMAT* mp3wfe = (MPEGLAYER3WAVEFORMAT*)wfein;
*/
	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_PCM;
	pmt->formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfe, 0, pmt->FormatLength());
	wfe->cbSize = 0;
	wfe->wFormatTag = WAVE_FORMAT_PCM;
	wfe->nChannels = wfein->nChannels;
	wfe->wBitsPerSample = 16;
	wfe->nBlockAlign = wfe->nChannels*wfe->wBitsPerSample/8;
	wfe->nSamplesPerSec = wfein->nSamplesPerSec;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec*wfe->nBlockAlign;

	return S_OK;
}

HRESULT CMpaDecFilter::StartStreaming()
{
	HRESULT hr = __super::StartStreaming();
	if(FAILED(hr)) return hr;

	mad_stream_init(&m_stream);
	mad_frame_init(&m_frame);
	mad_synth_init(&m_synth);

	mad_stream_options(&m_stream, 0/*options*/);

	m_fSyncLost = false;

	return S_OK;
}

HRESULT CMpaDecFilter::StopStreaming()
{
	mad_synth_finish(&m_synth);
	mad_frame_finish(&m_frame);
	mad_stream_finish(&m_stream);

	return __super::StopStreaming();
}

//
// CMpaDecInputPin
//

CMpaDecInputPin::CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMpaDecInputPin"), pFilter, phr, pName)
{
}
