// Media Player Classic.  Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

// PPageTweaks.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageTweaks.h"
#include "..\..\DSUtil\DSUtil.h"

// CPPageTweaks dialog

IMPLEMENT_DYNAMIC(CPPageTweaks, CPPageBase)
CPPageTweaks::CPPageTweaks()
	: CPPageBase(CPPageTweaks::IDD, CPPageTweaks::IDD)
	, m_fDisabeXPToolbars(FALSE)
	, m_fUseWMASFReader(FALSE)
	, m_nJumpDistS(0)
	, m_nJumpDistM(0)
	, m_nJumpDistL(0)
{
	DWORD ver = ::GetVersion();
	m_fXPOrBetter = (int)ver >= 0 && (((ver<<8)&0xff00)|((ver>>8)&0xff)) >= 0x0501;

	m_fWMASFReader = SUCCEEDED(CComPtr<IBaseFilter>().CoCreateInstance(
		GUIDFromCString(_T("{187463A0-5BB7-11D3-ACBE-0080C75E246E}")))); // WM ASF Reader
}

CPPageTweaks::~CPPageTweaks()
{
}

void CPPageTweaks::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK3, m_fDisabeXPToolbars);
	DDX_Control(pDX, IDC_CHECK3, m_fDisabeXPToolbarsCtrl);
	DDX_Check(pDX, IDC_CHECK2, m_fUseWMASFReader);
	DDX_Control(pDX, IDC_CHECK2, m_fUseWMASFReaderCtrl);
	DDX_Text(pDX, IDC_EDIT1, m_nJumpDistS);
	DDX_Text(pDX, IDC_EDIT2, m_nJumpDistM);
	DDX_Text(pDX, IDC_EDIT3, m_nJumpDistL);
}

BOOL CPPageTweaks::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_fDisabeXPToolbars = s.fDisabeXPToolbars;
	m_fUseWMASFReader = s.fUseWMASFReader;
	m_nJumpDistS = s.nJumpDistS;
	m_nJumpDistM = s.nJumpDistM;
	m_nJumpDistL = s.nJumpDistL;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageTweaks::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.fDisabeXPToolbars = !!m_fDisabeXPToolbars;
	s.fUseWMASFReader = !!m_fUseWMASFReader;
	s.nJumpDistS = m_nJumpDistS;
	s.nJumpDistM = m_nJumpDistM;
	s.nJumpDistL = m_nJumpDistL;

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageTweaks, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateCheck3)
	ON_UPDATE_COMMAND_UI(IDC_CHECK2, OnUpdateCheck2)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// CPPageTweaks message handlers

void CPPageTweaks::OnUpdateCheck3(CCmdUI* pCmdUI)
{
	if(!m_fXPOrBetter)
	{
		pCmdUI->Enable(FALSE);
		pCmdUI->SetCheck(TRUE);
	}
}

void CPPageTweaks::OnUpdateCheck2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_fWMASFReader);
}

void CPPageTweaks::OnBnClickedButton1()
{
	m_nJumpDistS = 1000;
	m_nJumpDistM = 5000;
	m_nJumpDistL = 20000;

	UpdateData(FALSE);
}
