#include "StdAfx.h"
#include ".\oggfile.h"

COggFile::COggFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr)
{
	if(FAILED(hr)) return;

	hr = Init();
}

HRESULT COggFile::Init()
{
	Seek(0);
	if(!Sync()) return E_FAIL;

	return S_OK;
}

bool COggFile::Sync()
{
	__int64 pos = m_pos;

	DWORD dw;
	for(int i = 0; // 64k is the max page size usually, ... rfc's suggestion
		i < 65536 && S_OK == __super::Read((BYTE*)&dw, sizeof(dw)); 
		i++, m_pos = pos + i)
	{
		if(dw == 'SggO')
		{
			m_pos = pos + i;
			return(true);
		}
	}

	m_pos = pos;

	return(false);
}

bool COggFile::Read(OggPageHeader& hdr)
{
	return Sync() && S_OK == __super::Read((BYTE*)&hdr, sizeof(hdr)) && hdr.capture_pattern == 'SggO';
}

bool COggFile::Read(OggPage& page, bool fFull)
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
        if(S_OK != __super::Read(&b, 1)) return(false);
		packetlen += b;
		if(1/*b < 0xff*/) {page.m_lens.AddTail(packetlen); pagelen += packetlen; packetlen = 0;}
	}

	if(fFull)
	{
		page.SetSize(pagelen);
		if(S_OK != __super::Read(page.GetData(), page.GetSize())) 
			return(false);
	}
	else
	{
		Seek(GetPos()+pagelen);
		page.m_lens.RemoveAll();
	}

	return(true);
}
