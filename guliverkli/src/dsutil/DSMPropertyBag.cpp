#include "StdAfx.h"
#include "DSUtil.h"
#include "DSMPropertyBag.h"

CDSMPropertyBag::CDSMPropertyBag()
{
}

CDSMPropertyBag::~CDSMPropertyBag()
{
}

// IPropertyBag

STDMETHODIMP CDSMPropertyBag::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog)
{
	CheckPointer(pVar, E_POINTER);
	if(pVar->vt != VT_EMPTY) return E_INVALIDARG;
	CStringW value = Lookup(pszPropName);
	if(value.IsEmpty()) return E_FAIL;
	CComVariant(value).Detach(pVar);
	return S_OK;
}

STDMETHODIMP CDSMPropertyBag::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
	return SetProperty(pszPropName, pVar);
}

// IPropertyBag2

STDMETHODIMP CDSMPropertyBag::Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pvarValue, E_POINTER);
	CheckPointer(phrError, E_POINTER);
	for(ULONG i = 0; i < cProperties; phrError[i] = S_OK, i++)
		CComVariant(Lookup(pPropBag[i].pstrName)).Detach(pvarValue);
	return S_OK;
}

STDMETHODIMP CDSMPropertyBag::Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pvarValue, E_POINTER);
	for(ULONG i = 0; i < cProperties; i++)
		SetProperty(pPropBag[i].pstrName, &pvarValue[i]);
	return S_OK;
}

STDMETHODIMP CDSMPropertyBag::CountProperties(ULONG* pcProperties)
{
	CheckPointer(pcProperties, E_POINTER);
	*pcProperties = GetSize();
	return S_OK;
}

STDMETHODIMP CDSMPropertyBag::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pcProperties, E_POINTER);
	for(ULONG i = 0; i < cProperties; i++, iProperty++, (*pcProperties)++) 
	{
		CStringW key = GetKeyAt(iProperty);
		pPropBag[i].pstrName = (LPWSTR)CoTaskMemAlloc((key.GetLength()+1)*sizeof(WCHAR));
		if(!pPropBag[i].pstrName) return E_FAIL;
        wcscpy(pPropBag[i].pstrName, key);
	}
	return S_OK;
}

STDMETHODIMP CDSMPropertyBag::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog)
{
	return E_NOTIMPL;
}

// IDSMProperyBag

HRESULT CDSMPropertyBag::SetProperty(LPCWSTR key, LPCWSTR value)
{
	CheckPointer(key, E_POINTER);
	CheckPointer(value, E_POINTER);
	if(!Lookup(key).IsEmpty()) SetAt(key, value);
	else Add(key, value);
	return S_OK;
}

HRESULT CDSMPropertyBag::SetProperty(LPCWSTR key, VARIANT* var)
{
	CheckPointer(key, E_POINTER);
	CheckPointer(var, E_POINTER);
	if((var->vt & (VT_BSTR | VT_BYREF)) != VT_BSTR) return E_INVALIDARG;
	return SetProperty(key, var->bstrVal);
}

