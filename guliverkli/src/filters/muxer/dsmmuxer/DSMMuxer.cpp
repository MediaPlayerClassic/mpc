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
#include "DSMMuxer.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
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

CDSMMuxerFilter::CDSMMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseMuxerFilter(pUnk, phr, __uuidof(this))
{
	if(phr) *phr = S_OK;
}

CDSMMuxerFilter::~CDSMMuxerFilter()
{
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

	// TODO: write chapters
}

void CDSMMuxerFilter::MuxPacket(IBitStream* pBS, MuxerPacket* pPacket)
{
	if(pPacket->IsEOS())
		return;

	if(pPacket->pPin->CurrentMediaType().majortype == MEDIATYPE_Text)
	{
		//
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

		// TODO
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
	int len = 0;
	CList<SyncPoint> isps;
	REFERENCE_TIME rtPrev = 0, rt;
	UINT64 fpPrev = 0, fp;

	POSITION pos = m_isps.GetHeadPosition();
	while(pos)
	{
		SyncPoint& sp = m_isps.GetNext(pos);

		rt = sp.rtStart - rtPrev; rtPrev = sp.rtStart;
		fp = sp.fp - fpPrev; fpPrev = sp.fp;

		SyncPoint sp2;
		sp2.fp = fp;
		sp2.rtStart = rt;
		isps.AddTail(sp2);

		len += 1 + GetByteLength(myabs(rt)) + GetByteLength(fp); // flags + rt + fp
	}

	MuxPacketHeader(pBS, DSMP_SYNCPOINTS, len);

	pos = isps.GetHeadPosition();
	while(pos)
	{
		SyncPoint& sp = isps.GetNext(pos);

		int irt = GetByteLength(myabs(sp.rtStart));
		int ifp = GetByteLength(sp.fp);

		pBS->BitWrite(sp.rtStart < 0, 1);
		pBS->BitWrite(irt, 3);
		pBS->BitWrite(ifp, 3);
		pBS->BitWrite(0, 1); // reserved
		pBS->BitWrite(myabs(sp.rtStart), irt<<3);
		pBS->BitWrite(sp.fp, ifp<<3);
	}
}

void CDSMMuxerFilter::IndexSyncPoint(MuxerPacket* p, __int64 fp)
{
	// FIXME: the very last syncpoints won't get moved to m_isps because there are no more syncpoints to trigger it!

	if(fp < 0 || !p || !p->IsTimeValid() || !p->IsSyncPoint()) 
		return;

	ASSERT(p->rtStart >= m_rtPrevSyncPoint);
	m_rtPrevSyncPoint = p->rtStart;

	SyncPoint sp;
	sp.fp = fp;
	sp.id = p->pPin->GetID();
	sp.rtStart = p->rtStart;

	if(m_isps.IsEmpty()) 
	{
		m_isps.AddTail(sp);
	}
	else if(!m_sps.IsEmpty())
	{
		SyncPoint& head = m_sps.GetHead();
		SyncPoint& tail = m_sps.GetTail();

		if(head.rtStart - m_isps.GetTail().rtStart > 1000000) // 100ms limit, just in case every stream had only keyframes, then sycnpoints would be too frequent
		{
			SyncPoint sp;
			sp.fp = head.fp;
			sp.rtStart = tail.rtStart;
			m_isps.AddTail(sp);
		}
	}

	POSITION pos = m_sps.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		SyncPoint& sp2 = m_sps.GetNext(pos);
		if(sp2.id == sp.id || sp2.rtStop <= sp.rtStart)
			m_sps.RemoveAt(cur);
	}

	m_sps.AddTail(sp);
}
