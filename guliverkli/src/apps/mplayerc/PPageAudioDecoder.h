/* 
 *	Copyright (C) 2003-2004 Gabest
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

#include "PPageBase.h"
#include "..\..\filters\transform\MpaDecFilter\MpaDecFilter.h"
#include "afxwin.h"
#include "afxcmn.h"

// CPPageAudioDecoder dialog

class CPPageAudioDecoder : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAudioDecoder)

protected:
    CInterfaceList<IMpaDecFilter> m_pMDFs;

public:
	CPPageAudioDecoder(IFilterGraph* pFG);   // standard constructor
	virtual ~CPPageAudioDecoder();

// Dialog Data
	enum { IDD = IDD_PPAGEAUDIODEC };
	int m_iSampleFormat;
	BOOL m_fNormalize;
	BOOL m_fAc3SpeakerConfig;
	int m_iAc3SpeakerConfig;
	BOOL m_fAc3SpeakerConfigLFE;
	BOOL m_fAc3DynamicRangeControl;
	CComboBox m_ac3sclist;
	BOOL m_fDtsSpeakerConfig;
	int m_iDtsSpeakerConfig;
	BOOL m_fDtsSpeakerConfigLFE;
	BOOL m_fDtsDynamicRangeControl;
	CComboBox m_dtssclist;
	int m_iAacSpeakerConfig;
	int m_boost;
	CSliderCtrl m_boostctrl;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
