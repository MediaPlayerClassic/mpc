#include "StdAfx.h"
#include "..\..\..\DSUtil\DSUtil.h"
#include <initguid.h>
#include "BaseSplitter.h"

#define MAXBUFFERS 2
#define MINPACKETS 5
#define MAXPACKETS 500

//
// CAsyncFileReader
//

CAsyncFileReader::CAsyncFileReader(CString fn, HRESULT& hr) : CUnknown(NAME(""), NULL, &hr)
{
	hr = Open(fn, modeRead|shareDenyWrite|typeBinary|osSequentialScan) ? S_OK : E_FAIL;
}

STDMETHODIMP CAsyncFileReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IAsyncReader)
		QI(IFileHandle)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CAsyncFileReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	try
	{
		if(llPosition+lLength > GetLength()) return E_FAIL; // strangly the Seek below can return llPosition even if the file is not that big (?)
		if(llPosition != Seek(llPosition, begin)) return E_FAIL;
		if((UINT)lLength < Read(pBuffer, lLength)) return E_FAIL;
	}
	catch(CFileException* e)
	{
		e->Delete();
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CAsyncFileReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	if(pTotal) *pTotal = GetLength();
	if(pAvailable) *pAvailable = GetLength();
	return S_OK;
}

// IFileHandle

STDMETHODIMP_(HANDLE) CAsyncFileReader::GetFileHandle()
{
	return m_hFile;
}

//
// CBaseSplitterInputPin
//

CBaseSplitterInputPin::CBaseSplitterInputPin(TCHAR* pName, CBaseSplitterFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBasePin(pName, pFilter, pLock, phr, L"Input", PINDIR_INPUT)
{
}

CBaseSplitterInputPin::~CBaseSplitterInputPin()
{
}

HRESULT CBaseSplitterInputPin::GetAsyncReader(IAsyncReader** ppAsyncReader)
{
	CheckPointer(ppAsyncReader, E_POINTER);
	*ppAsyncReader = NULL;
	CheckPointer(m_pAsyncReader, VFW_E_NOT_CONNECTED);
	(*ppAsyncReader = m_pAsyncReader)->AddRef();
	return S_OK;
}

STDMETHODIMP CBaseSplitterInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CBaseSplitterInputPin::CheckMediaType(const CMediaType* pmt)
{
	return S_OK;
/*
	return pmt->majortype == MEDIATYPE_Stream
		? S_OK
		: E_INVALIDARG;
*/
}

HRESULT CBaseSplitterInputPin::CheckConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CheckConnect(pPin)))
		return hr;

	if(CComQIPtr<IAsyncReader> pAsyncReader = pPin)
	{
		hr = S_OK;
	}
	else
	{
		hr = E_NOINTERFACE;
	}

	return hr;
}

HRESULT CBaseSplitterInputPin::BreakConnect()
{
	HRESULT hr;

	if(FAILED(hr = __super::BreakConnect()))
		return hr;

	if(FAILED(hr = ((CBaseSplitterFilter*)m_pFilter)->BreakConnect(PINDIR_INPUT, this)))
		return hr;

	m_pAsyncReader.Release();

	return S_OK;
}

HRESULT CBaseSplitterInputPin::CompleteConnect(IPin* pPin)
{
	HRESULT hr;

	if(FAILED(hr = __super::CompleteConnect(pPin)))
		return hr;

	CheckPointer(pPin, E_POINTER);
	m_pAsyncReader = pPin;
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	if(FAILED(hr = ((CBaseSplitterFilter*)m_pFilter)->CompleteConnect(PINDIR_INPUT, this)))
		return hr;

	return S_OK;
}

STDMETHODIMP CBaseSplitterInputPin::BeginFlush()
{
	return E_UNEXPECTED;
}

STDMETHODIMP CBaseSplitterInputPin::EndFlush()
{
	return E_UNEXPECTED;
}

//
// CBaseSplitterOutputPin
//

CBaseSplitterOutputPin::CBaseSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int nBuffers)
	: CBaseOutputPin(NAME("CBaseSplitterOutputPin"), pFilter, pLock, phr, pName)
	, m_hrDeliver(S_OK) // just in case it were asked before the worker thread could be created and reset it
	, m_fFlushing(false)
	, m_eEndFlush(TRUE)
{
	m_mts.Copy(mts);
	m_nBuffers = nBuffers > 0 ? nBuffers : MAXBUFFERS;
}

CBaseSplitterOutputPin::CBaseSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int nBuffers)
	: CBaseOutputPin(NAME("CBaseSplitterOutputPin"), pFilter, pLock, phr, pName)
	, m_hrDeliver(S_OK) // just in case it were asked before the worker thread could be created and reset it
	, m_fFlushing(false)
	, m_eEndFlush(TRUE)
{
	m_nBuffers = nBuffers > 0 ? nBuffers : MAXBUFFERS;
}

CBaseSplitterOutputPin::~CBaseSplitterOutputPin()
{
}

STDMETHODIMP CBaseSplitterOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		riid == __uuidof(IMediaSeeking) ? m_pFilter->QueryInterface(riid, ppv) : 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CBaseSplitterOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    HRESULT hr = NOERROR;

	pProperties->cBuffers = m_nBuffers;
	pProperties->cbBuffer = m_mt.lSampleSize;

    ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) return hr;

    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;
    ASSERT(Actual.cBuffers == pProperties->cBuffers);

    return NOERROR;
}

HRESULT CBaseSplitterOutputPin::CheckMediaType(const CMediaType* pmt)
{
	for(int i = 0; i < m_mts.GetCount(); i++)
	{
		if(*pmt == m_mts[i])
		// if(pmt->majortype == m_mts[i].majortype && pmt->subtype == m_mts[i].subtype)
			return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT CBaseSplitterOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= m_mts.GetCount()) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mts[iPosition];

	return S_OK;
}

STDMETHODIMP CBaseSplitterOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}

//

HRESULT CBaseSplitterOutputPin::Active()
{
    CAutoLock cAutoLock(m_pLock);

	if(m_Connected) 
		Create();

	return __super::Active();
}

HRESULT CBaseSplitterOutputPin::Inactive()
{
    CAutoLock cAutoLock(m_pLock);

	if(ThreadExists())
		CallWorker(CMD_EXIT);

	return __super::Inactive();
}

HRESULT CBaseSplitterOutputPin::DeliverBeginFlush()
{
	m_eEndFlush.Reset();
	m_fFlushed = false;
	m_fFlushing = true;
	m_hrDeliver = S_FALSE;
	CAutoLock cAutoLock(&m_csQueueLock);
	m_packets.RemoveAll();
	HRESULT hr = IsConnected() ? GetConnected()->BeginFlush() : S_OK;
	if(S_OK != hr) m_eEndFlush.Set();
	return(hr);
}

HRESULT CBaseSplitterOutputPin::DeliverEndFlush()
{
	if(!ThreadExists()) return S_FALSE;
	HRESULT hr = IsConnected() ? GetConnected()->EndFlush() : S_OK;
	m_hrDeliver = S_OK;
	m_fFlushing = false;
	m_fFlushed = true;
	m_eEndFlush.Set();
	return hr;
}

HRESULT CBaseSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if(m_fFlushing) return S_FALSE;
	m_rtStart = tStart;
	if(!ThreadExists()) return S_FALSE;
	HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);
	if(S_OK != hr) return hr;
	MakeISCRHappy();
	return hr;
}

int CBaseSplitterOutputPin::QueueCount()
{
	CAutoLock cAutoLock(&m_csQueueLock);
	return m_packets.GetCount();
}

void CBaseSplitterOutputPin::QueueAdd(CAutoPtr<Packet> p)
{
	CAutoLock cAutoLock(&m_csQueueLock);
	m_packets.AddHead(p);
}

CAutoPtr<Packet> CBaseSplitterOutputPin::QueueRemove()
{
	CAutoLock cAutoLock(&m_csQueueLock);
	return m_packets.RemoveTail();
}

HRESULT CBaseSplitterOutputPin::QueueEndOfStream()
{
	return QueuePacket(CAutoPtr<Packet>()); // NULL means EndOfStream
}

HRESULT CBaseSplitterOutputPin::QueuePacket(CAutoPtr<Packet> p)
{
	if(!ThreadExists()) return S_FALSE;

	do
	{
		if(//QueueCount() < MAXPACKETS/2 ||
		 (((CBaseSplitterFilter*)m_pFilter)->IsAnyPinDrying()
			&& QueueCount() < MAXPACKETS)) break;
		else Sleep(1);
/*
		if(QueueCount() >= MAXPACKETS) Sleep(1);
		else break;
*/
	}
	while(S_OK == m_hrDeliver);

	if(S_OK != m_hrDeliver) return m_hrDeliver;

	QueueAdd(p);

	return m_hrDeliver;
}

DWORD CBaseSplitterOutputPin::ThreadProc()
{
	m_hrDeliver = S_OK;
	m_fFlushing = m_fFlushed = false;
	m_eEndFlush.Set();

	while(1)
	{
		Sleep(1);

		DWORD cmd;
		if(CheckRequest(&cmd))
		{
			m_hThread = NULL;
			cmd = GetRequest();
			Reply(S_OK);
			ASSERT(cmd == CMD_EXIT);
			return 0;
		}

		int cnt = 0;
		do
		{
			CAutoPtr<Packet> p;

			{
				CAutoLock cAutoLock(&m_csQueueLock);
				if((cnt = QueueCount()) > 0)
					p = QueueRemove();
			}

			if(S_OK == m_hrDeliver && cnt > 0)
			{
				ASSERT(!m_fFlushing);

				m_fFlushed = false;

				// flushing can still start here, to release a blocked deliver call

				HRESULT hr = p 
					? DeliverPacket(p) 
					: DeliverEndOfStream();

				m_eEndFlush.Wait(); // .. so we have to wait until it is done

				if(hr != S_OK && !m_fFlushed) // and only report the error in m_hrDeliver if we didn't flush the stream
				{
					CAutoLock cAutoLock(&m_csQueueLock);
					m_hrDeliver = hr;
					break;
				}
			}
		}
		while(--cnt > 0);
	}
}

HRESULT CBaseSplitterOutputPin::DeliverPacket(CAutoPtr<Packet> p)
{
	HRESULT hr;

	if(p->pData.GetCount() == 0)
		return S_OK;

	double dRate = 1.0;
	if(SUCCEEDED(((CBaseSplitterFilter*)m_pFilter)->GetRate(&dRate)))
	{
		p->rtStart = (REFERENCE_TIME)((double)p->rtStart / dRate);
		p->rtStop = (REFERENCE_TIME)((double)p->rtStop / dRate);
	}

	do
	{
		CComPtr<IMediaSample> pSample;
		if(S_OK != (hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0))) break;
		DWORD nBytes = p->pData.GetCount();
		if(nBytes > pSample->GetSize())
		{
			pSample.Release();
			if(S_OK != (hr = __super::DeliverBeginFlush())) break;
			if(S_OK != (hr = __super::DeliverEndFlush())) break;
			if(S_OK != (hr = m_pAllocator->Decommit())) break;
			ALLOCATOR_PROPERTIES props, actual;
			if(S_OK != (hr = m_pAllocator->GetProperties(&props))) break;
			props.cbBuffer = nBytes*3/2;
			if(S_OK != (hr = m_pAllocator->SetProperties(&props, &actual))) break;
			if(S_OK != (hr = m_pAllocator->Commit())) break;
			if(S_OK != (hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0))) break;
		}
		BYTE* pData = NULL;
		if(S_OK != (hr = pSample->GetPointer(&pData)) || !pData) break;
		memcpy(pData, p->pData.GetData(), nBytes);
		if(S_OK != (hr = pSample->SetActualDataLength(nBytes))) break;
		if(S_OK != (hr = pSample->SetTime(&p->rtStart, &p->rtStop))) break;
		if(S_OK != (hr = pSample->SetMediaTime(NULL, NULL))) break;
		if(S_OK != (hr = pSample->SetDiscontinuity(p->bDiscontinuity))) break;
		if(S_OK != (hr = pSample->SetSyncPoint(p->bSyncPoint))) break;
		if(S_OK != (hr = pSample->SetPreroll(p->rtStart < 0))) break;
		if(S_OK != (hr = Deliver(pSample))) break;
	}
	while(false);

	return hr;
}

void CBaseSplitterOutputPin::MakeISCRHappy()
{
	CComPtr<IPin> pPinTo = this, pTmp;
	while(pPinTo && SUCCEEDED(pPinTo->ConnectedTo(&pTmp)) && (pPinTo = pTmp))
	{
		pTmp = NULL;

		CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPinTo);

		if(GetCLSID(pBF) == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) // ISCR
		{
			CAutoPtr<Packet> p(new Packet());
			p->TrackNumber = (DWORD)-1;
			p->rtStart = -1; p->rtStop = 0;
			p->bSyncPoint = FALSE;
			p->pData.SetSize(2);
			strcpy((char*)p->pData.GetData(), " ");
			QueuePacket(p);
			break;
		}

		pPinTo = GetFirstPin(pBF, PINDIR_OUTPUT);
	}
}

HRESULT CBaseSplitterOutputPin::GetDeliveryBuffer(IMediaSample** ppSample, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	return __super::GetDeliveryBuffer(ppSample, pStartTime, pEndTime, dwFlags);
}

HRESULT CBaseSplitterOutputPin::Deliver(IMediaSample* pSample)
{
	return __super::Deliver(pSample);
}

//
// CBaseSplitterFilter
//

CBaseSplitterFilter::CBaseSplitterFilter(LPCTSTR pName, LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(pName, pUnk, this, clsid)
	, m_rtStart(0), m_rtStop(0), m_rtCurrent(0)
	, m_dRate(1.0)
	, m_nOpenProgress(100)
	, m_fAbort(false)
{
	if(phr) *phr = S_OK;

	m_pInput.Attach(new CBaseSplitterInputPin(NAME("CBaseSplitterInputPin"), this, this, phr));
}

CBaseSplitterFilter::~CBaseSplitterFilter()
{
	CAutoLock cAutoLock(this);

	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();
}

STDMETHODIMP CBaseSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	if(m_pInput && riid == __uuidof(IFileSourceFilter)) 
		return E_NOINTERFACE;

	return 
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		QI(IAMOpenProgress)
		QI2(IAMMediaContent)
		QI(IChapterInfo)
		QI(IKeyFrameInfo)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CBaseSplitterFilter::DeliverBeginFlush()
{
	m_fFlushing = true;
	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverBeginFlush();
}

void CBaseSplitterFilter::DeliverEndFlush()
{
	POSITION pos = m_pOutputs.GetHeadPosition();
	while(pos) m_pOutputs.GetNext(pos)->DeliverEndFlush();
	m_fFlushing = false;
	m_eEndFlush.Set();
}

DWORD CBaseSplitterFilter::ThreadProc()
{
	if(!InitDeliverLoop())
	{
		while(1)
		{
			DWORD cmd = GetRequest();
			if(cmd == CMD_EXIT) CAMThread::m_hThread = NULL;
			Reply(S_OK);
			if(cmd == CMD_EXIT) return 0;
		}
	}

	SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	m_eEndFlush.Set();
	m_fFlushing = false;

	bool fFirstRun = true;

	while(1)
	{
		DWORD cmd = fFirstRun ? -1 : GetRequest();

		fFirstRun = false;

		if(cmd == CMD_EXIT)
		{
			CAMThread::m_hThread = NULL;
			Reply(S_OK);
			return 0;
		}

		m_rtStart = m_rtNewStart;
		m_rtStop = m_rtNewStop;

		SeekDeliverLoop(m_rtStart);

		if(cmd != -1)
			Reply(S_OK);

		m_eEndFlush.Wait();

		m_bDiscontinuitySent.RemoveAll();
		m_pActivePins.RemoveAll();

		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos && !m_fFlushing)
		{
			CBaseSplitterOutputPin* pPin = m_pOutputs.GetNext(pos);
			if(pPin->IsConnected())
			{
				pPin->DeliverNewSegment(m_rtStart, m_rtStop, m_dRate);
				m_pActivePins.AddTail(pPin);
			}
		}

		DoDeliverLoop();

		pos = m_pActivePins.GetHeadPosition();
		while(pos && !CheckRequest(&cmd))
			m_pActivePins.GetNext(pos)->QueueEndOfStream();
	}

	ASSERT(0); // we should only exit via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
}

HRESULT CBaseSplitterFilter::DeliverPacket(CAutoPtr<Packet> p)
{
	HRESULT hr = S_FALSE;

	CBaseSplitterOutputPin* pPin = NULL;
	if(!m_pPinMap.Lookup(p->TrackNumber, pPin) || !pPin 
	|| !pPin->IsConnected() || !m_pActivePins.Find(pPin))
		return S_FALSE;

	m_rtCurrent = p->rtStart;

	p->rtStart -= m_rtStart;
	p->rtStop -= m_rtStart;

	ASSERT(p->rtStart <= p->rtStop);

	DWORD TrackNumber = p->TrackNumber;
	BOOL bDiscontinuity = p->bDiscontinuity = !m_bDiscontinuitySent.Find(p->TrackNumber);

	hr = pPin->QueuePacket(p);

	if(S_OK != hr)
	{
		if(POSITION pos = m_pActivePins.Find(pPin))
			m_pActivePins.RemoveAt(pos);

		if(!m_pActivePins.IsEmpty()) // only die when all pins are down
			hr = S_OK;

		return hr;
	}

	if(bDiscontinuity)
		m_bDiscontinuitySent.AddTail(TrackNumber);

	return hr;
}


bool CBaseSplitterFilter::IsAnyPinDrying()
{
	POSITION pos = m_pActivePins.GetHeadPosition();
	while(pos)
	{
		if(m_pActivePins.GetNext(pos)->QueueCount() < MINPACKETS)
			return(true);
	}

	return(false);
}

HRESULT CBaseSplitterFilter::BreakConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CBaseSplitterInputPin* pIn = (CBaseSplitterInputPin*)pPin;

		// TODO: do something here!!!
/*
		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos) m_pOutputs.GetNext(pos)->Disconnect();
		m_pOutputs.RemoveAll();
*/
//		m_pFile.Free();
	}
	else if(dir == PINDIR_OUTPUT)
	{
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

HRESULT CBaseSplitterFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin)
{
	CheckPointer(pPin, E_POINTER);

	if(dir == PINDIR_INPUT)
	{
		CBaseSplitterInputPin* pIn = (CBaseSplitterInputPin*)pPin;

		HRESULT hr;

		CComPtr<IAsyncReader> pAsyncReader;
		if(FAILED(hr = pIn->GetAsyncReader(&pAsyncReader))
		|| FAILED(hr = CreateOutputs(pAsyncReader)))
			return hr;
	}
	else if(dir == PINDIR_OUTPUT)
	{
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

int CBaseSplitterFilter::GetPinCount()
{
	return (m_pInput ? 1 : 0) + m_pOutputs.GetCount();
}

CBasePin* CBaseSplitterFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n >= 0 && n < (int)m_pOutputs.GetCount())
	{
		if(POSITION pos = m_pOutputs.FindIndex(n))
			return m_pOutputs.GetAt(pos);
	}

	if(n == m_pOutputs.GetCount() && m_pInput)
	{
		return m_pInput;
	}

	return NULL;
}

STDMETHODIMP CBaseSplitterFilter::Stop()
{
	CAutoLock cAutoLock(this);

	DeliverBeginFlush();
	CallWorker(CMD_EXIT);
	DeliverEndFlush();

	HRESULT hr;
	if(FAILED(hr = __super::Stop()))
		return hr;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr;
	if(FAILED(hr = __super::Pause()))
		return hr;

	if(fs == State_Stopped)
	{
		Create();
	}

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;
	if(FAILED(hr = __super::Run(tStart)))
		return hr;

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CBaseSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CheckPointer(pszFileName, E_POINTER);

	HRESULT hr = E_FAIL;
	CComPtr<IAsyncReader> pAsyncReader = (IAsyncReader*)new CAsyncFileReader(CString(pszFileName), hr);
	if(FAILED(hr)) return hr;

	if(FAILED(hr = CreateOutputs(pAsyncReader)))
		return hr;

	m_fn = pszFileName;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_fn.GetLength()+1)*sizeof(WCHAR))))
		return E_OUTOFMEMORY;
	wcscpy(*ppszFileName, m_fn);
	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CBaseSplitterFilter::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities = 
		AM_SEEKING_CanGetStopPos|
		AM_SEEKING_CanGetDuration|
		AM_SEEKING_CanSeekAbsolute|
		AM_SEEKING_CanSeekForwards|
		AM_SEEKING_CanSeekBackwards, S_OK : E_POINTER;
}
STDMETHODIMP CBaseSplitterFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if(*pCapabilities == 0) return S_OK;
	DWORD caps;
	GetCapabilities(&caps);
	if((caps&*pCapabilities) == 0) return E_FAIL;
	if(caps == *pCapabilities) return S_OK;
	return S_FALSE;
}
STDMETHODIMP CBaseSplitterFilter::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CBaseSplitterFilter::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CBaseSplitterFilter::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CBaseSplitterFilter::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CBaseSplitterFilter::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
// STDMETHODIMP CBaseSplitterFilter::GetDuration(LONGLONG* pDuration); // derived class implements this
STDMETHODIMP CBaseSplitterFilter::GetStopPosition(LONGLONG* pStop) {return GetDuration(pStop);}
STDMETHODIMP CBaseSplitterFilter::GetCurrentPosition(LONGLONG* pCurrent) {return E_NOTIMPL;}
STDMETHODIMP CBaseSplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}
STDMETHODIMP CBaseSplitterFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	CAutoLock cAutoLock(this);

	if(!pCurrent && !pStop
	|| (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
		return S_OK;

	REFERENCE_TIME 
		rtCurrent = m_rtCurrent,
		rtStop = m_rtStop;

	if(pCurrent)
	switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
	case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
	case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
	}

	if(pStop)
	switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
	{
	case AM_SEEKING_NoPositioning: break;
	case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
	case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
	case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
	}

	if(m_rtCurrent == rtCurrent && m_rtStop == rtStop)
		return S_OK;

DbgLog((LOG_TRACE, 0, _T("Seek Started")));

	m_rtNewStart = m_rtCurrent = rtCurrent;
	m_rtNewStop = rtStop;

	if(ThreadExists())
	{
		DeliverBeginFlush();
		CallWorker(CMD_SEEK);
		DeliverEndFlush();
	}

DbgLog((LOG_TRACE, 0, _T("Seek Ended")));

	return S_OK;
}
STDMETHODIMP CBaseSplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) *pCurrent = m_rtCurrent;
	if(pStop) *pStop = m_rtStop;
	return S_OK;
}
STDMETHODIMP CBaseSplitterFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest) *pEarliest = 0;
	return GetDuration(pLatest);
}
STDMETHODIMP CBaseSplitterFilter::SetRate(double dRate) {return dRate > 0 ? m_dRate = dRate, S_OK : E_INVALIDARG;}
STDMETHODIMP CBaseSplitterFilter::GetRate(double* pdRate) {return pdRate ? *pdRate = m_dRate, S_OK : E_POINTER;}
STDMETHODIMP CBaseSplitterFilter::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}

// IAMOpenProgress

STDMETHODIMP CBaseSplitterFilter::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	CheckPointer(pllTotal, E_POINTER);
	CheckPointer(pllCurrent, E_POINTER);

	*pllTotal = 100;
	*pllCurrent = m_nOpenProgress;

	return S_OK;
}

STDMETHODIMP CBaseSplitterFilter::AbortOperation()
{
	m_fAbort = true;
	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CBaseSplitterFilter::get_AuthorName(BSTR* pbstrAuthorName)
{
	return GetMediaContentStr(pbstrAuthorName, AuthorName);
}

STDMETHODIMP CBaseSplitterFilter::get_Title(BSTR* pbstrTitle)
{
	return GetMediaContentStr(pbstrTitle, Title);
}

STDMETHODIMP CBaseSplitterFilter::get_Rating(BSTR* pbstrRating)
{
	return GetMediaContentStr(pbstrRating, Rating);
}

STDMETHODIMP CBaseSplitterFilter::get_Description(BSTR* pbstrDescription)
{
	return GetMediaContentStr(pbstrDescription, Description);
}

STDMETHODIMP CBaseSplitterFilter::get_Copyright(BSTR* pbstrCopyright)
{
	return GetMediaContentStr(pbstrCopyright, Copyright);
}

HRESULT CBaseSplitterFilter::GetMediaContentStr(BSTR* pBSTR, mctype type)
{
	CheckPointer(pBSTR, E_POINTER);
	CAutoLock cAutoLock(this);
	*pBSTR = m_mcs[(int)type].AllocSysString();
	return S_OK;
}

HRESULT CBaseSplitterFilter::SetMediaContentStr(CStringW str, mctype type)
{
	CAutoLock cAutoLock(this);
	m_mcs[(int)type] = str.Trim();
	return S_OK;
}

// IChapterInfo

STDMETHODIMP_(UINT) CBaseSplitterFilter::GetChapterCount(UINT aChapterID)
{
	return 0;
}

STDMETHODIMP_(UINT) CBaseSplitterFilter::GetChapterId(UINT aParentChapterId, UINT aIndex)
{
	return CHAPTER_BAD_ID;
}

STDMETHODIMP_(UINT) CBaseSplitterFilter::GetChapterCurrentId()
{
	return CHAPTER_BAD_ID;
}

STDMETHODIMP_(BOOL) CBaseSplitterFilter::GetChapterInfo(UINT aChapterID, struct ChapterElement* pStructureToFill)
{
	CheckPointer(pStructureToFill, E_POINTER);
	return FALSE;
}

STDMETHODIMP_(BSTR) CBaseSplitterFilter::GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2])
{
	return NULL;
}

// IKeyFrameInfo

STDMETHODIMP CBaseSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	return E_NOTIMPL;
}
