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

// OpenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "OpenDlg.h"
#include <atlbase.h>
#include <shlobj.h>
#include "..\..\DSUtil\DSUtil.h"

// COpenDlg dialog

//IMPLEMENT_DYNAMIC(COpenDlg, CResizableDialog)
COpenDlg::COpenDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(COpenDlg::IDD, pParent)
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


BEGIN_MESSAGE_MAP(COpenDlg, CResizableDialog)
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

	AddAnchor(m_mrucombo, TOP_LEFT, TOP_RIGHT);
	AddAnchor(m_mrucombo2, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSEBUTTON, TOP_RIGHT);
	AddAnchor(IDC_BROWSEBUTTON2, TOP_RIGHT);
	AddAnchor(IDOK, TOP_CENTER);
	AddAnchor(IDCANCEL, TOP_CENTER);
	AddAnchor(IDC_STATIC1, TOP_LEFT, TOP_RIGHT);

	CRect r;
	GetWindowRect(r);
	CSize s = r.Size();
	SetMinTrackSize(s);
	s.cx = 1000;
	SetMaxTrackSize(s);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

static CString GetFileName(CString str)
{
	CPath p = str;
	p.StripPath();
	return (LPCTSTR)p;
}

void COpenDlg::OnBnClickedBrowsebutton()
{
	UpdateData();

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

	COpenFileDialog fd(mask, true, NULL, m_path, 
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
	while(pos)
	{
/*
		CString str = fd.GetNextPathName(pos);
		POSITION insertpos = m_fns.GetTailPosition();
		while(insertpos && GetFileName(str).CompareNoCase(GetFileName(m_fns.GetAt(insertpos))) <= 0)
			m_fns.GetPrev(insertpos);
		if(!insertpos) m_fns.AddHead(str);
		else m_fns.InsertAfter(insertpos, str);
*/
		m_fns.AddTail(fd.GetNextPathName(pos));
	}

	if(m_fns.GetCount() > 1 
	|| m_fns.GetCount() == 1 
		&& (m_fns.GetHead()[m_fns.GetHead().GetLength()-1] == '\\'
		|| m_fns.GetHead()[m_fns.GetHead().GetLength()-1] == '*'))
	{
		m_fMultipleFiles = true;
		EndDialog(IDOK);
		return;
	}

	m_mrucombo.SetWindowText(fd.GetPathName());
}

void COpenDlg::OnBnClickedBrowsebutton2()
{
	UpdateData();

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

	COpenFileDialog fd(mask, false, NULL, m_path2, 
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
	m_mrucombo.GetWindowText(m_path);
	pCmdUI->Enable(AfxGetAppSettings().Formats.GetEngine(m_path) == DirectShow);
}

// OpenDlg.cpp : implementation file
//

#include "OpenDlg.h"
#include <dlgs.h>

#define __DUMMY__ _T("*.*")

bool COpenFileDialog::m_fAllowDirSelection = false;
WNDPROC COpenFileDialog::m_wndProc = NULL;

// COpenFileDialog

IMPLEMENT_DYNAMIC(COpenFileDialog, CFileDialog)
COpenFileDialog::COpenFileDialog(CStringArray& mask, bool fAllowDirSelection, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd)
	: CFileDialog(TRUE, lpszDefExt, lpszFileName, dwFlags|OFN_NOVALIDATE, lpszFilter, pParentWnd, 0)
	, m_mask(mask)
{
	m_fAllowDirSelection = fAllowDirSelection;
	m_pOFN->lpstrInitialDir = lpszFileName;
}

COpenFileDialog::~COpenFileDialog()
{
}

BEGIN_MESSAGE_MAP(COpenFileDialog, CFileDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// COpenFileDialog message handlers

LRESULT CALLBACK COpenFileDialog::WindowProcNew(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message ==  WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK
	&& m_fAllowDirSelection)
	{
		CAutoVectorPtr<TCHAR> path;
		path.Allocate(MAX_PATH+1); // MAX_PATH should be bigger for multiple selection, but we are only interested if it's zero length
		// note: allocating MAX_PATH only will cause a buffer overrun for too long strings, and will result in a silent app disappearing crash, 100% reproducable
		if(::GetDlgItemText(hwnd, cmb13, (TCHAR*)path, MAX_PATH) == 0)
			::SendMessage(hwnd, CDM_SETCONTROLTEXT, edt1, (LPARAM)__DUMMY__);
	}

	return CallWindowProc(COpenFileDialog::m_wndProc, hwnd, message, wParam, lParam);
}

BOOL COpenFileDialog::OnInitDialog()
{
	CFileDialog::OnInitDialog();

	m_wndProc = (WNDPROC)SetWindowLong(GetParent()->m_hWnd, GWL_WNDPROC, (LONG)WindowProcNew);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void COpenFileDialog::OnDestroy()
{
	int i = GetPathName().Find(__DUMMY__);
	if(i >= 0) m_pOFN->lpstrFile[i] = m_pOFN->lpstrFile[i+1] = 0;

	CFileDialog::OnDestroy();
}

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
		break;
	}

	return FALSE;   // not handled
}

BOOL COpenFileDialog::OnIncludeItem(OFNOTIFYEX* pOFNEx, LRESULT* pResult)
{
	TCHAR buff[MAX_PATH];
	if(!SHGetPathFromIDList((LPCITEMIDLIST)pOFNEx->pidl, buff)) 
	{
		STRRET s;
		HRESULT hr = ((IShellFolder*)pOFNEx->psf)->GetDisplayNameOf((LPCITEMIDLIST)pOFNEx->pidl, SHGDN_NORMAL|SHGDN_FORPARSING, &s);
		if(S_OK != hr) return FALSE;
		switch(s.uType)
		{
		case STRRET_CSTR: _tcscpy(buff, CString(s.cStr)); break;
		case STRRET_WSTR: _tcscpy(buff, CString(s.pOleStr)); CoTaskMemFree(s.pOleStr); break;
		default: return FALSE;
		}
	}

	CString fn(buff);
/*
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(GetFileAttributesEx(fn, GetFileExInfoStandard, &fad)
	&& (fad.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;
*/
	int i = fn.ReverseFind('.'), j = fn.ReverseFind('\\');
	if(i < 0 || i < j) 
		return FALSE;

	CString mask = m_mask[pOFNEx->lpOFN->nFilterIndex-1] + _T(";");
	CString ext = fn.Mid(i).MakeLower() + _T(";");

	*pResult = mask.Find(ext) >= 0 || mask.Find(_T("*.*")) >= 0;

	return TRUE;
}
