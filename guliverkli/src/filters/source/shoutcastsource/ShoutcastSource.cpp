/* 
 *	Copyright (C) 2003 Gabest
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

#include "stdafx.h"
#include <initguid.h>
#include "shoutcastsource.h"
#include "..\..\..\DSUtil\DSUtil.h"

#define MAXFRAMESIZE ((144 * 320000 / 8000) + 1)
#define BUFFERS 2
#define MINBUFFERLENGTH 1000000i64
#define AVGBUFFERLENGTH 30000000i64
#define MAXBUFFERLENGTH 100000000i64

static const DWORD s_bitrate[2][16] =
{
	{1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},
	{1,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0}
};
static const DWORD s_freq[4][4] =
{
	{11025,12000,8000,0},
	{0,0,0,0},
	{22050,24000,16000,0},
	{44100,48000,32000,0}
};
static const BYTE s_channels[2][4] =
{
	{1,1,1,1}, // only mono for mpeg2/2.5 layer3 ???
	{2,2,2,1} // stereo, joint stereo, dual, mono
};

typedef struct
{
	WORD sync;
	BYTE version;
	BYTE layer;
	DWORD bitrate;
	DWORD freq;
	BYTE channels;
	DWORD framesize;

	bool ExtractHeader(CSocket& socket)
	{
		BYTE buff[4];
		if(4 != socket.Receive(buff, 4, MSG_PEEK))
			return(false);

		sync = (buff[0]<<4)|(buff[1]>>4)|1;
		version = (buff[1]>>3)&3;
		layer = 4 - ((buff[1]>>1)&3);
		bitrate = s_bitrate[version&1][buff[2]>>4]*1000;
		freq = s_freq[version][(buff[2]>>2)&3];
		channels = s_channels[version&1][(buff[3]>>2)&3];
		framesize = freq ? ((((version&1)?144:72) * bitrate / freq) + ((buff[2]>>1)&1)) : 0;

		return(sync == 0xfff && layer == 3 && bitrate != 0 && freq != 0);
	}

} mp3hdr;

#include <initguid.h>

// 00000055-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_MP3,
0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
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
	},
};

const AMOVIESETUP_FILTER sudFilter =
{
    &__uuidof(CShoutcastSource),	// Filter CLSID
    L"ShoutcastSource",				// String name
    MERIT_UNLIKELY,					// Filter merit
    sizeof(sudOpPin)/sizeof(sudOpPin[0]), // Number of pins
    sudOpPin						// Pin information
};

CFactoryTemplate g_Templates[] =
{
	{ L"ShoutcastSource"
	, &__uuidof(CShoutcastSource)
	, CShoutcastSource::CreateInstance
	, NULL
	, &sudFilter}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

class CMyApp : public CWinApp
{
public:
	CMyApp() {}

	BOOL InitInstance()
	{
		if(!AfxSocketInit(NULL))
		{
			AfxMessageBox(_T("AfxSocketInit failed!"));
			return FALSE;
		}

		return DllEntryPoint(AfxGetInstanceHandle(), DLL_PROCESS_ATTACH, 0) 
			? __super::InitInstance() 
			: FALSE;
	}

	int ExitInstance()
	{
		DllEntryPoint(AfxGetInstanceHandle(), DLL_PROCESS_DETACH, 0); 
		return __super::ExitInstance();
	}
};

CMyApp theApp;

//
// CShoutcastSource
//

CUnknown* WINAPI CShoutcastSource::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CShoutcastSource(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

CShoutcastSource::CShoutcastSource(LPUNKNOWN lpunk, HRESULT* phr)
	: CSource(NAME("CShoutcastSource"), lpunk, __uuidof(this))
{
#ifndef REGISTER_FILTER
	AfxSocketInit();
#endif
}

CShoutcastSource::~CShoutcastSource()
{
}

STDMETHODIMP CShoutcastSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IFileSourceFilter)
		QI(IAMFilterMiscFlags)
		QI(IAMOpenProgress)
		QI2(IAMMediaContent)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CShoutcastSource::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt) 
{
	if(GetPinCount() > 0)
		return VFW_E_ALREADY_CONNECTED;

	HRESULT hr = E_OUTOFMEMORY;

	if(!(new CShoutcastStream(pszFileName, this, &hr)) || FAILED(hr))
		return hr;

	m_fn = pszFileName;

	return S_OK;
}

STDMETHODIMP CShoutcastSource::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	if(!ppszFileName) return E_POINTER;
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
		return E_OUTOFMEMORY;

	wcscpy(*ppszFileName, m_fn);

	return S_OK;
}

// IAMFilterMiscFlags

ULONG CShoutcastSource::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

// IAMOpenProgress

STDMETHODIMP CShoutcastSource::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	if(m_iPins == 1)
	{
        if(pllTotal) *pllTotal = 100;
		if(pllCurrent) *pllCurrent = ((CShoutcastStream*)m_paStreams[0])->GetBufferFullness();
		return S_OK;
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CShoutcastSource::AbortOperation()
{
	return E_NOTIMPL;
}

// IAMMediaContent

STDMETHODIMP CShoutcastSource::get_Title(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	if(m_iPins == 1)
	{
		*pbstrTitle = ((CShoutcastStream*)m_paStreams[0])->GetTitle().AllocSysString();
		return S_OK;
	}

	return E_UNEXPECTED;
}

// CShoutcastStream

CShoutcastStream::CShoutcastStream(const WCHAR* wfn, CShoutcastSource* pParent, HRESULT* phr)
	: CSourceStream(NAME("ShoutcastStream"), phr, pParent, L"Output")
	, m_fBuffering(false)
{
	ASSERT(phr);

	*phr = S_OK;

	CString fn(wfn);
	if(fn.Find(_T("://")) < 0) fn = _T("http://") + fn;

#ifdef REGISTER_FILTER
//fn = _T("http://66.250.32.195:8012/");
//fn = _T("http://localhost:8000/");
//fn = _T("http://210.120.247.49:9570"); // 48khz/320kbps/stereo, choppy
//fn = _T("http://205.188.234.38:8002"); // no output, freezes stop
//fn = _T("http://64.236.34.141/stream/1005");
//fn = _T("http://218.145.30.106:11000"); // 128kbps korean
//fn = _T("http://65.206.46.110:8020"); // 96kbps
fn = _T("http://218.145.30.106:11000");
//fn = _T("http://radio.sluchaj.com:8000/radio.ogg"); // ogg
// http://www.oddsock.org/icecast2yp/ // more ogg via icecast2
#endif

	if(!m_url.CrackUrl(fn))
	{
		*phr = E_FAIL;
		return;
	}

	if(m_url.GetUrlPathLength() == 0)
		m_url.SetUrlPath(_T("/"));

	if(m_url.GetPortNumber() == ATL_URL_INVALID_PORT_NUMBER)
		m_url.SetPortNumber(ATL_URL_DEFAULT_HTTP_PORT);

#ifdef REGISTER_FILTER
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif

	if(!m_socket.Create() || !m_socket.Connect(m_url))
	{
		*phr = E_FAIL;
		return;
	}

	m_socket.Close();
}

CShoutcastStream::~CShoutcastStream()
{
}

void CShoutcastStream::EmptyBuffer()
{
	CAutoLock cAutoLock(&m_queue);
	m_queue.RemoveAll();
}

LONGLONG CShoutcastStream::GetBufferFullness()
{
	CAutoLock cAutoLock(&m_queue);
	if(!m_fBuffering) return 100;
	if(m_queue.IsEmpty()) return 0;
	LONGLONG ret = 100i64*(m_queue.GetTail().rtStart - m_queue.GetHead().rtStart) / AVGBUFFERLENGTH;
	return(min(ret, 100));
}

CString CShoutcastStream::GetTitle()
{
	CAutoLock cAutoLock(&m_queue);
	return(m_title);
}

HRESULT CShoutcastStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    HRESULT hr = NOERROR;

	pProperties->cBuffers = BUFFERS;
	pProperties->cbBuffer = MAXFRAMESIZE;

    ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
    ASSERT(Actual.cBuffers == pProperties->cBuffers);

    return NOERROR;
}

HRESULT CShoutcastStream::FillBuffer(IMediaSample* pSample)
{
	HRESULT hr;

	BYTE* pData = NULL;
	if(FAILED(hr = pSample->GetPointer(&pData)) || !pData)
		return S_FALSE;

	do
	{
		// do we have to refill our buffer?
		{
			CAutoLock cAutoLock(&m_queue);
			if(!m_queue.IsEmpty() && m_queue.GetHead().rtStart < m_queue.GetTail().rtStart - MINBUFFERLENGTH)
				break; // nope, that's great
		}

		TRACE(_T("START BUFFERING\n"));
		m_fBuffering = true;

		while(1)
		{
			if(fExitThread) // playback stopped?
				return S_FALSE;

			Sleep(50);

			CAutoLock cAutoLock(&m_queue);
			if(!m_queue.IsEmpty() && m_queue.GetHead().rtStart < m_queue.GetTail().rtStart - AVGBUFFERLENGTH)
				break; // this is enough
		}

		pSample->SetDiscontinuity(TRUE);

		DeliverBeginFlush();
		DeliverEndFlush();

		TRACE(_T("END BUFFERING\n"));
		m_fBuffering = false;
	}
	while(false);

	{
		CAutoLock cAutoLock(&m_queue);
		ASSERT(!m_queue.IsEmpty());
		if(!m_queue.IsEmpty())
		{
			mp3frame f = m_queue.RemoveHead();
			DWORD len = min(pSample->GetSize(), f.len);
			memcpy(pData, f.pData, len);
			pSample->SetActualDataLength(len);
			pSample->SetTime(&f.rtStart, &f.rtStop);
			m_title = f.title;
		}
	}

	pSample->SetSyncPoint(TRUE);

	return S_OK;
}

HRESULT CShoutcastStream::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

    pmt->SetType(&MEDIATYPE_Audio);
    pmt->SetSubtype(&MEDIASUBTYPE_MP3);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	wfe->wFormatTag = (WORD)MEDIASUBTYPE_MP3.Data1;
	wfe->nChannels = (WORD)m_socket.m_channels;
	wfe->nSamplesPerSec = m_socket.m_freq;
	wfe->nAvgBytesPerSec = m_socket.m_bitrate/8;
	wfe->nBlockAlign = 1;
	wfe->wBitsPerSample = 0;

    return NOERROR;
}

HRESULT CShoutcastStream::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->majortype == MEDIATYPE_Audio
	&& pmt->subtype == MEDIASUBTYPE_MP3
	&& pmt->formattype == FORMAT_WaveFormatEx) return S_OK;

	return E_INVALIDARG;
}

static UINT SocketThreadProc(LPVOID pParam)
{
	return ((CShoutcastStream*)pParam)->SocketThreadProc();
}

UINT CShoutcastStream::SocketThreadProc()
{
	fExitThread = false;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

#ifdef REGISTER_FILTER
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif

	AfxSocketInit();

	CAutoVectorPtr<BYTE> pData;
	if(!m_socket.Create() || !m_socket.Connect(m_url)
	|| !pData.Allocate(max(m_socket.m_metaint, MAXFRAMESIZE)))
	{
		m_socket.Close(); 
		return 1;
	}

	REFERENCE_TIME m_rtSampleTime = 0;
	
	while(!fExitThread)
	{
		int len = MAXFRAMESIZE;
		len = m_socket.Receive(pData, len);
		if(len <= 0) break;

		mp3frame f(len);
		memcpy(f.pData, pData, len);
		f.rtStop = (f.rtStart = m_rtSampleTime) + (10000000i64 * len * 8/m_socket.m_bitrate);
		m_rtSampleTime = f.rtStop;
		f.title = m_socket.m_title;

		CAutoLock cAutoLock(&m_queue);
		m_queue.AddTail(f);
	}

	m_socket.Close();

	return 0;
}

HRESULT CShoutcastStream::OnThreadCreate()
{
	EmptyBuffer();

	fExitThread = true;
	m_hSocketThread = AfxBeginThread(::SocketThreadProc, this)->m_hThread;
	while(fExitThread) Sleep(10);

	return NOERROR;
}

HRESULT CShoutcastStream::OnThreadDestroy()
{
	EmptyBuffer();
	
	fExitThread = true;
	m_socket.CancelBlockingCall();
	WaitForSingleObject(m_hSocketThread, -1);

	return NOERROR;
}

HRESULT CShoutcastStream::Inactive()
{
	fExitThread = true;
	return __super::Inactive();
}

//

int CShoutcastStream::CShoutcastSocket::Receive(void* lpBuf, int nBufLen, int nFlags)
{
	if(nFlags&MSG_PEEK) 
		return __super::Receive(lpBuf, nBufLen, nFlags);

	if(m_metaint > 0 && m_nBytesRead + nBufLen > m_metaint)
		nBufLen = m_metaint - m_nBytesRead;

	int len = __super::Receive(lpBuf, nBufLen, nFlags);
	if(len <= 0) return len;

	if((m_nBytesRead += len) == m_metaint)
	{
		m_nBytesRead = 0;

		static BYTE buff[255*16], b = 0;
		memset(buff, 0, sizeof(buff));
		if(1 == __super::Receive(&b, 1) && b && b*16 == __super::Receive(buff, b*16))
		{
			CStringA str = (LPCSTR)buff, title("StreamTitle='");
			int i = str.Find(title);
			if(i >= 0)
			{
				i += title.GetLength();
				int j = str.Find('\'', i);
				if(j > i) m_title = str.Mid(i, j - i);
			}
		}
	}

	return len;
}

bool CShoutcastStream::CShoutcastSocket::Connect(CUrl& url)
{
	if(!__super::Connect(url.GetHostName(), url.GetPortNumber()))
		return(false);

	CStringA str;
	str.Format(
		"GET %s HTTP/1.0\r\n"
		"Icy-MetaData:1\r\n"
//		"User-Agent: shoutcastsource\r\n"
		"Host: %s\r\n"
		"Accept: */*\r\n"
		"Connection: Keep-Alive\r\n"
		"\r\n", CStringA(url.GetUrlPath()), CStringA(url.GetHostName()));
	int len = Send((BYTE*)(LPCSTR)str, str.GetLength());

	m_nBytesRead = 0;
	m_metaint = 0;
	m_bitrate = 0;

	bool fOK = false;
	int metaint = 0;

	str.Empty();
	BYTE cur = 0, prev = 0;
	while(Receive(&cur, 1) == 1 && cur && !(cur == '\n' && prev == '\n'))
	{
		if(cur == '\r')
			continue;

		if(cur == '\n')
		{
			str.MakeLower();
			if(str.Find("icy 200 ok") >= 0) fOK = true;
			else if(1 == sscanf(str, "icy-br:%d", &m_bitrate)) m_bitrate *= 1000;
			else if(1 == sscanf(str, "icy-metaint:%d", &metaint)) metaint = metaint;
			str.Empty();
		}
		else
		{
			str += cur;
		}

		prev = cur;
		cur = 0;
	}

	if(!fOK || m_bitrate == 0) {Close(); return(false);}

	m_metaint = metaint;
	m_nBytesRead = 0;

	return(FindSync());
}

bool CShoutcastStream::CShoutcastSocket::FindSync()
{
	m_freq = -1;
	m_channels = -1;

	BYTE b;
	for(int i = MAXFRAMESIZE; i > 0; i--, Receive(&b, 1))
	{
		mp3hdr h;
		if(h.ExtractHeader(*this) && m_bitrate == h.bitrate)
		{
			if(h.bitrate > 1) m_bitrate = h.bitrate;
			m_freq = h.freq;
			m_channels = h.channels;
			return(true);
		}
	}

	return(false);
}
