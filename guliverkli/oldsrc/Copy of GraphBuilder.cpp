#include "stdafx.h"
#include "mplayerc.h"
#include <afxtempl.h>
#include "GraphBuilder.h"
#include "..\..\DSUtil\DSUtil.h"
#include "..\..\filters\filters.h"
#include "..\..\..\include\moreuuids.h"

CGraphBuilder::CGraphBuilder(IGraphBuilder* pGB)
	: m_pGB(pGB)
{
	m_pFM.CoCreateInstance(CLSID_FilterMapper2);
}

CGraphBuilder::~CGraphBuilder()
{
}

void CGraphBuilder::SaveFilters(CInterfaceList<IBaseFilter>& bfl)
{
	bfl.RemoveAll();
	BeginEnumFilters(m_pGB, pEF, pBF)
		bfl.AddTail(pBF);
	EndEnumFilters
}

void CGraphBuilder::RestoreFilters(CInterfaceList<IBaseFilter>& bfl)
{
	BeginEnumFilters(m_pGB, pEF, pBF)
		if(!bfl.Find(pBF)) m_pGB->RemoveFilter(pBF);
	EndEnumFilters
}

HRESULT CGraphBuilder::SafeAddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
	if(!m_pGB || !pBF)
		return E_FAIL;

	FILTER_INFO fi;
	if(SUCCEEDED(pBF->QueryFilterInfo(&fi)))
	{
		if(fi.pGraph)
		{
			fi.pGraph->Release();
		}
		else // not in graph yet?
		{
			CStringW name(L"Unknown filter");

			if(!pName)
			{
				CLSID clsid = GetCLSID(pBF);
				LPOLESTR lpsz = NULL;
				StringFromCLSID(clsid, &lpsz);
				pName = lpsz ? lpsz : name;
			}

			if(FAILED(m_pGB->AddFilter(pBF, pName)))
				return E_FAIL;
		}

		return S_OK;
	}

	return E_FAIL;
}

HRESULT CGraphBuilder::CreateFilter(const CLSID& clsid, IBaseFilter** ppBF)
{
	CheckPointer(ppBF, E_POINTER);

	ASSERT(*ppBF == NULL);

	HRESULT hr = S_OK;

	if(clsid == CLSID_AVI2AC3Filter)
		*ppBF = new CAVI2AC3Filter(NULL, &hr);

	if(!ppBF) hr = E_FAIL;
	else (*ppBF)->AddRef();

	return hr;
}

void CGraphBuilder::Reset()
{
}

HRESULT CGraphBuilder::Render(LPCTSTR lpsz)
{
	if(!m_pGB) return E_FAIL;

	Reset();

	CString fn = CString(lpsz).Trim();
	if(fn.IsEmpty()) return E_FAIL;
	CStringW fnw = fn;
	CString ext = CPath(fn).GetExtension().MakeLower();

	HRESULT hr;

	CComQIPtr<IBaseFilter> pBF;

	if(!pBF && ext == _T("cda"))
	{
		hr = S_OK;
		CComPtr<IFileSourceFilter> pReader = new CCDDAReader(NULL, &hr);
		if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
			pBF = pReader;
	}

	if(!pBF)
	{
		hr = S_OK;
		CComPtr<IFileSourceFilter> pReader = new CCDXAReader(NULL, &hr);
		if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
			pBF = pReader;
	}

	if(!pBF) //&& ext == _T("ifo"))
	{
		hr = S_OK;
		CComPtr<IFileSourceFilter> pReader = new CVTSReader(NULL, &hr);
		if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
			pBF = pReader;
	}

	if(!pBF) //&& (ext == _T("fli") || ext == _T("flc")))
	{
		hr = S_OK;
		CComPtr<IFileSourceFilter> pReader = new CFLICSource(NULL, &hr);
		if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
			pBF = pReader;
	}

	if(!pBF) //&& ext == _T("d2v"))
	{
		hr = S_OK;
		CComPtr<IFileSourceFilter> pReader = new CD2VSource(NULL, &hr);
		if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
			pBF = pReader;
	}

	if(!pBF) //&& (ext == _T("dts") || ext == _T("ac3")))
	{
		hr = S_OK;
		CComPtr<IFileSourceFilter> pReader = new CDTSAC3Source(NULL, &hr);
		if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
			pBF = pReader;
	}

	if(!pBF && AfxGetAppSettings().fUseWMASFReader && fn.Find(_T("://")) < 0)
	{
		bool fWindowsMedia = (ext == _T("asf") || ext == _T("wmv") || ext == _T("wma"));

		if(!fWindowsMedia)
		{
			if(FILE* f = _tfopen(fn, _T("rb")))
			{
				BYTE buff[4];
				memset(buff, 0, sizeof(buff));
				fread(buff, 1, 4, f);
				if(*(DWORD*)buff == 0x75b22630)
					fWindowsMedia = true;
				fclose(f);
			}
		}

		if(fWindowsMedia)
		{
			CLSID clsid;
			CLSIDFromString(L"{187463A0-5BB7-11D3-ACBE-0080C75E246E}", &clsid); // WM ASF Reader
//			CLSIDFromString(L"{6B6D0800-9ADA-11D0-A520-00A0D10129C0}", &clsid); // Windows Media Source filter
			CComPtr<IFileSourceFilter> pReader;
			hr = pReader.CoCreateInstance(clsid);
			if(SUCCEEDED(hr) && SUCCEEDED(pReader->Load(fnw, NULL)))
				pBF = pReader;
		}
	}

	if(!pBF)
	{
        if(FAILED(hr = m_pGB->AddSourceFilter(fnw, fnw, &pBF)))
			return hr;
	}

	ASSERT(pBF);

	return Render(pBF);
}

HRESULT CGraphBuilder::Render(IBaseFilter* pBF)
{
	if(!m_pGB || !m_pFM || !pBF)
		return E_FAIL;

	if(FAILED(SafeAddFilter(pBF, NULL)))
		return E_FAIL;

	int nRendered = 0, nTotal = 0;

	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pPinTmp;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
		|| SUCCEEDED(pPin->ConnectedTo(&pPinTmp)))
			continue;

		nTotal++;
		if(SUCCEEDED(Render(pPin))) nRendered++;
	}
	EndEnumPins

	return 
		nRendered == nTotal ? S_OK :
		nRendered > 0 ? VFW_S_PARTIAL_RENDER :
		E_FAIL;
}

typedef struct {CLSID clsid; CComPtr<IBaseFilter> pBF; CComPtr<IMoniker> pMoniker; CStringW DispName, FriendlyName;} RegFilter;
typedef struct {ULONGLONG merit; int i;} SortFilter;

static int compare(const void* sf1, const void* sf2)
{
	ULONGLONG m1 = ((SortFilter*)sf1)->merit;
	ULONGLONG m2 = ((SortFilter*)sf2)->merit;
	if(m1 < m2) return 1;
	else if(m1 > m2) return -1;
	return 0;
}

HRESULT CGraphBuilder::Render(IPin* pPin)
{
	if(!m_pGB || !m_pFM || !pPin)
		return E_FAIL;

	PIN_DIRECTION dir;
	CComPtr<IPin> pPinTmp;
	if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
	|| SUCCEEDED(pPin->ConnectedTo(&pPinTmp)))
		return E_UNEXPECTED;

	PIN_INFO pi;
	if(FAILED(pPin->QueryPinInfo(&pi)))
		return E_UNEXPECTED;

	if(pi.pFilter) pi.pFilter->Release();

	if(pi.achName && pi.achName[0] == '~')
		return S_OK;

	// 1. stream builder interface

	if(CComQIPtr<IStreamBuilder> pSB = pPin)
	{
		CInterfaceList<IBaseFilter> bfl;
		SaveFilters(bfl);

		if(SUCCEEDED(pSB->Render(pPin, m_pGB)))
			return S_OK;

		pSB->Backout(pPin, m_pGB);

		RestoreFilters(bfl);
	}

	// 2. filter cache

	if(CComQIPtr<IGraphConfig> pGC = m_pGB)
	{
		CComPtr<IEnumFilters> pEF;
		if(SUCCEEDED(pGC->EnumCacheFilter(&pEF)))
		{
			for(CComPtr<IBaseFilter> pBF; S_OK == pEF->Next(1, &pBF, NULL); pBF = NULL)
			{
				CInterfaceList<IBaseFilter> bfl;
				SaveFilters(bfl);

				pGC->RemoveFilterFromCache(pBF);

				HRESULT hr;
				if(SUCCEEDED(hr = Render(pBF)))
					return hr;

				pGC->AddFilterToCache(pBF);

				RestoreFilters(bfl);
			}
		}
	}

	// 3. disconnected inputs

	BeginEnumFilters(m_pGB, pEF, pBF)
	{
		HRESULT hr;
		if(SUCCEEDED(hr = Render(pPin, pBF)))
			return hr;
	}
	EndEnumFilters

	// 4. registry (thru filter mapper 2)

	CArray<GUID> guids; // yuck, but won't hurt much, the number of media types are usually low

    BeginEnumMediaTypes(pPin, pEM, pmt)
	{
		guids.Add(pmt->majortype);
		guids.Add(pmt->subtype);
	}
	EndEnumMediaTypes(pmt)

	CComPtr<IEnumMoniker> pEM;
	if(guids.GetCount() > 0 
	&& SUCCEEDED(m_pFM->EnumMatchingFilters(
		&pEM, 0, FALSE, MERIT_DO_NOT_USE+1, 
		TRUE, guids.GetCount(), guids.GetData(), NULL, NULL, FALSE,
		FALSE, 0, NULL, NULL, NULL)))
	{
		CArray<RegFilter> rfs;
		CArray<SortFilter> sfs;
		for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
		{
			SortFilter sf;
			sf.merit = MERIT_DO_NOT_USE+1;
			sf.i = sfs.GetCount();

			RegFilter rf;
			rf.clsid = GUID_NULL;
			rf.pMoniker = pMoniker;

			LPOLESTR str = NULL;
			if(FAILED(pMoniker->GetDisplayName(0, 0, &str)))
				continue;
			rf.DispName = rf.FriendlyName = str;
            CoTaskMemFree(str), str = NULL;

            CComPtr<IPropertyBag> pPB;
			if(SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
			{
				CComVariant var;
				if(SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
					rf.FriendlyName = var.bstrVal;

                var.Clear();
				if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
				{			
					BSTR* pstr;
					if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
					{
						sf.merit = *((DWORD*)pstr + 1);
						SafeArrayUnaccessData(var.parray);
					}
				}
			}

			rfs.Add(rf);
			sfs.Add(sf);
		}

		// add built-in helper filters

		for(int i = 0; i < guids.GetCount(); i += 2)
		{
			if(guids[i] == MEDIATYPE_Audio && 
			(guids[i+1] == MEDIASUBTYPE_WAVE_DOLBY_AC3 || guids[i+1] == MEDIASUBTYPE_WAVE_DTS))
			{
				SortFilter sf;
				sf.merit = MERIT_PREFERRED-1;
				sf.i = sfs.GetCount();

				RegFilter rf;
				rf.clsid = CLSID_AVI2AC3Filter;
				rf.FriendlyName = L"AVI<->AC3/DTS";

				rfs.Add(rf);
				sfs.Add(sf);

				break;
			}
		}
/*
		for(int i = 0; i < guids.GetCount(); i += 2)
		{
			if(guids[i] == MEDIATYPE_Video && guids[i+1] == MEDIASUBTYPE_YUY2)
			{
				SortFilter sf;
				sf.merit = 0x100000000i64;
				sf.i = sfs.GetCount();

				RegFilter rf;
				rf.clsid = CLSID_VideoMixingRenderer;
				rf.FriendlyName = L"VideoMixingRenderer";

				rfs.Add(rf);
				sfs.Add(sf);

				break;
			}
		}
*/
		if(sfs.GetCount() > 1)
			qsort(sfs.GetData(), sfs.GetCount(), sizeof(SortFilter), compare);

		for(int i = 0; i < sfs.GetCount(); i++)
		{
			RegFilter& rf = rfs[sfs[i].i];

			CComPtr<IBaseFilter> pBF;
			if((pBF = rf.pBF) 
			|| rf.pMoniker && SUCCEEDED(rf.pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pBF))
			/*|| rf.clsid != GUID_NULL && SUCCEEDED(pBF.CoCreateInstance(rf.clsid))
			|| rf.clsid != GUID_NULL && SUCCEEDED(CreateFilter(rf.clsid, &pBF))*/)
			{
				CInterfaceList<IBaseFilter> bfl;
				SaveFilters(bfl);

				if(FAILED(SafeAddFilter(pBF, rf.FriendlyName)))
					continue;

				HRESULT hr;
				if(SUCCEEDED(hr = Render(pPin, pBF)))
					return hr;

				m_pGB->RemoveFilter(pBF);

				RestoreFilters(bfl);
			}
		}
	}

	guids.RemoveAll();

	// 5. renderers

//	return m_pGB->Render(pPin);
	// TODO

	return E_FAIL;
}

HRESULT CGraphBuilder::Render(IPin* pPin, IBaseFilter* pBF)
{
	if(!pPin || !pBF)
		return E_FAIL;

	if(FAILED(SafeAddFilter(pBF, NULL)))
		return E_FAIL;

	BeginEnumPins(pBF, pEP, pPinTo)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pPinTmp;
		if(FAILED(pPinTo->QueryDirection(&dir)) || dir != PINDIR_INPUT
		|| SUCCEEDED(pPinTo->ConnectedTo(&pPinTmp)))
			continue;

		// maybe we should call Connect instead
		if(SUCCEEDED(m_pGB->ConnectDirect(pPin, pPinTo, NULL)))
			return Render(pBF);
	}
	EndEnumPins

	return E_FAIL;
}