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

// ComPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "ComPropertyPage.h"
#include "ComPropertySheet.h"


// CComPropertyPage dialog

IMPLEMENT_DYNAMIC(CComPropertyPage, CPropertyPage)
CComPropertyPage::CComPropertyPage(IPropertyPage* pPage)
	: CPropertyPage(CComPropertyPage::IDD), m_pPage(pPage)
{
	PROPPAGEINFO ppi;
	m_pPage->GetPageInfo(&ppi);
	m_pPSP->pszTitle = (m_strCaption = ppi.pszTitle);
	m_psp.dwFlags |= PSP_USETITLE;
}

CComPropertyPage::~CComPropertyPage()
{
}

void CComPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CComPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CRect r;
	PROPPAGEINFO ppi;
	m_pPage->GetPageInfo(&ppi);
	r = CRect(CPoint(0,0), ppi.size);
	m_pPage->Activate(m_hWnd, r, FALSE);
	m_pPage->Show(SW_SHOW);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CComPropertyPage::OnDestroy()
{
	CPropertyPage::OnDestroy();

	m_pPage->Deactivate();
}

BOOL CComPropertyPage::OnSetActive()
{
	SetModified(S_OK == m_pPage->IsPageDirty());

	CWnd* pParent = GetParent();
	if(pParent->IsKindOf(RUNTIME_CLASS(CComPropertySheet)))
	{
		CComPropertySheet* pSheet = (CComPropertySheet*)pParent;
		pSheet->OnActivated(this);
	}

	return CPropertyPage::OnSetActive();
}

BOOL CComPropertyPage::OnKillActive()
{
	SetModified(FALSE);

	return CPropertyPage::OnKillActive();
}


BEGIN_MESSAGE_MAP(CComPropertyPage, CPropertyPage)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CComPropertyPage message handlers

void CComPropertyPage::OnOK()
{
	if(S_OK == m_pPage->IsPageDirty()) m_pPage->Apply();
	SetModified(FALSE);

	CPropertyPage::OnOK();
}
