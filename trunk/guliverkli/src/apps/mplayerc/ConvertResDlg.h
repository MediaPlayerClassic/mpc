#pragma once
#include "afxwin.h"


// CConvertResDlg dialog

class CConvertResDlg : public CResizableDialog
{
public:
	CConvertResDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConvertResDlg();

// Dialog Data
	enum { IDD = IDD_CONVERTRESDIALOG };
	CString m_name;
	CString m_mime;
	CComboBox m_mimectrl;
	CString m_desc;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
};
