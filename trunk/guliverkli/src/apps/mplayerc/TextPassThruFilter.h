/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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

#include "..\..\subtitles\STS.h"

[uuid("E2BA9B7B-B65D-4804-ACB2-89C3E55511DB")]
class CTextPassThruFilter : public CTransformFilter
{
	REFERENCE_TIME m_rtOffset;

	CMainFrame* m_pMainFrame;
	CComPtr<ISubStream> m_pRTS;

	void SetName();

	CCritSec m_csReceive;

public:
	CTextPassThruFilter(CMainFrame* pMainFrame);
	virtual ~CTextPassThruFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return 
			m_pRTS && riid == __uuidof(ISubStream) ? m_pRTS->QueryInterface(riid, ppv) :
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

    HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT BreakConnect(PIN_DIRECTION dir);
	HRESULT CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin);
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};
