#pragma once
#include "afxcmn.h"
#include "PPageBase.h"

// CPPagePlayback dialog

class CPPagePlayback : public CPPageBase
{
	DECLARE_DYNAMIC(CPPagePlayback)

	CStringArray m_AudioRendererDisplayNames;

public:
	CPPagePlayback();
	virtual ~CPPagePlayback();

	CSliderCtrl m_volumectrl;
	CSliderCtrl m_balancectrl;
	int m_nVolume;
	int m_nBalance;
	int m_iLoopForever;
	CEdit m_loopnumctrl;
	int m_nLoops;
	BOOL m_fRewind;
	int m_iZoomLevel;
	BOOL m_iRememberZoomLevel;
	int m_iVideoRendererType;
	CComboBox m_iVideoRendererTypeCtrl;
	int m_iAudioRendererType;
	CComboBox m_iAudioRendererTypeCtrl;
	BOOL m_fAutoloadAudio;
	BOOL m_fAutoloadSubtitles;
	BOOL m_fEnableWorkerThreadForOpening;
	BOOL m_fReportFailedPins;

// Dialog Data
	enum { IDD = IDD_PPAGEPLAYBACK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedRadio12(UINT nID);
	afx_msg void OnUpdateLoopNum(CCmdUI* pCmdUI);
};
