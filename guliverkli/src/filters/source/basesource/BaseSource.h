#pragma once

#include "..\..\..\DSUtil\DSUtil.h"

template<class TStream>
class CBaseSource
	: public CSource
	, public IFileSourceFilter
	, public IAMFilterMiscFlags
{
protected:
	CStringW m_fn;

public:
	CBaseSource(TCHAR* name, LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid)
		: CSource(name, lpunk, clsid)
	{
		if(phr) *phr = S_OK;
	}

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		CheckPointer(ppv, E_POINTER);

		return 
			QI(IFileSourceFilter)
			QI(IAMFilterMiscFlags)
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
	{
		// TODO: destroy any already existing pins and create new, now we are just going die nicely instead of doing it :)
		if(GetPinCount() > 0)
			return VFW_E_ALREADY_CONNECTED;

		HRESULT hr = S_OK;
		if(!(new TStream(pszFileName, this, &hr)))
			return E_OUTOFMEMORY;

		if(FAILED(hr))
			return hr;

		m_fn = pszFileName;

		return S_OK;
	}

	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
	{
		if(!ppszFileName) return E_POINTER;
		
		if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
			return E_OUTOFMEMORY;

		wcscpy(*ppszFileName, m_fn);

		return S_OK;
	}

	// IAMFilterMiscFlags

	STDMETHODIMP_(ULONG) GetMiscFlags()
	{
		return AM_FILTER_MISC_FLAGS_IS_SOURCE;
	}
};

class CBaseStream 
	: public CSourceStream
	, public CSourceSeeking
{
protected:
	CCritSec m_cSharedState;

	REFERENCE_TIME m_AvgTimePerFrame;
	REFERENCE_TIME m_rtSampleTime, m_rtPosition;

	BOOL m_bDiscontinuity, m_bFlushing;

	HRESULT OnThreadStartPlay();
	HRESULT OnThreadCreate();

private:
	void UpdateFromSeek();
	STDMETHODIMP SetRate(double dRate);

	HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate() {return S_OK;}

public:
    CBaseStream(TCHAR* name, CSource* pParent, HRESULT* phr);
	virtual ~CBaseStream();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT FillBuffer(IMediaSample* pSample);

    virtual HRESULT FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len /*in+out*/) = 0;

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};
