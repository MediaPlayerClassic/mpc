#include "StdAfx.h"
#include ".\oggfile.h"

COggFile::COggFile(IAsyncReader* pReader, HRESULT& hr)
	: m_pReader(pReader)
	, m_pos(0), m_len(0)
{
	LONGLONG total = 0, available;
	pReader->Length(&total, &available);
	m_len = total;

	hr = Init();
}

COggFile::~COggFile()
{
}

HRESULT COggFile::Init()
{
	if(!m_pReader) return E_UNEXPECTED;

	Seek(0);
	if(!Sync()) return E_FAIL;

	return S_OK;
}

HRESULT COggFile::Read(BYTE* pData, LONG len)
{
	HRESULT hr = m_pReader->SyncRead(m_pos, len, pData);
	m_pos += len;
	return hr;
}

bool COggFile::Sync()
{
	DWORD dw;
	for(__int64 i = m_pos, j = m_pos + 65536; // 64k is the max page size
		i < j && S_OK == m_pReader->SyncRead(i, sizeof(dw), (BYTE*)&dw); 
		i++)
	{
		if(dw == 'SggO')
		{
			m_pos = i;
			return(true);
		}
	}

	return(false);
}

bool COggFile::Read(OggPageHeader& hdr)
{
	return Sync() && S_OK == Read((BYTE*)&hdr, sizeof(hdr)) && hdr.capture_pattern == 'SggO';
}

bool COggFile::Read(OggPage& page)
{
	memset(&page.m_hdr, 0, sizeof(page.m_hdr));
	page.m_lens.RemoveAll();
	page.SetSize(0);

	if(!Read(page.m_hdr))
		return(false);

	int pagelen = 0, packetlen = 0;
	for(BYTE i = 0; i < page.m_hdr.number_page_segments; i++)
	{
		BYTE b;
        if(S_OK != Read(&b, 1)) return(false);
		packetlen += b;
		if(1/*b < 0xff*/) {page.m_lens.AddTail(packetlen); pagelen += packetlen; packetlen = 0;}
	}
	page.SetSize(pagelen);
	if(S_OK != Read(page.GetData(), page.GetSize())) 
		return(false);

	return(true);
}
