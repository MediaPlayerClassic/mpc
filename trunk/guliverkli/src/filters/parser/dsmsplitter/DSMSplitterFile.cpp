#include "StdAfx.h"
#include "DSMSplitterFile.h"

CDSMSplitterFile::CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr) 
	: CBaseSplitterFile(pReader, hr)
	, m_rtFirst(0)
	, m_rtDuration(0)
{
	if(FAILED(hr)) return;

	hr = Init();
}

HRESULT CDSMSplitterFile::Init()
{
	Seek(0);

	if(BitRead(DSMSW_SIZE<<3) != DSMSW || BitRead(5) != DSMP_FILE)
		return E_FAIL;

	Seek(0);

	m_mts.RemoveAll();
	m_rtFirst = m_rtDuration = 0;

	dsmp_t type;
	UINT64 len;
	int limit = 65536;

	// examine the beginning of the file ...

	while(Sync(type, len, 0) && GetPos() < limit)
	{
		__int64 pos = GetPos();

		if(type == DSMP_MEDIATYPE)
		{
			BYTE id;
			CMediaType mt;
			if(Read(len, id, mt)) m_mts[id] = mt;
		}
		else if(type == DSMP_SAMPLE)
		{
			Packet p;
			if(Read(len, &p, false) && p.rtStart != Packet::INVALID_TIME)
			{
				m_rtFirst = p.rtStart;
				break;
			}
		}
		else if(type == DSMP_SYNCPOINTS)
		{
			Read(len, m_sps);
		}

		Seek(pos + len);
	}

	if(type != DSMP_SAMPLE)
		return E_FAIL;

	// ... and the end 

	for(int i = 1, j = (int)((GetLength()+limit/2)/limit); i <= j; i++)
	{
		__int64 seekpos = max(0, (__int64)GetLength()-i*limit);
		Seek(seekpos);

		while(Sync(type, len, limit) && GetPos() < seekpos+limit)
		{
			__int64 pos = GetPos();

			if(type == DSMP_SYNCPOINTS)
			{
				Read(len, m_sps);
			}
			else if(type == DSMP_SAMPLE)
			{
				Packet p;
				if(Read(len, &p, false) && p.rtStart != Packet::INVALID_TIME)
				{
					m_rtDuration = max(m_rtDuration, p.rtStop - m_rtFirst); // max isn't really needed, only for safety
					i = j;
				}	
			}

			Seek(pos + len);
		}
	}

	return m_mts.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDSMSplitterFile::Sync(dsmp_t& type, UINT64& len, UINT64 limit)
{
	BitByteAlign();

	limit += 5;

	for(UINT64 id = 0; (id&((1ui64<<(DSMSW_SIZE<<3))-1)) != DSMSW; id = (id << 8) | (BYTE)BitRead(8))
	{
		if(limit-- == 0 || GetPos() >= GetLength()-2)
			return(false);
	}

	type = (dsmp_t)BitRead(5);
	len = BitRead(((int)BitRead(3)+1)<<3);

	return(true);
}

bool CDSMSplitterFile::Read(UINT64 len, BYTE& id, CMediaType& mt)
{
	id = (BYTE)BitRead(8);
	ByteRead((BYTE*)&mt.majortype, sizeof(mt.majortype));
	ByteRead((BYTE*)&mt.subtype, sizeof(mt.subtype));
	mt.bFixedSizeSamples = (BOOL)BitRead(1);
	mt.bTemporalCompression = (BOOL)BitRead(1);
	mt.lSampleSize = (ULONG)BitRead(30);
	ByteRead((BYTE*)&mt.formattype, sizeof(mt.formattype));
	mt.AllocFormatBuffer((ULONG)len - (1+sizeof(GUID)*3));
	ByteRead(mt.Format(), mt.FormatLength());
	return true;
}

bool CDSMSplitterFile::Read(UINT64 len, Packet* p, bool fData)
{
	if(!p) return false;

	p->TrackNumber = (DWORD)BitRead(8);
	p->bSyncPoint = (BOOL)BitRead(1);
	bool fSign = !!BitRead(1);
	int iTimeStamp = (int)BitRead(3);
	int iDuration = (int)BitRead(3);

	if(fSign && !iTimeStamp)
	{
		ASSERT(!iDuration);
		p->rtStart = Packet::INVALID_TIME;
		p->rtStop = Packet::INVALID_TIME + 1;
	}
	else
	{
		p->rtStart = (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		p->rtStop = p->rtStart + BitRead(iDuration<<3);
	}

	if(fData)
	{
        p->pData.SetSize((INT_PTR)len - (2 + iTimeStamp + iDuration));
		ByteRead(p->pData.GetData(), p->pData.GetSize());
	}

	return true;
}

bool CDSMSplitterFile::Read(UINT64 len, CArray<SyncPoint>& sps)
{
	SyncPoint sp = {0, 0};
	sps.RemoveAll();

	while(len > 0)
	{
		bool fSign = !!BitRead(1);
		int iTimeStamp = (int)BitRead(3);
		int iFilePos = (int)BitRead(3);
		BitRead(1); // reserved

		sp.rt += (REFERENCE_TIME)BitRead(iTimeStamp<<3) * (fSign ? -1 : 1);
		sp.fp += BitRead(iFilePos<<3);
		sps.Add(sp);

		len -= 1 + iTimeStamp + iFilePos;
	}

	if(len != 0)
	{
		sps.RemoveAll();
		return false;
	}

	return true;
}

static int range_bsearch(const CArray<CDSMSplitterFile::SyncPoint>& sps, REFERENCE_TIME rt)
{
	int i = 0, j = sps.GetCount() - 1, ret = -1;

	if(j >= 0 && rt >= sps[j].rt) return(j);

	while(i < j)
	{
		int mid = (i + j) >> 1;
		REFERENCE_TIME midrt = sps[mid].rt;
		if(rt == midrt) {ret = mid; break;}
		else if(rt < midrt) {ret = -1; if(j == mid) mid--; j = mid;}
		else if(rt > midrt) {ret = mid; if(i == mid) mid++; i = mid;}
	}

	return ret;
}

__int64 CDSMSplitterFile::FindSyncPoint(REFERENCE_TIME rt)
{
	rt += m_rtFirst;

	if(!m_sps.IsEmpty())
	{
		int i = range_bsearch(m_sps, rt);
		if(i >= 0) return m_sps[i].fp;
	}
	else
	{
		// TODO
	}

	return 0;
}
