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

#include <afxwin.h>
#include <afxcmn.h>
#include <atlimage.h>
#include "resource.h"
#include "..\..\filters\transform\Mpeg2DecFilter\IMpeg2DecFilter.h"

// CPPageMpegDecoder dialog

class CPPageMpegDecoder : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageMpegDecoder)

private:
	CComQIPtr<IMpeg2DecFilter> m_pMpeg2DecFilter;

public:
	CPPageMpegDecoder(IFilterGraph* pFG);
	virtual ~CPPageMpegDecoder();

// Dialog Data
	enum { IDD = IDD_PPAGEMPEG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	CComboBox m_dilist;
	CSliderCtrl m_brightctrl;
	CSliderCtrl m_contctrl;
	CSliderCtrl m_huectrl;
	CSliderCtrl m_satctrl;
	BOOL m_fForcedSubs;
	BOOL m_fPlanarYUV;
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedCheck2();
};
