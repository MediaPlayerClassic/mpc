// PPageBase.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageBase.h"


// CPPageBase dialog

IMPLEMENT_DYNAMIC(CPPageBase, CCmdUIPropertyPage)
CPPageBase::CPPageBase(UINT nIDTemplate, UINT nIDCaption)
	: CCmdUIPropertyPage(nIDTemplate, nIDCaption)
{
}

CPPageBase::~CPPageBase()
{
}

void CPPageBase::DoDataExchange(CDataExchange* pDX)
{
	CCmdUIPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPPageBase, CCmdUIPropertyPage)
END_MESSAGE_MAP()


// CPPageBase message handlers

BOOL CPPageBase::OnSetActive()
{
	AfxGetApp()->WriteProfileInt(ResStr(IDS_R_SETTINGS), _T("LastUsedPage"), (UINT)m_pPSP->pszTemplate);

	return CCmdUIPropertyPage::OnSetActive();
}
