// Media Player Classic.  Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

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
