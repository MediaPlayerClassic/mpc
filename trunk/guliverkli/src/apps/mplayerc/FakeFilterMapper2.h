#pragma once

#include <afxtempl.h>
#include <atlbase.h>

class Filter
{
public:
	bool fDisabled, fTemporary;
	enum {REGISTERED, EXTERNAL} type;
	// REGISTERED
	CStringW dispname;
	// EXTERNAL
	CString path, name;
	CLSID clsid;
	// props
	CList<GUID> guids, backup;
	enum {PREFERRED, BLOCK, MERIT};
	int iLoadType;
	DWORD dwMerit;

	Filter() {fTemporary = false;}
	Filter(Filter* f)
	{
		fDisabled = f->fDisabled;
		fTemporary = f->fTemporary;
		type = f->type;
		dispname = f->dispname;
		path = f->path;
		name = f->name;
		clsid = f->clsid;
		guids.AddTail(&f->guids);
		backup.AddTail(&f->backup);
		iLoadType = f->iLoadType;
		dwMerit = f->dwMerit;
	}
};
/*
class CFilterMapper2 : protected CUnknown, protected IFilterMapper2
{
	static bool fInitialized;

	CComPtr<IFilterMapper2> m_pFM2;
	CString m_path;

protected:
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFilterMapper2
	
	STDMETHODIMP CreateCategory(REFCLSID clsidCategory, DWORD dwCategoryMerit, LPCWSTR Description);
	STDMETHODIMP UnregisterFilter(const CLSID* pclsidCategory, const OLECHAR* szInstance, REFCLSID Filter);	
	STDMETHODIMP RegisterFilter(REFCLSID clsidFilter, LPCWSTR Name, IMoniker** ppMoniker, const CLSID* pclsidCategory, const OLECHAR* szInstance, const REGFILTER2* prf2);
	STDMETHODIMP EnumMatchingFilters(IEnumMoniker** ppEnum, DWORD dwFlags, BOOL bExactMatch, DWORD dwMerit, 
		BOOL bInputNeeded, DWORD cInputTypes, const GUID* pInputTypes, const REGPINMEDIUM* pMedIn, const CLSID* pPinCategoryIn, BOOL bRender, 
		BOOL bOutputNeeded, DWORD cOutputTypes, const GUID* pOutputTypes, const REGPINMEDIUM* pMedOut, const CLSID* pPinCategoryOut);

public: 
	CFilterMapper2();
	virtual ~CFilterMapper2();

	static void Init();

	static IFilterMapper2* m_pFilterMapper2;
	CList<Filter*> m_filters;
	void Register(CString path);
};
*/

class CFilterMapper2 : protected CUnknown, public IFilterMapper2
{
	static bool fInitialized;

	CComPtr<IUnknown> m_pFM2;
	CString m_path;

	bool m_fRefCounted, m_fAllowUnreg;

protected:
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFilterMapper2
	
	STDMETHODIMP CreateCategory(REFCLSID clsidCategory, DWORD dwCategoryMerit, LPCWSTR Description);
	STDMETHODIMP UnregisterFilter(const CLSID* pclsidCategory, const OLECHAR* szInstance, REFCLSID Filter);	
	STDMETHODIMP RegisterFilter(REFCLSID clsidFilter, LPCWSTR Name, IMoniker** ppMoniker, const CLSID* pclsidCategory, const OLECHAR* szInstance, const REGFILTER2* prf2);
	STDMETHODIMP EnumMatchingFilters(IEnumMoniker** ppEnum, DWORD dwFlags, BOOL bExactMatch, DWORD dwMerit, 
		BOOL bInputNeeded, DWORD cInputTypes, const GUID* pInputTypes, const REGPINMEDIUM* pMedIn, const CLSID* pPinCategoryIn, BOOL bRender, 
		BOOL bOutputNeeded, DWORD cOutputTypes, const GUID* pOutputTypes, const REGPINMEDIUM* pMedOut, const CLSID* pPinCategoryOut);

public: 
	CFilterMapper2(bool fRefCounted, bool fAllowUnreg = false, LPUNKNOWN pUnkOuter = NULL);
	virtual ~CFilterMapper2();

	void SetInner(IUnknown* pUnk) {m_pFM2 = pUnk;}

	static void Init();

	static IFilterMapper2* m_pFilterMapper2;
	CList<Filter*> m_filters;
	void Register(CString path);
};
