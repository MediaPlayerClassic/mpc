/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <atlcoll.h>
#include <afxtempl.h>
#include <Shlwapi.h>
#include <atlpath.h>
#include <Vfw.h>
#include "..\..\include\winddk\devioctl.h"
#include "..\..\include\winddk\ntddcdrm.h"
#include "DSUtil.h"

void DumpStreamConfig(TCHAR* fn, IAMStreamConfig* pAMVSCCap)
{
	CString s, ss;
	CStdioFile f;
	if(!f.Open(fn, CFile::modeCreate|CFile::modeWrite|CFile::typeText))
		return;

	int cnt = 0, size = 0;
	if(FAILED(pAMVSCCap->GetNumberOfCapabilities(&cnt, &size)))
		return;

	s.Empty();
	s.Format(_T("cnt %d, size %d\n"), cnt, size);
	f.WriteString(s);

	if(size == sizeof(VIDEO_STREAM_CONFIG_CAPS))
	{
		for(int i = 0; i < cnt; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;

			VIDEO_STREAM_CONFIG_CAPS caps;
			memset(&caps, 0, sizeof(caps));

			s.Empty();
			ss.Format(_T("%d\n"), i); s += ss;
			f.WriteString(s);

			if(FAILED(pAMVSCCap->GetStreamCaps(i, &pmt, (BYTE*)&caps)))
				continue;

			{
				s.Empty();
				ss.Format(_T("VIDEO_STREAM_CONFIG_CAPS\n")); s += ss;
				ss.Format(_T("\tVideoStandard 0x%08x\n"), caps.VideoStandard); s += ss;
				ss.Format(_T("\tInputSize %dx%d\n"), caps.InputSize); s += ss;
				ss.Format(_T("\tCroppingSize %dx%d - %dx%d\n"), caps.MinCroppingSize, caps.MaxCroppingSize); s += ss;
				ss.Format(_T("\tCropGranularity %d, %d\n"), caps.CropGranularityX, caps.CropGranularityY); s += ss;
				ss.Format(_T("\tCropAlign %d, %d\n"), caps.CropAlignX, caps.CropAlignY); s += ss;
				ss.Format(_T("\tOutputSize %dx%d - %dx%d\n"), caps.MinOutputSize, caps.MaxOutputSize); s += ss;
				ss.Format(_T("\tOutputGranularity %d, %d\n"), caps.OutputGranularityX, caps.OutputGranularityY); s += ss;
				ss.Format(_T("\tStretchTaps %d, %d\n"), caps.StretchTapsX, caps.StretchTapsY); s += ss;
				ss.Format(_T("\tShrinkTaps %d, %d\n"), caps.ShrinkTapsX, caps.ShrinkTapsY); s += ss;
				ss.Format(_T("\tFrameInterval %I64d, %I64d (%.4f, %.4f)\n"), 
					caps.MinFrameInterval, caps.MaxFrameInterval,
					(float)10000000/caps.MinFrameInterval, (float)10000000/caps.MaxFrameInterval); s += ss;
				ss.Format(_T("\tBitsPerSecond %d - %d\n"), caps.MinBitsPerSecond, caps.MaxBitsPerSecond); s += ss;
				f.WriteString(s);
			}

			BITMAPINFOHEADER* pbh;
			if(pmt->formattype == FORMAT_VideoInfo) 
			{
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
				pbh = &vih->bmiHeader;

				s.Empty();
				ss.Format(_T("FORMAT_VideoInfo\n")); s += ss;
				ss.Format(_T("\tAvgTimePerFrame %I64d, %.4f\n"), vih->AvgTimePerFrame, (float)10000000/vih->AvgTimePerFrame); s += ss;
				ss.Format(_T("\trcSource %d,%d,%d,%d\n"), vih->rcSource); s += ss;
				ss.Format(_T("\trcTarget %d,%d,%d,%d\n"), vih->rcTarget); s += ss;
				f.WriteString(s);
			}
			else if(pmt->formattype == FORMAT_VideoInfo2) 
			{
				VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
				pbh = &vih->bmiHeader;

				s.Empty();
				ss.Format(_T("FORMAT_VideoInfo2\n")); s += ss;
				ss.Format(_T("\tAvgTimePerFrame %I64d, %.4f\n"), vih->AvgTimePerFrame, (float)10000000/vih->AvgTimePerFrame); s += ss;
				ss.Format(_T("\trcSource %d,%d,%d,%d\n"), vih->rcSource); s += ss;
				ss.Format(_T("\trcTarget %d,%d,%d,%d\n"), vih->rcTarget); s += ss;
				ss.Format(_T("\tdwInterlaceFlags 0x%x\n"), vih->dwInterlaceFlags); s += ss;
				ss.Format(_T("\tdwPictAspectRatio %d:%d\n"), vih->dwPictAspectRatioX, vih->dwPictAspectRatioY); s += ss;
				f.WriteString(s);
			}
			else
			{
				DeleteMediaType(pmt);
				continue;
			}

			s.Empty();
			ss.Format(_T("BITMAPINFOHEADER\n")); s += ss;
			ss.Format(_T("\tbiCompression %x\n"), pbh->biCompression); s += ss;
			ss.Format(_T("\tbiWidth %d\n"), pbh->biWidth); s += ss;
			ss.Format(_T("\tbiHeight %d\n"), pbh->biHeight); s += ss;
			ss.Format(_T("\tbiBitCount %d\n"), pbh->biBitCount); s += ss;
			ss.Format(_T("\tbiPlanes %d\n"), pbh->biPlanes); s += ss;
			ss.Format(_T("\tbiSizeImage %d\n"), pbh->biSizeImage); s += ss;
			f.WriteString(s);

			DeleteMediaType(pmt);
		}
	}
	else if(size == sizeof(AUDIO_STREAM_CONFIG_CAPS))
	{
		// TODO
	}
}

int CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC)
{
	nIn = nOut = 0;
	nInC = nOutC = 0;

	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir;
		if(SUCCEEDED(pPin->QueryDirection(&dir)))
		{
			CComPtr<IPin> pPinConnectedTo;
			pPin->ConnectedTo(&pPinConnectedTo);

			if(dir == PINDIR_INPUT) {nIn++; if(pPinConnectedTo) nInC++;}
			else if(dir == PINDIR_OUTPUT) {nOut++; if(pPinConnectedTo) nOutC++;}
		}
	}
	EndEnumPins

	return(nIn+nOut);
}

bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	return(fCountConnectedOnly ? nOutC > 1 : nOut > 1);
}

bool IsMultiplexer(IBaseFilter* pBF, bool fCountConnectedOnly)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	return(fCountConnectedOnly ? nInC > 1 : nIn > 1);
}

bool IsStreamStart(IBaseFilter* pBF)
{
	CComQIPtr<IAMFilterMiscFlags> pAMMF(pBF);
	if(pAMMF && pAMMF->GetMiscFlags()&AM_FILTER_MISC_FLAGS_IS_SOURCE)
		return(true);

	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	AM_MEDIA_TYPE mt;
	CComPtr<IPin> pIn = GetFirstPin(pBF);
	return((nOut > 1)
		|| (nOut > 0 && nIn == 1 && pIn && SUCCEEDED(pIn->ConnectionMediaType(&mt)) && mt.majortype == MEDIATYPE_Stream));
}

bool IsStreamEnd(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	return(nOut == 0);
}

bool IsVideoRenderer(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);

	if(nInC > 0 && nOut == 0)
	{
		BeginEnumPins(pBF, pEP, pPin)
		{
			AM_MEDIA_TYPE mt;
			if(S_OK != pPin->ConnectionMediaType(&mt))
				continue;

			FreeMediaType(mt);

			return(!!(mt.majortype == MEDIATYPE_Video));
				/*&& (mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_VideoInfo2));*/
		}
		EndEnumPins
	}

	CLSID clsid;
	memcpy(&clsid, &GUID_NULL, sizeof(clsid));
	pBF->GetClassID(&clsid);

	return(clsid == CLSID_VideoRenderer || clsid == CLSID_VideoRendererDefault);
}

#include <initguid.h>

DEFINE_GUID(CLSID_ReClock, 
0x9dc15360, 0x914c, 0x46b8, 0xb9, 0xdf, 0xbf, 0xe6, 0x7f, 0xd3, 0x6c, 0x6a); 

bool IsAudioWaveRenderer(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);

	if(nInC > 0 && nOut == 0 && CComQIPtr<IBasicAudio>(pBF))
	{
		BeginEnumPins(pBF, pEP, pPin)
		{
			AM_MEDIA_TYPE mt;
			if(S_OK != pPin->ConnectionMediaType(&mt))
				continue;

			FreeMediaType(mt);

			return(!!(mt.majortype == MEDIATYPE_Audio)
				/*&& mt.formattype == FORMAT_WaveFormatEx*/);
		}
		EndEnumPins
	}

	CLSID clsid;
	memcpy(&clsid, &GUID_NULL, sizeof(clsid));
	pBF->GetClassID(&clsid);

	return(clsid == CLSID_DSoundRender || clsid == CLSID_AudioRender || clsid == CLSID_ReClock);
}

IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin)
{
	BeginEnumPins(pBF, pEP, pPin)
	{
		if(pInputPin && pInputPin != pPin) continue;

		PIN_DIRECTION dir;
		CComPtr<IPin> pPinConnectedTo;
		if(SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
		&& SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo)))
		{
			return(GetFilterFromPin(pPinConnectedTo));
		}
	}
	EndEnumPins

	return(NULL);
}

IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin)
{
	BeginEnumPins(pBF, pEP, pPin)
	{
		if(pInputPin && pInputPin != pPin) continue;

		PIN_DIRECTION dir;
		CComPtr<IPin> pPinConnectedTo;
		if(SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
		&& SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo)))
		{
			IPin* pRet = pPinConnectedTo.Detach();
			pRet->Release();
			return(pRet);
		}
	}
	EndEnumPins

	return(NULL);
}

IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	if(!pBF) return(NULL);

	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir2;
		pPin->QueryDirection(&dir2);
		if(dir == dir2)
		{
			IPin* pRet = pPin.Detach();
			pRet->Release();
			return(pRet);
		}
	}
	EndEnumPins

	return(NULL);
}

IPin* GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	if(!pBF) return(NULL);

	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir2;
		pPin->QueryDirection(&dir2);
		CComPtr<IPin> pPinTo;
		if(dir == dir2 && (S_OK != pPin->ConnectedTo(&pPinTo)))
		{
			IPin* pRet = pPin.Detach();
			pRet->Release();
			return(pRet);
		}
	}
	EndEnumPins

	return(NULL);
}

int RenderFilterPins(IBaseFilter* pBF, IGraphBuilder* pGB, bool fAll)
{
	int nPinsRendered = 0;

	CInterfaceList<IPin> pProcessedPins;

	BeginEnumPins(pBF, pEP, pPin)
	{
		PIN_DIRECTION dir;
		CComPtr<IPin> pPinConnectedTo;
		if(SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_OUTPUT
		&& FAILED(pPin->ConnectedTo(&pPinConnectedTo))
		&& !pProcessedPins.Find(pPin))
		{
			CPinInfo pi;
			if(SUCCEEDED(pPin->QueryPinInfo(&pi)))
			{
				if(pi.achName[0] == '~' && !fAll)
				{
					pProcessedPins.AddTail(pPin);
					continue;
				}
			}

			if(FAILED(pPin->ConnectedTo(&pPinConnectedTo)))
			{
				if(SUCCEEDED(pGB->Render(pPin)) && SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo)))
					nPinsRendered++;
			}
				
			pEP->Reset();
		}

		pProcessedPins.AddTail(pPin);
	}
	EndEnumPins

	return(nPinsRendered);
}

void NukeDownstream(IBaseFilter* pBF, IFilterGraph* pFG)
{
	if(!pBF) return;

	BeginEnumPins(pBF, pEP, pPin)
	{
		CComPtr<IPin> pPinTo;
		CPinInfo pi;
        if(SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo
		&& SUCCEEDED(pPinTo->QueryPinInfo(&pi)))
		{
			if(pi.dir == PINDIR_INPUT)
			{
				NukeDownstream(pi.pFilter, pFG);
				pFG->Disconnect(pPinTo);
				pFG->Disconnect(pPin);
				pFG->RemoveFilter(pi.pFilter);
			}
		}
	}
	EndEnumPins
}

void NukeDownstream(IPin* pPin, IFilterGraph* pFG)
{
	CComPtr<IPin> pPinTo;
	if(!pPin || FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo)
		return;

	CPinInfo pi;
	if(FAILED(pPinTo->QueryPinInfo(&pi)) || pi.dir != PINDIR_INPUT)
		return;

	NukeDownstream(pi.pFilter, pFG);

	pFG->RemoveFilter(pi.pFilter);
}

HRESULT AddFilterToGraph(IFilterGraph* pFG, IBaseFilter* pBF, WCHAR* pName)
{
	return GetGraphFromFilter(pBF) ? S_FALSE : pFG->AddFilter(pBF, pName);
}

IBaseFilter* FindFilter(LPCWSTR clsid, IFilterGraph* pFG)
{
	CLSID clsid2;
	CLSIDFromString(CComBSTR(clsid), &clsid2);
	return(FindFilter(clsid2, pFG));
}

IBaseFilter* FindFilter(const CLSID& clsid, IFilterGraph* pFG)
{
	BeginEnumFilters(pFG, pEF, pBF)
	{
		CLSID clsid2;
		if(SUCCEEDED(pBF->GetClassID(&clsid2)) && clsid == clsid2)
			return(pBF);
	}
	EndEnumFilters

	return NULL;
}

CStringW GetFilterName(IBaseFilter* pBF)
{
	CStringW name;
	CFilterInfo fi;
	if(pBF && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
		name = fi.achName;
	return(name);
}

CStringW GetPinName(IPin* pPin)
{
	CStringW name;
	CPinInfo pi;
	if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi))) 
		name = pi.achName;
	return(name);
}

IFilterGraph* GetGraphFromFilter(IBaseFilter* pBF)
{
	IFilterGraph* pGraph = NULL;
	CFilterInfo fi;
	if(pBF && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
		pGraph = fi.pGraph;
	return(pGraph);
}

IBaseFilter* GetFilterFromPin(IPin* pPin)
{
	IBaseFilter* pBF = NULL;
	CPinInfo pi;
	if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi)))
		pBF = pi.pFilter;
	return(pBF);
}

IPin* AppendFilter(IPin* pPin, CString DisplayName, IGraphBuilder* pGB)
{
	IPin* pRet = pPin;

	CInterfaceList<IBaseFilter> pFilters;

	do
	{
		if(!pPin || DisplayName.IsEmpty() || !pGB)
			break;

		CComPtr<IPin> pPinTo;
		PIN_DIRECTION dir;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT || SUCCEEDED(pPin->ConnectedTo(&pPinTo)))
			break;

		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);

		CComPtr<IMoniker> pMoniker;
		ULONG chEaten;
		if(S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker))
			break;

		CComPtr<IBaseFilter> pBF;
		if(FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF)
			break;

		CComPtr<IPropertyBag> pPB;
		if(FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB)))
			break;

		CComVariant var;
		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			break;

		pFilters.AddTail(pBF);
		BeginEnumFilters(pGB, pEnum, pBF2) 
			pFilters.AddTail(pBF2); 
		EndEnumFilters

		if(FAILED(pGB->AddFilter(pBF, CStringW(var.bstrVal))))
			break;

		BeginEnumFilters(pGB, pEnum, pBF2) 
			if(!pFilters.Find(pBF2) && SUCCEEDED(pGB->RemoveFilter(pBF2))) 
				pEnum->Reset();
		EndEnumFilters

		pPinTo = GetFirstPin(pBF, PINDIR_INPUT);
		if(!pPinTo)
		{
			pGB->RemoveFilter(pBF);
			break;
		}

		HRESULT hr;
		if(FAILED(hr = pGB->ConnectDirect(pPin, pPinTo, NULL)))
		{
			hr = pGB->Connect(pPin, pPinTo);
			pGB->RemoveFilter(pBF);
			break;
		}

		BeginEnumFilters(pGB, pEnum, pBF2) 
			if(!pFilters.Find(pBF2) && SUCCEEDED(pGB->RemoveFilter(pBF2))) 
				pEnum->Reset();
		EndEnumFilters

		pRet = GetFirstPin(pBF, PINDIR_OUTPUT);
		if(!pRet)
		{
			pRet = pPin;
			pGB->RemoveFilter(pBF);
			break;
		}
	}
	while(false);

	return(pRet);
}

IPin* InsertFilter(IPin* pPin, CString DisplayName, IGraphBuilder* pGB)
{
	do
	{
		if(!pPin || DisplayName.IsEmpty() || !pGB)
			break;

		PIN_DIRECTION dir;
		if(FAILED(pPin->QueryDirection(&dir)))
			break;

		CComPtr<IPin> pFrom, pTo;

		if(dir == PINDIR_INPUT)
		{
			pPin->ConnectedTo(&pFrom);
			pTo = pPin;
		}
		else if(dir == PINDIR_OUTPUT)
		{
			pFrom = pPin;
			pPin->ConnectedTo(&pTo);
		}
		
		if(!pFrom || !pTo)
			break;

		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);

		CComPtr<IMoniker> pMoniker;
		ULONG chEaten;
		if(S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker))
			break;

		CComPtr<IBaseFilter> pBF;
		if(FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF)
			break;

		CComPtr<IPropertyBag> pPB;
		if(FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB)))
			break;

		CComVariant var;
		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			break;

		if(FAILED(pGB->AddFilter(pBF, CStringW(var.bstrVal))))
			break;

		CComPtr<IPin> pFromTo = GetFirstPin(pBF, PINDIR_INPUT);
		if(!pFromTo)
		{
			pGB->RemoveFilter(pBF);
			break;
		}

		if(FAILED(pGB->Disconnect(pFrom)) || FAILED(pGB->Disconnect(pTo)))
		{
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, NULL);
			break;
		}

		HRESULT hr;
		if(FAILED(hr = pGB->ConnectDirect(pFrom, pFromTo, NULL)))
		{
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, NULL);
			break;
		}

		CComPtr<IPin> pToFrom = GetFirstPin(pBF, PINDIR_OUTPUT);
		if(!pToFrom)
		{
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, NULL);
			break;
		}

		if(FAILED(pGB->ConnectDirect(pToFrom, pTo, NULL)))
		{
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, NULL);
			break;
		}

		pPin = pToFrom;
	}
	while(false);

	return(pPin);
}

int Eval_Exception(int n_except)
{
    if(n_except == STATUS_ACCESS_VIOLATION)
	{
		AfxMessageBox(_T("The property page of this filter has just caused a\nmemory access violation. The application will gently die now :)"));
	}
        
	return EXCEPTION_CONTINUE_SEARCH;
}

void MyOleCreatePropertyFrame(HWND hwndOwner, UINT x, UINT y, LPCOLESTR lpszCaption, ULONG cObjects, LPUNKNOWN FAR* lplpUnk, ULONG cPages, LPCLSID lpPageClsID, LCID lcid, DWORD dwReserved, LPVOID lpvReserved)
{
	__try
	{
		OleCreatePropertyFrame(hwndOwner, x, y, lpszCaption, cObjects, lplpUnk, cPages, lpPageClsID, lcid, dwReserved, lpvReserved);
	}
	__except(Eval_Exception(GetExceptionCode()))
	{
		// No code; this block never executed.
	}
}

void ShowPPage(CString DisplayName, HWND hParentWnd)
{
	CComPtr<IBindCtx> pBindCtx;
	CreateBindCtx(0, &pBindCtx);

	CComPtr<IMoniker> pMoniker;
	ULONG chEaten;
	if(S_OK != MkParseDisplayName(pBindCtx, CStringW(DisplayName), &chEaten, &pMoniker))
		return;

	CComPtr<IBaseFilter> pBF;
	if(FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF)
		return;

	ShowPPage(pBF, hParentWnd);	
}

void ShowPPage(IUnknown* pUnk, HWND hParentWnd)
{
	CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk;
	if(!pSPP) return;

	CString str;

	CComQIPtr<IBaseFilter> pBF = pSPP;
	CFilterInfo fi;
	CComQIPtr<IPin> pPin = pSPP;
	CPinInfo pi;
	if(pBF && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
		str = fi.achName;
	else if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi)))
		str = pi.achName;

	CAUUID caGUID;
	caGUID.pElems = NULL;
	if(SUCCEEDED(pSPP->GetPages(&caGUID)))
	{
		IUnknown* lpUnk = NULL;
		pSPP.QueryInterface(&lpUnk);
		MyOleCreatePropertyFrame(
			hParentWnd, 0, 0, CStringW(str), 
			1, (IUnknown**)&lpUnk, 
			caGUID.cElems, caGUID.pElems, 
			0, 0, NULL);
		lpUnk->Release();

		if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
	}
}

CLSID GetCLSID(IBaseFilter* pBF)
{
	CLSID clsid = GUID_NULL;
	pBF->GetClassID(&clsid);
	return(clsid);
}

CLSID GetCLSID(IPin* pPin)
{
	return(GetCLSID(GetFilterFromPin(pPin)));
}

bool IsCLSIDRegistered(LPCTSTR clsid)
{
	CString rootkey1(_T("CLSID\\"));
	CString rootkey2(_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\"));

	return ERROR_SUCCESS == CRegKey().Open(HKEY_CLASSES_ROOT, rootkey1 + clsid, KEY_READ)
		|| ERROR_SUCCESS == CRegKey().Open(HKEY_CLASSES_ROOT, rootkey2 + clsid, KEY_READ);
}

bool IsCLSIDRegistered(const CLSID& clsid)
{
	bool fRet = false;

	LPOLESTR pStr = NULL;
	if(S_OK == StringFromCLSID(clsid, &pStr) && pStr)
	{
		fRet = IsCLSIDRegistered(CString(pStr));
		CoTaskMemFree(pStr);
	}

	return(fRet);
}

void StringToBin(CString s, CByteArray& data)
{
	s.Trim();
	ASSERT((s.GetLength()&1) == 0);
	data.SetSize(s.GetLength()/2);

	BYTE b = 0;

	s.MakeUpper();
	for(int i = 0, j = s.GetLength(); i < j; i++)
	{
		TCHAR c = s[i];
		if(c >= '0' && c <= '9') 
		{
			if(!(i&1)) b = ((char(c-'0')<<4)&0xf0)|(b&0x0f);
			else b = (char(c-'0')&0x0f)|(b&0xf0);
		}
		else if(c >= 'A' && c <= 'F')
		{
			if(!(i&1)) b = ((char(c-'A'+10)<<4)&0xf0)|(b&0x0f);
			else b = (char(c-'A'+10)&0x0f)|(b&0xf0);
		}
		else break;

		if(i&1)
		{
			data[i>>1] = b;
			b = 0;
		}
	}
}

CString BinToString(BYTE* ptr, int len)
{
	CString ret;

	while(len-- > 0)
	{
		TCHAR high, low;
		high = (*ptr>>4) >= 10 ? (*ptr>>4)-10 + 'A' : (*ptr>>4) + '0';
		low = (*ptr&0xf) >= 10 ? (*ptr&0xf)-10 + 'A' : (*ptr&0xf) + '0';

		CString s;
		s.Format(_T("%c%c"), high, low);
		ret += s;

		ptr++;
	}

	return(ret);
}

static void FindFiles(CString fn, CStringList& files)
{
	CString path = fn;
	path.Replace('/', '\\');
	path = path.Left(path.ReverseFind('\\')+1);

	WIN32_FIND_DATA findData;
	HANDLE h = FindFirstFile(fn, &findData);
	if(h != INVALID_HANDLE_VALUE)
	{
		do {files.AddTail(path + findData.cFileName);}
		while(FindNextFile(h, &findData));

		FindClose(h);
	}
}

cdrom_t GetCDROMType(TCHAR drive, CStringList& files)
{
	files.RemoveAll();

	CString path;
	path.Format(_T("%c:"), drive);

	if(GetDriveType(path + _T("\\")) == DRIVE_CDROM)
	{
		// CDROM_VideoCD
		FindFiles(path + _T("\\mpegav\\avseq??.dat"), files);
		FindFiles(path + _T("\\mpegav\\avseq??.mpg"), files);
		FindFiles(path + _T("\\mpeg2\\avseq??.dat"), files);
		FindFiles(path + _T("\\mpeg2\\avseq??.mpg"), files);
		FindFiles(path + _T("\\mpegav\\music??.dat"), files);
		FindFiles(path + _T("\\mpegav\\music??.mpg"), files);
		FindFiles(path + _T("\\mpeg2\\music??.dat"), files);
		FindFiles(path + _T("\\mpeg2\\music??.mpg"), files);
		if(files.GetCount() > 0) return CDROM_VideoCD;

		// CDROM_Audio
		if(!(GetVersion()&0x80000000))
		{
			HANDLE hDrive = CreateFile(CString(_T("\\\\.\\")) + path, GENERIC_READ, FILE_SHARE_READ, NULL, 
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			if(hDrive != INVALID_HANDLE_VALUE)
			{
				DWORD BytesReturned;
				CDROM_TOC TOC;
				if(DeviceIoControl(hDrive, IOCTL_CDROM_READ_TOC, NULL, 0, &TOC, sizeof(TOC), &BytesReturned, 0))
				{
					for(int i = TOC.FirstTrack; i <= TOC.LastTrack; i++)
					{
						// MMC-3 Draft Revision 10g: Table 222 – Q Sub-channel control field
						TOC.TrackData[i-1].Control &= 5;
						if(TOC.TrackData[i-1].Control == 0 || TOC.TrackData[i-1].Control == 1) 
						{
							CString fn;
							fn.Format(_T("%s\\track%02d.cda"), path, i);
							files.AddTail(fn);
						}
					}
				}

				CloseHandle(hDrive);
			}
		}
		if(files.GetCount() > 0) return CDROM_Audio;

		// it is a cdrom but nothing special
		return CDROM_Unknown;
	}
	
	return CDROM_NotFound;
}

CString GetDriveLabel(TCHAR drive)
{
	CString label;

	CString path;
	path.Format(_T("%c:\\"), drive);
	TCHAR VolumeNameBuffer[MAX_PATH], FileSystemNameBuffer[MAX_PATH];
	DWORD VolumeSerialNumber, MaximumComponentLength, FileSystemFlags;
	if(GetVolumeInformation(path, 
		VolumeNameBuffer, MAX_PATH, &VolumeSerialNumber, &MaximumComponentLength, 
		&FileSystemFlags, FileSystemNameBuffer, MAX_PATH))
	{
		label = VolumeNameBuffer;
	}

	return(label);
}

bool GetKeyFrames(CString fn, CUIntArray& kfs)
{
	kfs.RemoveAll();

	CString fn2 = CString(fn).MakeLower();
	if(fn2.Mid(fn2.ReverseFind('.')+1) == _T("avi"))
	{
		AVIFileInit();
	    
		PAVIFILE pfile;
		if(AVIFileOpen(&pfile, fn, OF_SHARE_DENY_WRITE, 0L) == 0)
		{
			AVIFILEINFO afi;
			memset(&afi, 0, sizeof(afi));
			AVIFileInfo(pfile, &afi, sizeof(AVIFILEINFO));

			CComPtr<IAVIStream> pavi;
			if(AVIFileGetStream(pfile, &pavi, streamtypeVIDEO, 0) == AVIERR_OK)
			{
				AVISTREAMINFO si;
				AVIStreamInfo(pavi, &si, sizeof(si));

				if(afi.dwCaps&AVIFILECAPS_ALLKEYFRAMES)
				{
					kfs.SetSize(si.dwLength);
					for(int kf = 0; kf < si.dwLength; kf++) kfs[kf] = kf;
				}
				else
				{
					for(int kf = 0; ; kf++)
					{
						kf = pavi->FindSample(kf, FIND_KEY|FIND_NEXT);
						if(kf < 0 || kfs.GetCount() > 0 && kfs[kfs.GetCount()-1] >= kf) break;
						kfs.Add(kf);
					}

					if(kfs.GetCount() > 0 && kfs[kfs.GetCount()-1] < si.dwLength-1)
					{
						kfs.Add(si.dwLength-1);
					}
				}
			}

			AVIFileRelease(pfile);
		}

		AVIFileExit();
	}

	return(kfs.GetCount() > 0);
}

DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps)
{
	DVD_HMSF_TIMECODE hmsf = 
	{
		(BYTE)((rt/10000000/60/60)),
		(BYTE)((rt/10000000/60)%60),
		(BYTE)((rt/10000000)%60),
		(BYTE)(1.0*((rt/10000)%1000) * fps / 1000)
	};

	return hmsf;
}

REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps)
{
	if(fps == 0) {hmsf.bFrames = 0; fps = 1;}
	return (REFERENCE_TIME)((((REFERENCE_TIME)hmsf.bHours*60+hmsf.bMinutes)*60+hmsf.bSeconds)*1000+1.0*hmsf.bFrames*1000/fps)*10000;
}

HRESULT AddToRot(IUnknown* pUnkGraph, DWORD* pdwRegister) 
{
    CComPtr<IRunningObjectTable> pROT;
    if(FAILED(GetRunningObjectTable(0, &pROT)))
		return E_FAIL;

	WCHAR wsz[256];
    swprintf(wsz, L"FilterGraph %08p pid %08x (MPC)", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());

    HRESULT hr;

	CComPtr<IMoniker> pMoniker;
    if(SUCCEEDED(hr = CreateItemMoniker(L"!", wsz, &pMoniker)))
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, pMoniker, pdwRegister);

	return hr;
}

void RemoveFromRot(DWORD& dwRegister)
{
	CComPtr<IRunningObjectTable> pROT;
    if(SUCCEEDED(GetRunningObjectTable(0, &pROT)))
        pROT->Revoke(dwRegister);
	dwRegister = 0;
}

void memsetd(void* dst, unsigned int c, int nbytes)
{
	__asm
	{
		mov eax, c
		mov ecx, nbytes
		shr ecx, 2
		mov edi, dst
		cld
		rep stosd
	}
}

bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih)
{
	if(pmt)
	{
		if(pmt->formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
			memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
		}
		else if(pmt->formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
			memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
		}

		return(true);
	}
	
	return(false);
}

bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih)
{
	AM_MEDIA_TYPE* pmt = NULL;
	pMS->GetMediaType(&pmt);
	if(pmt)
	{
		bool fRet = ExtractBIH(pmt, bih);
		DeleteMediaType(pmt);
		return(fRet);
	}
	
	return(false);
}

unsigned __int64 GetFileVersion(LPCTSTR fn)
{
	unsigned __int64 ret = 0;

	DWORD buff[4];
	VS_FIXEDFILEINFO* pvsf = (VS_FIXEDFILEINFO*)buff;
	DWORD d; // a variable that GetFileVersionInfoSize sets to zero (but why is it needed ?????????????????????????????? :)
	DWORD len = GetFileVersionInfoSize((TCHAR*)fn, &d);
	
	if(len)
	{
		TCHAR* b1 = new TCHAR[len];
		if(b1)
		{
            UINT uLen;
            if(GetFileVersionInfo((TCHAR*)fn, 0, len, b1) && VerQueryValue(b1, _T("\\"), (void**)&pvsf, &uLen))
            {
				ret = ((unsigned __int64)pvsf->dwFileVersionMS<<32) | pvsf->dwFileVersionLS;
            }

            delete [] b1;
		}
	}

	return ret;
}

bool CreateFilter(CStringW DisplayName, IBaseFilter** ppBF, CStringW& FriendlyName)
{
	if(!ppBF) return(false);

	*ppBF = NULL;
	FriendlyName.Empty();

	CComPtr<IBindCtx> pBindCtx;
	CreateBindCtx(0, &pBindCtx);

	CComPtr<IMoniker> pMoniker;
	ULONG chEaten;
	if(S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker))
		return(false);

	if(FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)ppBF)) || !*ppBF)
		return(false);

	CComPtr<IPropertyBag> pPB;
	CComVariant var;
	if(SUCCEEDED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))
	&& SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
		FriendlyName = var.bstrVal;

	return(true);
}

IBaseFilter* AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB)
{
	do
	{
		if(!pPin || !pMoniker || !pGB)
			break;

		CComPtr<IPin> pPinTo;
		PIN_DIRECTION dir;
		if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT || SUCCEEDED(pPin->ConnectedTo(&pPinTo)))
			break;

		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);

		CComPtr<IPropertyBag> pPB;
		if(FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB)))
			break;

		CComVariant var;
		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			break;

		CComPtr<IBaseFilter> pBF;
		if(FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF)
			break;

		if(FAILED(pGB->AddFilter(pBF, CStringW(var.bstrVal))))
			break;

		BeginEnumPins(pBF, pEP, pPinTo)
		{
			PIN_DIRECTION dir;
			if(FAILED(pPinTo->QueryDirection(&dir)) || dir != PINDIR_INPUT)
				continue;

			if(SUCCEEDED(pGB->ConnectDirect(pPin, pPinTo, NULL)))
				return(pBF);
		}
		EndEnumFilters

		pGB->RemoveFilter(pBF);
	}
	while(false);

	return(NULL);
}

typedef struct
{
	CString path;
	HINSTANCE hInst;
	CLSID clsid;
} ExternalObject;

static CList<ExternalObject> s_extobjs;

HRESULT LoadExternalObject(LPCTSTR path, REFCLSID clsid, REFIID iid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	CString fullpath = MakeFullPath(path);

	HINSTANCE hInst = NULL;
	bool fFound = false;

	POSITION pos = s_extobjs.GetHeadPosition();
	while(pos)
	{
		ExternalObject& eo = s_extobjs.GetNext(pos);
		if(!eo.path.CompareNoCase(fullpath))
		{
			hInst = eo.hInst;
			fFound = true;
			break;
		}
	}

	HRESULT hr = E_FAIL;

	if(hInst || (hInst = CoLoadLibrary(CComBSTR(fullpath), TRUE)))
	{
		typedef HRESULT (__stdcall * PDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
		PDllGetClassObject p = (PDllGetClassObject)GetProcAddress(hInst, "DllGetClassObject");

		if(p && FAILED(hr = p(clsid, iid, ppv)))
		{
			CComPtr<IClassFactory> pCF;
			if(SUCCEEDED(hr = p(clsid, __uuidof(IClassFactory), (void**)&pCF)))
			{
				hr = pCF->CreateInstance(NULL, iid, ppv);
			}
		}
	}

	if(FAILED(hr) && hInst && !fFound)
	{
		CoFreeLibrary(hInst);
		return hr;
	}

	if(hInst && !fFound)
	{
		ExternalObject eo;
		eo.path = fullpath;
		eo.hInst = hInst;
		eo.clsid = clsid;
		s_extobjs.AddTail(eo);
	}

	return hr;
}

HRESULT LoadExternalFilter(LPCTSTR path, REFCLSID clsid, IBaseFilter** ppBF)
{
	return LoadExternalObject(path, clsid, __uuidof(IBaseFilter), (void**)ppBF);
}

HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP)
{
	CLSID clsid2 = GUID_NULL;
	if(FAILED(pP->GetClassID(&clsid2))) return E_FAIL;

	POSITION pos = s_extobjs.GetHeadPosition();
	while(pos)
	{
		ExternalObject& eo = s_extobjs.GetNext(pos);
		if(eo.clsid == clsid2)
		{
			return LoadExternalObject(eo.path, clsid, __uuidof(IPropertyPage), (void**)ppPP);
		}
	}

	return E_FAIL;
}

void UnloadExternalObjects()
{
	POSITION pos = s_extobjs.GetHeadPosition();
	while(pos)
	{
		ExternalObject& eo = s_extobjs.GetNext(pos);
		CoFreeLibrary(eo.hInst);
	}
	s_extobjs.RemoveAll();
}

CString MakeFullPath(LPCTSTR path)
{
	CString full(path);
	full.Replace('/', '\\');

	CString fn;
	fn.ReleaseBuffer(GetModuleFileName(AfxGetInstanceHandle(), fn.GetBuffer(MAX_PATH), MAX_PATH));
	CPath p(fn);

	if(full.GetLength() >= 2 && full[0] == '\\' && full[1] != '\\')
	{
		p.StripToRoot();
		full = CString(p) + full.Mid(1); 
	}
	else if(full.Find(_T(":\\")) < 0)
	{
		p.RemoveFileSpec();
		p.AddBackslash();
		full = CString(p) + full;
	}

	CPath c(full);
	c.Canonicalize();
	return CString(c);
}

//

CString GetMediaTypeName(const GUID& guid)
{
	CString ret = guid == GUID_NULL 
		? _T("Any type") 
		: CString(GuidNames[guid]);

	if(ret == _T("FOURCC GUID"))
	{
		CString str;
		if(guid.Data1 >= 0x10000)
			str.Format(_T("Video: %c%c%c%c"), (guid.Data1>>0)&0xff, (guid.Data1>>8)&0xff, (guid.Data1>>16)&0xff, (guid.Data1>>24)&0xff);
		else
			str.Format(_T("Audio: 0x%08x"), guid.Data1);
		ret = str;
	}
	else if(ret == _T("Unknown GUID Name"))
	{
		WCHAR null[128] = {0}, buff[128];
		StringFromGUID2(GUID_NULL, null, 127);
		ret = CString(CStringW(StringFromGUID2(guid, buff, 127) ? buff : null));
	}

	return ret;
}

GUID GUIDFromCString(CString str)
{
	GUID guid = GUID_NULL;
	CLSIDFromString(CComBSTR(str), &guid);
	return guid;
}

CString CStringFromGUID(const GUID& guid)
{
	WCHAR null[128] = {0}, buff[128];
	StringFromGUID2(GUID_NULL, null, 127);
	return CString(StringFromGUID2(guid, buff, 127) > 0 ? buff : null);
}

CStringW UTF8To16(LPCSTR utf8)
{
	CStringW str;
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0)-1;
	if(n < 0) return str;
	str.ReleaseBuffer(MultiByteToWideChar(CP_UTF8, 0, utf8, -1, str.GetBuffer(n), n+1)-1);
	return str;
}

CStringA UTF16To8(LPCWSTR utf16)
{
	CStringA str;
	int n = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL)-1;
	if(n < 0) return str;
	str.ReleaseBuffer(WideCharToMultiByte(CP_UTF8, 0, utf16, -1, str.GetBuffer(n), n+1, NULL, NULL)-1);
	return str;
}

static struct {LPCSTR name, iso6392, iso6391;} s_isolangs[] = 
{
	{"Abkhazian", "abk", "ab"},
	{"Achinese", "ace", ""},
	{"Acoli", "ach", ""},
	{"Adangme", "ada", ""},
	{"Afar", "aar", "aa"},
	{"Afrihili", "afh", ""},
	{"Afrikaans", "afr", "af"},
	{"Afro-Asiatic (Other)", "afa", ""},
	{"Akan", "aka", "ak"},
	{"Akkadian", "akk", ""},
	{"Albanian", "alb", "sq"},
	{"", "sqi", "sq"},
	{"Aleut", "ale", ""},
	{"Algonquian languages", "alg", ""},
	{"Altaic (Other)", "tut", ""},
	{"Amharic", "amh", "am"},
	{"Apache languages", "apa", ""},
	{"Arabic", "ara", "ar"},
	{"Aragonese", "arg", "an"},
	{"Aramaic", "arc", ""},
	{"Arapaho", "arp", ""},
	{"Araucanian", "arn", ""},
	{"Arawak", "arw", ""},
	{"Armenian", "arm", "hy"},
	{"", "hye", "hy"},
	{"Artificial (Other)", "art", ""},
	{"Assamese", "asm", "as"},
	{"Asturian; Bable", "ast", ""},
	{"Athapascan languages", "ath", ""},
	{"Australian languages", "aus", ""},
	{"Austronesian (Other)", "map", ""},
	{"Avaric", "ava", "av"},
	{"Avestan", "ave", "ae"},
	{"Awadhi", "awa", ""},
	{"Aymara", "aym", "ay"},
	{"Azerbaijani", "aze", "az"},
	{"Bable; Asturian", "ast", ""},
	{"Balinese", "ban", ""},
	{"Baltic (Other)", "bat", ""},
	{"Baluchi", "bal", ""},
	{"Bambara", "bam", "bm"},
	{"Bamileke languages", "bai", ""},
	{"Banda", "bad", ""},
	{"Bantu (Other)", "bnt", ""},
	{"Basa", "bas", ""},
	{"Bashkir", "bak", "ba"},
	{"Basque", "baq", "eu"},
	{"", "eus", "eu"},
	{"Batak (Indonesia)", "btk", ""},
	{"Beja", "bej", ""},
	{"Belarusian", "bel", "be"},
	{"Bemba", "bem", ""},
	{"Bengali", "ben", "bn"},
	{"Berber (Other)", "ber", ""},
	{"Bhojpuri", "bho", ""},
	{"Bihari", "bih", "bh"},
	{"Bikol", "bik", ""},
	{"Bini", "bin", ""},
	{"Bislama", "bis", "bi"},
	{"Bokmål, Norwegian; Norwegian Bokmål", "nob", "nb"},
	{"Bosnian", "bos", "bs"},
	{"Braj", "bra", ""},
	{"Breton", "bre", "br"},
	{"Buginese", "bug", ""},
	{"Bulgarian", "bul", "bg"},
	{"Buriat", "bua", ""},
	{"Burmese", "bur", "my"},
	{"", "mya", "my"},
	{"Caddo", "cad", ""},
	{"Carib", "car", ""},
	{"Spanish; Castilian", "spa", "es"},
	{"Catalan", "cat", "ca"},
	{"Caucasian (Other)", "cau", ""},
	{"Cebuano", "ceb", ""},
	{"Celtic (Other)", "cel", ""},
	{"Central American Indian (Other)", "cai", ""},
	{"Chagatai", "chg", ""},
	{"Chamic languages", "cmc", ""},
	{"Chamorro", "cha", "ch"},
	{"Chechen", "che", "ce"},
	{"Cherokee", "chr", ""},
	{"Chewa; Chichewa; Nyanja", "nya", "ny"},
	{"Cheyenne", "chy", ""},
	{"Chibcha", "chb", ""},
	{"Chichewa; Chewa; Nyanja", "nya", "ny"},
	{"Chinese", "chi", "zh"},
	{"", "zho", "zh"},
	{"Chinook jargon", "chn", ""},
	{"Chipewyan", "chp", ""},
	{"Choctaw", "cho", ""},
	{"Chuang; Zhuang", "zha", "za"},
	{"Church Slavic; Old Church Slavonic", "chu", "cu"},
	{"Old Church Slavonic; Old Slavonic; ", "chu", "cu"},
	{"Church Slavonic; Old Bulgarian; Church Slavic;", "chu", "cu"},
	{"Old Slavonic; Church Slavonic; Old Bulgarian;", "chu", "cu"},
	{"Church Slavic; Old Church Slavonic", "chu", "cu"},
	{"Chuukese", "chk", ""},
	{"Chuvash", "chv", "cv"},
	{"Coptic", "cop", ""},
	{"Cornish", "cor", "kw"},
	{"Corsican", "cos", "co"},
	{"Cree", "cre", "cr"},
	{"Creek", "mus", ""},
	{"Creoles and pidgins (Other)", "crp", ""},
	{"Creoles and pidgins,", "cpe", ""},
	//   {"English-based (Other)", "", ""},
	{"Creoles and pidgins,", "cpf", ""},
	//   {"French-based (Other)", "", ""},
	{"Creoles and pidgins,", "cpp", ""},
	//   {"Portuguese-based (Other)", "", ""},
	{"Croatian", "scr", "hr"},
	{"", "hrv", "hr"},
	{"Cushitic (Other)", "cus", ""},
	{"Czech", "cze", "cs"},
	{"", "ces", "cs"},
	{"Dakota", "dak", ""},
	{"Danish", "dan", "da"},
	{"Dargwa", "dar", ""},
	{"Dayak", "day", ""},
	{"Delaware", "del", ""},
	{"Dinka", "din", ""},
	{"Divehi", "div", "dv"},
	{"Dogri", "doi", ""},
	{"Dogrib", "dgr", ""},
	{"Dravidian (Other)", "dra", ""},
	{"Duala", "dua", ""},
	{"Dutch; Flemish", "dut", "nl"},
	{"", "nld", "nl"},
	{"Dutch, Middle (ca. 1050-1350)", "dum", ""},
	{"Dyula", "dyu", ""},
	{"Dzongkha", "dzo", "dz"},
	{"Efik", "efi", ""},
	{"Egyptian (Ancient)", "egy", ""},
	{"Ekajuk", "eka", ""},
	{"Elamite", "elx", ""},
	{"English", "eng", "en"},
	{"English, Middle (1100-1500)", "enm", ""},
	{"English, Old (ca.450-1100)", "ang", ""},
	{"Esperanto", "epo", "eo"},
	{"Estonian", "est", "et"},
	{"Ewe", "ewe", "ee"},
	{"Ewondo", "ewo", ""},
	{"Fang", "fan", ""},
	{"Fanti", "fat", ""},
	{"Faroese", "fao", "fo"},
	{"Fijian", "fij", "fj"},
	{"Finnish", "fin", "fi"},
	{"Finno-Ugrian (Other)", "fiu", ""},
	{"Flemish; Dutch", "dut", "nl"},
	{"", "nld", "nl"},
	{"Fon", "fon", ""},
	{"French", "fre", "fr"},
	{"", "fra*", "fr"},
	{"French, Middle (ca.1400-1600)", "frm", ""},
	{"French, Old (842-ca.1400)", "fro", ""},
	{"Frisian", "fry", "fy"},
	{"Friulian", "fur", ""},
	{"Fulah", "ful", "ff"},
	{"Ga", "gaa", ""},
	{"Gaelic; Scottish Gaelic", "gla", "gd"},
	{"Gallegan", "glg", "gl"},
	{"Ganda", "lug", "lg"},
	{"Gayo", "gay", ""},
	{"Gbaya", "gba", ""},
	{"Geez", "gez", ""},
	{"Georgian", "geo", "ka"},
	{"", "kat", "ka"},
	{"German", "ger", "de"},
	{"", "deu", "de"},
	{"German, Low; Saxon, Low; Low German; Low Saxon", "nds", ""},
	{"German, Middle High (ca.1050-1500)", "gmh", ""},
	{"German, Old High (ca.750-1050)", "goh", ""},
	{"Germanic (Other)", "gem", ""},
	{"Gikuyu; Kikuyu", "kik", "ki"},
	{"Gilbertese", "gil", ""},
	{"Gondi", "gon", ""},
	{"Gorontalo", "gor", ""},
	{"Gothic", "got", ""},
	{"Grebo", "grb", ""},
	{"Greek, Ancient (to 1453)", "grc", ""},
	{"Greek, Modern (1453-)", "gre", "el"},
	{"", "ell", "el"},
	{"Greenlandic; Kalaallisut", "kal", "kl"},
	{"Guarani", "grn", "gn"},
	{"Gujarati", "guj", "gu"},
	{"Gwich´in", "gwi", ""},
	{"Haida", "hai", ""},
	{"Hausa", "hau", "ha"},
	{"Hawaiian", "haw", ""},
	{"Hebrew", "heb", "he"},
	{"Herero", "her", "hz"},
	{"Hiligaynon", "hil", ""},
	{"Himachali", "him", ""},
	{"Hindi", "hin", "hi"},
	{"Hiri Motu", "hmo", "ho"},
	{"Hittite", "hit", ""},
	{"Hmong", "hmn", ""},
	{"Hungarian", "hun", "hu"},
	{"Hupa", "hup", ""},
	{"Iban", "iba", ""},
	{"Icelandic", "ice", "is"},
	{"", "isl", "is"},
	{"Ido", "ido", "io"},
	{"Igbo", "ibo", "ig"},
	{"Ijo", "ijo", ""},
	{"Iloko", "ilo", ""},
	{"Inari Sami", "smn", ""},
	{"Indic (Other)", "inc", ""},
	{"Indo-European (Other)", "ine", ""},
	{"Indonesian", "ind", "id"},
	{"Ingush", "inh", ""},
	{"Interlingua (International", "ina", "ia"},
	//   {"Auxiliary Language Association)", "", ""},
	{"Interlingue", "ile", "ie"},
	{"Inuktitut", "iku", "iu"},
	{"Inupiaq", "ipk", "ik"},
	{"Iranian (Other)", "ira", ""},
	{"Irish", "gle", "ga"},
	{"Irish, Middle (900-1200)", "mga", ""},
	{"Irish, Old (to 900)", "sga", ""},
	{"Iroquoian languages", "iro", ""},
	{"Italian", "ita", "it"},
	{"Japanese", "jpn", "ja"},
	{"Javanese", "jav", "jv"},
	{"Judeo-Arabic", "jrb", ""},
	{"Judeo-Persian", "jpr", ""},
	{"Kabardian", "kbd", ""},
	{"Kabyle", "kab", ""},
	{"Kachin", "kac", ""},
	{"Kalaallisut; Greenlandic", "kal", "kl"},
	{"Kamba", "kam", ""},
	{"Kannada", "kan", "kn"},
	{"Kanuri", "kau", "kr"},
	{"Kara-Kalpak", "kaa", ""},
	{"Karen", "kar", ""},
	{"Kashmiri", "kas", "ks"},
	{"Kawi", "kaw", ""},
	{"Kazakh", "kaz", "kk"},
	{"Khasi", "kha", ""},
	{"Khmer", "khm", "km"},
	{"Khoisan (Other)", "khi", ""},
	{"Khotanese", "kho", ""},
	{"Kikuyu; Gikuyu", "kik", "ki"},
	{"Kimbundu", "kmb", ""},
	{"Kinyarwanda", "kin", "rw"},
	{"Kirghiz", "kir", "ky"},
	{"Komi", "kom", "kv"},
	{"Kongo", "kon", "kg"},
	{"Konkani", "kok", ""},
	{"Korean", "kor", "ko"},
	{"Kosraean", "kos", ""},
	{"Kpelle", "kpe", ""},
	{"Kru", "kro", ""},
	{"Kuanyama; Kwanyama", "kua", "kj"},
	{"Kumyk", "kum", ""},
	{"Kurdish", "kur", "ku"},
	{"Kurukh", "kru", ""},
	{"Kutenai", "kut", ""},
	{"Kwanyama, Kuanyama", "kua", "kj"},
	{"Ladino", "lad", ""},
	{"Lahnda", "lah", ""},
	{"Lamba", "lam", ""},
	{"Lao", "lao", "lo"},
	{"Latin", "lat", "la"},
	{"Latvian", "lav", "lv"},
	{"Letzeburgesch; Luxembourgish", "ltz", "lb"},
	{"Lezghian", "lez", ""},
	{"Limburgan; Limburger; Limburgish", "lim", "li"},
	{"Limburger; Limburgan; Limburgish;", "lim", "li"},
	{"Limburgish; Limburger; Limburgan", "lim", "li"},
	{"Lingala", "lin", "ln"},
	{"Lithuanian", "lit", "lt"},
	{"Low German; Low Saxon; German, Low; Saxon, Low", "nds", ""},
	{"Low Saxon; Low German; Saxon, Low; German, Low", "nds", ""},
	{"Lozi", "loz", ""},
	{"Luba-Katanga", "lub", "lu"},
	{"Luba-Lulua", "lua", ""},
	{"Luiseno", "lui", ""},
	{"Lule Sami", "smj", ""},
	{"Lunda", "lun", ""},
	{"Luo (Kenya and Tanzania)", "luo", ""},
	{"Lushai", "lus", ""},
	{"Luxembourgish; Letzeburgesch", "ltz", "lb"},
	{"Macedonian", "mac", "mk"},
	{"", "mkd", "mk"},
	{"Madurese", "mad", ""},
	{"Magahi", "mag", ""},
	{"Maithili", "mai", ""},
	{"Makasar", "mak", ""},
	{"Malagasy", "mlg", "mg"},
	{"Malay", "may", "ms"},
	{"", "msa", "ms"},
	{"Malayalam", "mal", "ml"},
	{"Maltese", "mlt", "mt"},
	{"Manchu", "mnc", ""},
	{"Mandar", "mdr", ""},
	{"Mandingo", "man", ""},
	{"Manipuri", "mni", ""},
	{"Manobo languages", "mno", ""},
	{"Manx", "glv", "gv"},
	{"Maori", "mao", "mi"},
	{"", "mri", "mi"},
	{"Marathi", "mar", "mr"},
	{"Mari", "chm", ""},
	{"Marshallese", "mah", "mh"},
	{"Marwari", "mwr", ""},
	{"Masai", "mas", ""},
	{"Mayan languages", "myn", ""},
	{"Mende", "men", ""},
	{"Micmac", "mic", ""},
	{"Minangkabau", "min", ""},
	{"Miscellaneous languages", "mis", ""},
	{"Mohawk", "moh", ""},
	{"Moldavian", "mol", "mo"},
	{"Mon-Khmer (Other)", "mkh", ""},
	{"Mongo", "lol", ""},
	{"Mongolian", "mon", "mn"},
	{"Mossi", "mos", ""},
	{"Multiple languages", "mul", ""},
	{"Munda languages", "mun", ""},
	{"Nahuatl", "nah", ""},
	{"Nauru", "nau", "na"},
	{"Navaho, Navajo", "nav", "nv"},
	{"Navajo; Navaho", "nav", "nv"},
	{"Ndebele, North", "nde", "nd"},
	{"Ndebele, South", "nbl", "nr"},
	{"Ndonga", "ndo", "ng"},
	{"Neapolitan", "nap", ""},
	{"Nepali", "nep", "ne"},
	{"Newari", "new", ""},
	{"Nias", "nia", ""},
	{"Niger-Kordofanian (Other)", "nic", ""},
	{"Nilo-Saharan (Other)", "ssa", ""},
	{"Niuean", "niu", ""},
	{"Norse, Old", "non", ""},
	{"North American Indian (Other)", "nai", ""},
	{"Northern Sami", "sme", "se"},
	{"North Ndebele", "nde", "nd"},
	{"Norwegian", "nor", "no"},
	{"Norwegian Bokmål; Bokmål, Norwegian", "nob", "nb"},
	{"Norwegian Nynorsk; Nynorsk, Norwegian", "nno", "nn"},
	{"Nubian languages", "nub", ""},
	{"Nyamwezi", "nym", ""},
	{"Nyanja; Chichewa; Chewa", "nya", "ny"},
	{"Nyankole", "nyn", ""},
	{"Nynorsk, Norwegian; Norwegian Nynorsk", "nno", "nn"},
	{"Nyoro", "nyo", ""},
	{"Nzima", "nzi", ""},
	{"Occitan (post 1500},; Provençal", "oci", "oc"},
	{"Ojibwa", "oji", "oj"},
	{"Old Bulgarian; Old Slavonic; Church Slavonic;", "chu", "cu"},
	{"Oriya", "ori", "or"},
	{"Oromo", "orm", "om"},
	{"Osage", "osa", ""},
	{"Ossetian; Ossetic", "oss", "os"},
	{"Ossetic; Ossetian", "oss", "os"},
	{"Otomian languages", "oto", ""},
	{"Pahlavi", "pal", ""},
	{"Palauan", "pau", ""},
	{"Pali", "pli", "pi"},
	{"Pampanga", "pam", ""},
	{"Pangasinan", "pag", ""},
	{"Panjabi", "pan", "pa"},
	{"Papiamento", "pap", ""},
	{"Papuan (Other)", "paa", ""},
	{"Persian", "per", "fa"},
	{"", "fas", "fa"},
	{"Persian, Old (ca.600-400 B.C.)", "peo", ""},
	{"Philippine (Other)", "phi", ""},
	{"Phoenician", "phn", ""},
	{"Pohnpeian", "pon", ""},
	{"Polish", "pol", "pl"},
	{"Portuguese", "por", "pt"},
	{"Prakrit languages", "pra", ""},
	{"Provençal; Occitan (post 1500)", "oci", "oc"},
	{"Provençal, Old (to 1500)", "pro", ""},
	{"Pushto", "pus", "ps"},
	{"Quechua", "que", "qu"},
	{"Raeto-Romance", "roh", "rm"},
	{"Rajasthani", "raj", ""},
	{"Rapanui", "rap", ""},
	{"Rarotongan", "rar", ""},
	{"Reserved for local use", "qaa-qtz", ""},
	{"Romance (Other)", "roa", ""},
	{"Romanian", "rum", "ro"},
	{"", "ron", "ro"},
	{"Romany", "rom", ""},
	{"Rundi", "run", "rn"},
	{"Russian", "rus", "ru"},
	{"Salishan languages", "sal", ""},
	{"Samaritan Aramaic", "sam", ""},
	{"Sami languages (Other)", "smi", ""},
	{"Samoan", "smo", "sm"},
	{"Sandawe", "sad", ""},
	{"Sango", "sag", "sg"},
	{"Sanskrit", "san", "sa"},
	{"Santali", "sat", ""},
	{"Sardinian", "srd", "sc"},
	{"Sasak", "sas", ""},
	{"Saxon, Low; German, Low; Low Saxon; Low German", "nds", ""},
	{"Scots", "sco", ""},
	{"Scottish Gaelic; Gaelic", "gla", "gd"},
	{"Selkup", "sel", ""},
	{"Semitic (Other)", "sem", ""},
	{"Serbian", "scc", "sr"},
	{"", "srp", "sr"},
	{"Serer", "srr", ""},
	{"Shan", "shn", ""},
	{"Shona", "sna", "sn"},
	{"Sichuan Yi", "iii", "ii"},
	{"Sidamo", "sid", ""},
	{"Sign languages", "sgn", ""},
	{"Siksika", "bla", ""},
	{"Sindhi", "snd", "sd"},
	{"Sinhalese", "sin", "si"},
	{"Sino-Tibetan (Other)", "sit", ""},
	{"Siouan languages", "sio", ""},
	{"Skolt Sami", "sms", ""},
	{"Slave (Athapascan)", "den", ""},
	{"Slavic (Other)", "sla", ""},
	{"Slovak", "slo", "sk"},
	{"", "slk", "sk"},
	{"Slovenian", "slv", "sl"},
	{"Sogdian", "sog", ""},
	{"Somali", "som", "so"},
	{"Songhai", "son", ""},
	{"Soninke", "snk", ""},
	{"Sorbian languages", "wen", ""},
	{"Sotho, Northern", "nso", ""},
	{"Sotho, Southern", "sot", "st"},
	{"South American Indian (Other)", "sai", ""},
	{"Southern Sami", "sma", ""},
	{"South Ndebele", "nbl", "nr"},
	{"Spanish; Castilian", "spa", "es"},
	{"Sukuma", "suk", ""},
	{"Sumerian", "sux", ""},
	{"Sundanese", "sun", "su"},
	{"Susu", "sus", ""},
	{"Swahili", "swa", "sw"},
	{"Swati", "ssw", "ss"},
	{"Swedish", "swe", "sv"},
	{"Syriac", "syr", ""},
	{"Tagalog", "tgl", "tl"},
	{"Tahitian", "tah", "ty"},
	{"Tai (Other)", "tai", ""},
	{"Tajik", "tgk", "tg"},
	{"Tamashek", "tmh", ""},
	{"Tamil", "tam", "ta"},
	{"Tatar", "tat", "tt"},
	{"Telugu", "tel", "te"},
	{"Tereno", "ter", ""},
	{"Tetum", "tet", ""},
	{"Thai", "tha", "th"},
	{"Tibetan", "tib", "bo"},
	{"", "bod", "bo"},
	{"Tigre", "tig", ""},
	{"Tigrinya", "tir", "ti"},
	{"Timne", "tem", ""},
	{"Tiv", "tiv", ""},
	{"Tlingit", "tli", ""},
	{"Tok Pisin", "tpi", ""},
	{"Tokelau", "tkl", ""},
	{"Tonga (Nyasa)", "tog", ""},
	{"Tonga (Tonga Islands)", "ton", "to"},
	{"Tsimshian", "tsi", ""},
	{"Tsonga", "tso", "ts"},
	{"Tswana", "tsn", "tn"},
	{"Tumbuka", "tum", ""},
	{"Tupi languages", "tup", ""},
	{"Turkish", "tur", "tr"},
	{"Turkish, Ottoman (1500-1928)", "ota", ""},
	{"Turkmen", "tuk", "tk"},
	{"Tuvalu", "tvl", ""},
	{"Tuvinian", "tyv", ""},
	{"Twi", "twi", "tw"},
	{"Ugaritic", "uga", ""},
	{"Uighur", "uig", "ug"},
	{"Ukrainian", "ukr", "uk"},
	{"Umbundu", "umb", ""},
	{"Undetermined", "und", ""},
	{"Urdu", "urd", "ur"},
	{"Uzbek", "uzb", "uz"},
	{"Vai", "vai", ""},
	{"Venda", "ven", "ve"},
	{"Vietnamese", "vie", "vi"},
	{"Volapük", "vol", "vo"},
	{"Votic", "vot", ""},
	{"Wakashan languages", "wak", ""},
	{"Walamo", "wal", ""},
	{"Walloon", "wln", "wa"},
	{"Waray", "war", ""},
	{"Washo", "was", ""},
	{"Welsh", "wel", "cy"},
	{"", "cym", "cy"},
	{"Wolof", "wol", "wo"},
	{"Xhosa", "xho", "xh"},
	{"Yakut", "sah", ""},
	{"Yao", "yao", ""},
	{"Yapese", "yap", ""},
	{"Yiddish", "yid", "yi"},
	{"Yoruba", "yor", "yo"},
	{"Yupik languages", "ypk", ""},
	{"Zande", "znd", ""},
	{"Zapotec", "zap", ""},
	{"Zenaga", "zen", ""},
	{"Zhuang; Chuang", "zha", "za"},
	{"Zulu", "zul", "zu"},
	{"Zuni", "zun", ""},
	{"", "", ""}
};

CString ISO6391ToLanguage(LPCSTR code)
{
	CHAR tmp[2+1];
	strncpy(tmp, code, 2);
	tmp[2] = 0;
	_strlwr(tmp);
	for(int i = 0, j = sizeof(s_isolangs)/sizeof(s_isolangs[0]); i < j; i++)
		if(!strcmp(s_isolangs[i].iso6391, tmp))
		{
			CString ret = CString(CStringA(s_isolangs[i].name));
			int i = ret.Find(';');
			if(i > 0) ret = ret.Left(i);
			return ret;
		}
	return(_T(""));
}

CString ISO6392ToLanguage(LPCSTR code)
{
	CHAR tmp[3+1];
	strncpy(tmp, code, 3);
	tmp[3] = 0;
	_strlwr(tmp);
	for(int i = 0, j = sizeof(s_isolangs)/sizeof(s_isolangs[0]); i < j; i++)
		if(!strcmp(s_isolangs[i].iso6392, tmp))
		{
			CString ret = CString(CStringA(s_isolangs[i].name));
			int i = ret.Find(';');
			if(i > 0) ret = ret.Left(i);
			return ret;
		}
	return(_T(""));
}
