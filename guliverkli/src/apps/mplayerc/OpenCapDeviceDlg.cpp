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

// OpenCapDeviceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "OpenCapDeviceDlg.h"
#include "..\..\DSUtil\DSUtil.h"

// COpenCapDeviceDlg dialog

//IMPLEMENT_DYNAMIC(COpenCapDeviceDlg, CResizableDialog)
COpenCapDeviceDlg::COpenCapDeviceDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(COpenCapDeviceDlg::IDD, pParent)
	, m_vidstr(_T(""))
	, m_audstr(_T(""))
{
}

COpenCapDeviceDlg::~COpenCapDeviceDlg()
{
}

void COpenCapDeviceDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_vidctrl);
	DDX_Control(pDX, IDC_COMBO2, m_audctrl);
}

BEGIN_MESSAGE_MAP(COpenCapDeviceDlg, CResizableDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// COpenCapDeviceDlg message handlers

BOOL COpenCapDeviceDlg::OnInitDialog()
{
	__super::OnInitDialog();

	BeginEnumSysDev(CLSID_VideoInputDeviceCategory, pMoniker)
	{
//		m_vidmonikers.Add(pMoniker);

		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		m_vidctrl.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;
		if(SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName)))
		{
			m_vidnames.Add(CString(strName));
			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if(m_vidctrl.GetCount())
		m_vidctrl.SetCurSel(0);

	BeginEnumSysDev(CLSID_AudioInputDeviceCategory, pMoniker)
	{
//		m_audmonikers.Add(pMoniker);

		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		m_audctrl.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;
		if(SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName)))
		{
			m_audnames.Add(CString(strName));
			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if(m_audctrl.GetCount())
		m_audctrl.SetCurSel(0);

	AddAnchor(m_vidctrl, TOP_LEFT, TOP_RIGHT);
	AddAnchor(m_audctrl, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, TOP_CENTER);
	AddAnchor(IDCANCEL, TOP_CENTER);

	CRect r;
	GetWindowRect(r);
	CSize s = r.Size();
	SetMinTrackSize(s);
	s.cx = 1000;
	SetMaxTrackSize(s);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void COpenCapDeviceDlg::OnBnClickedOk()
{
	UpdateData();

	if(m_vidctrl.GetCurSel() >= 0) 
	{
//		m_vidctrl.GetLBText(m_vidctrl.GetCurSel(), m_vidfrstr);
		m_vidstr = m_vidnames[m_vidctrl.GetCurSel()];
/*
		m_pVidCap = NULL;
		m_vidmonikers[m_vidctrl.GetCurSel()]->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pVidCap);
*/
	}

	if(m_audctrl.GetCurSel() >= 0)
	{
//		m_audctrl.GetLBText(m_audctrl.GetCurSel(), m_audfrstr);
		m_audstr = m_audnames[m_audctrl.GetCurSel()];
/*
		m_pAudCap = NULL;
		m_audmonikers[m_audctrl.GetCurSel()]->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pAudCap);
*/
	}

	OnOK();
}
