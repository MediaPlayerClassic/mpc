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

#pragma once

#include <atlcoll.h>
#include <afxtempl.h>
#include "libmad-0.15.0b/msvc++/mad.h"

[uuid("3D446B6F-71DE-4437-BE15-8CE47174340F")]
class CMpaDecFilter : public CTransformFilter
{
	CCritSec m_csReceive;

	struct mad_stream m_stream;
	struct mad_frame m_frame;
	struct mad_synth m_synth;

	CArray<BYTE> m_buff;
	REFERENCE_TIME m_rtStart;

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
};

class CMpaDecInputPin : public CTransformInputPin, public IKsPropertySet
{
	int m_varient;
	BYTE m_Challenge[10], m_KeyCheck[5], m_Key[10];
	BYTE m_DiscKey[6], m_TitleKey[6];

public:
    CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IMemInputPin
    STDMETHODIMP Receive(IMediaSample* pSample);

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};
