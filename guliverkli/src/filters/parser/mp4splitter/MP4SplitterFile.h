#pragma once

#include "..\BaseSplitter\BaseSplitter.h"
// #include "Ap4AsyncReaderStream.h" // FIXME

class CMP4SplitterFile : public CBaseSplitterFile
{
	void* /* AP4_File* */ m_pAp4File;

	HRESULT Init();

public:
	CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr);
	virtual ~CMP4SplitterFile();

	void* /* AP4_Movie* */ GetMovie();

	CAtlMap<DWORD, CMediaType> m_mts;
	REFERENCE_TIME m_rtDuration;
};
