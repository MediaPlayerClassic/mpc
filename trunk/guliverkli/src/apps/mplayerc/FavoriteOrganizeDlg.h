#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CFavoriteOrganizeDlg dialog

class CFavoriteOrganizeDlg : public CDialog
{
	DECLARE_DYNAMIC(CFavoriteOrganizeDlg)

private:
	CStringList m_sl[3];
	void SetupList(bool fSave);

public:
	CFavoriteOrganizeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFavoriteOrganizeDlg();

// Dialog Data
	enum { IDD = IDD_FAVORGANIZE };

	CTabCtrl m_tab;
	CListCtrl m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnTcnSelchangingTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnLvnEndlabeleditList2(NMHDR *pNMHDR, LRESULT *pResult);
};
