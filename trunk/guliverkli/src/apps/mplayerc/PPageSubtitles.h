#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include "resource.h"
#include "..\..\subtitles\STS.h"
#include "afxwin.h"

// CPPageSubtitles dialog

class CPPageSubtitles : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSubtitles)

public:
	CPPageSubtitles();
	virtual ~CPPageSubtitles();

	BOOL m_fOverridePlacement;
	int m_nHorPos;
	CEdit m_nHorPosEdit;
	CSpinButtonCtrl m_nHorPosCtrl;
	int m_nVerPos;
	CEdit m_nVerPosEdit;
	CSpinButtonCtrl m_nVerPosCtrl;
	int m_nSPCSize;
	CSpinButtonCtrl m_nSPCSizeCtrl;
	CComboBox m_spmaxres;

// Dialog Data
	enum { IDD = IDD_PPAGESUBTITLES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdatePosOverride(CCmdUI* pCmdUI);
};
