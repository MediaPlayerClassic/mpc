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

// PPageRealMediaQuickTime.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageRealMediaQuickTime.h"

// CPPageRealMediaQuickTime dialog

IMPLEMENT_DYNAMIC(CPPageRealMediaQuickTime, CPPageBase)
CPPageRealMediaQuickTime::CPPageRealMediaQuickTime()
	: CPPageBase(CPPageRealMediaQuickTime::IDD, CPPageRealMediaQuickTime::IDD)
	, m_fIntRealMedia(FALSE)
	, m_fRealMediaRenderless(FALSE)
	, m_iQuickTimeRenderer(0)
{
}

CPPageRealMediaQuickTime::~CPPageRealMediaQuickTime()
{
}

void CPPageRealMediaQuickTime::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK2, m_fIntRealMedia);
	DDX_Check(pDX, IDC_CHECK1, m_fRealMediaRenderless);
	DDX_Radio(pDX, IDC_RADIO1, m_iQuickTimeRenderer);
	DDX_Control(pDX, IDC_EDIT2, m_RealMediaQuickTimeFPS);
}


BEGIN_MESSAGE_MAP(CPPageRealMediaQuickTime, CPPageBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// CPPageRealMediaQuickTime message handlers

BOOL CPPageRealMediaQuickTime::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_fIntRealMedia = s.fIntRealMedia;
//	m_fRealMediaRenderless = s.fRealMediaRenderless;
//	m_iQuickTimeRenderer = s.iQuickTimeRenderer;
	m_RealMediaQuickTimeFPS = s.RealMediaQuickTimeFPS;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageRealMediaQuickTime::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.fIntRealMedia = !!m_fIntRealMedia;
//	s.fRealMediaRenderless = !!m_fRealMediaRenderless;
//	s.iQuickTimeRenderer = m_iQuickTimeRenderer;
	float f;
	if(m_RealMediaQuickTimeFPS.GetFloat(f)) s.RealMediaQuickTimeFPS = f;
	m_RealMediaQuickTimeFPS = s.RealMediaQuickTimeFPS;

	UpdateData(FALSE);

	return __super::OnApply();
}

void CPPageRealMediaQuickTime::OnBnClickedButton1()
{
	UpdateData(FALSE);
}
