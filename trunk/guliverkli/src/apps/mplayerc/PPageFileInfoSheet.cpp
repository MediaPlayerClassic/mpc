// PPageFileInfoSheet.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFileInfoSheet.h"

// CPPageFileInfoSheet

IMPLEMENT_DYNAMIC(CPPageFileInfoSheet, CPropertySheet)
CPPageFileInfoSheet::CPPageFileInfoSheet(CString fn, IFilterGraph* pFG, CWnd* pParentWnd)
	: CPropertySheet(_T("Properties"), pParentWnd, 0)
	, m_clip(fn, pFG)
	, m_details(fn, pFG)
{
	AddPage(&m_clip);
	AddPage(&m_details);
}

CPPageFileInfoSheet::~CPPageFileInfoSheet()
{
}


BEGIN_MESSAGE_MAP(CPPageFileInfoSheet, CPropertySheet)
END_MESSAGE_MAP()

// CPPageFileInfoSheet message handlers

BOOL CPPageFileInfoSheet::OnInitDialog()
{
	BOOL fRet = __super::OnInitDialog();
	
	GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_APPLY_NOW)->ShowWindow(SW_HIDE);
	GetDlgItem(IDOK)->SetWindowText(_T("Close"));

	CRect r;
	GetDlgItem(ID_APPLY_NOW)->GetWindowRect(&r);
	ScreenToClient(r);
	GetDlgItem(IDOK)->MoveWindow(r);

	return fRet;
}

