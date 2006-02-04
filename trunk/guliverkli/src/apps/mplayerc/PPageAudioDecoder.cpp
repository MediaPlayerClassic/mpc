/* 
 *	Copyright (C) 2003-2006 Gabest
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

// PPageAudioDecoder.cpp : implementation file
//

#include "stdafx.h"
#include <math.h>
#include "mplayerc.h"
#include "PPageAudioDecoder.h"
#include ".\ppageaudiodecoder.h"

// CPPageAudioDecoder dialog

IMPLEMENT_DYNAMIC(CPPageAudioDecoder, CPPageBase)
CPPageAudioDecoder::CPPageAudioDecoder(IFilterGraph* pFG)
	: CPPageBase(CPPageAudioDecoder::IDD, CPPageAudioDecoder::IDD)
	, m_iSampleFormat(0)
	, m_fNormalize(FALSE)
	, m_fAc3SpeakerConfig(FALSE)
	, m_iAc3SpeakerConfig(0)
	, m_fAc3SpeakerConfigLFE(FALSE)
	, m_fAc3DynamicRangeControl(FALSE)
	, m_fDtsSpeakerConfig(FALSE)
	, m_iDtsSpeakerConfig(0)
	, m_fDtsSpeakerConfigLFE(FALSE)
	, m_fDtsDynamicRangeControl(FALSE)
	, m_iAacSpeakerConfig(0)
	, m_boost(0)
{
	BeginEnumFilters(pFG, pEF, pBF)
		if(CComQIPtr<IMpaDecFilter> pMpaDecFilter = pBF)
			m_pMDFs.AddTail(pMpaDecFilter);
	EndEnumFilters
}

CPPageAudioDecoder::~CPPageAudioDecoder()
{
}

void CPPageAudioDecoder::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, m_iSampleFormat);
	DDX_Check(pDX, IDC_CHECK3, m_fNormalize);
	DDX_Radio(pDX, IDC_RADIO5, m_fAc3SpeakerConfig);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iAc3SpeakerConfig);
	DDX_Check(pDX, IDC_CHECK1, m_fAc3SpeakerConfigLFE);
	DDX_Check(pDX, IDC_CHECK2, m_fAc3DynamicRangeControl);
	DDX_Control(pDX, IDC_COMBO1, m_ac3sclist);
	DDX_Radio(pDX, IDC_RADIO7, m_fDtsSpeakerConfig);
	DDX_CBIndex(pDX, IDC_COMBO2, m_iDtsSpeakerConfig);
	DDX_Check(pDX, IDC_CHECK6, m_fDtsSpeakerConfigLFE);
	DDX_Check(pDX, IDC_CHECK4, m_fDtsDynamicRangeControl);
	DDX_Control(pDX, IDC_COMBO2, m_dtssclist);
	DDX_Check(pDX, IDC_CHECK5, m_iAacSpeakerConfig);
	DDX_Slider(pDX, IDC_SLIDER1, m_boost);
	DDX_Control(pDX, IDC_SLIDER1, m_boostctrl);
}


BEGIN_MESSAGE_MAP(CPPageAudioDecoder, CPPageBase)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CPPageAudioDecoder message handlers

BOOL CPPageAudioDecoder::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_iSampleFormat = s.mpasf;
	m_fNormalize = s.mpanormalize;
	m_fAc3SpeakerConfig = s.ac3sc < 0;
	m_iAc3SpeakerConfig = A52_STEREO;
	m_fAc3SpeakerConfigLFE = !!(abs(s.ac3sc)&A52_LFE);
	m_fAc3DynamicRangeControl = s.ac3drc;
	m_fDtsSpeakerConfig = s.dtssc < 0;
	m_iDtsSpeakerConfig = DTS_STEREO;
	m_fDtsSpeakerConfigLFE = !!(abs(s.dtssc)&DTS_LFE);
	m_fDtsDynamicRangeControl = s.dtsdrc;
	m_iAacSpeakerConfig = s.aacsc; // FIXME
	m_boost = (int)(50.0f*log10(s.mpaboost));
	m_boostctrl.SetRange(0, 100);

	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("Mono")), A52_MONO);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("Dual Mono")), A52_CHANNEL);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("Stereo")), A52_STEREO);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("Dolby Stereo")), A52_DOLBY);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("3 Front")), A52_3F);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("2 Front + 1 Rear")), A52_2F1R);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("3 Front + 1 Rear")), A52_3F1R);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("2 Front + 2 Rear")), A52_2F2R);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("3 Front + 2 Rear")), A52_3F2R);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("Channel 1")), A52_CHANNEL1);
	m_ac3sclist.SetItemData(m_ac3sclist.AddString(_T("Channel 2")), A52_CHANNEL2);

	for(int i = 0, j = abs(s.ac3sc)&A52_CHANNEL_MASK; i < m_ac3sclist.GetCount(); i++)
	{
		if(m_ac3sclist.GetItemData(i) == j)
		{
			m_iAc3SpeakerConfig = i;
			break;
		}
	}

	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("Mono")), DTS_MONO);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("Dual Mono")), DTS_CHANNEL);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("Stereo")), DTS_STEREO);
	//m_dtssclist.SetItemData(m_dtssclist.AddString(_T("Stereo ..")), DTS_STEREO_SUMDIFF);
	//m_dtssclist.SetItemData(m_dtssclist.AddString(_T("Stereo ..")), DTS_STEREO_TOTAL);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("3 Front")), DTS_3F);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("2 Front + 1 Rear")), DTS_2F1R);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("3 Front + 1 Rear")), DTS_3F1R);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("2 Front + 2 Rear")), DTS_2F2R);
	m_dtssclist.SetItemData(m_dtssclist.AddString(_T("3 Front + 2 Rear")), DTS_3F2R);

	for(int i = 0, j = abs(s.dtssc)&DTS_CHANNEL_MASK; i < m_dtssclist.GetCount(); i++)
	{
		if(m_dtssclist.GetItemData(i) == j)
		{
			m_iDtsSpeakerConfig = i;
			break;
		}
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageAudioDecoder::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.mpasf = m_iSampleFormat;
	s.mpanormalize = !!m_fNormalize;
	s.ac3sc = m_ac3sclist.GetItemData(m_ac3sclist.GetCurSel());
	s.ac3sc |= m_fAc3SpeakerConfigLFE?A52_LFE:0;
	s.ac3sc *= m_fAc3SpeakerConfig?-1:1;
	s.ac3drc = !!m_fAc3DynamicRangeControl;
	s.dtssc = m_dtssclist.GetItemData(m_dtssclist.GetCurSel());
	s.dtssc |= m_fDtsSpeakerConfigLFE?DTS_LFE:0;
	s.dtssc *= m_fDtsSpeakerConfig?-1:1;
	s.dtsdrc = !!m_fDtsDynamicRangeControl;
	s.aacsc = !!m_iAacSpeakerConfig;
	s.mpaboost = (float)pow(10.0, (double)m_boost/50);

	POSITION pos = m_pMDFs.GetHeadPosition();
	while(pos)
	{
		CComPtr<IMpaDecFilter> pMpaDecFilter = m_pMDFs.GetNext(pos);
		pMpaDecFilter->SetSampleFormat((SampleFormat)s.mpasf);
		pMpaDecFilter->SetNormalize(s.mpanormalize);
		pMpaDecFilter->SetSpeakerConfig(IMpaDecFilter::ac3, s.ac3sc);
		pMpaDecFilter->SetDynamicRangeControl(IMpaDecFilter::ac3, s.ac3drc);
		pMpaDecFilter->SetSpeakerConfig(IMpaDecFilter::dts, s.dtssc);
		pMpaDecFilter->SetDynamicRangeControl(IMpaDecFilter::dts, s.dtsdrc);
		pMpaDecFilter->SetSpeakerConfig(IMpaDecFilter::aac, s.aacsc);
		pMpaDecFilter->SetBoost(s.mpaboost);
	}

	return __super::OnApply();
}

void CPPageAudioDecoder::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SetModified();

	CPPageBase::OnHScroll(nSBCode, nPos, pScrollBar);
}
