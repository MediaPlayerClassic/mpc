#pragma once

#include <atlbase.h>

class CWavDestOutputPin : public CTransformOutputPin
{
public:
    CWavDestOutputPin(CTransformFilter* pFilter, HRESULT* phr);

    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes** ppEnum);
    HRESULT CheckMediaType(const CMediaType* pmt);
};

[uuid("8685214E-4D32-4058-BE04-D01104F00B0C")]
class CWavDestFilter : public CTransformFilter
{
public:
    CWavDestFilter(LPUNKNOWN pUnk, HRESULT* pHr);
    ~CWavDestFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;

    HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
    HRESULT Receive(IMediaSample* pSample);

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType*mtIn, const CMediaType* mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

    HRESULT StartStreaming();
    HRESULT StopStreaming();

    HRESULT CompleteConnect(PIN_DIRECTION direction, IPin* pReceivePin)
	{
		return S_OK;
	}

private:

    HRESULT Copy(IMediaSample* pSource, IMediaSample* pDest) const;
    HRESULT Transform(IMediaSample* pMediaSample);
    HRESULT Transform(AM_MEDIA_TYPE* pType, const signed char ContrastLevel) const;

    ULONG m_cbWavData;
    ULONG m_cbHeader;
};
