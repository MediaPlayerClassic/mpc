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

// RegFilterChooserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include <dmo.h>
#include "RegFilterChooserDlg.h"
#include "GraphBuilder.h"
#include ".\regfilterchooserdlg.h"
#include "..\..\DSUtil\DSUtil.h"


// CRegFilterChooserDlg dialog

IMPLEMENT_DYNAMIC(CRegFilterChooserDlg, CDialog)
CRegFilterChooserDlg::CRegFilterChooserDlg(CWnd* pParent /*=NULL*/)
	: CCmdUIDialog(CRegFilterChooserDlg::IDD, pParent)
{
}

CRegFilterChooserDlg::~CRegFilterChooserDlg()
{
	POSITION pos = m_filters.GetHeadPosition();
	while(pos) delete m_filters.GetNext(pos);
}

void CRegFilterChooserDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

void CRegFilterChooserDlg::AddToList(IMoniker* pMoniker)
{
	CComPtr<IPropertyBag> pPB;
	if(SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
	{
		CComVariant var;
		if(SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
		{
			m_list.SetItemDataPtr(
				m_list.AddString(CString(CStringW(var.bstrVal))), 
				m_monikers.AddTail(pMoniker));
		}
	}

}


BEGIN_MESSAGE_MAP(CRegFilterChooserDlg, CCmdUIDialog)
	ON_LBN_DBLCLK(IDC_LIST1, OnLbnDblclkList1)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// CRegFilterChooserDlg message handlers

BOOL CRegFilterChooserDlg::OnInitDialog()
{
	__super::OnInitDialog();

	BeginEnumSysDev(CLSID_LegacyAmFilterCategory, pMoniker)
	{
		AddToList(pMoniker);
	}
	EndEnumSysDev

	BeginEnumSysDev(DMOCATEGORY_VIDEO_EFFECT, pMoniker)
	{
		AddToList(pMoniker);
	}
	EndEnumSysDev

	BeginEnumSysDev(DMOCATEGORY_AUDIO_EFFECT, pMoniker)
	{
		AddToList(pMoniker);
	}
	EndEnumSysDev


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CRegFilterChooserDlg::OnLbnDblclkList1()
{
	SendMessage(WM_COMMAND, IDOK);
}

void CRegFilterChooserDlg::OnUpdateOK(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_list.GetCurSel() >= 0);
}

void CRegFilterChooserDlg::OnBnClickedOk()
{
	if(CComPtr<IMoniker> pMoniker = m_monikers.GetAt((POSITION)m_list.GetItemDataPtr(m_list.GetCurSel())))
	{
		CGraphRegFilter gf(pMoniker);
		Filter* f = new Filter;
		f->fDisabled = false;
		f->type = Filter::REGISTERED;
		f->name = gf.GetName();
		f->dispname = gf.GetDispName();
		gf.GetGUIDs(f->guids);
		gf.GetGUIDs(f->backup);
		f->dwMerit = gf.GetDWORDMerit();
		f->iLoadType = Filter::MERIT;
		m_filters.AddTail(f);
	}

	__super::OnOK();
}

void CRegFilterChooserDlg::OnBnClickedButton1()
{
	CFileDialog dlg(TRUE, NULL, NULL, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY, 
		_T("DirectShow Filters (*.dll,*.ax)|*.dll;*.ax|"), this, 0);

	if(dlg.DoModal() == IDOK)
	{
		CFilterMapper2 fm2(false);
		fm2.Register(dlg.GetPathName());
		m_filters.AddTail(&fm2.m_filters);
		fm2.m_filters.RemoveAll();

		__super::OnOK();
	}
}
