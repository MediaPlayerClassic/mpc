#pragma once

class CDirectVobSubAllocator : public CMemAllocator
{
protected:
    CBaseFilter* m_pFilter;				// Delegate reference counts to
    CMediaType m_mt;
	bool m_fMediaTypeChanged;

public:
	CDirectVobSubAllocator(CBaseFilter* pFilter, HRESULT* phr);
#ifdef DEBUG
	~CDirectVobSubAllocator();
#endif

	STDMETHODIMP_(ULONG) NonDelegatingAddRef() {return m_pFilter->AddRef();}
	STDMETHODIMP_(ULONG) NonDelegatingRelease() {return m_pFilter->Release();}

	void NotifyMediaType(CMediaType mt);

	STDMETHODIMP GetBuffer(IMediaSample** ppBuffer,
		REFERENCE_TIME* pStartTime,
		REFERENCE_TIME* pEndTime,
		DWORD dwFlags);
};
