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
#include "Ap4IsmaCryp.h"
#include "Ap4AvcCAtom.h"

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

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	if(AP4_Movie* movie = (AP4_Movie*)m_pFile->GetMovie())
	{
		for(AP4_List<AP4_Track>::Item* item = movie->GetTracks().FirstItem();
			item;
			item = item->GetNext())
		{
			AP4_Track* track = item->GetData();

			if(track->GetType() != AP4_Track::TYPE_VIDEO 
			&& track->GetType() != AP4_Track::TYPE_AUDIO
			&& track->GetType() != AP4_Track::TYPE_TEXT)
				continue;

			AP4_Sample sample;

			if(!AP4_SUCCEEDED(track->GetSample(0, sample)) || sample.GetDescriptionIndex() == 0xFFFFFFFF)
				continue;

			CArray<CMediaType> mts;

			CMediaType mt;
			mt.SetSampleSize(1);

			VIDEOINFOHEADER* vih = NULL;
			WAVEFORMATEX* wfe = NULL;

			AP4_DataBuffer empty;

			if(AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex()))
			{
				AP4_MpegSampleDescription* mpeg_desc = NULL;

				if(desc->GetType() == AP4_SampleDescription::TYPE_MPEG)
				{
					mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(desc);
				}
				else if(desc->GetType() == AP4_SampleDescription::TYPE_ISMACRYP)
				{
					AP4_IsmaCrypSampleDescription* isma_desc = dynamic_cast<AP4_IsmaCrypSampleDescription*>(desc);
					mpeg_desc = isma_desc->GetOriginalSampleDescription();
				}

				if(AP4_MpegVideoSampleDescription* video_desc = 
					dynamic_cast<AP4_MpegVideoSampleDescription*>(mpeg_desc))
				{
					const AP4_DataBuffer* di = video_desc->GetDecoderInfo();
					if(!di) di = &empty;

					mt.majortype = MEDIATYPE_Video;

					mt.formattype = FORMAT_VideoInfo;
					vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + di->GetDataSize());
					memset(vih, 0, mt.FormatLength());
					vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
					vih->bmiHeader.biWidth = (LONG)video_desc->GetWidth();
					vih->bmiHeader.biHeight = (LONG)video_desc->GetHeight();
					memcpy(vih + 1, di->GetData(), di->GetDataSize());

					switch(video_desc->GetObjectTypeId())
					{
					case AP4_MPEG4_VISUAL_OTI:
						mt.subtype = FOURCCMap('v4pm');
						mt.formattype = FORMAT_MPEG2Video;
						{
						MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + di->GetDataSize());
						memset(vih, 0, mt.FormatLength());
						vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
						vih->hdr.bmiHeader.biWidth = (LONG)video_desc->GetWidth();
						vih->hdr.bmiHeader.biHeight = (LONG)video_desc->GetHeight();
						vih->hdr.bmiHeader.biCompression = 'v4pm';
						vih->hdr.bmiHeader.biPlanes = 1;
						vih->hdr.bmiHeader.biBitCount = 24;
						vih->hdr.dwPictAspectRatioX = vih->hdr.bmiHeader.biWidth;
						vih->hdr.dwPictAspectRatioY = vih->hdr.bmiHeader.biHeight;
						vih->cbSequenceHeader = di->GetDataSize();
						memcpy(vih->dwSequenceHeader, di->GetData(), di->GetDataSize());
						}
						mts.Add(mt);
						break;
					case AP4_MPEG2_VISUAL_SIMPLE_OTI:
					case AP4_MPEG2_VISUAL_MAIN_OTI:
					case AP4_MPEG2_VISUAL_SNR_OTI:
					case AP4_MPEG2_VISUAL_SPATIAL_OTI:
					case AP4_MPEG2_VISUAL_HIGH_OTI:
					case AP4_MPEG2_VISUAL_422_OTI:
						mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
						{
						m_pFile->Seek(sample.GetOffset());
						CBaseSplitterFileEx::seqhdr h;
						CMediaType mt2;
						if(m_pFile->Read(h, sample.GetSize(), &mt2))
							mt = mt2;
						}
						mts.Add(mt);
						break;
					case AP4_MPEG1_VISUAL_OTI: // ???
						mt.subtype = MEDIASUBTYPE_MPEG1Payload;
						mts.Add(mt);
						break;
					}

					if(mt.subtype == GUID_NULL)
					{
						TRACE(_T("Unknown video OBI: %02x\n"), video_desc->GetObjectTypeId());
					}
				}
				else if(AP4_MpegAudioSampleDescription* audio_desc = 
					dynamic_cast<AP4_MpegAudioSampleDescription*>(mpeg_desc))
				{
					const AP4_DataBuffer* di = audio_desc->GetDecoderInfo();
					if(!di) di = &empty;

					mt.majortype = MEDIATYPE_Audio;
					mt.formattype = FORMAT_WaveFormatEx;

					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + di->GetDataSize());
					memset(wfe, 0, mt.FormatLength());
					wfe->nSamplesPerSec = audio_desc->GetSampleRate();
					wfe->nAvgBytesPerSec = audio_desc->GetAvgBitrate()*8; // GetSampleSize()
					wfe->nChannels = audio_desc->GetChannelCount();
					wfe->cbSize = (WORD)di->GetDataSize();
					memcpy(wfe + 1, di->GetData(), di->GetDataSize());

					switch(audio_desc->GetObjectTypeId())
					{
					case AP4_MPEG4_AUDIO_OTI:
					case AP4_MPEG2_AAC_AUDIO_MAIN_OTI: // ???
					case AP4_MPEG2_AAC_AUDIO_LC_OTI: // ???
					case AP4_MPEG2_AAC_AUDIO_SSRP_OTI: // ???
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_AAC);
						mts.Add(mt);
						break;
					case AP4_MPEG2_PART3_AUDIO_OTI: // ???
						break;
					case AP4_MPEG1_AUDIO_OTI:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MP3);
						{
						m_pFile->Seek(sample.GetOffset());
						CBaseSplitterFileEx::mpahdr h;
						CMediaType mt2;
						if(m_pFile->Read(h, sample.GetSize(), false, &mt2))
							mt = mt2;
						}
						mts.Add(mt);
						break;
					}

					if(mt.subtype == GUID_NULL)
					{
						TRACE(_T("Unknown audio OBI: %02x\n"), audio_desc->GetObjectTypeId());
					}
				}
				else if(AP4_UnknownSampleDescription* unknown_desc = 
					dynamic_cast<AP4_UnknownSampleDescription*>(desc)) // TEMP
				{
					AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

					if(dynamic_cast<AP4_TextSampleEntry*>(sample_entry)
					|| dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry))
					{
						mt.majortype = MEDIATYPE_Subtitle;
						mt.subtype = MEDIASUBTYPE_ASS;
						mt.formattype = FORMAT_SubtitleInfo;
						CStringA hdr = "[Script Info]\nScriptType: v4.00+\n";
						SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + hdr.GetLength());
						memset(si, 0, mt.FormatLength());
						si->dwOffset = sizeof(SUBTITLEINFO);
						strcpy_s(si->IsoLang, countof(si->IsoLang), track->GetTrackLanguage().c_str());
						wcscpy_s(si->TrackName, countof(si->TrackName), CStringW(track->GetTrackName().c_str()));
						memcpy(si + 1, (LPCSTR)hdr, hdr.GetLength());
						// mts.Add(mt); // TODO

						mt.subtype = MEDIASUBTYPE_UTF8;
						si = (SUBTITLEINFO*)mt.ReallocFormatBuffer(sizeof(SUBTITLEINFO));
						mts.Add(mt);
					}
				}
			}
			else if(AP4_Avc1SampleEntry* avc1 = dynamic_cast<AP4_Avc1SampleEntry*>(
				track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd/avc1")))
			{
				if(AP4_AvcCAtom* avcC = dynamic_cast<AP4_AvcCAtom*>(avc1->GetChild(AP4_ATOM_TYPE_AVCC)))
				{
					const AP4_DataBuffer* di = avcC->GetDecoderInfo();
					if(!di) di = &empty;

					const AP4_Byte* data = di->GetData();
					AP4_Size size = di->GetDataSize();

					mt.majortype = MEDIATYPE_Video;
					mt.subtype = FOURCCMap('1cva');
					mt.formattype = FORMAT_MPEG2Video;

					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + size);
					memset(vih, 0, mt.FormatLength());
					vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biWidth = (LONG)avc1->GetWidth();
					vih->hdr.bmiHeader.biHeight = (LONG)avc1->GetHeight();
					vih->hdr.bmiHeader.biCompression = '1cva';
					vih->hdr.bmiHeader.biPlanes = 1;
					vih->hdr.bmiHeader.biBitCount = 24;
					vih->hdr.dwPictAspectRatioX = vih->hdr.bmiHeader.biWidth;
					vih->hdr.dwPictAspectRatioY = vih->hdr.bmiHeader.biHeight;
					vih->dwProfile = data[1];
					vih->dwLevel = data[3];
					vih->dwFlags = (data[4] & 3) + 1;

					vih->cbSequenceHeader = 0;

					BYTE* src = (BYTE*)data + 5;
					BYTE* dst = (BYTE*)vih->dwSequenceHeader;

					BYTE* src_end = (BYTE*)data + size;
					BYTE* dst_end = (BYTE*)vih->dwSequenceHeader + size;

					for(int i = 0; i < 2; i++)
					{
						for(int n = *src++ & 0x1f; n > 0; n--)
						{
							int len = ((src[0] << 8) | src[1]) + 2;
							if(src + len > src_end || dst + len > dst_end) {ASSERT(0); break;}
							memcpy(dst, src, len);
							src += len; 
							dst += len;
							vih->cbSequenceHeader += len;
						}
					}

					mts.Add(mt);
				}
			}
			else if(AP4_VisualSampleEntry* s263 = dynamic_cast<AP4_VisualSampleEntry*>(
				track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd/s263")))
			{
				mt.majortype = MEDIATYPE_Video;
				mt.subtype = FOURCCMap('362s');
				mt.formattype = FORMAT_VideoInfo;
				vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
				memset(vih, 0, mt.FormatLength());
				vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
				vih->bmiHeader.biWidth = (LONG)s263->GetWidth();
				vih->bmiHeader.biHeight = (LONG)s263->GetHeight();
				vih->bmiHeader.biCompression = '362s';
				mts.Add(mt);
			}
			else if(AP4_AudioSampleEntry* samr = dynamic_cast<AP4_AudioSampleEntry*>(
				track->GetTrakAtom()->FindChild("mdia/minf/stbl/stsd/samr")))
			{
				mt.majortype = MEDIATYPE_Audio;
				mt.subtype = FOURCCMap('rmas');
				mt.formattype = FORMAT_WaveFormatEx;
				wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(wfe, 0, mt.FormatLength());
				wfe->nSamplesPerSec = samr->GetSampleRate();
				wfe->nChannels = samr->GetChannelCount();
				mts.Add(mt);
			}

			REFERENCE_TIME rtDuration = 10000i64 * track->GetDurationMs();
			if(m_rtDuration < rtDuration) m_rtDuration = rtDuration;

			DWORD id = track->GetId();

			CStringW name, lang;
			name.Format(L"Output %d", id);

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

			CAutoPtr<CBaseSplitterOutputPin> pPinOut(new CBaseSplitterOutputPin(mts, name, this, this, &hr));

			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));

			m_trackpos[id] = trackpos();
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

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
						CStringA str;

						if(size >= 2 && ptr[2] == 0xfe && ptr[3] == 0xff)
						{
							CStringW wstr = CStringW((LPCWSTR)&ptr[2], size/2);
							for(int i = 0; i < wstr.GetLength(); i++) wstr.SetAt(i, ((WORD)wstr[i] >> 8) | ((WORD)wstr[i] << 8));
							str = UTF16To8(wstr);
						}
						else
						{
							str = CStringA((LPCSTR)&ptr[2], size);
						}

						// TODO: still very incomplete

						if(CBaseSplitterOutputPin* pPin = GetOutputPin(p->TrackNumber))
						if(pPin->CurrentMediaType().subtype == MEDIASUBTYPE_ASS)
						{
							CStringW wstr = UTF8To16(str);

							AP4_SampleDescription* desc = track->GetSampleDescription(sample.GetDescriptionIndex());

							if(AP4_UnknownSampleDescription* unknown_desc = dynamic_cast<AP4_UnknownSampleDescription*>(desc)) // TEMP
							{
								AP4_SampleEntry* sample_entry = unknown_desc->GetSampleEntry();

								if(AP4_TextSampleEntry* text = dynamic_cast<AP4_TextSampleEntry*>(sample_entry))
								{
									const AP4_TextSampleEntry::AP4_TextDescription& d = text->GetDescription();
								}
								else if(AP4_Tx3gSampleEntry* tx3g = dynamic_cast<AP4_Tx3gSampleEntry*>(sample_entry))
								{
									const AP4_Tx3gSampleEntry::AP4_Tx3gDescription& d = tx3g->GetDescription();

									int align = 2;
									signed char h = (signed char)d.HorizontalJustification;
									signed char v = (signed char)d.VerticalJustification;
									if(h == 0 && v < 0) align = 1;
									else if(h > 0 && v < 0) align = 2;
									else if(h < 0 && v < 0) align = 3;
									else if(h == 0 && v > 0) align = 4;
									else if(h > 0 && v > 0) align = 5;
									else if(h < 0 && v > 0) align = 6;
									else if(h == 0 && v == 0) align = 7;
									else if(h > 0 && v == 0) align = 8;
									else if(h < 0 && v == 0) align = 9;
									wstr.Format(L"{\\an%d}%s", align, CStringW(wstr));

									if(d.BackgroundColor)
									{
										DWORD c = d.BackgroundColor;
										wstr.Format(L"{\\3c%02x%02x%02x}%s", (c>>8)&0xff, (c>>16)&0xff, (c>>24)&0xff, CStringW(wstr));
										if(c&0xff) wstr.Format(L"{\\3a%02x}%s", 255 - (c&0xff), CStringW(wstr));
									}

									if(d.Style.Font.Color)
									{
										DWORD c = d.Style.Font.Color;
										wstr.Format(L"{\\1c%02x%02x%02x}%s", (c>>8)&0xff, (c>>16)&0xff, (c>>24)&0xff, CStringW(wstr));
										if(c&0xff) wstr.Format(L"{\\1a%02x}%s", 255 - (c&0xff), CStringW(wstr));
									}

									if(d.Style.Font.Size) wstr.Format(L"{\\fs%d}%s", d.Style.Font.Size, CStringW(wstr));

									if(d.Style.Font.Face&1) wstr = L"{\\b1}" + wstr;
									if(d.Style.Font.Face&2) wstr = L"{\\i1}" + wstr;
									if(d.Style.Font.Face&4) wstr = L"{\\u1}" + wstr;
								}
							}

							str = "0,0,Default,,0000,0000,0000,," + UTF16To8(wstr);
						}

						p->pData.SetSize(str.GetLength());
						memcpy(p->pData.GetData(), (LPCSTR)str, str.GetLength());

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
