#pragma once
#include <atlsimpcoll.h>

[uuid("232FD5D2-4954-41E7-BF9B-09E1257B1A95")]
interface IDSMPropertyBag : public IPropertyBag2
{
	STDMETHOD(SetProperty) (LPCWSTR key, LPCWSTR value) = 0;
	STDMETHOD(SetProperty) (LPCWSTR key, VARIANT* var) = 0;
};

class CDSMPropertyBag : public ATL::CSimpleMap<CStringW, CStringW>, public IDSMPropertyBag, public IPropertyBag
{
	BOOL Add(const CStringW& key, const CStringW& val) {return __super::Add(key, val);}
	BOOL SetAt(const CStringW& key, const CStringW& val) {return __super::SetAt(key, val);}

public:
	CDSMPropertyBag();
	virtual ~CDSMPropertyBag();

	// IPropertyBag

    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT* pVar);

	// IPropertyBag2

	STDMETHODIMP Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError);
	STDMETHODIMP Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue);
	STDMETHODIMP CountProperties(ULONG* pcProperties);
	STDMETHODIMP GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties);
	STDMETHODIMP LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog);

	// IDSMPropertyBag

	STDMETHODIMP SetProperty(LPCWSTR key, LPCWSTR value);
	STDMETHODIMP SetProperty(LPCWSTR key, VARIANT* var);
};
