/* 
 *	Copyright (C) 2003-2004 Gabest
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

int CDSMMuxerFilter::GetByteLength(UINT64 data, int min)
{
	int i = 7;
	while(i >= min && ((BYTE*)&data)[i] == 0) i--;
	return ++i;
}

void CDSMMuxerFilter::WritePacketHeader(dsmp_t type, UINT64 len)
{
	ASSERT(type < 32);

	int i = GetByteLength(len);

	m_pStream->BitWrite(DSMSW, DSMSW_SIZE<<3);
	m_pStream->BitWrite(type, 5);
	m_pStream->BitWrite(i-1, 3);
	m_pStream->BitWrite(len, i<<3);
}

void CDSMMuxerFilter::WriteHeader()
{
	WritePacketHeader(DSMP_FILE, 0);

	POSITION pos = m_pPins.GetHeadPosition();
	while(pos)
	{
		CBaseMuxerInputPin* pPin = m_pPins.GetNext(pos);
		const CMediaType& mt = pPin->CurrentMediaType();

		ASSERT((mt.lSampleSize >> 30) == 0); // you don't need >1GB samples, do you?

		WritePacketHeader(DSMP_MEDIATYPE, 5 + sizeof(GUID)*3 + mt.FormatLength());
		m_pStream->BitWrite(pPin->GetID(), 8);
		m_pStream->ByteWrite(&mt.majortype, sizeof(mt.majortype));
		m_pStream->ByteWrite(&mt.subtype, sizeof(mt.subtype));
		m_pStream->BitWrite(mt.bFixedSizeSamples, 1);
		m_pStream->BitWrite(mt.bTemporalCompression, 1);
		m_pStream->BitWrite(mt.lSampleSize, 30);
		m_pStream->ByteWrite(&mt.formattype, sizeof(mt.formattype));
		m_pStream->ByteWrite(mt.Format(), mt.FormatLength());
	}

	m_sps.RemoveAll();
	m_isps.RemoveAll();
}

void CDSMMuxerFilter::WritePacket(Packet* pPacket)
{
	if(pPacket->IsEOS())
		return;

	ASSERT(!pPacket->IsSyncPoint() || pPacket->IsTimeValid());

	REFERENCE_TIME rtTimeStamp = _I64_MIN, rtDuration = 0;
	int iTimeStamp = 0, iDuration = 0;

	if(pPacket->IsTimeValid())
	{
		rtTimeStamp = pPacket->rtStart;
		rtDuration = max(pPacket->rtStop - pPacket->rtStart, 0);

		iTimeStamp = GetByteLength(myabs(rtTimeStamp), 0);
		ASSERT(iTimeStamp <= 7);

		iDuration = GetByteLength(rtDuration, 0);
		ASSERT(iDuration <= 7);

		IndexSyncPoint(pPacket);
	}

	int len = 2 + iTimeStamp + iDuration + pPacket->pData.GetCount(); // id + flags + data 

	WritePacketHeader(DSMP_SAMPLE, len);
	m_pStream->BitWrite(pPacket->pPin->GetID(), 8);
	m_pStream->BitWrite(pPacket->IsSyncPoint(), 1);
	m_pStream->BitWrite(rtTimeStamp < 0, 1);
	m_pStream->BitWrite(iTimeStamp, 3);
	m_pStream->BitWrite(iDuration, 3);
	m_pStream->BitWrite(myabs(rtTimeStamp), iTimeStamp<<3);
	m_pStream->BitWrite(rtDuration, iDuration<<3);
	m_pStream->ByteWrite(pPacket->pData.GetData(), pPacket->pData.GetCount());
}

void CDSMMuxerFilter::WriteFooter()
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

		len += 1 + GetByteLength(myabs(rt), 0) + GetByteLength(fp, 0); // flags + rt + fp
	}

	WritePacketHeader(DSMP_SYNCPOINTS, len);

	pos = isps.GetHeadPosition();
	while(pos)
	{
		SyncPoint& sp = isps.GetNext(pos);

		int irt = GetByteLength(myabs(sp.rtStart), 0);
		int ifp = GetByteLength(sp.fp, 0);

		m_pStream->BitWrite(sp.rtStart < 0, 1);
		m_pStream->BitWrite(irt, 3);
		m_pStream->BitWrite(ifp, 3);
		m_pStream->BitWrite(0, 1); // reserved
		m_pStream->BitWrite(myabs(sp.rtStart), irt<<3);
		m_pStream->BitWrite(sp.fp, ifp<<3);
	}
}

void CDSMMuxerFilter::IndexSyncPoint(Packet* p)
{
	// TODO: the very last syncpoints won't get moved to m_isps because there are no more syncpoints to trigger it!

	if(!p->IsTimeValid() || !p->IsSyncPoint()) 
		return;

	SyncPoint sp;
	sp.fp = m_pStream->GetPos();
	sp.id = p->pPin->GetID();
	sp.rtStart = p->rtStart;
	sp.rtStop = p->pPin->IsSubtitleStream() ? p->rtStop : _I64_MAX;

	if(m_isps.IsEmpty()) 
	{
		m_isps.AddTail(sp);
	}
	else if(!m_sps.IsEmpty())
	{
		SyncPoint& head = m_sps.GetHead();
		SyncPoint& tail = m_sps.GetTail();

		if(head.rtStart - m_isps.GetTail().rtStart > 10000000)
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
