/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

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
