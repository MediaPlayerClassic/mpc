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

#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include "OggFile.h"
#include "..\BaseSplitter\BaseSplitter.h"
/*
class OggPacket : public Packet
{
public:
	bool fStartValid;
};
*/
class COggSplitterOutputPin : public CBaseSplitterOutputPin
{
protected:
	CCritSec m_csPackets;
	CAutoPtrList<Packet> m_packets;
	CAutoPtr<Packet> m_lastpacket;
	REFERENCE_TIME m_rt;

public:
	COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT UnpackPage(OggPage& page);
	virtual HRESULT UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len) = 0;
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position) = 0;
	CAutoPtr<Packet> GetPacket();
    
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

class COggVorbisOutputPin : public COggSplitterOutputPin
{
	DWORD m_audio_sample_rate;

	virtual HRESULT UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggDirectShowOutputPin : public COggSplitterOutputPin
{
	virtual HRESULT UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggStreamOutputPin : public COggSplitterOutputPin
{
	__int64 m_time_unit, m_samples_per_unit;
	DWORD m_default_len;

	virtual HRESULT UnpackPacket(CAutoPtr<Packet>& p, BYTE* pData, int len);
	virtual REFERENCE_TIME GetRefTime(__int64 granule_position);

public:
	COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggVideoOutputPin : public COggStreamOutputPin
{
public:
	COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggAudioOutputPin : public COggStreamOutputPin
{
public:
	COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

class COggTextOutputPin : public COggStreamOutputPin
{
public:
	COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
};

[uuid("9FF48807-E133-40AA-826F-9B2959E5232D")]
class COggSplitterFilter : public CBaseSplitterFilter
{
	REFERENCE_TIME m_rtDuration;

protected:
	CAutoPtr<COggFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool InitDeliverLoop();
	void SeekDeliverLoop(REFERENCE_TIME rt);
	void DoDeliverLoop();

public:
	COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~COggSplitterFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	// IMediaSeeking

	STDMETHODIMP GetDuration(LONGLONG* pDuration);
};

[uuid("6D3688CE-3E9D-42F4-92CA-8A11119D25CD")]
class COggSourceFilter : public COggSplitterFilter
{
public:
	COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};
