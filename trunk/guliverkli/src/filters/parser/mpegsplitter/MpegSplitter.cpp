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

#include "StdAfx.h"
#include <mmreg.h>
#include <initguid.h>
#include "MpegSplitter.h"
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
//	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD}, // cdxa filter should take care of this
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_PROGRAM},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
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
      countof(sudPinTypesIn), // Number of types
      sudPinTypesIn         // Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      0,					// Number of types
      NULL					// Pin information
    },
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMpegSplitterFilter), L"Mpeg Splitter", /*MERIT_NORMAL+1*/ MERIT_DO_NOT_USE, countof(sudpPins), sudpPins},
	{&__uuidof(CMpegSourceFilter), L"Mpeg Source", MERIT_DO_NOT_USE, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
	{L"Mpeg Splitter", &__uuidof(CMpegSplitterFilter), CMpegSplitterFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"Mpeg Source", &__uuidof(CMpegSourceFilter), CMpegSourceFilter::CreateInstance, NULL, &sudFilter[1]},
};

int g_cTemplates = countof(g_Templates);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG1System, _T("0,16,FFFFFFFFF100010001800001FFFFFFFF,000001BA2100010001800001000001BB"), NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_PROGRAM, _T("0,5,FFFFFFFFC0,000001BA40"), NULL);
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MPEG2_TRANSPORT, _T("0,4,,47,188,4,,47,376,4,,47"), NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
//	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG1System);
//	UnRegisterSourceFilter(MEDIASUBTYPE_MPEG2_PROGRAM);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI CMpegSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMpegSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CUnknown* WINAPI CMpegSourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMpegSourceFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// CMpegSplitterFilter
//

CMpegSplitterFilter::CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMpegSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

STDMETHODIMP CMpegSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return 
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMpegSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();

	m_pFile.Attach(new CMpegSplitterFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	//

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	for(int i = 0; i < countof(m_pFile->m_streams); i++)
	{
		POSITION pos = m_pFile->m_streams[i].GetHeadPosition();
		while(pos)
		{
			CMpegSplitterFile::stream& s = m_pFile->m_streams[i].GetNext(pos);

			CArray<CMediaType> mts;
			mts.Add(s.mt);

			CStringW name = CMpegSplitterFile::CStreamList::ToString(i);

			HRESULT hr;
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(new CBaseSplitterOutputPin(mts, name, this, this, &hr));
			if(S_OK == AddOutputPin(s, pPinOut))
				break;
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration = 10000000i64 * m_pFile->GetLength() / m_pFile->m_rate;

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMpegSplitterFilter::InitDeliverLoop()
{
	if(!m_pFile) return(false);

	// TODO

	return(true);
}

void CMpegSplitterFilter::SeekDeliverLoop(REFERENCE_TIME rt)
{
	if(rt <= 0 || m_rtDuration <= 0)
	{
		m_pFile->Seek(0);
	}
	else
	{
		__int64 len = m_pFile->GetLength();
		__int64 seekpos = (__int64)(1.0*rt/m_rtDuration*len);
		__int64 minseekpos = _I64_MAX;

		REFERENCE_TIME rtmax = rt + m_pFile->m_rtMin;
		REFERENCE_TIME rtmin = rtmax - 5000000i64;

		for(int i = 0; i < countof(m_pFile->m_streams)-1; i++)
		{
			POSITION pos = m_pFile->m_streams[i].GetHeadPosition();
			while(pos)
			{
				DWORD TrackNum = m_pFile->m_streams[i].GetNext(pos);

				CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNum);
				if(pPin && pPin->IsConnected())
				{
					m_pFile->Seek(seekpos);

					REFERENCE_TIME pdt = _I64_MIN;

					for(int j = 0; j < 10; j++)
					{
						REFERENCE_TIME rt = m_pFile->NextPTS(TrackNum);
						// TRACE(_T("[%d/%04x]: rt=%I64d, fp=%I64d\n"), i, TrackNum, rt, m_pFile->GetPos());
				
						if(rt < 0) break;

						REFERENCE_TIME dt = rt - rtmax;
						if(dt > 0 && dt == pdt) dt = 10000000i64;

						// TRACE(_T("dt=%I64d\n"), dt);

						if(rtmin <= rt && rt <= rtmax || pdt > 0 && dt < 0)
						{
							// TRACE(_T("minseekpos: %I64d -> "), minseekpos);
							minseekpos = min(minseekpos, m_pFile->GetPos());
							// TRACE(_T("%I64d\n"), minseekpos);
							break;
						}

						m_pFile->Seek(m_pFile->GetPos() - (__int64)(1.0*dt/m_rtDuration*len));
		
						pdt = dt;
					}
				}
			}
		}

		if(minseekpos != _I64_MAX) seekpos = minseekpos;
		m_pFile->Seek(seekpos);
	}
}

void CMpegSplitterFilter::DoDeliverLoop()
{
	HRESULT hr = S_OK;

	int skip = 0;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetPos() < m_pFile->GetLength())
	{
		BYTE b;

		if(m_pFile->m_type == CMpegSplitterFile::ps || m_pFile->m_type == CMpegSplitterFile::es)
		{
			if(!m_pFile->Next(b))
			{
				m_pFile->Seek(m_pFile->GetPos() + skip);
				skip = max(1, skip) * 2;
				continue;
			}

			skip = 0;

			if(b == 0xba) // program stream header
			{
				CMpegSplitterFile::pshdr h;
				if(!m_pFile->Read(h)) continue;
			}
			else if(b == 0xbb) // program stream system header
			{
				CMpegSplitterFile::pssyshdr h;
				if(!m_pFile->Read(h)) continue;
			}
			else if(b >= 0xbd && b < 0xf0) // pes packet
			{
				CMpegSplitterFile::peshdr h;
				if(!m_pFile->Read(h, b) || !h.len) continue;

				if(h.type == CMpegSplitterFile::mpeg2 && h.scrambling) {ASSERT(0); break;}

				__int64 pos = m_pFile->GetPos();

				DWORD TrackNumber = m_pFile->AddStream(0, b, h.len);

				if(GetOutputPin(TrackNumber))
				{
					CAutoPtr<Packet> p(new Packet());
					p->TrackNumber = TrackNumber;
					p->bSyncPoint = !!h.fpts;
					p->rtStart = h.fpts ? (h.pts - m_pFile->m_rtMin) : Packet::INVALID_TIME;
					p->rtStop = p->rtStart+1;
					p->pData.SetSize(h.len - (m_pFile->GetPos() - pos));
					m_pFile->ByteRead(p->pData.GetData(), h.len - (m_pFile->GetPos() - pos));
					hr = DeliverPacket(p);
				}

				m_pFile->Seek(pos + h.len);
			}
		}
		else if(m_pFile->m_type == CMpegSplitterFile::ts)
		{
			CMpegSplitterFile::trhdr h;
			if(!m_pFile->Read(h))
			{
				m_pFile->Seek(m_pFile->GetPos() + skip);
				skip = max(1, skip) * 2;
				continue;
			}

			skip = 0;

			if(h.scrambling) {ASSERT(0); break;}

			__int64 pos = m_pFile->GetPos();

			if(h.payload && h.pid >= 16 && h.pid < 0x1fff)
			{
				DWORD TrackNumber = h.pid;

				CMpegSplitterFile::peshdr h2;
				if(h.payloadstart && m_pFile->Next(b, 4) && m_pFile->Read(h2, b)) // pes packet
				{
					if(h2.type == CMpegSplitterFile::mpeg2 && h2.scrambling) {ASSERT(0); break;}
					TrackNumber = m_pFile->AddStream(h.pid, b, h.bytes - (m_pFile->GetPos() - pos));
				}

				if(GetOutputPin(TrackNumber))
				{
					CAutoPtr<Packet> p(new Packet());
					p->TrackNumber = TrackNumber;
					p->bSyncPoint = !!h2.fpts;
					p->rtStart = h2.fpts ? (h2.pts - m_pFile->m_rtMin) : Packet::INVALID_TIME;
					p->rtStop = p->rtStart+1;
					p->pData.SetSize(h.bytes - (m_pFile->GetPos() - pos));
					m_pFile->ByteRead(p->pData.GetData(), h.bytes - (m_pFile->GetPos() - pos));
					hr = DeliverPacket(p);
				}
			}

			m_pFile->Seek(pos + h.bytes);
		}
	}
}

// IAMStreamSelect

STDMETHODIMP CMpegSplitterFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	*pcStreams = 0;
	for(int i = 0; i < countof(m_pFile->m_streams); i++)
		(*pcStreams) += m_pFile->m_streams[i].GetCount();

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::Enable(long lIndex, DWORD dwFlags)
{
	if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
		return E_NOTIMPL;

	for(int i = 0, j = 0; i < countof(m_pFile->m_streams); i++)
	{
		int cnt = m_pFile->m_streams[i].GetCount();
		
		if(lIndex >= j && lIndex < j+cnt)
		{
			lIndex -= j;

			POSITION pos = m_pFile->m_streams[i].FindIndex(lIndex);
			if(!pos) return E_UNEXPECTED;

			DWORD from = 0;
			CMpegSplitterFile::stream& to = m_pFile->m_streams[i].GetAt(pos);

			pos = m_pFile->m_streams[i].GetHeadPosition();
			while(pos)
			{
				CMpegSplitterFile::stream& from = m_pFile->m_streams[i].GetNext(pos);
				if(GetOutputPin(from))
					return RenameOutputPin(from, to, &to.mt);
			}
		}

		j += cnt;
	}

	return S_FALSE;
}

STDMETHODIMP CMpegSplitterFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	for(int i = 0, j = 0; i < countof(m_pFile->m_streams); i++)
	{
		int cnt = m_pFile->m_streams[i].GetCount();
		
		if(lIndex >= j && lIndex < j+cnt)
		{
			lIndex -= j;
			
			POSITION pos = m_pFile->m_streams[i].FindIndex(lIndex);
			if(!pos) return E_UNEXPECTED;

			CMpegSplitterFile::stream& s = m_pFile->m_streams[i].GetAt(pos);

			if(ppmt) *ppmt = CreateMediaType(&s.mt);
			if(pdwFlags) *pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
			if(plcid) *plcid = 0;
			if(pdwGroup) *pdwGroup = i;
			if(ppObject) *ppObject = NULL;
			if(ppUnk) *ppUnk = NULL;
			
			if(ppszName)
			{
				*ppszName = NULL;

				CStringW name = CMpegSplitterFile::CStreamList::ToString(i);

				CStringW str;
				str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id); // TODO: make this nicer

				*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
				if(*ppszName == NULL) return E_OUTOFMEMORY;
				wcscpy(*ppszName, str);
			}
		}

		j += cnt;
	}

	return S_OK;
}

//
// CMpegSourceFilter
//

CMpegSourceFilter::CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMpegSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
