#pragma once
#include "afxwin.h"


// CAuthDlg dialog

class CAuthDlg : public CDialog
{
	DECLARE_DYNAMIC(CAuthDlg)

private:
	CString DEncrypt(CString pw);
	CMapStringToString m_logins;

public:
	CAuthDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAuthDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG3 };
	CComboBox m_usernamectrl;
	CString m_username;
	CString m_password;
	BOOL m_remember;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnEnSetfocusEdit3();
};
