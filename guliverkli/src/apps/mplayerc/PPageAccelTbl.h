#pragma once

#include <afxcmn.h>
#include <afxtempl.h>
#include "PPageBase.h"
#include "PlayerListCtrl.h"
#include "StaticLink.h"

// CPPageAccelTbl dialog

class CPPageAccelTbl : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAccelTbl)

private:
	enum {COL_CMD, COL_MOD, COL_KEY, COL_TYPE, COL_ID, COL_MOUSE, COL_RMCMD, COL_RMREPCNT};
	CList<wmcmd> m_wmcmds;

	void SetupList();

	int m_counter;

public:
	CPPageAccelTbl();
	virtual ~CPPageAccelTbl();

	static CString MakeAccelModLabel(BYTE fVirt);
	static CString MakeAccelVkeyLabel(WORD key, bool fVirtKey);
	static CString MakeAccelShortcutLabel(UINT id);
	static CString MakeAccelShortcutLabel(ACCEL& a);
	static CString MakeMouseButtonLabel(UINT mouse);

// Dialog Data
	enum { IDD = IDD_PPAGEACCELTBL };
	CPlayerListCtrl m_list;
	BOOL m_fWinLirc;
	CString m_WinLircAddr;
	CEdit m_WinLircEdit;
	CStaticLink m_WinLircLink;
	BOOL m_fUIce;
	CString m_UIceAddr;
	CEdit m_UIceEdit;
	CStaticLink m_UIceLink;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTimer(UINT nIDEvent);
};
