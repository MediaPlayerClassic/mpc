#pragma once
#include "PPageBase.h"

// CPPageTweaks dialog

class CPPageTweaks : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageTweaks)

private:
	bool m_fXPOrBetter, m_fWMASFReader;

public:
	CPPageTweaks();
	virtual ~CPPageTweaks();

	BOOL m_fDisabeXPToolbars;
	CButton m_fDisabeXPToolbarsCtrl;
	BOOL m_fUseWMASFReader;
	CButton m_fUseWMASFReaderCtrl;

// Dialog Data
	enum { IDD = IDD_PPAGETWEAKS };
	int m_nJumpDistS;
	int m_nJumpDistM;
	int m_nJumpDistL;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateCheck3(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCheck2(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton1();
};
