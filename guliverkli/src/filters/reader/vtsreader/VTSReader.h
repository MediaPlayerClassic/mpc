#pragma once

#include <atlbase.h>
#include "..\asyncreader\asyncio.h"
#include "..\asyncreader\asyncrdr.h"

class CVobFile;

class CVTSStream : public CAsyncStream
{
private:
    CCritSec m_csLock;

	CAutoPtr<CVobFile> m_vob;
	int m_off;

public:
	CVTSStream();
	virtual ~CVTSStream();

	bool Load(const WCHAR* fnw);

    HRESULT SetPointer(LONGLONG llPos);
    HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
    LONGLONG Size(LONGLONG* pSizeAvailable);
    DWORD Alignment();
    void Lock();
	void Unlock();
};

[uuid("773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73")]
class CVTSReader 
	: public CAsyncReader
	, public IFileSourceFilter
{
	CVTSStream m_stream;
	CStringW m_fn;

public:
    CVTSReader(IUnknown* pUnk, HRESULT* phr);
	~CVTSReader();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);
};
