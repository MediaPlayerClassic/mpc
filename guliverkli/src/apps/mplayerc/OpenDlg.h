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

#pragma once
#include "CmdUIDialog.h"
#include "afxwin.h"

// COpenDlg dialog

class COpenDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(COpenDlg)

public:
	COpenDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COpenDlg();

	bool m_fMultipleFiles;
	CStringList m_fns;

// Dialog Data
	enum { IDD = IDD_OPENDIALOG };
	CComboBox m_mrucombo;
	CString m_path;
	CComboBox m_mrucombo2;
	CString m_path2;
	CButton m_openbtn2;
	CStatic m_label2;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBrowsebutton();
	afx_msg void OnBnClickedBrowsebutton2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnUpdateDub(CCmdUI* pCmdUI);
};

// COpenFileDialog

class COpenFileDialog : public CFileDialog
{
	DECLARE_DYNAMIC(COpenFileDialog)

private:
	CStringArray& m_mask;

public:
	COpenFileDialog(CStringArray& mask, bool fAllowDirSelection,
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);
	virtual ~COpenFileDialog();

	static bool m_fAllowDirSelection;
	static WNDPROC m_wndProc;
	static LRESULT CALLBACK WindowProcNew(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam);

	void OnInitDone();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnIncludeItem(OFNOTIFYEX* pOFNEx, LRESULT* pResult);
public:
	afx_msg void OnDestroy();
};


