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

bool COggFile::Sync(HANDLE hBreak)
{
	__int64 pos = m_pos;

	DWORD dw;
	for(__int64 i = 0, j = hBreak ? m_len - m_pos : 65536;
		i < j && S_OK == Read((BYTE*)&dw, sizeof(dw)) 
			&& ((i&0xffff) || !hBreak || WaitForSingleObject(hBreak, 0) != WAIT_OBJECT_0); 
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

bool COggFile::Read(OggPageHeader& hdr, HANDLE hBreak)
{
	return Sync(hBreak) && S_OK == Read((BYTE*)&hdr, sizeof(hdr)) && hdr.capture_pattern == 'SggO';
}

bool COggFile::Read(OggPage& page, bool fFull, HANDLE hBreak)
{
	memset(&page.m_hdr, 0, sizeof(page.m_hdr));
	page.m_lens.RemoveAll();
	page.SetSize(0);

	if(!Read(page.m_hdr, hBreak))
		return(false);

	int pagelen = 0, packetlen = 0;
	for(BYTE i = 0; i < page.m_hdr.number_page_segments; i++)
	{
		BYTE b;
        if(S_OK != Read(&b, 1)) return(false);
		packetlen += b;
		if(1/*b < 0xff*/) {page.m_lens.AddTail(packetlen); pagelen += packetlen; packetlen = 0;}
	}

	if(fFull)
	{
		page.SetSize(pagelen);
		if(S_OK != Read(page.GetData(), page.GetSize())) 
			return(false);
	}
	else
	{
		Seek(GetPos()+pagelen);
		page.m_lens.RemoveAll();
	}

	return(true);
}
