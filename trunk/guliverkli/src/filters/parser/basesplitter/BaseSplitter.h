#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include <qnetwork.h>
#include "..\..\..\..\include\IChapterInfo.h"
#include "..\..\..\..\include\IKeyFrameInfo.h"

class Packet
{
public:
	DWORD TrackNumber;
	BOOL bDiscontinuity, bSyncPoint;
	static const REFERENCE_TIME INVALID_TIME = ~0;
	REFERENCE_TIME rtStart, rtStop;
	CArray<BYTE> pData;
	AM_MEDIA_TYPE* pmt;
	Packet() {pmt = NULL; bDiscontinuity = FALSE;}
	virtual ~Packet() {if(pmt) DeleteMediaType(pmt);}
	virtual int GetSize() {return pData.GetSize();}
};

class CPacketQueue : public CCritSec, protected CAutoPtrList<Packet>
{
	int m_size;

public:
	CPacketQueue() : m_size(0) {}

	void Add(CAutoPtr<Packet> p)
	{
		CAutoLock cAutoLock(this);
		if(p) m_size += p->GetSize();
		AddTail(p);
	}

	CAutoPtr<Packet> Remove()
	{
		CAutoLock cAutoLock(this);
		ASSERT(__super::GetCount() > 0);
		CAutoPtr<Packet> p = RemoveHead();
		if(p) m_size -= p->GetSize();
		return p;
	}

	void RemoveAll()
	{
		CAutoLock cAutoLock(this);
		m_size = 0;
		__super::RemoveAll();
	}

	int GetCount()
	{
		CAutoLock cAutoLock(this); 
		return __super::GetCount();
	}
	
	int GetSize()
	{
		CAutoLock cAutoLock(this); 
		return m_size;
	}

};

[uuid("7D55F67A-826E-40B9-8A7D-3DF0CBBD272D")]
interface IFileHandle : public IUnknown
{
	STDMETHOD_(HANDLE, GetFileHandle)() = 0;
};

class CAsyncFileReader : public CUnknown, public IAsyncReader, public CFile, public IFileHandle
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

	// IFileHandle

	STDMETHODIMP_(HANDLE) GetFileHandle();
};

class CBaseSplitterFile
{
	CComPtr<IAsyncReader> m_pAsyncReader;
	CAutoVectorPtr<BYTE> m_pCache;
	__int64 m_cachepos, m_cachelen, m_cachetotal;

protected:
	UINT64 m_bitbuff;
	int m_bitlen;

	__int64 m_pos, m_len;

public:
	CBaseSplitterFile(IAsyncReader* pReader, HRESULT& hr, int cachelen = 2048);
	virtual ~CBaseSplitterFile() {}

	__int64 GetPos() {return m_pos - (m_bitlen>>3);}
	__int64 GetLength() {return m_len;}
	virtual void Seek(__int64 pos) {m_pos = min(max(pos, 0), m_len); BitFlush();}
	virtual HRESULT Read(BYTE* pData, __int64 len);

	UINT64 BitRead(int nBits, bool fPeek = false);
	void BitByteAlign(), BitFlush();
	HRESULT ByteRead(BYTE* pData, __int64 len);
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

class CBaseSplitterOutputPin : public CBaseOutputPin, protected CAMThread, public IMediaSeeking
{
protected:
	CArray<CMediaType> m_mts;
	int m_nBuffers;

private:
	CPacketQueue m_queue;

	HRESULT m_hrDeliver;

	bool m_fFlushing, m_fFlushed;
	CAMEvent m_eEndFlush;

	enum {CMD_EXIT};
    DWORD ThreadProc();

	void MakeISCRHappy();

	// please only use DeliverPacket from the derived class
    HRESULT GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
    HRESULT Deliver(IMediaSample* pSample);

protected:
	REFERENCE_TIME m_rtStart;

	// override this if you need some second level stream specific demuxing (optional)
	// the default implementation will send the sample as is
	virtual HRESULT DeliverPacket(CAutoPtr<Packet> p);

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

public:
	CBaseSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int nBuffers = 0);
	CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int nBuffers = 0);
	virtual ~CBaseSplitterOutputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT SetName(LPCWSTR pName);

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

	int QueueCount();
	int QueueSize();
    HRESULT QueueEndOfStream();
	HRESULT QueuePacket(CAutoPtr<Packet> p);

	// returns true for everything which (the lack of) would not block other streams (subtitle streams, basically)
	virtual bool IsDiscontinuous();
};

[uuid("46070104-1318-4A82-8822-E99AB7CD15C1")]
interface IBufferInfo : public IUnknown
{
	STDMETHOD_(int, GetCount()) = 0;
	STDMETHOD(GetStatus(int i, int& samples, int& size)) = 0;
};

class CBaseSplitterFilter 
	: public CBaseFilter
	, public CCritSec
	, protected CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IAMOpenProgress
	, public IAMMediaContent
	, public IAMExtendedSeeking
	, public IChapterInfo
	, public IKeyFrameInfo
	, public IBufferInfo
{
	CCritSec m_csPinMap;
	CMap<DWORD, DWORD, CBaseSplitterOutputPin*, CBaseSplitterOutputPin*> m_pPinMap;

	CCritSec m_csmtnew;
	CMap<DWORD, DWORD, CMediaType, CMediaType> m_mtnew;

protected:
	CStringW m_fn;

	CAutoPtr<CBaseSplitterInputPin> m_pInput;
	CAutoPtrList<CBaseSplitterOutputPin> m_pOutputs;

	CBaseSplitterOutputPin* GetOutputPin(DWORD TrackNum);
	HRESULT AddOutputPin(DWORD TrackNum, CAutoPtr<CBaseSplitterOutputPin> pPin);
	HRESULT RenameOutputPin(DWORD TrackNumSrc, DWORD TrackNumDst, const AM_MEDIA_TYPE* pmt);
	virtual HRESULT DeleteOutputs();
	virtual HRESULT CreateOutputs(IAsyncReader* pAsyncReader) = 0; // override this ...

	LONGLONG m_nOpenProgress;
	bool m_fAbort;

	REFERENCE_TIME m_rtDuration; // derived filter should set this at the end of CreateOutputs
	REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;
	double m_dRate;

	CList<UINT64> m_bDiscontinuitySent;
	CList<CBaseSplitterOutputPin*> m_pActivePins;

	CAMEvent m_eEndFlush;
	bool m_fFlushing;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	HRESULT DeliverPacket(CAutoPtr<Packet> p);

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

	bool IsAnyPinDrying();

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

private:
	REFERENCE_TIME m_rtLastStart, m_rtLastStop;
	CList<void*> m_LastSeekers;

public:
	virtual HRESULT SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

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

	// IAMExtendedSeeking

	STDMETHODIMP get_ExSeekCapabilities(long* pExCapabilities);
	STDMETHODIMP get_MarkerCount(long* pMarkerCount);
	STDMETHODIMP get_CurrentMarker(long* pCurrentMarker);
	STDMETHODIMP GetMarkerTime(long MarkerNum, double* pMarkerTime);
	STDMETHODIMP GetMarkerName(long MarkerNum, BSTR* pbstrMarkerName);
	STDMETHODIMP put_PlaybackSpeed(double Speed) {return E_NOTIMPL;}
	STDMETHODIMP get_PlaybackSpeed(double* pSpeed) {return E_NOTIMPL;}

	// IChapterInfo

	STDMETHODIMP_(UINT) GetChapterCount(UINT aChapterID);
	STDMETHODIMP_(UINT) GetChapterId(UINT aParentChapterId, UINT aIndex);
	STDMETHODIMP_(UINT) GetChapterCurrentId();
	STDMETHODIMP_(BOOL) GetChapterInfo(UINT aChapterID, struct ChapterElement* pStructureToFill);
	STDMETHODIMP_(BSTR) GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2]);

	// IKeyFrameInfo

	STDMETHODIMP_(HRESULT) GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP_(HRESULT) GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

	// IBufferInfo

	STDMETHODIMP_(int) GetCount();
	STDMETHODIMP GetStatus(int i, int& samples, int& size);
};

