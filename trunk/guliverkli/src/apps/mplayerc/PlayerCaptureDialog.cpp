/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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

// PlayerCaptureDialog.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "mainfrm.h"
#include "PlayerCaptureDialog.h"
#include "..\..\DSUtil\DSUtil.h"
#include "..\..\..\include\Ogg\OggDS.h"
#include "..\..\filters\muxer\wavdest\wavdest.h"
#include "..\..\filters\muxer\MatroskaMuxer\MatroskaMuxer.h"

static bool LoadMediaType(CStringW DisplayName, AM_MEDIA_TYPE** ppmt)
{
	bool fRet = false;

	if(!ppmt) return(fRet);

	*ppmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
	if(!*ppmt) return(fRet);

	memset(*ppmt, 0, sizeof(AM_MEDIA_TYPE));

	BYTE* pData;
	UINT len;
	if(AfxGetApp()->GetProfileBinary(_T("Capture\\") + CString(DisplayName), _T("MediaType"), &pData, &len))
	{
		if(len != sizeof(AM_MEDIA_TYPE)) return(fRet);
		memcpy(*ppmt, pData, len);
		delete [] pData;

		(*ppmt)->cbFormat = 0;
		(*ppmt)->pbFormat = NULL;

		fRet = true;

		if(AfxGetApp()->GetProfileBinary(_T("Capture\\") + CString(DisplayName), _T("Format"), &pData, &len))
		{
			if(!len) return(fRet);
			(*ppmt)->cbFormat = len;
			(*ppmt)->pbFormat = (BYTE*)CoTaskMemAlloc(len);
			memcpy((*ppmt)->pbFormat, pData, len);
			delete [] pData;

		}
	}

	return(fRet);
}

static void SaveMediaType(CStringW DisplayName, AM_MEDIA_TYPE* pmt)
{
	if(DisplayName.IsEmpty() || !pmt) return;

	AfxGetApp()->WriteProfileBinary(_T("Capture\\") + CString(DisplayName), _T("MediaType"), (BYTE*)pmt, sizeof(AM_MEDIA_TYPE));
	AfxGetApp()->WriteProfileBinary(_T("Capture\\") + CString(DisplayName), _T("Format"), pmt->pbFormat, pmt->cbFormat);
}

static void LoadDefaultCodec(CArray<Codec>& codecs, CComboBox& box, const GUID& cat)
{
	int len = box.GetCount();
	if(len >= 0) box.SetCurSel(0);

	if(cat == GUID_NULL) return;

	CString DisplayName = AfxGetApp()->GetProfileString(_T("Capture\\") + CStringFromGUID(cat), _T("DisplayName"));

	for(int i = 0; i < len; i++)
	{
		int iSel = box.GetItemData(i);
		if(iSel < 0) continue;
		
		Codec& c = codecs[iSel];
		if(DisplayName == c.DisplayName)
		{
			box.SetCurSel(i);
			if(!c.pBF)
				c.pMoniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&c.pBF);
			break;
		}
	}
}

static void SaveDefaultCodec(CArray<Codec>& codecs, CComboBox& box, const GUID& cat)
{
	if(cat == GUID_NULL) return;

	CString guid = CStringFromGUID(cat);

	AfxGetApp()->WriteProfileString(_T("Capture\\") + guid, NULL, NULL);

	int iSel = box.GetCurSel();
	if(iSel < 0) return;
	iSel = box.GetItemData(iSel);
	if(iSel < 0) return;

	Codec& codec = codecs[iSel];

	AfxGetApp()->WriteProfileString(_T("Capture\\") + guid, _T("DisplayName"), CString(codec.DisplayName));
}

static void SetupDefaultCaps(AM_MEDIA_TYPE* pmt, VIDEO_STREAM_CONFIG_CAPS& caps)
{
	memset(&caps, 0, sizeof(caps));

	if(!pmt) return;

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;

	BITMAPINFOHEADER* bih = (pmt->formattype == FORMAT_VideoInfo)
		? &((VIDEOINFOHEADER*)pmt->pbFormat)->bmiHeader
		: (pmt->formattype == FORMAT_VideoInfo2)
		? &((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader
		: NULL;

    caps.guid = GUID_NULL;
    caps.VideoStandard = 0;
	caps.InputSize.cx = bih->biWidth;
	caps.InputSize.cy = abs(bih->biHeight);
    caps.MinCroppingSize = caps.MaxCroppingSize = caps.InputSize;
    caps.CropGranularityX = caps.CropGranularityY = 1;
    caps.CropAlignX = caps.CropAlignY = 1;
    caps.MinOutputSize = CSize(64, 64);
    caps.MaxOutputSize = CSize(768, 576);
    caps.OutputGranularityX = 16;
	caps.OutputGranularityY = 1;
    caps.StretchTapsX = caps.StretchTapsY = 0;
    caps.ShrinkTapsX = caps.ShrinkTapsY = 0;
    caps.MinFrameInterval = 100000i64;
    caps.MaxFrameInterval = 100000000i64;
    caps.MinBitsPerSecond = caps.MaxBitsPerSecond = 0;
}

static void SetupDefaultCaps(AM_MEDIA_TYPE* pmt, AUDIO_STREAM_CONFIG_CAPS& caps)
{
	memset(&caps, 0, sizeof(caps));

	if(!pmt) return;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->pbFormat;

    caps.guid = GUID_NULL;
	caps.MinimumChannels = caps.MaximumChannels = wfe->nChannels;
    caps.ChannelsGranularity = 1;
	caps.MinimumBitsPerSample = caps.MaximumBitsPerSample = wfe->wBitsPerSample;
    caps.BitsPerSampleGranularity = 1;
	caps.MinimumSampleFrequency = caps.MaximumSampleFrequency = wfe->nSamplesPerSec;
    caps.SampleFrequencyGranularity = 1;
}

template<class T>
static void SetupMediaTypes(CComPtr<IAMStreamConfig> pAMSC,
							CFormatArray<T>& tfa, 
							CComboBox& type, CComboBox& dim,
							CMediaType& mt)
{
	tfa.RemoveAll();
	type.ResetContent();
	dim.ResetContent();
	type.EnableWindow(FALSE);
	dim.EnableWindow(FALSE);

	if(!pAMSC) return;

	AM_MEDIA_TYPE* pcurmt = NULL;
	pAMSC->GetFormat(&pcurmt);

	int iCount = 0, iSize;
	if(SUCCEEDED(pAMSC->GetNumberOfCapabilities(&iCount, &iSize))
	&& iSize == sizeof(T) && iCount > 0)
	{
		for(int i = 0; i < iCount; i++)
		{
			T caps;
			AM_MEDIA_TYPE* pmt;
			if(SUCCEEDED(pAMSC->GetStreamCaps(i, &pmt, (BYTE*)&caps)))
			{
				tfa.AddFormat(pmt, caps);
			}
		}
	}

	if(tfa.GetCount() == 0)
	{
		if(pcurmt && (pcurmt->majortype == MEDIATYPE_Video || pcurmt->majortype == MEDIATYPE_Audio))
		{
			AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
			CopyMediaType(pmt, pcurmt);
			T caps;
			SetupDefaultCaps(pmt, caps);
            tfa.AddFormat(pmt, caps);
		}
		else
		{
			mt.majortype = GUID_NULL;
			if(pcurmt) DeleteMediaType(pcurmt);
			return;
		}
	}

	for(int i = 0; i < (int)tfa.GetCount(); i++)
	{
		CFormat<T>* pf = tfa[i];
		int j = type.AddString(pf->name);
		type.SetItemData(j, (DWORD_PTR)pf);
	}

	CFormat<T>* pf = NULL;
	CFormatElem<T>* pfe = NULL;

	if(!pcurmt)
	{
		pf = tfa[0];
		pfe = pf->GetAt(0);
	}
	else if(!tfa.FindFormat(pcurmt, NULL, &pf, &pfe) && !tfa.FindFormat(pcurmt, &pf))
	{
		if(pcurmt) DeleteMediaType(pcurmt);
		return;
	}

	for(int i = 0; i < (int)pf->GetCount(); i++)
	{
		CFormatElem<T>* pfe = pf->GetAt(i);
		int j = dim.AddString(tfa.MakeDimensionName(pfe));
		dim.SetItemData(j, (DWORD_PTR)pfe);
	}

	int iType = type.SetCurSel(type.FindStringExact(0, pf->name));
	if(iType < 0 && type.GetCount()) type.SetCurSel(0);
	int iDim = dim.SetCurSel(dim.FindStringExact(0, tfa.MakeDimensionName(pfe)));
//	if(iDim < 0 && dim.GetCount()) dim.SetCurSel(iDim = 0);

	CorrectComboListWidth(type, type.GetParent()->GetFont());
	CorrectComboListWidth(dim, dim.GetParent()->GetFont());

	if(iDim >= 0) mt = *((CFormatElem<T>*)dim.GetItemData(iDim))->pmt;
	else if(pcurmt) mt = *pcurmt;

	type.EnableWindow(type.GetCount() > 0);
	dim.EnableWindow(dim.GetCount() > 0);

	if(pcurmt) DeleteMediaType(pcurmt);
}

template<class T>
static bool SetupDimension(CFormatArray<T>& tfa, 
						   CComboBox& type, CComboBox& dim)
{
	CString str;
	dim.GetWindowText(str);

	dim.ResetContent();
	dim.EnableWindow(FALSE);

	int iSel = type.GetCurSel();
	if(iSel < 0) return(false);

	CFormat<T>* pf = (CFormat<T>*)type.GetItemData(iSel);

	for(int i = 0; i < (int)pf->GetCount(); i++)
	{
		CFormatElem<T>* pfe = pf->GetAt(i);
		dim.SetItemData(dim.AddString(tfa.MakeDimensionName(pfe)), (DWORD_PTR)pfe);
	}

	CorrectComboListWidth(dim, dim.GetParent()->GetFont());

	dim.SetCurSel(dim.FindStringExact(0, str));
	dim.EnableWindow(dim.GetCount() > 0);

	return(dim.GetCurSel() >= 0);
}

static void InitCodecList(CArray<Codec>& codecs, CComboBox& box, const GUID& cat)
{
	codecs.RemoveAll();
	box.ResetContent();
	box.EnableWindow(FALSE);

	box.SetItemData(box.AddString(_T("Uncompressed")), (DWORD_PTR)-1);

	BeginEnumSysDev(cat, pMoniker)
	{
		Codec c;

		c.pMoniker = pMoniker;
/*
		CComPtr<IBaseFilter> pBF;
		if(FAILED(pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF)
			continue;

		c.pBF = pBF;
*/
		LPOLESTR strName = NULL;
		if(FAILED(pMoniker->GetDisplayName(NULL, NULL, &strName)))
			continue;

		c.DisplayName = strName;
		CoTaskMemFree(strName);

		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			continue;
		
		c.FriendlyName = var.bstrVal;

		CStringW str = CStringW(c.DisplayName).MakeLower();
		if(str.Find(L"@device:dmo:") == 0)
			c.FriendlyName = _T("(DMO) ") + c.FriendlyName;
		else if(str.Find(L"@device:sw:") == 0)
			c.FriendlyName = _T("(DS) ") + c.FriendlyName;
		else if(str.Find(L"@device:cm:") == 0)
			c.FriendlyName = _T("(VfW) ") + c.FriendlyName;

		box.SetItemData(
			box.AddString(c.FriendlyName),
			(DWORD_PTR)codecs.Add(c));
	}
	EndEnumSysDev

	box.EnableWindow(box.GetCount() > 1);

	CorrectComboListWidth(box, box.GetParent()->GetFont());

	LoadDefaultCodec(codecs, box, cat);
}

static int ShowPPage(CArray<Codec>& codecs, CComboBox& box, HWND hWnd = NULL)
{
	int iSel = box.GetCurSel();
	if(iSel < 0) return(-1);

	iSel = box.GetItemData(iSel);
	if(iSel < 0) return(-1);

	Codec& c = codecs[iSel];

	if(!c.pBF)
	{
		c.pMoniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&c.pBF);
	}

	if(CComQIPtr<ISpecifyPropertyPages> pSPP = c.pBF)
	{
		CAUUID caGUID;
		caGUID.pElems = NULL;
		if(SUCCEEDED(pSPP->GetPages(&caGUID)))
		{
			IUnknown* lpUnk = NULL;
			pSPP.QueryInterface(&lpUnk);
			OleCreatePropertyFrame(
				hWnd, 0, 0, CStringW(c.FriendlyName), 
				1, (IUnknown**)&lpUnk, 
				caGUID.cElems, caGUID.pElems, 
				0, 0, NULL);
			lpUnk->Release();

			if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
		}
	}
	else if(CComQIPtr<IAMVfwCompressDialogs> pAMVfWCD = c.pBF)
	{
		if(pAMVfWCD->ShowDialog(VfwCompressDialog_QueryConfig, NULL) == S_OK)
			pAMVfWCD->ShowDialog(VfwCompressDialog_Config, hWnd);		
	}

	return(iSel);
}

// CPlayerCaptureDialog dialog

IMPLEMENT_DYNAMIC(CPlayerCaptureDialog, CDialog)
CPlayerCaptureDialog::CPlayerCaptureDialog()
	: CDialog(CPlayerCaptureDialog::IDD, NULL)
	, m_vidfps(0)
	, m_muxtype(0)
	, m_file(_T(""))
	, m_fVidOutput(TRUE)
	, m_fAudOutput(TRUE)
	, m_fVidPreview(FALSE)
	, m_fAudPreview(FALSE)
	, m_nVidBuffers(0)
	, m_nAudBuffers(0)
	, m_pVidBuffer(NULL)
	, m_pAudBuffer(NULL)
	, m_fSepAudio(FALSE)
{
}

CPlayerCaptureDialog::~CPlayerCaptureDialog()
{
	EmptyVideo();
	EmptyAudio();
}

BOOL CPlayerCaptureDialog::Create(CWnd* pParent)
{
	if(!__super::Create(IDD, pParent))
		return FALSE;

	EmptyVideo();
	EmptyAudio();

	return TRUE;
}

void CPlayerCaptureDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO4, m_vidinput);
	DDX_Control(pDX, IDC_COMBO1, m_vidtype);
	DDX_Control(pDX, IDC_COMBO5, m_viddimension);
	DDX_Control(pDX, IDC_SPIN1, m_vidhor);
	DDX_Control(pDX, IDC_SPIN2, m_vidver);
	DDX_Control(pDX, IDC_EDIT3, m_vidfpsedit);
	DDX_Control(pDX, IDC_BUTTON1, m_vidsetres);
	DDX_Control(pDX, IDC_COMBO3, m_audinput);
	DDX_Control(pDX, IDC_COMBO2, m_audtype);
	DDX_Control(pDX, IDC_COMBO6, m_auddimension);
	DDX_Control(pDX, IDC_COMBO7, m_vidcodec);
	DDX_Control(pDX, IDC_COMBO9, m_vidcodectype);
	DDX_Control(pDX, IDC_COMBO10, m_vidcodecdimension);
	DDX_Check(pDX, IDC_CHECK2, m_fVidOutput);
	DDX_Control(pDX, IDC_CHECK2, m_vidoutput);
	DDX_Check(pDX, IDC_CHECK4, m_fVidPreview);
	DDX_Control(pDX, IDC_CHECK4, m_vidpreview);
	DDX_Control(pDX, IDC_COMBO8, m_audcodec);
	DDX_Control(pDX, IDC_COMBO12, m_audcodectype);
	DDX_Control(pDX, IDC_COMBO11, m_audcodecdimension);
	DDX_Check(pDX, IDC_CHECK3, m_fAudOutput);
	DDX_Control(pDX, IDC_CHECK3, m_audoutput);
	DDX_Check(pDX, IDC_CHECK5, m_fAudPreview);
	DDX_Control(pDX, IDC_CHECK5, m_audpreview);
	DDX_Text(pDX, IDC_EDIT4, m_file);
	DDX_Radio(pDX, IDC_RADIO1, m_muxtype);
	DDX_Control(pDX, IDC_BUTTON2, m_recordbtn);
	DDX_Text(pDX, IDC_EDIT9, m_nVidBuffers);
	DDX_Text(pDX, IDC_EDIT12, m_nAudBuffers);
	DDX_Check(pDX, IDC_CHECK7, m_fSepAudio);
}

BOOL CPlayerCaptureDialog::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam == VK_RETURN)
		{
			CWnd* pFocused = GetFocus();
			if(pFocused && pFocused->m_hWnd == m_vidfpsedit.m_hWnd)
			{
				UpdateGraph();
			}
		}
	}

	return __super::PreTranslateMessage(pMsg);
}


void CPlayerCaptureDialog::EmptyVideo()
{
	// first save channel from previous session

	if(m_pAMTuner && !m_VidDisplayName.IsEmpty())
	{
		long lChannel = 0, lVivSub = 0, lAudSub = 0;
		m_pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);
		AfxGetApp()->WriteProfileInt(_T("Capture\\") + CString(m_VidDisplayName), _T("Channel"), lChannel);
	}

	//

	m_vfa.RemoveAll();

	m_pAMXB = NULL;
	m_pAMTuner = NULL;
	m_pAMVSC = NULL;

	if(IsWindow(m_hWnd))
	{
		m_vidinput.ResetContent();
		m_vidinput.EnableWindow(FALSE);
		m_vidtype.ResetContent();
		m_vidtype.EnableWindow(FALSE);
		m_viddimension.ResetContent();
		m_viddimension.EnableWindow(FALSE);
		m_vidhor.EnableWindow(FALSE);
		m_vidver.EnableWindow(FALSE);
		m_vidfpsedit.EnableWindow(FALSE);
		m_vidfps = 0;
		m_vidsetres.EnableWindow(FALSE);
		UpdateData(FALSE);
	}
}

void CPlayerCaptureDialog::EmptyAudio()
{
	m_afa.RemoveAll();

	m_pAMASC = NULL;
	m_pAMAIM.RemoveAll();

	if(IsWindow(m_hWnd))
	{
		m_audinput.ResetContent();
		m_audinput.EnableWindow(FALSE);
		m_audtype.ResetContent();
		m_audtype.EnableWindow(FALSE);
		m_auddimension.ResetContent();
		m_auddimension.EnableWindow(FALSE);
		UpdateData(FALSE);
	}
}

void CPlayerCaptureDialog::UpdateMediaTypes()
{
	UpdateData();

	// fps

	CString fps;
	m_vidfpsedit.GetWindowText(fps);
	if(!fps.IsEmpty())
	{
		float ffps;
		_stscanf(fps, _T("%f"), &ffps);
		if(ffps > 0) m_vidfps = ffps;
	}

	// video

	{
		AM_MEDIA_TYPE* pmt = NULL;
		VIDEO_STREAM_CONFIG_CAPS* pcaps = NULL;

		int i = m_viddimension.GetCurSel();
		if(i >= 0)
		{
			pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
			CopyMediaType(pmt, ((CVidFormatElem*)m_viddimension.GetItemData(i))->pmt);
			pcaps = &((CVidFormatElem*)m_viddimension.GetItemData(i))->caps;
		}
		else if(m_pAMVSC)
		{
			m_pAMVSC->GetFormat(&pmt);
		}

		if(pmt)
		{
			if(m_vidfps > 0)
			{
				REFERENCE_TIME atpf = (REFERENCE_TIME)(10000000.0 / m_vidfps);

				if(pcaps)
				{
					// FIXME: some drivers do not set the interval right and they still accept the preferable but unfortunately out-of-range fps
//					atpf = min(max(atpf, pcaps->MinFrameInterval), pcaps->MaxFrameInterval);
				}

				if(pmt->formattype == FORMAT_VideoInfo)
					((VIDEOINFOHEADER*)pmt->pbFormat)->AvgTimePerFrame = atpf;
				else if(pmt->formattype == FORMAT_VideoInfo2)
					((VIDEOINFOHEADER2*)pmt->pbFormat)->AvgTimePerFrame = atpf;
			}

			BITMAPINFOHEADER* bih = (pmt->formattype == FORMAT_VideoInfo)
				? &((VIDEOINFOHEADER*)pmt->pbFormat)->bmiHeader
				: (pmt->formattype == FORMAT_VideoInfo2)
				? &((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader
				: NULL;

			bih->biWidth = m_vidhor.GetPos();
			bih->biHeight = m_vidver.GetPos();
			bih->biSizeImage = bih->biWidth*bih->biHeight*bih->biBitCount>>3;

			SaveMediaType(m_VidDisplayName, pmt);

			CopyMediaType(&m_mtv, pmt);
			DeleteMediaType(pmt);
		}
	}

	// audio

	{
		AM_MEDIA_TYPE* pmt = NULL;

		int i = m_auddimension.GetCurSel();
		if(i >= 0)
		{
			pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
			CopyMediaType(pmt, ((CAudFormatElem*)m_auddimension.GetItemData(i))->pmt);
		}
		else if(m_pAMASC)
		{
			m_pAMASC->GetFormat(&pmt);
		}

		if(pmt)
		{
			SaveMediaType(m_AudDisplayName, pmt);

			CopyMediaType(&m_mta, pmt);  
			DeleteMediaType(pmt);
		}
	}
}

void CPlayerCaptureDialog::UpdateUserDefinableControls()
{
	int iSel = m_viddimension.GetCurSel();
	if(iSel < 0) return;

	CVidFormatElem* pvfe = (CVidFormatElem*)m_viddimension.GetItemData(iSel);
	if(!pvfe) return;

	if(!m_pAMVSC) return;

	AM_MEDIA_TYPE* pmt = NULL;
	m_pAMVSC->GetFormat(&pmt);
	if(!pmt) return;

	BITMAPINFOHEADER* bih = (pmt->formattype == FORMAT_VideoInfo)
		? &((VIDEOINFOHEADER*)pmt->pbFormat)->bmiHeader
		: (pmt->formattype == FORMAT_VideoInfo2)
		? &((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader
		: NULL;

	if(!bih) return;

	UDACCEL ua[3] = {{0,0},{2,0},{4,0}};

	int w = m_vidhor.GetPos(), h = m_vidver.GetPos();

	m_vidhor.SetRange((short)pvfe->caps.MinOutputSize.cx, (short)pvfe->caps.MaxOutputSize.cx);
/*	if(bih->biCompression == mmioFOURCC('Y','U','Y','2')) // FIXME: bt8x8 drivers seem to crop the right side in yuv2 mode if the width is not divisable by 64
		pvfe->caps.OutputGranularityX = 64;
*/
	ua[0].nInc = pvfe->caps.OutputGranularityX;
	ua[1].nInc = pvfe->caps.OutputGranularityX*2;
	ua[2].nInc = pvfe->caps.OutputGranularityX*4;
	m_vidhor.SetAccel(3, ua);

	m_vidver.SetRange((short)pvfe->caps.MinOutputSize.cy, (short)pvfe->caps.MaxOutputSize.cy);
	ua[0].nInc = pvfe->caps.OutputGranularityY;
	ua[1].nInc = pvfe->caps.OutputGranularityY*2;
	ua[2].nInc = pvfe->caps.OutputGranularityY*4;
	m_vidver.SetAccel(3, ua);

	m_vidhor.SetPos(bih->biWidth);
	m_vidver.SetPos(abs(bih->biHeight));

	CString fps;
	fps.Format(_T("%.4f"), (float)(10000000.0 / ((VIDEOINFOHEADER*)pmt->pbFormat)->AvgTimePerFrame));
	m_vidfpsedit.SetWindowText(fps);

	DeleteMediaType(pmt);
}

void CPlayerCaptureDialog::UpdateVideoCodec()
{
	int iSel = m_vidcodec.GetCurSel();
	if(iSel >= 0) iSel = m_vidcodec.GetItemData(iSel);

	m_pVidEnc = iSel < 0 ? NULL : m_pVidEncArray[iSel].pBF;
	m_pVidEncMoniker = iSel < 0 ? NULL : m_pVidEncArray[iSel].pMoniker;

//	CString DisplayName = iSel < 0 ? _T("") : CString(m_pVidEncArray[iSel].DisplayName.m_str);
	CComQIPtr<IAMStreamConfig> pAMSC = GetFirstPin(m_pVidEnc, PINDIR_OUTPUT);

	SetupMediaTypes(pAMSC, m_vcfa, m_vidcodectype, m_vidcodecdimension, m_mtcv);

	SaveDefaultCodec(m_pVidEncArray, m_vidcodec, CLSID_VideoCompressorCategory);

//	SaveMediaType(DisplayName, &m_mtcv);
}

void CPlayerCaptureDialog::UpdateAudioCodec()
{
	int iSel = m_audcodec.GetCurSel();
	if(iSel >= 0) iSel = m_audcodec.GetItemData(iSel);

	m_pAudEnc = iSel < 0 ? NULL : m_pAudEncArray[iSel].pBF;
	m_pAudEncMoniker = iSel < 0 ? NULL : m_pAudEncArray[iSel].pMoniker;

//	CString DisplayName = iSel < 0 ? _T("") : CString(m_pAudEncArray[iSel].DisplayName.m_str);
	CComQIPtr<IAMStreamConfig> pAMSC = GetFirstPin(m_pAudEnc, PINDIR_OUTPUT); 

	SetupMediaTypes(pAMSC, m_acfa, m_audcodectype, m_audcodecdimension, m_mtca);

	SaveDefaultCodec(m_pAudEncArray, m_audcodec, CLSID_AudioCompressorCategory);

//	SaveMediaType(DisplayName, &m_mtca);
}

void CPlayerCaptureDialog::UpdateMuxer()
{
	m_pMux = NULL;
	m_pAudMux = NULL;

	UpdateData();

	if(m_muxtype == 0)
	{
		m_pMux.CoCreateInstance(CLSID_AviDest);
	}
	else if(m_muxtype == 1)
	{
		m_pMux.CoCreateInstance(CLSID_OggMux);
	}
	else if(m_muxtype == 2)
	{
//		m_pMux.CoCreateInstance(__uuidof(CMatrsokaMuxerFilter));
		HRESULT hr;
		m_pMux = new CMatroskaMuxerFilter(NULL, &hr);
	}
	else
	{
		return;
	}

	if(m_fSepAudio)
	{
/*		if(m_muxtype == 0)
		{
*/			HRESULT hr;
			m_pAudMux = new CWavDestFilter(NULL, &hr);
/*		}
		else if(m_muxtype == 1)
		{
			m_pAudMux.CoCreateInstance(CLSID_OggMux);
		}
		else
		{
			return;
		}
*/	}
}

void CPlayerCaptureDialog::UpdateOutputControls()
{
	UpdateData();

	m_recordbtn.EnableWindow(!m_file.IsEmpty() && (m_pAMVSC && m_fVidOutput || m_pAMASC && m_fAudOutput));
	m_vidcodec.EnableWindow(TRUE);
	m_audcodec.EnableWindow(TRUE);
}

void CPlayerCaptureDialog::UpdateGraph()
{
	UpdateMediaTypes();

//	UpdateMuxer();

	((CMainFrame*)AfxGetMainWnd())->BuildGraphVideoAudio(!!m_fVidPreview, false, !!m_fAudPreview, false);

	UpdateUserDefinableControls();
}

void CPlayerCaptureDialog::EnableControls(CWnd* pWnd, bool fEnable)
{
	if(fEnable)
	{
		for(CWnd* pChild = pWnd->GetWindow(GW_CHILD); pChild; pChild = pChild->GetNextWindow())
		{
			BOOL fEnabled;
			if(m_wndenabledmap.Lookup(pChild->m_hWnd, fEnabled))
				pChild->EnableWindow(fEnabled);
			EnableControls(pChild, fEnable);
		}

		if(pWnd->m_hWnd == m_hWnd) 
			m_wndenabledmap.RemoveAll();

		m_recordbtn.SetWindowText(_T("Record"));
	}
	else
	{
		if(pWnd->m_hWnd == m_hWnd)
			m_wndenabledmap.RemoveAll();

		for(CWnd* pChild = pWnd->GetWindow(GW_CHILD); pChild; pChild = pChild->GetNextWindow())
		{
			m_wndenabledmap[pChild->m_hWnd] = pChild->IsWindowEnabled();
			pChild->EnableWindow(FALSE);
			EnableControls(pChild, fEnable);
		}

		m_recordbtn.EnableWindow(TRUE);
		m_recordbtn.SetWindowText(_T("Stop"));
	}
}

void CPlayerCaptureDialog::SetupVideoControls(
	CStringW DisplayName, 
	CComPtr<IAMStreamConfig> pAMSC, CComPtr<IAMCrossbar> pAMXB, CComPtr<IAMTVTuner> pAMTuner)
{
	EmptyVideo();

	// crossbar

	if(m_pAMXB = pAMXB)
	{
		long OutputPinCount, InputPinCount;
		if(SUCCEEDED(pAMXB->get_PinCounts(&OutputPinCount, &InputPinCount)))
		{
			for(int i = 0; i < InputPinCount; i++)
			{
				long PinIndexRelated, PhysicalType;
				if(FAILED(pAMXB->get_CrossbarPinInfo(TRUE, i, &PinIndexRelated, &PhysicalType)))
					continue;

				if(PhysicalType >= PhysConn_Audio_Tuner)
					continue;

				CString str;
				switch(PhysicalType)
				{
				case PhysConn_Video_Tuner: str = _T("Tuner"); break;
				case PhysConn_Video_Composite: str = _T("Composite"); break;
				case PhysConn_Video_SVideo: str = _T("SVideo"); break;
				case PhysConn_Video_RGB: str = _T("RGB"); break;
				case PhysConn_Video_YRYBY: str = _T("YRYBY"); break;
				case PhysConn_Video_SerialDigital: str = _T("SerialDigital"); break;
				case PhysConn_Video_ParallelDigital: str = _T("ParallelDigital"); break;
				case PhysConn_Video_SCSI: str = _T("SCSI"); break;
				case PhysConn_Video_AUX: str = _T("AUX"); break;
				case PhysConn_Video_1394: str = _T("1394"); break;
				case PhysConn_Video_USB: str = _T("USB"); break;
				case PhysConn_Video_VideoDecoder: str = _T("VideoDecoder"); break;
				case PhysConn_Video_VideoEncoder: str = _T("VideoEncoder"); break;
				case PhysConn_Video_SCART: str = _T("SCART"); break;
				default: str.Format(_T("PhysicalType %d"), PhysicalType); break;
				}

				m_vidinput.SetItemData(m_vidinput.AddString(str), i);
			}
		}	
	}	

	if(m_vidinput.GetCount() > 0)
	{
		m_vidinput.EnableWindow(TRUE);

		long OutputPinCount, InputPinCount;
		if(SUCCEEDED(pAMXB->get_PinCounts(&OutputPinCount, &InputPinCount)))
		{
			for(int i = 0; i < OutputPinCount; i++)
			{
				long InputPinIndex;
				if(S_OK == pAMXB->get_IsRoutedTo(i, &InputPinIndex))
				{
					for(int j = 0; j < m_vidinput.GetCount(); j++)
					{
						if(m_vidinput.GetItemData(j) == InputPinIndex)
						{
							m_vidinput.SetCurSel(j);
							i = OutputPinCount;
							break;
						}
					}
				}
			}
		}
	}

	// tuner

	if(m_pAMTuner = pAMTuner)
	{
		if(!DisplayName.IsEmpty()) // load saved channel
		{
			long lChannel = AfxGetApp()->GetProfileInt(_T("Capture\\") + CString(DisplayName), _T("Channel"), -1);
			if(lChannel >= 0)
				m_pAMTuner->put_Channel(lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
		}

		// TODO:...
	}

	// streamconfig

	if(m_pAMVSC = pAMSC)
	{
		m_VidDisplayName = DisplayName;

		AM_MEDIA_TYPE* pmt;
		if(LoadMediaType(DisplayName, &pmt))
		{
			pAMSC->SetFormat(pmt);
			DeleteMediaType(pmt);
		}

		SetupMediaTypes(pAMSC, m_vfa, m_vidtype, m_viddimension, m_mtv);
	}

	if(m_vidtype.GetCount() > 0)
	{
		m_vidfpsedit.EnableWindow(TRUE);
		m_vidhor.EnableWindow(TRUE);
		m_vidver.EnableWindow(TRUE);
		m_vidsetres.EnableWindow(TRUE);
	}

	{
		m_vidoutput.EnableWindow(TRUE);
		m_vidpreview.EnableWindow(TRUE);
	}

	UpdateMediaTypes();

	UpdateUserDefinableControls();

	UpdateOutputControls();
}

void CPlayerCaptureDialog::SetupVideoControls(
	CStringW DisplayName, 
	CComPtr<IAMStreamConfig> pAMSC, CComPtr<IAMVfwCaptureDialogs> pAMVfwCD)
{
	EmptyVideo();

	if(m_pAMVfwCD = pAMVfwCD)
	{
		if(S_OK == m_pAMVfwCD->HasDialog(VfwCaptureDialog_Source))
			m_vidinput.SetItemData(m_vidinput.AddString(_T("Source")), (DWORD_PTR)VfwCaptureDialog_Source);
		if(S_OK == m_pAMVfwCD->HasDialog(VfwCaptureDialog_Format))
			m_vidinput.SetItemData(m_vidinput.AddString(_T("Format")), (DWORD_PTR)VfwCaptureDialog_Format);
		if(S_OK == m_pAMVfwCD->HasDialog(VfwCaptureDialog_Display))
			m_vidinput.SetItemData(m_vidinput.AddString(_T("Display")), (DWORD_PTR)VfwCaptureDialog_Display);

		if(m_vidinput.GetCount() > 0)
		{
			m_vidinput.EnableWindow(TRUE);
			m_vidinput.SetCurSel(0);
		}
	}

	// streamconfig

	if(m_pAMVSC = pAMSC)
	{
		m_VidDisplayName = DisplayName;

		AM_MEDIA_TYPE* pmt;
		if(LoadMediaType(DisplayName, &pmt))
		{
			pAMSC->SetFormat(pmt);
			DeleteMediaType(pmt);
		}

		SetupMediaTypes(pAMSC, m_vfa, m_vidtype, m_viddimension, m_mtv);
	}

	if(m_vidtype.GetCount() > 0)
	{
		m_vidfpsedit.EnableWindow(TRUE);
		m_vidhor.EnableWindow(TRUE);
		m_vidver.EnableWindow(TRUE);
		m_vidsetres.EnableWindow(TRUE);
	}

	{
		m_vidoutput.EnableWindow(TRUE);
		m_vidpreview.EnableWindow(TRUE);
	}

	UpdateMediaTypes();

	UpdateUserDefinableControls();

	UpdateOutputControls();
}

void CPlayerCaptureDialog::SetupAudioControls(
	CStringW DisplayName, 
	CComPtr<IAMStreamConfig> pAMSC, CInterfaceArray<IAMAudioInputMixer>& pAMAIM)
{
	EmptyAudio();

	// input selection

	if(pAMAIM.GetCount() > 0)
	{
		m_pAMAIM.Copy(pAMAIM);

		int iSel = -1;

		for(int i = 0; i < (int)m_pAMAIM.GetCount(); i++)
		{
			CComQIPtr<IPin> pPin = m_pAMAIM[i];
			CPinInfo pi;
			if(!pPin || FAILED(pPin->QueryPinInfo(&pi)))
				break;

			int j = m_audinput.AddString(CString(pi.achName));
			m_audinput.SetItemData(j, (DWORD_PTR)i);

			BOOL fEnable;
			if(SUCCEEDED(m_pAMAIM[i]->get_Enable(&fEnable)) && fEnable)
				iSel = i;
		}

		if(m_audinput.GetCount() > 0)
		{
			for(int i = 0; i < m_audinput.GetCount(); i++)
			{
				if(m_audinput.GetItemData(i) == iSel)
				{
					m_audinput.SetCurSel(i);
					break;
				}
			}

			m_audinput.EnableWindow(TRUE);
		}
	}

	// stream config

	if(m_pAMASC = pAMSC)
	{
		m_AudDisplayName = DisplayName;

		AM_MEDIA_TYPE* pmt;
		if(LoadMediaType(DisplayName, &pmt))
		{
			pAMSC->SetFormat(pmt);
			DeleteMediaType(pmt);
		}

		SetupMediaTypes(pAMSC, m_afa, m_audtype, m_auddimension, m_mta);
	}

//	if(m_audtype.GetCount() > 0)
	{
		m_audoutput.EnableWindow(TRUE);
		m_audpreview.EnableWindow(TRUE);
	}

	UpdateMediaTypes();

	UpdateUserDefinableControls();

	UpdateOutputControls();
}

bool CPlayerCaptureDialog::IsTunerActive()
{
	int iSel = m_vidinput.GetCurSel();
	if(iSel < 0) return(false);
	iSel = m_vidinput.GetItemData(iSel);
	if(iSel < 0) return(false);

	long PinIndexRelated, PhysicalType;
	return(m_pAMXB
		&& SUCCEEDED(m_pAMXB->get_CrossbarPinInfo(TRUE, iSel, &PinIndexRelated, &PhysicalType))
		&& PhysicalType == PhysConn_Video_Tuner);
}

BEGIN_MESSAGE_MAP(CPlayerCaptureDialog, CDialog)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO4, OnVideoInput)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnVideoType)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnVideoDimension)
	ON_BN_CLICKED(IDC_BUTTON1, OnOverrideVideoDimension)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnAudioInput)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnAudioType)
	ON_CBN_SELCHANGE(IDC_COMBO6, OnAudioDimension)
	ON_BN_CLICKED(IDC_CHECK2, OnRecordVideo)
	ON_CBN_SELCHANGE(IDC_COMBO7, OnVideoCodec)
	ON_CBN_SELCHANGE(IDC_COMBO9, OnVideoCodecType)
	ON_CBN_SELCHANGE(IDC_COMBO10, OnVideoCodecDimension)
	ON_BN_CLICKED(IDC_CHECK3, OnRecordAudio)
	ON_CBN_SELCHANGE(IDC_COMBO8, OnAudioCodec)
	ON_CBN_SELCHANGE(IDC_COMBO12, OnAudioCodecType)
	ON_CBN_SELCHANGE(IDC_COMBO11, OnAudioCodecDimension)
	ON_BN_CLICKED(IDC_BUTTON3, OnOpenFile)
	ON_BN_CLICKED(IDC_RADIO1, OnMuxerType)
	ON_BN_CLICKED(IDC_RADIO2, OnMuxerType)
	ON_BN_CLICKED(IDC_RADIO3, OnMuxerType)
	ON_BN_CLICKED(IDC_BUTTON2, OnRecord)
	ON_EN_CHANGE(IDC_EDIT9, OnEnChangeEdit9)
	ON_EN_CHANGE(IDC_EDIT12, OnEnChangeEdit12)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK4, OnBnClickedVidAudPreview)
	ON_BN_CLICKED(IDC_CHECK5, OnBnClickedVidAudPreview)
	ON_BN_CLICKED(IDC_CHECK7, OnBnClickedCheck7)
END_MESSAGE_MAP()


// CPlayerCaptureDialog message handlers

BOOL CPlayerCaptureDialog::OnInitDialog()
{
	__super::OnInitDialog();

	InitCodecList(m_pVidEncArray, m_vidcodec, CLSID_VideoCompressorCategory);
	UpdateVideoCodec();

	InitCodecList(m_pAudEncArray, m_audcodec, CLSID_AudioCompressorCategory);
	UpdateAudioCodec();

	bool fEnableOgm = IsCLSIDRegistered(_T("{8cae96b7-85b1-4605-b23c-17ff5262b296}"));
	GetDlgItem(IDC_RADIO2)->EnableWindow(fEnableOgm);

	m_nVidBuffers = AfxGetApp()->GetProfileInt(_T("Capture"), _T("VidBuffers"), 50);
	m_nAudBuffers = AfxGetApp()->GetProfileInt(_T("Capture"), _T("AudBuffers"), 50);
	m_fVidOutput = !!AfxGetApp()->GetProfileInt(_T("Capture"), _T("VidOutput"), TRUE);
	m_fAudOutput = !!AfxGetApp()->GetProfileInt(_T("Capture"), _T("AudOutput"), TRUE);
	m_fVidPreview = !!AfxGetApp()->GetProfileInt(_T("Capture"), _T("VidPreview"), TRUE);
	m_fAudPreview = !!AfxGetApp()->GetProfileInt(_T("Capture"), _T("AudPreview"), TRUE);
	m_muxtype = AfxGetApp()->GetProfileInt(_T("Capture"), _T("FileFormat"), 0);
	m_file = AfxGetApp()->GetProfileString(_T("Capture"), _T("FileName"), _T(""));
	m_fSepAudio = AfxGetApp()->GetProfileInt(_T("Capture"), _T("SepAudio"), TRUE);

//	UpdateMuxer();

	UpdateData(FALSE);

	OnMuxerType();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPlayerCaptureDialog::OnDestroy()
{
	UpdateData();

	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("VidOutput"), m_fVidOutput);
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("AudOutput"), m_fAudOutput);
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("VidPreview"), m_fVidPreview);
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("AudPreview"), m_fAudPreview);
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("FileFormat"), m_muxtype);
	AfxGetApp()->WriteProfileString(_T("Capture"), _T("FileName"), m_file);
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("SepAudio"), m_fSepAudio);

	__super::OnDestroy();
}

void CPlayerCaptureDialog::OnVideoInput()
{
	int iSel = m_vidinput.GetCurSel();
	if(iSel < 0) return;
	iSel = m_vidinput.GetItemData(iSel);
	if(iSel < 0) return;

	if(m_pAMXB)
	{
		long PinIndexRelated, PhysicalType;
		if(FAILED(m_pAMXB->get_CrossbarPinInfo(TRUE, iSel, &PinIndexRelated, &PhysicalType)))
			return;

		long OutputPinCount, InputPinCount;
		if(FAILED(m_pAMXB->get_PinCounts(&OutputPinCount, &InputPinCount)))
			return;

		for(int i = 0; i < OutputPinCount; i++)
		{
			if(S_OK == m_pAMXB->CanRoute(i, iSel))
			{
				m_pAMXB->Route(i, iSel);
				break;
			}
		}

		if(PinIndexRelated >= 0)
		{
			for(int i = 0; i < OutputPinCount; i++)
			{
				if(S_OK == m_pAMXB->CanRoute(i, PinIndexRelated))
				{
					m_pAMXB->Route(i, PinIndexRelated);
					break;
				}
			}
		}
	}
	else if(m_pAMVfwCD)
	{
		if(S_OK == m_pAMVfwCD->HasDialog(iSel))
		{
			HRESULT hr = m_pAMVfwCD->ShowDialog(iSel, m_hWnd);

			if(VFW_E_NOT_STOPPED == hr)
			{
				((CMainFrame*)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_PLAY_STOP);
				hr = m_pAMVfwCD->ShowDialog(iSel, m_hWnd);
				((CMainFrame*)AfxGetMainWnd())->SendMessage(WM_COMMAND, ID_PLAY_PLAY);
			}

			if(VFW_E_CANNOT_CONNECT == hr)
				UpdateGraph();
		}
	}
}

void CPlayerCaptureDialog::OnVideoType()
{
	if(SetupDimension(m_vfa, m_vidtype, m_viddimension))
		OnVideoDimension();
}

void CPlayerCaptureDialog::OnVideoDimension()
{
	int iSel = m_viddimension.GetCurSel();
	if(iSel < 0) return;

	CVidFormatElem* pvfe = (CVidFormatElem*)m_viddimension.GetItemData(iSel);
	if(!pvfe) return;

	BITMAPINFOHEADER* bih = (pvfe->pmt->formattype == FORMAT_VideoInfo)
		? &((VIDEOINFOHEADER*)pvfe->pmt->pbFormat)->bmiHeader
		: (pvfe->pmt->formattype == FORMAT_VideoInfo2)
		? &((VIDEOINFOHEADER2*)pvfe->pmt->pbFormat)->bmiHeader
		: NULL;

	m_vidhor.SetRange(0, 32767);
	m_vidver.SetRange(0, 32767);
	m_vidhor.SetPos(bih->biWidth);
	m_vidver.SetPos(abs(bih->biHeight));
	CString fps;
	fps.Format(_T("%.4f"), (float)(10000000.0 / ((VIDEOINFOHEADER*)pvfe->pmt->pbFormat)->AvgTimePerFrame));
	m_vidfpsedit.SetWindowText(fps);

	UpdateGraph();
}

void CPlayerCaptureDialog::OnOverrideVideoDimension()
{
	UpdateGraph();
}

void CPlayerCaptureDialog::OnAudioInput()
{
	int iSel = m_audinput.GetCurSel();

	for(int i = 0; i < (int)m_pAMAIM.GetCount(); i++)
	{
		m_pAMAIM[m_audinput.GetItemData(i)]->put_Enable(i == iSel ? TRUE : FALSE);
	}
}

void CPlayerCaptureDialog::OnAudioType()
{
	if(SetupDimension(m_afa, m_audtype, m_auddimension))
		OnAudioDimension();
}

void CPlayerCaptureDialog::OnAudioDimension()
{
	UpdateGraph();
}

void CPlayerCaptureDialog::OnRecordVideo()
{
	UpdateOutputControls();
}

void CPlayerCaptureDialog::OnVideoCodec()
{
	ShowPPage(m_pVidEncArray, m_vidcodec, m_hWnd);
	UpdateVideoCodec();
}

void CPlayerCaptureDialog::OnVideoCodecType()
{
	if(SetupDimension(m_vcfa, m_vidcodectype, m_vidcodecdimension))
		OnVideoCodecDimension();
}

void CPlayerCaptureDialog::OnVideoCodecDimension()
{
	int i = m_vidcodecdimension.GetCurSel();
	if(i >= 0)
	{
		CopyMediaType(&m_mtcv, ((CVidFormatElem*)m_vidcodecdimension.GetItemData(i))->pmt);

		// we have to recreate the encoder, otherwise it will accept the new media type for only the first time
		m_pVidEnc = NULL;
		m_pVidEncMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pVidEnc);
	}
}

void CPlayerCaptureDialog::OnRecordAudio()
{
	UpdateOutputControls();
}

void CPlayerCaptureDialog::OnAudioCodec()
{
	ShowPPage(m_pAudEncArray, m_audcodec, m_hWnd);
	UpdateAudioCodec();
}

void CPlayerCaptureDialog::OnAudioCodecType()
{
	if(SetupDimension(m_acfa, m_audcodectype, m_audcodecdimension))
		OnAudioCodecDimension();
}

void CPlayerCaptureDialog::OnAudioCodecDimension()
{
	int i = m_audcodecdimension.GetCurSel();
	if(i >= 0)
	{
		CopyMediaType(&m_mtca, ((CAudFormatElem*)m_audcodecdimension.GetItemData(i))->pmt);

		// we have to recreate the encoder, otherwise it will accept the new media type for only the first time
		m_pAudEnc = NULL;
		m_pAudEncMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pAudEnc);
/*
		SaveMediaType(
			CString(m_pAudEncArray[m_audcodec.GetItemData(m_audcodec.GetCurSel())].DisplayName.m_str), 
			&m_mtca);
*/	}
}

void CPlayerCaptureDialog::OnOpenFile()
{
	CFileDialog fd(FALSE, NULL, NULL, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, 
		_T("Media files (*.avi,*.ogm)|*.avi;*.ogm|"), this, 0);

	if(fd.DoModal() == IDOK)
	{
		CString str = fd.GetPathName();

		CString ext = str.Mid(str.ReverseFind('.')+1).MakeLower();
		if(ext == _T("avi")) m_muxtype = 0;
		else if(ext == _T("ogm")) m_muxtype = 1;
		else if(ext == _T("mkv")) m_muxtype = 2;
		else
		{
			if(m_muxtype == 0) str += _T(".avi");
			else if(m_muxtype == 1) str += _T(".ogm");
			else if(m_muxtype == 2) str += _T(".mkv");
		}

		m_file = str;

		UpdateData(FALSE);
	}

	UpdateOutputControls();
}

void CPlayerCaptureDialog::OnMuxerType()
{
//	UpdateMuxer();

	UpdateData();

	CString ext = m_file.Mid(m_file.ReverseFind('.')+1).MakeLower();

	if(m_muxtype == 0 && (ext == _T("ogm") || ext == _T("mkv")))
	{
		m_file = m_file.Left(m_file.GetLength()-4) + _T(".avi");
	}
	else if(m_muxtype == 1 && (ext == _T("avi") || ext == _T("mkv")))
	{
		m_file = m_file.Left(m_file.GetLength()-4) + _T(".ogm");
	}
	else if(m_muxtype == 2 && (ext == _T("avi") || ext == _T("ogm")))
	{
		m_file = m_file.Left(m_file.GetLength()-4) + _T(".mkv");
	}

	GetDlgItem(IDC_EDIT9)->EnableWindow(m_muxtype != 2);
	GetDlgItem(IDC_EDIT12)->EnableWindow(m_muxtype != 2);

	UpdateData(FALSE);
}

void CPlayerCaptureDialog::OnRecord()
{
	UpdateData();

	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if(!pFrame) return;

	if(!pFrame->m_fCapturing)
	{
		UpdateMuxer();

		CComQIPtr<IFileSinkFilter2> pFSF = m_pMux;
		if(pFSF)
		{
			m_pDst = m_pMux;
		}
		else
		{
			m_pDst = NULL;
			m_pDst.CoCreateInstance(CLSID_FileWriter);
			pFSF = m_pDst;
		}

		if(!pFSF
		|| FAILED(pFSF->SetFileName(CStringW(m_file), NULL))
		|| FAILED(pFSF->SetMode(AM_FILE_OVERWRITE)))
		{
			AfxMessageBox(_T("Error initializing the output file"));
			return;
		}

		CString audfn = m_file.Left(m_file.ReverseFind('.')+1);
		if(m_fSepAudio && m_fAudOutput && m_pAudMux && !audfn.IsEmpty())
		{
//			if(m_muxtype == 0) audfn += _T("wav");
//			else if(m_muxtype == 1) audfn += _T("ogg");
//			else if(m_muxtype == 2) audfn += _T("mkv");
			audfn += _T("wav");

			CComQIPtr<IFileSinkFilter2> pFSF = m_pAudMux;
			if(pFSF)
			{
				m_pAudDst = m_pAudMux;
			}
			else
			{
				m_pAudDst = NULL;
				m_pAudDst.CoCreateInstance(CLSID_FileWriter);
				pFSF = m_pAudDst;
			}

			if(!pFSF
			|| FAILED(pFSF->SetFileName(CStringW(audfn), NULL))
			|| FAILED(pFSF->SetMode(AM_FILE_OVERWRITE)))
			{
				AfxMessageBox(_T("Error initializing the audio output file"));
				return;
			}
		}

		m_pVidBuffer = m_fVidOutput && m_nVidBuffers > 0 && m_muxtype != 2 ? new CBufferFilter(NULL, NULL) : NULL;
		if(CComQIPtr<IBufferFilter> pVB = m_pVidBuffer)
			{pVB->SetBuffers(m_nVidBuffers); pVB->SetPriority(THREAD_PRIORITY_NORMAL);}

		m_pAudBuffer = m_fAudOutput && m_nAudBuffers > 0 && m_muxtype != 2 ? new CBufferFilter(NULL, NULL) : NULL;
		if(CComQIPtr<IBufferFilter> pAB = m_pAudBuffer)
			{pAB->SetBuffers(m_nAudBuffers); pAB->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);}

		EnableControls(this, false);

		pFrame->StartCapture();

		SetTimer(1, 100, NULL);
	}
	else
	{
		KillTimer(1);

		pFrame->StopCapture();
/*
		{
			if(FILE* f = _tfopen(m_file, _T("rb+")))
			{
				fseek(f, 0x20, SEEK_SET);
				unsigned short mspf = (unsigned short)(((VIDEOINFOHEADER*)m_mtv.pbFormat)->AvgTimePerFrame / 10);
				fwrite(&mspf, 1, 2, f);
				fclose(f);
			}
		}
*/
		EnableControls(this, true);

		m_pVidBuffer = NULL;
		m_pAudBuffer = NULL;
	}
}

void CPlayerCaptureDialog::OnEnChangeEdit9()
{
	UpdateData();
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("VidBuffers"), max(m_nVidBuffers, 0));
}

void CPlayerCaptureDialog::OnEnChangeEdit12()
{
	UpdateData();
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("AudBuffers"), max(m_nAudBuffers, 0));
}

void CPlayerCaptureDialog::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == 1)
	{
		if(((CMainFrame*)AfxGetMainWnd())->m_fCapturing)
		{
			ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
			if(GetDiskFreeSpaceEx(m_file.Left(m_file.ReverseFind('\\')+1), &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)
			&& FreeBytesAvailable.QuadPart < 1024i64*1024*10)
			{
				OnRecord();
			}
		}
	}

	__super::OnTimer(nIDEvent);
}

void CPlayerCaptureDialog::OnBnClickedVidAudPreview()
{
	UpdateData();
	UpdateGraph();
}

void CPlayerCaptureDialog::OnBnClickedCheck7()
{
//	UpdateMuxer();
}