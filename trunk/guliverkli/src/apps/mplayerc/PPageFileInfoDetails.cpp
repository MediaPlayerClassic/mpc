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

// PPageFileInfoDetails.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFileInfoDetails.h"
#include <atlbase.h>
#include "..\..\DSUtil\DSUtil.h"
#include "d3d9.h"
#include "Vmr9.h"

// CPPageFileInfoDetails dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoDetails, CPropertyPage)
CPPageFileInfoDetails::CPPageFileInfoDetails(CString fn, IFilterGraph* pFG, ISubPicAllocatorPresenter* pCAP)
	: CPropertyPage(CPPageFileInfoDetails::IDD, CPPageFileInfoDetails::IDD)
	, m_fn(fn)
	, m_pFG(pFG)
	, m_pCAP(pCAP)
	, m_hIcon(NULL)
	, m_type(_T("Not known"))
	, m_size(_T("Not known"))
	, m_time(_T("Not known"))
	, m_res(_T("Not known"))
	, m_created(_T("Not known"))
{
}

CPPageFileInfoDetails::~CPPageFileInfoDetails()
{
	if(m_hIcon) DestroyIcon(m_hIcon);
}

void CPPageFileInfoDetails::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Text(pDX, IDC_EDIT4, m_type);
	DDX_Text(pDX, IDC_EDIT3, m_size);
	DDX_Text(pDX, IDC_EDIT2, m_time);
	DDX_Text(pDX, IDC_EDIT5, m_res);
	DDX_Text(pDX, IDC_EDIT6, m_created);
	DDX_Control(pDX, IDC_EDIT7, m_encoding);
}

BEGIN_MESSAGE_MAP(CPPageFileInfoDetails, CPropertyPage)
END_MESSAGE_MAP()

inline int LNKO(int a, int b)
{
	if(a == 0 || b == 0)
		return(1);

	while(a != b)
	{
		if(a < b) b -= a;
		else if(a > b) a -= b;
	}

	return(a);
}

// CPPageFileInfoDetails message handlers

BOOL CPPageFileInfoDetails::OnInitDialog()
{
	__super::OnInitDialog();

	CString ext = m_fn.Left(m_fn.Find(_T("://"))+1).TrimRight(':');
	if(ext.IsEmpty() || !ext.CompareNoCase(_T("file")))
		ext = _T(".") + m_fn.Mid(m_fn.ReverseFind('.')+1);

	if(m_hIcon = LoadIcon(m_fn, false))
		m_icon.SetIcon(m_hIcon);

	if(!LoadType(ext, m_type))
		m_type = _T("Not known");

	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile(m_fn, &wfd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);

		__int64 size = (__int64(wfd.nFileSizeHigh)<<32)|wfd.nFileSizeLow;
		__int64 shortsize = size;
		CString measure = _T("B");
		if(shortsize > 10240) shortsize /= 1024, measure = _T("KB");
		if(shortsize > 10240) shortsize /= 1024, measure = _T("MB");
		if(shortsize > 10240) shortsize /= 1024, measure = _T("GB");
		m_size.Format(_T("%I64d%s (%I64d bytes)"), shortsize, measure, size);

		SYSTEMTIME t;
		FileTimeToSystemTime(&wfd.ftCreationTime, &t);
		TCHAR buff[256];
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &t, NULL, buff, 256);
		m_created = buff;
		m_created += _T(" ");
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &t, NULL, buff, 256);
		m_created += buff;
	}

	REFERENCE_TIME rtDur = 0;
	CComQIPtr<IMediaSeeking> pMS = m_pFG;
	if(pMS && SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur > 0)
	{
		m_time.Format(_T("%02d:%02d:%02d"), 
			int(rtDur/10000000/60/60),
			int((rtDur/10000000/60)%60),
			int((rtDur/10000000)%60));
	}

	CSize wh(0, 0), arxy(0, 0);
	long fps = 0;

	if(m_pCAP)
	{
		wh = m_pCAP->GetVideoSize(false);
		arxy = m_pCAP->GetVideoSize(true);
	}
	else
	{
		if(CComQIPtr<IBasicVideo> pBV = m_pFG)
		{
			if(SUCCEEDED(pBV->GetVideoSize(&wh.cx, &wh.cy)))
			{
				if(CComQIPtr<IBasicVideo2> pBV2 = m_pFG)
					pBV2->GetPreferredAspectRatio(&arxy.cx, &arxy.cy);
			}
			else
			{
				wh.SetSize(0, 0);
			}
		}

		if(wh.cx == 0 && wh.cy == 0)
		{
			BeginEnumFilters(m_pFG, pEF, pBF)
			{
				if(CComQIPtr<IBasicVideo> pBV = pBF)
				{
					pBV->GetVideoSize(&wh.cx, &wh.cy);
					if(CComQIPtr<IBasicVideo2> pBV2 = pBF)
						pBV2->GetPreferredAspectRatio(&arxy.cx, &arxy.cy);
					break;
				}
				else if(CComQIPtr<IVMRWindowlessControl> pWC = pBF)
				{
					pWC->GetNativeVideoSize(&wh.cx, &wh.cy, &arxy.cx, &arxy.cy);
					break;
				}
				else if(CComQIPtr<IVMRWindowlessControl9> pWC = pBF)
				{
					pWC->GetNativeVideoSize(&wh.cx, &wh.cy, &arxy.cx, &arxy.cy);
					break;
				}
			}
			EndEnumFilters
		}
	}

	if(wh.cx > 0 && wh.cy > 0)
	{
		m_res.Format(_T("%d x %d"), wh.cx, wh.cy);

		int lnko = 0;
		do
		{
			lnko = LNKO(arxy.cx, arxy.cy);
			if(lnko > 1) arxy.cx /= lnko, arxy.cy /= lnko;
		}
		while(lnko > 1);

		if(arxy.cx > 0 && arxy.cy > 0 && arxy.cx*wh.cy != arxy.cy*wh.cx)
		{
			CString ar;
			ar.Format(_T(" (AR %d:%d)"), arxy.cx, arxy.cy);
			m_res += ar;
		}
	}

	m_fn.TrimRight('/');
	m_fn.Replace('\\', '/');
	m_fn = m_fn.Mid(m_fn.ReverseFind('/')+1);

	UpdateData(FALSE);

	InitEncoding();

	m_pFG = NULL;
	m_pCAP = NULL;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

#include <mmreg.h>
#include <initguid.h>
#include "..\..\..\include\moreuuids.h"
#include "..\..\..\include\ogg\OggDS.h"

void CPPageFileInfoDetails::InitEncoding()
{
	CList<CString> sl;

	BeginEnumFilters(m_pFG, pEF, pBF)
	{
		if(IPin* pPin = GetFirstPin(pBF, PINDIR_INPUT))
		{
			CMediaType mt;
			if(FAILED(pPin->ConnectionMediaType(&mt)) || mt.majortype != MEDIATYPE_Stream)
				continue;
		}

		BeginEnumPins(pBF, pEP, pPin)
		{
			CMediaType mt;
			PIN_DIRECTION dir;
			if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
			|| FAILED(pPin->ConnectionMediaType(&mt)))
				continue;

			CString type, codec, dim, rate;

			if(mt.majortype == MEDIATYPE_Video)
			{
				type = _T("Video");

				BITMAPINFOHEADER bih;
				bool fBIH = ExtractBIH(&mt, &bih);

				int w, h, arx, ary;
				bool fDim = ExtractDim(&mt, w, h, arx, ary);

				if(fBIH && bih.biCompression)
				{
					codec = GetVideoCodecName(mt.subtype, bih.biCompression);
				}

				if(fDim)
				{
					dim.Format(_T("%dx%d"), w, h);
					if(w*ary != h*arx) dim.Format(_T("%s (%d:%d)"), CString(dim), arx, ary);
				}

				if(mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_MPEGVideo)
				{
					VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;
					rate.Format(_T("%0.2ffps %dKbps"), vih->AvgTimePerFrame ? 10000000.0f / vih->AvgTimePerFrame : 0.0f, vih->dwBitRate/1000);
				}
				else if(mt.formattype == FORMAT_VideoInfo2 || mt.formattype == FORMAT_MPEG2_VIDEO || mt.formattype == FORMAT_DiracVideoInfo)
				{
					VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.pbFormat;
					rate.Format(_T("%0.2ffps %dKbps"), vih->AvgTimePerFrame ? 10000000.0f / vih->AvgTimePerFrame : 0.0f, vih->dwBitRate/1000);
				}

				if(mt.formattype == FORMAT_MPEGVideo)
				{
					codec = _T("MPEG1 Video");
				}
				else if(mt.formattype == FORMAT_MPEG2_VIDEO)
				{
					codec = _T("MPEG2 Video");
				}
				else if(mt.formattype == FORMAT_DiracVideoInfo)
				{
					codec = _T("Dirac Video");
				}
			}
			else if(mt.majortype == MEDIATYPE_Audio)
			{
				type = _T("Audio");

				if(mt.formattype == FORMAT_WaveFormatEx)
				{
					WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
					if(wfe->wFormatTag > WAVE_FORMAT_PCM && wfe->wFormatTag < WAVE_FORMAT_EXTENSIBLE
					&& wfe->wFormatTag != WAVE_FORMAT_IEEE_FLOAT)
					{
						codec = GetAudioCodecName(mt.subtype, wfe->wFormatTag);
						dim.Format(_T("%dHz"), wfe->nSamplesPerSec);
						if(wfe->nChannels == 1) dim.Format(_T("%s mono"), CString(dim));
						else if(wfe->nChannels == 2) dim.Format(_T("%s stereo"), CString(dim));
						else dim.Format(_T("%s %dch"), CString(dim), wfe->nChannels);
						rate.Format(_T("%dKbps"), wfe->nAvgBytesPerSec*8/1000);
					}
				}
				else if(mt.formattype == FORMAT_VorbisFormat)
				{
					VORBISFORMAT* vf = (VORBISFORMAT*)mt.Format();

					codec = GetAudioCodecName(mt.subtype, 0);
					dim.Format(_T("%dHz"), vf->nSamplesPerSec);
					if(vf->nChannels == 1) dim.Format(_T("%s mono"), CString(dim));
					else if(vf->nChannels == 2) dim.Format(_T("%s stereo"), CString(dim));
					else dim.Format(_T("%s %dch"), CString(dim), vf->nChannels);
					rate.Format(_T("%dKbps"), vf->nAvgBitsPerSec/1000);
				}
				else if(mt.formattype == FORMAT_VorbisFormat2)
				{
					VORBISFORMAT2* vf = (VORBISFORMAT2*)mt.Format();

					codec = GetAudioCodecName(mt.subtype, 0);
					dim.Format(_T("%dHz"), vf->SamplesPerSec);
					if(vf->Channels == 1) dim.Format(_T("%s mono"), CString(dim));
					else if(vf->Channels == 2) dim.Format(_T("%s stereo"), CString(dim));
					else dim.Format(_T("%s %dch"), CString(dim), vf->Channels);
				}				
			}

			CString str;
			if(!codec.IsEmpty()) str += codec + _T(", ");
			if(!dim.IsEmpty()) str += dim + _T(" ");
			if(!rate.IsEmpty()) str += rate + _T(" ");
			str.Trim(_T(" ,"));
			if(!str.IsEmpty()) sl.AddTail(type + _T(": ") + str);
		}
		EndEnumPins
	}
	EndEnumFilters

	CString text = Implode(sl, '\n');
	text.Replace(_T("\n"), _T("\r\n"));
	m_encoding.SetWindowText(text);
}

CString CPPageFileInfoDetails::GetVideoCodecName(const GUID& subtype, DWORD biCompression)
{
	CString str;

	static CMap<DWORD, DWORD, CString, CString> names;

	if(names.IsEmpty())
	{
		names['WMV1'] = _T("Windows Media Video 7");
		names['WMV2'] = _T("Windows Media Video 8");
		names['WMV3'] = _T("Windows Media Video 9");
		names['DIV3'] = _T("DivX 3");
		names['DX50'] = _T("DivX 5");
		names['MP4V'] = _T("MPEG4 Video");
		names['RV10'] = _T("RealVideo 1");
		names['RV20'] = _T("RealVideo 2");
		names['RV30'] = _T("RealVideo 3");
		names['RV40'] = _T("RealVideo 4");
		// names[''] = _T("");
	}

	if(biCompression)
	{
		DWORD fcc = ((biCompression&0xff)<<24)|((biCompression&0xff00)<<8)|((biCompression&0xff0000)>>8)|((biCompression&0xff000000)>>24);
		if(!names.Lookup(fcc, str))
		{
			if(subtype == MEDIASUBTYPE_DiracVideo) str = _T("Dirac Video");
			// else if(subtype == ) str = _T("");
			else if(biCompression < 256) str.Format(_T("%d"), biCompression);
			else str.Format(_T("%4.4hs"), &biCompression);
		}
	}

	return str;
}

CString CPPageFileInfoDetails::GetAudioCodecName(const GUID& subtype, WORD wFormatTag)
{
	CString str;

	static CMap<WORD, WORD, CString, CString> names;

	if(names.IsEmpty())
	{
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
		if(subtype == MEDIASUBTYPE_Vorbis || subtype == MEDIASUBTYPE_Vorbis2) str = _T("Vorbis");
		else if(subtype == MEDIASUBTYPE_MP4A) str = _T("MPEG4 Audio");
		else if(subtype == MEDIASUBTYPE_FLAC_FRAMED) str = _T("FLAC (framed)");
		// else if(subtype == ) str = _T("");
		else str.Format(_T("0x%04x"), wFormatTag);
	}

	return str;
}
