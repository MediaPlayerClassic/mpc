#pragma once

#include <atlbase.h>

[uuid("63EF0035-3FFE-4c41-9230-4346E028BE20")]
interface IBufferFilter : public IUnknown
{
	STDMETHOD(SetBuffers) (int nBuffers) = 0;
	STDMETHOD_(int, GetBuffers) () = 0;
	STDMETHOD_(int, GetFreeBuffers) () = 0;
	STDMETHOD(SetPriority) (DWORD dwPriority = THREAD_PRIORITY_NORMAL) = 0;
};

[uuid("DA2B3D77-2F29-4fd2-AC99-DEE4A8A13BF0")]
class CBufferFilter : public CTransformFilter, public IBufferFilter
{
	int m_nSamplesToBuffer;

public:
	CBufferFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CBufferFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IBufferFilter
	STDMETHODIMP SetBuffers(int nBuffers);
	STDMETHODIMP_(int) GetBuffers();
	STDMETHODIMP_(int) GetFreeBuffers();
	STDMETHODIMP SetPriority(DWORD dwPriority = THREAD_PRIORITY_NORMAL);

	HRESULT Receive(IMediaSample* pSample);
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT StopStreaming();
};

class CBufferFilterOutputPin : public CTransformOutputPin
{
	class CBufferFilterOutputQueue : public COutputQueue
	{
	public:
		CBufferFilterOutputQueue(IPin* pInputPin, HRESULT* phr,
				DWORD dwPriority = THREAD_PRIORITY_NORMAL,
				BOOL bAuto = FALSE, BOOL bQueue = TRUE,
				LONG lBatchSize = 1, BOOL bBatchExact = FALSE,
				LONG lListSize = DEFAULTCACHE,
				bool bFlushingOpt = false)
			: COutputQueue(pInputPin, phr, bAuto, bQueue, lBatchSize, bBatchExact, lListSize, dwPriority, bFlushingOpt)
		{
		}

		int GetQueueCount()
		{
			return m_List ? m_List->GetCount() : -1;
		}

		bool SetPriority(DWORD dwPriority)
		{
			return m_hThread ? !!::SetThreadPriority(m_hThread, dwPriority) : false;
		}
	};

public:
	CBufferFilterOutputPin(CTransformFilter* pFilter, HRESULT* phr);

	CAutoPtr<CBufferFilterOutputQueue> m_pOutputQueue;

	HRESULT Active();
    HRESULT Inactive();

	HRESULT Deliver(IMediaSample* pMediaSample);
    HRESULT DeliverEndOfStream();
    HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};
