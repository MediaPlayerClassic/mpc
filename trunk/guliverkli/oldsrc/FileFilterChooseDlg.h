#pragma once

#include "CmdUIDialog.h"

// CFileFilterChooseDlg dialog

class CFileFilterChooseDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CFileFilterChooseDlg)

public:
	CFileFilterChooseDlg(
		CWnd* pParent = NULL, 
		CString path = _T(""), 
		CString name = _T(""), 
		CString clsid = _T("{00000000-0000-0000-0000-000000000000}"));   // standard constructor
	virtual ~CFileFilterChooseDlg();

// Dialog Data
	enum { IDD = IDD_ADDFILEFILTER };
	CString m_path;
	CString m_name;
	CString m_clsid;
	enum {Unknown, Video, Audio};
	int m_iMediaType;
	BOOL m_fDVDDecoder;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
};
