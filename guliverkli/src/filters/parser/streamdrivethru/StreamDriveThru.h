#pragma once

#include <atlbase.h>
#include "..\..\..\DSUtil\DSUtil.h"


class CStreamDriveThruInputPin : public CBasePin
{
	CComQIPtr<IAsyncReader> m_pAsyncReader;

public:
	CStreamDriveThruInputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CStreamDriveThruInputPin();

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

class CStreamDriveThruOutputPin : public CBaseOutputPin
{
	CComQIPtr<IStream> m_pStream;

public:
	CStreamDriveThruOutputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CStreamDriveThruOutputPin();

	HRESULT GetStream(IStream** ppStream);

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin* pPin);

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};

[uuid("534FE6FD-F1F0-4aec-9F45-FF397320CE33")]
class CStreamDriveThruFilter : public CBaseFilter, protected CAMThread, public IMediaSeeking
{
	CCritSec m_csLock;

	CStreamDriveThruInputPin* m_pInput;
	CStreamDriveThruOutputPin* m_pOutput;

protected:
	enum {CMD_EXIT, CMD_STOP, CMD_PAUSE, CMD_RUN};
    DWORD ThreadProc();

	LONGLONG m_position;

public:
	CStreamDriveThruFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CStreamDriveThruFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

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

