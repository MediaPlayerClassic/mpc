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
#include "MatroskaSplitter.h"

#include <initguid.h>
#include "..\..\..\..\include\matroska\matroska.h"
#include "..\..\..\..\include\ogg\OggDS.h"
#include "..\..\..\..\include\moreuuids.h"

using namespace MatroskaReader;

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
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
	{&__uuidof(CMatroskaSplitterFilter), L"Matroska Splitter", MERIT_NORMAL, sizeof(sudpPins)/sizeof(sudpPins[0]), sudpPins},
	{&__uuidof(CMatroskaSourceFilter), L"Matroska Source", MERIT_NORMAL, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
	{L"Matroska Splitter", &__uuidof(CMatroskaSplitterFilter), CMatroskaSplitterFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"Matroska Source", &__uuidof(CMatroskaSourceFilter), CMatroskaSourceFilter::CreateInstance, NULL, &sudFilter[1]},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		CLSID_AsyncReader, 
		MEDIASUBTYPE_Matroska, 
		_T("0,4,,1A45DFA3"), 
		_T(".mkv"), _T(".mka"), _T(".mks"), NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Matroska);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI CMatroskaSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMatroskaSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CUnknown* WINAPI CMatroskaSourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMatroskaSourceFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// CMatroskaSplitterFilter
//

CMatroskaSplitterFilter::CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CMatroskaSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

CMatroskaSplitterFilter::~CMatroskaSplitterFilter()
{
}

HRESULT CMatroskaSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	if(m_pOutputs.GetCount() > 0) return VFW_E_ALREADY_CONNECTED;

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pPinMap.RemoveAll();
	m_pTrackEntryMap.RemoveAll(); 

	m_pFile.Attach(new CMatroskaFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = 0;

	POSITION pos = m_pFile->m_segment.Tracks.GetHeadPosition();
	while(pos)
	{
		Track* pT = m_pFile->m_segment.Tracks.GetNext(pos);

		POSITION pos2 = pT->TrackEntries.GetHeadPosition();
		while(pos2)
		{
			TrackEntry* pTE = pT->TrackEntries.GetNext(pos2);

			CStringA CodecID = pTE->CodecID.ToString();

			CStringW Name;
			Name.Format(L"Output %I64d", (UINT64)pTE->TrackNumber);

			CMediaType mt;
			CArray<CMediaType> mts;

			if(pTE->TrackType == TrackEntry::TypeVideo)
			{
				Name.Format(L"Video %I64d", (UINT64)pTE->TrackNumber);

				if(CodecID == "V_MS/VFW/FOURCC")
				{
					BITMAPINFOHEADER* pbmi = (BITMAPINFOHEADER*)(BYTE*)pTE->CodecPrivate;

					mt.majortype = MEDIATYPE_Video;
					mt.subtype = FOURCCMap(pbmi->biCompression);
					mt.formattype = FORMAT_VideoInfo;
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.GetCount() - sizeof(BITMAPINFOHEADER));
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(&pvih->bmiHeader, pbmi, pTE->CodecPrivate.GetCount());
					if(pTE->v.FramePerSec > 0) 
						pvih->AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / pTE->v.FramePerSec);
					else if(pTE->DefaultDuration > 0)
						pvih->AvgTimePerFrame = (REFERENCE_TIME)pTE->DefaultDuration / 100;
					switch(pbmi->biCompression)
					{
					case BI_RGB: case BI_BITFIELDS: mt.subtype = 
								pbmi->biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
								pbmi->biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
								pbmi->biBitCount == 32 ? MEDIASUBTYPE_ARGB32 :
								MEDIASUBTYPE_NULL;
								break;
//					case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
//					case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
					}
					mt.SetSampleSize(pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4);
					mts.Add(mt);
				}
				else if(CodecID == "V_UNCOMPRESSED")
				{
				}
				else if(CodecID.Find("V_MPEG4/") == 0) // TODO: find out which V_MPEG4/*/* ids can be mapped to 'mp4v'
				{
					mt.majortype = MEDIATYPE_Video;
					mt.subtype = FOURCCMap('v4pm');
					mt.formattype = FORMAT_VideoInfo;
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
					memset(pvih, 0, mt.FormatLength());
					pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
					pvih->bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					pvih->bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					pvih->bmiHeader.biCompression = 'v4pm';
					if(pTE->v.FramePerSec > 0) 
						pvih->AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / pTE->v.FramePerSec);
					else if(pTE->DefaultDuration > 0)
						pvih->AvgTimePerFrame = (REFERENCE_TIME)pTE->DefaultDuration / 100;
					mt.SetSampleSize(pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4);
					mts.Add(mt);

					// TODO: add (-1,0) dummy frame to timeoverride when it is /asp (that is having b-frames almost certainly)
				}
				else if(CodecID.Find("V_REAL/RV") == 0)
				{
					mt.majortype = MEDIATYPE_Video;
					mt.subtype = FOURCCMap('00VR' + ((CodecID[9]-0x30)<<16));
					mt.formattype = FORMAT_VideoInfo;
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pTE->CodecPrivate.GetCount());
					memset(mt.Format(), 0, mt.FormatLength());
					memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());
					pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
					pvih->bmiHeader.biWidth = (LONG)pTE->v.PixelWidth;
					pvih->bmiHeader.biHeight = (LONG)pTE->v.PixelHeight;
					pvih->bmiHeader.biCompression = mt.subtype.Data1;
					if(pTE->v.FramePerSec > 0) 
						pvih->AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / pTE->v.FramePerSec);
					else if(pTE->DefaultDuration > 0)
						pvih->AvgTimePerFrame = (REFERENCE_TIME)pTE->DefaultDuration / 100;
					mt.SetSampleSize(pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4);
					mts.Add(mt);
				}
/*
				else if(CodecID == "V_DSHOW/MPEG1VIDEO") // V_MPEG1
				{
					mt.majortype = MEDIATYPE_Video;
					mt.subtype = MEDIASUBTYPE_MPEG1Payload;
					mt.formattype = FORMAT_MPEGVideo;
					MPEG1VIDEOINFO* pm1vi = (MPEG1VIDEOINFO*)mt.AllocFormatBuffer(pTE->CodecPrivate.GetSize());
					memcpy(pm1vi, pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetSize());
					mt.SetSampleSize(pm1vi->hdr.bmiHeader.biWidth*pm1vi->hdr.bmiHeader.biHeight*4);
					mts.Add(mt);
				}
*/
				if(pTE->v.DisplayWidth != 0 && pTE->v.DisplayHeight != 0)
				{
					for(int i = 0; i < mts.GetCount(); i++)
					{
						if(mts[i].formattype != FORMAT_VideoInfo)
							continue;

						DWORD vih1 = FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);
						DWORD vih2 = FIELD_OFFSET(VIDEOINFOHEADER2, bmiHeader);
						DWORD bmi = mts[i].FormatLength() - FIELD_OFFSET(VIDEOINFOHEADER, bmiHeader);

						mt.formattype = FORMAT_VideoInfo2;
						mt.AllocFormatBuffer(vih2 + bmi);
						memcpy(mt.Format(), mts[i].Format(), vih1);
						memset(mt.Format() + vih1, 0, vih2 - vih1);
						memcpy(mt.Format() + vih2, mts[i].Format() + vih1, bmi);
						((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioX = (DWORD)pTE->v.DisplayWidth;
						((VIDEOINFOHEADER2*)mt.Format())->dwPictAspectRatioY = (DWORD)pTE->v.DisplayHeight;

						mts.InsertAt(i++, mt);
					}
				}
			}
			else if(pTE->TrackType == TrackEntry::TypeAudio)
			{
				Name.Format(L"Audio %I64d", (UINT64)pTE->TrackNumber);

				mt.majortype = MEDIATYPE_Audio;
				mt.formattype = FORMAT_WaveFormatEx;
				WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(pwfe, 0, mt.FormatLength());
				pwfe->nChannels = (WORD)pTE->a.Channels;
				pwfe->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
				pwfe->wBitsPerSample = (WORD)pTE->a.BitDepth;
				pwfe->nBlockAlign = (WORD)((pwfe->nChannels * pwfe->wBitsPerSample) / 8);
				pwfe->nAvgBytesPerSec = pwfe->nSamplesPerSec * pwfe->nBlockAlign;
				mt.SetSampleSize(pwfe->nChannels*pwfe->nSamplesPerSec*32>>3);

				if(CodecID == "A_VORBIS")
				{
					BYTE* p = (BYTE*)pTE->CodecPrivate;
					CArray<long> sizes;
					for(BYTE n = *p++; n > 0; n--)
					{
						long size = 0;
						do {size += *p;} while(*p++ == 0xff);
						sizes.Add(size);
					}

					long totalsize = 0;
					for(int i = 0; i < sizes.GetCount(); i++)
						totalsize += sizes[i];

					sizes.Add(pTE->CodecPrivate.GetSize() - (p - (BYTE*)pTE->CodecPrivate) - totalsize);
					totalsize += sizes[sizes.GetCount()-1];

					if(sizes.GetCount() == 3)
					{
						mt.subtype = MEDIASUBTYPE_Vorbis2;
						mt.formattype = FORMAT_VorbisFormat2;
						VORBISFORMAT2* pvf2 = (VORBISFORMAT2*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT2) + totalsize);
						memset(pvf2, 0, mt.FormatLength());
						pvf2->Channels = (WORD)pTE->a.Channels;
						pvf2->SamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
						pvf2->BitsPerSample = (DWORD)pTE->a.BitDepth;
						BYTE* p2 = mt.Format() + sizeof(VORBISFORMAT2);
						for(int i = 0; i < sizes.GetCount(); p += sizes[i], p2 += sizes[i], i++)
							memcpy(p2, p, pvf2->HeaderSize[i] = sizes[i]);

						mt.SetSampleSize(pvf2->Channels*pvf2->SamplesPerSec*32>>3);
						mt.SetSampleSize(max(mt.GetSampleSize(), (UINT_PTR)pTE->CodecPrivate.GetSize()));
						mts.Add(mt);
					}

					mt.subtype = MEDIASUBTYPE_Vorbis;
					mt.formattype = FORMAT_VorbisFormat;
					VORBISFORMAT* pvf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
					memset(pvf, 0, mt.FormatLength());
					pvf->nChannels = (WORD)pTE->a.Channels;
					pvf->nSamplesPerSec = (DWORD)pTE->a.SamplingFrequency;
					pvf->nMinBitsPerSec = pvf->nMaxBitsPerSec = pvf->nAvgBitsPerSec = -1;
					mt.SetSampleSize(pvf->nChannels*pvf->nSamplesPerSec*32>>3);
					mt.SetSampleSize(max(mt.GetSampleSize(), (UINT_PTR)pTE->CodecPrivate.GetSize()));
					mts.Add(mt);
				}
				else if(CodecID == "A_MPEG/L3")
				{
					mt.subtype = FOURCCMap(pwfe->wFormatTag = 0x55);
					mts.Add(mt);
				}
				else if(CodecID == "A_AC3")
				{
					mt.subtype = FOURCCMap(pwfe->wFormatTag = 0x2000);
					mts.Add(mt);
				}
				else if(CodecID == "A_DTS")
				{
					mt.subtype = FOURCCMap(pwfe->wFormatTag = 0x2001);
					mts.Add(mt);
				}
				else if(CodecID == "A_MS/ACM")
				{
					pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(pTE->CodecPrivate.GetCount());
					memcpy(pwfe, (WAVEFORMATEX*)(BYTE*)pTE->CodecPrivate, pTE->CodecPrivate.GetCount());
					mt.subtype = FOURCCMap(pwfe->wFormatTag);
					mt.SetSampleSize(pwfe->nChannels*pwfe->nSamplesPerSec*32>>3);
					mts.Add(mt);
				}
				else if(CodecID == "A_PCM/INT/LIT")
				{
					mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_PCM);
					mts.Add(mt);
				}
				else if(CodecID == "A_PCM/FLOAT/IEEE")
				{
					mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_IEEE_FLOAT);
					mts.Add(mt);
				}
				else if(CodecID.Find("A_AAC/") == 0)
				{
					mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_AAC);
					BYTE* pExtra = mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX)+5) + sizeof(WAVEFORMATEX);
					(pwfe = (WAVEFORMATEX*)mt.pbFormat)->cbSize = 2;

					char profile, srate_idx;

					if(CodecID.Find("/MAIN") > 0) profile = 0;
					else if(CodecID.Find("/SBR") > 0) profile = -1;
					else if(CodecID.Find("/LC") > 0) profile = 1;
					else if(CodecID.Find("/SSR") > 0) profile = 2;
					else if(CodecID.Find("/LTP") > 0) profile = 3;
					else continue;

					if(92017 <= pwfe->nSamplesPerSec) srate_idx = 0;
					else if(75132 <= pwfe->nSamplesPerSec) srate_idx = 1;
					else if(55426 <= pwfe->nSamplesPerSec) srate_idx = 2;
					else if(46009 <= pwfe->nSamplesPerSec) srate_idx = 3;
					else if(37566 <= pwfe->nSamplesPerSec) srate_idx = 4;
					else if(27713 <= pwfe->nSamplesPerSec) srate_idx = 5;
					else if(23004 <= pwfe->nSamplesPerSec) srate_idx = 6;
					else if(18783 <= pwfe->nSamplesPerSec) srate_idx = 7;
					else if(13856 <= pwfe->nSamplesPerSec) srate_idx = 8;
					else if(11502 <= pwfe->nSamplesPerSec) srate_idx = 9;
					else if(9391 <= pwfe->nSamplesPerSec) srate_idx = 10;
					else srate_idx = 11;
   
					pExtra[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);
					pExtra[1] = ((srate_idx & 0x1) << 7) | ((BYTE)pTE->a.Channels << 3);

					mts.Add(mt);

					if(profile < 0)
					{
						pExtra[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);

						((WAVEFORMATEX*)mt.pbFormat)->cbSize = 5;

						pwfe->nSamplesPerSec *= 2;
						pwfe->nAvgBytesPerSec *= 2;

						if(92017 <= pwfe->nSamplesPerSec) srate_idx = 0;
						else if(75132 <= pwfe->nSamplesPerSec) srate_idx = 1;
						else if(55426 <= pwfe->nSamplesPerSec) srate_idx = 2;
						else if(46009 <= pwfe->nSamplesPerSec) srate_idx = 3;
						else if(37566 <= pwfe->nSamplesPerSec) srate_idx = 4;
						else if(27713 <= pwfe->nSamplesPerSec) srate_idx = 5;
						else if(23004 <= pwfe->nSamplesPerSec) srate_idx = 6;
						else if(18783 <= pwfe->nSamplesPerSec) srate_idx = 7;
						else if(13856 <= pwfe->nSamplesPerSec) srate_idx = 8;
						else if(11502 <= pwfe->nSamplesPerSec) srate_idx = 9;
						else if(9391 <= pwfe->nSamplesPerSec) srate_idx = 10;
						else srate_idx = 11;

						pExtra[2] = 0x2B7>>3;
						pExtra[3] = (0x2B7<<5) | 5;
						pExtra[4] = (1<<7) | (srate_idx<<3);

						mts.InsertAt(0, mt);
					}
				}
				else if(CodecID.Find("A_REAL/") == 0 && CodecID.GetLength() >= 11)
				{
					mt.subtype = FOURCCMap((DWORD)CodecID[7]|((DWORD)CodecID[8]<<8)|((DWORD)CodecID[9]<<16)|((DWORD)CodecID[10]<<24));
					BYTE* p = mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + pTE->CodecPrivate.GetCount());
					memcpy(p + sizeof(WAVEFORMATEX), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetCount());

					mts.Add(mt);
				}
			}
			else if(pTE->TrackType == TrackEntry::TypeSubtitle)
			{
				Name.Format(L"Subtitle %I64d", (UINT64)pTE->TrackNumber);

				mt.SetSampleSize(0x10000);

				if(CodecID == "S_TEXT/ASCII")
				{
					mt.majortype = MEDIATYPE_Text;
					mt.subtype = MEDIASUBTYPE_NULL;
					mt.formattype = FORMAT_None;
					mts.Add(mt);
				}
				else if(CodecID == "S_TEXT/UTF8")
				{
					mt.majortype = MEDIATYPE_Subtitle;
					mt.subtype = MEDIASUBTYPE_UTF8;
					mt.formattype = FORMAT_SubtitleInfo;
					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO));
					memset(psi, 0, mt.FormatLength());
					psi->dwOffset = sizeof(SUBTITLEINFO);
					strncpy(psi->IsoLang, pTE->Language, 3);
					mts.Add(mt);
				}
				else if(CodecID == "S_SSA" || CodecID == "S_TEXT/SSA")
				{
					mt.majortype = MEDIATYPE_Subtitle;
					mt.subtype = MEDIASUBTYPE_SSA;
					mt.formattype = FORMAT_SubtitleInfo;
					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + pTE->CodecPrivate.GetSize());
					memset(psi, 0, mt.FormatLength());
					strncpy(psi->IsoLang, pTE->Language, 3);
					memcpy(mt.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetSize());
					mts.Add(mt);
				}
				else if(CodecID == "S_ASS" || CodecID == "S_TEXT/ASS")
				{
					mt.majortype = MEDIATYPE_Subtitle;
					mt.subtype = MEDIASUBTYPE_ASS;
					mt.formattype = FORMAT_SubtitleInfo;
					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + pTE->CodecPrivate.GetSize());
					memset(psi, 0, mt.FormatLength());
					strncpy(psi->IsoLang, pTE->Language, 3);
					memcpy(mt.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetSize());
					mts.Add(mt);
				}
				else if(CodecID == "S_USF" || CodecID == "S_TEXT/USF")
				{
					mt.majortype = MEDIATYPE_Subtitle;
					mt.subtype = MEDIASUBTYPE_USF;
					mt.formattype = FORMAT_SubtitleInfo;
					SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + pTE->CodecPrivate.GetSize());
					memset(psi, 0, mt.FormatLength());
					strncpy(psi->IsoLang, pTE->Language, 3);
					memcpy(mt.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), pTE->CodecPrivate.GetData(), pTE->CodecPrivate.GetSize());
					mts.Add(mt);
				}
				else
				{
					mts.RemoveAll();
				}
			}

			if(mts.IsEmpty())
			{
				TRACE(_T("CMatroskaSourceFilter: Unsupported TrackType %s (%I64d)\n"), CString(CodecID), (UINT64)pTE->TrackType);
				continue;
			}

			HRESULT hr;

			CAutoPtr<CBaseSplitterOutputPin> pPinOut(new CMatroskaSplitterOutputPin(
				(int)pTE->MinCache, pTE->DefaultDuration/100, mts, Name, this, this, &hr));
			if(pPinOut)
			{
				m_pPinMap[(DWORD)pTE->TrackNumber] = pPinOut;
				m_pTrackEntryMap[(DWORD)pTE->TrackNumber] = pTE;
				m_pOutputs.AddTail(pPinOut);
			}
		}
	}

	Info& info = m_pFile->m_segment.SegmentInfo;
	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = (REFERENCE_TIME)(info.Duration*info.TimeCodeScale/100);

#ifdef DEBUG
	for(int i = 1, j = GetChapterCount(CHAPTER_ROOT_ID); i <= j; i++)
	{
		UINT id = GetChapterId(CHAPTER_ROOT_ID, i);
		struct ChapterElement ce;
		BOOL b = GetChapterInfo(id, &ce);
		BSTR bstr = GetChapterStringInfo(id, "eng", "");
		if(bstr) ::SysFreeString(bstr);
	}
#endif

	// TODO

	SetMediaContentStr(info.Title, Title);
/*	SetMediaContentStr(, AuthorName);
	SetMediaContentStr(, Copyright);
	SetMediaContentStr(, Description);
*/

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

void CMatroskaSplitterFilter::SendVorbisHeaderSample()
{
	HRESULT hr;

	POSITION pos = m_pTrackEntryMap.GetStartPosition();
	while(pos)
	{
		DWORD TrackNumber = 0;
		TrackEntry* pTE = NULL;
		m_pTrackEntryMap.GetNextAssoc(pos, TrackNumber, pTE);

		CBaseSplitterOutputPin* pPin = NULL;
		m_pPinMap.Lookup(TrackNumber, pPin);

		if(!(pTE && pPin && pPin->IsConnected()))
			continue;

		if(pTE->CodecID.ToString() == "A_VORBIS" && pPin->CurrentMediaType().subtype == MEDIASUBTYPE_Vorbis
		&& pTE->CodecPrivate.GetSize() > 0)
		{
			BYTE* ptr = (BYTE*)pTE->CodecPrivate;

			CList<long> sizes;
			long last = 0;
			for(BYTE n = *ptr++; n > 0; n--)
			{
				long size = 0;
				do {size += *ptr;} while(*ptr++ == 0xff);
				sizes.AddTail(size);
				last += size;
			}
			sizes.AddTail(pTE->CodecPrivate.GetSize() - (ptr - (BYTE*)pTE->CodecPrivate) - last);

			hr = S_OK;

			POSITION pos = sizes.GetHeadPosition();
			while(pos && SUCCEEDED(hr))
			{
				long len = sizes.GetNext(pos);

				CAutoPtr<Packet> p(new Packet());
				p->TrackNumber = (DWORD)pTE->TrackNumber;
				p->rtStart = 0; p->rtStop = 1;
				p->bSyncPoint = FALSE;
				p->pData.SetSize(len);
				memcpy(p->pData.GetData(), ptr, len);
				ptr += len;

				hr = DeliverPacket(p);
			}

			if(FAILED(hr))
				TRACE(_T("ERROR: Vorbis initialization failed for stream %I64d\n"), TrackNumber);
		}
	}
}

bool CMatroskaSplitterFilter::InitDeliverLoop()
{
	CMatroskaNode Root(m_pFile);
	if(!m_pFile
	|| !(m_pSegment = Root.Child(0x18538067))
	|| !(m_pCluster = m_pSegment->Child(0x1F43B675)))
		return(false);

	Cluster c0;
	c0.ParseTimeCode(m_pCluster);
	m_rtOffset = m_pFile->m_segment.GetRefTime(c0.TimeCode);

	// reindex if needed

	if(m_pFile->m_segment.Cues.GetCount() == 0)
	{
		m_nOpenProgress = 0;
		m_pFile->m_segment.SegmentInfo.Duration.Set(0);

		UINT64 TrackNumber = m_pFile->m_segment.GetMasterTrack();

		CAutoPtr<Cue> pCue(new Cue());

		do
		{
			Cluster c;
			c.ParseTimeCode(m_pCluster);

			m_pFile->m_segment.SegmentInfo.Duration.Set((float)c.TimeCode - m_rtOffset/10000);

			CAutoPtr<CuePoint> pCuePoint(new CuePoint());
			CAutoPtr<CueTrackPosition> pCueTrackPosition(new CueTrackPosition());
			pCuePoint->CueTime.Set(c.TimeCode);
			pCueTrackPosition->CueTrack.Set(TrackNumber);
			pCueTrackPosition->CueClusterPosition.Set(m_pCluster->m_filepos - m_pSegment->m_start);
			pCuePoint->CueTrackPositions.AddTail(pCueTrackPosition);
			pCue->CuePoints.AddTail(pCuePoint);

			m_nOpenProgress = m_pFile->GetPos()*100/m_pFile->GetLength();

			DWORD cmd;
			if(CheckRequest(&cmd))
			{
				if(cmd == CMD_EXIT) m_fAbort = true;
				else Reply(S_OK);
			}
		}
		while(!m_fAbort && m_pCluster->Next(true));

		m_nOpenProgress = 100;

		if(!m_fAbort) m_pFile->m_segment.Cues.AddTail(pCue);

		m_fAbort = false;
	}

	m_pCluster.Free();
	m_pBlock.Free();

	return(true);
}

void CMatroskaSplitterFilter::SeekDeliverLoop(REFERENCE_TIME rt)
{
	m_pCluster = m_pSegment->Child(0x1F43B675);
	m_pBlock.Free();

	rt += m_rtOffset;

	if(rt <= m_rtOffset)
	{
		m_pBlock = m_pCluster->Child(0xA0);
	}
	else
	{
		QWORD lastCueClusterPosition = -1;

		Segment& s = m_pFile->m_segment;

		UINT64 TrackNumber = s.GetMasterTrack();

		POSITION pos1 = s.Cues.GetHeadPosition();
		while(pos1)
		{
			Cue* pCue = s.Cues.GetNext(pos1);

			POSITION pos2 = pCue->CuePoints.GetTailPosition();
			while(pos2)
			{
				CuePoint* pCuePoint = pCue->CuePoints.GetPrev(pos2);

				if(rt < s.GetRefTime(pCuePoint->CueTime))
					continue;

				POSITION pos3 = pCuePoint->CueTrackPositions.GetHeadPosition();
				while(pos3)
				{
					CueTrackPosition* pCueTrackPositions = pCuePoint->CueTrackPositions.GetNext(pos3);

					if(TrackNumber != pCueTrackPositions->CueTrack)
						continue;

					if(lastCueClusterPosition == pCueTrackPositions->CueClusterPosition)
						continue;

					lastCueClusterPosition = pCueTrackPositions->CueClusterPosition;

					m_pCluster->SeekTo(m_pSegment->m_start + pCueTrackPositions->CueClusterPosition);
					m_pCluster->Parse();

					bool fFoundKeyFrame = false;
/*
					if(pCueTrackPositions->CueBlockNumber > 0)
					{
						// TODO: CueBlockNumber only tells the block num of the track and not for all mixed in the cluster
						m_nLastBlock = (int)pCueTrackPositions->CueBlockNumber;
						fFoundKeyFrame = true;
					}
					else
*/
					{
						Cluster c;
						c.ParseTimeCode(m_pCluster);

						if(CAutoPtr<CMatroskaNode> pBlock = m_pCluster->Child(0xA0))
						{
							bool fPassedCueTime = false;

							do
							{
								CBlockNode blocks;
								blocks.Parse(pBlock, false);

								POSITION pos4 = blocks.GetHeadPosition();
								while(!fPassedCueTime && pos4)
								{
									Block* b = blocks.GetNext(pos4);

									if(b->TrackNumber == pCueTrackPositions->CueTrack && rt < s.GetRefTime(c.TimeCode+b->TimeCode)
									|| rt + 5000000i64 < s.GetRefTime(c.TimeCode+b->TimeCode)) // allow 500ms difference between tracks, just in case intreleaving wasn't that much precise
									{
										fPassedCueTime = true;
									}
									else if(b->TrackNumber == pCueTrackPositions->CueTrack && !b->ReferenceBlock.IsValid())
									{
										fFoundKeyFrame = true;
										m_pBlock = pBlock->Copy();
									}
								}
							}
							while(!fPassedCueTime && pBlock->Next(true));
						}
					}

					if(fFoundKeyFrame)
						pos1 = pos2 = pos3 = NULL;
				}
			}
		}

		if(!m_pBlock)
		{
			m_pCluster = m_pSegment->Child(0x1F43B675);
			m_pBlock = m_pCluster->Child(0xA0);
		}
	}
}

void CMatroskaSplitterFilter::DoDeliverLoop()
{
	HRESULT hr = S_OK;
	
	SendVorbisHeaderSample(); // HACK: init vorbis decoder with the headers

	do
	{
		Cluster c;
		c.ParseTimeCode(m_pCluster);

		if(!m_pBlock) m_pBlock = m_pCluster->Child(0xA0);
		if(!m_pBlock) continue;

		do
		{
			CBlockNode b;
			b.Parse(m_pBlock, true);

			while(b.GetCount())
			{
				CAutoPtr<MatroskaPacket> p(new MatroskaPacket());
				p->b = b.RemoveHead();
				p->bSyncPoint = !p->b->ReferenceBlock.IsValid();
				p->TrackNumber = (DWORD)p->b->TrackNumber;
				p->rtStart = m_pFile->m_segment.GetRefTime((REFERENCE_TIME)c.TimeCode + p->b->TimeCode);
				p->rtStop = p->rtStart + (p->b->BlockDuration.IsValid() ? m_pFile->m_segment.GetRefTime(p->b->BlockDuration) : 1);

				p->rtStart -= m_rtOffset;
				p->rtStop -= m_rtOffset;

				hr = DeliverPacket(p);
			}
		}
		while(m_pBlock->Next(true) && SUCCEEDED(hr) && !CheckRequest(NULL));

		m_pBlock.Free();
	}
	while(m_pCluster->Next(true) && SUCCEEDED(hr) && !CheckRequest(NULL));

	m_pCluster.Free();
}

// IMediaSeeking

STDMETHODIMP CMatroskaSplitterFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CheckPointer(m_pFile, VFW_E_NOT_CONNECTED);

	Segment& s = m_pFile->m_segment;
	*pDuration = s.GetRefTime((INT64)s.SegmentInfo.Duration);

	return S_OK;
}

// IChapterInfo

STDMETHODIMP_(UINT) CMatroskaSplitterFilter::GetChapterCount(UINT aChapterID)
{
	CheckPointer(m_pFile, __super::GetChapterCount(aChapterID));
	ChapterAtom* ca = m_pFile->m_segment.FindChapterAtom(aChapterID);
	return ca ? ca->ChapterAtoms.GetCount() : 0;
}

STDMETHODIMP_(UINT) CMatroskaSplitterFilter::GetChapterId(UINT aParentChapterId, UINT aIndex)
{
	CheckPointer(m_pFile, __super::GetChapterId(aParentChapterId, aIndex));
	ChapterAtom* ca = m_pFile->m_segment.FindChapterAtom(aParentChapterId);
	if(!ca) return CHAPTER_BAD_ID;
	POSITION pos = ca->ChapterAtoms.FindIndex(aIndex-1);
	if(!pos) return CHAPTER_BAD_ID;
	return (UINT)ca->ChapterAtoms.GetAt(pos)->ChapterUID;
}

STDMETHODIMP_(UINT) CMatroskaSplitterFilter::GetChapterCurrentId()
{
	CheckPointer(m_pFile, __super::GetChapterCurrentId());
	// TODO
	return CHAPTER_BAD_ID;
}

STDMETHODIMP_(BOOL) CMatroskaSplitterFilter::GetChapterInfo(UINT aChapterID, struct ChapterElement* pStructureToFill)
{
	CheckPointer(pStructureToFill, E_POINTER);
	CheckPointer(m_pFile, __super::GetChapterCurrentId());
	ChapterAtom* ca = m_pFile->m_segment.FindChapterAtom(aChapterID);
	if(!ca) return FALSE;
	pStructureToFill->Size = sizeof(*pStructureToFill);
	pStructureToFill->Type = ca->ChapterAtoms.IsEmpty() ? AtomicChapter : SubChapter; // ?
	pStructureToFill->ChapterId = (UINT)ca->ChapterUID;
	pStructureToFill->rtStart = ca->ChapterTimeStart / 100;
	pStructureToFill->rtStop = ca->ChapterTimeEnd / 100;
	return TRUE;
}

STDMETHODIMP_(BSTR) CMatroskaSplitterFilter::GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2])
{
	CheckPointer(m_pFile, __super::GetChapterStringInfo(aChapterID, PreferredLanguage, CountryCode));
	ChapterAtom* ca = m_pFile->m_segment.FindChapterAtom(aChapterID);
	if(!ca) return NULL;

	if(!PreferredLanguage[0]) strncpy(PreferredLanguage, "eng", 3);
	tolower(PreferredLanguage[0]); tolower(PreferredLanguage[1]); tolower(PreferredLanguage[2]);
	tolower(CountryCode[0]); tolower(CountryCode[1]);

	ChapterDisplay* first = NULL;
	ChapterDisplay* partial = NULL;

	POSITION pos = ca->ChapterDisplays.GetHeadPosition();
	while(pos)
	{
		ChapterDisplay* cd = ca->ChapterDisplays.GetNext(pos);

		if(!first) first = cd;

		if(!strncmp(cd->ChapLanguage, PreferredLanguage, 3))
		{
			if(!strncmp(cd->ChapCountry, CountryCode, 2))
				return cd->ChapString.AllocSysString();

			if(!partial) partial = cd;
		}
	}

	return 
		partial ? partial->ChapString.AllocSysString() : 
		first ? first->ChapString.AllocSysString() : 
		NULL;
}

// IKeyFrameInfo

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if(!m_pFile) return E_UNEXPECTED;

	HRESULT hr = S_OK;

	nKFs = 0;

	POSITION pos = m_pFile->m_segment.Cues.GetHeadPosition();
	while(pos) nKFs += m_pFile->m_segment.Cues.GetNext(pos)->CuePoints.GetCount();

	return hr;
}

STDMETHODIMP CMatroskaSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if(!m_pFile) return E_UNEXPECTED;
	if(*pFormat != TIME_FORMAT_MEDIA_TIME) return E_INVALIDARG;

	UINT nKFsTmp = 0;

	POSITION pos1 = m_pFile->m_segment.Cues.GetHeadPosition();
	while(pos1 && nKFsTmp < nKFs)
	{
		Cue* pCue = m_pFile->m_segment.Cues.GetNext(pos1);

		POSITION pos2 = pCue->CuePoints.GetHeadPosition();
		while(pos2 && nKFsTmp < nKFs)
			pKFs[nKFsTmp++] = m_pFile->m_segment.GetRefTime(pCue->CuePoints.GetNext(pos2)->CueTime);
	}

	nKFs = nKFsTmp;

	return S_OK;
}

//
// CMatroskaSourceFilter
//

CMatroskaSourceFilter::CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CMatroskaSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// CMatroskaSplitterOutputPin
//

CMatroskaSplitterOutputPin::CMatroskaSplitterOutputPin(
		int nMinCache, REFERENCE_TIME rtDefaultDuration,
		CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
	, m_nMinCache(nMinCache), m_rtDefaultDuration(rtDefaultDuration)
{
	m_nMinCache = max(m_nMinCache, 2);
}

CMatroskaSplitterOutputPin::~CMatroskaSplitterOutputPin()
{
}

HRESULT CMatroskaSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(&m_csQueue);
		m_packets.RemoveAll();
		m_rob.RemoveAll();
		m_tos.RemoveAll();
	}

	return __super::DeliverEndFlush();
}

HRESULT CMatroskaSplitterOutputPin::DeliverEndOfStream()
{
	CAutoLock cAutoLock(&m_csQueue);

	// send out the last remaining packets from the queue

	while(m_rob.GetCount())
	{
		MatroskaPacket* mp = m_rob.RemoveHead();
		if(m_rob.GetCount() && !mp->b->BlockDuration.IsValid()) 
			mp->rtStop = m_rob.GetHead()->rtStart;
		else if(m_rob.GetCount() == 0 && m_rtDefaultDuration > 0)
			mp->rtStop = mp->rtStart + m_rtDefaultDuration;

		timeoverride to = {mp->rtStart, mp->rtStop};
		m_tos.AddTail(to);
	}

	while(m_packets.GetCount())
	{
		HRESULT hr = DeliverBlock(m_packets.RemoveHead());
		if(hr != S_OK) return hr;
	}

	return __super::DeliverEndOfStream();
}

HRESULT CMatroskaSplitterOutputPin::DeliverPacket(CAutoPtr<Packet> p)
{
	MatroskaPacket* mp = dynamic_cast<MatroskaPacket*>(p.m_p);
	if(!mp) return __super::DeliverPacket(p);

	// don't try to understand what's happening here, it's magic

	CAutoLock cAutoLock(&m_csQueue);

	CAutoPtr<MatroskaPacket> p2;
	p.Detach();
	p2.Attach(mp);
	m_packets.AddTail(p2);

	POSITION pos = m_rob.GetTailPosition();
	for(int i = m_nMinCache-1; i > 0 && pos && mp->b->ReferencePriority < m_rob.GetAt(pos)->b->ReferencePriority; i--)
		m_rob.GetPrev(pos);

	if(!pos) m_rob.AddHead(mp);
	else m_rob.InsertAfter(pos, mp);

	mp = NULL;

	if(m_rob.GetCount() == m_nMinCache+1)
	{
		ASSERT(m_nMinCache > 0);
		pos = m_rob.GetHeadPosition();
		MatroskaPacket* mp1 = m_rob.GetNext(pos);
		MatroskaPacket* mp2 = m_rob.GetNext(pos);
		if(!mp1->b->BlockDuration.IsValid())
		{
			mp1->b->BlockDuration.Set(1); // just to set it valid

			if(mp1->rtStart >= mp2->rtStart)
			{
/*				CString str;
				str.Format(_T("mp1->rtStart (%I64d) >= mp2->rtStart (%I64d)!!!\n"), mp1->rtStart, mp2->rtStart);
				AfxMessageBox(str);
*/				TRACE(_T("mp1->rtStart (%I64d) >= mp2->rtStart (%I64d)!!!\n"), mp1->rtStart, mp2->rtStart);
			}
			else
			{
				mp1->rtStop = mp2->rtStart;
			}
		}
	}

	while(m_packets.GetCount())
	{
		mp = m_packets.GetHead();
		if(!mp->b->BlockDuration.IsValid()) break;
        
		mp = m_rob.RemoveHead();
		timeoverride to = {mp->rtStart, mp->rtStop};
		m_tos.AddTail(to);

		HRESULT hr = DeliverBlock(m_packets.RemoveHead());
		if(hr != S_OK) return hr;
	}

	return S_OK;
}

HRESULT CMatroskaSplitterOutputPin::DeliverBlock(MatroskaPacket* p)
{
	HRESULT hr = S_FALSE;

	if(m_tos.GetCount())
	{
		timeoverride to = m_tos.RemoveHead();
//		if(p->TrackNumber == 2)
		TRACE(_T("(track=%d) %I64d, %I64d -> %I64d, %I64d\n"), p->TrackNumber, p->rtStart, p->rtStop, to.rtStart, to.rtStop);
		p->rtStart = to.rtStart;
		p->rtStop = to.rtStop;
	}
		
	REFERENCE_TIME 
		rtStart = p->rtStart,
		rtDelta = (p->rtStop - p->rtStart) / p->b->BlockData.GetCount(),
		rtStop = p->rtStart + rtDelta;

	POSITION pos = p->b->BlockData.GetHeadPosition();
	while(pos)
	{
		CAutoPtr<Packet> tmp(new Packet());
		tmp->TrackNumber = -1;
		tmp->bDiscontinuity = p->bDiscontinuity;
		tmp->bSyncPoint = p->bSyncPoint;
		tmp->rtStart = rtStart;
		tmp->rtStop = rtStop;
		tmp->pData.Copy(*p->b->BlockData.GetNext(pos));
		if(S_OK != (hr = DeliverPacket(tmp))) break;

		rtStart += rtDelta;
		rtStop += rtDelta;

		p->bSyncPoint = false;
		p->bDiscontinuity = false;
	}

	return hr;
}
