#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include "MatroskaFile.h"

class CMatroskaSplitterInputPin : public CBasePin
{
	CComQIPtr<IAsyncReader> m_pAsyncReader;
	CAutoPtr<Matroska::CMatroskaFile> m_pFile;

public:
	CMatroskaSplitterInputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaSplitterInputPin();

	HRESULT GetAsyncReader(IAsyncReader** ppAsyncReader);
	Matroska::CMatroskaFile* GetFile() {return m_pFile;}

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
/*
    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin* pPin);
*/
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

[uuid("62BEFB74-07A4-4434-AA41-8F926F5A961B")]
class CMatroskaSplitterFilter 
	: public CBaseFilter
	, public CCritSec
	, protected CAMThread
	, public IMediaSeeking
{
	CMatroskaSplitterInputPin* m_pInput;
	CAutoPtrList<CMatroskaSplitterOutputPin> m_pOutputs;
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
	CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaSplitterFilter();

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

