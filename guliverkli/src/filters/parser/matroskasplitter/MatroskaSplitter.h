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

class CMatroskaSplitterInputPin : public CBasePin
{
	CComQIPtr<IAsyncReader> m_pAsyncReader;

public:
	CMatroskaSplitterInputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaSplitterInputPin();

	HRESULT GetAsyncReader(IAsyncReader** ppAsyncReader);

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT CheckMediaType(const CMediaType* pmt);

    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pPin);

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
};

class CMatroskaSplitterOutputPin : public CBaseOutputPin
{
	CMediaType m_mt;
	CAutoPtr<COutputQueue> m_pOutputQueue;

public:
	CMatroskaSplitterOutputPin(CMediaType& mt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaSplitterOutputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

	// Queueing

	HRESULT Active();
    HRESULT Inactive();

	HRESULT Deliver(IMediaSample* pMediaSample);
    HRESULT DeliverEndOfStream();
    HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

[uuid("0A68C3B5-9164-4a54-AFAF-995B2FF0E0D4")]
class CMatroskaSourceFilter 
	: public CBaseFilter
	, public CCritSec
	, protected CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
{
	class CFileReader : public CUnknown, public IAsyncReader
	{
		CFile m_file;

	public:
		CFileReader(CString fn, HRESULT& hr);

		DECLARE_IUNKNOWN;
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		// IAsyncReader

		STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) {return E_NOTIMPL;}
        STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser) {return E_NOTIMPL;}
        STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) {return E_NOTIMPL;}
		STDMETHODIMP SyncReadAligned(IMediaSample* pSample) {return E_NOTIMPL;}
		STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
		STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
		STDMETHODIMP BeginFlush() {return E_NOTIMPL;}
		STDMETHODIMP EndFlush() {return E_NOTIMPL;}
	};

	CStringW m_fn;

protected:
	CAutoPtr<CMatroskaSplitterInputPin> m_pInput;
	CAutoPtrList<CMatroskaSplitterOutputPin> m_pOutputs;

	CAutoPtr<Matroska::CMatroskaFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	CMap<UINT64, UINT64, CMatroskaSplitterOutputPin*, CMatroskaSplitterOutputPin*> m_mapTrackToPin;
	CMap<UINT64, UINT64, Matroska::TrackEntry*, Matroska::TrackEntry*> m_mapTrackToTrackEntry;

	CCritSec m_csSend;

	bool m_fSeeking;
	REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent;
	double m_dRate;

	void SendVorbisHeaderSample();
	void SendFakeTextSample();

	CList<UINT64> m_bDiscontinuitySent;
	CList<CBaseOutputPin*> m_pActivePins;

	HRESULT DeliverBlock(Matroska::Block* pBlock);

protected:
	enum {CMD_EXIT, CMD_RUN, CMD_SEEK};
    DWORD ThreadProc();

public:
	CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaSourceFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT BreakConnect(PIN_DIRECTION dir, CBasePin* pPin);
	HRESULT CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

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

[uuid("149D2E01-C32E-4939-80F6-C07B81015A7A")]
class CMatroskaSplitterFilter : public CMatroskaSourceFilter
{
public:
	CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};
