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

#include "stdafx.h"
#include "GSdx9.h"
#include "GSSettingsDlg.h"

IMPLEMENT_DYNAMIC(CGSSettingsDlg, CDialog)
CGSSettingsDlg::CGSSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGSSettingsDlg::IDD, pParent)
	, m_fDisableShaders(FALSE)
	, m_fHalfVRes(FALSE)
	, m_fSoftRenderer(FALSE)
{
}

CGSSettingsDlg::~CGSSettingsDlg()
{
}

void CGSSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_fDisableShaders);
	DDX_Check(pDX, IDC_CHECK3, m_fHalfVRes);
	DDX_Check(pDX, IDC_CHECK2, m_fSoftRenderer);
}

BEGIN_MESSAGE_MAP(CGSSettingsDlg, CDialog)
END_MESSAGE_MAP()

// CGSSettingsDlg message handlers

BOOL CGSSettingsDlg::OnInitDialog()
{
	__super::OnInitDialog();

    CWinApp* pApp = AfxGetApp();

	m_fDisableShaders = !!pApp->GetProfileInt(_T("Settings"), _T("DisableShaders"), FALSE);
	m_fHalfVRes = !!pApp->GetProfileInt(_T("Settings"), _T("HalfVRes"), FALSE);
	m_fSoftRenderer = !!pApp->GetProfileInt(_T("Settings"), _T("SoftRenderer"), FALSE);
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGSSettingsDlg::OnOK()
{
	CWinApp* pApp = AfxGetApp();

	UpdateData();
	pApp->WriteProfileInt(_T("Settings"), _T("DisableShaders"), m_fDisableShaders);
	pApp->WriteProfileInt(_T("Settings"), _T("HalfVRes"), m_fHalfVRes);
	pApp->WriteProfileInt(_T("Settings"), _T("SoftRenderer"), m_fSoftRenderer);

	__super::OnOK();
}
