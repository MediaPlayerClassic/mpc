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

// PPageAudioDecoder.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageAudioDecoder.h"

// CPPageAudioDecoder dialog

IMPLEMENT_DYNAMIC(CPPageAudioDecoder, CPPageBase)
CPPageAudioDecoder::CPPageAudioDecoder(IFilterGraph* pFG)
	: CPPageBase(CPPageAudioDecoder::IDD, CPPageAudioDecoder::IDD)
	, m_iSampleFormat(0)
	, m_fSpeakerConfig(FALSE)
	, m_iSpeakerConfig(0)
	, m_fSpeakerConfigLFE(FALSE)
	, m_fDynamicRangeControl(FALSE)
	, m_fNormalize(FALSE)
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
	DDX_Radio(pDX, IDC_RADIO5, m_fSpeakerConfig);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iSpeakerConfig);
	DDX_Check(pDX, IDC_CHECK1, m_fSpeakerConfigLFE);
	DDX_Check(pDX, IDC_CHECK2, m_fDynamicRangeControl);
	DDX_Control(pDX, IDC_COMBO1, m_sclist);
	DDX_Check(pDX, IDC_CHECK3, m_fNormalize);
}


BEGIN_MESSAGE_MAP(CPPageAudioDecoder, CPPageBase)
END_MESSAGE_MAP()


// CPPageAudioDecoder message handlers

BOOL CPPageAudioDecoder::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_iSampleFormat = s.mpasf;
	m_fSpeakerConfig = s.mpasc < 0;
	m_iSpeakerConfig = A52_STEREO;
	m_fSpeakerConfigLFE = !!(abs(s.mpasc)&A52_LFE);
	m_fDynamicRangeControl = s.mpadrc;
	m_fNormalize = s.mpanormalize;

	m_sclist.SetItemData(m_sclist.AddString(_T("Mono")), A52_MONO);
	m_sclist.SetItemData(m_sclist.AddString(_T("Dual Mono")), A52_CHANNEL);
	m_sclist.SetItemData(m_sclist.AddString(_T("Stereo")), A52_STEREO);
	m_sclist.SetItemData(m_sclist.AddString(_T("Dolby Stereo")), A52_DOLBY);
	m_sclist.SetItemData(m_sclist.AddString(_T("3 Front")), A52_3F);
	m_sclist.SetItemData(m_sclist.AddString(_T("2 Front + 1 Rear")), A52_2F1R);
	m_sclist.SetItemData(m_sclist.AddString(_T("3 Front + 1 Rear")), A52_3F1R);
	m_sclist.SetItemData(m_sclist.AddString(_T("2 Front + 2 Rear")), A52_2F2R);
	m_sclist.SetItemData(m_sclist.AddString(_T("3 Front + 2 Rear")), A52_3F2R);
	m_sclist.SetItemData(m_sclist.AddString(_T("Channel 1")), A52_CHANNEL1);
	m_sclist.SetItemData(m_sclist.AddString(_T("Channel 2")), A52_CHANNEL2);

	for(int i = 0, j = abs(s.mpasc)&A52_CHANNEL_MASK; i < m_sclist.GetCount(); i++)
	{
		if(m_sclist.GetItemData(i) == j)
		{
			m_iSpeakerConfig = i;
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
	s.mpasc = m_sclist.GetItemData(m_sclist.GetCurSel());
	s.mpasc |= m_fSpeakerConfigLFE?A52_LFE:0;
	s.mpasc *= m_fSpeakerConfig?-1:1;
	s.mpadrc = !!m_fDynamicRangeControl;
	s.mpanormalize = !!m_fNormalize;

	POSITION pos = m_pMDFs.GetHeadPosition();
	while(pos)
	{
		CComPtr<IMpaDecFilter> pMpaDecFilter = m_pMDFs.GetNext(pos);
		pMpaDecFilter->SetSampleFormat((SampleFormat)s.mpasf);
		pMpaDecFilter->SetNormalize(s.mpanormalize);
		pMpaDecFilter->SetSpeakerConfig(s.mpasc);
		pMpaDecFilter->SetDynamicRangeControl(s.mpadrc);
	}

	return __super::OnApply();
}
