#pragma once
#include "PPageBase.h"
#include "PlayerListCtrl.h"

// CPPageMouse dialog

class CPPageMouse : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageMouse)

private:
	enum {COL_BTN, COL_1CLK, COL_2CLK};
	CList<MouseCmd> m_MouseCmds;
	CString GetCmdName(UINT cmd);

public:
	CPPageMouse();
	virtual ~CPPageMouse();

// Dialog Data
	enum { IDD = IDD_PPAGEMOUSE };
	CPlayerListCtrl m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
};
