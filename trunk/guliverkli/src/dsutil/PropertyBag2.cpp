#include "StdAfx.h"
#include "DSUtil.h"
#include "PropertyBag2.h"

CPropertyBag2::CPropertyBag2()
{
}

CPropertyBag2::~CPropertyBag2()
{
}

void CPropertyBag2::SetProperty(const CStringW& key, const CStringW& value)
{
	if(!Lookup(key).IsEmpty()) SetAt(key, value);
	else Add(key, value);
}

void CPropertyBag2::SetProperty(const CStringW& key, const VARIANT& var)
{
	bool bstr = (var.vt & (VT_BSTR | VT_BYREF)) == VT_BSTR;
	ASSERT(bstr);
	if(bstr) SetProperty(key, var.bstrVal);
}

// IPropertyBag2

STDMETHODIMP CPropertyBag2::Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pvarValue, E_POINTER);
	CheckPointer(phrError, E_POINTER);
	for(ULONG i = 0; i < cProperties; phrError[i] = S_OK, i++)
		CComVariant(Lookup(pPropBag[i].pstrName)).Detach(pvarValue);
	return S_OK;
}

STDMETHODIMP CPropertyBag2::Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue)
{
	CheckPointer(pPropBag, E_POINTER);
	CheckPointer(pvarValue, E_POINTER);
	for(ULONG i = 0; i < cProperties; i++)
		SetProperty(pPropBag[i].pstrName, pvarValue[i]);
	return S_OK;
}

STDMETHODIMP CPropertyBag2::CountProperties(ULONG* pcProperties)
{
	CheckPointer(pcProperties, E_POINTER);
	*pcProperties = GetSize();
	return S_OK;
}

STDMETHODIMP CPropertyBag2::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties)
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

STDMETHODIMP CPropertyBag2::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog)
{
	return E_NOTIMPL;
}

