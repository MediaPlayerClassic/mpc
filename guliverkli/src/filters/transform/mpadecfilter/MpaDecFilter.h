/* 
 *	Copyright (C) 2003-2004 Gabest
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

#pragma once

#include <atlcoll.h>
#include <afxtempl.h>
#include "libmad-0.15.0b\msvc++\mad.h"
#include "a52dec-0.7.4\vc++\inttypes.h"
#include "a52dec-0.7.4\include\a52.h"
#include "..\..\..\decss\DeCSSInputPin.h"
#include "IMpaDecFilter.h"

[uuid("3D446B6F-71DE-4437-BE15-8CE47174340F")]
class CMpaDecFilter : public CTransformFilter, public IMpaDecFilter
{
protected:
	CCritSec m_csReceive;

	a52_state_t* m_a52_state;

	struct mad_stream m_stream;
	struct mad_frame m_frame;
	struct mad_synth m_synth;

	CArray<BYTE> m_buff;
	REFERENCE_TIME m_rtStart;

	HRESULT ProcessLPCM(bool fPreroll, bool fDiscontinuity);
	HRESULT ProcessAC3(bool fPreroll, bool fDiscontinuity);
	HRESULT ProcessDTS(bool fPreroll, bool fDiscontinuity);
	HRESULT ProcessMPA(bool fPreroll, bool fDiscontinuity);

	HRESULT GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData);
	HRESULT Deliver(CArray<float>& pBuff, bool fPreroll, bool fDiscontinuity, 
		DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask = 0);
	HRESULT Deliver(CArray<BYTE>& pBuff, bool fPreroll, bool fDiscontinuity, 
		DWORD nSamplesPerSec, REFERENCE_TIME rtDur);
	HRESULT ReconnectOutput(int nSamples, CMediaType& mt);

protected:
	CCritSec m_csProps;
	SampleFormat m_iSampleFormat;
	int m_iSpeakerConfig;
	bool m_fDynamicRangeControl;

public:
	CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMpaDecFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT Receive(IMediaSample* pIn);

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

	// IMpaDecFilter

	STDMETHODIMP SetSampleFormat(SampleFormat sf);
	STDMETHODIMP_(SampleFormat) GetSampleFormat();
	STDMETHODIMP SetSpeakerConfig(int sc);
	STDMETHODIMP_(int) GetSpeakerConfig();
	STDMETHODIMP SetDynamicRangeControl(bool fDRC);
	STDMETHODIMP_(bool) GetDynamicRangeControl();
};

class CMpaDecInputPin : public CDeCSSInputPin
{
public:
    CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);
};
