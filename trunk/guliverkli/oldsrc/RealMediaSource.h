#pragma once

#include <afxtempl.h>

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

class CRealMediaSource : 
		public CSource,
//		public CPersistStream,
		public IFileSourceFilter,
		public IMediaSeeking,
//		public IAMMediaContent,
		public IRMAErrorSink,
		public IRMAClientAdviseSink,
		public IRMAAuthenticationManager,
		public IRMASiteSupplier,
		public IRMASiteWatcher
{

	FPRMCREATEENGINE	m_fpCreateEngine;
	FPRMCLOSEENGINE	 	m_fpCloseEngine;
	HMODULE				m_hRealMediaCore;

    CComPtr<IRMAPlayer> m_pPlayer;
    CComPtr<IRMAClientEngine> m_pEngine;
    CComQIPtr<IRMASiteManager, &IID_IRMASiteManager> m_pSiteManager;
    CComQIPtr<IRMACommonClassFactory, &IID_IRMACommonClassFactory> m_pCommonClassFactory;

	CMap<UINT32, UINT32&, IRMASite*, IRMASite*&> m_CreatedSites;

    REFERENCE_TIME m_nCurrentPos, m_nDuration;

public:
    DECLARE_IUNKNOWN;
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName);

/*
    // CPersistStream override
	DWORD GetSoftwareVersion();
    HRESULT WriteToStream(IStream* pStream);
    HRESULT ReadFromStream(IStream* pStream);
    STDMETHODIMP GetClassID(CLSID* pClsid);
*/
	// IFileSourceFilter interface
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE *pmt);
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt);

	// IMediaSeeking interface
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

	// IRMASiteWatcher
    STDMETHODIMP AttachSite(IRMASite* pSite);
    STDMETHODIMP DetachSite();
    STDMETHODIMP ChangingPosition(PNxPoint posOld, REF(PNxPoint) posNew);
	STDMETHODIMP ChangingSize(PNxSize sizeOld, REF(PNxSize) sizeNew);

private:
    CRealMediaSource(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CRealMediaSource();

//	CCritSec m_propsLock;

	CString m_fn;
};

class CRealMediaStream : public CSourceStream
{
public:
    CRealMediaStream(/*char* fn, */HRESULT* phr, CRealMediaSource* pParent, LPCWSTR pPinName);
	virtual ~CRealMediaStream();

    HRESULT FillBuffer(IMediaSample* pms);

    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);

	HRESULT CheckConnect(IPin* pPin);
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

    HRESULT OnThreadCreate();

//	bool Open(CString fn);

private:
/*    CCritSec m_cSharedState;

	bool m_fRAWOutput;

	CSimpleTextSubtitle m_sts;
	
	CString m_name;
	int m_duration, m_filesize;
	BYTE* m_data;
	bool m_fOutputDone;

	int m_currentidx;
*/
};

