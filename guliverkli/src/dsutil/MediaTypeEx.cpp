#include "StdAfx.h"
#include "DSUtil.h"
#include "MediaTypeEx.h"

#include <mmreg.h>
#include <initguid.h>
#include "..\..\include\moreuuids.h"
#include "..\..\include\ogg\OggDS.h"

CMediaTypeEx::CMediaTypeEx()
{
}

CString CMediaTypeEx::ToString(IPin* pPin)
{
	CString packing, type, codec, dim, rate, dur;

	// TODO

	if(majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
	{
		packing = _T("Encrypted MPEG2 Pack");
	}
	else if(majortype == MEDIATYPE_MPEG2_PACK)
	{
		packing = _T("MPEG2 Pack");
	}
	else if(majortype == MEDIATYPE_MPEG2_PES)
	{
		packing = _T("MPEG2 PES");
	}
	
	if(majortype == MEDIATYPE_Video)
	{
		type = _T("Video");

		BITMAPINFOHEADER bih;
		bool fBIH = ExtractBIH(this, &bih);

		int w, h, arx, ary;
		bool fDim = ExtractDim(this, w, h, arx, ary);

		if(fBIH && bih.biCompression)
		{
			codec = GetVideoCodecName(subtype, bih.biCompression);
		}

		if(codec.IsEmpty())
		{
			if(formattype == FORMAT_MPEGVideo) codec = _T("MPEG1 Video");
			else if(formattype == FORMAT_MPEG2_VIDEO) codec = _T("MPEG2 Video");
			else if(formattype == FORMAT_DiracVideoInfo) codec = _T("Dirac Video");
		}

		if(fDim)
		{
			dim.Format(_T("%dx%d"), w, h);
			if(w*ary != h*arx) dim.Format(_T("%s (%d:%d)"), CString(dim), arx, ary);
		}

		if(formattype == FORMAT_VideoInfo || formattype == FORMAT_MPEGVideo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;
			if(vih->AvgTimePerFrame) rate.Format(_T("%0.2ffps "), 10000000.0f / vih->AvgTimePerFrame);
			if(vih->dwBitRate) rate.Format(_T("%s%dKbps"), CString(rate), vih->dwBitRate/1000);
		}
		else if(formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO || formattype == FORMAT_DiracVideoInfo)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pbFormat;
			if(vih->AvgTimePerFrame) rate.Format(_T("%0.2ffps "), 10000000.0f / vih->AvgTimePerFrame);
			if(vih->dwBitRate) rate.Format(_T("%s%dKbps"), CString(rate), vih->dwBitRate/1000);
		}

		rate.Trim();

		if(subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
		{
			type = _T("Subtitle");
			codec = _T("DVD Subpicture");
		}
	}
	else if(majortype == MEDIATYPE_Audio)
	{
		type = _T("Audio");

		if(formattype == FORMAT_WaveFormatEx)
		{
			WAVEFORMATEX* wfe = (WAVEFORMATEX*)Format();

			if(wfe->wFormatTag/* > WAVE_FORMAT_PCM && wfe->wFormatTag < WAVE_FORMAT_EXTENSIBLE
			&& wfe->wFormatTag != WAVE_FORMAT_IEEE_FLOAT*/
			|| subtype != GUID_NULL)
			{
				codec = GetAudioCodecName(subtype, wfe->wFormatTag);
				dim.Format(_T("%dHz"), wfe->nSamplesPerSec);
				if(wfe->nChannels == 1) dim.Format(_T("%s mono"), CString(dim));
				else if(wfe->nChannels == 2) dim.Format(_T("%s stereo"), CString(dim));
				else dim.Format(_T("%s %dch"), CString(dim), wfe->nChannels);
				if(wfe->nAvgBytesPerSec) rate.Format(_T("%dKbps"), wfe->nAvgBytesPerSec*8/1000);
			}
		}
		else if(formattype == FORMAT_VorbisFormat)
		{
			VORBISFORMAT* vf = (VORBISFORMAT*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim.Format(_T("%dHz"), vf->nSamplesPerSec);
			if(vf->nChannels == 1) dim.Format(_T("%s mono"), CString(dim));
			else if(vf->nChannels == 2) dim.Format(_T("%s stereo"), CString(dim));
			else dim.Format(_T("%s %dch"), CString(dim), vf->nChannels);
			if(vf->nAvgBitsPerSec) rate.Format(_T("%dKbps"), vf->nAvgBitsPerSec/1000);
		}
		else if(formattype == FORMAT_VorbisFormat2)
		{
			VORBISFORMAT2* vf = (VORBISFORMAT2*)Format();

			codec = GetAudioCodecName(subtype, 0);
			dim.Format(_T("%dHz"), vf->SamplesPerSec);
			if(vf->Channels == 1) dim.Format(_T("%s mono"), CString(dim));
			else if(vf->Channels == 2) dim.Format(_T("%s stereo"), CString(dim));
			else dim.Format(_T("%s %dch"), CString(dim), vf->Channels);
		}				
	}
	else if(majortype == MEDIATYPE_Text)
	{
		type = _T("Text");
	}
	else if(majortype == MEDIATYPE_Subtitle)
	{
		type = _T("Subtitle");
		codec = GetSubtitleCodecName(subtype);
	}
	else
	{
		type = _T("Unknown");
	}

	if(CComQIPtr<IMediaSeeking> pMS = pPin)
	{
		REFERENCE_TIME rtDur = 0;
		if(SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur)
		{
			rtDur /= 10000000;
			int s = rtDur%60;
			rtDur /= 60;
			int m = rtDur%60;
			rtDur /= 60;
			int h = rtDur;
			if(h) dur.Format(_T("%d:%02d:%02d"), h, m, s);
			else if(m) dur.Format(_T("%02d:%02d"), m, s);
			else if(s) dur.Format(_T("%ds"), s);
		}
	}

	CString str;
	if(!codec.IsEmpty()) str += codec + _T(" ");
	if(!dim.IsEmpty()) str += dim + _T(" ");
	if(!rate.IsEmpty()) str += rate + _T(" ");
	if(!dur.IsEmpty()) str += dur + _T(" ");
	str.Trim(_T(" ,"));
	
	if(!str.IsEmpty()) str = type + _T(": ") + str;
	else str = type;

	return str;
}

CString CMediaTypeEx::GetVideoCodecName(const GUID& subtype, DWORD biCompression)
{
	CString str;

	static CAtlMap<CString, CString, CStringElementTraits<CString> > names;

	if(names.IsEmpty())
	{
		names[_T("WMV1")] = _T("Windows Media Video 7");
		names[_T("WMV2")] = _T("Windows Media Video 8");
		names[_T("WMV3")] = _T("Windows Media Video 9");
		names[_T("DIV3")] = _T("DivX 3");
		names[_T("DX50")] = _T("DivX 5");
		names[_T("MP4V")] = _T("MPEG4 Video");
		names[_T("AVC1")] = _T("MPEG4 Video (AVC)");
		names[_T("RV10")] = _T("RealVideo 1");
		names[_T("RV20")] = _T("RealVideo 2");
		names[_T("RV30")] = _T("RealVideo 3");
		names[_T("RV40")] = _T("RealVideo 4");
		// names[_T("")] = _T("");
	}

	if(biCompression)
	{
		CString fcc;
		fcc.Format(_T("%4.4hs"), &biCompression);
		fcc.MakeUpper();

		if(!names.Lookup(fcc, str))
		{
			if(subtype == MEDIASUBTYPE_DiracVideo) str = _T("Dirac Video");
			// else if(subtype == ) str = _T("");
			else if(biCompression < 256) str.Format(_T("%d"), biCompression);
			else str = fcc;
		}
	}

	return str;
}

CString CMediaTypeEx::GetAudioCodecName(const GUID& subtype, WORD wFormatTag)
{
	CString str;

	static CAtlMap<WORD, CString> names;

	if(names.IsEmpty())
	{
		names[WAVE_FORMAT_PCM] = _T("PCM");
		names[WAVE_FORMAT_EXTENSIBLE] = _T("WAVE_FORMAT_EXTENSIBLE");
		names[WAVE_FORMAT_IEEE_FLOAT] = _T("IEEE Float");
		names[WAVE_FORMAT_ADPCM] = _T("MS ADPCM");
		names[WAVE_FORMAT_ALAW] = _T("aLaw");
		names[WAVE_FORMAT_MULAW] = _T("muLaw");
		names[WAVE_FORMAT_DRM] = _T("DRM");
		names[WAVE_FORMAT_OKI_ADPCM] = _T("OKI ADPCM");
		names[WAVE_FORMAT_DVI_ADPCM] = _T("DVI ADPCM");
		names[WAVE_FORMAT_IMA_ADPCM] = _T("IMA ADPCM");
		names[WAVE_FORMAT_MEDIASPACE_ADPCM] = _T("Mediaspace ADPCM");
		names[WAVE_FORMAT_SIERRA_ADPCM] = _T("Sierra ADPCM");
		names[WAVE_FORMAT_G723_ADPCM] = _T("G723 ADPCM");
		names[WAVE_FORMAT_DIALOGIC_OKI_ADPCM] = _T("Dialogic OKI ADPCM");
		names[WAVE_FORMAT_MEDIAVISION_ADPCM] = _T("Media Vision ADPCM");
		names[WAVE_FORMAT_YAMAHA_ADPCM] = _T("Yamaha ADPCM");
		names[WAVE_FORMAT_DSPGROUP_TRUESPEECH] = _T("DSP Group Truespeech");
		names[WAVE_FORMAT_DOLBY_AC2] = _T("Dolby AC2");
		names[WAVE_FORMAT_GSM610] = _T("GSM610");
		names[WAVE_FORMAT_MSNAUDIO] = _T("MSN Audio");
		names[WAVE_FORMAT_ANTEX_ADPCME] = _T("Antex ADPCME");
		names[WAVE_FORMAT_CS_IMAADPCM] = _T("Crystal Semiconductor IMA ADPCM");
		names[WAVE_FORMAT_ROCKWELL_ADPCM] = _T("Rockwell ADPCM");
		names[WAVE_FORMAT_ROCKWELL_DIGITALK] = _T("Rockwell Digitalk");
		names[WAVE_FORMAT_G721_ADPCM] = _T("G721");
		names[WAVE_FORMAT_G728_CELP] = _T("G728");
		names[WAVE_FORMAT_MSG723] = _T("MSG723");
		names[WAVE_FORMAT_MPEG] = _T("MPEG Audio");
		names[WAVE_FORMAT_MPEGLAYER3] = _T("MPEG Audio Layer 3");
		names[WAVE_FORMAT_LUCENT_G723] = _T("Lucent G723");
		names[WAVE_FORMAT_VOXWARE] = _T("Voxware");
		names[WAVE_FORMAT_G726_ADPCM] = _T("G726");
		names[WAVE_FORMAT_G722_ADPCM] = _T("G722");
		names[WAVE_FORMAT_G729A] = _T("G729A");
		names[WAVE_FORMAT_MEDIASONIC_G723] = _T("MediaSonic G723");
		names[WAVE_FORMAT_ZYXEL_ADPCM] = _T("ZyXEL ADPCM");
		names[WAVE_FORMAT_RHETOREX_ADPCM] = _T("Rhetorex ADPCM");
		names[WAVE_FORMAT_VIVO_G723] = _T("Vivo G723");
		names[WAVE_FORMAT_VIVO_SIREN] = _T("Vivo Siren");
		names[WAVE_FORMAT_DIGITAL_G723] = _T("Digital G723");
		names[WAVE_FORMAT_SANYO_LD_ADPCM] = _T("Sanyo LD ADPCM");
		names[WAVE_FORMAT_CREATIVE_ADPCM] = _T("Creative ADPCM");
		names[WAVE_FORMAT_CREATIVE_FASTSPEECH8] = _T("Creative Fastspeech 8");
		names[WAVE_FORMAT_CREATIVE_FASTSPEECH10] = _T("Creative Fastspeech 10");
		names[WAVE_FORMAT_UHER_ADPCM] = _T("UHER ADPCM");
		names[WAVE_FORMAT_DOLBY_AC3] = _T("Dolby AC3");
		names[WAVE_FORMAT_DVD_DTS] = _T("DTS");
		names[WAVE_FORMAT_AAC] = _T("AAC");
		names[WAVE_FORMAT_FLAC] = _T("FLAC");
		names[WAVE_FORMAT_TTA1] = _T("TTA");
		names[WAVE_FORMAT_14_4] = _T("RealAudio 14.4");
		names[WAVE_FORMAT_28_8] = _T("RealAudio 28.8");
		names[WAVE_FORMAT_ATRC] = _T("RealAudio ATRC");
		names[WAVE_FORMAT_COOK] = _T("RealAudio COOK");
		names[WAVE_FORMAT_DNET] = _T("RealAudio DNET");
		names[WAVE_FORMAT_RAAC] = _T("RealAudio RAAC");
		names[WAVE_FORMAT_RACP] = _T("RealAudio RACP");
		names[WAVE_FORMAT_SIPR] = _T("RealAudio SIPR");
		names[WAVE_FORMAT_PS2_PCM] = _T("PS2 PCM");
		names[WAVE_FORMAT_PS2_ADPCM] = _T("PS2 ADPCM");
		names[0x0160] = _T("Windows Media Audio");
		names[0x0161] = _T("Windows Media Audio");
		names[0x0162] = _T("Windows Media Audio");
		names[0x0163] = _T("Windows Media Audio");
		// names[] = _T("");
	}

	if(!names.Lookup(wFormatTag, str))
	{
		if(subtype == MEDIASUBTYPE_Vorbis) str = _T("Vorbis (deprecated)");
		else if(subtype == MEDIASUBTYPE_Vorbis2) str = _T("Vorbis");
		else if(subtype == MEDIASUBTYPE_MP4A) str = _T("MPEG4 Audio");
		else if(subtype == MEDIASUBTYPE_FLAC_FRAMED) str = _T("FLAC (framed)");
		else if(subtype == MEDIASUBTYPE_DOLBY_AC3) str += _T("Dolby AC3");
		else if(subtype == MEDIASUBTYPE_DTS) str += _T("DTS");
		// else if(subtype == ) str = _T("");
		else str.Format(_T("0x%04x"), wFormatTag);
	}

	if(wFormatTag == WAVE_FORMAT_PCM)
	{
		if(subtype == MEDIASUBTYPE_DOLBY_AC3) str += _T(" (AC3)");
		else if(subtype == MEDIASUBTYPE_DTS) str += _T(" (DTS)");
	}

	return str;
}

CString CMediaTypeEx::GetSubtitleCodecName(const GUID& subtype)
{
	CString str;

	static CAtlMap<GUID, CString> names;

	if(names.IsEmpty())
	{
		names[MEDIASUBTYPE_UTF8] = _T("UTF-8");
		names[MEDIASUBTYPE_SSA] = _T("SubStation Alpha");
		names[MEDIASUBTYPE_ASS] = _T("Advanced SubStation Alpha");
		names[MEDIASUBTYPE_USF] = _T("Universal Subtitle Format");
		names[MEDIASUBTYPE_VOBSUB] = _T("VobSub");
		// names[''] = _T("");
	}

	if(names.Lookup(subtype, str))
	{

	}

	return str;
}
