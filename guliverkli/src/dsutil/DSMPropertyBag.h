#pragma once
#include <atlsimpcoll.h>

// IDSMPropertyBag

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

// IDSMResourceBag

[uuid("EBAFBCBE-BDE0-489A-9789-05D5692E3A93")]
interface IDSMResourceBag : public IUnknown
{
	STDMETHOD_(DWORD, ResGetCount) () = 0;
	STDMETHOD(ResGet) (DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag) = 0;
	STDMETHOD(ResSet) (DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag) = 0;
	STDMETHOD_(DWORD, ResAppend) (LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag) = 0;
	STDMETHOD(ResRemoveAt) (DWORD iIndex) = 0;
	STDMETHOD(ResRemoveAll) (DWORD_PTR tag) = 0;
};

class CDSMResource
{
public:
	DWORD_PTR tag;
	CStringW name, desc, mime;
	CArray<BYTE> data;
	void operator = (const CDSMResource& r) {tag = r.tag; name = r.name; desc = r.desc; mime = r.mime; data.Copy(r.data);}
};

class CDSMResourceBag : public IDSMResourceBag
{
protected:
	CArray<CDSMResource> m_resources;

public:
	CDSMResourceBag();

	// IDSMResourceBag

	STDMETHODIMP_(DWORD) ResGetCount();
	STDMETHODIMP ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag = NULL);
	STDMETHODIMP ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag = 0);
	STDMETHODIMP_(DWORD) ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag = 0);
	STDMETHODIMP ResRemoveAt(DWORD iIndex);
	STDMETHOD(ResRemoveAll) (DWORD_PTR tag = 0);
};
