#pragma once
#include "afxwin.h"
#include "CmdUIDialog.h"

// CRegFilterChooserDlg dialog

class CRegFilterChooserDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CRegFilterChooserDlg)

	CInterfaceList<IMoniker> m_monikers;
	void AddToList(IMoniker* pMoniker);

public:
	CRegFilterChooserDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRegFilterChooserDlg();

	CList<Filter*> m_filters;

// Dialog Data
	enum { IDD = IDD_ADDREGFILTER };
	CListBox m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLbnDblclkList1();
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();
};
