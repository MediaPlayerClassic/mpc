#pragma once

class CBaseVideoFilter : public CTransformFilter
{
private:
    HRESULT Receive(IMediaSample* pIn);
	HRESULT ReconnectOutput(int w, int h);

	// these are private for a reason, don't bother them
	DWORD m_win, m_hin, m_arxin, m_aryin;
	DWORD m_wout, m_hout, m_arxout, m_aryout;

	long m_cBuffers;

protected:
	CCritSec m_csReceive;

	int m_w, m_h, m_arx, m_ary;

	HRESULT GetDeliveryBuffer(int w, int h, IMediaSample** ppOut);
	HRESULT CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype);
	HRESULT CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype);

	virtual void GetOutputSize(int& w, int& h, int& arx, int& ary) {}
	virtual HRESULT Transform(IMediaSample* pIn) = 0;

public:
	CBaseVideoFilter(TCHAR* pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long cBuffers = 1);
	virtual ~CBaseVideoFilter();

	int GetPinCount();
	CBasePin* GetPin(int n);

    HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckOutputType(const CMediaType& mtOut);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt);
};

class CBaseVideoInputAllocator : public CMemAllocator
{
	CMediaType m_mt;

public:
	CBaseVideoInputAllocator(HRESULT* phr);
	void SetMediaType(const CMediaType& mt);
	STDMETHODIMP GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
};

class CBaseVideoInputPin : public CTransformInputPin
{
	CBaseVideoInputAllocator* m_pAllocator;

public:
	CBaseVideoInputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	~CBaseVideoInputPin();

	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
};

class CBaseVideoOutputPin : public CTransformOutputPin
{
public:
	CBaseVideoOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);

    HRESULT CheckMediaType(const CMediaType* mtOut);
};
