#include "stdafx.h"
#include <windows.h>
#include <commdlg.h>
#include "mplayerc.h"
#include "mainfrm.h"
#include "textpassthrufilter.h"

#define __GAB1_LANGUAGE_UNICODE__ 2
#define __GAB1_RAWTEXTSUBTITLE__ 4
#define __GAB2__ "GAB2"

CTextPassThruFilter::CTextPassThruFilter(CMainFrame* pMainFrame, CSimpleTextSubtitle* pSTS, CCritSec* pSubLock) 
	: CTransformFilter(NAME("CTextPassThruFilter"), NULL, __uuidof(this))
	, m_pMainFrame(pMainFrame)
	, m_pSTS(pSTS)
	, m_pSubLock(pSubLock)
{
	m_rtOffset = 0;
}

CTextPassThruFilter::~CTextPassThruFilter()
{
}

void CTextPassThruFilter::SetName()
{
	if(CComQIPtr<IPropertyBag> pPB = m_pInput->GetConnected())
	{
		CComVariant var;
		if(SUCCEEDED(pPB->Read(L"LANGUAGE", &var, NULL)))
		{
			CAutoLock cAutoLock(m_pSubLock);
			m_pSTS->m_name = CString(var.bstrVal) + _T(" (embeded)");
		}
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

	REFERENCE_TIME rtStart, rtStop;
	if(SUCCEEDED(pIn->GetTime(&rtStart, &rtStop)))
	{
		int tstart = (int)((rtStart + m_rtOffset) / 10000);
		int tstop = (int)((rtStop + m_rtOffset) / 10000);

		CStringA str = (char*)pDataIn;

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
					CAutoLock cAutoLock(m_pSubLock);
					m_pSTS->Open((BYTE*)ptr, size, DEFAULT_CHARSET, name);
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

			if(!str.IsEmpty() && rtStart < rtStop && m_pSTS)
			{
				CAutoLock cAutoLock(m_pSubLock);

				SetName();

				m_pSTS->Add(AToW(str), false, tstart, tstop);
				m_pMainFrame->InvalidateSubtitle(rtStart + m_rtOffset, (DWORD_PTR)m_pSTS);
			}
		}
	}

	return S_OK;
}

HRESULT CTextPassThruFilter::CheckInputType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_Text) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTextPassThruFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return (SUCCEEDED(CheckInputType(mtIn)) && SUCCEEDED(CheckInputType(mtOut))) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
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

HRESULT CTextPassThruFilter::GetMediaType(int iPosition, CMediaType* pMediaType)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CopyMediaType(pMediaType, &m_pInput->CurrentMediaType());

	return S_OK;
}

HRESULT CTextPassThruFilter::CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin)
{
	SetName();

	return __super::CompleteConnect(dir, pReceivePin);
}

HRESULT CTextPassThruFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_rtOffset = tStart;

	SetName();

	return __super::NewSegment(tStart, tStop, dRate);
}
