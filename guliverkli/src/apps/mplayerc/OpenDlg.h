#pragma once
#include "CmdUIDialog.h"

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
	COpenFileDialog(CStringArray& mask,
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);
	virtual ~COpenFileDialog();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnIncludeItem(OFNOTIFYEX* pOFNEx, LRESULT* pResult);
};


