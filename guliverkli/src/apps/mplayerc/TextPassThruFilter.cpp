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
#include "textpassthrufilter.h"
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
	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	pIn->GetPointer(&pDataIn);
	pOut->GetPointer(&pDataOut);

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if(!pDataIn || !pDataOut || len > size || len <= 0) return S_FALSE;

	memcpy(pDataOut, pDataIn, min(len, size));
	pOut->SetActualDataLength(min(len, size));

	CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pRTS;

	REFERENCE_TIME rtStart, rtStop;
	if(SUCCEEDED(pIn->GetTime(&rtStart, &rtStop)))
	{
		int tstart = (int)((rtStart + m_rtOffset) / 10000);
		int tstop = (int)((rtStop + m_rtOffset) / 10000);

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

				str.Replace("<i>", "{\\i1}");
				str.Replace("</i>", "{\\i}");
				str.Replace("<b>", "{\\b1}");
				str.Replace("</b>", "{\\b}");
				str.Replace("<u>", "{\\u1}");
				str.Replace("</u>", "{\\u}");

				if(!str.IsEmpty() && rtStart < rtStop)
				{
					CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);

					SetName();

					pRTS->Add(AToW(str), false, tstart, tstop);
					m_pMainFrame->InvalidateSubtitle((DWORD_PTR)pRTS, rtStart + m_rtOffset);
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
				m_pMainFrame->InvalidateSubtitle((DWORD_PTR)pRTS, rtStart + m_rtOffset);
			}
		}
		// TODO: handle SSA, ASS, USF subtypes
	}

	return S_OK;
}

HRESULT CTextPassThruFilter::CheckInputType(const CMediaType* mtIn)
{
	return mtIn->majortype == MEDIATYPE_Text 
		|| mtIn->majortype == MEDIATYPE_Subtitle && mtIn->subtype == MEDIASUBTYPE_UTF8
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
			
		CMediaType& mt = m_pInput->CurrentMediaType();

		if(mt.majortype == MEDIATYPE_Subtitle)
		{
			CStringA name(((SUBTITLEINFO*)mt.pbFormat)->IsoLang, 3);
			if(name != "")
			{
				CAutoLock cAutoLock(&m_pMainFrame->m_csSubLock);
				pRTS->m_name = name + _T(" (embeded)");
			}
		}
	}

	return __super::CompleteConnect(dir, pReceivePin);
}

HRESULT CTextPassThruFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_rtOffset = tStart;

	CMediaType& mt = m_pInput->CurrentMediaType();

	if(mt.majortype == MEDIATYPE_Text)
	{
		SetName();
	}

	return __super::NewSegment(tStart, tStop, dRate);
}
