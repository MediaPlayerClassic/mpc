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

#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include "MatroskaFile.h"

#define MAXCLUSTERTIME 1000
#define MAXBLOCKS 50

class CMatroskaMuxerInputPin : public CBaseInputPin
{
	CAutoPtr<MatroskaWriter::TrackEntry> m_pTE;
	CAutoPtrArray<MatroskaWriter::CBinary> m_pVorbisHdrs;

	bool m_fActive;
	CCritSec m_csReceive;

	REFERENCE_TIME m_rtLastStart, m_rtLastStop;

public:
	CMatroskaMuxerInputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaMuxerInputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	MatroskaWriter::TrackEntry* GetTrackEntry() {return m_pTE;}

	REFERENCE_TIME m_rtDur;

	CCritSec m_csQueue;
	CAutoPtrList<MatroskaWriter::BlockGroup> m_blocks;
	bool m_fEndOfStreamReceived;

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin* pPin);
	HRESULT Active(), Inactive();

    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();

	STDMETHODIMP Receive(IMediaSample* pSample);
    STDMETHODIMP EndOfStream();
};

class CMatroskaMuxerOutputPin : public CBaseOutputPin
{
public:
	CMatroskaMuxerOutputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaMuxerOutputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};

[uuid("1E1299A2-9D42-4F12-8791-D79E376F4143")]
class CMatroskaMuxerFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IAMFilterMiscFlags
	, public IMediaSeeking
{
protected:
	CAutoPtrList<CMatroskaMuxerInputPin> m_pInputs;
	CAutoPtr<CMatroskaMuxerOutputPin> m_pOutput;

	REFERENCE_TIME m_rtCurrent;

	enum {CMD_EXIT, CMD_RUN};
	DWORD ThreadProc();

public:
	CMatroskaMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaMuxerFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	void AddInput();
	UINT GetTrackNumber(CBasePin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IAMFilterMiscFlags

	STDMETHODIMP_(ULONG) GetMiscFlags();

	// IMediaSeeking

	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);
};

