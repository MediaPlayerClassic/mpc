#include "StdAfx.h"
#include "MP4SplitterFile.h"
#include "Ap4AsyncReaderStream.cpp" // FIXME
#include "..\..\..\DSUtil\DSUtil.h"
#include "..\..\..\..\include\moreuuids.h"

//


CMP4SplitterFile::CMP4SplitterFile(IAsyncReader* pReader, HRESULT& hr) 
	: CBaseSplitterFile(pReader, hr)
	, m_rtDuration(0)
	, m_pAp4File(NULL)
{
	if(FAILED(hr)) return;

	hr = Init();
}

CMP4SplitterFile::~CMP4SplitterFile()
{
	delete (AP4_File*)m_pAp4File;
}

void* /* AP4_Movie* */ CMP4SplitterFile::GetMovie()
{
	ASSERT(m_pAp4File);
	return m_pAp4File ? ((AP4_File*)m_pAp4File)->GetMovie() : NULL;
}

HRESULT CMP4SplitterFile::Init()
{
	Seek(0);

	m_mts.RemoveAll();
	m_rtDuration = 0;
	delete (AP4_File*)m_pAp4File;

	AP4_ByteStream* stream = new AP4_AsyncReaderStream(this);

	m_pAp4File = new AP4_File(*stream);

	if(AP4_Movie* movie = ((AP4_File*)m_pAp4File)->GetMovie())
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
						m_mts[track->GetId()] = mt;
						break;
					case AP4_MPEG2_VISUAL_SIMPLE_OTI:
					case AP4_MPEG2_VISUAL_MAIN_OTI:
					case AP4_MPEG2_VISUAL_SNR_OTI:
					case AP4_MPEG2_VISUAL_SPATIAL_OTI:
					case AP4_MPEG2_VISUAL_HIGH_OTI:
					case AP4_MPEG2_VISUAL_422_OTI:
						mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
						{
						AP4_DataBuffer data;
						track->ReadSample(0, sample, data);
						CMediaType mt2;
						if(MakeMPEG2MediaType(mt2, (BYTE*)data.GetData(), data.GetDataSize(), video_desc->GetWidth(), video_desc->GetHeight()))
							mt = mt2;
						}
						m_mts[track->GetId()] = mt;
						break;
					case AP4_MPEG1_VISUAL_OTI: // ???
						mt.subtype = MEDIASUBTYPE_MPEG1Payload;
						m_mts[track->GetId()] = mt;
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
						m_mts[track->GetId()] = mt;
						break;
					case AP4_MPEG2_PART3_AUDIO_OTI: // ???
						break;
					case AP4_MPEG1_AUDIO_OTI:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MP3);
						m_mts[track->GetId()] = mt;
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
						mt.subtype = MEDIASUBTYPE_UTF8;
						mt.formattype = FORMAT_SubtitleInfo;
						SUBTITLEINFO* si = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO));
						memset(si, 0, mt.FormatLength());
						strcpy_s(si->IsoLang, sizeof(si->IsoLang)/sizeof(si->IsoLang[0]), track->GetTrackLanguage().c_str());
						wcscpy_s(si->TrackName, sizeof(si->TrackName)/sizeof(si->TrackName[0]), CStringW(track->GetTrackName().c_str()));
						si->dwOffset = sizeof(SUBTITLEINFO);
						m_mts[track->GetId()] = mt;
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

					m_mts[track->GetId()] = mt;
				}
			}
		}

		POSITION pos = m_mts.GetStartPosition();
		while(pos)
		{
			AP4_Track* track = movie->GetTrack(m_mts.GetNextKey(pos));
			REFERENCE_TIME rtDuration = 10000i64 * track->GetDurationMs();
			if(m_rtDuration < rtDuration) m_rtDuration = rtDuration;
		}	
	}

	stream->Release();

	return m_mts.GetCount() > 0 ? S_OK : E_FAIL;
}
