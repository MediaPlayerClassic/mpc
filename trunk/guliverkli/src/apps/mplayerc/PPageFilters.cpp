

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

// PPageFilters.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFilters.h"
#include "..\..\filters\filters.h"

// CPPageFilters dialog

IMPLEMENT_DYNAMIC(CPPageFilters, CPPageBase)
CPPageFilters::CPPageFilters()
	: CPPageBase(CPPageFilters::IDD, CPPageFilters::IDD)
	, m_cdda(FALSE)
	, m_cdxa(FALSE)
	, m_vts(FALSE)
	, m_flic(FALSE)
	, m_dvd2avi(FALSE)
	, m_dtsac3(FALSE)
	, m_shoutcast(FALSE)
	, m_avi(FALSE)
	, m_matroska(FALSE)
	, m_realmedia(FALSE)
	, m_realvideo(FALSE)
	, m_realaudio(FALSE)
	, m_mpeg(FALSE)
	, m_mpeg1(FALSE)
	, m_mpeg2(FALSE)
	, m_mpa(FALSE)
	, m_radgt(FALSE)
	, m_ogg(FALSE)
	, m_lpcm(FALSE)
	, m_ac3(FALSE)
	, m_dts(FALSE)
	, m_aac(FALSE)
	, m_nut(FALSE)
	, m_diracsplitter(FALSE)
	, m_diracvideo(FALSE)
{
}

CPPageFilters::~CPPageFilters()
{
}

void CPPageFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_CDDA, m_cdda);
	DDX_Check(pDX, IDC_CHECK_CDXA, m_cdxa);
	DDX_Check(pDX, IDC_CHECK_VTS, m_vts);
	DDX_Check(pDX, IDC_CHECK_FLIC, m_flic);
	DDX_Check(pDX, IDC_CHECK_D2V, m_dvd2avi);
	DDX_Check(pDX, IDC_CHECK_DTSAC3, m_dtsac3);
	DDX_Check(pDX, IDC_CHECK_MATROSKA, m_matroska);
	DDX_Check(pDX, IDC_CHECK_SHOUTCAST, m_shoutcast);
	DDX_Check(pDX, IDC_CHECK_RM, m_realmedia);
	DDX_Check(pDX, IDC_CHECK_AVI, m_avi);
	DDX_Check(pDX, IDC_CHECK_RV, m_realvideo);
	DDX_Check(pDX, IDC_CHECK_RA, m_realaudio);
	DDX_Check(pDX, IDC_CHECK_MPG, m_mpeg);
	DDX_Check(pDX, IDC_CHECK_MPG1, m_mpeg1);
	DDX_Check(pDX, IDC_CHECK_MPG2, m_mpeg2);
	DDX_Check(pDX, IDC_CHECK_MPA, m_mpa);
	DDX_Check(pDX, IDC_CHECK_RADGT, m_radgt);
	DDX_Check(pDX, IDC_CHECK_ROQ, m_roq);
	DDX_Check(pDX, IDC_CHECK_OGG, m_ogg);
	DDX_Check(pDX, IDC_CHECK_LPCM, m_lpcm);
	DDX_Check(pDX, IDC_CHECK_AC3, m_ac3);
	DDX_Check(pDX, IDC_CHECK_DTS, m_dts);
	DDX_Check(pDX, IDC_CHECK_AAC, m_aac);
	DDX_Check(pDX, IDC_CHECK_NUT, m_nut);
	DDX_Check(pDX, IDC_CHECK_DIRAC_SPLITTER, m_diracsplitter);
	DDX_Check(pDX, IDC_CHECK_DIRAC_DECODER, m_diracvideo);
}

BEGIN_MESSAGE_MAP(CPPageFilters, CPPageBase)
END_MESSAGE_MAP()


// CPPageFilters message handlers

BOOL CPPageFilters::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_cdda = !!(s.SrcFilters&SRC_CDDA);
	m_cdxa = !!(s.SrcFilters&SRC_CDXA);
	m_vts = !!(s.SrcFilters&SRC_VTS);
	m_flic = !!(s.SrcFilters&SRC_FLIC);
	m_dvd2avi = !!(s.SrcFilters&SRC_DVD2AVI);
	m_dtsac3 = !!(s.SrcFilters&SRC_DTSAC3);
	m_shoutcast = !!(s.SrcFilters&SRC_SHOUTCAST);
	m_avi = !!(s.SrcFilters&SRC_AVI);
	m_matroska = !!(s.SrcFilters&SRC_MATROSKA);
	m_realmedia = !!(s.SrcFilters&SRC_REALMEDIA);
	m_realvideo = !!(s.TraFilters&TRA_REALVID);
	m_realaudio = !!(s.TraFilters&TRA_REALAUD);
	m_mpeg = !!(s.SrcFilters&SRC_MPEG);
	m_mpeg1 = !!(s.TraFilters&TRA_MPEG1);
	m_mpeg2 = !!(s.TraFilters&TRA_MPEG2);
	m_mpa = !!(s.TraFilters&TRA_MPEGAUD);
	m_radgt = !!(s.SrcFilters&SRC_RADGT);
	m_roq = !!(s.SrcFilters&SRC_ROQ);
	m_ogg = !!(s.SrcFilters&SRC_OGG);
	m_lpcm = !!(s.TraFilters&TRA_LPCM);
	m_ac3 = !!(s.TraFilters&TRA_AC3);
	m_dts = !!(s.TraFilters&TRA_DTS);
	m_aac = !!(s.TraFilters&TRA_AAC);
	m_nut = !!(s.SrcFilters&SRC_NUT);
	m_diracsplitter = !!(s.SrcFilters&SRC_DIRAC);
	m_diracvideo = !!(s.TraFilters&TRA_DIRAC);

	UpdateData(FALSE);

	CreateToolTip();

	__if_not_exists(CRadGtSplitterFilter)
	{
		CWnd* pWnd = GetDlgItem(IDC_CHECK_RADGT);
		pWnd->EnableWindow(FALSE);
		CString str;
		pWnd->GetWindowText(str);
		pWnd->SetWindowText(str + _T(" (n/a)"));
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageFilters::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.SrcFilters = s.TraFilters = 0;
	if(m_cdda) s.SrcFilters |= SRC_CDDA;
	if(m_cdxa) s.SrcFilters |= SRC_CDXA;
	if(m_vts) s.SrcFilters |= SRC_VTS;
	if(m_flic) s.SrcFilters |= SRC_FLIC;
	if(m_dvd2avi) s.SrcFilters |= SRC_DVD2AVI;
	if(m_dtsac3) s.SrcFilters |= SRC_DTSAC3;
	if(m_shoutcast) s.SrcFilters |= SRC_SHOUTCAST;
	if(m_avi) s.SrcFilters |= SRC_AVI;
	if(m_matroska) s.SrcFilters |= SRC_MATROSKA;
	if(m_realmedia) s.SrcFilters |= SRC_REALMEDIA;
	if(m_realvideo) s.TraFilters |= TRA_REALVID;
	if(m_realaudio) s.TraFilters |= TRA_REALAUD;
	if(m_mpeg) s.SrcFilters |= SRC_MPEG;
	if(m_mpeg1) s.TraFilters |= TRA_MPEG1;
	if(m_mpeg2) s.TraFilters |= TRA_MPEG2;
	if(m_mpa) s.TraFilters |= TRA_MPEGAUD;
	if(m_radgt) s.SrcFilters |= SRC_RADGT;
	if(m_roq) s.SrcFilters |= SRC_ROQ;
	if(m_ogg) s.SrcFilters |= SRC_OGG;
	if(m_lpcm) s.TraFilters |= TRA_LPCM;
	if(m_ac3) s.TraFilters |= TRA_AC3;
	if(m_dts) s.TraFilters |= TRA_DTS;
	if(m_aac) s.TraFilters |= TRA_AAC;
	if(m_nut) s.SrcFilters |= SRC_NUT;
	if(m_diracsplitter) s.SrcFilters |= SRC_DIRAC;
	if(m_diracvideo) s.TraFilters |= TRA_DIRAC;

	return __super::OnApply();
}

