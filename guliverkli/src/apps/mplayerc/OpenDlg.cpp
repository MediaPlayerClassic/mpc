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

// OpenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "OpenDlg.h"
#include <atlbase.h>
#include "..\..\DSUtil\DSUtil.h"

// COpenDlg dialog

IMPLEMENT_DYNAMIC(COpenDlg, CCmdUIDialog)
COpenDlg::COpenDlg(CWnd* pParent /*=NULL*/)
	: CCmdUIDialog(COpenDlg::IDD, pParent)
	, m_path(_T(""))
	, m_path2(_T(""))
	, m_fMultipleFiles(false)
{
}

COpenDlg::~COpenDlg()
{
}

void COpenDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_mrucombo);
	DDX_CBString(pDX, IDC_COMBO1, m_path);
	DDX_Control(pDX, IDC_COMBO2, m_mrucombo2);
	DDX_CBString(pDX, IDC_COMBO2, m_path2);
	DDX_Control(pDX, IDC_STATIC1, m_label2);
	DDX_Control(pDX, IDC_BROWSEBUTTON2, m_openbtn2);
}


BEGIN_MESSAGE_MAP(COpenDlg, CCmdUIDialog)
	ON_BN_CLICKED(IDC_BROWSEBUTTON, OnBnClickedBrowsebutton)
	ON_BN_CLICKED(IDC_BROWSEBUTTON2, OnBnClickedBrowsebutton2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateDub)
	ON_UPDATE_COMMAND_UI(IDC_COMBO2, OnUpdateDub)
	ON_UPDATE_COMMAND_UI(IDC_BROWSEBUTTON2, OnUpdateDub)
END_MESSAGE_MAP()


// COpenDlg message handlers

BOOL COpenDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CRecentFileList& MRU = AfxGetAppSettings().MRU;
	MRU.ReadList();
	m_mrucombo.ResetContent();
	for(int i = 0; i < MRU.GetSize(); i++)
		if(!MRU[i].IsEmpty())
			m_mrucombo.AddString(MRU[i]);
	CorrectComboListWidth(m_mrucombo, GetFont());

	CRecentFileList& MRUDub = AfxGetAppSettings().MRUDub;
	MRUDub.ReadList();
	m_mrucombo2.ResetContent();
	for(int i = 0; i < MRUDub.GetSize(); i++)
		if(!MRUDub[i].IsEmpty())
			m_mrucombo2.AddString(MRUDub[i]);
	CorrectComboListWidth(m_mrucombo2, GetFont());

	if(m_mrucombo.GetCount() > 0) m_mrucombo.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void COpenDlg::OnBnClickedBrowsebutton()
{
	CMediaFormats& mf = AfxGetAppSettings().Formats;

	CString filter;
	CStringArray mask;

	filter += _T("Media files (all types)|__dummy|");
	mask.Add(_T(""));

	for(int i = 0; i < mf.GetCount(); i++) 
		mask[0] += mf[i].GetFilter() + _T(";");
	mask[0].TrimRight(_T(";"));

	for(int i = 0; i < mf.GetCount(); i++)
	{
		CMediaFormatCategory& mfc = mf[i];
		filter += mfc.GetLabel() + _T("|__dummy|");
		mask.Add(mfc.GetFilter());
	}

	filter += _T("All files (*.*)|__dummy|");
	mask.Add(_T("*.*"));

	filter += _T("|");

	COpenFileDialog fd(mask, NULL, NULL, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_ENABLEINCLUDENOTIFY, 
		filter, this);

	CAutoVectorPtr<TCHAR> buff;
	buff.Allocate(10000);
	buff[0] = 0;
	fd.m_pOFN->lpstrFile = buff;
	fd.m_pOFN->nMaxFile = 10000;

	if(fd.DoModal() != IDOK) return;

	m_fns.RemoveAll();

	POSITION pos = fd.GetStartPosition();
	while(pos) m_fns.AddTail(fd.GetNextPathName(pos));

	if(m_fns.GetCount() > 1)
	{
		m_fMultipleFiles = true;
		EndDialog(IDOK);
		return;
	}

	m_mrucombo.SetWindowText(fd.GetPathName());
}

void COpenDlg::OnBnClickedBrowsebutton2()
{
	CMediaFormats& mf = AfxGetAppSettings().Formats;

	CString filter;
	CStringArray mask;

	filter += _T("Audio files (all types)|__dummy|");
	mask.Add(_T(""));

	for(int i = 0; i < mf.GetCount(); i++)
	{
		CMediaFormatCategory& mfc = mf[i];
		if(!mfc.IsAudioOnly() || mfc.GetEngineType() != DirectShow) continue;
		mask[0] += mf[i].GetFilter() + _T(";");
	}
	mask[0].TrimRight(_T(";"));

	for(int i = 0; i < mf.GetCount(); i++)
	{
		CMediaFormatCategory& mfc = mf[i];
		if(!mfc.IsAudioOnly() || mfc.GetEngineType() != DirectShow) continue;
		filter += mfc.GetLabel() + _T("|__dummy|");
		mask.Add(mfc.GetFilter());
	}

	filter += _T("All files (*.*)|__dummy|");
	mask.Add(_T("*.*"));

	filter += _T("|");

	COpenFileDialog fd(mask, NULL, NULL, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ENABLEINCLUDENOTIFY, 
		filter, this);

	if(fd.DoModal() != IDOK) return;

	m_mrucombo2.SetWindowText(fd.GetPathName());
}

void COpenDlg::OnBnClickedOk()
{
	UpdateData();

	m_fns.RemoveAll();
	m_fns.AddTail(m_path);
	if(m_mrucombo2.IsWindowEnabled())
		m_fns.AddTail(m_path2);

	m_fMultipleFiles = false;

	OnOK();
}

void COpenDlg::OnUpdateDub(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(AfxGetAppSettings().Formats.GetEngine(m_path) == DirectShow);
}

// OpenDlg.cpp : implementation file
//

#include "OpenDlg.h"

// COpenFileDialog

IMPLEMENT_DYNAMIC(COpenFileDialog, CFileDialog)
COpenFileDialog::COpenFileDialog(CStringArray& mask, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd)
	: CFileDialog(TRUE, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, 0)
	, m_mask(mask)
{
}

COpenFileDialog::~COpenFileDialog()
{
}


BEGIN_MESSAGE_MAP(COpenFileDialog, CFileDialog)
END_MESSAGE_MAP()



// COpenFileDialog message handlers

BOOL COpenFileDialog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);

	OFNOTIFY* pNotify = (OFNOTIFY*)lParam;
	// allow message map to override
	if (__super::OnNotify(wParam, lParam, pResult))
	{
		ASSERT(pNotify->hdr.code != CDN_INCLUDEITEM);
		return TRUE;
	}

	switch(pNotify->hdr.code)
	{
	case CDN_INCLUDEITEM:
		if(OnIncludeItem((OFNOTIFYEX*)lParam, pResult))
			return TRUE;
	}

	return FALSE;   // not handled
}

BOOL COpenFileDialog::OnIncludeItem(OFNOTIFYEX* pOFNEx, LRESULT* pResult)
{
	TCHAR buff[MAX_PATH];
	if(!SHGetPathFromIDList((LPCITEMIDLIST)pOFNEx->pidl, buff)) 
		return FALSE;

	CString fn(buff);

	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(GetFileAttributesEx(fn, GetFileExInfoStandard, &fad)
	&& (fad.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;

	int i = fn.ReverseFind('.'), j = fn.ReverseFind('\\');
	if(i < 0 || i < j) 
		return FALSE;

	CString mask = m_mask[pOFNEx->lpOFN->nFilterIndex-1] + _T(";");
	CString ext = fn.Mid(i).MakeLower() + _T(";");

	*pResult = mask.Find(ext) >= 0 || mask.Find(_T("*.*")) >= 0;

	return TRUE;
}
