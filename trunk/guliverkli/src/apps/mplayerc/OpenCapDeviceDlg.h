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

#include "afxwin.h"
#include <afxtempl.h>
#include <atlcoll.h>

// COpenCapDeviceDlg dialog

class COpenCapDeviceDlg : public CDialog
{
	DECLARE_DYNAMIC(COpenCapDeviceDlg)

private:
	CStringArray m_vidnames, m_audnames;
//	CInterfaceArray<IMoniker> m_vidmonikers, m_audmonikers;

public:
	COpenCapDeviceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COpenCapDeviceDlg();

	CComboBox m_vidctrl;
	CComboBox m_audctrl;

//	CString m_vidfrstr, m_audfrstr;
	CString m_vidstr, m_audstr;
//	CComPtr<IBaseFilter> m_pVidCap, m_pAudCap;

// Dialog Data
	enum { IDD = IDD_OPENCAPDEVICEDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedOk();
};
