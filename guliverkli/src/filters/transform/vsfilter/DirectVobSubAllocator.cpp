#include "stdafx.h"
#include "DirectVobSubAllocator.h"
#include "..\..\..\DSUtil\DSUtil.h"

CDirectVobSubAllocator::CDirectVobSubAllocator(CBaseFilter* pFilter, HRESULT* phr) :
    CMemAllocator(NAME("CDirectVobSubAllocator"), NULL, phr),
    m_pFilter(pFilter),
	m_fMediaTypeChanged(false)
{
	ASSERT(phr);
	ASSERT(pFilter);
}

#ifdef DEBUG
CDirectVobSubAllocator::~CDirectVobSubAllocator()
{
    ASSERT(m_bCommitted == FALSE);
}
#endif

STDMETHODIMP CDirectVobSubAllocator::GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	if(!m_bCommitted)
        return VFW_E_NOT_COMMITTED;

	if(m_fMediaTypeChanged)
	{
		BITMAPINFOHEADER bih;
		ExtractBIH(&m_mt, &bih);

		ALLOCATOR_PROPERTIES Properties, Actual;
		if(FAILED(GetProperties(&Properties))) return E_FAIL;

		int biSizeImage = (bih.biWidth*abs(bih.biHeight)*bih.biBitCount)>>3;

		if(bih.biSizeImage < biSizeImage)
		{
			// bugus intervideo mpeg2 decoder doesn't seem to adjust biSizeImage to the really needed buffer size
			bih.biSizeImage = biSizeImage;
		}

		if(Properties.cbBuffer < bih.biSizeImage || !m_bCommitted)
		{
			Properties.cbBuffer = bih.biSizeImage;
			if(FAILED(Decommit())) return E_FAIL;
			if(FAILED(SetProperties(&Properties, &Actual))) return E_FAIL;
			if(FAILED(Commit())) return E_FAIL;
			ASSERT(Actual.cbBuffer >= Properties.cbBuffer);
			if(Actual.cbBuffer < Properties.cbBuffer) return E_FAIL;
		}
	}

	HRESULT hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if(m_fMediaTypeChanged && SUCCEEDED(hr))
	{
		(*ppBuffer)->SetMediaType(&m_mt);
		m_fMediaTypeChanged = false;
	}

	return hr;
}

void CDirectVobSubAllocator::NotifyMediaType(CMediaType mt)
{
    m_mt = mt;
	m_fMediaTypeChanged = true;
}
