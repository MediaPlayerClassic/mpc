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

#include "StdAfx.h"
#include "OggSplitter.h"

#include <initguid.h>
#include "..\..\..\..\include\ogg\OggDS.h"
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      countof(sudPinTypesIn), // Number of types
      sudPinTypesIn         // Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      0,					// Number of types
      NULL					// Pin information
    },
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(COggSplitterFilter), L"Ogg Splitter", MERIT_NORMAL+1, countof(sudpPins), sudpPins},
	{&__uuidof(COggSourceFilter), L"Ogg Source", MERIT_NORMAL+1, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
	{L"Ogg Splitter", &__uuidof(COggSplitterFilter), COggSplitterFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"Ogg Source", &__uuidof(COggSourceFilter), COggSourceFilter::CreateInstance, NULL, &sudFilter[1]},
};

int g_cTemplates = countof(g_Templates);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		CLSID_AsyncReader, 
		MEDIASUBTYPE_Ogg, 
		_T("0,4,,4F676753"), // OggS
		_T(".ogg"), _T(".ogm"), NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Ogg);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI COggSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new COggSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CUnknown* WINAPI COggSourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new COggSourceFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// COggSplitterFilter
//

COggSplitterFilter::COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("COggSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

COggSplitterFilter::~COggSplitterFilter()
{
}

HRESULT COggSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	if(m_pOutputs.GetCount() > 0) return VFW_E_ALREADY_CONNECTED;

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pPinMap.RemoveAll();

	m_pFile.Attach(new COggFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = 0;

	m_rtDuration = 0;

	m_pFile->Seek(0);
	OggPage page;
	for(int i = 0, nWaitForMore = 0; m_pFile->Read(page); i++)
	{
		BYTE* p = page.GetData();

		BYTE type = *p++;
		if(!(type&1) && nWaitForMore == 0)
			break;

		CStringW name;
		name.Format(L"Stream %d", i);

		HRESULT hr;

		if(type == 1 && (page.m_hdr.header_type_flag & OggPageHeader::first))
		{
			CAutoPtr<CBaseSplitterOutputPin> pPinOut;

			if(!memcmp(p, "vorbis", 6))
			{
				name.Format(L"Vorbis %d", i);
				pPinOut.Attach(new COggVorbisOutputPin((OggVorbisIdHeader*)(p+6), name, this, this, &hr));
				nWaitForMore++;
			}
			else if(!memcmp(p, "video", 5))
			{
				name.Format(L"Video %d", i);
				pPinOut.Attach(new COggVideoOutputPin((OggStreamHeader*)p, name, this, this, &hr));
			}
			else if(!memcmp(p, "audio", 5))
			{
				name.Format(L"Audio %d", i);
				pPinOut.Attach(new COggAudioOutputPin((OggStreamHeader*)p, name, this, this, &hr));
			}
			else if(!memcmp(p, "text", 4))
			{
				name.Format(L"Text %d", i);
				pPinOut.Attach(new COggTextOutputPin((OggStreamHeader*)p, name, this, this, &hr));
			}
			else if(!memcmp(p, "Direct Show Samples embedded in Ogg", 35))
			{
				name.Format(L"DirectShow %d", i);
				pPinOut.Attach(new COggDirectShowOutputPin((AM_MEDIA_TYPE*)(p+35+sizeof(GUID)), name, this, this, &hr));
			}

			if(pPinOut)
			{
				m_pPinMap[page.m_hdr.bitstream_serial_number] = pPinOut;
				m_pOutputs.AddTail(pPinOut);
			}
		}

		if(type&1)
		{
			CBaseSplitterOutputPin* pPin = NULL;
			COggVorbisOutputPin* pOggVorbisPin = NULL;
			if(m_pPinMap.Lookup(page.m_hdr.bitstream_serial_number, pPin)
			&& (pOggVorbisPin = dynamic_cast<COggVorbisOutputPin*>(pPin)))
			{
				pOggVorbisPin->UnpackInitPage(page);
				if(pOggVorbisPin->IsInitialized()) nWaitForMore--;
			}
		}
	}

	// m_rtDuration = // TODO

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration;

	// TODO
/*
	SetMediaContentStr(, Title);
	SetMediaContentStr(, AuthorName);
	SetMediaContentStr(, Copyright);
	SetMediaContentStr(, Description);
*/

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool COggSplitterFilter::InitDeliverLoop()
{
	if(!m_pFile) return(false);

	// reindex if needed


	return(true);
}

void COggSplitterFilter::SeekDeliverLoop(REFERENCE_TIME rt)
{
	if(rt <= 0)
	{
		m_pFile->Seek(0);
	}
	else
	{
	}
}

void COggSplitterFilter::DoDeliverLoop()
{
	HRESULT hr = S_OK;

	// TODO: implement seeking
	m_pFile->Seek(0);
	REFERENCE_TIME rt = 0;

	OggPage page;
	while(m_pFile->Read(page) && SUCCEEDED(hr) && !CheckRequest(NULL))
	{
		CBaseSplitterOutputPin* pPin = NULL;
		COggSplitterOutputPin* pOggPin = NULL;
		if(!m_pPinMap.Lookup(page.m_hdr.bitstream_serial_number, pPin)
		|| !(pOggPin = dynamic_cast<COggSplitterOutputPin*>(pPin)))
		{
			ASSERT(0);
			continue;
		}

		if(!pOggPin->IsConnected())
			continue;

		if(FAILED(hr = pOggPin->UnpackPage(page))) 
			break;

		CAutoPtr<Packet> p;
		while((p = pOggPin->GetPacket()) && SUCCEEDED(hr = DeliverPacket(p)) && !CheckRequest(NULL));
	}
}

// IMediaSeeking

STDMETHODIMP COggSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	*pDuration = m_rtDuration;

	return S_OK;
}

//
// COggSourceFilter
//

COggSourceFilter::COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: COggSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// COggSplitterOutputPin
//

COggSplitterOutputPin::COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(pName, pFilter, pLock, phr)
{
}

HRESULT COggSplitterOutputPin::UnpackPage(OggPage& page)
{
	/*
	// TODO
	ASSERT(m_prev_page_sequence_number == page.m_hdr.page_sequence_number);
	page.m_hdr.page_sequence_number = page.m_hdr.page_sequence_number;
	*/

	POSITION first = page.m_lens.GetHeadPosition();
	while(first && page.m_lens.GetAt(first) == 255) page.m_lens.GetNext(first);
	if(!first) first = page.m_lens.GetTailPosition();

	POSITION last = page.m_lens.GetTailPosition();
	while(last && page.m_lens.GetAt(last) == 255) page.m_lens.GetPrev(last);
	if(!last) last = page.m_lens.GetTailPosition();

	BYTE* pData = page.GetData();

	int i = 0, j = 0, len = 0;

    for(POSITION pos = page.m_lens.GetHeadPosition(); pos; page.m_lens.GetNext(pos))
	{
		len = page.m_lens.GetAt(pos);
		j += len;

		if(len < 255 || pos == page.m_lens.GetTailPosition())
		{
			if(first == pos && (page.m_hdr.header_type_flag & OggPageHeader::continued))
			{
				ASSERT(m_lastpacket);

				if(m_lastpacket)
				{
					int size = m_lastpacket->pData.GetSize();
					m_lastpacket->pData.SetSize(size + j-i);
					memcpy(m_lastpacket->pData.GetData() + size, pData + i, j-i);

					CAutoLock csAutoLock(&m_csPackets);

					if(len < 255) m_packets.AddTail(m_lastpacket);
				}
			}
			else
			{
				if(last == pos && page.m_hdr.granule_position != -1)
					m_rt = GetRefTime(page.m_hdr.granule_position);

				CAutoPtr<Packet> p(new Packet());

				p->TrackNumber = page.m_hdr.bitstream_serial_number;

				if(S_OK == UnpackPacket(p, pData + i, j-i))
				{
TRACE(_T("[%d]: %d, %I64d -> %I64d\n"), (int)p->TrackNumber, p->pData.GetSize(), p->rtStart, p->rtStop);

					CAutoLock csAutoLock(&m_csPackets);

					m_rt = p->rtStop;

					if(len < 255) m_packets.AddTail(p);
					else m_lastpacket = p;
				}
			}

			i = j;
		}
	}

	return S_OK;
}

CAutoPtr<Packet> COggSplitterOutputPin::GetPacket()
{
	CAutoPtr<Packet> p;
	CAutoLock csAutoLock(&m_csPackets);
	if(m_packets.GetCount()) p = m_packets.RemoveHead();
	return p;
}

HRESULT COggSplitterOutputPin::DeliverEndFlush()
{
	CAutoLock csAutoLock(&m_csPackets);
	m_packets.RemoveAll();
	return __super::DeliverEndFlush();
}

HRESULT COggSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock csAutoLock(&m_csPackets);
	m_rt = tStart;
	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

//
// COggVorbisOutputPin
//

COggVorbisOutputPin::COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_audio_sample_rate = h->audio_sample_rate;
	m_blocksize[0] = 1<<h->blocksize_0;
	m_blocksize[1] = 1<<h->blocksize_1;

	CMediaType mt;
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_Vorbis;
	mt.formattype = FORMAT_VorbisFormat;
	VORBISFORMAT* vf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
	memset(mt.Format(), 0, mt.FormatLength());
	vf->nChannels = h->audio_channels;
	vf->nSamplesPerSec = h->audio_sample_rate;
	vf->nAvgBitsPerSec = h->bitrate_nominal;
	vf->nMinBitsPerSec = h->bitrate_minimum;
	vf->nMaxBitsPerSec = h->bitrate_maximum;
	vf->fQuality = -1;
	mt.SetSampleSize(8192);
	m_mts.Add(mt);
}

REFERENCE_TIME COggVorbisOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = granule_position * 10000000 / m_audio_sample_rate;
	return rt;
}

class bitstream
{
	BYTE* m_p;
	int m_len, m_pos;
public:
	bitstream(BYTE* p, int len, bool startattheend = false) : m_p(p), m_len(len*8) {m_pos = !startattheend ? 0 : len*8;}
	bool hasbits(int cnt)
	{
		int pos = m_pos+cnt;
		return(pos >= 0 && pos < m_len);
	}
	unsigned int showbits(int cnt) // a bit unclean, but works and can read backwards too! :P
	{
		if(!hasbits(cnt)) {ASSERT(0); return 0;}
		unsigned int ret = 0, off = 0;
		BYTE* p = m_p;
		if(cnt < 0)
		{
			p += (m_pos+cnt)>>3;
			off = (m_pos+cnt)&7;
			cnt = abs(cnt);
			ret = (*p++&(~0<<off))>>off; off = 8 - off; cnt -= off;
		}
		else
		{
			p += m_pos>>3;
			off = m_pos&7;
			ret = (*p++>>off)&((1<<min(cnt,8))-1); off = 0; cnt -= 8 - off;
		}
		while(cnt > 0) {ret |= (*p++&((1<<min(cnt,8))-1)) << off; off += 8; cnt -= 8;}
		return ret;
	}
	unsigned int getbits(int cnt)
	{
		unsigned int ret = showbits(cnt);
		m_pos += cnt;
		return ret;
	}
};

HRESULT COggVorbisOutputPin::UnpackInitPage(OggPage& page)
{
	HRESULT hr = __super::UnpackPage(page);

	while(m_packets.GetCount())
	{
		Packet* p = m_packets.GetHead();
		if(p->pData.GetCount() >= 6 && p->pData.GetData()[0] == 0x05)
		{
			bitstream bs(p->pData.GetData(), p->pData.GetCount(), true);
			while(bs.hasbits(-1) && bs.getbits(-1) != 1);
			for(int cnt = 0; bs.hasbits(-8-16-16-1-6); cnt++)
			{
				unsigned int modes = bs.showbits(-6)+1;

				unsigned int mapping = bs.getbits(-8);
				unsigned int transformtype = bs.getbits(-16);
				unsigned int windowtype = bs.getbits(-16);
				unsigned int blockflag = bs.getbits(-1);

				if(transformtype != 0 || windowtype != 0)
				{
					ASSERT(modes == cnt);
					break;
				}

				m_blockflags.InsertAt(0, !!blockflag);
			}
		}

		m_initpackets.AddTail(m_packets.RemoveHead());
	}

	return hr;
}

HRESULT COggVorbisOutputPin::UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len)
{
	if(len > 0 && m_blockflags.GetCount())
	{
		bitstream bs(pData, len);
		if(bs.getbits(1) == 0)
		{
			int x = m_blockflags.GetCount()-1, n = 0;
			while(x) {n++; x >>= 1;}
			DWORD blocksize = m_blocksize[m_blockflags[bs.getbits(n)]?1:0];
			if(m_lastblocksize) m_rt += GetRefTime((m_lastblocksize + blocksize) >> 2);
			m_lastblocksize = blocksize;
		}
	}

	p->bSyncPoint = TRUE;
	p->rtStart = m_rt;
	p->rtStop = m_rt;
	p->pData.SetSize(len);
	memcpy(p->pData.GetData(), pData, len);

	return S_OK;
}

HRESULT COggVorbisOutputPin::DeliverPacket(CAutoPtr<Packet> p)
{
	if(p->pData.GetSize() > 0 && (p->pData.GetData()[0]&1))
		return S_OK;

	return __super::DeliverPacket(p);
}

HRESULT COggVorbisOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);

	m_lastblocksize = 0;

	POSITION pos = m_initpackets.GetHeadPosition();
	while(pos)
	{
		Packet* pi = m_initpackets.GetNext(pos);
		CAutoPtr<Packet> p(new Packet());
		p->TrackNumber = pi->TrackNumber;
		p->bDiscontinuity = p->bSyncPoint = TRUE;
		p->rtStart = p->rtStop = 0;
		p->pData.Copy(pi->pData);
		__super::DeliverPacket(p);
	}

	return hr;
}

//
// COggDirectShowOutputPin
//

COggDirectShowOutputPin::COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	CMediaType mt;
	memcpy((AM_MEDIA_TYPE*)&mt, pmt, FIELD_OFFSET(AM_MEDIA_TYPE, pUnk));
	mt.SetFormat((BYTE*)(pmt+1), pmt->cbFormat);
	mt.SetSampleSize(1);
	if(mt.majortype == MEDIATYPE_Video) // TODO: find samples for audio and find out what to return in GetRefTime...
		m_mts.Add(mt);
}

REFERENCE_TIME COggDirectShowOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = 0; // TODO

	if(m_mt.majortype == MEDIATYPE_Video)
	{
		rt = granule_position * ((VIDEOINFOHEADER*)m_mt.Format())->AvgTimePerFrame;
	}
	else if(m_mt.majortype == MEDIATYPE_Audio)
	{
		rt = granule_position; // ((WAVEFORMATEX*)m_mt.Format())->
	}

	return rt;
}

HRESULT COggDirectShowOutputPin::UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len)
{
	int i = 0;

	BYTE hdr = pData[i++];

	if(!(hdr&1))
	{
		BYTE nLenBytes = (hdr>>6)|((hdr&2)<<1);
		__int64 Length = 0;
		for(int j = 0; j < nLenBytes; j++)
			Length |= (__int64)pData[i++] << (j << 3);

		p->bSyncPoint = !!(hdr&8);
		p->rtStart = m_rt;
		p->rtStop = m_rt + (nLenBytes ? GetRefTime(Length) : GetRefTime(1));
		p->pData.SetSize(len-i);
		memcpy(p->pData.GetData(), &pData[i], len-i);

		return S_OK;
	}

	return S_FALSE;
}

//
// COggStreamOutputPin
//

COggStreamOutputPin::COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_time_unit = h->time_unit;
	m_samples_per_unit = h->samples_per_unit;
	m_default_len = h->default_len;
}

REFERENCE_TIME COggStreamOutputPin::GetRefTime(__int64 granule_position)
{
	return granule_position * m_time_unit / m_samples_per_unit;
}

HRESULT COggStreamOutputPin::UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len)
{
	int i = 0;

	BYTE hdr = pData[i++];

	if(!(hdr&1))
	{
		BYTE nLenBytes = (hdr>>6)|((hdr&2)<<1);
		__int64 Length = 0;
		for(int j = 0; j < nLenBytes; j++)
			Length |= (__int64)pData[i++] << (j << 3);

		p->bSyncPoint = !!(hdr&8);
		p->rtStart = m_rt;
		p->rtStop = m_rt + (nLenBytes ? GetRefTime(Length) : GetRefTime(m_default_len));
		p->pData.SetSize(len-i);
		memcpy(p->pData.GetData(), &pData[i], len-i);

		return S_OK;
	}

	return S_FALSE;
}

//
// COggVideoOutputPin
//

COggVideoOutputPin::COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = FOURCCMap(MAKEFOURCC(h->subtype[0],h->subtype[1],h->subtype[2],h->subtype[3]));
	mt.formattype = FORMAT_VideoInfo;
	VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	memset(mt.Format(), 0, mt.FormatLength());
	pvih->AvgTimePerFrame = h->time_unit / h->samples_per_unit;
	pvih->bmiHeader.biWidth = h->v.w;
	pvih->bmiHeader.biHeight = h->v.h;
	pvih->bmiHeader.biBitCount = (WORD)h->bps;
	pvih->bmiHeader.biCompression = mt.subtype.Data1;
	switch(pvih->bmiHeader.biCompression)
	{
	case BI_RGB: case BI_BITFIELDS: mt.subtype = 
		pvih->bmiHeader.biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
		pvih->bmiHeader.biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
		pvih->bmiHeader.biBitCount == 32 ? MEDIASUBTYPE_RGB32 :
		MEDIASUBTYPE_NULL;
		break;
	case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
	case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
	}
	mt.SetSampleSize(max(h->buffersize, 1));
	m_mts.Add(mt);
}

//
// COggAudioOutputPin
//

COggAudioOutputPin::COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = FOURCCMap(strtol(h->subtype, NULL, 16));
	mt.formattype = FORMAT_WaveFormatEx;
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(mt.Format(), 0, mt.FormatLength());
	wfe->wFormatTag = (WORD)mt.subtype.Data1;
	wfe->nChannels = h->a.nChannels;
	wfe->nSamplesPerSec = (DWORD)(10000000i64 * h->samples_per_unit / h->time_unit);
	wfe->wBitsPerSample = (WORD)h->bps;
	wfe->nAvgBytesPerSec = h->a.nAvgBytesPerSec; // TODO: verify for PCM
	wfe->nBlockAlign = h->a.nBlockAlign; // TODO: verify for PCM
	mt.SetSampleSize(max(h->buffersize, 1));
	m_mts.Add(mt);
}

//
// COggTextOutputPin
//

COggTextOutputPin::COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;
	mt.majortype = MEDIATYPE_Text;
	mt.subtype = MEDIASUBTYPE_NULL;
	mt.formattype = FORMAT_None;
	mt.SetSampleSize(1);
	m_mts.Add(mt);
}
