#pragma once

#include "..\BaseSplitter\BaseSplitter.h"
#include "..\..\..\..\include\dsm\dsm.h"

template<class T>
int range_bsearch(const CArray<T>& array, REFERENCE_TIME rt)
{
	int i = 0, j = array.GetCount() - 1, ret = -1;
	if(j >= 0 && rt >= array[j].rt) return j;
	while(i < j)
	{
		int mid = (i + j) >> 1;
		REFERENCE_TIME midrt = array[mid].rt;
		if(rt == midrt) {ret = mid; break;}
		else if(rt < midrt) {ret = -1; if(j == mid) mid--; j = mid;}
		else if(rt > midrt) {ret = mid; if(i == mid) mid++; i = mid;}
	}
	return ret;
}

class CDSMSplitterFile : public CBaseSplitterFile
{
	HRESULT Init();

public:
	CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr);

	CMap<BYTE,BYTE,CMediaType,CMediaType&> m_mts;
	REFERENCE_TIME m_rtFirst, m_rtDuration;

	struct SyncPoint {REFERENCE_TIME rt; __int64 fp;};
	CArray<SyncPoint> m_sps;

	struct Chapter {REFERENCE_TIME rt; CStringW name;};
	CArray<Chapter> m_cs;

	bool Sync(dsmp_t& type, UINT64& len, UINT64 limit = 65536);
	bool Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, UINT64 limit = 65536);
	bool Read(UINT64 len, BYTE& id, CMediaType& mt);
	bool Read(UINT64 len, Packet* p, bool fData = true);
	bool Read(UINT64 len, CArray<SyncPoint>& sps);
	bool Read(UINT64 len, CArray<Chapter>& cs);
	
	__int64 FindSyncPoint(REFERENCE_TIME rt);
};
