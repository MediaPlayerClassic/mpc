#pragma once
#include "PPageBase.h"
#include "FloatEdit.h"

// CPPageRealMediaQuickTime dialog

class CPPageRealMediaQuickTime : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageRealMediaQuickTime)

public:
	CPPageRealMediaQuickTime();
	virtual ~CPPageRealMediaQuickTime();

// Dialog Data
	enum { IDD = IDD_PPAGERMQT };
	BOOL m_fIntRealMedia;
	BOOL m_fRealMediaRenderless;
	int m_iQuickTimeRenderer;
	CFloatEdit m_RealMediaQuickTimeFPS;
	int m_iRtspHandler;
	BOOL m_fRtspFileExtFirst;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
};
