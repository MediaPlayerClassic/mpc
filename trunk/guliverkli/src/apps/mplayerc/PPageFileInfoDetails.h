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

// CPPageFileInfoDetails dialog

class CPPageFileInfoDetails : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPageFileInfoDetails)

private:
	CComPtr<IFilterGraph> m_pFG;

	HICON m_hIcon;

public:
	CPPageFileInfoDetails(CString fn, IFilterGraph* pFG);
	virtual ~CPPageFileInfoDetails();

// Dialog Data
	enum { IDD = IDD_FILEPROPDETAILS };

	CStatic m_icon;
	CString m_fn;
	CString m_type;
	CString m_size;
	CString m_time;
	CString m_res;
	CString m_created;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
};
