#pragma once

class CDirectVobSubFilter;

class CTextInputPin : public CBaseInputPin
{
    CDirectVobSubFilter* m_pFilter;

	CCritSec* m_pSubLock;
	CComPtr<ISubStream> m_pSubStream;

public:
    CTextInputPin(CDirectVobSubFilter* pFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr);

    HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT CompleteConnect(IPin* pReceivePin);
	HRESULT BreakConnect();
    STDMETHODIMP Receive(IMediaSample* pSample);

	ISubStream* GetSubStream() {return m_pSubStream;}
};

