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

// CPPageMpegDecoder.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "MainFrm.h"
#include "PPageMpegDecoder.h"
#include "..\..\DSUtil\DSUtil.h"
#include "..\..\filters\transform\Mpeg2DecFilter\Mpeg2DecFilter.h"
#include ".\ppagempegdecoder.h"

// CPPageMpegDecoder dialog

IMPLEMENT_DYNAMIC(CPPageMpegDecoder, CPPageBase)
CPPageMpegDecoder::CPPageMpegDecoder(IFilterGraph* pFG)
	: CPPageBase(CPPageMpegDecoder::IDD, CPPageMpegDecoder::IDD)
	, m_fForcedSubs(FALSE)
	, m_fPlanarYUV(FALSE)
{
	m_pMpeg2DecFilter = FindFilter(__uuidof(CMpeg2DecFilter), pFG);
}

CPPageMpegDecoder::~CPPageMpegDecoder()
{
}

void CPPageMpegDecoder::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO2, m_dilist);
	DDX_Control(pDX, IDC_SLIDER1, m_brightctrl);
	DDX_Control(pDX, IDC_SLIDER2, m_contctrl);
	DDX_Control(pDX, IDC_SLIDER3, m_huectrl);
	DDX_Control(pDX, IDC_SLIDER4, m_satctrl);
	DDX_Check(pDX, IDC_CHECK1, m_fForcedSubs);
	DDX_Check(pDX, IDC_CHECK2, m_fPlanarYUV);
}


BEGIN_MESSAGE_MAP(CPPageMpegDecoder, CPPageBase)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_CHECK1, OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_CHECK2, OnBnClickedCheck2)
END_MESSAGE_MAP()


// CPPageMpegDecoder message handlers

BOOL CPPageMpegDecoder::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_dilist.SetItemData(m_dilist.AddString(_T("Auto")), 0);
	m_dilist.SetItemData(m_dilist.AddString(_T("Weave")), 1);
	m_dilist.SetItemData(m_dilist.AddString(_T("Blend")), 2);
	m_dilist.SetCurSel(s.mpegdi);
	m_brightctrl.SetRange(0, 2*128);
	m_brightctrl.SetTic(128);
	m_brightctrl.SetPos((int)(s.mpegbright)+128);
	m_contctrl.SetRange(0, 200);
	m_contctrl.SetTic(100);
	m_contctrl.SetPos((int)(100*s.mpegcont));
	m_huectrl.SetRange(0, 2*180);
	m_huectrl.SetTic(180);
	m_huectrl.SetPos((int)(s.mpeghue)+180);
	m_satctrl.SetRange(0, 200);
	m_satctrl.SetTic(100);
	m_satctrl.SetPos((int)(100*s.mpegsat));
	m_fForcedSubs = s.mpegforcedsubs;
	m_fPlanarYUV = s.mpegplanaryuv;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageMpegDecoder::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.mpegdi = m_dilist.GetCurSel();
	s.mpegbright = (double)m_brightctrl.GetPos()-128;
	s.mpegcont = (double)m_contctrl.GetPos() / 100;
	s.mpeghue = (double)m_huectrl.GetPos()-180;
	s.mpegsat = (double)m_satctrl.GetPos() / 100;
	s.mpegforcedsubs = !!m_fForcedSubs;
	s.mpegplanaryuv = !!m_fPlanarYUV;

	if(m_pMpeg2DecFilter)
	{
		m_pMpeg2DecFilter->SetDeinterlaceMethod((ditype)s.mpegdi);
		m_pMpeg2DecFilter->SetBrightness(s.mpegbright);
		m_pMpeg2DecFilter->SetContrast(s.mpegcont);
		m_pMpeg2DecFilter->SetHue(s.mpeghue);
		m_pMpeg2DecFilter->SetSaturation(s.mpegsat);
		m_pMpeg2DecFilter->EnableForcedSubtitles(s.mpegforcedsubs);
		m_pMpeg2DecFilter->EnablePlanarYUV(s.mpegplanaryuv);
	}

	return __super::OnApply();
}

void CPPageMpegDecoder::OnCbnSelchangeCombo2()
{
	SetModified();

	if(m_pMpeg2DecFilter)
		m_pMpeg2DecFilter->SetDeinterlaceMethod((ditype)m_dilist.GetCurSel());
}

void CPPageMpegDecoder::OnCbnSelchangeCombo1()
{
	SetModified();
}

void CPPageMpegDecoder::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SetModified();

	if(m_pMpeg2DecFilter)
	{
		if(*pScrollBar == m_brightctrl)
			m_pMpeg2DecFilter->SetBrightness((double)m_brightctrl.GetPos()-128);
		else if(*pScrollBar == m_contctrl)
			m_pMpeg2DecFilter->SetContrast((double)m_contctrl.GetPos() / 100);
		else if(*pScrollBar == m_huectrl)
			m_pMpeg2DecFilter->SetHue((double)m_huectrl.GetPos()-180);
		else if(*pScrollBar == m_satctrl)
			m_pMpeg2DecFilter->SetSaturation((double)m_satctrl.GetPos() / 100);
	}

	CPPageBase::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageMpegDecoder::OnBnClickedCheck1()
{
	SetModified();

	UpdateData();

	if(m_pMpeg2DecFilter)
		m_pMpeg2DecFilter->EnableForcedSubtitles(!!m_fForcedSubs);
}

void CPPageMpegDecoder::OnBnClickedButton1()
{
	SetModified();

	m_brightctrl.SetPos(128);
	m_contctrl.SetPos(100);
	m_huectrl.SetPos(180);
	m_satctrl.SetPos(100);

	if(m_pMpeg2DecFilter)
	{
		m_pMpeg2DecFilter->SetBrightness((double)m_brightctrl.GetPos()-128);
		m_pMpeg2DecFilter->SetContrast((double)m_contctrl.GetPos() / 100);
		m_pMpeg2DecFilter->SetHue((double)m_huectrl.GetPos()-180);
		m_pMpeg2DecFilter->SetSaturation((double)m_satctrl.GetPos() / 100);
	}
}

void CPPageMpegDecoder::OnBnClickedCheck2()
{
	SetModified();

	UpdateData();

	if(m_pMpeg2DecFilter)
		m_pMpeg2DecFilter->EnablePlanarYUV(!!m_fPlanarYUV);
}
