#pragma once

#include <atlbase.h>
#include <afxtempl.h>
#include "..\BaseSource\BaseSource.h"

class CDTSAC3Stream;

[uuid("B4A7BE85-551D-4594-BDC7-832B09185041")]
class CDTSAC3Source : public CBaseSource<CDTSAC3Stream>
{
public:
	CDTSAC3Source(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CDTSAC3Source();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};

class CDTSAC3Stream : public CBaseStream
{
	CFile m_file;
	int m_nFileOffset, m_nBytesPerFrame, m_nAvgBytesPerSec, m_nSamplesPerSec;
	GUID m_subtype;
	WORD m_wFormatTag;
	BYTE m_streamid;

	bool CheckDTS(const CMediaType* pmt);
	bool CheckWAVEDTS(const CMediaType* pmt);
	bool CheckAC3(const CMediaType* pmt);
	bool CheckWAVEAC3(const CMediaType* pmt);

public:
    CDTSAC3Stream(const WCHAR* wfn, CSource* pParent, HRESULT* phr);
	virtual ~CDTSAC3Stream();

    HRESULT FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len);
    
	HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
};
