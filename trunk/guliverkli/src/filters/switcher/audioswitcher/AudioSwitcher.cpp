#include "stdafx.h"
#include "Shlwapi.h"
#include <atlpath.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "AudioSwitcher.h"
#include "Audio.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\Ogg\OggDS.h"

#define BLOCKSTREAM
//

#define PauseGraph \
	CComQIPtr<IMediaControl> _pMC(m_pGraph); \
	OAFilterState _fs = -1; \
	if(_pMC) _pMC->GetState(1000, &_fs); \
	if(_fs == State_Running) \
		_pMC->Pause(); \
 \
	HRESULT _hr = E_FAIL; \
	CComQIPtr<IMediaSeeking> _pMS((IUnknown*)(INonDelegatingUnknown*)m_pGraph); \
	LONGLONG _rtNow = 0; \
	if(_pMS) _hr = _pMS->GetCurrentPosition(&_rtNow); \

#define ResumeGraph \
	if(SUCCEEDED(_hr) && _pMS) \
		_hr = _pMS->SetPositions(&_rtNow, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning); \
 \
	if(_fs == State_Running && _pMS) \
		_pMC->Run(); \

//
// CStreamSwitcherPassThru
//

CStreamSwitcherPassThru::CStreamSwitcherPassThru(LPUNKNOWN pUnk, HRESULT* phr, CStreamSwitcherFilter* pFilter)
	: CMediaPosition(NAME("CStreamSwitcherPassThru"), pUnk)
	, m_pFilter(pFilter)
{
}

STDMETHODIMP CStreamSwitcherPassThru::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
		QI(IMediaSeeking)
		CMediaPosition::NonDelegatingQueryInterface(riid, ppv);
}

template<class T>
HRESULT GetPeer(CStreamSwitcherFilter* pFilter, T** ppT)
{
    *ppT = NULL;

	CBasePin* pPin = pFilter->GetInputPin();
	if(!pPin) return E_NOTIMPL;

    CComPtr<IPin> pConnected;
    if(FAILED(pPin->ConnectedTo(&pConnected))) 
		return E_NOTIMPL;

	if(CComQIPtr<T> pT = pConnected)
	{
		*ppT = pT.Detach();
		return S_OK;
	}

	return E_NOTIMPL;
}

#define CallPeerSeeking(call) \
	CComPtr<IMediaSeeking> pMS; \
	if(FAILED(GetPeer(m_pFilter, &pMS))) return E_NOTIMPL; \
	return pMS->##call; \

#define CallPeer(call) \
	CComPtr<IMediaPosition> pMP; \
	if(FAILED(GetPeer(m_pFilter, &pMP))) return E_NOTIMPL; \
	return pMP->##call; \

#define CallPeerSeekingAll(call) \
	HRESULT hr = E_NOTIMPL; \
	POSITION pos = m_pFilter->m_pInputs.GetHeadPosition(); \
	while(pos) \
	{ \
		CBasePin* pPin = m_pFilter->m_pInputs.GetNext(pos); \
		CComPtr<IPin> pConnected; \
	    if(FAILED(pPin->ConnectedTo(&pConnected))) \
			continue; \
		if(CComQIPtr<IMediaSeeking> pMS = pConnected) \
		{ \
			HRESULT hr2 = pMS->call; \
			if(pPin == m_pFilter->GetInputPin()) \
				hr = hr2; \
		} \
	} \
	return hr; \

#define CallPeerAll(call) \
	HRESULT hr = E_NOTIMPL; \
	POSITION pos = m_pFilter->m_pInputs.GetHeadPosition(); \
	while(pos) \
	{ \
		CBasePin* pPin = m_pFilter->m_pInputs.GetNext(pos); \
		CComPtr<IPin> pConnected; \
	    if(FAILED(pPin->ConnectedTo(&pConnected))) \
			continue; \
		if(CComQIPtr<IMediaPosition> pMP = pConnected) \
		{ \
			HRESULT hr2 = pMP->call; \
			if(pPin == m_pFilter->GetInputPin()) \
				hr = hr2; \
		} \
	} \
	return hr; \


// IMediaSeeking

STDMETHODIMP CStreamSwitcherPassThru::GetCapabilities(DWORD* pCaps)
	{CallPeerSeeking(GetCapabilities(pCaps));}
STDMETHODIMP CStreamSwitcherPassThru::CheckCapabilities(DWORD* pCaps)
	{CallPeerSeeking(CheckCapabilities(pCaps));}
STDMETHODIMP CStreamSwitcherPassThru::IsFormatSupported(const GUID* pFormat)
	{CallPeerSeeking(IsFormatSupported(pFormat));}
STDMETHODIMP CStreamSwitcherPassThru::QueryPreferredFormat(GUID* pFormat)
	{CallPeerSeeking(QueryPreferredFormat(pFormat));}
STDMETHODIMP CStreamSwitcherPassThru::SetTimeFormat(const GUID* pFormat)
	{CallPeerSeeking(SetTimeFormat(pFormat));}
STDMETHODIMP CStreamSwitcherPassThru::GetTimeFormat(GUID* pFormat)
	{CallPeerSeeking(GetTimeFormat(pFormat));}
STDMETHODIMP CStreamSwitcherPassThru::IsUsingTimeFormat(const GUID* pFormat)
	{CallPeerSeeking(IsUsingTimeFormat(pFormat));}
STDMETHODIMP CStreamSwitcherPassThru::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
	{CallPeerSeeking(ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat));}
STDMETHODIMP CStreamSwitcherPassThru::SetPositions(LONGLONG* pCurrent, DWORD CurrentFlags, LONGLONG* pStop, DWORD StopFlags)
	{CallPeerSeekingAll(SetPositions(pCurrent, CurrentFlags, pStop, StopFlags));}
STDMETHODIMP CStreamSwitcherPassThru::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
	{CallPeerSeeking(GetPositions(pCurrent, pStop));}
STDMETHODIMP CStreamSwitcherPassThru::GetCurrentPosition(LONGLONG* pCurrent)
	{CallPeerSeeking(GetCurrentPosition(pCurrent));}
STDMETHODIMP CStreamSwitcherPassThru::GetStopPosition(LONGLONG* pStop)
	{CallPeerSeeking(GetStopPosition(pStop));}
STDMETHODIMP CStreamSwitcherPassThru::GetDuration(LONGLONG* pDuration)
	{CallPeerSeeking(GetDuration(pDuration));}
STDMETHODIMP CStreamSwitcherPassThru::GetPreroll(LONGLONG* pllPreroll)
	{CallPeerSeeking(GetPreroll(pllPreroll));}
STDMETHODIMP CStreamSwitcherPassThru::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
	{CallPeerSeeking(GetAvailable(pEarliest, pLatest));}
STDMETHODIMP CStreamSwitcherPassThru::GetRate(double* pdRate)
	{CallPeerSeeking(GetRate(pdRate));}
STDMETHODIMP CStreamSwitcherPassThru::SetRate(double dRate)
	{if(0.0 == dRate) return E_INVALIDARG;
	CallPeerSeekingAll(SetRate(dRate));}

// IMediaPosition

STDMETHODIMP CStreamSwitcherPassThru::get_Duration(REFTIME* plength)
	{CallPeer(get_Duration(plength));}
STDMETHODIMP CStreamSwitcherPassThru::get_CurrentPosition(REFTIME* pllTime)
	{CallPeer(get_CurrentPosition(pllTime));}
STDMETHODIMP CStreamSwitcherPassThru::put_CurrentPosition(REFTIME llTime)
	{CallPeerAll(put_CurrentPosition(llTime));}
STDMETHODIMP CStreamSwitcherPassThru::get_StopTime(REFTIME* pllTime)
	{CallPeer(get_StopTime(pllTime));}
STDMETHODIMP CStreamSwitcherPassThru::put_StopTime(REFTIME llTime)
	{CallPeerAll(put_StopTime(llTime));}
STDMETHODIMP CStreamSwitcherPassThru::get_PrerollTime(REFTIME * pllTime)
	{CallPeer(get_PrerollTime(pllTime));}
STDMETHODIMP CStreamSwitcherPassThru::put_PrerollTime(REFTIME llTime)
	{CallPeerAll(put_PrerollTime(llTime));}
STDMETHODIMP CStreamSwitcherPassThru::get_Rate(double* pdRate)
	{CallPeer(get_Rate(pdRate));}
STDMETHODIMP CStreamSwitcherPassThru::put_Rate(double dRate)
	{if(0.0 == dRate) return E_INVALIDARG;
	CallPeerAll(put_Rate(dRate));}
STDMETHODIMP CStreamSwitcherPassThru::CanSeekForward(LONG* pCanSeekForward)
	{CallPeer(CanSeekForward(pCanSeekForward));}
STDMETHODIMP CStreamSwitcherPassThru::CanSeekBackward(LONG* pCanSeekBackward) 
	{CallPeer(CanSeekBackward(pCanSeekBackward));}


//
// CStreamSwitcherAllocator
//

CStreamSwitcherAllocator::CStreamSwitcherAllocator(CStreamSwitcherInputPin* pPin, HRESULT* phr)
	: CMemAllocator(NAME("CStreamSwitcherAllocator"), NULL, phr)
	, m_pPin(pPin)
	, m_fMediaTypeChanged(false)
{
	ASSERT(phr);
	ASSERT(pPin);
}

#ifdef DEBUG
CStreamSwitcherAllocator::~CStreamSwitcherAllocator()
{
    ASSERT(m_bCommitted == FALSE);
}
#endif

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingAddRef()
{
	return m_pPin->m_pFilter->AddRef();
}

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingRelease()
{
	return m_pPin->m_pFilter->Release();
}

STDMETHODIMP CStreamSwitcherAllocator::GetBuffer(
	IMediaSample** ppBuffer, 
	REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, 
	DWORD dwFlags)
{
	HRESULT hr = VFW_E_NOT_COMMITTED;

	if(!m_bCommitted)
        return hr;
/*
TRACE(_T("CStreamSwitcherAllocator::GetBuffer m_pPin->m_evBlock.Wait() + %x\n"), this);
	m_pPin->m_evBlock.Wait();
TRACE(_T("CStreamSwitcherAllocator::GetBuffer m_pPin->m_evBlock.Wait() - %x\n"), this);
*/
	if(m_fMediaTypeChanged)
	{
		if(!m_pPin || !m_pPin->m_pFilter)
			return hr;

		CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pPin->m_pFilter)->GetOutputPin();
		if(!pOut || !pOut->CurrentAllocator())
			return hr;

		ALLOCATOR_PROPERTIES Properties, Actual;
		if(FAILED(pOut->CurrentAllocator()->GetProperties(&Actual))) 
			return hr;
		if(FAILED(GetProperties(&Properties))) 
			return hr;

		if(!m_bCommitted || Properties.cbBuffer < Actual.cbBuffer)
		{
			Properties.cbBuffer = Actual.cbBuffer;
			if(FAILED(Decommit())) return hr;
			if(FAILED(SetProperties(&Properties, &Actual))) return hr;
			if(FAILED(Commit())) return hr;
			ASSERT(Actual.cbBuffer >= Properties.cbBuffer);
			if(Actual.cbBuffer < Properties.cbBuffer) return hr;
		}
	}

	hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if(m_fMediaTypeChanged && SUCCEEDED(hr))
	{
		(*ppBuffer)->SetMediaType(&m_mt);
		m_fMediaTypeChanged = false;
	}

	return hr;
}

void CStreamSwitcherAllocator::NotifyMediaType(const CMediaType& mt)
{
	CopyMediaType(&m_mt, &mt);
	m_fMediaTypeChanged = true;
}


//
// CStreamSwitcherInputPin
//

CStreamSwitcherInputPin::CStreamSwitcherInputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr, LPCWSTR pName)
    : CBaseInputPin(NAME("CStreamSwitcherInputPin"), pFilter, &pFilter->m_csState, phr, pName)
	, m_Allocator(this, phr)
	, m_bSampleSkipped(FALSE)
	, m_bQualityChanged(FALSE)
	, m_bUsingOwnAllocator(FALSE)
	, m_evBlock(TRUE)
	, m_fCanBlock(false)
{
	m_bCanReconnectWhenActive = TRUE;
}

HRESULT CStreamSwitcherInputPin::QueryAcceptDownstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_OK;

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();

	if(pOut && pOut->IsConnected())
	{
		if(CComPtr<IPinConnection> pPC = pOut->CurrentPinConnection())
		{
			hr = pPC->DynamicQueryAccept(pmt);
			if(hr == S_OK) return S_OK;
		}

		hr = pOut->GetConnected()->QueryAccept(pmt);
	}

	return hr;
}

bool CStreamSwitcherInputPin::IsSelected()
{
	return(this == ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin());
}

void CStreamSwitcherInputPin::Block(bool fBlock)
{
	if(fBlock) m_evBlock.Reset();
	else m_evBlock.Set();
}

HRESULT CStreamSwitcherInputPin::InitializeOutputSample(IMediaSample* pInSample, IMediaSample** ppOutSample)
{
	if(!pInSample || !ppOutSample) 
		return E_POINTER;

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();
	ASSERT(pOut->GetConnected());

    CComPtr<IMediaSample> pOutSample;

	DWORD dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;

    if(!(m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT))
		dwFlags |= AM_GBF_NOTASYNCPOINT;

	HRESULT hr = pOut->GetDeliveryBuffer(&pOutSample
        , m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID ? &m_SampleProps.tStart : NULL
        , m_SampleProps.dwSampleFlags & AM_SAMPLE_STOPVALID ? &m_SampleProps.tStop : NULL
        , dwFlags);

    if(FAILED(hr))
		return hr;

	if(!pOutSample) 
		return E_FAIL;

    if(CComQIPtr<IMediaSample2> pOutSample2 = pOutSample)
	{
        AM_SAMPLE2_PROPERTIES OutProps;
		EXECUTE_ASSERT(SUCCEEDED(pOutSample2->GetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, tStart), (PBYTE)&OutProps)));
        OutProps.dwTypeSpecificFlags = m_SampleProps.dwTypeSpecificFlags;
        OutProps.dwSampleFlags =
            (OutProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED) |
            (m_SampleProps.dwSampleFlags & ~AM_SAMPLE_TYPECHANGED);

        OutProps.tStart = m_SampleProps.tStart;
        OutProps.tStop  = m_SampleProps.tStop;
        OutProps.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId);

        hr = pOutSample2->SetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId), (PBYTE)&OutProps);
        if(m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY)
			m_bSampleSkipped = FALSE;
    }
    else
	{
        if(m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID)
			pOutSample->SetTime(&m_SampleProps.tStart, &m_SampleProps.tStop);

		if(m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT)
			pOutSample->SetSyncPoint(TRUE);

		if(m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY)
		{
			pOutSample->SetDiscontinuity(TRUE);
            m_bSampleSkipped = FALSE;
        }

		LONGLONG MediaStart, MediaEnd;
        if(pInSample->GetMediaTime(&MediaStart, &MediaEnd) == NOERROR)
			pOutSample->SetMediaTime(&MediaStart, &MediaEnd);
    }

	*ppOutSample = pOutSample.Detach();

	return S_OK;
}

// pure virtual

HRESULT CStreamSwitcherInputPin::CheckMediaType(const CMediaType* pmt)
{
	return ((CStreamSwitcherFilter*)m_pFilter)->CheckMediaType(pmt);
}

// virtual 

HRESULT CStreamSwitcherInputPin::CheckConnect(IPin* pPin)
{
	return (IPin*)((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin() == pPin 
		? E_FAIL
		: CBaseInputPin::CheckConnect(pPin);
}

HRESULT CStreamSwitcherInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
	if(FAILED(hr)) return hr;

    ((CStreamSwitcherFilter*)m_pFilter)->CompleteConnect(PINDIR_INPUT, this, pReceivePin);

	m_fCanBlock = false;
	bool fForkedSomewhere = false;

	CStringW fileName;
	CStringW pinName;

    IPin* pPin = (IPin*)this;
	IBaseFilter* pBF = (IBaseFilter*)m_pFilter;

	while((pPin = GetUpStreamPin(pBF, pPin)) && (pBF = GetFilterFromPin(pPin)))
	{
		if(IsSplitter(pBF))
		{
			pinName = GetPinName(pPin);
		}

		CLSID clsid = GetCLSID(pBF);
		if(clsid == CLSID_AviSplitter || clsid == CLSID_OggSplitter)
			m_fCanBlock = true;

		int nIn, nOut, nInC, nOutC;
		CountPins(pBF, nIn, nOut, nInC, nOutC);
		fForkedSomewhere = fForkedSomewhere || nInC > 1 || nOutC > 1;

		if(CComQIPtr<IFileSourceFilter> pFSF = pBF)
		{
			WCHAR* pszName = NULL;
			AM_MEDIA_TYPE mt;
			if(SUCCEEDED(pFSF->GetCurFile(&pszName, &mt)) && pszName)
			{
				fileName = pszName;
				CoTaskMemFree(pszName);

				fileName.Replace('\\', '/');
				CStringW fn = fileName.Mid(fileName.ReverseFind('/')+1);
				if(!fn.IsEmpty()) fileName = fn;

				if(!pinName.IsEmpty()) fileName += L" / " + pinName;

				WCHAR* pName = new WCHAR[fileName.GetLength()+1];
				if(pName)
				{
					wcscpy(pName, fileName);
					if(m_pName) delete [] m_pName;
					m_pName = pName;
				}
			}

			break;
		}

		pPin = GetFirstPin(pBF);
	}

	if(!fForkedSomewhere)
		m_fCanBlock = true;

	return S_OK;
}

HRESULT CStreamSwitcherInputPin::Active()
{
	Block(!IsSelected());

	return CBaseInputPin::Active();
}

HRESULT CStreamSwitcherInputPin::Inactive()
{
	Block(false);

	return CBaseInputPin::Inactive();
}

// IPin

STDMETHODIMP CStreamSwitcherInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = CBaseInputPin::QueryAccept(pmt);
	if(S_OK != hr) return hr;

	return QueryAcceptDownstream(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	// FIXME: this locked up once
//    CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csReceive);

	HRESULT hr;
	if(S_OK != (hr = QueryAcceptDownstream(pmt)))
		return VFW_E_TYPE_NOT_ACCEPTED;

	if(m_Connected) 
		m_Connected->Release(), m_Connected = NULL;

	return SUCCEEDED(CBaseInputPin::ReceiveConnection(pConnector, pmt)) ? S_OK : E_FAIL;
}

STDMETHODIMP CStreamSwitcherInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
    CheckPointer(ppAllocator, E_POINTER);

    if(m_pAllocator == NULL)
	{
        (m_pAllocator = &m_Allocator)->AddRef();
    }

    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;

    return NOERROR;
} 

STDMETHODIMP CStreamSwitcherInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly)
{
	HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
	if(FAILED(hr)) return hr;

	m_bUsingOwnAllocator = (pAllocator == (IMemAllocator*)&m_Allocator);

	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::BeginFlush()
{
    CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csState);

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();
    if(!IsConnected() || !pOut || !pOut->IsConnected())
		return VFW_E_NOT_CONNECTED;

	HRESULT hr = CBaseInputPin::BeginFlush();
    if(FAILED(hr)) 
		return hr;

	return IsSelected() ? pOut->DeliverBeginFlush() : Block(false), S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::EndFlush()
{
	CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csState);

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();
    if(!IsConnected() || !pOut || !pOut->IsConnected())
		return VFW_E_NOT_CONNECTED;

	HRESULT hr = CBaseInputPin::EndFlush();
    if(FAILED(hr)) 
		return hr;

	return IsSelected() ? pOut->DeliverEndFlush() : Block(true), S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::EndOfStream()
{
    CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();
	if(!IsConnected() || !pOut || !pOut->IsConnected())
		return VFW_E_NOT_CONNECTED;

	return IsSelected() ? pOut->DeliverEndOfStream() : S_OK;
}

// IMemInputPin

STDMETHODIMP CStreamSwitcherInputPin::Receive(IMediaSample* pSample)
{
	AM_MEDIA_TYPE* pmt = NULL;
	if(SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt)
	{
		const CMediaType mt(*pmt);
		DeleteMediaType(pmt), pmt = NULL;
		SetMediaType(&mt);
	}

	// DAMN!!!!!! this doesn't work if the stream we are blocking 
	// shares the same thread with another stream, mpeg splitters 
	// are usually like that. Our nicely built up multithreaded 
	// strategy is useless because of this, ARRRRRRGHHHHHH.

#ifdef BLOCKSTREAM
	if(m_fCanBlock)
		m_evBlock.Wait();
#endif

	if(!IsSelected())
	{
#ifdef BLOCKSTREAM
		if(m_fCanBlock)
			return S_FALSE;
#endif

		TRACE(_T("&^%$#@\n"));
//Sleep(32);
		return E_FAIL; // a stupid fix for this stupid problem
	}

    CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();
	ASSERT(pOut->GetConnected());

	HRESULT hr = CBaseInputPin::Receive(pSample);
	if(S_OK != hr) return hr;

	if(m_SampleProps.dwStreamId != AM_STREAM_MEDIA)
	{
		return pOut->Deliver(pSample);
	}

	//

	ALLOCATOR_PROPERTIES props, actual;
	hr = m_pAllocator->GetProperties(&props);
	hr = pOut->CurrentAllocator()->GetProperties(&actual);

	REFERENCE_TIME rtStart = 0, rtStop = 0;
	if(S_OK == pSample->GetTime(&rtStart, &rtStop))
	{
		//
	}

	CMediaType mtOut = m_mt;
	mtOut.lSampleSize = props.cbBuffer;
	mtOut = ((CStreamSwitcherFilter*)m_pFilter)->CreateNewOutputMediaType(mtOut, rtStop-rtStart);

	bool fTypeChanged = false;

	if(mtOut != pOut->CurrentMediaType() || mtOut.lSampleSize > pOut->CurrentMediaType().lSampleSize
	|| ((CStreamSwitcherFilter*)m_pFilter)->m_fResetOutputMediaType)
	{
		((CStreamSwitcherFilter*)m_pFilter)->m_fResetOutputMediaType = false;

		fTypeChanged = true;

		m_SampleProps.dwSampleFlags |= AM_SAMPLE_TYPECHANGED/*|AM_SAMPLE_DATADISCONTINUITY|AM_SAMPLE_TIMEDISCONTINUITY*/;

/*
		if(CComQIPtr<IPinConnection> pPC(pOut->CurrentPinConnection()))
		{
			HANDLE hEOS = CreateEvent(NULL, FALSE, FALSE, NULL);
			hr = pPC->NotifyEndOfStream(hEOS);
			hr = pOut->DeliverEndOfStream();
			WaitForSingleObject(hEOS, 3000);
			CloseHandle(hEOS);
		}
*/
		if(props.cBuffers < 8 && mtOut.majortype == MEDIATYPE_Audio)
			props.cBuffers = 8;

		props.cbBuffer = mtOut.lSampleSize;

		if(/*memcmp(&actual, &props, sizeof(ALLOCATOR_PROPERTIES))*/
		   actual.cbAlign != props.cbAlign
		|| actual.cbPrefix != props.cbPrefix
		|| actual.cBuffers < props.cBuffers
		|| actual.cbBuffer < props.cbBuffer)
		{
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();

			hr = pOut->CurrentAllocator()->Decommit();
			hr = pOut->CurrentAllocator()->SetProperties(&props, &actual);
			hr = pOut->CurrentAllocator()->Commit();
		}
	}

	CComPtr<IMediaSample> pOutSample;
	if(FAILED(InitializeOutputSample(pSample, &pOutSample)))
		return E_FAIL;

	pmt = NULL;
	if(SUCCEEDED(pOutSample->GetMediaType(&pmt)) && pmt)
	{
		const CMediaType mt(*pmt);
		DeleteMediaType(pmt), pmt = NULL;
		// TODO
		ASSERT(0);
	}

	if(fTypeChanged)
	{
		pOut->SetMediaType(&mtOut);
		pOutSample->SetMediaType(&mtOut);
		((CStreamSwitcherFilter*)m_pFilter)->OnNewOutputMediaType(m_mt, mtOut);
	}

	// Transform

	hr = ((CStreamSwitcherFilter*)m_pFilter)->Transform(pSample, pOutSample);

	//

    if(S_OK == hr)
	{
		hr = pOut->Deliver(pOutSample);
        m_bSampleSkipped = FALSE;

		if(FAILED(hr))
		{
			((CStreamSwitcherFilter*)m_pFilter)->ResetOutputMediaType();
		}
	}
    else if(S_FALSE == hr)
	{
		hr = S_OK;
		pOutSample = NULL;
		m_bSampleSkipped = TRUE;
		
		if(!m_bQualityChanged)
		{
			m_pFilter->NotifyEvent(EC_QUALITY_CHANGE, 0, 0);
			m_bQualityChanged = TRUE;
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if(!IsConnected())
		return S_OK;

	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();
    if(!pOut || !pOut->IsConnected())
		return VFW_E_NOT_CONNECTED;

	HRESULT hr = pOut->DeliverNewSegment(tStart, tStop, dRate);

	return hr;
}


//
// CStreamSwitcherOutputPin
//

CStreamSwitcherOutputPin::CStreamSwitcherOutputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(NAME("CStreamSwitcherOutputPin"), pFilter, &pFilter->m_csState, phr, L"Out")
{
//	m_bCanReconnectWhenActive = TRUE;
}

STDMETHODIMP CStreamSwitcherOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv, sizeof(PVOID));
    *ppv = NULL;

    if(riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
	{
        if(m_pStreamSwitcherPassThru == NULL)
		{
			HRESULT hr = S_OK;
			m_pStreamSwitcherPassThru = (IUnknown*)(INonDelegatingUnknown*)
				new CStreamSwitcherPassThru(GetOwner(), &hr, (CStreamSwitcherFilter*)m_pFilter);

			if(!m_pStreamSwitcherPassThru) return E_OUTOFMEMORY;
            if(FAILED(hr)) return hr;
        }

        return m_pStreamSwitcherPassThru->QueryInterface(riid, ppv);
    }
/*
	else if(riid == IID_IStreamBuilder)
	{
		return GetInterface((IStreamBuilder*)this, ppv);		
	}
*/
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CStreamSwitcherOutputPin::QueryAcceptUpstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_FALSE;

	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();

	if(pIn && pIn->IsConnected() && (pIn->IsUsingOwnAllocator() || pIn->CurrentMediaType() == *pmt))
	{
		if(CComQIPtr<IPin> pPinTo = pIn->GetConnected())
		{
			if(S_OK != (hr = pPinTo->QueryAccept(pmt)))
				return VFW_E_TYPE_NOT_ACCEPTED;
		}
		else
		{
			return E_FAIL;
		}
	}

	return hr;
}

// pure virtual

HRESULT CStreamSwitcherOutputPin::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();
	if(!pIn || !pIn->IsConnected()) return E_UNEXPECTED;

	CComPtr<IMemAllocator> pAllocatorIn;
	pIn->GetAllocator(&pAllocatorIn);
	if(!pAllocatorIn) return E_UNEXPECTED;

	HRESULT hr;
    if(FAILED(hr = pAllocatorIn->GetProperties(pProperties))) 
		return hr;

	if(pProperties->cBuffers < 8 && pIn->CurrentMediaType().majortype == MEDIATYPE_Audio)
		pProperties->cBuffers = 8;

	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

	return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR);
}

// virtual

HRESULT CStreamSwitcherOutputPin::CheckConnect(IPin* pPin)
{
	return IsAudioWaveRenderer(GetFilterFromPin(pPin)) ? CBaseOutputPin::CheckConnect(pPin) : E_FAIL;
//	return CComQIPtr<IPinConnection>(pPin) ? CBaseOutputPin::CheckConnect(pPin) : E_NOINTERFACE;
//	return CBaseOutputPin::CheckConnect(pPin);
}

HRESULT CStreamSwitcherOutputPin::BreakConnect()
{
	m_pPinConnection = NULL;
	return CBaseOutputPin::BreakConnect();
}

HRESULT CStreamSwitcherOutputPin::CompleteConnect(IPin* pReceivePin)
{
	m_pPinConnection = CComQIPtr<IPinConnection>(pReceivePin);
	return CBaseOutputPin::CompleteConnect(pReceivePin);
}

HRESULT CStreamSwitcherOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return ((CStreamSwitcherFilter*)m_pFilter)->CheckMediaType(pmt);
}

HRESULT CStreamSwitcherOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();
	if(!pIn || !pIn->IsConnected()) return E_UNEXPECTED;

	CComPtr<IEnumMediaTypes> pEM;
	if(FAILED(pIn->GetConnected()->EnumMediaTypes(&pEM)))
		return VFW_S_NO_MORE_ITEMS;

	if(iPosition > 0 && FAILED(pEM->Skip(iPosition)))
		return VFW_S_NO_MORE_ITEMS;

	AM_MEDIA_TYPE* pmt2 = NULL;
	if(S_OK != pEM->Next(1, &pmt2, NULL) || !pmt2)
		return VFW_S_NO_MORE_ITEMS;

	CopyMediaType(pmt, pmt2);
	DeleteMediaType(pmt2);
/*
	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CopyMediaType(pmt, &pIn->CurrentMediaType());
*/
	return S_OK;
}

// IPin

STDMETHODIMP CStreamSwitcherOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = CBaseOutputPin::QueryAccept(pmt);
	if(S_OK != hr) return hr;

	return QueryAcceptUpstream(pmt);
}

// IQualityControl

STDMETHODIMP CStreamSwitcherOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();
	if(!pIn || !pIn->IsConnected()) return VFW_E_NOT_CONNECTED;
    return pIn->PassNotify(q);
}

// IStreamBuilder

STDMETHODIMP CStreamSwitcherOutputPin::Render(IPin* ppinOut, IGraphBuilder* pGraph)
{
	CComPtr<IBaseFilter> pBF;
	pBF.CoCreateInstance(CLSID_DSoundRender);
	if(!pBF || FAILED(pGraph->AddFilter(pBF, L"Default DirectSound Device")))
	{
		return E_FAIL;
	}

	if(FAILED(pGraph->ConnectDirect(ppinOut, GetFirstDisconnectedPin(pBF, PINDIR_INPUT), NULL)))
	{
		pGraph->RemoveFilter(pBF);
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherOutputPin::Backout(IPin* ppinOut, IGraphBuilder* pGraph)
{
	return S_OK;
}

//
// CStreamSwitcherFilter
//

CStreamSwitcherFilter::CStreamSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid) 
	: CBaseFilter(NAME("CStreamSwitcherFilter"), lpunk, &m_csState, clsid)
	, m_fResetOutputMediaType(false)
{
	if(phr) *phr = S_OK;

	HRESULT hr = S_OK;

	do
	{
		CAutoPtr<CStreamSwitcherInputPin> pInput;
		CAutoPtr<CStreamSwitcherOutputPin> pOutput;

		hr = S_OK;
        pInput.Attach(new CStreamSwitcherInputPin(this, &hr, L"Channel 1"));
		if(!pInput || FAILED(hr)) break;

		hr = S_OK;
		pOutput.Attach(new CStreamSwitcherOutputPin(this, &hr));
        if(!pOutput || FAILED(hr)) break;

		CAutoLock cAutoLock(&m_csPins);
        
		m_pInputs.AddHead(m_pInput = pInput.Detach());
		m_pOutput = pOutput.Detach();

		return;
	}
	while(false);

	if(phr) *phr = E_FAIL;
}

CStreamSwitcherFilter::~CStreamSwitcherFilter()
{
	CAutoLock cAutoLock(&m_csPins);

	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos) delete m_pInputs.GetNext(pos);
	m_pInputs.RemoveAll();
	m_pInput = NULL;

	delete m_pOutput;
	m_pOutput = NULL;
}

STDMETHODIMP CStreamSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAMStreamSelect)
		CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

//

int CStreamSwitcherFilter::GetPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	return(1 + (int)m_pInputs.GetCount());
}

CBasePin* CStreamSwitcherFilter::GetPin(int n)
{
	CAutoLock cAutoLock(&m_csPins);

	if(n < 0 || n >= GetPinCount()) return NULL;
	else if(n == 0) return m_pOutput;
	else return m_pInputs.GetAt(m_pInputs.FindIndex(n-1));
}

int CStreamSwitcherFilter::GetConnectedInputPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	int nConnected = 0;

	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos)
	{
		if(m_pInputs.GetNext(pos)->IsConnected()) 
			nConnected++;
	}

	return(nConnected);
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetConnectedInputPin(int n)
{
	if(n >= 0)
	{
		POSITION pos = m_pInputs.GetHeadPosition();
		while(pos)
		{
			CStreamSwitcherInputPin* pPin = m_pInputs.GetNext(pos);
			if(pPin->IsConnected())
			{
				if(n == 0) return(pPin);
				n--;
			}
		}
	}

	return NULL;
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetInputPin()
{
	return m_pInput;
}

CStreamSwitcherOutputPin* CStreamSwitcherFilter::GetOutputPin()
{
	return m_pOutput;
}

//

HRESULT CStreamSwitcherFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin, IPin* pReceivePin)
{
	if(dir == PINDIR_INPUT)
	{
		CAutoLock cAutoLock(&m_csPins);

		int nConnected = GetConnectedInputPinCount();

		if(nConnected == 1)
		{
			m_pInput = (CStreamSwitcherInputPin*)pPin;
		}

		if(nConnected == m_pInputs.GetCount())
		{
			CStringW name;
			name.Format(L"Channel %d", ++m_PinVersion);

			HRESULT hr = S_OK;
			CStreamSwitcherInputPin* pPin = new CStreamSwitcherInputPin(this, &hr, name);
			if(!pPin || FAILED(hr)) return E_FAIL;
			m_pInputs.AddTail(pPin);
		}
	}

	return S_OK;
}

// this should be very thread safe, I hope it is, it must be... :)

void CStreamSwitcherFilter::SelectInput(CStreamSwitcherInputPin* pInput)
{
	// make sure no input thinks it is active
	m_pInput = NULL;

	// release blocked GetBuffer in our own allocator & block all Receive
	POSITION pos = m_pInputs.GetHeadPosition();
	while(pos)
	{
		CStreamSwitcherInputPin* pPin = m_pInputs.GetNext(pos);
		pPin->Block(false);
		// a few Receive calls can arrive here, but since m_pInput == NULL neighter of them gets delivered
		pPin->Block(true);
	}

	// this will let waiting GetBuffer() calls go on inside our Receive()
	if(m_pOutput)
	{
		m_pOutput->DeliverBeginFlush();
		m_pOutput->DeliverEndFlush();

		ResetOutputMediaType();
	}

	if(!pInput) return;

	// set new input
	m_pInput = pInput;

	// let it go
	m_pInput->Block(false);
}

//

HRESULT CStreamSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	HRESULT hr;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;
	if(FAILED(hr = pOut->GetPointer(&pDataOut))) return hr;

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if(!pDataIn || !pDataOut /*|| len > size || len <= 0*/) return S_FALSE; // FIXME

	memcpy(pDataOut, pDataIn, min(len, size));
	pOut->SetActualDataLength(min(len, size));

	return S_OK;
}

CMediaType CStreamSwitcherFilter::CreateNewOutputMediaType(CMediaType mt, REFERENCE_TIME rtLen)
{
	return(mt);
}

// IAMStreamSelect

STDMETHODIMP CStreamSwitcherFilter::Count(DWORD* pcStreams)
{
	if(!pcStreams) return E_POINTER;

	CAutoLock cAutoLock(&m_csPins);

	*pcStreams = GetConnectedInputPinCount();

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	CAutoLock cAutoLock(&m_csPins);

	CBasePin* pPin = GetConnectedInputPin(lIndex);
	if(!pPin) return E_INVALIDARG;

	if(ppmt)
		*ppmt = CreateMediaType(&m_pOutput->CurrentMediaType());

	if(pdwFlags)
		*pdwFlags = (m_pInput == pPin) ? AMSTREAMSELECTINFO_EXCLUSIVE : 0;

	if(plcid)
		*plcid = 0;

	if(pdwGroup)
		*pdwGroup = 0;

	if(ppszName && (*ppszName = (WCHAR*)CoTaskMemAlloc((wcslen(pPin->Name())+1)*sizeof(WCHAR))))
		wcscpy(*ppszName, pPin->Name());

	if(ppObject)
		*ppObject = NULL;

	if(ppUnk)
		*ppUnk = NULL;

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	if(dwFlags != AMSTREAMSELECTENABLE_ENABLE)
		return E_NOTIMPL;

	PauseGraph;

	CStreamSwitcherInputPin* pNewInput = GetConnectedInputPin(lIndex);
	if(!pNewInput) return E_INVALIDARG;

	SelectInput(pNewInput);

	ResumeGraph;

	return S_OK;
}

//////////

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesIn)/sizeof(sudPinTypesIn[0]), // Number of types
      sudPinTypesIn		// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      sizeof(sudPinTypesOut)/sizeof(sudPinTypesOut[0]), // Number of types
      sudPinTypesOut		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter =
{
    &__uuidof(CAudioSwitcherFilter),	// Filter CLSID
    L"AudioSwitcher",			// String name
    MERIT_DO_NOT_USE,       // Filter merit // MERIT_PREFERRED+1
    sizeof(sudpPins)/sizeof(sudpPins[0]), // Number of pins
    sudpPins                // Pin information
};

CFactoryTemplate g_Templates[] =
{
    { L"AudioSwitcher"
    , &__uuidof(CAudioSwitcherFilter)
    , CAudioSwitcherFilter::CreateInstance
    , NULL
    , &sudFilter }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CAudioSwitcherFilter
//

CUnknown* WINAPI CAudioSwitcherFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CAudioSwitcherFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// CAudioSwitcherFilter
//

CAudioSwitcherFilter::CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CStreamSwitcherFilter(lpunk, phr, __uuidof(this))
{
	if(phr)
	{
		if(FAILED(*phr)) return;
		else *phr = S_OK;
	}

	m_fCustomChannelMapping = false;
	memset(m_pSpeakerToChannelMap, 0, sizeof(m_pSpeakerToChannelMap));
	m_fDownSampleTo441 = false;
	m_rtAudioTimeShift = 0;
	m_rtNextStart = 0;
	m_rtNextStop = 1;
}

STDMETHODIMP CAudioSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAudioSwitcherFilter)
		CStreamSwitcherFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAudioSwitcherFilter::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->formattype == FORMAT_WaveFormatEx
	&& ((WAVEFORMATEX*)pmt->pbFormat)->nChannels > 2
	&& ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
		return VFW_E_INVALIDMEDIATYPE; // stupid iviaudio tries to fool us

	return (pmt->majortype == MEDIATYPE_Audio
			&& pmt->formattype == FORMAT_WaveFormatEx
			&& (((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 8
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 16
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 24
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wBitsPerSample == 32)
			&& (((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_PCM
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_IEEE_FLOAT
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF
				|| ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_EXTENSIBLE))
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

#define mixchannels(type, sumtype, mintype, maxtype) \
	sumtype sum = 0; \
	int num = 0; \
	for(int j = 0; j < 18 && j < wfe->nChannels; j++) \
	{ \
		if(Channel&(1<<j)) \
		{ \
			num++; \
			sum += *(type*)&pDataIn[bps*(j + wfe->nChannels*k)]; \
		} \
	} \
	sum = min(max(sum, mintype), maxtype); \
	*(type*)&pDataOut[bps*(i + wfeout->nChannels*k)] = (type)sum; \

HRESULT CAudioSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if(!pInPin || !pOutPin) 
		return __super::Transform(pIn, pOut);

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;
	WAVEFORMATEX* wfeout = (WAVEFORMATEX*)pOutPin->CurrentMediaType().pbFormat;
	WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
	WAVEFORMATEXTENSIBLE* wfexout = (WAVEFORMATEXTENSIBLE*)wfeout;

	int bps = wfe->wBitsPerSample>>3;

	int len = pIn->GetActualDataLength() / (bps*wfe->nChannels);
	int lenout = len * wfeout->nSamplesPerSec / wfe->nSamplesPerSec;

	REFERENCE_TIME rtStart, rtStop;
	if(SUCCEEDED(pIn->GetTime(&rtStart, &rtStop)))
	{
		rtStart += m_rtAudioTimeShift;
		rtStop += m_rtAudioTimeShift;
		pOut->SetTime(&rtStart, &rtStop);

		m_rtNextStart = rtStart;
		m_rtNextStop = rtStop;
	}
	else
	{
		pOut->SetTime(&m_rtNextStart, &m_rtNextStop);
	}

	m_rtNextStart += 10000000i64*len/wfe->nSamplesPerSec;
	m_rtNextStop += 10000000i64*len/wfe->nSamplesPerSec;

	bool fPCM = wfe->wFormatTag == WAVE_FORMAT_PCM
		|| wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;

	bool fFloat = wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT
		|| wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

	if(!fPCM && !fFloat)
		return __super::Transform(pIn, pOut);

	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	HRESULT hr;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;
	if(FAILED(hr = pOut->GetPointer(&pDataOut))) return hr;

	if(!pDataIn || !pDataOut || len <= 0 || lenout <= 0) return S_FALSE;

	memset(pDataOut, 0, pOut->GetSize());

	if(m_fCustomChannelMapping)
	{
		if(m_chs[wfe->nChannels-1].GetCount() > 0)
		{
			for(int i = 0; i < wfeout->nChannels; i++)
			{
				DWORD Channel = m_chs[wfe->nChannels-1][i].Channel, nChannels = 0;

				for(int k = 0; k < len; k++)
				{
					if(fPCM && wfe->wBitsPerSample == 8)
					{
						mixchannels(unsigned char, __int64, 0, UCHAR_MAX);
					}
					else if(fPCM && wfe->wBitsPerSample == 16)
					{
						mixchannels(short, __int64, SHRT_MIN, SHRT_MAX);
					}
					else if(fPCM && wfe->wBitsPerSample == 24)
					{
//						mixchannels(_int24, __int64, _INT24_MIN, _INT24_MAX);

						__int64 sum = 0;
						int num = 0;
						for(int j = 0; j < 18 && j < wfe->nChannels; j++)
						{
							if(Channel&(1<<j))
							{
								num++;
								int tmp;
								memcpy((BYTE*)&tmp+1, &pDataIn[bps*(j + wfe->nChannels*k)], 3);
								tmp>>=8;
								sum += tmp;
							}
						}
						sum = min(max(sum, -(1<<24)), (1<<24)-1);
						memcpy(&pDataOut[bps*(i + wfeout->nChannels*k)], (BYTE*)&sum, 3);
					}
					else if(fPCM && wfe->wBitsPerSample == 32)
					{
						mixchannels(int, __int64, INT_MIN, INT_MAX);
					}
					else if(fFloat && wfe->wBitsPerSample == 32)
					{
						mixchannels(float, double, -1, 1);
					}
					else if(fFloat && wfe->wBitsPerSample == 64)
					{
						mixchannels(double, double, -1, 1);
					}
				}
			}
		}
		else
		{
			BYTE* pDataOut = NULL;
			HRESULT hr;
			if(FAILED(hr = pOut->GetPointer(&pDataOut)) || !pDataOut) return hr;
			memset(pDataOut, 0, pOut->GetSize());
		}
	}
	else
	{
		HRESULT hr;
		if(S_OK != (hr = CStreamSwitcherFilter::Transform(pIn, pOut)))
			return hr;
	}

	if(m_fDownSampleTo441
	&& wfe->nSamplesPerSec > 44100 && wfeout->nSamplesPerSec == 44100 
	&& wfe->wBitsPerSample <= 16 && fPCM)
	{
		if(BYTE* buff = new BYTE[len*bps])
		{
			for(int ch = 0; ch < wfeout->nChannels; ch++)
			{
				memset(buff, 0, len*bps);

				for(int i = 0; i < len; i++)
					memcpy(buff + i*bps, (char*)pDataOut + (ch + i*wfeout->nChannels)*bps, bps);

				m_pResamplers[ch]->Downsample(buff, len, buff, lenout);

				for(int i = 0; i < lenout; i++)
					memcpy((char*)pDataOut + (ch + i*wfeout->nChannels)*bps, buff + i*bps, bps);
			}

			delete [] buff;
		}
	}

	pOut->SetActualDataLength(lenout*bps*wfeout->nChannels);

	return S_OK;
}

CMediaType CAudioSwitcherFilter::CreateNewOutputMediaType(CMediaType mt, REFERENCE_TIME rtLen)
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	CStreamSwitcherOutputPin* pOutPin = GetOutputPin();
	if(!pInPin || !pOutPin || ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag == WAVE_FORMAT_DOLBY_AC3_SPDIF) 
		return __super::CreateNewOutputMediaType(mt, rtLen);

	if(m_fCustomChannelMapping)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;

		m_chs[wfe->nChannels-1].RemoveAll();

		DWORD mask = DWORD((__int64(1)<<wfe->nChannels)-1);
		for(int i = 0; i < 18; i++)
		{
			if(m_pSpeakerToChannelMap[wfe->nChannels-1][i]&mask)
			{
				ChMap cm = {1<<i, m_pSpeakerToChannelMap[wfe->nChannels-1][i]};
				m_chs[wfe->nChannels-1].Add(cm);
			}
		}

		if(m_chs[wfe->nChannels-1].GetCount() > 0)
		{
			mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
			WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)mt.pbFormat;
			wfex->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
			wfex->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			wfex->Samples.wValidBitsPerSample = wfe->wBitsPerSample;
			wfex->SubFormat = 
				wfe->wFormatTag == WAVE_FORMAT_PCM ? KSDATAFORMAT_SUBTYPE_PCM :
				wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT :
				wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? ((WAVEFORMATEXTENSIBLE*)wfe)->SubFormat :
				KSDATAFORMAT_SUBTYPE_PCM; // can't happen

			wfex->dwChannelMask = 0;
			for(int i = 0; i < m_chs[wfe->nChannels-1].GetCount(); i++)
				wfex->dwChannelMask |= m_chs[wfe->nChannels-1][i].Speaker;

			wfex->Format.nChannels = (WORD)m_chs[wfe->nChannels-1].GetCount();
			wfex->Format.nBlockAlign = wfex->Format.nChannels*wfex->Format.wBitsPerSample>>3;
			wfex->Format.nAvgBytesPerSec = wfex->Format.nBlockAlign*wfex->Format.nSamplesPerSec;
		}
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;

	if(m_fDownSampleTo441)
	{
		if(wfe->nSamplesPerSec > 44100 && wfe->wBitsPerSample <= 16)
		{
			wfe->nSamplesPerSec = 44100;
			wfe->nAvgBytesPerSec = wfe->nBlockAlign*wfe->nSamplesPerSec;
		}
	}

	mt.lSampleSize = (ULONG)max(mt.lSampleSize, wfe->nAvgBytesPerSec * rtLen / 10000000i64);
	mt.lSampleSize = (mt.lSampleSize + (wfe->nBlockAlign-1)) & ~(wfe->nBlockAlign-1);

	return mt;
}

void CAudioSwitcherFilter::OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut)
{
	const WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn.pbFormat;
	const WAVEFORMATEX* wfeout = (WAVEFORMATEX*)mtOut.pbFormat;

	m_pResamplers.RemoveAll();
	for(int i = 0; i < wfeout->nChannels; i++)
	{
		CAutoPtr<AudioStreamResampler> pResampler;
		pResampler.Attach(new AudioStreamResampler(wfeout->wBitsPerSample>>3, wfe->nSamplesPerSec, wfeout->nSamplesPerSec, true));
		m_pResamplers.Add(pResampler);
	}
}

// IAudioSwitcherFilter

STDMETHODIMP CAudioSwitcherFilter::GetInputSpeakerConfig(DWORD* pdwChannelMask)
{
	if(!pdwChannelMask) 
		return E_POINTER;

	*pdwChannelMask = 0;

	CStreamSwitcherInputPin* pInPin = GetInputPin();
	if(!pInPin || !pInPin->IsConnected())
		return E_UNEXPECTED;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat;

	if(wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
		*pdwChannelMask = wfex->dwChannelMask;
	}
	else
	{
		*pdwChannelMask = 0/*wfe->nChannels == 1 ? 4 : wfe->nChannels == 2 ? 3 : 0*/;
	}

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::GetSpeakerConfig(bool* pfCustomChannelMapping, DWORD pSpeakerToChannelMap[18][18])
{
	if(pfCustomChannelMapping) *pfCustomChannelMapping = m_fCustomChannelMapping;
	memcpy(pSpeakerToChannelMap, m_pSpeakerToChannelMap, sizeof(m_pSpeakerToChannelMap));

	return S_OK;
}

STDMETHODIMP CAudioSwitcherFilter::SetSpeakerConfig(bool fCustomChannelMapping, DWORD pSpeakerToChannelMap[18][18])
{
	if(m_State == State_Stopped || m_fCustomChannelMapping != fCustomChannelMapping
	|| memcmp(m_pSpeakerToChannelMap, pSpeakerToChannelMap, sizeof(m_pSpeakerToChannelMap)))
	{
		PauseGraph;
		
		CStreamSwitcherInputPin* pInput = GetInputPin();

		SelectInput(NULL);

		m_fCustomChannelMapping = fCustomChannelMapping;
		memcpy(m_pSpeakerToChannelMap, pSpeakerToChannelMap, sizeof(m_pSpeakerToChannelMap));

		SelectInput(pInput);

		ResumeGraph;
	}

	return S_OK;
}

STDMETHODIMP_(int) CAudioSwitcherFilter::GetNumberOfInputChannels()
{
	CStreamSwitcherInputPin* pInPin = GetInputPin();
	return pInPin ? ((WAVEFORMATEX*)pInPin->CurrentMediaType().pbFormat)->nChannels : 0;
}

STDMETHODIMP_(bool) CAudioSwitcherFilter::IsDownSamplingTo441Enabled()
{
	return(m_fDownSampleTo441);
}

STDMETHODIMP CAudioSwitcherFilter::EnableDownSamplingTo441(bool fEnable)
{
	if(m_fDownSampleTo441 != fEnable)
	{
		PauseGraph;

		m_fDownSampleTo441 = fEnable;

		ResumeGraph;
	}

	return S_OK;
}

STDMETHODIMP_(REFERENCE_TIME) CAudioSwitcherFilter::GetAudioTimeShift()
{
	return(m_rtAudioTimeShift);
}

STDMETHODIMP CAudioSwitcherFilter::SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift)
{
	m_rtAudioTimeShift = rtAudioTimeShift;
	return S_OK;
}
