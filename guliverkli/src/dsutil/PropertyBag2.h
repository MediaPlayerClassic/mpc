#pragma once
#include <atlsimpcoll.h>

class CPropertyBag2 : public ATL::CSimpleMap<CStringW, CStringW>, public IPropertyBag2
{
	BOOL Add(const CStringW& key, const CStringW& val) {return __super::Add(key, val);}
	BOOL SetAt(const CStringW& key, const CStringW& val) {return __super::SetAt(key, val);}

public:
	CPropertyBag2();
	virtual ~CPropertyBag2();

	void SetProperty(const CStringW& key, const CStringW& value);
	void SetProperty(const CStringW& key, const VARIANT& var);

	// IPropertyBag2

	STDMETHODIMP Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError);
	STDMETHODIMP Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue);
	STDMETHODIMP CountProperties(ULONG* pcProperties);
	STDMETHODIMP GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties);
	STDMETHODIMP LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog);
};
