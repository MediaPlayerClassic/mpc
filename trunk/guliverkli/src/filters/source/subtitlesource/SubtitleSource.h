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
#include <afxtempl.h>
#include "..\..\..\subtitles\RTS.h"

[uuid("E288E4DF-C762-4809-A8D8-1D7C1E3CCD97")]
class CSubtitleSource
	: public CSource
	, public IFileSourceFilter
	, public IAMFilterMiscFlags
{
	CStringW m_fn;

public:
	CSubtitleSource(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CSubtitleSource();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IAMFilterMiscFlags
	STDMETHODIMP_(ULONG) GetMiscFlags();
};

class CSubtitleStream 
	: public CSourceStream
	, public CSourceSeeking
{
	CCritSec m_cSharedState;

	int m_nPosition;

	BOOL m_bDiscontinuity, m_bFlushing;

	HRESULT OnThreadStartPlay();
	HRESULT OnThreadCreate();

	void UpdateFromSeek();
	STDMETHODIMP SetRate(double dRate);

	HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate() {return S_OK;}

protected:
	CRenderedTextSubtitle m_rts;

public:
    CSubtitleStream(const WCHAR* wfn, CSubtitleSource* pParent, HRESULT* phr);
	virtual ~CSubtitleStream();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT FillBuffer(IMediaSample* pSample);
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	
	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};

