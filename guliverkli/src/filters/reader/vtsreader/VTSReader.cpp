#include "StdAfx.h"
#include "..\..\..\decss\VobFile.h"
#include "vtsreader.h"
#include "..\..\..\DSUtil\DSUtil.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PROGRAM},
};

const AMOVIESETUP_PIN sudOpPin[] =
{
	{
		L"Output",              // Pin string name
		FALSE,                  // Is it rendered
		TRUE,                   // Is it an output
		FALSE,                  // Can we have none
		FALSE,                  // Can we have many
		&CLSID_NULL,            // Connects to filter
		NULL,                   // Connects to pin
		sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
		sudPinTypesOut			// Pin details
	}
};

const AMOVIESETUP_FILTER sudFilter =
{
    &__uuidof(CVTSReader),		// Filter CLSID
    L"VTS Reader",			// String name
    MERIT_UNLIKELY,			// Filter merit
    sizeof(sudOpPin)/sizeof(sudOpPin[0]), // Number of pins
    sudOpPin				// Pin information
};

CFactoryTemplate g_Templates[] =
{
	{ L"VTS Reader"
	, &__uuidof(CVTSReader)
	, CVTSReader::CreateInstance
	, NULL
	, &sudFilter}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	if(GetVersion()&0x80000000) 
		return E_NOTIMPL;

	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}"), 
		_T("0"), _T("0,12,,445644564944454F2D565453")); // "DVDVIDEO-VTS"

	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}"), 
		_T("Source Filter"), _T("{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}"));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{773EAEDE-D5EE-4fce-9C8F-C4F53D0A2F73}"));

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CVTSReader
//

CUnknown* WINAPI CVTSReader::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CVTSReader(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CVTSReader::CVTSReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CVTSReader"), pUnk, &m_stream, phr, __uuidof(this))
{
	if(phr) *phr = S_OK;

	if(GetVersion()&0x80000000)
	{
		if(phr) *phr = E_NOTIMPL;
		return;
	}
}

CVTSReader::~CVTSReader()
{
}

STDMETHODIMP CVTSReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IFileSourceFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CVTSReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt) 
{
	if(!m_stream.Load(pszFileName))
		return E_FAIL;

	m_fn = pszFileName;

	CMediaType mt;
	mt.majortype = MEDIATYPE_Stream;
	mt.subtype = MEDIASUBTYPE_MPEG2_PROGRAM;
	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CVTSReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	if(!ppszFileName) return E_POINTER;
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
		return E_OUTOFMEMORY;

	wcscpy(*ppszFileName, m_fn);

	return S_OK;
}

// CVTSStream

CVTSStream::CVTSStream() : m_off(0)
{
	m_vob.Attach(new CVobFile());
}

CVTSStream::~CVTSStream()
{
}

bool CVTSStream::Load(const WCHAR* fnw)
{
	CStringList sl;
	return(m_vob && m_vob->Open(CString(fnw), sl) /*&& m_vob->IsDVD()*/);
}

HRESULT CVTSStream::SetPointer(LONGLONG llPos)
{
	m_off = (int)(llPos&2047);
	int lba = (int)(llPos/2048);

	return lba == m_vob->Seek(lba) ? S_OK : S_FALSE;
}

HRESULT CVTSStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	CAutoLock lck(&m_csLock);

	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	while(len > 0)
	{
		BYTE buff[2048];
		if(!m_vob->Read(buff))
			break;

		int size = min(2048 - m_off, min(len, 2048));

		memcpy(ptr, &buff[m_off], size);

		m_off = (m_off + size)&2047;

		if(m_off > 0)
			m_vob->Seek(m_vob->GetPosition()-1);

		ptr += size;
		len -= size;
	}

	if(pdwBytesRead)
		*pdwBytesRead = ptr - pbBuffer;

	return S_OK;
}

LONGLONG CVTSStream::Size(LONGLONG* pSizeAvailable)
{
	LONGLONG len = 2048i64*m_vob->GetLength();
	if(pSizeAvailable) *pSizeAvailable = len;
	return(len);
}

DWORD CVTSStream::Alignment()
{
    return 1;
}

void CVTSStream::Lock()
{
    m_csLock.Lock();
}

void CVTSStream::Unlock()
{
    m_csLock.Unlock();
}

