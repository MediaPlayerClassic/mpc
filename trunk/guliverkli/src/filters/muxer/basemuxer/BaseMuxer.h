/* 
 *	Copyright (C) 2003-2004 Gabest
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

#pragma once

#include "BaseMuxerInputPin.h"
#include "BaseMuxerOutputPin.h"

class CBaseMuxerFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IMediaSeeking
{
protected:
	struct Packet
	{
		CBaseMuxerInputPin* pPin;
		REFERENCE_TIME rtStart, rtStop;
		CArray<BYTE> pData;
		enum flag_t {empty = 0, timevalid = 1, syncpoint = 2, discontinuity = 4, eos = 8};
		DWORD flags;
		struct Packet() {pPin = NULL; rtStart = rtStop = _I64_MIN; flags = empty;}
		bool IsTimeValid() {return !!(flags & timevalid);}
		bool IsSyncPoint() {return !!(flags & syncpoint);}
		bool IsDiscontinuity() {return !!(flags & discontinuity);}
		bool IsEOS() {return !!(flags & eos);}
	};

	struct Index
	{
		bool fSyncPoint;
		UINT64 fp;
		REFERENCE_TIME rt;
		struct Index(bool fSyncPoint = false, UINT64 fp = 0, REFERENCE_TIME rt = _I64_MIN)
		{
			this->fSyncPoint = fSyncPoint;
			this->fp = fp;
			this->rt = rt;
		}
	};

	class CIndexList : public CList<Index> {public: void operator = (CIndexList& l) {RemoveAll(); AddTail(&l);}};
	typedef CMap<CBaseMuxerInputPin*, CBaseMuxerInputPin*, CIndexList, CIndexList&> CIndexMap;

	class CStream
	{
		CComPtr<IStream> m_pStream;
		UINT64 m_bitbuff;
		int m_bitlen;

	public:
		CStream(IStream* pStream);
		virtual ~CStream();

		UINT64 GetPos();
		UINT64 Seek(UINT64 pos); // it's a _stream_, please don't seek if you don't have to 

		void ByteWrite(const void* pData, int len);
		void BitWrite(UINT64 data, int len);
		void BitFlush();
	};

private:
	CAutoPtrList<CBaseMuxerInputPin> m_pInputs;
	CAutoPtr<CBaseMuxerOutputPin> m_pOutput;

	REFERENCE_TIME m_rtCurrent;

	CCritSec m_csQueue;
	CAutoPtrList<Packet> m_queue;
	CMap<CBaseMuxerInputPin*, CBaseMuxerInputPin*, int, int&> m_pActivePins;
	bool PeekQueue();

	enum {CMD_EXIT, CMD_RUN};
	DWORD ThreadProc();

protected:
	CList<CBaseMuxerInputPin*> m_pPins;
	CAutoPtr<CStream> m_pStream;
	CIndexMap m_pIndexMap, m_pIndexMapSyncPoint;

	virtual void WriteHeader() = 0;
	virtual void WritePacket(Packet* pPacket) = 0;
	virtual void WriteFooter() = 0;

public:
	CBaseMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid);
	virtual ~CBaseMuxerFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	void AddInput();
	void Receive(IMediaSample* pIn, CBaseMuxerInputPin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

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
};

