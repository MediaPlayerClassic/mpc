#include "StdAfx.h"
#include "DSUtil.h"
#include "DSMPropertyBag.h"

//
// CDSMPropertyBag
//

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
		pPropBag[i].pstrName = (BSTR)CoTaskMemAlloc((key.GetLength()+1)*sizeof(WCHAR));
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

HRESULT CDSMPropertyBag::GetProperty(LPCWSTR key, BSTR* value)
{
	CheckPointer(key, E_POINTER);
	CheckPointer(value, E_POINTER);
	int i = FindKey(key);
	if(i < 0) return E_FAIL;
	*value = GetValueAt(i).AllocSysString();
	return S_OK;
}

HRESULT CDSMPropertyBag::DelAllProperties()
{
	RemoveAll();
	return S_OK;
}

HRESULT CDSMPropertyBag::DelProperty(LPCWSTR key)
{
	return Remove(key) ? S_OK : S_FALSE;
}

//
// CDSMResource
//

CCritSec CDSMResource::m_csResources;
CAtlMap<DWORD, CDSMResource*> CDSMResource::m_resources;

CDSMResource::CDSMResource() 
	: mime(_T("application/octet-stream"))
	, tag(0)
{
	CAutoLock cAutoLock(&m_csResources);
	m_resources.SetAt((DWORD)this, this);
}

CDSMResource::CDSMResource(LPCWSTR name, LPCWSTR desc, LPCWSTR mime, BYTE* pData, int len, DWORD_PTR tag)
{
	this->name = name;
	this->desc = desc;
	this->mime = mime;
	data.SetSize(len);
	memcpy(data.GetData(), pData, data.GetSize());
	this->tag = tag;

	CAutoLock cAutoLock(&m_csResources);
	m_resources.SetAt((DWORD)this, this);
}

CDSMResource::~CDSMResource()
{
	CAutoLock cAutoLock(&m_csResources);
	m_resources.RemoveKey((DWORD)this);
}

void CDSMResource::operator = (const CDSMResource& r)
{
	tag = r.tag;
	name = r.name;
	desc = r.desc;
	mime = r.mime;
	data.Copy(r.data);
}

//
// CDSMResourceBag
//

CDSMResourceBag::CDSMResourceBag()
{
}

// IDSMResourceBag

STDMETHODIMP_(DWORD) CDSMResourceBag::ResGetCount()
{
	return m_resources.GetCount();
}

STDMETHODIMP CDSMResourceBag::ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag)
{
	if(ppData) CheckPointer(pDataLen, E_POINTER);

	if((INT_PTR)iIndex >= m_resources.GetCount())
		return E_INVALIDARG;

	CDSMResource& r = m_resources[iIndex];

	if(ppName) *ppName = r.name.AllocSysString();
	if(ppDesc) *ppDesc = r.desc.AllocSysString();
	if(ppMime) *ppMime = r.mime.AllocSysString();
	if(ppData) {*pDataLen = r.data.GetSize(); memcpy(*ppData = (BYTE*)CoTaskMemAlloc(*pDataLen), r.data.GetData(), *pDataLen);}
	if(pTag) *pTag = r.tag;

	return S_OK;
}

STDMETHODIMP CDSMResourceBag::ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag)
{
	if((INT_PTR)iIndex >= m_resources.GetCount())
		return E_INVALIDARG;

	CDSMResource& r = m_resources[iIndex];

	if(pName) r.name = pName;
	if(pDesc) r.desc = pDesc;
	if(pMime) r.mime = pMime;
	if(pData || len == 0) {r.data.SetSize(len); if(pData) memcpy(r.data.GetData(), pData, r.data.GetSize());}
	r.tag = tag;

	return S_OK;
}

STDMETHODIMP CDSMResourceBag::ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag)
{
	return ResSet(m_resources.Add(CDSMResource()), pName, pDesc, pMime, pData, len, tag);
}

STDMETHODIMP CDSMResourceBag::ResRemoveAt(DWORD iIndex)
{
	if((INT_PTR)iIndex >= m_resources.GetCount())
		return E_INVALIDARG;

	m_resources.RemoveAt(iIndex);

	return S_OK;
}

STDMETHODIMP CDSMResourceBag::ResRemoveAll(DWORD_PTR tag)
{
	if(tag)
	{
		for(int i = m_resources.GetCount() - 1; i >= 0; i--)
			if(m_resources[i].tag == tag)
				m_resources.RemoveAt(i);
	}
	else
	{
		m_resources.RemoveAll();
	}

	return S_OK;
}

//
// CDSMChapter
//

CDSMChapter::CDSMChapter(REFERENCE_TIME rt, LPCWSTR name)
{
	this->rt = rt;
	this->name = name;
}

void CDSMChapter::operator = (const CDSMChapter& c)
{
	rt = c.rt;
	name = c.name;
}

//
// CDSMChapterBag
//

CDSMChapterBag::CDSMChapterBag()
{
	m_fSorted = false;
}

// IDSMRChapterBag

STDMETHODIMP_(DWORD) CDSMChapterBag::ChapGetCount()
{
	return m_chapters.GetCount();
}

STDMETHODIMP CDSMChapterBag::ChapGet(DWORD iIndex, REFERENCE_TIME* prt, BSTR* ppName)
{
	if((INT_PTR)iIndex >= m_chapters.GetCount())
		return E_INVALIDARG;

	CDSMChapter& c = m_chapters[iIndex];

	if(prt) *prt = c.rt;
	if(ppName) *ppName = c.name.AllocSysString();

	return S_OK;
}

STDMETHODIMP CDSMChapterBag::ChapSet(DWORD iIndex, REFERENCE_TIME rt, LPCWSTR pName)
{
	if((INT_PTR)iIndex >= m_chapters.GetCount())
		return E_INVALIDARG;

	CDSMChapter& c = m_chapters[iIndex];

	c.rt = rt;
	if(pName) c.name = pName;

	m_fSorted = false;

	return S_OK;
}

STDMETHODIMP CDSMChapterBag::ChapAppend(REFERENCE_TIME rt, LPCWSTR pName)
{
	return ChapSet(m_chapters.Add(CDSMChapter()), rt, pName);
}

STDMETHODIMP CDSMChapterBag::ChapRemoveAt(DWORD iIndex)
{
	if((INT_PTR)iIndex >= m_chapters.GetCount())
		return E_INVALIDARG;

	m_chapters.RemoveAt(iIndex);

	return S_OK;
}

STDMETHODIMP CDSMChapterBag::ChapRemoveAll()
{
	m_chapters.RemoveAll();

	m_fSorted = false;

	return S_OK;
}

STDMETHODIMP_(long) CDSMChapterBag::ChapLookup(REFERENCE_TIME* prt, BSTR* ppName)
{
	CheckPointer(prt, -1);

	ChapSort();

	int i = range_bsearch(m_chapters, *prt);
	if(i < 0) return -1;

	*prt = m_chapters[i].rt;
	if(ppName) *ppName = m_chapters[i].name.AllocSysString();

	return i;
}

static int chapter_comp(const void* a, const void* b)
{
	if(((CDSMChapter*)a)->rt > ((CDSMChapter*)b)->rt) return 1;
	else if(((CDSMChapter*)a)->rt < ((CDSMChapter*)b)->rt) return -1;
	return 0;
}

STDMETHODIMP CDSMChapterBag::ChapSort()
{
	if(m_fSorted) return S_FALSE;
	qsort(m_chapters.GetData(), m_chapters.GetCount(), sizeof(CDSMChapter), chapter_comp);
	m_fSorted = true;
	return S_OK;
}