// VSRipPage.cpp : implementation file
//

#include "stdafx.h"
#include <afxpriv.h>
#include "VSRip.h"
#include "VSRipPage.h"

// CVSRipPage dialog

IMPLEMENT_DYNAMIC(CVSRipPage, CDialog)
CVSRipPage::CVSRipPage(IVSFRipper* pVSFRipper, UINT nIDTemplate, CWnd* pParent /*=NULL*/)
	: CDialog(nIDTemplate, pParent)
	, m_pVSFRipper(pVSFRipper)
{
	m_cRef = 1;
}

CVSRipPage::~CVSRipPage()
{
}

void CVSRipPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVSRipPage, CDialog)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CVSRipPage message handlers

void CVSRipPage::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);

	m_pVSFRipper->SetCallBack(bShow ? (IVSFRipperCallback*)this : NULL);	
}

