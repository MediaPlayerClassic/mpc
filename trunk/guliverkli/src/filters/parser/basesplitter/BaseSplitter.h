#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include <qnetwork.h>
#include "..\..\..\..\include\IChapterInfo.h"

class Packet
{
public:
	virtual ~Packet() {}
	DWORD TrackNumber;
	BOOL bDiscontinuity, bSyncPoint;
	REFERENCE_TIME rtStart, rtStop;
	CArray<BYTE> pData;
};

class CAsyncFileReader : public CUnknown, public IAsyncReader, public CFile
{
public:
	CAsyncFileReader(CString fn, HRESULT& hr);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAsyncReader

	STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) {return E_NOTIMPL;}
    STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser) {return E_NOTIMPL;}
    STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) {return E_NOTIMPL;}
	STDMETHODIMP SyncReadAligned(IMediaSample* pSample) {return E_NOTIMPL;}
	STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
	STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
	STDMETHODIMP BeginFlush() {return E_NOTIMPL;}
	STDMETHODIMP EndFlush() {return E_NOTIMPL;}
};

class CBaseSplitterFilter;

class CBaseSplitterInputPin : public CBasePin
{
protected:
	CComQIPtr<IAsyncReader> m_pAsyncReader;

public:
	CBaseSplitterInputPin(TCHAR* pName, CBaseSplitterFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseSplitterInputPin();

	HRESULT GetAsyncReader(IAsyncReader** ppAsyncReader);

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT CheckMediaType(const CMediaType* pmt);

    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pPin);

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
};

class CBaseSplitterOutputPin : public CBaseOutputPin, protected CAMThread
{
	CArray<CMediaType> m_mts;

private:
	CCritSec m_csQueueLock;
	CAutoPtrList<Packet> m_packets;
	HRESULT m_hrDeliver;

	bool m_fFlushing, m_fFlushed;
	CAMEvent m_eEndFlush;

	enum {CMD_EXIT};
    DWORD ThreadProc();

	void MakeISCRHappy();

protected:
	REFERENCE_TIME m_rtStart;

	// override this if you need some second level stream specific demuxing (optional)
	// the default implementation will send the sample as is
	virtual HRESULT DeliverPacket(CAutoPtr<Packet> p);

public:
	CBaseSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseSplitterOutputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	CMediaType& CurrentMediaType() {return m_mt;}

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

	// Queueing

	HRESULT Active();
    HRESULT Inactive();

    HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

    HRESULT QueueEndOfStream();
	HRESULT QueuePacket(CAutoPtr<Packet> p);
};

class CBaseSplitterFilter 
	: public CBaseFilter
	, public CCritSec
	, protected CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IAMOpenProgress
	, public IAMMediaContent
	, public IChapterInfo
{
protected:
	CStringW m_fn;

	CAutoPtr<CBaseSplitterInputPin> m_pInput;
	CAutoPtrList<CBaseSplitterOutputPin> m_pOutputs;
	CMap<DWORD, DWORD, CBaseSplitterOutputPin*, CBaseSplitterOutputPin*> m_pPinMap;

	LONGLONG m_nOpenProgress;
	bool m_fAbort;

	REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;
	double m_dRate;

	CList<UINT64> m_bDiscontinuitySent;
	CList<CBaseSplitterOutputPin*> m_pActivePins;

	CAMEvent m_eEndFlush;
	bool m_fFlushing;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	HRESULT DeliverPacket(CAutoPtr<Packet> p);

	// override this ...
	virtual HRESULT CreateOutputs(IAsyncReader* pAsyncReader) = 0; 

protected:
	enum {CMD_EXIT, CMD_SEEK};
    DWORD ThreadProc();

	// ... and also override all these too
	virtual bool InitDeliverLoop() = 0;
	virtual void SeekDeliverLoop(REFERENCE_TIME rt) = 0;
	virtual void DoDeliverLoop() = 0;

protected:
	enum mctype {AuthorName, Title, Rating, Description, Copyright, MCLast};
	CStringW m_mcs[MCLast];
	HRESULT GetMediaContentStr(BSTR* pBSTR, mctype type);
	HRESULT SetMediaContentStr(CStringW str, mctype type);

public:
	CBaseSplitterFilter(LPCTSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseSplitterFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT BreakConnect(PIN_DIRECTION dir, CBasePin* pPin);
	HRESULT CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

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
	STDMETHODIMP GetDuration(LONGLONG* pDuration) = 0; // and the final thing to override here
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	// IAMOpenProgress

	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
	STDMETHODIMP AbortOperation();

	// IDispatch

	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {return E_NOTIMPL;}

	// IAMMediaContent

	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating);
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) {return E_NOTIMPL;}

	// IChapterInfo

	STDMETHODIMP_(UINT) GetChapterCount(UINT aChapterID);
	STDMETHODIMP_(UINT) GetChapterId(UINT aParentChapterId, UINT aIndex);
	STDMETHODIMP_(UINT) GetChapterCurrentId();
	STDMETHODIMP_(BOOL) GetChapterInfo(UINT aChapterID, struct ChapterElement* pStructureToFill);
	STDMETHODIMP_(BSTR) GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2]);
};

