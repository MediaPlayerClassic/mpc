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
#include "..\BaseSplitter\BaseSplitter.h"

class CAviFile;

class CAviSplitterOutputPin : public CBaseSplitterOutputPin
{
public:
	CAviSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT CheckConnect(IPin* pPin);
};

[uuid("9736D831-9D6C-4E72-B6E7-560EF9181001")]
class CAviSplitterFilter : public CBaseSplitterFilter, public IPropertyBag
{
	CAutoVectorPtr<DWORD> m_tFrame;
	CAutoVectorPtr<UINT64> m_tSize, m_tFilePos;

	REFERENCE_TIME m_rtDuration;

protected:
	CAutoPtr<CAviFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool InitDeliverLoop();
	void SeekDeliverLoop(REFERENCE_TIME rt);
	void DoDeliverLoop();

	HRESULT ReIndex(UINT64 end);
	HRESULT DoDeliverLoop(UINT64 end);

public:
	CAviSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CAviSplitterFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IMediaSeeking

	STDMETHODIMP GetDuration(LONGLONG* pDuration);

	// IPropertyBag

	STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog);
	STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT* pVar);
};

[uuid("CEA8DEFF-0AF7-4DB9-9A38-FB3C3AEFC0DE")]
class CAviSourceFilter : public CAviSplitterFilter
{
public:
	CAviSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};
