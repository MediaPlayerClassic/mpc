#include "stdafx.h"
#include "DirectVobSubFilter.h"
#include "DirectVobSubInputPin.h"

CDirectVobSubInputPin::CDirectVobSubInputPin(CDirectVobSubFilter* pFilter, HRESULT* phr) 
	: CTransformInputPin(NAME("CDirectVobSubInputPin"), pFilter, phr, L"XForm In")
	, m_pFilter(pFilter)
{
}

STDMETHODIMP CDirectVobSubInputPin::Disconnect()
{
	if(m_pFilter->m_pOutput && m_pFilter->m_pOutput->IsConnected())
		m_pFilter->m_pOutput->Disconnect();

	return CTransformInputPin::Disconnect();
}

STDMETHODIMP CDirectVobSubInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
    CheckPointer(ppAllocator, E_POINTER);

    if(m_pAllocator == NULL)
	{
        m_pAllocator = (IMemAllocator*)&m_pFilter->m_Allocator;
        m_pAllocator->AddRef();
    }

    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;

    return NOERROR;
} 

STDMETHODIMP CDirectVobSubInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly)
{
	HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
	if(FAILED(hr)) return hr;

	m_pFilter->m_fUsingOwnAllocator = (pAllocator == (IMemAllocator*)&m_pFilter->m_Allocator);

	return NOERROR;
}

STDMETHODIMP CDirectVobSubInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cObjectLock(m_pLock);
/*
	if(m_Connected)
	{
		CMediaType mt(*pmt);

		CStdioFile  f;
		f.Open(_T("c:\\vsfilter.txt"), CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite|CFile::typeText|CFile::osWriteThrough);
		f.Seek(0, CFile::end);
		f.WriteString(_T("QueryAccept\n"));
		BITMAPINFOHEADER bih;
		CString str;
		ExtractBIH(&m_mt, &bih);
		str.Format(_T("in: %dx%d %dbpp 0x%08x %d\n"), 
			bih.biWidth, bih.biHeight, bih.biBitCount, bih.biCompression, bih.biSizeImage);
		f.WriteString(str);
		ExtractBIH(pmt, &bih);
		str.Format(_T("in (new): %dx%d %dbpp 0x%08x %d\n"), 
			bih.biWidth, bih.biHeight, bih.biBitCount, bih.biCompression, bih.biSizeImage);
		f.WriteString(str);
		ExtractBIH(&m_pFilter->m_pOutput->CurrentMediaType(), &bih);
		str.Format(_T("out: %dx%d %dbpp 0x%08x %d\n"), 
			bih.biWidth, bih.biHeight, bih.biBitCount, bih.biCompression, bih.biSizeImage);
		f.WriteString(str);
		str.Format(_T("sub: %dx%d %dbpp %d\n"), 
			m_pFilter->m_spd.w, m_pFilter->m_spd.h, m_pFilter->m_spd.bpp, m_pFilter->m_spd.type);
		f.WriteString(str);
	}
*/
	return __super::QueryAccept(pmt);
}

STDMETHODIMP CDirectVobSubInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cObjectLock(m_pLock);

	if(m_Connected)
	{
		CMediaType mt(*pmt);

		BITMAPINFOHEADER bih, bihCur;
		ExtractBIH(&mt, &bih);
		ExtractBIH(&m_mt, &bihCur);

		// HACK: for the intervideo filter, when it tries to change the pitch from 720 to 704...
		if(bihCur.biWidth != bih.biWidth  && bihCur.biHeight == bih.biHeight)
		{
			return S_OK;
		}

		return (CheckMediaType(&mt) != S_OK || SetMediaType(&mt) != S_OK)
			? VFW_E_TYPE_NOT_ACCEPTED
			: S_OK;

		// TODO: send ReceiveConnection downstream
	}

	return __super::ReceiveConnection(pConnector, pmt);
}
