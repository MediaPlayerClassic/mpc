#pragma once
#include "afxcmn.h"
#include "PPageBase.h"
#include "afxwin.h"

// CPPagePlayer dialog

class CPPagePlayer : public CPPageBase
{
	DECLARE_DYNAMIC(CPPagePlayer)

private:
	CArray<dispmode> m_dms;

public:
	CPPagePlayer();
	virtual ~CPPagePlayer();

	int m_iAllowMultipleInst;
	int m_iTitleBarTextStyle;
	BOOL m_iAlwaysOnTop;
	BOOL m_iShowBarsWhenFullScreen;
	int m_nShowBarsWhenFullScreenTimeOut;
	BOOL m_fSetFullscreenRes;
	CComboBox m_dispmodecombo;
	BOOL m_fExitFullScreenAtTheEnd;
	BOOL m_fRememberWindowPos;
	BOOL m_fRememberWindowSize;
	BOOL m_fUseIni;
	CSpinButtonCtrl m_nTimeOutCtrl;

// Dialog Data
	enum { IDD = IDD_PPAGEPLAYER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedCheck8();
	afx_msg void OnUpdateTimeout(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDispModeCombo(CCmdUI* pCmdUI);
	BOOL m_fTrayIcon;
};
