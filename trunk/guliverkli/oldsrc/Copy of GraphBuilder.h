#pragma once

#include <atlcoll.h>

class CGraphBuilder
{
	CComPtr<IGraphBuilder> m_pGB;
	CComPtr<IFilterMapper2> m_pFM;

	void SaveFilters(CInterfaceList<IBaseFilter>& bfl);
	void RestoreFilters(CInterfaceList<IBaseFilter>& bfl);

	HRESULT SafeAddFilter(IBaseFilter* pBF, LPCWSTR pName);

	HRESULT CreateFilter(const CLSID& clsid, IBaseFilter** ppBF);

public:
	CGraphBuilder(IGraphBuilder* pGB);
	virtual ~CGraphBuilder();

	void Reset();

	HRESULT Render(LPCTSTR fn);
	HRESULT Render(IBaseFilter* pBF);
	HRESULT Render(IPin* pPin);
	HRESULT Render(IPin* pPin, IBaseFilter* pBFTo);
};
