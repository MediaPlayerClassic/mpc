/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "MP4Splitter.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

#include "Ap4.h"
#include "Ap4File.h"
#include "Ap4StssAtom.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MP4},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMP4SplitterFilter), L"MP4 Splitter", MERIT_NORMAL, countof(sudpPins), sudpPins},
	{&__uuidof(CMP4SourceFilter), L"MP4 Source", MERIT_NORMAL, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMP4SplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CMP4SourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	CList<CString> chkbytes;

	chkbytes.AddTail(_T("4,4,,66747970")); // ftyp
	chkbytes.AddTail(_T("4,4,,6d6f6f76")); // moov
	chkbytes.AddTail(_T("4,4,,6d646174")); // mdat
	chkbytes.AddTail(_T("4,4,,736b6970")); // skip

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_MP4, chkbytes, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_MP4);

	return AMovieDllRegisterServer2(TRUE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, dwReason, 0); // "DllMain" of the dshow baseclasses;
}

#endif

//
// CMP4SplitterFilter
//

CMP4SplitterFilter::CMP4SplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMP4SplitterFilter"), pUnk, phr, __uuidof(this))
{
}

CMP4SplitterFilter::~CMP4SplitterFilter()
{
}

HRESULT CMP4SplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_trackpos.RemoveAll();

	m_pFile.Free();
	m_pFile.Attach(new CMP4SplitterFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->m_rtDuration;

	CArray<DWORD> ids;

	POSITION pos = m_pFile->m_mts.GetStartPosition();
	while(pos)
	{
		DWORD id;
		CMediaType mt;
		m_pFile->m_mts.GetNextAssoc(pos, id, mt);

		CStringW name, lang;
		name.Format(L"Output %d", id);

		if(AP4_Track* track = movie->GetTrack(id))
		{
			AP4_String TrackName = track->GetTrackName();
			AP4_String TrackLanguage = track->GetTrackLanguage();
			
			if(!TrackName.empty())
			{
				name.Format(L"%s", CStringW(TrackName.c_str()));
				SetProperty(L"NAME", CStringW(TrackName.c_str()));
			}

			if(!TrackLanguage.empty())
			{
				if(TrackLanguage != "und") name += L" (" + CStringW(TrackLanguage.c_str()) + L")";
				SetProperty(L"LANG", CStringW(TrackLanguage.c_str()));
			}
		}

		CArray<CMediaType> mts;
		mts.Add(mt);

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(new CBaseSplitterOutputPin(mts, name, this, this, &hr));

		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));

		m_trackpos[id] = trackpos();
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CMP4SplitterFilter::DemuxInit()
{
	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while(pos)
	{
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		pPair->m_value.index = 0;
		pPair->m_value.ts = 0;

		AP4_Track* track = movie->GetTrack(pPair->m_key);
		
		AP4_Sample sample;
		if(AP4_SUCCEEDED(track->GetSample(0, sample)))
			pPair->m_value.ts = sample.GetCts();
	}

	return true;
}

void CMP4SplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	AP4_TimeStamp ts = (AP4_TimeStamp)(rt / 10000);

	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while(pos)
	{
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		AP4_Track* track = movie->GetTrack(pPair->m_key);

		if(AP4_FAILED(track->GetSampleIndexForTimeStampMs(ts, pPair->m_value.index)))
			pPair->m_value.index = 0;

		AP4_Sample sample;
		if(AP4_SUCCEEDED(track->GetSample(pPair->m_value.index, sample)))
			pPair->m_value.ts = sample.GetCts();

		// FIXME: slow search & stss->m_Entries is private

		if(AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss")))
		{
			if(stss->m_Entries.ItemCount() > 0)
			{
				AP4_Cardinal i = -1;
				while(++i < stss->m_Entries.ItemCount() && stss->m_Entries[i]-1 <= pPair->m_value.index);
				if(i > 0) i--;
				pPair->m_value.index = stss->m_Entries[i]-1;
			}
		}
	}
}

bool CMP4SplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	while(SUCCEEDED(hr) && !CheckRequest(NULL))
	{
		CAtlMap<DWORD, trackpos>::CPair* pPairNext = NULL;
		REFERENCE_TIME rtNext = 0;

		POSITION pos = m_trackpos.GetStartPosition();
		while(pos)
		{
			CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

			AP4_Track* track = movie->GetTrack(pPair->m_key);

			REFERENCE_TIME rt = 10000000i64 * pPair->m_value.ts / track->GetMediaTimeScale();

			if(pPair->m_value.index < track->GetSampleCount() && (!pPairNext || rt < rtNext))
			{
				pPairNext = pPair;
				rtNext = rt;
			}
		}

		if(!pPairNext) break;

		AP4_Track* track = movie->GetTrack(pPairNext->m_key);

		AP4_Sample sample;
		AP4_DataBuffer data;

		if(AP4_SUCCEEDED(track->ReadSample(pPairNext->m_value.index, sample, data)))
		{
			CAutoPtr<Packet> p(new Packet());

			p->TrackNumber = (DWORD)track->GetId();
			p->rtStart = 10000000i64 * sample.GetCts() / track->GetMediaTimeScale();
			p->rtStop = p->rtStart + 1;
			p->bSyncPoint = TRUE;

			if(track->GetType() == AP4_Track::TYPE_TEXT)
			{
				const AP4_Byte* ptr = data.GetData();
				AP4_Size avail = data.GetDataSize();

				if(avail > 2)
				{
					AP4_UI16 size = (ptr[0] << 8) | ptr[1];

					if(size <= avail-2)
					{
						p->pData.SetSize(size);
						memcpy(p->pData.GetData(), &ptr[2], size);

						AP4_Sample sample;
						if(AP4_SUCCEEDED(track->GetSample(pPairNext->m_value.index+1, sample)))
							p->rtStop = 10000000i64 * sample.GetCts() / track->GetMediaTimeScale();
					}
				}
			}
			else
			{
				p->pData.SetSize(data.GetDataSize());
				memcpy(p->pData.GetData(), data.GetData(), data.GetDataSize());
			}

			// FIXME: slow search & stss->m_Entries is private

			if(AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss")))
			{
				if(stss->m_Entries.ItemCount() > 0)
				{
					p->bSyncPoint = FALSE;

					AP4_Cardinal i = -1;
					while(++i < stss->m_Entries.ItemCount())
						if(stss->m_Entries[i]-1 == pPairNext->m_value.index)
							p->bSyncPoint = TRUE;
				}
			}

			hr = DeliverPacket(p);
		}

		{
			AP4_Sample sample;
			if(AP4_SUCCEEDED(track->GetSample(++pPairNext->m_value.index, sample)))
				pPairNext->m_value.ts = sample.GetCts();
		}

	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CMP4SplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);

	if(!m_pFile) return E_UNEXPECTED;

	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while(pos)
	{
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		AP4_Track* track = movie->GetTrack(pPair->m_key);

		if(track->GetType() != AP4_Track::TYPE_VIDEO)
			continue;

		if(AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss")))
		{
			nKFs = stss->m_Entries.ItemCount();
			return S_OK;
		}
	}

	return E_FAIL;
}

STDMETHODIMP CMP4SplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if(*pFormat != TIME_FORMAT_MEDIA_TIME) return E_INVALIDARG;

	if(!m_pFile) return E_UNEXPECTED;

	AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie();

	POSITION pos = m_trackpos.GetStartPosition();
	while(pos)
	{
		CAtlMap<DWORD, trackpos>::CPair* pPair = m_trackpos.GetNext(pos);

		AP4_Track* track = movie->GetTrack(pPair->m_key);

		if(track->GetType() != AP4_Track::TYPE_VIDEO)
			continue;

		if(AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(track->GetTrakAtom()->FindChild("mdia/minf/stbl/stss")))
		{
			nKFs = 0;

			for(AP4_Cardinal i = 0; i < stss->m_Entries.ItemCount(); i++)
			{
				AP4_Sample sample;
				if(AP4_SUCCEEDED(track->GetSample(stss->m_Entries[i]-1, sample)))
					pKFs[nKFs++] = 10000000i64 * sample.GetCts() / track->GetMediaTimeScale();
			}

			return S_OK;
		}
	}

	return E_FAIL;
}

//
// CMP4SourceFilter
//

CMP4SourceFilter::CMP4SourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMP4SplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
