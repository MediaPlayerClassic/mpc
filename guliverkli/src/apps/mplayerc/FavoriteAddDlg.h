#pragma once
#include "CmdUIDialog.h"


// CFavoriteAddDlg dialog

class CFavoriteAddDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CFavoriteAddDlg)

private:
	CString m_shortname, m_fullname;

public:
	CFavoriteAddDlg(CString shortname, CString fullname, CWnd* pParent = NULL);   // standard constructor
	virtual ~CFavoriteAddDlg();

// Dialog Data
	enum { IDD = IDD_FAVADD };

	CComboBox m_namectrl;
	CString m_name;
	BOOL m_fRememberPos;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
};
