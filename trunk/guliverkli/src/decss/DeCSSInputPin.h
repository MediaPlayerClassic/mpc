#pragma once

class CDeCSSInputPin : public CTransformInputPin, public IKsPropertySet
{
	int m_varient;
	BYTE m_Challenge[10], m_KeyCheck[5], m_Key[10];
	BYTE m_DiscKey[6], m_TitleKey[6];

protected:
	// return S_FALSE here if you don't want the base class 
	// to call CTransformFilter::Receive with this sample
	virtual HRESULT Transform(IMediaSample* pSample) {return S_OK;}

public:
    CDeCSSInputPin(TCHAR* pObjectName, CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IMemInputPin
    STDMETHODIMP Receive(IMediaSample* pSample);

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};