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

#include <afxwin.h>
#include <atlcoll.h>
#include "CmdUIDialog.h"
#include "GraphBuilder.h"

// CMediaTypesDlg dialog

class CMediaTypesDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CMediaTypesDlg)

private:
	CArray<CGraphBuilder::DeadEnd*> m_DeadEnds;
	enum {UNKNOWN, VIDEO, AUDIO} m_type;
	GUID m_subtype;
	void AddLine(CString str = _T("\n"));
	void AddMediaType(AM_MEDIA_TYPE* pmt);

public:
	CMediaTypesDlg(CGraphBuilder& gb, CWnd* pParent = NULL);   // standard constructor
	virtual ~CMediaTypesDlg();

// Dialog Data
	enum { IDD = IDD_MEDIATYPESDIALOG };
	CComboBox m_pins;
	CEdit m_report;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateButton1(CCmdUI* pCmdUI);
};
