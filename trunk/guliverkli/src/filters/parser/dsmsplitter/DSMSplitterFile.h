#pragma once

#include "..\BaseSplitter\BaseSplitter.h"
#include "..\..\..\..\include\dsm\dsm.h"
#include "..\..\..\DSUtil\DSMPropertyBag.h"

class CDSMSplitterFile : public CBaseSplitterFile
{
	HRESULT Init(CArray<CDSMResource>& resources);

public:
	CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr, CArray<CDSMResource>& resources);

	CAtlMap<BYTE, CMediaType> m_mts;
	REFERENCE_TIME m_rtFirst, m_rtDuration;

	struct SyncPoint {REFERENCE_TIME rt; __int64 fp;};
	CArray<SyncPoint> m_sps;

	struct Chapter {REFERENCE_TIME rt; CStringW name;};
	CArray<Chapter> m_cs;

	typedef CAtlMap<CStringA, CStringW, CStringElementTraits<CStringA>, CStringElementTraits<CStringW> > CStreamInfoMap;
	CStreamInfoMap m_fim;
	CAtlMap<BYTE, CStreamInfoMap> m_sim;

	bool Sync(dsmp_t& type, UINT64& len, __int64 limit = 65536);
	bool Sync(UINT64& syncpos, dsmp_t& type, UINT64& len, __int64 limit = 65536);
	bool Read(__int64 len, BYTE& id, CMediaType& mt);
	bool Read(__int64 len, Packet* p, bool fData = true);
	bool Read(__int64 len, CArray<SyncPoint>& sps);
	bool Read(__int64 len, CArray<Chapter>& cs);
	bool Read(__int64 len, CStreamInfoMap& im);
	bool Read(__int64 len, CArray<CDSMResource>& resources);
	__int64 Read(__int64 len, CStringW& str);
	
	__int64 FindSyncPoint(REFERENCE_TIME rt);
};
