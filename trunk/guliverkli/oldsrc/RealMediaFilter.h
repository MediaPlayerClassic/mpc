#pragma once

#include <afxtempl.h>

#include <dshow.h>

#include "pntypes.h"
#include "pnwintyp.h"
#include "pncom.h"
#include "rmapckts.h"
#include "rmacomm.h"
#include "rmamon.h"
#include "rmafiles.h"
#include "rmaengin.h"
#include "rmacore.h"
#include "rmaclsnk.h"
#include "rmaerror.h"
#include "rmaauth.h"
#include "rmawin.h"
#include "rmasite2.h"
#include "rmaausvc.h"

// {DD6958EF-4B90-4131-B9B0-21FCEC6CB377}
DEFINE_GUID(CLSID_RealMediaFilter, 
0xdd6958ef, 0x4b90, 0x4131, 0xb9, 0xb0, 0x21, 0xfc, 0xec, 0x6c, 0xb3, 0x77);

#include "RealMediaPlayer.h"
/*
#define EC_RM_VIDEO_CHANGED (EC_USER+1)
#define EC_RM_AUDIO_CHANGED (EC_USER+2)
#define EC_RM_ERROR (EC_USER+3)
*/
class CRealMediaFilter
	: public CUnknown
//	, public IGraphBuilder
//	, public IMediaControl
//	, public IMediaEventEx
	, public IBaseFilter
	, public IEnumPins
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IVideoWindow
	, public IBasicVideo
	, public IBasicAudio
	, public IRMAErrorSink
	, public IRMAClientAdviseSink
	, public IRMAAuthenticationManager
	, public IRMASiteSupplier
	, public IRMAPassiveSiteWatcher
	, public IRMAAudioHook
{
protected:
	CRealMediaFilter();
	virtual ~CRealMediaFilter();

private:
	HWND m_hWndParent;
	DWORD m_wndStyle;
	CRealMediaPlayerWindow m_wndWindowFrame, m_wndDestFrame;
	CSize m_VideoSize;

	//

	FPRMCREATEENGINE	m_fpCreateEngine;
	FPRMCLOSEENGINE	 	m_fpCloseEngine;
	HMODULE				m_hRealMediaCore;

	CComPtr<IRMAClientEngine> m_pEngine;
    CComPtr<IRMAPlayer> m_pPlayer;
	CComQIPtr<IRMAAudioPlayer, &IID_IRMAAudioPlayer> m_pAudioPlayer;
	CComPtr<IRMAVolume> m_pVolume;
    CComQIPtr<IRMASiteManager, &IID_IRMASiteManager> m_pSiteManager;
    CComQIPtr<IRMACommonClassFactory, &IID_IRMACommonClassFactory> m_pCommonClassFactory;

	CComQIPtr<IRMASite, &IID_IRMASite> m_pTheSite;
	CComQIPtr<IRMASite2, &IID_IRMASite2> m_pTheSite2;
	CMap<UINT32, UINT32&, IRMASite*, IRMASite*&> m_CreatedSites;

	//

	CStringW m_name;
	CComPtr<IFilterGraph> m_pGraph;
	FILTER_STATE m_State;
	REFERENCE_TIME m_nCurrent, m_nDuration;

	//

	bool Init();
	void Deinit();

public:
    DECLARE_IUNKNOWN;

	static CRealMediaFilter* CRealMediaFilter::CreateInstance();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    // IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);

	// IPersist
	STDMETHODIMP GetClassID(CLSID* pClassID);

    // IMediaFilter
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, FILTER_STATE* State);
	STDMETHODIMP SetSyncSource(IReferenceClock* pClock);
	STDMETHODIMP GetSyncSource(IReferenceClock** pClock);

    // IBaseFilter
	STDMETHODIMP EnumPins(IEnumPins** ppEnum);
	STDMETHODIMP FindPin(LPCWSTR Id, IPin** ppPin);
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
	STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName);
	STDMETHODIMP QueryVendorInfo(LPWSTR* pVendorInfo);

    // IEnumPins
    STDMETHODIMP Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched);
    STDMETHODIMP Skip(ULONG cPins);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumPins** ppEnum);

	// IFileSourceFilter
    STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	// IVideoWindow
    STDMETHODIMP put_Caption(BSTR strCaption);    
    STDMETHODIMP get_Caption(BSTR* strCaption);
	STDMETHODIMP put_WindowStyle(long WindowStyle);
	STDMETHODIMP get_WindowStyle(long* WindowStyle);
	STDMETHODIMP put_WindowStyleEx(long WindowStyleEx);
	STDMETHODIMP get_WindowStyleEx(long* WindowStyleEx);
	STDMETHODIMP put_AutoShow(long AutoShow);
	STDMETHODIMP get_AutoShow(long* AutoShow);
	STDMETHODIMP put_WindowState(long WindowState);
	STDMETHODIMP get_WindowState(long* WindowState);
	STDMETHODIMP put_BackgroundPalette(long BackgroundPalette);
	STDMETHODIMP get_BackgroundPalette(long* pBackgroundPalette);
	STDMETHODIMP put_Visible(long Visible);
	STDMETHODIMP get_Visible(long* pVisible);
	STDMETHODIMP put_Left(long Left);
	STDMETHODIMP get_Left(long* pLeft);
	STDMETHODIMP put_Width(long Width);
	STDMETHODIMP get_Width(long* pWidth);
	STDMETHODIMP put_Top(long Top);
	STDMETHODIMP get_Top(long* pTop);
	STDMETHODIMP put_Height(long Height);
	STDMETHODIMP get_Height(long* pHeight);
	STDMETHODIMP put_Owner(OAHWND Owner);
	STDMETHODIMP get_Owner(OAHWND* Owner);
	STDMETHODIMP put_MessageDrain(OAHWND Drain);
	STDMETHODIMP get_MessageDrain(OAHWND* Drain);
	STDMETHODIMP get_BorderColor(long* Color);
	STDMETHODIMP put_BorderColor(long Color);
	STDMETHODIMP get_FullScreenMode(long* FullScreenMode);
	STDMETHODIMP put_FullScreenMode(long FullScreenMode);
    STDMETHODIMP SetWindowForeground(long Focus);
    STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd, long uMsg, LONG_PTR wParam, LONG_PTR lParam);
    STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height);
	STDMETHODIMP GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
	STDMETHODIMP GetMinIdealImageSize(long* pWidth, long* pHeight);
	STDMETHODIMP GetMaxIdealImageSize(long* pWidth, long* pHeight);
	STDMETHODIMP GetRestorePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
	STDMETHODIMP HideCursor(long HideCursor);
	STDMETHODIMP IsCursorHidden(long* CursorHidden);

	// IBasicVideo
    STDMETHODIMP get_AvgTimePerFrame(REFTIME* pAvgTimePerFrame);
    STDMETHODIMP get_BitRate(long* pBitRate);
    STDMETHODIMP get_BitErrorRate(long* pBitErrorRate);
    STDMETHODIMP get_VideoWidth(long* pVideoWidth);
    STDMETHODIMP get_VideoHeight(long* pVideoHeight);
    STDMETHODIMP put_SourceLeft(long SourceLeft);
    STDMETHODIMP get_SourceLeft(long* pSourceLeft);
    STDMETHODIMP put_SourceWidth(long SourceWidth);
    STDMETHODIMP get_SourceWidth(long* pSourceWidth);
    STDMETHODIMP put_SourceTop(long SourceTop);
    STDMETHODIMP get_SourceTop(long* pSourceTop);
    STDMETHODIMP put_SourceHeight(long SourceHeight);
    STDMETHODIMP get_SourceHeight(long* pSourceHeight);
    STDMETHODIMP put_DestinationLeft(long DestinationLeft);
    STDMETHODIMP get_DestinationLeft(long* pDestinationLeft);
    STDMETHODIMP put_DestinationWidth(long DestinationWidth);
    STDMETHODIMP get_DestinationWidth(long* pDestinationWidth);
    STDMETHODIMP put_DestinationTop(long DestinationTop);
    STDMETHODIMP get_DestinationTop(long* pDestinationTop);
    STDMETHODIMP put_DestinationHeight(long DestinationHeight);
    STDMETHODIMP get_DestinationHeight(long* pDestinationHeight);
    STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height);
    STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
    STDMETHODIMP SetDefaultSourcePosition();
    STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height);
    STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
    STDMETHODIMP SetDefaultDestinationPosition();
    STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight);
    STDMETHODIMP GetVideoPaletteEntries(long StartIndex, long Entries, long* pRetrieved, long* pPalette);
    STDMETHODIMP GetCurrentImage(long* pBufferSize, long* pDIBImage);
    STDMETHODIMP IsUsingDefaultSource();
    STDMETHODIMP IsUsingDefaultDestination();

	// IBasicAudio
    STDMETHODIMP put_Volume(long lVolume);
    STDMETHODIMP get_Volume(long* plVolume);
    STDMETHODIMP put_Balance(long lBalance);
    STDMETHODIMP get_Balance(long* plBalance);

	// IRMAErrorSink
    STDMETHODIMP ErrorOccurred(const UINT8 unSeverity, const UINT32 ulRMACode, const UINT32 ulUserCode, const char* pUserString, const char* pMoreInfoURL);

    // IRMAClientAdviseSink
    STDMETHODIMP OnPosLength(UINT32 ulPosition, UINT32 ulLength);
    STDMETHODIMP OnPresentationOpened();
    STDMETHODIMP OnPresentationClosed();
    STDMETHODIMP OnStatisticsChanged();
    STDMETHODIMP OnPreSeek(UINT32 ulOldTime, UINT32 ulNewTime);
    STDMETHODIMP OnPostSeek(UINT32 ulOldTime, UINT32 ulNewTime);
    STDMETHODIMP OnStop();
    STDMETHODIMP OnPause(UINT32 ulTime);
    STDMETHODIMP OnBegin(UINT32 ulTime);
    STDMETHODIMP OnBuffering(UINT32 ulFlags, UINT16 unPercentComplete);
    STDMETHODIMP OnContacting(const char* pHostName);

	// IRMAAuthenticationManager
    STDMETHODIMP HandleAuthenticationRequest(IRMAAuthenticationManagerResponse* pResponse);

	// IRMASiteSupplier
    STDMETHODIMP SitesNeeded(UINT32 uRequestID, IRMAValues* pSiteProps);
    STDMETHODIMP SitesNotNeeded(UINT32 uRequestID);
    STDMETHODIMP BeginChangeLayout();
    STDMETHODIMP DoneChangeLayout();

	// IRMAPassiveSiteWatcher
    STDMETHODIMP PositionChanged(PNxPoint* pos);
	STDMETHODIMP SizeChanged(PNxSize* size);

	// IRMAAudioHook
	STDMETHODIMP OnBuffer(RMAAudioData* pAudioInData, RMAAudioData* pAudioOutData);
	STDMETHODIMP OnInit(RMAAudioFormat* pFormat);
};
