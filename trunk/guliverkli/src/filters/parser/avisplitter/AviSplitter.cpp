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

#include "StdAfx.h"
#include <mmreg.h>
#include "..\..\..\DSUtil\DSUtil.h"
#include "AviSplitter.h"

/////////

#include <Aviriff.h> // conflicts with vfw.h..., so we have to define CAviFile here and not in the .h file

class CAviFile
{
	CComPtr<IAsyncReader> m_pReader;
	UINT64 m_pos, m_len;

	HRESULT Init();
	HRESULT Parse(DWORD parentid, UINT64 end);

public:
	CAviFile(IAsyncReader* pReader, HRESULT& hr);

	UINT64 GetPos() {return m_pos;}
	UINT64 GetLength() {return m_len;}
	void Seek(UINT64 pos)
	{
		m_pos = pos;
	}
	HRESULT Read(void* pData, LONG len);
	template<typename T> HRESULT Read(T& var, int offset = 0);

	AVIMAINHEADER m_avih;
	struct ODMLExtendedAVIHeader {DWORD dwTotalFrames;} m_dmlh;
//	VideoPropHeader m_vprp;
	struct strm_t
	{
		AVISTREAMHEADER strh;
		CArray<BYTE> strf;
		CStringA strn;
		CAutoPtr<AVISUPERINDEX> indx;
//		struct chunk {UINT64 size, filepos; bool fKeyFrame;};
		struct chunk {union {UINT64 size:63, fKeyFrame:1;}; UINT64 filepos;}; // making it a union saves a couple of megs
		CArray<chunk> cs;
		UINT64 totalsize;
		REFERENCE_TIME GetRefTime(DWORD frame, UINT64 size);
		int GetFrame(REFERENCE_TIME rt);
		int GetKeyFrame(REFERENCE_TIME rt);
		DWORD GetChunkSize(DWORD size);
		bool IsRawSubtitleStream();
	};
	CAutoPtrArray<strm_t> m_strms;
	CMap<DWORD, DWORD&, CStringA, CStringA&> m_info;
	CAutoPtr<AVIOLDINDEX> m_idx1;

	CList<UINT64> m_movis;
    
	REFERENCE_TIME GetTotalTime();
	HRESULT BuildIndex();
	void EmptyIndex();
	bool IsInterleaved();
};

#define TRACKNUM(fcc) (10*((fcc&0xff)-0x30) + (((fcc>>8)&0xff)-0x30))
#define TRACKTYPE(fcc) ((WORD)((((DWORD)fcc>>24)&0xff)|((fcc>>8)&0xff00)))

///////

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_Avi},
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
      sizeof(sudPinTypesIn)/sizeof(sudPinTypesIn[0]), // Number of types
      sudPinTypesIn         // Pin information
    },
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{ &__uuidof(CAviSplitterFilter)			// Filter CLSID
    , L"Avi Splitter"						// String name
    , MERIT_NORMAL+1						// Filter merit
    , sizeof(sudpPins)/sizeof(sudpPins[0])	// Number of pins
	, sudpPins},							// Pin information
	{ &__uuidof(CAviSourceFilter)			// Filter CLSID
    , L"Avi Source"							// String name
    , MERIT_NORMAL+1						// Filter merit
    , 0										// Number of pins
	, NULL},								// Pin information
};

CFactoryTemplate g_Templates[] =
{
	{L"Avi Splitter", &__uuidof(CAviSplitterFilter), CAviSplitterFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"Avi Source", &__uuidof(CAviSourceFilter), CAviSourceFilter::CreateInstance, NULL, &sudFilter[1]},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(MEDIASUBTYPE_Avi);
	CString asyncfilter = CStringFromGUID(CLSID_AsyncReader);
	CString srcfilter = CStringFromGUID(__uuidof(CAviSourceFilter));

	SetRegKeyValue(_T("Media Type\\") + null, subtype, _T("0"), _T("0,4,,52494646,8,4,41564920")); // RIFFxxxxAVI_
	SetRegKeyValue(_T("Media Type\\") + null, subtype, _T("Source Filter"), srcfilter);

	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("0"), _T("0,4,,52494646,8,4,41564920")); // RIFFxxxxAVI_
	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("Source Filter"), asyncfilter);

	DeleteRegKey(_T("Media Type\\Extensions"), _T(".avi"));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(MEDIASUBTYPE_Avi);

	DeleteRegKey(_T("Media Type\\") + null, subtype);
	DeleteRegKey(_T("Media Type\\") + majortype, subtype);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI CAviSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CAviSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CUnknown* WINAPI CAviSourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CAviSourceFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// CAviSplitterFilter
//

CAviSplitterFilter::CAviSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CAviSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_timeformat(TIME_FORMAT_MEDIA_TIME)
{
}

CAviSplitterFilter::~CAviSplitterFilter()
{
}

STDMETHODIMP CAviSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(IPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAviSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	if(m_pOutputs.GetCount() > 0) return VFW_E_ALREADY_CONNECTED;

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pPinMap.RemoveAll();

	m_tFrame.Free();
	m_tSize.Free();
	m_tFilePos.Free();

	m_pFile.Attach(new CAviFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->GetTotalTime();

	for(DWORD i = 0; i < m_pFile->m_strms.GetCount(); i++)
	{
		CAviFile::strm_t* s = m_pFile->m_strms[i];

		CMediaType mt;
		CArray<CMediaType> mts;
		
		CStringW name, label;

		if(s->strh.fccType == FCC('vids'))
		{
			label = L"Video";

			ASSERT(s->strf.GetSize() >= sizeof(BITMAPINFOHEADER));

			BITMAPINFOHEADER* pbmi = &((BITMAPINFO*)s->strf.GetData())->bmiHeader;

			mt.majortype = MEDIATYPE_Video;
			mt.subtype = FOURCCMap(pbmi->biCompression);
			mt.formattype = FORMAT_VideoInfo;
			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + s->strf.GetSize() - sizeof(BITMAPINFOHEADER));
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(&pvih->bmiHeader, s->strf.GetData(), s->strf.GetSize());
			if(s->strh.dwRate > 0) 
				pvih->AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 * s->strh.dwScale / s->strh.dwRate);
			switch(pbmi->biCompression)
			{
			case BI_RGB: case BI_BITFIELDS: mt.subtype = 
						pbmi->biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
						pbmi->biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
						pbmi->biBitCount == 32 ? MEDIASUBTYPE_RGB32 :
						MEDIASUBTYPE_NULL;
						break;
//			case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
//			case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
			}

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
				? s->strh.dwSuggestedBufferSize*3/2
				: (pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4));
			mts.Add(mt);
		}
		else if(s->strh.fccType == FCC('auds'))
		{
			label = L"Audio";

			ASSERT(s->strf.GetSize() >= sizeof(WAVEFORMATEX)
				|| s->strf.GetSize() == sizeof(PCMWAVEFORMAT));

            WAVEFORMATEX* pwfe = (WAVEFORMATEX*)s->strf.GetData();

			mt.majortype = MEDIATYPE_Audio;
			mt.subtype = FOURCCMap(pwfe->wFormatTag);
			mt.formattype = FORMAT_WaveFormatEx;
			mt.SetFormat(s->strf.GetData(), max(s->strf.GetSize(), sizeof(WAVEFORMATEX)));
			pwfe = (WAVEFORMATEX*)mt.Format();
			if(s->strf.GetSize() == sizeof(PCMWAVEFORMAT)) pwfe->cbSize = 0;
			if(pwfe->wFormatTag == WAVE_FORMAT_PCM) pwfe->nBlockAlign = pwfe->nChannels*pwfe->wBitsPerSample>>3;
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
				? s->strh.dwSuggestedBufferSize*3/2
				: (pwfe->nChannels*pwfe->nSamplesPerSec*32>>3));
			mts.Add(mt);
		}
		else if(s->strh.fccType == FCC('mids'))
		{
			label = L"Midi";

			mt.majortype = MEDIATYPE_Midi;
			mt.subtype = MEDIASUBTYPE_NULL;
			mt.formattype = FORMAT_None;
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
				? s->strh.dwSuggestedBufferSize*3/2
				: (1024*1024));
			mts.Add(mt);
		}
		else if(s->strh.fccType == FCC('txts'))
		{
			label = L"Text";

			mt.majortype = MEDIATYPE_Text;
			mt.subtype = MEDIASUBTYPE_NULL;
			mt.formattype = FORMAT_None;
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
				? s->strh.dwSuggestedBufferSize*3/2
				: (1024*1024));
			mts.Add(mt);
		}
		else if(s->strh.fccType == FCC('iavs'))
		{
			label = L"Interleaved";

			ASSERT(s->strh.fccHandler == FCC('dvsd'));

			mt.majortype = MEDIATYPE_Interleaved;
			mt.subtype = FOURCCMap(s->strh.fccHandler);
			mt.formattype = FORMAT_DvInfo;
			mt.SetFormat(s->strf.GetData(), max(s->strf.GetSize(), sizeof(DVINFO)));
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
				? s->strh.dwSuggestedBufferSize*3/2
				: (1024*1024));
			mts.Add(mt);
		}

		if(mts.IsEmpty())
		{
			TRACE(_T("CAviSourceFilter: Unsupported stream (%d)\n"), i);
			continue;
		}

		name.Format(L"%s %d", !s->strn.IsEmpty() ? CStringW(CString(s->strn)) : label, i);

		HRESULT hr;

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(new CAviSplitterOutputPin(mts, name, this, this, &hr));
		if(pPinOut)
		{
			m_pPinMap[i] = pPinOut;
			m_pOutputs.AddTail(pPinOut);
		}
	}

	{
		CStringA str;
		DWORD id;

		id = FCC('INAM');
		if(m_pFile->m_info.Lookup(id, str)) SetMediaContentStr(CStringW(CString(str)), Title);
		id = FCC('IART');
		if(m_pFile->m_info.Lookup(id, str)) SetMediaContentStr(CStringW(CString(str)), AuthorName);
		id = FCC('ICOP');
		if(m_pFile->m_info.Lookup(id, str)) SetMediaContentStr(CStringW(CString(str)), Copyright);
		id = FCC('ISBJ');
		if(m_pFile->m_info.Lookup(id, str)) SetMediaContentStr(CStringW(CString(str)), Description);
//		id = FCC();
//		if(m_pFile->m_info.Lookup(id, str)) SetMediaContentStr(CStringW(CString(str)), Rating);
	}

	m_tFrame.Attach(new DWORD[m_pFile->m_avih.dwStreams]);
	m_tSize.Attach(new UINT64[m_pFile->m_avih.dwStreams]);
	m_tFilePos.Attach(new UINT64[m_pFile->m_avih.dwStreams]);

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CAviSplitterFilter::InitDeliverLoop()
{
	if(!m_pFile) return(false);

	// reindex if needed

	bool fReIndex = false;

	for(int i = 0; i < m_pFile->m_avih.dwStreams && !fReIndex; i++)
	{
		if(m_pFile->m_strms[i]->cs.GetCount() == 0) 
			fReIndex = true;
	}

	if(fReIndex)
	{
		m_fAbort = false;
		m_nOpenProgress = 0;

		m_rtDuration = 0;

		memset((UINT64*)m_tSize, 0, sizeof(UINT64)*m_pFile->m_avih.dwStreams);
		m_pFile->Seek(0);
        ReIndex(m_pFile->GetLength());

		if(m_fAbort) m_pFile->EmptyIndex();

		m_fAbort = false;
		m_nOpenProgress = 100;
	}

	return(true);
}

HRESULT CAviSplitterFilter::ReIndex(UINT64 end)
{
	HRESULT hr = S_OK;

	while(S_OK == hr && m_pFile->GetPos() < end && SUCCEEDED(hr) && !m_fAbort)
	{
		UINT64 pos = m_pFile->GetPos();

		DWORD id = 0, size;
		if(S_OK != m_pFile->Read(id) || id == 0)
			return E_FAIL;

		if(id == FCC('RIFF') || id == FCC('LIST'))
		{
			if(S_OK != m_pFile->Read(size) || S_OK != m_pFile->Read(id))
				return E_FAIL;

			size += (size&1) + 8;

			if(id == FCC('AVI ') || id == FCC('AVIX') || id == FCC('movi') || id == FCC('rec '))
				hr = ReIndex(pos + size);
		}
		else
		{
			if(S_OK != m_pFile->Read(size))
				return E_FAIL;

			DWORD TrackNumber = TRACKNUM(id);

			if(TrackNumber < m_pFile->m_strms.GetCount())
			{
				CAviFile::strm_t* s = m_pFile->m_strms[TrackNumber];

				WORD type = TRACKTYPE(id);

				if(type == 'db' || type == 'dc' || /*type == 'pc' ||*/ type == 'wb' || type == '__')
				{
					CAviFile::strm_t::chunk c;
					c.filepos = pos;
					c.size = m_tSize[TrackNumber];
					c.fKeyFrame = size > 0; // TODO: find a better way...
					s->cs.Add(c);

					m_tSize[TrackNumber] += s->GetChunkSize(size);

					REFERENCE_TIME rt = s->GetRefTime(s->cs.GetCount()-1, m_tSize[TrackNumber]);
					m_rtDuration = max(rt, m_rtDuration);
				}
			}

			size += (size&1) + 8;
		}

		m_pFile->Seek(pos + size);

		m_nOpenProgress = m_pFile->GetPos()*100/m_pFile->GetLength();

		DWORD cmd;
		if(CheckRequest(&cmd))
		{
			if(cmd == CMD_EXIT) m_fAbort = true;
			else Reply(S_OK);
		}
	}

	return hr;
}

void CAviSplitterFilter::SeekDeliverLoop(REFERENCE_TIME rt)
{
	memset((DWORD*)m_tFrame, 0, sizeof(DWORD)*m_pFile->m_avih.dwStreams);
	memset((UINT64*)m_tSize, 0, sizeof(UINT64)*m_pFile->m_avih.dwStreams);
	memset((UINT64*)m_tFilePos, 0, sizeof(UINT64)*m_pFile->m_avih.dwStreams);
	m_pFile->Seek(0);

	DbgLog((LOG_TRACE, 0, _T("Seek: %I64d"), rt/10000));

	if(rt > 0)
	{
		UINT64 minfilepos = ~0;

		for(int j = 0; j < m_pFile->m_strms.GetCount(); j++)
		{
			CAviFile::strm_t* s = m_pFile->m_strms[j];

			int f = s->GetKeyFrame(rt);
			m_tFilePos[j] = f >= 0 ? s->cs[f].filepos : 0;

			if(!s->IsRawSubtitleStream())
				minfilepos = min(minfilepos, m_tFilePos[j]);
		}

		m_pFile->Seek(minfilepos);

		for(int j = 0; j < m_pFile->m_strms.GetCount(); j++)
		{
			CAviFile::strm_t* s = m_pFile->m_strms[j];

			for(int i = 0; i < s->cs.GetCount(); i++)
			{
				CAviFile::strm_t::chunk& c = s->cs[i];
				if(c.filepos >= minfilepos)
				{
					m_tSize[j] = s->cs[i].size;
					m_tFrame[j] = i;
					break;
				}
			}
		}

		DbgLog((LOG_TRACE, 0, _T("filepos: %I64d"), minfilepos));
	}
}

void CAviSplitterFilter::DoDeliverLoop()
{
	UINT64 pos = m_pFile->GetPos();

	HRESULT hr = S_OK;

	for(int i = 0; i < m_pFile->m_strms.GetCount() && SUCCEEDED(hr) && !CheckRequest(NULL); i++)
	{
		CAviFile::strm_t* s = m_pFile->m_strms[i];
		if(!s->IsRawSubtitleStream()) continue;

		for(int j = 0; j < s->cs.GetCount() && SUCCEEDED(hr) && !CheckRequest(NULL); j++)
		{
			CAviFile::strm_t::chunk& c = s->cs[j];

			m_pFile->Seek(c.filepos);
            
			DWORD id = -1, size;
			if(S_OK != m_pFile->Read(id) || TRACKNUM(id) != i
			|| S_OK != m_pFile->Read(size))
				break;

			CAutoPtr<Packet> p(new Packet());
			p->TrackNumber = i;
			p->bSyncPoint = TRUE;
			p->rtStart = m_rtStart;
			p->rtStop = m_rtStart+1;
			p->pData.SetSize(size);
			if(S_OK != (hr = m_pFile->Read(p->pData.GetData(), p->pData.GetSize()))) break;

			hr = DeliverPacket(p);
		}
	}

	m_pFile->Seek(pos);

	DoDeliverLoop(m_pFile->GetLength());
}

HRESULT CAviSplitterFilter::DoDeliverLoop(UINT64 end)
{
	HRESULT hr = S_OK;

	while(m_pFile->GetPos() < end && SUCCEEDED(hr) && !CheckRequest(NULL))
	{
		UINT64 pos = m_pFile->GetPos();

		DWORD id = 0, size;
		if(S_OK != m_pFile->Read(id) || id == 0)
			return E_FAIL;

		if(id == FCC('RIFF') || id == FCC('LIST'))
		{
			if(S_OK != m_pFile->Read(size) || S_OK != m_pFile->Read(id))
				return E_FAIL;

			size += (size&1) + 8;

			if(id == FCC('AVI ') || id == FCC('AVIX') || id == FCC('movi') || id == FCC('rec '))
				hr = DoDeliverLoop(pos + size);
		}
		else
		{
			if(S_OK != m_pFile->Read(size))
				return E_FAIL;

			DWORD TrackNumber = TRACKNUM(id);

			if(TrackNumber < m_pFile->m_strms.GetCount())
			{
				CAviFile::strm_t* s = m_pFile->m_strms[TrackNumber];

				if(!s->IsRawSubtitleStream())
				{
					WORD type = TRACKTYPE(id);

					if(type == 'db' || type == 'dc' /*|| type == 'pc'*/ || type == 'wb' || type == '__') // TODO: check these agains the fcc in the index
					{
						CAutoPtr<Packet> p(new Packet());

						p->TrackNumber = TrackNumber;

						ASSERT(s->cs.GetCount() == 0 || m_tFrame[TrackNumber] < s->cs.GetCount());

						if(m_tFrame[TrackNumber] < s->cs.GetCount() && s->cs[m_tFrame[TrackNumber]].filepos != pos)
						{
							for(int i = m_tFrame[TrackNumber]; i < s->cs.GetCount(); i++)
							{
								if(s->cs[i].filepos == pos)
								{
									DbgLog((LOG_TRACE, 0, _T("WARNING: Missing %d frames (track=%d)"), i - m_tFrame[TrackNumber], TrackNumber));
									m_tFrame[TrackNumber] = i;
									m_tSize[TrackNumber] = s->cs[i].size;
									break;
								}
							}
						}

						p->bSyncPoint = 0 <= m_tFrame[TrackNumber] && m_tFrame[TrackNumber] < s->cs.GetCount() 
								? (bool)s->cs[m_tFrame[TrackNumber]].fKeyFrame 
								: TRUE;

						p->rtStart = s->GetRefTime(m_tFrame[TrackNumber], m_tSize[TrackNumber]);
						m_tFrame[TrackNumber]++; 
						m_tSize[TrackNumber] += s->GetChunkSize(size);
						p->rtStop = s->GetRefTime(m_tFrame[TrackNumber], m_tSize[TrackNumber]);

						if(pos >= m_tFilePos[TrackNumber])
						{
							p->pData.SetSize(size);
							if(S_OK != (hr = m_pFile->Read(p->pData.GetData(), p->pData.GetSize()))) break;
/*
							DbgLog((LOG_TRACE, 0, _T("%d (%d): %I64d - %I64d, %I64d - %I64d (size = %d)"), 
								p->TrackNumber, (int)p->bSyncPoint,
								(p->rtStart)/10000, (p->rtStop)/10000, 
								(p->rtStart-m_rtStart)/10000, (p->rtStop-m_rtStart)/10000,
								size));
*/
							hr = DeliverPacket(p);
						}
					}
				}
			}

			size += (size&1) + 8;
		}

		m_pFile->Seek(pos + size);
	}

	return hr;
}

// IMediaSeeking

STDMETHODIMP CAviSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	*pDuration = m_rtDuration;

	return S_OK;
}

//

STDMETHODIMP CAviSplitterFilter::IsFormatSupported(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	HRESULT hr = __super::IsFormatSupported(pFormat);
	if(S_OK == hr) return hr;
	return *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CAviSplitterFilter::GetTimeFormat(GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = m_timeformat;
	return S_OK;
}

STDMETHODIMP CAviSplitterFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	return *pFormat == m_timeformat ? S_OK : S_FALSE;
}

STDMETHODIMP CAviSplitterFilter::SetTimeFormat(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	if(S_OK != IsFormatSupported(pFormat)) return E_FAIL;
	m_timeformat = *pFormat;
	return S_OK;
}

STDMETHODIMP CAviSplitterFilter::GetStopPosition(LONGLONG* pStop)
{
	CheckPointer(pStop, E_POINTER);
	if(FAILED(__super::GetStopPosition(pStop))) return E_FAIL;
	if(m_timeformat == TIME_FORMAT_MEDIA_TIME) return S_OK;
	LONGLONG rt = *pStop;
	if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, rt, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
	return S_OK;
}

STDMETHODIMP CAviSplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	CheckPointer(pTarget, E_POINTER);

	GUID& SourceFormat = pSourceFormat ? *pSourceFormat : m_timeformat;
	GUID& TargetFormat = pTargetFormat ? *pTargetFormat : m_timeformat;
	
	if(TargetFormat == SourceFormat)
	{
		*pTarget = Source; 
		return S_OK;
	}
	else if(TargetFormat == TIME_FORMAT_FRAME && SourceFormat == TIME_FORMAT_MEDIA_TIME)
	{
		for(int i = 0; i < (int)m_pFile->m_strms.GetCount(); i++)
		{
			CAviFile::strm_t* s = m_pFile->m_strms[i];
			if(s->strh.fccType == FCC('vids'))
			{
				*pTarget = s->GetFrame(Source);
				return S_OK;
			}
		}
	}
	else if(TargetFormat == TIME_FORMAT_MEDIA_TIME && SourceFormat == TIME_FORMAT_FRAME)
	{
		for(int i = 0; i < (int)m_pFile->m_strms.GetCount(); i++)
		{
			CAviFile::strm_t* s = m_pFile->m_strms[i];
			if(s->strh.fccType == FCC('vids'))
			{
				if(Source < 0 || Source >= s->cs.GetCount()) return E_FAIL;
				CAviFile::strm_t::chunk& c = s->cs[(int)Source];
				*pTarget = s->GetRefTime((DWORD)Source, c.size);
				return S_OK;
			}
		}
	}
	
	return E_FAIL;
}

STDMETHODIMP CAviSplitterFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(m_timeformat != TIME_FORMAT_FRAME)
		return __super::SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);


	if(!pCurrent && !pStop
	|| (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
		return S_OK;

	REFERENCE_TIME 
		rtCurrent = m_rtCurrent,
		rtStop = m_rtStop;

	if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
	&& FAILED(ConvertTimeFormat(&rtCurrent, &TIME_FORMAT_FRAME, rtCurrent, &TIME_FORMAT_MEDIA_TIME))) 
		return E_FAIL;
	if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
	&& FAILED(ConvertTimeFormat(&rtStop, &TIME_FORMAT_FRAME, rtStop, &TIME_FORMAT_MEDIA_TIME)))
		return E_FAIL;

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

	if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
	&& pCurrent)
		if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_MEDIA_TIME, rtCurrent, &TIME_FORMAT_FRAME))) return E_FAIL;
	if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
	&& pStop)
		if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_MEDIA_TIME, rtStop, &TIME_FORMAT_FRAME))) return E_FAIL;

	return __super::SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CAviSplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	HRESULT hr;
	if(FAILED(hr = __super::GetPositions(pCurrent, pStop)) || m_timeformat != TIME_FORMAT_FRAME)
		return hr;

	if(pCurrent)
		if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_FRAME, *pCurrent, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
	if(pStop)
		if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, *pStop, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;

	return S_OK;
}


// IPropertyBag

STDMETHODIMP CAviSplitterFilter::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog)
{
	CheckPointer(pszPropName, E_POINTER);
	CheckPointer(pVar, E_POINTER);
	if(pVar->vt != VT_EMPTY) return E_FAIL;
	if(!m_pFile) return E_UNEXPECTED;

	CStringW name(pszPropName);
	if(name.Find(L"INFO/") == 0 && name.GetLength() == 9)
	{
		DWORD fcc = mmioFOURCC(name[5], name[6], name[7], name[8]);
		CStringA str;
		if(m_pFile->m_info.Lookup(fcc, str))
		{
			*pVar = CComVariant(str);
			return S_OK;
		}
	}

	return E_FAIL;
}

STDMETHODIMP CAviSplitterFilter::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
	return E_NOTIMPL;
}

// IKeyFrameInfo

STDMETHODIMP CAviSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if(!m_pFile) return E_UNEXPECTED;

	HRESULT hr = S_OK;

	nKFs = 0;

	for(int i = 0; i < (int)m_pFile->m_strms.GetCount(); i++)
	{
		CAviFile::strm_t* s = m_pFile->m_strms[i];
		if(s->strh.fccType != FCC('vids')) continue;

		for(int j = 0; j < s->cs.GetCount(); j++)
		{
			CAviFile::strm_t::chunk& c = s->cs[j];
			if(c.fKeyFrame) nKFs++;
		}

		if(nKFs == s->cs.GetCount())
			hr = S_FALSE;

		break;
	}

	return hr;
}

STDMETHODIMP CAviSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if(!m_pFile) return E_UNEXPECTED;
	if(*pFormat != TIME_FORMAT_MEDIA_TIME && *pFormat != TIME_FORMAT_FRAME) return E_INVALIDARG;

	UINT nKFsTmp = 0;

	for(int i = 0; i < (int)m_pFile->m_strms.GetCount(); i++)
	{
		CAviFile::strm_t* s = m_pFile->m_strms[i];
		if(s->strh.fccType != FCC('vids')) continue;

		bool fConvertToRefTime = !!(*pFormat == TIME_FORMAT_MEDIA_TIME);

		for(int j = 0; j < s->cs.GetCount() && nKFsTmp < nKFs; j++)
		{
			if(s->cs[j].fKeyFrame)
				pKFs[nKFsTmp++] = fConvertToRefTime ? s->GetRefTime(j, s->cs[j].size) : j;
		}

		break;
	}

	nKFs = nKFsTmp;

	return S_OK;
}

//
// CAviSourceFilter
//

CAviSourceFilter::CAviSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CAviSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// CAviSplitterOutputPin
//

CAviSplitterOutputPin::CAviSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

HRESULT CAviSplitterOutputPin::CheckConnect(IPin* pPin)
{
	int iPosition = 0;
	CMediaType mt;
	while(S_OK == GetMediaType(iPosition++, &mt))
	{
		if(mt.majortype == MEDIATYPE_Video 
		&& (mt.subtype == FOURCCMap(FCC('IV32'))
		|| mt.subtype == FOURCCMap(FCC('IV31'))
		|| mt.subtype == FOURCCMap(FCC('IF09'))))
		{
			CLSID clsid = GetCLSID(GetFilterFromPin(pPin));
			if(clsid == CLSID_VideoMixingRenderer || clsid == CLSID_OverlayMixer)
				return E_FAIL;
		}

		mt.InitMediaType();
	}

	return __super::CheckConnect(pPin);
}

//
// CAviFile
//

CAviFile::CAviFile(IAsyncReader* pReader, HRESULT& hr)
	: m_pReader(pReader)
	, m_pos(0), m_len(0)
{
	LONGLONG total = 0, available;
	pReader->Length(&total, &available);
	m_len = total;

	hr = Init();
}

HRESULT CAviFile::Read(void* pData, LONG len)
{
	HRESULT hr = m_pReader->SyncRead(m_pos, len, (BYTE*)pData);
	m_pos += len;
	return hr;
}

template<typename T> 
HRESULT CAviFile::Read(T& var, int offset)
{
	memset(&var, 0, sizeof(var));
	HRESULT hr = Read((BYTE*)&var + offset, sizeof(var) - offset);
	return hr;
}

HRESULT CAviFile::Init()
{
	if(!m_pReader) return E_UNEXPECTED;

	Seek(0);
	DWORD dw[3];
	if(S_OK != Read(dw) || dw[0] != FCC('RIFF') || (dw[2] != FCC('AVI ') && dw[2] != FCC('AVIX')))
		return E_FAIL;

	Seek(0);
	HRESULT hr = Parse(0, GetLength());
	if(m_movis.GetCount() == 0) // FAILED(hr) is allowed as long as there was a movi chunk found
		return E_FAIL;

	if(m_avih.dwStreams == 0 && m_strms.GetCount() > 0)
		m_avih.dwStreams = m_strms.GetCount();

	if(m_avih.dwStreams != m_strms.GetCount())
		return E_FAIL;

	for(int i = 0; i < m_avih.dwStreams; i++)
	{
		strm_t* s = m_strms[i];
		if(s->strh.fccType != FCC('auds')) continue;
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)s->strf.GetData();
		if(wfe->wFormatTag == 0x55 && wfe->nBlockAlign == 1152 
		&& s->strh.dwScale == 1 && s->strh.dwRate != wfe->nSamplesPerSec)
		{
			// correcting encoder bugs...
			s->strh.dwScale = 1152;
			s->strh.dwRate = wfe->nSamplesPerSec;
		}
	}

	if(FAILED(BuildIndex()))
		EmptyIndex();

	if(!IsInterleaved())
		return E_FAIL;

	return S_OK;
}

HRESULT CAviFile::Parse(DWORD parentid, UINT64 end)
{
	HRESULT hr = S_OK;

	CAutoPtr<strm_t> strm;

	while(S_OK == hr && GetPos() < end)
	{
		UINT64 pos = GetPos();

		DWORD id = 0, size;
		if(S_OK != Read(id) || id == 0)
			return E_FAIL;

		if(id == FCC('RIFF') || id == FCC('LIST'))
		{
			if(S_OK != Read(size) || S_OK != Read(id))
				return E_FAIL;

			size += (size&1) + 8;

			TRACE(_T("CAviFile::Parse(..): LIST '%c%c%c%c'\n"), 
				TCHAR((id>>0)&0xff), 
				TCHAR((id>>8)&0xff), 
				TCHAR((id>>16)&0xff),
				TCHAR((id>>24)&0xff));

			if(id == FCC('movi'))
			{
				m_movis.AddTail(pos);
			}
			else
			{
				hr = Parse(id, pos + size);
			}
		}
		else
		{
			if(S_OK != Read(size))
				return E_FAIL;

			TRACE(_T("CAviFile::Parse(..): '%c%c%c%c'\n"), 
				TCHAR((id>>0)&0xff), 
				TCHAR((id>>8)&0xff), 
				TCHAR((id>>16)&0xff),
				TCHAR((id>>24)&0xff));

			if(parentid == FCC('INFO') && size > 0)
			{
				switch(id)
				{
				case FCC('IARL'): // Archival Location. Indicates where the subject of the file is archived.
				case FCC('IART'): // Artist. Lists the artist of the original subject of the file; for example, “Michaelangelo.”
				case FCC('ICMS'): // Commissioned. Lists the name of the person or organization that commissioned the subject of the file; for example, “Pope Julian II.”
				case FCC('ICMT'): // Comments. Provides general comments about the file or the subject of the file. If the comment is several sentences long, end each sentence with a period. Do not include new-line characters.
				case FCC('ICOP'): // Copyright. Records the copyright information for the file; for example, “Copyright Encyclopedia International 1991.” If there are multiple copyrights, separate them by a semicolon followed by a space.
				case FCC('ICRD'): // Creation date. Specifies the date the subject of the file was created. List dates in year-month-day format, padding one-digit months and days with a zero on the left; for example, “1553-05-03” for May 3, 1553.
				case FCC('ICRP'): // Cropped. Describes whether an image has been cropped and, if so, how it was cropped; for example, “lower-right corner.”
				case FCC('IDIM'): // Dimensions. Specifies the size of the original subject of the file; for example, “8.5 in h, 11 in w.”
				case FCC('IDPI'): // Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as “300.”
				case FCC('IENG'): // Engineer. Stores the name of the engineer who worked on the file. If there are multiple engineers, separate the names by a semicolon and a blank; for example, “Smith, John; Adams, Joe.”
				case FCC('IGNR'): // Genre. Describes the original work, such as “landscape,” “portrait,” “still life,” etc.
				case FCC('IKEY'): // Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon and a blank; for example, “Seattle; aerial view; scenery.”
				case FCC('ILGT'): // Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
				case FCC('IMED'): // Medium. Describes the original subject of the file, such as “computer image,” “drawing,” “lithograph,” and so on.
				case FCC('INAM'): // Name. Stores the title of the subject of the file, such as “Seattle From Above.”
				case FCC('IPLT'): // Palette Setting. Specifies the number of colors requested when digitizing an image, such as “256.”
				case FCC('IPRD'): // Product. Specifies the name of the title the file was originally intended for, such as “Encyclopedia of Pacific Northwest Geography.”
				case FCC('ISBJ'): // Subject. Describes the contents of the file, such as “Aerial view of Seattle.”
				case FCC('ISFT'): // Software. Identifies the name of the software package used to create the file, such as “Microsoft WaveEdit.”
				case FCC('ISHP'): // Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
				case FCC('ISRC'): // Source. Identifies the name of the person or organization who supplied the original subject of the file; for example, “Trey Research.”
				case FCC('ISRF'): // Source Form. Identifies the original form of the material that was digitized, such as “slide,” “paper,” “map,” and so on. This is not necessarily the same as IMED.
				case FCC('ITCH'): // Technician. Identifies the technician who digitized the subject file; for example, “Smith, John.”
					{
						CStringA str;
						if(S_OK != Read(str.GetBufferSetLength(size), size)) return E_FAIL;
						m_info[id] = str;
						break;
					}
				}
			}

			switch(id)
			{
			case FCC('avih'):
				m_avih.fcc = FCC('avih');
				m_avih.cb = size;
				if(S_OK != Read(m_avih, 8)) return E_FAIL;
				break;
			case FCC('strh'):
				if(!strm) strm.Attach(new strm_t());
				strm->strh.fcc = FCC('strh');
				strm->strh.cb = size;
				if(S_OK != Read(strm->strh, 8)) return E_FAIL;
				break;
			case FCC('strn'):
				if(S_OK != Read(strm->strn.GetBufferSetLength(size), size)) return E_FAIL;
				break;
			case FCC('strf'):
				if(!strm) strm.Attach(new strm_t());
				strm->strf.SetSize(size);
				if(S_OK != Read(strm->strf.GetData(), size)) return E_FAIL;
				break;
			case FCC('indx'):
				if(!strm) strm.Attach(new strm_t());
				ASSERT(strm->indx == NULL);
				strm->indx.Attach((AVISUPERINDEX*)new BYTE[size + 8]);
				strm->indx->fcc = FCC('indx');
				strm->indx->cb = size;
				if(S_OK != Read((BYTE*)(AVISUPERINDEX*)strm->indx + 8, size)) return E_FAIL;
				ASSERT(strm->indx->wLongsPerEntry == 4 && strm->indx->bIndexType == AVI_INDEX_OF_INDEXES);
				break;
			case FCC('dmlh'):
				if(S_OK != Read(m_dmlh)) return E_FAIL;
				break;
			case FCC('vprp'):
//				if(S_OK != Read(m_vprp)) return E_FAIL;
				break;
			case FCC('idx1'):
				ASSERT(m_idx1 == NULL);
				m_idx1.Attach((AVIOLDINDEX*)new BYTE[size + 8]);
				m_idx1->fcc = FCC('idx1');
				m_idx1->cb = size;
				if(S_OK != Read((BYTE*)(AVIOLDINDEX*)m_idx1 + 8, size)) return E_FAIL;
				break;
			}

			size += (size&1) + 8;
		}

		Seek(pos + size);
	}

	if(strm) m_strms.Add(strm);

	return hr;
}

REFERENCE_TIME CAviFile::GetTotalTime()
{
	REFERENCE_TIME t = 0/*10i64*m_avih.dwMicroSecPerFrame*m_avih.dwTotalFrames*/;

	for(int i = 0; i < m_avih.dwStreams; i++)
	{
		strm_t* s = m_strms[i];
		REFERENCE_TIME t2 = s->GetRefTime(s->cs.GetCount(), s->totalsize);
		t = max(t, t2);
	}

	if(t == 0) t = 10i64*m_avih.dwMicroSecPerFrame*m_avih.dwTotalFrames;

	return(t);
}

HRESULT CAviFile::BuildIndex()
{
	EmptyIndex();

	int nSuperIndexes = 0;

	for(int i = 0; i < m_avih.dwStreams; i++)
	{
		strm_t* s = m_strms[i];
		if(s->indx && s->indx->nEntriesInUse > 0) nSuperIndexes++;
	}

	if(nSuperIndexes == m_avih.dwStreams)
	{
		for(int i = 0; i < m_avih.dwStreams; i++)
		{
			strm_t* s = m_strms[i];

			AVISUPERINDEX* idx = (AVISUPERINDEX*)s->indx;

			DWORD nEntriesInUse = 0;

			for(int j = 0; j < idx->nEntriesInUse; j++)
			{
				Seek(idx->aIndex[j].qwOffset);

				AVISTDINDEX stdidx;
				if(S_OK != Read(&stdidx, FIELD_OFFSET(AVISTDINDEX, aIndex)))
				{
					EmptyIndex();
					return E_FAIL;
				}

				nEntriesInUse += stdidx.nEntriesInUse;
			} 

			s->cs.SetSize(nEntriesInUse);

			DWORD frame = 0;
			UINT64 size = 0;

			for(int j = 0; j < idx->nEntriesInUse; j++)
			{
				Seek(idx->aIndex[j].qwOffset);

				CAutoPtr<AVISTDINDEX> p((AVISTDINDEX*)new BYTE[idx->aIndex[j].dwSize]);
				if(!p || S_OK != Read((AVISTDINDEX*)p, idx->aIndex[j].dwSize)) 
				{
					EmptyIndex();
					return E_FAIL;
				}

				for(int k = 0, l = 0; k < p->nEntriesInUse; k++)
				{
					s->cs[frame].size = size;
					s->cs[frame].filepos = p->qwBaseOffset + p->aIndex[k].dwOffset - 8;
					s->cs[frame].fKeyFrame = !(p->aIndex[k].dwSize&AVISTDINDEX_DELTAFRAME);

					frame++;
					size += s->GetChunkSize(p->aIndex[k].dwSize&AVISTDINDEX_SIZEMASK);
				}
			}

			s->totalsize = size;
		}
	}
	else if(AVIOLDINDEX* idx = m_idx1)
	{
		int len = idx->cb/sizeof(idx->aIndex[0]);

		for(int i = 0; i < m_avih.dwStreams; i++)
		{
			strm_t* s = m_strms[i];

			int nFrames = 0;

			for(int j = 0; j < len; j++)
			{
				if(TRACKNUM(idx->aIndex[j].dwChunkId) == i)
					nFrames++;
			}

			s->cs.SetSize(nFrames);

			DWORD frame = 0;
			UINT64 size = 0;

			UINT64 offset = m_movis.GetHead() + 8;

			for(int j = 0, k = 0; j < len; j++)
			{
				DWORD TrackNumber = TRACKNUM(idx->aIndex[j].dwChunkId);

				if(TrackNumber == i)
				{
					if(j == 0 && idx->aIndex[j].dwOffset > offset)
					{
						DWORD id;
						Seek(offset + idx->aIndex[j].dwOffset);
						Read(id);
						if(id != idx->aIndex[j].dwChunkId)
						{
							TRACE(_T("WARNING: CAviFile::Init() detected absolute chunk addressing in \'idx1\'"));
							offset = 0;
						}
					}

					s->cs[frame].size = size;
					s->cs[frame].filepos = offset + idx->aIndex[j].dwOffset;
					s->cs[frame].fKeyFrame = !!(idx->aIndex[j].dwFlags&AVIIF_KEYFRAME) 
						|| s->strh.fccType == FCC('auds'); // FIXME: some audio index is without any kf flag

					frame++;
					size += s->GetChunkSize(idx->aIndex[j].dwSize);
				}
			}

			s->totalsize = size;
		}
	}

	m_idx1.Free();
	for(int i = 0; i < m_avih.dwStreams; i++)
		m_strms[i]->indx.Free();

	return S_OK;
}

void CAviFile::EmptyIndex()
{
	for(int i = 0; i < m_avih.dwStreams; i++)
	{
		strm_t* s = m_strms[i];
		s->cs.RemoveAll();
		s->totalsize = 0;
	}
}

bool CAviFile::IsInterleaved()
{
	if(m_strms.GetCount() < 2)
		return(true);

	if(m_avih.dwFlags&AVIF_ISINTERLEAVED)
		return(true);

	for(int i = 0; i < m_avih.dwStreams; i++)
	{
		if(m_strms[i]->cs.GetCount() < 500) continue; // we can buffer up to 500 samples...

		for(int j = 0; j < m_avih.dwStreams; j++)
		{
			if(i == j) continue;

			if(m_strms[j]->strh.fccType == FCC('txts') && m_strms[j]->cs.GetCount() == 1) continue;

			CArray<strm_t::chunk>& cs1 = m_strms[i]->cs;
			CArray<strm_t::chunk>& cs2 = m_strms[j]->cs;

			if(cs2.GetCount() == 0) continue;

			if(cs1[cs1.GetCount()-1].filepos < cs2[0].filepos)
				return(false);
		}
	}
/*
	if(AVIOLDINDEX* idx = m_idx1)
	{
		int len = idx->cb/sizeof(idx->aIndex[0]);

		CList<DWORD> ids;

		for(int i = 1; i < len; i++)
		{
			if(idx->aIndex[i-1].dwChunkId != idx->aIndex[i].dwChunkId)
			{
				if(ids.Find(idx->aIndex[i].dwChunkId))
					return(true);

				ids.AddTail(idx->aIndex[i-1].dwChunkId);
			}
		}

		return(false);
	}
	else
	{
		// TODO
	}
*/
//	return(false);
	return(true);
}

REFERENCE_TIME CAviFile::strm_t::GetRefTime(DWORD frame, UINT64 size)
{
	REFERENCE_TIME rt = -1;

	if(strh.fccType == FCC('auds'))
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)strf.GetData();

		rt = ((10000000i64 * size + wfe->nBlockAlign/2) / wfe->nBlockAlign * strh.dwScale + strh.dwRate/2) / strh.dwRate;
	}
	else
	{
		rt = (10000000i64 * frame * strh.dwScale + strh.dwRate/2) / strh.dwRate;
	}

	return(rt);
}

int CAviFile::strm_t::GetFrame(REFERENCE_TIME rt)
{
	int frame = -1;

	if(strh.fccType == FCC('auds'))
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)strf.GetData();

		INT64 size = ((rt * strh.dwRate + strh.dwScale/2) / strh.dwScale * wfe->nBlockAlign + 10000000i64/2) / 10000000i64;

		for(frame = 0; frame < cs.GetCount(); frame++)
		{
			if(cs[frame].size > size)
			{
				frame--;
				break;
			}
		}
	}
	else
	{
		frame = (int)(((rt * strh.dwRate + strh.dwScale/2) / strh.dwScale + 10000000i64/2) / 10000000i64);
	}

	if(frame >= cs.GetCount())
		frame = -1;

	return(frame);
}

int CAviFile::strm_t::GetKeyFrame(REFERENCE_TIME rt)
{
	int i = GetFrame(rt);
	for(; i > 0; i--) {if(cs[i].fKeyFrame) break;}
	return(i);
}

DWORD CAviFile::strm_t::GetChunkSize(DWORD size)
{
	if(strh.fccType == FCC('auds'))
	{
		WORD nBlockAlign = ((WAVEFORMATEX*)strf.GetData())->nBlockAlign;
		size = (size + (nBlockAlign-1)) / nBlockAlign * nBlockAlign; // round up for nando's vbr hack
	}

	return(size);
}

bool CAviFile::strm_t::IsRawSubtitleStream()
{
	return(strh.fccType == FCC('txts') && cs.GetCount() == 1);
}

