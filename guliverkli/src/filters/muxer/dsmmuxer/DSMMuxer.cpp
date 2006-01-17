/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include <mmreg.h>
#include "DSMMuxer.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include <qnetwork.h>
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_DirectShowMedia}
};

const AMOVIESETUP_PIN sudpPins[] =
{
    {L"Input", FALSE, FALSE, FALSE, TRUE, &CLSID_NULL, NULL, 0, NULL},
    {L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CDSMMuxerFilter), L"DSM Muxer", MERIT_DO_NOT_USE, countof(sudpPins), sudpPins}
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDSMMuxerFilter>, NULL, &sudFilter[0]}
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, dwReason, 0); // "DllMain" of the dshow baseclasses;
}

#endif

template<typename T> static T myabs(T n) {return n >= 0 ? n : -n;}

static int GetByteLength(UINT64 data, int min = 0)
{
	int i = 7;
	while(i >= min && ((BYTE*)&data)[i] == 0) i--;
	return ++i;
}

//
// CDSMMuxerFilter
//

CDSMMuxerFilter::CDSMMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, bool fAutoChap, bool fAutoRes)
	: CBaseMuxerFilter(pUnk, phr, __uuidof(this))
	, m_fAutoChap(fAutoChap)
	, m_fAutoRes(fAutoRes)
	, m_bOutputRawStreams(FALSE)
{
	if(phr) *phr = S_OK;
}

CDSMMuxerFilter::~CDSMMuxerFilter()
{
}

STDMETHODIMP CDSMMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(IDSMMuxerFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CDSMMuxerFilter::MuxPacketHeader(IBitStream* pBS, dsmp_t type, UINT64 len)
{
	ASSERT(type < 32);

	int i = GetByteLength(len, 1);

	pBS->BitWrite(DSMSW, DSMSW_SIZE<<3);
	pBS->BitWrite(type, 5);
	pBS->BitWrite(i-1, 3);
	pBS->BitWrite(len, i<<3);
}

void CDSMMuxerFilter::MuxFileInfo(IBitStream* pBS)
{
	int len = 1;
	CSimpleMap<CStringA, CStringA> si;

	for(int i = 0; i < GetSize(); i++)
	{
		CStringA key = CStringA(CString(GetKeyAt(i))), value = UTF16To8(GetValueAt(i));
		if(key.GetLength() != 4) continue;
		si.Add(key, value);
		len += 4 + value.GetLength() + 1;
	}

	MuxPacketHeader(pBS, DSMP_FILEINFO, len);
	pBS->BitWrite(DSMF_VERSION, 8);
	for(int i = 0; i < si.GetSize(); i++)
	{
		CStringA key = si.GetKeyAt(i), value = si.GetValueAt(i);
		pBS->ByteWrite((LPCSTR)key, 4);
		pBS->ByteWrite((LPCSTR)value, value.GetLength()+1);
	}

}

void CDSMMuxerFilter::MuxStreamInfo(IBitStream* pBS, CBaseMuxerInputPin* pPin)
{
	int len = 1;
	CSimpleMap<CStringA, CStringA> si;

	for(int i = 0; i < pPin->GetSize(); i++)
	{
		CStringA key = CStringA(CString(pPin->GetKeyAt(i))), value = UTF16To8(pPin->GetValueAt(i));
		if(key.GetLength() != 4) continue;
		si.Add(key, value);
		len += 4 + value.GetLength() + 1;
	}

	if(len > 1)
	{
		MuxPacketHeader(pBS, DSMP_STREAMINFO, len);
		pBS->BitWrite(pPin->GetID(), 8);
		for(int i = 0; i < si.GetSize(); i++)
		{
			CStringA key = si.GetKeyAt(i), value = si.GetValueAt(i);
			pBS->ByteWrite((LPCSTR)key, 4);
			pBS->ByteWrite((LPCSTR)value, value.GetLength()+1);
		}
	}
}

void CDSMMuxerFilter::MuxInit()
{
	m_sps.RemoveAll();
	m_isps.RemoveAll();
	m_rtPrevSyncPoint = _I64_MIN;
}

void CDSMMuxerFilter::MuxHeader(IBitStream* pBS)
{
	CString muxer;
	muxer.Format(_T("DSM Muxer (%s)"), CString(__TIMESTAMP__));

	SetProperty(L"MUXR", CStringW(muxer));
	SetProperty(L"DATE", CStringW(CTime::GetCurrentTime().FormatGmt(_T("%Y-%m-%d %H:%M:%S"))));

	MuxFileInfo(pBS);

	POSITION pos = m_pPins.GetHeadPosition();
	while(pos)
	{
		CBaseMuxerInputPin* pPin = m_pPins.GetNext(pos);
		const CMediaType& mt = pPin->CurrentMediaType();

		ASSERT((mt.lSampleSize >> 30) == 0); // you don't need >1GB samples, do you?

		MuxPacketHeader(pBS, DSMP_MEDIATYPE, 5 + sizeof(GUID)*3 + mt.FormatLength());
		pBS->BitWrite(pPin->GetID(), 8);
		pBS->ByteWrite(&mt.majortype, sizeof(mt.majortype));
		pBS->ByteWrite(&mt.subtype, sizeof(mt.subtype));
		pBS->BitWrite(mt.bFixedSizeSamples, 1);
		pBS->BitWrite(mt.bTemporalCompression, 1);
		pBS->BitWrite(mt.lSampleSize, 30);
		pBS->ByteWrite(&mt.formattype, sizeof(mt.formattype));
		pBS->ByteWrite(mt.Format(), mt.FormatLength());
		
		MuxStreamInfo(pBS, pPin);
	}

	// resources & chapters

	CInterfaceList<IDSMResourceBag> pRBs;
	pRBs.AddTail(this);

	CComQIPtr<IDSMChapterBag> pCB = (IUnknown*)(INonDelegatingUnknown*)this;

	pos = m_pPins.GetHeadPosition();
	while(pos)
	{
		for(CComPtr<IPin> pPin = m_pPins.GetNext(pos)->GetConnected(); pPin; pPin = GetUpStreamPin(GetFilterFromPin(pPin)))
		{
			if(m_fAutoRes)
			{
				CComQIPtr<IDSMResourceBag> pPB = GetFilterFromPin(pPin);
				if(pPB && !pRBs.Find(pPB)) pRBs.AddTail(pPB);
			}

			if(m_fAutoChap)
			{
				if(!pCB || pCB->ChapGetCount() == 0) pCB = GetFilterFromPin(pPin);				
			}
		}
	}

	// resources

	pos = pRBs.GetHeadPosition();
	while(pos)
	{
		IDSMResourceBag* pRB = pRBs.GetNext(pos);

		for(DWORD i = 0, j = pRB->ResGetCount(); i < j; i++)
		{
			CComBSTR name, desc, mime;
			BYTE* pData = NULL;
			DWORD len = 0;
			if(SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, NULL)))
			{
				CStringA utf8_name = UTF16To8(name);
				CStringA utf8_desc = UTF16To8(desc);
				CStringA utf8_mime = UTF16To8(mime);

				MuxPacketHeader(pBS, DSMP_RESOURCE, 
					1 + 
					utf8_name.GetLength()+1 + 
					utf8_desc.GetLength()+1 + 
					utf8_mime.GetLength()+1 + 
					len);

				pBS->BitWrite(0, 2);
				pBS->BitWrite(0, 6); // reserved
				pBS->ByteWrite(utf8_name, utf8_name.GetLength()+1);
				pBS->ByteWrite(utf8_desc, utf8_desc.GetLength()+1);
				pBS->ByteWrite(utf8_mime, utf8_mime.GetLength()+1);
				pBS->ByteWrite(pData, len);

				CoTaskMemFree(pData);
			}
		}
	}

	// chapters

	if(pCB)
	{
		CList<CDSMChapter> chapters;
		REFERENCE_TIME rtPrev = 0;
		int len = 0;

		pCB->ChapSort();

		for(DWORD i = 0; i < pCB->ChapGetCount(); i++)
		{
			CDSMChapter c;
			CComBSTR name;
			if(SUCCEEDED(pCB->ChapGet(i, &c.rt, &name)))
			{
				REFERENCE_TIME rtDiff = c.rt - rtPrev; rtPrev = c.rt; c.rt = rtDiff;
				c.name = name;
				len += 1 + GetByteLength(myabs(c.rt)) + UTF16To8(c.name).GetLength()+1;
				chapters.AddTail(c);
			}
		}

		if(chapters.GetCount())
		{
			MuxPacketHeader(pBS, DSMP_CHAPTERS, len);

			pos = chapters.GetHeadPosition();
			while(pos)
			{
				CDSMChapter& c = chapters.GetNext(pos);
				CStringA name = UTF16To8(c.name);
				int irt = GetByteLength(myabs(c.rt));
				pBS->BitWrite(c.rt < 0, 1);
				pBS->BitWrite(irt, 3);
				pBS->BitWrite(0, 4);
				pBS->BitWrite(myabs(c.rt), irt<<3);
				pBS->ByteWrite((LPCSTR)name, name.GetLength()+1);
			}
		}
	}
}

void CDSMMuxerFilter::MuxPacket(IBitStream* pBS, const MuxerPacket* pPacket)
{
	if(pPacket->IsEOS())
		return;

	if(pPacket->pPin->CurrentMediaType().majortype == MEDIATYPE_Text)
	{
		CStringA str((char*)pPacket->pData.GetData(), pPacket->pData.GetCount());
		str.Replace("\xff", " ");
		str.Replace("&nbsp;", " ");
		str.Replace("&nbsp", " ");
		str.Trim();
		if(str.IsEmpty())
			return;
	}

	ASSERT(!pPacket->IsSyncPoint() || pPacket->IsTimeValid());

	REFERENCE_TIME rtTimeStamp = _I64_MIN, rtDuration = 0;
	int iTimeStamp = 0, iDuration = 0;

	if(pPacket->IsTimeValid())
	{
		rtTimeStamp = pPacket->rtStart;
		rtDuration = max(pPacket->rtStop - pPacket->rtStart, 0);

		iTimeStamp = GetByteLength(myabs(rtTimeStamp));
		ASSERT(iTimeStamp <= 7);

		iDuration = GetByteLength(rtDuration);
		ASSERT(iDuration <= 7);

		IndexSyncPoint(pPacket, pBS->GetPos());
	}

	int len = 2 + iTimeStamp + iDuration + pPacket->pData.GetCount(); // id + flags + data 

	MuxPacketHeader(pBS, DSMP_SAMPLE, len);
	pBS->BitWrite(pPacket->pPin->GetID(), 8);
	pBS->BitWrite(pPacket->IsSyncPoint(), 1);
	pBS->BitWrite(rtTimeStamp < 0, 1);
	pBS->BitWrite(iTimeStamp, 3);
	pBS->BitWrite(iDuration, 3);
	pBS->BitWrite(myabs(rtTimeStamp), iTimeStamp<<3);
	pBS->BitWrite(rtDuration, iDuration<<3);
	pBS->ByteWrite(pPacket->pData.GetData(), pPacket->pData.GetCount());
}

void CDSMMuxerFilter::MuxFooter(IBitStream* pBS)
{
	// syncpoints

	int len = 0;
	CList<IndexedSyncPoint> isps;
	REFERENCE_TIME rtPrev = 0, rt;
	UINT64 fpPrev = 0, fp;

	POSITION pos = m_isps.GetHeadPosition();
	while(pos)
	{
		IndexedSyncPoint& isp = m_isps.GetNext(pos);
		TRACE(_T("sp[%d]: %I64d %I64x\n"), isp.id, isp.rt, isp.fp);

		rt = isp.rt - rtPrev; rtPrev = isp.rt;
		fp = isp.fp - fpPrev; fpPrev = isp.fp;

		IndexedSyncPoint isp2;
		isp2.fp = fp;
		isp2.rt = rt;
		isps.AddTail(isp2);

		len += 1 + GetByteLength(myabs(rt)) + GetByteLength(fp); // flags + rt + fp
	}

	MuxPacketHeader(pBS, DSMP_SYNCPOINTS, len);

	pos = isps.GetHeadPosition();
	while(pos)
	{
		IndexedSyncPoint& isp = isps.GetNext(pos);

		int irt = GetByteLength(myabs(isp.rt));
		int ifp = GetByteLength(isp.fp);

		pBS->BitWrite(isp.rt < 0, 1);
		pBS->BitWrite(irt, 3);
		pBS->BitWrite(ifp, 3);
		pBS->BitWrite(0, 1); // reserved
		pBS->BitWrite(myabs(isp.rt), irt<<3);
		pBS->BitWrite(isp.fp, ifp<<3);
	}
}

void CDSMMuxerFilter::IndexSyncPoint(const MuxerPacket* p, __int64 fp)
{
	// Yes, this is as complicated as it looks.
	// Rule #1: don't write this packet if you can't do it reliably. 
	// (think about overlapped subtitles, line1: 0->10, line2: 1->9)

	// FIXME: the very last syncpoints won't get moved to m_isps because there are no more syncpoints to trigger it!

	if(fp < 0 || !p || !p->IsTimeValid() || !p->IsSyncPoint()) 
		return;

	ASSERT(p->rtStart >= m_rtPrevSyncPoint);
	m_rtPrevSyncPoint = p->rtStart;

	SyncPoint sp;
	sp.id = p->pPin->GetID();
	sp.rtStart = p->rtStart;
	sp.rtStop = p->pPin->IsSubtitleStream() ? p->rtStop : _I64_MAX;
	sp.fp = fp;

	{
		SyncPoint& head = !m_sps.IsEmpty() ? m_sps.GetHead() : sp;
		SyncPoint& tail = !m_sps.IsEmpty() ? m_sps.GetTail() : sp;
		REFERENCE_TIME rtfp = !m_isps.IsEmpty() ? m_isps.GetTail().rtfp : _I64_MIN;

		if(head.rtStart > rtfp + 1000000) // 100ms limit, just in case every stream had only keyframes, then sycnpoints would be too frequent
		{
			IndexedSyncPoint isp;
			isp.id = head.id;
			isp.rt = tail.rtStart;
			isp.rtfp = head.rtStart;
			isp.fp = head.fp;
			m_isps.AddTail(isp);
		}
	}

	POSITION pos = m_sps.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		SyncPoint& sp2 = m_sps.GetNext(pos);
		if(sp2.id == sp.id && sp2.rtStop <= sp.rtStop || sp2.rtStop <= sp.rtStart)
			m_sps.RemoveAt(cur);
	}

	m_sps.AddTail(sp);
}

// IDSMMuxerFilter

STDMETHODIMP CDSMMuxerFilter::SetOutputRawStreams(BOOL b)
{
	CAutoLock cAutoLock(m_pLock);

	if(m_State != State_Stopped)
		return VFW_E_NOT_STOPPED;

	m_bOutputRawStreams = b;

	return S_OK;
}

//

void CDSMMuxerFilter::MuxHeader()
{
	CBaseMuxerOutputPin* pPin = GetOutputPin();

	if(!m_bOutputRawStreams || !pPin)
		return;

	CComQIPtr<IFileSinkFilter> pFSF = GetFilterFromPin(pPin->GetConnected());

	LPWSTR pfn = NULL;
	if(!pFSF || FAILED(pFSF->GetCurFile(&pfn, NULL)))
		return;

	CPathW path(pfn);
	path.RemoveExtension();

	CoTaskMemFree(pfn);

	POSITION pos = m_pPins.GetHeadPosition();
	for(int i = 1; pos; i++)
	{
		CBaseMuxerInputPin* pPin = m_pPins.GetNext(pos);

		const CMediaType& mt = pPin->CurrentMediaType();

		CString ext;
		if(mt.subtype == MEDIASUBTYPE_AAC) ext = _T("aac");
		else if(mt.subtype == MEDIASUBTYPE_MP3) ext = _T("mp3");
		else if(mt.subtype == FOURCCMap(WAVE_FORMAT_MPEG)) ext = _T("m1a");
		else if(mt.subtype == MEDIASUBTYPE_MPEG2_AUDIO) ext = _T("m2a");
		else if(mt.subtype == FOURCCMap(WAVE_FORMAT_DOLBY_AC3_SPDIF) || mt.subtype == MEDIASUBTYPE_DOLBY_AC3) ext = _T("ac3");
		else if(mt.subtype == MEDIASUBTYPE_WAVE_DTS || mt.subtype == MEDIASUBTYPE_DTS) ext = _T("dts");
		else if((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO) ext = _T("h264");
		else if(mt.subtype == FOURCCMap('GEPJ') || mt.subtype == FOURCCMap('gepj')) ext = _T("jpg");
		else if(mt.majortype == MEDIATYPE_Video && mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO) ext = _T("m2v");
		else if(mt.majortype == MEDIATYPE_Video && mt.subtype == MEDIASUBTYPE_MPEG1Payload) ext = _T("m1v");
		// TODO
		else if(mt.subtype == MEDIASUBTYPE_UTF8 || mt.majortype == MEDIATYPE_Text) ext = _T("srt");
		else if(mt.subtype == MEDIASUBTYPE_SSA) ext = _T("ssa");
		else if(mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2) ext = _T("ass");

		CString fn;
		fn.Format(_T("%s.%d"), CString((LPCWSTR)path), i);

		if(!ext.IsEmpty()) fn += '.' + ext;

		FILE* f = _tfopen(fn, _T("wb"));
		if(!f) continue;

		file_t file = {f, -1};
		m_pPinToFile[pPin] = file;

		if((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO)
		{
			MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.Format();

			for(DWORD i = 0; i < vih->cbSequenceHeader-2; i += 2)
			{
				DWORD sync = 0x01000000;
				fwrite(&sync, 1, 4, f);
				WORD size = (((BYTE*)vih->dwSequenceHeader)[i+0]<<8) | ((BYTE*)vih->dwSequenceHeader)[i+1];
				fwrite(&((BYTE*)vih->dwSequenceHeader)[i+2], 1, size, f);
				i += size;
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_UTF8)
		{
			BYTE bom[3] = {0xef, 0xbb, 0xbf};
			fwrite(bom, 1, 3, f);
		}
		else if(mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2)
		{
			SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();
			BYTE* p = (BYTE*)si + si->dwOffset;

			BYTE bom[3] = {0xef, 0xbb, 0xbf};
			if(memcmp(bom, p, 3) != 0) fwrite(bom, 1, 3, f);

			CStringA str((char*)p, mt.FormatLength() - (p - mt.Format()));
			str.Replace("\r", "");
			str.Replace("\n", "\r\n");
			fprintf(f, "%s\r\n", str);

			if(str.Find("[Events]") < 0) fprintf(f, "\r\n\r\n[Events]\r\n");
		}
	}
}

void CDSMMuxerFilter::MuxPacket(const MuxerPacket* pPacket)
{
	if(CAtlMap<CBaseMuxerInputPin*, file_t>::CPair* pPair = m_pPinToFile.Lookup(pPacket->pPin))
	{
		int iPacket = ++pPair->m_value.iPacket;
		FILE* f = pPair->m_value.pFile;
		const BYTE* pData = pPacket->pData.GetData();

		const CMediaType& mt = pPacket->pPin->CurrentMediaType();

		if(mt.subtype == MEDIASUBTYPE_AAC && mt.formattype == FORMAT_WaveFormatEx)
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

			int profile = 0;

			int srate_idx = 11;
			if(92017 <= wfe->nSamplesPerSec) srate_idx = 0;
			else if(75132 <= wfe->nSamplesPerSec) srate_idx = 1;
			else if(55426 <= wfe->nSamplesPerSec) srate_idx = 2;
			else if(46009 <= wfe->nSamplesPerSec) srate_idx = 3;
			else if(37566 <= wfe->nSamplesPerSec) srate_idx = 4;
			else if(27713 <= wfe->nSamplesPerSec) srate_idx = 5;
			else if(23004 <= wfe->nSamplesPerSec) srate_idx = 6;
			else if(18783 <= wfe->nSamplesPerSec) srate_idx = 7;
			else if(13856 <= wfe->nSamplesPerSec) srate_idx = 8;
			else if(11502 <= wfe->nSamplesPerSec) srate_idx = 9;
			else if(9391 <= wfe->nSamplesPerSec) srate_idx = 10;

			int channels = wfe->nChannels;

			if(wfe->cbSize >= 2)
			{
				BYTE* p = (BYTE*)(wfe+1);
				profile = (p[0]>>3)-1;
				srate_idx = ((p[0]&7)<<1)|((p[1]&0x80)>>7);
				channels = (p[1]>>3)&15;
			}

			int len = (pPacket->pData.GetSize() + 7) & 0x1fff;

			BYTE hdr[7] = {0xff, 0xf9};
			hdr[2] = (profile<<6) | (srate_idx<<2) | ((channels&4)>>2);
			hdr[3] = ((channels&3)<<6) | (len>>11);
			hdr[4] = (len>>3)&0xff;
			hdr[5] = ((len&7)<<5) | 0x1f;
			hdr[6] = 0xfc;

			fwrite(hdr, 1, 7, f);
		}
		else if((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO)
		{
			const BYTE* p = pData;
			int i = pPacket->pData.GetSize();

			while(i >= 4)
			{
				DWORD len = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];

				i -= len + 4;
				p += len + 4;
			}

			if(i == 0)
			{
				p = pData;
				i = pPacket->pData.GetSize();

				while(i >= 4)
				{
					DWORD len = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];

					DWORD sync = 0x01000000;
					fwrite(&sync, 1, 4, f);

					p += 4; 
					i -= 4;

					if(len > i || len == 1) {len = i; ASSERT(0);}

					fwrite(p, 1, len, f);

					p += len;
					i -= len;
				}

				return;
			}
		}
		else if(mt.subtype == MEDIASUBTYPE_UTF8 || mt.majortype == MEDIATYPE_Text)
		{
			CStringA str((char*)pData, pPacket->pData.GetSize());
			str.Trim();
			if(str.IsEmpty()) return;

			DVD_HMSF_TIMECODE start = RT2HMSF(pPacket->rtStart, 25);
			DVD_HMSF_TIMECODE stop = RT2HMSF(pPacket->rtStop, 25);

			fprintf(f, "%d\r\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\r\n%s\r\n\r\n", 
				iPacket+1,
				start.bHours, start.bMinutes, start.bSeconds, (int)((pPacket->rtStart/10000)%1000), 
				stop.bHours, stop.bMinutes, stop.bSeconds, (int)((pPacket->rtStop/10000)%1000),
				str);

			return;
		}
		else if(mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2)
		{
			CStringA str((char*)pData, pPacket->pData.GetSize());
			str.Trim();
			if(str.IsEmpty()) return;

			DVD_HMSF_TIMECODE start = RT2HMSF(pPacket->rtStart, 25);
			DVD_HMSF_TIMECODE stop = RT2HMSF(pPacket->rtStop, 25);

			int fields = mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

			CList<CStringA> sl;
			Explode(str, sl, ',', fields);
			if(sl.GetCount() < fields) return;

			CStringA readorder = sl.RemoveHead(); // TODO
			CStringA layer = sl.RemoveHead();
			CStringA style = sl.RemoveHead();
			CStringA actor = sl.RemoveHead();
			CStringA left = sl.RemoveHead();
			CStringA right = sl.RemoveHead();
			CStringA top = sl.RemoveHead();
			if(fields == 10) top += ',' + sl.RemoveHead(); // bottom
			CStringA effect = sl.RemoveHead();
			str = sl.RemoveHead();

			if(mt.subtype == MEDIASUBTYPE_SSA) layer = "Marked=0";

			fprintf(f, "Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%s,%s,%s,%s,%s\r\n",
				layer,
				start.bHours, start.bMinutes, start.bSeconds, (int)((pPacket->rtStart/100000)%100), 
				stop.bHours, stop.bMinutes, stop.bSeconds, (int)((pPacket->rtStop/100000)%100),
				style, actor, left, right, top, effect, str);

			return;
		}
		// else // TODO: restore more streams

		fwrite(pData, 1, pPacket->pData.GetSize(), f);
	}
}

void CDSMMuxerFilter::MuxFooter()
{
	POSITION pos = m_pPinToFile.GetStartPosition();
	while(pos) fclose(m_pPinToFile.GetNextValue(pos).pFile);
	m_pPinToFile.RemoveAll();
}
