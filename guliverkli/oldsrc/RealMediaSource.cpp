#include "stdafx.h"
#include <atlbase.h>

#include <streams.h>

#include <initguid.h>

#include "realmediasource.h"

#include "mainfrm.h"

// {120277F1-1A35-42aa-AE56-2E47D03B1C0B}
DEFINE_GUID(CLSID_RealMediaSource, 
0x120277f1, 0x1a35, 0x42aa, 0xae, 0x56, 0x2e, 0x47, 0xd0, 0x3b, 0x1c, 0xb);

CUnknown* WINAPI CRealMediaSource::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CRealMediaSource(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CRealMediaSource::CRealMediaSource(LPUNKNOWN lpunk, HRESULT* phr) : 
	CSource(NAME("RealMedia Source"), lpunk, CLSID_RealMediaSource)/*,
    CPersistStream(lpunk, phr)*/,
	m_fpCreateEngine(NULL), m_fpCloseEngine(NULL), m_hRealMediaCore(NULL)
{

//    CAutoLock cAutoLock(&m_cStateLock);
/*
	m_paStreams = (CRealMediaStream**) new CRealMediaStream*[1];
	if(m_paStreams == NULL)
	{
		*phr = E_OUTOFMEMORY;
		return;
	}
	
	m_paStreams[0] = new CRealMediaStream("RealMedia Source", phr, this, L"Output");
	if(m_paStreams[0] == NULL)
	{
		*phr = E_OUTOFMEMORY;
		return;
	}
*/
}

CRealMediaSource::~CRealMediaSource()
{
	JoinFilterGraph(NULL, NULL);
}

STDMETHODIMP CRealMediaSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return (riid == IID_IFileSourceFilter) ? GetInterface((IFileSourceFilter *)this, ppv)
		: (riid == IID_IMediaSeeking) ? GetInterface((IMediaSeeking *)this, ppv)
//		: (riid == IID_IPersistStream) ? GetInterface((IPersistStream *)this, ppv)
		: (riid == IID_IRMAErrorSink) ? GetInterface((IRMAErrorSink *)this, ppv)
		: (riid == IID_IRMAClientAdviseSink) ? GetInterface((IRMAClientAdviseSink *)this, ppv)
		: (riid == IID_IRMAAuthenticationManager) ? GetInterface((IRMAAuthenticationManager *)this, ppv)
		: (riid == IID_IRMASiteSupplier) ? GetInterface((IRMASiteSupplier *)this, ppv)
		: (riid == IID_IRMASiteWatcher) ? GetInterface((IRMASiteWatcher *)this, ppv)
		: CSource::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CRealMediaSource::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	HRESULT hr;
	if(S_OK != (hr = __super::JoinFilterGraph(pGraph, pName)))
		return hr;

	if(pGraph)
	{
		CRegKey key;
		if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, _T("Software\\RealNetworks\\Preferences\\DT_Common")))
			return E_FAIL;

		TCHAR buff[MAX_PATH];
		ULONG len = sizeof(buff);
		if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len))
			return E_FAIL;

		if(!(m_hRealMediaCore = LoadLibrary(CString(buff) + _T("pnen3260.dll")))) 
			return E_FAIL;

		m_fpCreateEngine = (FPRMCREATEENGINE)GetProcAddress(m_hRealMediaCore, "CreateEngine");
		m_fpCloseEngine = (FPRMCLOSEENGINE)GetProcAddress(m_hRealMediaCore, "CloseEngine");
	 
		if(m_fpCreateEngine == NULL || m_fpCloseEngine == NULL)
			return E_FAIL;

		if(PNR_OK != m_fpCreateEngine(&m_pEngine))
			return E_FAIL;

		if(PNR_OK != m_pEngine->CreatePlayer(*&m_pPlayer))
			return E_FAIL;

		if(!(m_pSiteManager = m_pPlayer) || !(m_pCommonClassFactory = m_pPlayer))
			return E_FAIL;

		CComQIPtr<IRMAErrorSinkControl, &IID_IRMAErrorSinkControl> pErrorSinkControl = m_pPlayer;
		if(pErrorSinkControl)
			pErrorSinkControl->AddErrorSink(static_cast<IRMAErrorSink*>(this), PNLOG_EMERG, PNLOG_INFO);

		if(PNR_OK != m_pPlayer->AddAdviseSink(static_cast<IRMAClientAdviseSink*>(this)))
			return E_FAIL;

		if(PNR_OK != m_pPlayer->SetClientContext((IUnknown*)(INonDelegatingUnknown*)(this)))
			return E_FAIL;
	}
	else
	{
		if(m_pPlayer)
		{
			m_pPlayer->Stop();

			CComQIPtr<IRMAErrorSinkControl, &IID_IRMAErrorSinkControl> pErrorSinkControl = m_pPlayer;
			if(pErrorSinkControl)
				pErrorSinkControl->RemoveErrorSink(static_cast<IRMAErrorSink*>(this));

			m_pPlayer->RemoveAdviseSink(static_cast<IRMAClientAdviseSink*>(this));

			m_pEngine->ClosePlayer(m_pPlayer);

			m_pSiteManager.Release();
			m_pCommonClassFactory.Release();

			m_pPlayer = NULL;
		}

		if(m_pEngine)
		{
			m_fpCloseEngine(m_pEngine);
			m_pEngine = NULL;
		}

		if(m_hRealMediaCore) 
		{
			FreeLibrary(m_hRealMediaCore);
			m_hRealMediaCore = NULL;
		}
	}

	return S_OK;
}

/*
DWORD CRealMediaSource::GetSoftwareVersion()
{
	return(0x0100);
}

HRESULT CSubtitleSource::WriteToStream(IStream* pStream)
{
    CAutoLock cAutolock(&m_propsLock);

	int len = m_fn.GetLength();
	HRESULT hr = pStream->Write(&len, sizeof(len), NULL);
	if(FAILED(hr)) return hr;

	return NOERROR;
}

HRESULT CSubtitleSource::ReadFromStream(IStream* pStream)
{
    CAutoLock cAutolock(&m_propsLock);

	int len;

    HRESULT hr = pStream->Read(&len, sizeof(len), NULL);
	if(FAILED(hr) || len <= 0) return hr;

	char* buff = new char[len+1];

    hr = pStream->Read(buff, len+1, NULL);
	if(FAILED(hr)) {delete [] buff; return hr;}

	m_fn = buff;

	delete [] buff;

	if(m_paStreams[0]) ((CSubtitleStream*)m_paStreams[0])->Open(m_fn);

    return NOERROR;
}

STDMETHODIMP CRealMediaSource::GetClassID(CLSID* pClsid)
{
    if(pClsid == NULL) return E_POINTER;

	*pClsid = CLSID_SubtitleSource;

    return NOERROR;
}
*/

// IFileSourceFilter

STDMETHODIMP CRealMediaSource::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	ASSERT(ppszFileName);
	
	if(m_fn.IsEmpty()) return(E_FAIL);

	*ppszFileName = (WCHAR*)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR));
	if(*ppszFileName == NULL) return S_FALSE;

	wcscpy(*ppszFileName, CStringW(m_fn));
/*
	CMediaType mt;
	((CRealMediaStream*)m_paStreams[0])->GetMediaType(0, &mt);
	*pmt = mt;
*/
	return(S_OK);
}

STDMETHODIMP CRealMediaSource::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if(!m_pGraph) return VFW_E_NOT_IN_GRAPH;

	if(PNR_OK != m_pPlayer->OpenURL(CStringA(pszFileName))) return E_FAIL;

m_pPlayer->Begin();

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CRealMediaSource::GetCapabilities(DWORD* pCapabilities)
{
	if(!pCapabilities) return E_POINTER;

	*pCapabilities = AM_SEEKING_CanSeekAbsolute|AM_SEEKING_CanGetCurrentPos|AM_SEEKING_CanGetDuration;

	return S_OK;
}

STDMETHODIMP CRealMediaSource::CheckCapabilities(DWORD* pCapabilities)
{
	if(!pCapabilities) return E_POINTER;

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;
	GetCapabilities(&caps);

	DWORD caps2 = caps & *pCapabilities;

	return caps2 == 0 ? E_FAIL : caps2 == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CRealMediaSource::IsFormatSupported(const GUID* pFormat)
{
	return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CRealMediaSource::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CRealMediaSource::GetTimeFormat(GUID* pFormat)
{
	if(!pFormat) return E_POINTER;

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CRealMediaSource::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CRealMediaSource::SetTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CRealMediaSource::GetDuration(LONGLONG* pDuration)
{
	if(!m_pPlayer) return E_FAIL;

	if(!pDuration) return E_POINTER;

	*pDuration = m_nDuration;

	return S_OK;
}

STDMETHODIMP CRealMediaSource::GetStopPosition(LONGLONG* pStop)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRealMediaSource::GetCurrentPosition(LONGLONG* pCurrent)
{
	if(!m_pPlayer) return E_FAIL;

	if(!pCurrent) return E_POINTER;

	*pCurrent = (REFERENCE_TIME)m_pPlayer->GetCurrentPlayTime()*10000;

	return S_OK;
}

STDMETHODIMP CRealMediaSource::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRealMediaSource::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(!m_pPlayer) return E_FAIL;

	if(!(dwCurrentFlags&AM_SEEKING_AbsolutePositioning)) return E_FAIL;

	return (PNR_OK == m_pPlayer->Seek((ULONG)(*pCurrent / 10000))) ? S_OK : E_FAIL;
}

STDMETHODIMP CRealMediaSource::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRealMediaSource::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRealMediaSource::SetRate(double dRate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRealMediaSource::GetRate(double* pdRate)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRealMediaSource::GetPreroll(LONGLONG* pllPreroll)
{
	return E_NOTIMPL;
}

// IRMAErrorSink

STDMETHODIMP CRealMediaSource::ErrorOccurred(const UINT8 unSeverity, const UINT32 ulRMACode, const UINT32 ulUserCode, const char* pUserString, const char* pMoreInfoURL)
{
    CComPtr<IRMABuffer> pBuffer;
    CComQIPtr<IRMAErrorMessages, &IID_IRMAErrorMessages> pErrorMessages = m_pPlayer;
	if(pErrorMessages) pBuffer = pErrorMessages->GetErrorText(ulRMACode);

    TRACE(_T("Report(%d, 0x%x, \"%s\", %ld, \"%s\", \"%s\")\n"),
		    unSeverity,
		    ulRMACode,
		    (pUserString && *pUserString) ? CString(pUserString) : _T("(NULL)"),
		    ulUserCode,
		    (pMoreInfoURL && *pMoreInfoURL) ? CString(pMoreInfoURL) : _T("(NULL)"),
                    pBuffer ? CString((char *)pBuffer->GetBuffer()) : _T("(unknown error)"));

	return PNR_OK;
}

// IRMAClientAdviseSink

STDMETHODIMP CRealMediaSource::OnPosLength(UINT32 ulPosition, UINT32 ulLength)
{
	TRACE(_T("OnPosLength\n"));

	m_nCurrentPos = (REFERENCE_TIME)ulPosition*10000;
	m_nDuration = (REFERENCE_TIME)ulLength*10000;

    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnPresentationOpened()
{
	TRACE(_T("OnPresentationOpened\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnPresentationClosed()
{
	TRACE(_T("OnPresentationClosed\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnStatisticsChanged()
{
	TRACE(_T("OnStatisticsChanged\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnPreSeek(UINT32 ulOldTime, UINT32 ulNewTime)
{
	TRACE(_T("OnPreSeek\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnPostSeek(UINT32 ulOldTime, UINT32 ulNewTime)
{
	TRACE(_T("OnPostSeek\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnStop()
{
	TRACE(_T("OnStop\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnPause(UINT32 ulTime)
{
	TRACE(_T("OnPause\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnBegin(UINT32 ulTime)
{
	TRACE(_T("OnBegin\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnBuffering(UINT32 ulFlags, UINT16 unPercentComplete)
{
	TRACE(_T("OnBuffering\n"));
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::OnContacting(const char* pHostName)
{
	TRACE(_T("OnContacting\n"));
    return PNR_OK;
}

// IRMAAuthenticationManager

STDMETHODIMP CRealMediaSource::HandleAuthenticationRequest(IRMAAuthenticationManagerResponse* pResponse)
{
/*
	char username[1024];
	char password[1024];
*/
	// TODO: ask the user for l/p

	return PNR_FAIL;
/*
	pResponse->AuthenticationRequestDone(PNR_OK, username, password);

	return PNR_OK;

	return pResponse->AuthenticationRequestDone(PNR_NOT_AUTHORIZED, NULL, NULL);
*/
}

// IRMASiteSupplier

STDMETHODIMP CRealMediaSource::SitesNeeded(UINT32 uRequestID, IRMAValues* pProps)
{
    if(!pProps) return PNR_INVALID_PARAMETER;

	HRESULT hr = PNR_OK;

    CComPtr<IRMASiteWindowed> pSiteWindowed;
    hr = m_pCommonClassFactory->CreateInstance(CLSID_IRMASiteWindowed, (void**)&pSiteWindowed);
    if(PNR_OK != hr)
		return hr;

	CComQIPtr<IRMASite, &IID_IRMASite> pSite = pSiteWindowed;
	CComQIPtr<IRMAValues, &IID_IRMAValues> pSiteProps = pSiteWindowed;
    if(!pSite || !pSiteProps)
		return E_NOINTERFACE;

    IRMABuffer* pValue;

    hr = pProps->GetPropertyCString("playto", pValue);
    if(PNR_OK == hr)
    {
		pSiteProps->SetPropertyCString("channel", pValue);
		pValue->Release();
    }
    else
    {
		hr = pProps->GetPropertyCString("name", pValue);
		if(PNR_OK == hr)
		{
			pSiteProps->SetPropertyCString("LayoutGroup", pValue);
			pValue->Release();
		}
    }

    hr = pSiteWindowed->Create(NULL, WS_OVERLAPPED | WS_THICKFRAME | WS_VISIBLE | WS_CLIPCHILDREN);
//	hr = pSiteWindowed->Create(
//		((CMainFrame*)AfxGetApp()->m_pMainWnd)->m_wndView.m_hWnd, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
    if(PNR_OK != hr)
		return hr;


/*
	PNxWindow* pw = pSiteWindowed->GetWindow();
	HWND hwnd = (HWND)pw->window;
*/

    pSite->AttachWatcher(static_cast<IRMASiteWatcher*>(this));

    hr = m_pSiteManager->AddSite(pSite);
    if(PNR_OK != hr)
		return hr;

	g_pSite = pSiteWindowed;

    m_CreatedSites[uRequestID] = pSite.Detach();

    return hr;
}

STDMETHODIMP CRealMediaSource::SitesNotNeeded(UINT32 uRequestID)
{
    CComPtr<IRMASite> pSite;
	if(!m_CreatedSites.Lookup(uRequestID, *&pSite))
		return PNR_INVALID_PARAMETER;

    m_pSiteManager->RemoveSite(pSite);

    pSite->DetachWatcher();

	CComQIPtr<IRMASiteWindowed, &IID_IRMASiteWindowed>(pSite)->Destroy();

    m_CreatedSites.RemoveKey(uRequestID);

    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::BeginChangeLayout()
{
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::DoneChangeLayout()
{
    return PNR_OK;
}

// IRMASiteWatcher

STDMETHODIMP CRealMediaSource::AttachSite(IRMASite* pSite)
{
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::DetachSite()
{
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::ChangingPosition(PNxPoint posOld, REF(PNxPoint) posNew)
{
    return PNR_OK;
}

STDMETHODIMP CRealMediaSource::ChangingSize(PNxSize sizeOld, REF(PNxSize) sizeNew)
{
    sizeNew.cx += GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
    sizeNew.cy += GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);

    return PNR_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////

CRealMediaStream::CRealMediaStream(/*char* fn,*/ HRESULT* phr, CRealMediaSource* pParent, LPCWSTR pPinName) : 
	CSourceStream(NAME("RealMedia Stream"), phr, pParent, pPinName)
{
/*
	m_duration = 0;
	m_filesize = 0;
	m_data = NULL;
*/
}

CRealMediaStream::~CRealMediaStream()
{
//	if(m_data) {delete [] m_data; m_data = NULL;}
}
/*
bool CRealMediaStream::Open(CString fn)
{
	if(m_data) {delete [] m_data; m_data = NULL;}

	if(!m_sts.Open(fn, DEFAULT_CHARSET) || m_sts.GetSize() <= 0) return(false);

	m_duration = m_sts.TranslateEnd(m_sts.GetSize()-1, 23.976); // 23.976 will give us the worst case if we have a FRAME based sub (well, for ordenary dvd rips)

	m_name = m_sts.m_name;

	CFileStatus status;
	if(!CFile::GetStatus(fn, status)) return(false);

	m_filesize = status.m_size;

	CFile f;
	if(!f.Open(fn, CFile::modeRead|CFile::typeBinary)) return(false);

	m_data = new BYTE[m_filesize];
	if(!m_data) return(false);

	if(f.Read(m_data, m_filesize) != m_filesize)
	{
		delete [] m_data; m_data = NULL;
		return(false);
	};

	return(true);
}
*/
HRESULT CRealMediaStream::FillBuffer(IMediaSample* pms)
{
/*
    BYTE* pData;
    long lDataLen;

    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

	if(m_fRAWOutput)
	{
        CAutoLock cAutoLockShared(&m_cSharedState);

		if(!pData || !m_data || !m_filesize || m_fOutputDone) return(S_FALSE);

		BYTE* ptr = pData;

		{
			// Format ID
			strcpy((char*)ptr, __GAB2__); 
			ptr += strlen(__GAB2__)+1;
		}

		{
			int len = (m_name.GetLength()+1)*sizeof(WCHAR);
			*((ushort*)ptr) = __GAB1_LANGUAGE_UNICODE__; ptr += 2;
			*((uint*)ptr) = len; ptr += 4;
#ifdef UNICODE
			wcscpy((WCHAR*)ptr, m_name); ptr += len;
#else
			mbstowcs((WCHAR*)ptr, m_name, m_name.GetLength()+1); ptr += len;
#endif
		}

		{
			int len = m_filesize;
			*((ushort*)ptr) = __GAB1_RAWTEXTSUBTITLE__; ptr += 2;
			*((uint*)ptr) = len; ptr += 4;
			memcpy(ptr, m_data, len); ptr += len;
		}

		REFERENCE_TIME start = __int64(0)*10000, stop = __int64(m_duration)*10000;
        pms->SetTime(&start, &stop);

		pms->SetActualDataLength(ptr - pData);

		m_fOutputDone = true;
	}
	else
    {
        CAutoLock cAutoLockShared(&m_cSharedState);

		REFERENCE_TIME start, stop;
		start = m_sts.TranslateSegmentStart(m_currentidx, 23.976), 
		stop = m_sts.TranslateSegmentEnd(m_currentidx, 23.976);
		if(start == -1 && stop == -1) return S_FALSE;
		const STSSegment* stss = m_sts.SearchSubs((int)start, 23.976);

		start = __int64(start)*10000;
		stop = __int64(stop)*10000;
        pms->SetTime(&start, &stop);

		BYTE* ptr = pData;

		*ptr = 0;

		for(int i = 0; stss && i < stss->subs.GetSize(); i++, ptr++)
		{
			if(ptr != pData) ptr[-1] = '\n';
			m_sts.GetMBCSStr(stss->subs[i], (char*)ptr, 10240);
			ptr += strlen((char*)ptr);
		}

		pms->SetActualDataLength(ptr - pData);

		m_currentidx++;
	}

	pms->SetSyncPoint(TRUE);
*/
/*

		int len = m_sts.GetSize();
		if(len < 0 || m_currentidx >= len) 
			return S_FALSE;

		REFERENCE_TIME 
			start = __int64(m_start)*10000, 
			stop = start + __int64(SUBALIGN)*10000;

        pms->SetTime(&start, &stop);

		m_start += SUBALIGN;

		BYTE* ptr = pData;

		// Format ID
		strcpy((char*)ptr, __GAB1__); 
		ptr += strlen(__GAB1__)+1;

//		if(m_start == SUBALIGN)
		{
			if(m_sts.IsEntryUnicode(0)) // m_fUnicode)
			{
				int size = (m_sts.m_name.GetLength()+1)*sizeof(WCHAR);
			
				*((ushort*)ptr) = __GAB1_LANGUAGE_UNICODE__; ptr += 2;
				*((ushort*)ptr) = size;	ptr += 2;
			
#ifdef UNICODE
				wcscpy((WCHAR*)ptr, m_sts.m_name); ptr += size;
#else
				mbstowcs((WCHAR*)ptr, m_sts.m_name, m_sts.m_name.GetLength()+1); ptr += size; // TODO: Use user specified charset
#endif
			}
			else
			{
				int size = (m_sts.m_name.GetLength()+1)*sizeof(char);

				*((ushort*)ptr) = __GAB1_LANGUAGE__; ptr += 2;
				*((ushort*)ptr) = size;	ptr += 2;

#ifdef UNICODE
				wcstombs((char*)ptr, m_sts.m_name, m_sts.m_name.GetLength()+1); ptr += size;
#else
				strcpy((char*)ptr, m_sts.m_name); ptr += size;
#endif
			}
		}
		
		for(int i = m_currentidx; i < len && m_sts[i].start < m_start; i++)
		{
			STSEntry& stse = m_sts[i];

			int start = m_sts.TranslateStart(i, 25);
			int end = m_sts.TranslateEnd(i, 25);

			bool fUnicode = m_sts.IsEntryUnicode(i);

			CString str = 
#ifdef UNICODE
				fUnicode ? m_sts.GetUnicodeStr(i) : 
#endif
				m_sts.GetMBCSStr(i);

			int len = str.GetLength()+1;

#ifdef UNICODE
			if(fUnicode)
			{
				int size = 4+4+len*sizeof(WCHAR);

				*((ushort*)ptr) = __GAB1_ENTRY_UNICODE__; ptr += 2;
				*((ushort*)ptr) = size;	ptr += 2;
				memcpy(ptr, &start, 4); ptr += 4;
				memcpy(ptr, &end, 4); ptr += 4;

				wcscpy((WCHAR*)ptr, str); ptr += len*sizeof(WCHAR);
			}
			else
#endif
			{
				int size = 4+4+len*sizeof(char);
			
				*((ushort*)ptr) = __GAB1_ENTRY__; ptr += 2;
				*((ushort*)ptr) = size;	ptr += 2;
				memcpy(ptr, &start, 4); ptr += 4;
				memcpy(ptr, &end, 4); ptr += 4;

#ifdef UNICODE
				wcstombs((char*)ptr, str, len+1); ptr += len*sizeof(char);
#else
				strcpy((char*)ptr, str); ptr += len*sizeof(char);
#endif

			}
		}

		m_currentidx = i;

		pms->SetActualDataLength(ptr - pData);

    }
*/
//    pms->SetSyncPoint(TRUE);

    return NOERROR;
}

HRESULT CRealMediaStream::GetMediaType(int iPosition, CMediaType* pmt)
{
/*
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

    pmt->SetType(&MEDIATYPE_Text);
    pmt->SetSubtype(&MEDIASUBTYPE_None);
    pmt->SetFormatType(&FORMAT_None);
    pmt->SetTemporalCompression(FALSE);
	pmt->SetVariableSize();
*/
    return NOERROR;
}

HRESULT CRealMediaStream::CheckConnect(IPin* pPin)
{
/*
	bool fAccept = true;

	PIN_INFO pi;
	pPin->QueryPinInfo(&pi);
	if(pi.pFilter)
	{
		FILTER_INFO fi;
		pi.pFilter->QueryFilterInfo(&fi);

		if(fi.pGraph)
		{
			fAccept = (wcsstr(fi.achName, L"Internal Script Command Renderer") == NULL); // sorry, but no!
			m_fRAWOutput = (CComQIPtr<IConfigAviMux>(pi.pFilter) || CComQIPtr<IDirectVobSub>(pi.pFilter));

			fi.pGraph->Release();
		}

		pi.pFilter->Release();

		return(fAccept ? CBaseOutputPin::CheckConnect(pPin) : E_FAIL);
	}
*/	
	return E_FAIL;
}

HRESULT CRealMediaStream::CheckMediaType(const CMediaType* pMediaType)
{
/*
    CAutoLock cAutoLock(m_pFilter->pStateLock());

	return IsEqualGUID(*pMediaType->Type(), MEDIATYPE_Text) ? S_OK : E_INVALIDARG;
*/
	return E_INVALIDARG;
}

HRESULT CRealMediaStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;
/*
    pProperties->cBuffers = 1;
	pProperties->cbBuffer = 100 + m_filesize;

    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties, &Actual);
    if(FAILED(hr)) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    ASSERT(Actual.cBuffers == 1);
*/
    return NOERROR;
}

HRESULT CRealMediaStream::OnThreadCreate()
{
//    CAutoLock cAutoLockShared(&m_cSharedState);
/*
	m_fOutputDone = false;

	m_currentidx = 0;
*/
    return NOERROR;
}


