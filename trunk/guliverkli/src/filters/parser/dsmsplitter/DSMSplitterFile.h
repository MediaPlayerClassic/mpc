#pragma once

#include "..\BaseSplitter\BaseSplitter.h"
#include "..\..\..\..\include\dsm\dsm.h"

class CDSMSplitterFile : public CBaseSplitterFile
{
	HRESULT Init();

public:
	CDSMSplitterFile(IAsyncReader* pReader, HRESULT& hr);

	struct SyncPoint {REFERENCE_TIME rt; __int64 fp;};
	CArray<SyncPoint> m_sps;

	CMap<BYTE,BYTE,CMediaType,CMediaType&> m_mts;
	REFERENCE_TIME m_rtFirst, m_rtDuration;

	bool Sync(dsmp_t& type, UINT64& len, UINT64 limit = 65536);
	bool Read(UINT64 len, BYTE& id, CMediaType& mt);
	bool Read(UINT64 len, Packet* p, bool fData = true);
	bool Read(UINT64 len, CArray<SyncPoint>& sps);
	
	__int64 FindSyncPoint(REFERENCE_TIME rt);
};
