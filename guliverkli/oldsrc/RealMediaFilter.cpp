#include "stdafx.h"
#include <math.h>
#include <atlbase.h>
#include <streams.h>
#include <initguid.h>
#include "realmediafilter.h"

// CRealMediaFilter

CRealMediaFilter::CRealMediaFilter() 
	: CUnknown(NAME("RealMediaFilter"), NULL)
	, m_hWndParent(NULL), m_wndStyle(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN)
	, m_fpCreateEngine(NULL), m_fpCloseEngine(NULL), m_hRealMediaCore(NULL)
	, m_State(State_Stopped), m_nCurrent(0), m_nDuration(0)
	, m_VideoSize(0, 0)
{
}

CRealMediaFilter::~CRealMediaFilter()
{
	Deinit();
}

bool CRealMediaFilter::Init()
{
	CRegKey key;
	if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, _T("Software\\RealNetworks\\Preferences\\DT_Common")))
		return(false);

	TCHAR buff[MAX_PATH];
	ULONG len = sizeof(buff);
	if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len))
		return(false);

	if(!(m_hRealMediaCore = LoadLibrary(CString(buff) + _T("pnen3260.dll")))) 
		return(false);

	m_fpCreateEngine = (FPRMCREATEENGINE)GetProcAddress(m_hRealMediaCore, "CreateEngine");
	m_fpCloseEngine = (FPRMCLOSEENGINE)GetProcAddress(m_hRealMediaCore, "CloseEngine");
	
	if(m_fpCreateEngine == NULL || m_fpCloseEngine == NULL)
		return(false);

	if(PNR_OK != m_fpCreateEngine(&m_pEngine))
		return(false);

	if(PNR_OK != m_pEngine->CreatePlayer(*&m_pPlayer))
		return(false);

	if(!(m_pSiteManager = m_pPlayer) || !(m_pCommonClassFactory = m_pPlayer))
		return(false);

	m_pAudioPlayer = m_pPlayer;
	m_pAudioPlayer->AddPostMixHook(static_cast<IRMAAudioHook*>(this), FALSE, FALSE);
//	m_pVolume = m_pAudioPlayer->GetDeviceVolume();
	m_pVolume = m_pAudioPlayer->GetAudioVolume();

	// IRMAVolume::SetVolume has a huge latency when used via GetAudioVolume,
	// but by lowering this audio pushdown thing it can get better
	CComQIPtr<IRMAAudioPushdown, &IID_IRMAAudioPushdown> pAP = m_pAudioPlayer;
	if(pAP) pAP->SetAudioPushdown(300); // 100ms makes the playback sound choppy, 200ms looks ok, but for safety we set this to 300ms... :P

	CComQIPtr<IRMAErrorSinkControl, &IID_IRMAErrorSinkControl> pErrorSinkControl = m_pPlayer;
	if(pErrorSinkControl) pErrorSinkControl->AddErrorSink(static_cast<IRMAErrorSink*>(this), PNLOG_EMERG, PNLOG_INFO);

	if(PNR_OK != m_pPlayer->AddAdviseSink(static_cast<IRMAClientAdviseSink*>(this)))
		return(false);

	if(PNR_OK != m_pPlayer->SetClientContext((IUnknown*)(INonDelegatingUnknown*)(this)))
		return(false);

	return(true);
}

void CRealMediaFilter::Deinit()
{
	if(m_pPlayer)
	{
		m_pPlayer->Stop();

		CComQIPtr<IRMAErrorSinkControl, &IID_IRMAErrorSinkControl> pErrorSinkControl = m_pPlayer;
		if(pErrorSinkControl) pErrorSinkControl->RemoveErrorSink(static_cast<IRMAErrorSink*>(this));

		m_pPlayer->RemoveAdviseSink(static_cast<IRMAClientAdviseSink*>(this));

		m_pVolume = NULL;
		m_pAudioPlayer->RemovePostMixHook(static_cast<IRMAAudioHook*>(this));
		m_pAudioPlayer.Release();

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

CRealMediaFilter* CRealMediaFilter::CreateInstance()
{
	CRealMediaFilter* pRMF = new CRealMediaFilter();
	if(pRMF) pRMF->AddRef();
	return(pRMF);
}

#define QI(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

STDMETHODIMP CRealMediaFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return 
		QI(IPersist)
		QI(IMediaFilter)
		QI(IBaseFilter)
		QI(IEnumPins)
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		QI(IVideoWindow)
		QI(IBasicVideo)
		QI(IBasicAudio)
		QI(IRMAErrorSink)
		QI(IRMAClientAdviseSink)
		QI(IRMAAuthenticationManager)
		QI(IRMASiteSupplier)
		QI(IRMAPassiveSiteWatcher)
		QI(IRMAAudioHook)
		CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

// IDispatch
STDMETHODIMP CRealMediaFilter::GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {return E_NOTIMPL;}

// IPersist
STDMETHODIMP CRealMediaFilter::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = CLSID_RealMediaFilter, S_OK : E_POINTER;
}

// IMediaFilter
STDMETHODIMP CRealMediaFilter::Stop()
{
	m_nCurrent = 0;
	return (PNR_OK == m_pPlayer->Stop()) ? S_OK : E_FAIL;
}
STDMETHODIMP CRealMediaFilter::Pause()
{
	return (PNR_OK == m_pPlayer->Pause()) || m_State == State_Stopped ? S_OK : E_FAIL;
}
STDMETHODIMP CRealMediaFilter::Run(REFERENCE_TIME tStart)
{
	return (PNR_OK == m_pPlayer->Begin()) ? S_OK : E_FAIL; // TODO
}
STDMETHODIMP CRealMediaFilter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE* State)
{
	return State ? *State = m_State, S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaFilter::SetSyncSource(IReferenceClock* pClock) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetSyncSource(IReferenceClock** pClock) {return E_NOTIMPL;}

// IBaseFilter
STDMETHODIMP CRealMediaFilter::EnumPins(IEnumPins** ppEnum)
{
	return ppEnum ? *ppEnum = static_cast<IEnumPins*>(this), AddRef(), S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaFilter::FindPin(LPCWSTR Id, IPin** ppPin)
{
	return VFW_E_NOT_FOUND;
}
STDMETHODIMP CRealMediaFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	if(!pInfo) return E_POINTER;
	wcsncpy(pInfo->achName, m_name, 128);
	if(pInfo->pGraph = m_pGraph) pInfo->pGraph->AddRef();
	return S_OK;
}
STDMETHODIMP CRealMediaFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	if(pGraph)
	{
		Init();
	}
	else
	{
		Deinit();
	}

	m_pGraph = pGraph;
	m_name = pName;

	return S_OK;
}
STDMETHODIMP CRealMediaFilter::QueryVendorInfo(LPWSTR* pVendorInfo) {return E_NOTIMPL;}

// IEnumPins
STDMETHODIMP CRealMediaFilter::Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched)
{
    if(pcFetched) *pcFetched = 0;
	return S_FALSE;
}
STDMETHODIMP CRealMediaFilter::Skip(ULONG cPins) {return S_FALSE;}
STDMETHODIMP CRealMediaFilter::Reset() {return S_OK;}
STDMETHODIMP CRealMediaFilter::Clone(IEnumPins** ppEnum) {return VFW_E_ENUM_OUT_OF_SYNC;}

// IFileSourceFilter
STDMETHODIMP CRealMediaFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CStringA fn(pszFileName);
	if(fn.Find("://") < 0) fn = "file:" + fn;

	if(PNR_OK != m_pPlayer->OpenURL(fn)) 
		return E_FAIL;

	m_pPlayer->Stop();

	return S_OK;
}
STDMETHODIMP CRealMediaFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt) {return E_NOTIMPL;}

// IMediaSeeking
STDMETHODIMP CRealMediaFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = AM_SEEKING_CanSeekAbsolute|AM_SEEKING_CanGetCurrentPos|AM_SEEKING_CanGetDuration, S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;
	GetCapabilities(&caps);

	DWORD caps2 = caps & *pCapabilities;

	return caps2 == 0 ? E_FAIL : caps2 == *pCapabilities ? S_OK : S_FALSE;
}
STDMETHODIMP CRealMediaFilter::IsFormatSupported(const GUID* pFormat)
{
	return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}
STDMETHODIMP CRealMediaFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}
STDMETHODIMP CRealMediaFilter::GetTimeFormat(GUID* pFormat)
{
	return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}
STDMETHODIMP CRealMediaFilter::SetTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}
STDMETHODIMP CRealMediaFilter::GetDuration(LONGLONG* pDuration)
{
	return pDuration ? *pDuration = m_nDuration, S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaFilter::GetStopPosition(LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	return pCurrent ? *pCurrent = m_nCurrent, S_OK : E_POINTER;
}
STDMETHODIMP CRealMediaFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	m_nCurrent = *pCurrent;
	return (dwCurrentFlags&AM_SEEKING_AbsolutePositioning) 
		&& (PNR_OK == m_pPlayer->Seek((ULONG)(*pCurrent / 10000))) ? S_OK : E_FAIL;
}
STDMETHODIMP CRealMediaFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetRate(double dRate) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetRate(double* pdRate) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetPreroll(LONGLONG* pllPreroll) {return E_NOTIMPL;}

// IVideoWindow
STDMETHODIMP CRealMediaFilter::put_Caption(BSTR strCaption) {return E_NOTIMPL;}    
STDMETHODIMP CRealMediaFilter::get_Caption(BSTR* strCaption) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_WindowStyle(long WindowStyle)
{
	m_wndStyle = WindowStyle;
	return S_OK;
}
STDMETHODIMP CRealMediaFilter::get_WindowStyle(long* WindowStyle) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_WindowStyleEx(long WindowStyleEx) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_WindowStyleEx(long* WindowStyleEx) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_AutoShow(long AutoShow) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_AutoShow(long* AutoShow) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_WindowState(long WindowState) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_WindowState(long* WindowState) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_BackgroundPalette(long BackgroundPalette) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_BackgroundPalette(long* pBackgroundPalette) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_Visible(long Visible) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_Visible(long* pVisible) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_Left(long Left) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_Left(long* pLeft) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_Width(long Width) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_Width(long* pWidth) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_Top(long Top) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_Top(long* pTop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_Height(long Height) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_Height(long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_Owner(OAHWND Owner)
{
	return IsWindow((HWND)Owner) ? m_hWndParent = (HWND)Owner, S_OK : E_FAIL;
}
STDMETHODIMP CRealMediaFilter::get_Owner(OAHWND* Owner)
{
	return Owner ? *Owner = (OAHWND)m_hWndParent : E_FAIL;
}
STDMETHODIMP CRealMediaFilter::put_MessageDrain(OAHWND Drain) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_MessageDrain(OAHWND* Drain) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_BorderColor(long* Color) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_BorderColor(long Color) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_FullScreenMode(long* FullScreenMode) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_FullScreenMode(long FullScreenMode) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetWindowForeground(long Focus) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetWindowPosition(long Left, long Top, long Width, long Height)
{
	if(!m_pTheSite) return E_UNEXPECTED;

	m_wndWindowFrame.MoveWindow(CRect(CPoint(Left, Top), CSize(Width, Height)));

	return S_OK;
}
STDMETHODIMP CRealMediaFilter::GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetMinIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetMaxIdealImageSize(long* pWidth, long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetRestorePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::HideCursor(long HideCursor) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::IsCursorHidden(long* CursorHidden) {return E_NOTIMPL;}

// IBasicVideo
STDMETHODIMP CRealMediaFilter::get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_BitRate(long* pBitRate) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_BitErrorRate(long* pBitErrorRate) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_VideoWidth(long* pVideoWidth) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_VideoHeight(long* pVideoHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_SourceLeft(long SourceLeft) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_SourceLeft(long* pSourceLeft) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_SourceWidth(long SourceWidth) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_SourceWidth(long* pSourceWidth) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_SourceTop(long SourceTop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_SourceTop(long* pSourceTop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_SourceHeight(long SourceHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_SourceHeight(long* pSourceHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_DestinationLeft(long DestinationLeft) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_DestinationLeft(long* pDestinationLeft) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_DestinationWidth(long DestinationWidth) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_DestinationWidth(long* pDestinationWidth) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_DestinationTop(long DestinationTop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_DestinationTop(long* pDestinationTop) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::put_DestinationHeight(long DestinationHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_DestinationHeight(long* pDestinationHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetSourcePosition(long Left, long Top, long Width, long Height) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetDefaultSourcePosition() {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetDestinationPosition(long Left, long Top, long Width, long Height)// {return E_NOTIMPL;}
{
	if(!m_pTheSite) return E_UNEXPECTED;

	PNxPoint p = {Left, Top};
	PNxSize s = {Width, Height};
	m_wndDestFrame.MoveWindow(CRect(CPoint(Left, Top), CSize(Width, Height)));
	m_pTheSite->SetSize(s);

	return S_OK;
}
STDMETHODIMP CRealMediaFilter::GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SetDefaultDestinationPosition() {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetVideoSize(long* pWidth, long* pHeight)
{
	if(!pWidth || !pHeight) return E_POINTER;
	*pWidth = m_VideoSize.cx;
	*pHeight = m_VideoSize.cy;
	return S_OK;
}
STDMETHODIMP CRealMediaFilter::GetVideoPaletteEntries(long StartIndex, long Entries, long* pRetrieved, long* pPalette) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::GetCurrentImage(long* pBufferSize, long* pDIBImage) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::IsUsingDefaultSource() {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::IsUsingDefaultDestination() {return E_NOTIMPL;}

// IBasicAudio
STDMETHODIMP CRealMediaFilter::put_Volume(long lVolume)
{
	if(!m_pVolume) return E_UNEXPECTED;

	UINT16 volume = (lVolume == -10000) ? 0 : (int)pow(10, ((double)lVolume)/5000+2);
	volume = max(min(volume, 100), 0);

	return PNR_OK == m_pVolume->SetVolume(volume) ? S_OK : E_FAIL;
}
STDMETHODIMP CRealMediaFilter::get_Volume(long* plVolume)
{
	if(!m_pVolume) return E_UNEXPECTED;

	CheckPointer(plVolume, E_POINTER);

	UINT16 volume = m_pVolume->GetVolume();
	volume = (int)((log10(volume)-2)*5000);
	volume = max(min(volume, 0), -10000);

	*plVolume = volume;

	return S_OK;
}
STDMETHODIMP CRealMediaFilter::put_Balance(long lBalance) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::get_Balance(long* plBalance) {return E_NOTIMPL;}

// IRMAErrorSink
STDMETHODIMP CRealMediaFilter::ErrorOccurred(const UINT8 unSeverity, const UINT32 ulRMACode, const UINT32 ulUserCode, const char* pUserString, const char* pMoreInfoURL)
{
	if(m_pGraph)
		CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_RM_ERROR, 0, 0);
	return PNR_OK;
}

// IRMAClientAdviseSink
STDMETHODIMP CRealMediaFilter::OnPosLength(UINT32 ulPosition, UINT32 ulLength)
{
	bool fLengthChanged = (m_nDuration != (REFERENCE_TIME)ulLength*10000);
	m_nCurrent = (REFERENCE_TIME)ulPosition*10000;
	m_nDuration = (REFERENCE_TIME)ulLength*10000;
	if(fLengthChanged) CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_LENGTH_CHANGED, 0, 0);
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnPresentationOpened() {return PNR_OK;}
STDMETHODIMP CRealMediaFilter::OnPresentationClosed() {return PNR_OK;}
STDMETHODIMP CRealMediaFilter::OnStatisticsChanged() {return PNR_OK;}
STDMETHODIMP CRealMediaFilter::OnPreSeek(UINT32 ulOldTime, UINT32 ulNewTime)
{
	m_nCurrent = (REFERENCE_TIME)ulNewTime*10000;
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnPostSeek(UINT32 ulOldTime, UINT32 ulNewTime)
{
	m_nCurrent = (REFERENCE_TIME)ulNewTime*10000;
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnStop()
{
	m_nCurrent = 0;
	m_State = State_Stopped;
	CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_COMPLETE, 0, 0);
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnPause(UINT32 ulTime)
{
	m_State = State_Paused;
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnBegin(UINT32 ulTime)
{
	m_State = State_Running;
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnBuffering(UINT32 ulFlags, UINT16 unPercentComplete)
{
	TRACE(_T("OnBuffering: %d%%\n"), unPercentComplete);
	return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::OnContacting(const char* pHostName) {return PNR_OK;}

// IRMAAuthenticationManager
STDMETHODIMP CRealMediaFilter::HandleAuthenticationRequest(IRMAAuthenticationManagerResponse* pResponse) {return E_NOTIMPL;}

// IRMASiteSupplier
STDMETHODIMP CRealMediaFilter::SitesNeeded(UINT32 uRequestID, IRMAValues* pProps)
{
    if(!pProps) return PNR_INVALID_PARAMETER;

	if(m_pTheSite || m_pTheSite2) return PNR_UNEXPECTED; // we only have one view...

	HRESULT hr = PNR_OK;

    CComPtr<IRMASiteWindowed> pSiteWindowed;
    hr = m_pCommonClassFactory->CreateInstance(CLSID_IRMASiteWindowed, (void**)&pSiteWindowed);
    if(PNR_OK != hr)
		return hr;

    if(!(m_pTheSite = pSiteWindowed) || !(m_pTheSite2 = pSiteWindowed))
		return E_NOINTERFACE;

	CComQIPtr<IRMAValues, &IID_IRMAValues> pSiteProps = pSiteWindowed;
    if(!pSiteProps)
		return E_NOINTERFACE;

    IRMABuffer* pValue;

	// no idea what these supposed to do...
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

	if(!m_wndWindowFrame.Create(NULL, NULL, m_wndStyle|WS_VISIBLE,
		CRect(0, 0, 0, 0), CWnd::FromHandle(m_hWndParent), 0, NULL))
		return E_FAIL;

	if(!m_wndDestFrame.Create(NULL, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
		CRect(0, 0, 0, 0), &m_wndWindowFrame, 0, NULL))
		return E_FAIL;

	hr = pSiteWindowed->Create(m_wndDestFrame.m_hWnd, WS_DISABLED|WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
    if(PNR_OK != hr)
		return hr;

    m_pTheSite2->AddPassiveSiteWatcher(static_cast<IRMAPassiveSiteWatcher*>(this));

    hr = m_pSiteManager->AddSite(m_pTheSite);
    if(PNR_OK != hr)
		return hr;

	CComQIPtr<IRMASite, &IID_IRMASite> pSite = pSiteWindowed;
    m_CreatedSites[uRequestID] = pSite.Detach();

    return hr;
}
STDMETHODIMP CRealMediaFilter::SitesNotNeeded(UINT32 uRequestID)
{
    CComPtr<IRMASite> pSite;
	if(!m_CreatedSites.Lookup(uRequestID, *&pSite))
		return PNR_INVALID_PARAMETER;

    m_pSiteManager->RemoveSite(pSite);

    m_pTheSite2->RemovePassiveSiteWatcher(static_cast<IRMAPassiveSiteWatcher*>(this));

	m_pTheSite.Release();
	m_pTheSite2.Release();

	CComQIPtr<IRMASiteWindowed, &IID_IRMASiteWindowed>(pSite)->Destroy();

	m_wndDestFrame.DestroyWindow();
	m_wndWindowFrame.DestroyWindow();

    m_CreatedSites.RemoveKey(uRequestID);

    return PNR_OK;
}
STDMETHODIMP CRealMediaFilter::BeginChangeLayout() {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::DoneChangeLayout()
{
	CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_RM_VIDEO_CHANGED, MAKELPARAM(m_VideoSize.cx, m_VideoSize.cy), 0);
	return PNR_OK;
}

// IRMAPassiveSiteWatcher
STDMETHODIMP CRealMediaFilter::PositionChanged(PNxPoint* pos) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::SizeChanged(PNxSize* size)
{
	if(m_VideoSize.cx == 0 || m_VideoSize.cy == 0)
	{
		m_VideoSize.cx = size->cx;
		m_VideoSize.cy = size->cy;
	}
	return PNR_OK;
}

// IRMAAudioHook
STDMETHODIMP CRealMediaFilter::OnBuffer(RMAAudioData* pAudioInData, RMAAudioData* pAudioOutData) {return E_NOTIMPL;}
STDMETHODIMP CRealMediaFilter::OnInit(RMAAudioFormat* pFormat)
{
	CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_RM_AUDIO_CHANGED, pFormat->uChannels, 0);
	return PNR_OK;
}

