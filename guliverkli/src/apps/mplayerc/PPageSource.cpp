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

// PPageSource.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageSource.h"


// CPPageSource dialog

IMPLEMENT_DYNAMIC(CPPageSource, CPPageBase)
CPPageSource::CPPageSource()
	: CPPageBase(CPPageSource::IDD, CPPageSource::IDD)
	, m_cdda(FALSE)
	, m_cdxa(FALSE)
	, m_vts(FALSE)
	, m_flic(FALSE)
	, m_dvd2avi(FALSE)
	, m_dtsac3(FALSE)
	, m_matroska(FALSE)
	, m_shoutcast(FALSE)
	, m_avi(FALSE)
{
}

CPPageSource::~CPPageSource()
{
}

void CPPageSource::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_cdda);
	DDX_Check(pDX, IDC_CHECK2, m_cdxa);
	DDX_Check(pDX, IDC_CHECK3, m_vts);
	DDX_Check(pDX, IDC_CHECK4, m_flic);
	DDX_Check(pDX, IDC_CHECK5, m_dvd2avi);
	DDX_Check(pDX, IDC_CHECK6, m_dtsac3);
	DDX_Check(pDX, IDC_CHECK7, m_matroska);
	DDX_Check(pDX, IDC_CHECK8, m_shoutcast);
	DDX_Check(pDX, IDC_CHECK9, m_realmedia);
	DDX_Check(pDX, IDC_CHECK10, m_avi);
}


BEGIN_MESSAGE_MAP(CPPageSource, CPPageBase)
END_MESSAGE_MAP()


// CPPageSource message handlers

BOOL CPPageSource::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_cdda = !!(s.SrcFilters&SRC_CDDA);
	m_cdxa = !!(s.SrcFilters&SRC_CDXA);
	m_vts = !!(s.SrcFilters&SRC_VTS);
	m_flic = !!(s.SrcFilters&SRC_FLIC);
	m_dvd2avi = !!(s.SrcFilters&SRC_DVD2AVI);
	m_dtsac3 = !!(s.SrcFilters&SRC_DTSAC3);
	m_matroska = !!(s.SrcFilters&SRC_MATROSKA);
	m_realmedia = !!(s.SrcFilters&SRC_REALMEDIA);
	m_shoutcast = !!(s.SrcFilters&SRC_SHOUTCAST);
	m_avi = !!(s.SrcFilters&SRC_AVI);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageSource::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.SrcFilters = 0;
	if(m_cdda) s.SrcFilters |= SRC_CDDA;
	if(m_cdxa) s.SrcFilters |= SRC_CDXA;
	if(m_vts) s.SrcFilters |= SRC_VTS;
	if(m_flic) s.SrcFilters |= SRC_FLIC;
	if(m_dvd2avi) s.SrcFilters |= SRC_DVD2AVI;
	if(m_dtsac3) s.SrcFilters |= SRC_DTSAC3;
	if(m_matroska) s.SrcFilters |= SRC_MATROSKA;
	if(m_realmedia) s.SrcFilters |= SRC_REALMEDIA;
	if(m_shoutcast) s.SrcFilters |= SRC_SHOUTCAST;
	if(m_avi) s.SrcFilters |= SRC_AVI;

	return __super::OnApply();
}
