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
#include "DirectVobSubFilter.h"
#include "TextInputPin.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\matroska\matroska.h"

// our first format id
#define __GAB1__ "GAB1"

// our tags for __GAB1__ (ushort) + size (ushort)

// "lang" + '0'
#define __GAB1_LANGUAGE__ 0
// (int)start+(int)stop+(char*)line+'0'
#define __GAB1_ENTRY__ 1
// L"lang" + '0'
#define __GAB1_LANGUAGE_UNICODE__ 2
// (int)start+(int)stop+(WCHAR*)line+'0'
#define __GAB1_ENTRY_UNICODE__ 3

// same as __GAB1__, but the size is (uint) and only __GAB1_LANGUAGE_UNICODE__ is valid
#define __GAB2__ "GAB2"

// (BYTE*)
#define __GAB1_RAWTEXTSUBTITLE__ 4

CTextInputPin::CTextInputPin(CDirectVobSubFilter* pFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr)
	: CBaseInputPin(NAME("CTextInputPin"), pFilter, pLock, phr, L"Input")
	, m_pFilter(pFilter)
	, m_pSubLock(pSubLock)
{
}

HRESULT CTextInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Text && pmt->subtype == MEDIASUBTYPE_NULL
		|| pmt->majortype == MEDIATYPE_Subtitle && pmt->subtype == MEDIASUBTYPE_UTF8
		|| pmt->majortype == MEDIATYPE_Subtitle && (pmt->subtype == MEDIASUBTYPE_SSA || pmt->subtype == MEDIASUBTYPE_ASS)
		? S_OK 
		: E_FAIL;
}

HRESULT CTextInputPin::CompleteConnect(IPin* pReceivePin)
{
	if(!(m_pSubStream = new CRenderedTextSubtitle(m_pSubLock))) return E_FAIL;

	CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

	if(m_mt.majortype == MEDIATYPE_Text)
	{
		pRTS->m_name = GetPinName(pReceivePin) + _T(" (embeded)");
	}
	else if(m_mt.majortype == MEDIATYPE_Subtitle)
	{
		SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;

		CStringA name(psi->IsoLang, 3);
		name.Trim();
		pRTS->m_name.Empty();
		if(name != "") pRTS->m_name += name + ' ';
		pRTS->m_name += _T("(embeded)");

		if((m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS) && psi->dwOffset > 0)
		{
			pRTS->Open(m_mt.pbFormat + psi->dwOffset, m_mt.cbFormat - psi->dwOffset, DEFAULT_CHARSET, pRTS->m_name);
		}
	}

	pRTS->CreateDefaultStyle(DEFAULT_CHARSET);

	m_pFilter->AddSubStream(m_pSubStream);

    HRESULT hr = m_pFilter->CompleteConnect(PINDIR_INPUT, pReceivePin);
    if(FAILED(hr)) 
		return hr;

    return CBaseInputPin::CompleteConnect(pReceivePin);
}

HRESULT CTextInputPin::BreakConnect()
{
	m_pFilter->RemoveSubStream(m_pSubStream);
	m_pSubStream = NULL;

	ASSERT(IsStopped());

	m_pFilter->BreakConnect(PINDIR_INPUT);

    return CBaseInputPin::BreakConnect();
}

STDMETHODIMP CTextInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	{
		CAutoLock cAutoLock(m_pSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;
		pRTS->RemoveAll();
		pRTS->CreateSegments();
	}

	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CTextInputPin::Receive(IMediaSample* pSample)
{
	HRESULT hr;

	hr = CBaseInputPin::Receive(pSample);
    if(FAILED(hr)) return hr;

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);
	tStart += m_tStart; 
	tStop += m_tStart;

	BYTE* pData = NULL;
    hr = pSample->GetPointer(&pData);
    if(FAILED(hr) || pData == NULL) return hr;

	int len = pSample->GetActualDataLength();

	bool fInvalidate = false;

	if(m_mt.majortype == MEDIATYPE_Text)
	{
		CAutoLock cAutoLock(m_pSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

		if(!strncmp((char*)pData, __GAB1__, strlen(__GAB1__)))
		{
			char* ptr = (char*)&pData[strlen(__GAB1__)+1];
			char* end = (char*)&pData[len];

			while(ptr < end)
			{
				WORD tag = *((WORD*)(ptr)); ptr += 2;
				WORD size = *((WORD*)(ptr)); ptr += 2;

				if(tag == __GAB1_LANGUAGE__)
				{
					pRTS->m_name = CString(ptr);
				}
				else if(tag == __GAB1_ENTRY__)
				{
					pRTS->Add(AToW(&ptr[8]), false, *(int*)ptr, *(int*)(ptr+4));
					fInvalidate = true;
				}
				else if(tag == __GAB1_LANGUAGE_UNICODE__)
				{
					pRTS->m_name = (WCHAR*)ptr;
				}
				else if(tag == __GAB1_ENTRY_UNICODE__)
				{
					pRTS->Add((WCHAR*)(ptr+8), true, *(int*)ptr, *(int*)(ptr+4));
					fInvalidate = true;
				}

				ptr += size;
			}
		}
		else if(!strncmp((char*)pData, __GAB2__, strlen(__GAB2__)))
		{
			char* ptr = (char*)&pData[strlen(__GAB2__)+1];
			char* end = (char*)&pData[len];

			while(ptr < end)
			{
				WORD tag = *((WORD*)(ptr)); ptr += 2;
				DWORD size = *((DWORD*)(ptr)); ptr += 4;

				if(tag == __GAB1_LANGUAGE_UNICODE__)
				{
					pRTS->m_name = (WCHAR*)ptr;
				}
				else if(tag == __GAB1_RAWTEXTSUBTITLE__)
				{
					pRTS->Open((BYTE*)ptr, size, DEFAULT_CHARSET, pRTS->m_name);
					fInvalidate = true;
				}

				ptr += size;
			}
		}
		else if(pData != 0 && len > 1 && *pData != 0)
		{
			CStringA str((char*)pData, len);

			str.Replace("\r\n", "\n");
			str.Trim();

			str.Replace("<i>", "{\\i1}");
			str.Replace("</i>", "{\\i}");
			str.Replace("<b>", "{\\b1}");
			str.Replace("</b>", "{\\b}");
			str.Replace("<u>", "{\\u1}");
			str.Replace("</u>", "{\\u}");

			if(!str.IsEmpty())
			{
				pRTS->Add(AToW(str), false, (int)(tStart / 10000), (int)(tStop / 10000));
				fInvalidate = true;

				m_pFilter->Post_EC_OLE_EVENT((char*)pData, (DWORD_PTR)(ISubStream*)m_pSubStream);
			}
		}
	}
	else if(m_mt.majortype == MEDIATYPE_Subtitle)
	{
		CAutoLock cAutoLock(m_pSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

		if(m_mt.subtype == MEDIASUBTYPE_UTF8)
		{
			CStringW str = UTF8To16(CStringA((LPCSTR)pData, len)).Trim();
			if(!str.IsEmpty())
			{
				pRTS->Add(str, true, (int)(tStart / 10000), (int)(tStop / 10000));
				fInvalidate = true;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS)
		{
			CStringW str = UTF8To16(CStringA((LPCSTR)pData, len)).Trim();
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
					pRTS->Add(stse.str, true, (int)(tStart / 10000), (int)(tStop / 10000), 
						stse.style, stse.actor, stse.effect, stse.marginRect, stse.layer, stse.readorder);
					fInvalidate = true;
				}
			}
		}
	}

	if(fInvalidate)
	{
		// IMPORTANT: m_pSubLock must not be locked when calling this
		m_pFilter->InvalidateSubtitle(tStart, (DWORD_PTR)(ISubStream*)m_pSubStream);
	}

    return S_OK;
}
