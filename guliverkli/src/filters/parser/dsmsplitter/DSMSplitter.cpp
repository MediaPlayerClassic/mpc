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
#include "DSMSplitter.h"
#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_DirectShowMedia},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 0, NULL}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CDSMSplitterFilter), L"DSM Splitter", MERIT_NORMAL, countof(sudpPins), sudpPins},
	{&__uuidof(CDSMSourceFilter), L"DSM Source", MERIT_NORMAL, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDSMSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CDSMSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	CString str;
	str.Format(_T("0,%d,,%%0%dI64x"), DSMSW_SIZE, DSMSW_SIZE*2);
	str.Format(CString(str), DSMSW);

	RegisterSourceFilter(
		CLSID_AsyncReader, 
		MEDIASUBTYPE_DirectShowMedia, 
		str, NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_DirectShowMedia);

	return AMovieDllRegisterServer2(TRUE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, dwReason, 0); // "DllMain" of the dshow baseclasses;
}

#endif

//
// CDSMSplitterFilter
//

CDSMSplitterFilter::CDSMSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CDSMSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

CDSMSplitterFilter::~CDSMSplitterFilter()
{
}

static int compare_id(const void* id1, const void* id2) {return (int)*(BYTE*)id1 - (int)*(BYTE*)id2;}

HRESULT CDSMSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(new CDSMSplitterFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->m_rtDuration;

	CArray<BYTE> ids;

	POSITION pos = m_pFile->m_mts.GetStartPosition();
	while(pos)
	{
		BYTE id;
		CMediaType mt;
		m_pFile->m_mts.GetNextAssoc(pos, id, mt);
		ids.Add(id);
	}

	qsort(ids.GetData(), ids.GetCount(), sizeof(BYTE), compare_id);

	for(int i = 0; i < ids.GetCount(); i++)
	{
		BYTE id = ids[i];
		CMediaType& mt = m_pFile->m_mts[id];

		CStringW name;
		name.Format(L"Output %02d", id);

		CArray<CMediaType> mts;
		mts.Add(mt);

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(new CBaseSplitterOutputPin(mts, name, this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));
	}
/*
	SetMediaContentStr(CStringW(m_pFile->), Title);
	SetMediaContentStr(CStringW(m_pFile->), AuthorName);
	SetMediaContentStr(CStringW(m_pFile->), Rating);
	SetMediaContentStr(CStringW(m_pFile->), Copyright);
	SetMediaContentStr(CStringW(m_pFile->), Description);
*/
	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDSMSplitterFilter::InitDeliverLoop()
{
	return(true);
}

void CDSMSplitterFilter::SeekDeliverLoop(REFERENCE_TIME rt)
{
	REFERENCE_TIME 
		rtFirst = m_pFile->m_rtFirst, 
		rtDuration = m_pFile->m_rtDuration;

	if(!rtDuration || rt <= 0)
	{
		m_pFile->Seek(0);
	}
	else
	{
		m_pFile->Seek(m_pFile->FindSyncPoint(rt));
/*
		CMap<DWORD,DWORD,__int64,__int64&> id2fp;

		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos)
		{
			CBaseSplitterOutputPin* pPin = m_pOutputs.GetNext(pos);
			BYTE id = (BYTE)GetOutputTrackNum(pPin);



			__int64 seekpos = 0;

			m_pFile->Seek(seekpos);

			while(m_pFile->GetPos() < m_pFile->GetLength())
			{
				dsmp_t type;
				UINT64 len;
				if(!m_pFile->Sync(type, len))
					continue;

				__int64 pos = m_pFile->GetPos();

				if(type == DSMP_SAMPLE)
				{
					CAutoPtr<Packet> p(new Packet());
					if(m_pFile->Read(len, p, false) && p->rtStart != Packet::INVALID_TIME)
					{
						REFERENCE_TIME dt = (p->rtStart -= rtFirst) - rt;

						if(dt > 0)
						{
							if(maxpos == pos) rtpos = maxpos;
							maxpos = pos;
							maxrt = p->rtStart;
						}
						else
						{
							minpos = pos;
							minrt = p->rtStart;
						}
						
						if(dt > 0 || dt < -1000000)
							break;
					}
				}

				m_pFile->Seek(pos + len);
			}

		}




		__int64 minpos = 0, maxpos = m_pFile->GetLength(), rtpos = -1;
		REFERENCE_TIME minrt = 0, maxrt = rtDuration;

		while(minpos < maxpos && rtpos < 0)
		{
			m_pFile->Seek((minpos + maxpos) / 2);

			while(m_pFile->GetPos() < maxpos)
			{
				dsmp_t type;
				UINT64 len;
				if(!m_pFile->Sync(type, len))
					continue;

				__int64 pos = m_pFile->GetPos();

				if(type == DSMP_SAMPLE)
				{
					CAutoPtr<Packet> p(new Packet());
					if(m_pFile->Read(len, p, false) && p->rtStart != Packet::INVALID_TIME)
					{
						REFERENCE_TIME dt = (p->rtStart -= rtFirst) - rt;

						if(dt > 0)
						{
							if(maxpos == pos) rtpos = maxpos;
							maxpos = pos;
							maxrt = p->rtStart;
						}
						else
						{
							minpos = pos;
							minrt = p->rtStart;
						}
						
						if(dt > 0 || dt < -1000000)
							break;
					}
				}

				m_pFile->Seek(pos + len);
			}
		}

		rtpos = rtpos;
*/
/*
		__int64 minpos = 0, maxpos = m_pFile->GetLength(), syncpointpos = 0;
		REFERENCE_TIME minrt = 0, maxrt = rtDuration;
		bool fCloseEnough = false;
		int i = 0;

		while(!fCloseEnough && minpos < maxpos)
		{
			__int64 seekpos = minpos + 1.0 * (maxpos - minpos) * (rt - minrt) / (maxrt - minrt);
			m_pFile->Seek(seekpos);

			while(m_pFile->GetPos() < maxpos)
			{
				dsmp_t type;
				UINT64 len;

				if(!m_pFile->Sync(type, len))
				{
//					if(m_pFile->GetPos() >= maxpos) maxpos = seekpos;
					continue;
				}

				__int64 pos = m_pFile->GetPos();

				if(type == DSMP_SAMPLE)
				{
					CAutoPtr<Packet> p(new Packet());
					if(m_pFile->Read(len, p, false) && p->rtStart != Packet::INVALID_TIME)
					{
						REFERENCE_TIME dt = (p->rtStart -= rtFirst) - rt;

						if(dt > 0)
						{
							maxpos = pos;
							maxrt = p->rtStart;
							break;
						}
						else if(p->bSyncPoint)
						{
							syncpointpos = max(seekpos, pos);
						}
					}
				}

				m_pFile->Seek(pos + len);
			}

//			__int64 minpos = 0, maxpos = m_pFile->GetLength();
//			REFERENCE_TIME minrt = 0, maxrt = rtDuration;
		}
*/
	}
}

bool CDSMSplitterFilter::DoDeliverLoop()
{
	HRESULT hr = S_OK;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetPos() < m_pFile->GetLength())
	{
		dsmp_t type;
		UINT64 len;

		if(!m_pFile->Sync(type, len))
			continue;

		__int64 pos = m_pFile->GetPos();

		if(type == DSMP_SAMPLE)
		{
			CAutoPtr<Packet> p(new Packet());
			if(m_pFile->Read(len, p))
			{
				p->rtStart -= m_pFile->m_rtFirst;
				p->rtStop -= m_pFile->m_rtFirst;
				hr = DeliverPacket(p);
			}
		}

		m_pFile->Seek(pos + len);
	}

	return(true);
}
/*
// IKeyFrameInfo

STDMETHODIMP CDSMSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if(!m_pFile) return E_UNEXPECTED;
	nKFs = m_pFile->m_irs.GetCount();
	return S_OK;
}

STDMETHODIMP CDSMSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if(!m_pFile) return E_UNEXPECTED;
	if(*pFormat != TIME_FORMAT_MEDIA_TIME) return E_INVALIDARG;

	UINT nKFsTmp = 0;
	POSITION pos = m_pFile->m_irs.GetHeadPosition();
	for(int i = 0; pos && nKFsTmp < nKFs; i++)
		pKFs[nKFsTmp++] = 10000i64*m_pFile->m_irs.GetNext(pos)->tStart;
	nKFs = nKFsTmp;

	return S_OK;
}

// IChapterInfo

STDMETHODIMP_(UINT) CDSMSplitterFilter::GetChapterCount(UINT aChapterID)
{
	return aChapterID == CHAPTER_ROOT_ID ? m_pChapters.GetCount() : 0;
}

STDMETHODIMP_(UINT) CDSMSplitterFilter::GetChapterId(UINT aParentChapterId, UINT aIndex)
{
	POSITION pos = m_pChapters.FindIndex(aIndex-1);
	if(aParentChapterId != CHAPTER_ROOT_ID || !pos)
		return CHAPTER_BAD_ID;
	return aIndex;
}

STDMETHODIMP_(BOOL) CDSMSplitterFilter::GetChapterInfo(UINT aChapterID, struct ChapterElement* pToFill)
{
	REFERENCE_TIME rtDur = 0;
	GetDuration(&rtDur);

	CheckPointer(pToFill, E_POINTER);
	POSITION pos = m_pChapters.FindIndex(aChapterID-1);
	if(!pos) return FALSE;
	CChapter* p = m_pChapters.GetNext(pos);
	WORD Size = pToFill->Size;
	if(Size >= sizeof(ChapterElement))
	{
		pToFill->Size = sizeof(ChapterElement);
		pToFill->Type = AtomicChapter;
		pToFill->ChapterId = aChapterID;
		pToFill->rtStart = p->m_rt;
		pToFill->rtStop = pos ? m_pChapters.GetNext(pos)->m_rt : rtDur;
	}
	return TRUE;
}

STDMETHODIMP_(BSTR) CDSMSplitterFilter::GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2])
{
	POSITION pos = m_pChapters.FindIndex(aChapterID-1);
	if(!pos) return NULL;
	return m_pChapters.GetAt(pos)->m_name.AllocSysString();
}
*/

//
// CDSMSourceFilter
//

CDSMSourceFilter::CDSMSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CDSMSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}
