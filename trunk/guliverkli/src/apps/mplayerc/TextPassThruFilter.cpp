/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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
#include <windows.h>
#include <commdlg.h>
#include "mplayerc.h"
#include "mainfrm.h"
#include "TextPassThruFilter.h"
#include "..\..\..\include\matroska\matroska.h"
#include "..\..\DSUtil\DSUtil.h"

#define __GAB1_LANGUAGE_UNICODE__ 2
#define __GAB1_RAWTEXTSUBTITLE__ 4
#define __GAB2__ "GAB2"

CTextPassThruFilter::CTextPassThruFilter(CMainFrame* pMainFrame) 
	: CTransformFilter(NAME("CTextPassThruFilter"), NULL, __uuidof(this))
	, m_pMainFrame(pMainFrame)
{
	m_rtOffset = 0;
}

CTextPassThruFilter::~CTextPassThruFilter()
{
}

void CTextPassThruFilter::SetName()
{
	CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pRTS;

	CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);

	if(CComQIPtr<IPropertyBag> pPB = m_pInput->GetConnected())
	{
		CComVariant var;
		if(SUCCEEDED(pPB->Read(L"LANGUAGE", &var, NULL)))
		{
			pRTS->m_name = CString(var.bstrVal) + _T(" (embeded)");
		}
	}

	if(pRTS->m_name.IsEmpty())
	{
		CPinInfo pi;
		m_pInput->GetConnected()->QueryPinInfo(&pi);
		pRTS->m_name = CString(CStringW(pi.achName)) + _T(" (embeded)");

	}
}

HRESULT CTextPassThruFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	CAutoLock cAutoLock(&m_csReceive);

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	pIn->GetPointer(&pDataIn);
	pOut->GetPointer(&pDataOut);

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if(!pDataIn || !pDataOut || len > size || len <= 0) 
		return S_FALSE;

	memcpy(pDataOut, pDataIn, min(len, size));
	pOut->SetActualDataLength(min(len, size));

	CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pRTS;

	bool fInvalidate = false;

	REFERENCE_TIME rtStart, rtStop;
	if(SUCCEEDED(pIn->GetTime(&rtStart, &rtStop)))
	{
		if(rtStart <= 0 && rtStop <= 0)
		{
			rtStart = 0; rtStop = 1;
			pOut->SetTime(&rtStart, &rtStop);
			pOut->SetPreroll(1);
			return S_OK;
		}

		rtStart += m_rtOffset;
		rtStop += m_rtOffset;

		int tstart = (int)(rtStart / 10000);
		int tstop = (int)(rtStop / 10000);

		CMediaType& mt = m_pInput->CurrentMediaType();

		if(mt.majortype == MEDIATYPE_Text)
		{
			CStringA str((char*)pDataIn, len);

			if(str == __GAB2__ && len > (long)strlen(__GAB2__)+1)
			{
				BYTE* ptr = pDataIn + strlen(__GAB2__)+1;
				BYTE* end = pDataIn + len;

				CString name = _T("(embeded)");

				while(ptr < end)
				{
					WORD tag = *(WORD*)ptr; ptr += 2;
					DWORD size = *(DWORD*)ptr; ptr += 4;

					if(tag == __GAB1_LANGUAGE_UNICODE__)
					{
						name = CString((WCHAR*)ptr) + _T("(embeded)");
					}
					else if(tag == __GAB1_RAWTEXTSUBTITLE__)
					{
						CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);
						pRTS->Open((BYTE*)ptr, size, DEFAULT_CHARSET, name);
					}

					ptr += size;
				}
			}
			else
			{
				str.Replace("\r\n", "\n");
				str.Trim();

				if(!str.IsEmpty() && rtStart < rtStop)
				{
					CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);

					SetName();

					pRTS->Add(AToW(str), false, tstart, tstop);
					fInvalidate = true;
				}
			}
		}
		else if(mt.majortype == MEDIATYPE_Subtitle && mt.subtype == MEDIASUBTYPE_UTF8)
		{
			CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);
			CStringW str = UTF8To16(CStringA((LPCSTR)pDataIn, len)).Trim();

			if(!str.IsEmpty())
			{
				pRTS->Add(str, true, tstart, tstop);
				fInvalidate = true;
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_SSA)
		{
			CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);
			CStringW str = UTF8To16(CStringA((LPCSTR)pDataIn, len)).Trim();

			if(!str.IsEmpty())
			{
				STSEntry stse;

				str.Replace(L",", L" ,");
				int i = 0, j = 0;
				for(CStringW token = str.Tokenize(L",", i); j < 9 && !token.IsEmpty(); token = ++j < 8 ? str.Tokenize(L",", i) : str.Mid(i))
				{
					token.Trim();

					switch(j)
					{
					case 0: stse.readorder = wcstol(token, NULL, 10); break;
					case 1: stse.layer = wcstol(token, NULL, 10); break;
					case 2: stse.style = token; break;
					case 3: stse.actor = token; break;
					case 4: stse.marginRect.left = wcstol(token, NULL, 10); break;
					case 5: stse.marginRect.right = wcstol(token, NULL, 10); break;
					case 6: stse.marginRect.top = stse.marginRect.bottom = wcstol(token, NULL, 10); break;
					case 7: stse.effect = token; break;
					case 8: token.Replace(L" ,", L","); stse.str = token; break;
					default: break;
					}
				}

				if(j == 9 && !stse.str.IsEmpty())
				{
					pRTS->Add(stse.str, true, tstart, tstop, 
						stse.style, stse.actor, stse.effect, stse.marginRect, 
						stse.layer, stse.readorder);
					fInvalidate = true;
				}
			}
		}

		// TODO: handle SSA, ASS, USF subtypes
	}

	if(fInvalidate)
		m_pMainFrame->InvalidateSubtitle((DWORD_PTR)(ISubStream*)m_pRTS, rtStart);

	return S_OK;
}

HRESULT CTextPassThruFilter::CheckInputType(const CMediaType* mtIn)
{
	return mtIn->majortype == MEDIATYPE_Text 
		|| mtIn->majortype == MEDIATYPE_Subtitle && mtIn->subtype == MEDIASUBTYPE_UTF8
		|| mtIn->majortype == MEDIATYPE_Subtitle && (mtIn->subtype == MEDIASUBTYPE_SSA || mtIn->subtype == MEDIASUBTYPE_ASS)
		? S_OK 
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTextPassThruFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn)) && SUCCEEDED(CheckInputType(mtOut))
		? S_OK 
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTextPassThruFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CComPtr<IMemAllocator> pAllocatorIn;
	m_pInput->GetAllocator(&pAllocatorIn);
	if(!pAllocatorIn) return E_UNEXPECTED;

	pAllocatorIn->GetProperties(pProperties);

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR);
}

HRESULT CTextPassThruFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 1) return VFW_S_NO_MORE_ITEMS;

	if(iPosition == 0)
	{
		*pmt = m_pInput->CurrentMediaType();
	}
	else if(iPosition == 1)
	{
        pmt->InitMediaType();
		pmt->majortype = MEDIATYPE_Text;
		pmt->subtype = MEDIASUBTYPE_NULL;
		pmt->formattype = FORMAT_None;
	}

	return S_OK;
}

HRESULT CTextPassThruFilter::BreakConnect(PIN_DIRECTION dir)
{
	if(dir == PINDIR_INPUT)
	{
		m_pRTS = NULL;
	}

	return __super::BreakConnect(dir);
}

HRESULT CTextPassThruFilter::CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin)
{
	if(dir == PINDIR_INPUT)
	{
		if(!(m_pRTS = new CRenderedTextSubtitle(&m_pMainFrame->m_csSubLock)))
			return E_OUTOFMEMORY;

		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pRTS;

		pRTS->CreateDefaultStyle(DEFAULT_CHARSET);
		pRTS->m_dstScreenSize = CSize(384, 288);

		SetName();
			
		CMediaType mt = m_pInput->CurrentMediaType();

		if(mt.majortype == MEDIATYPE_Subtitle)
		{
			SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.pbFormat;
			DWORD dwOffset = psi->dwOffset;

			CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);

			pRTS->m_name = ISO6392ToLanguage(psi->IsoLang);
			if(pRTS->m_name.IsEmpty()) pRTS->m_name = _T("English");

			if((mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS) && dwOffset > 0)
			{
				if(mt.pbFormat[dwOffset+0] != 0xef
				&& mt.pbFormat[dwOffset+1] != 0xbb
				&& mt.pbFormat[dwOffset+2] != 0xfb)
				{
					dwOffset -= 3;
					mt.pbFormat[dwOffset+0] = 0xef;
					mt.pbFormat[dwOffset+1] = 0xbb;
					mt.pbFormat[dwOffset+2] = 0xbf;
				}
				pRTS->Open(mt.pbFormat + dwOffset, mt.cbFormat - dwOffset, DEFAULT_CHARSET, pRTS->m_name);
			}
		}
	}

	return __super::CompleteConnect(dir, pReceivePin);
}

HRESULT CTextPassThruFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	m_rtOffset = tStart;

	CMediaType& mt = m_pInput->CurrentMediaType();

	if(mt.majortype == MEDIATYPE_Text)
	{
		SetName();
	}

	{
		CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pRTS;
		pRTS->RemoveAll();
		pRTS->CreateSegments();
	}

	return __super::NewSegment(tStart, tStop, dRate);
}
