#pragma once

#include <atlbase.h>
#include "..\asyncreader\asyncio.h"
#include "..\asyncreader\asyncrdr.h"

class CCDXAStream : public CAsyncStream
{
private:
    CCritSec m_csLock;

	HANDLE m_hFile;
	LONGLONG m_llPosition, m_llLength;
	int m_nFirstSector;

	bool LookForMediaSubType();

public:
	CCDXAStream();
	virtual ~CCDXAStream();

	bool Load(const WCHAR* fnw);

    HRESULT SetPointer(LONGLONG llPos);
    HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
    LONGLONG Size(LONGLONG* pSizeAvailable);
    DWORD Alignment();
    void Lock();
	void Unlock();

	GUID m_subtype;
};

[uuid("D367878E-F3B8-4235-A968-F378EF1B9A44")]
class CCDXAReader 
	: public CAsyncReader
	, public IFileSourceFilter
{
	CCDXAStream m_stream;
	CStringW m_fn;

public:
    CCDXAReader(IUnknown* pUnk, HRESULT* phr);
	~CCDXAReader();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);
};
