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

// PPageSheet.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageSheet.h"

// CPPageSheet

IMPLEMENT_DYNAMIC(CPPageSheet, CTreePropSheet)

CPPageSheet::CPPageSheet(LPCTSTR pszCaption, IUnknown* pAudioSwitcher, CWnd* pParentWnd, UINT idPage)
	: CTreePropSheet(pszCaption, pParentWnd, 0)
	, m_audioswitcher(pAudioSwitcher)
{
	AddPage(&m_player);
	AddPage(&m_formats);
	AddPage(&m_acceltbl);
	AddPage(&m_playback);
	AddPage(&m_dvd);
	AddPage(&m_realmediaquicktime);
	AddPage(&m_filters);
	AddPage(&m_audioswitcher);
	AddPage(&m_subtitles);
	AddPage(&m_substyle);
	AddPage(&m_tweaks);

	EnableStackedTabs(FALSE);

	SetTreeViewMode(TRUE, TRUE, FALSE);

	if(idPage || (idPage = AfxGetApp()->GetProfileInt(ResStr(IDS_R_SETTINGS), _T("LastUsedPage"), 0)))
	{
		for(int i = 0; i < GetPageCount(); i++)
		{
			if(GetPage(i)->m_pPSP->pszTemplate == MAKEINTRESOURCE(idPage))
			{
				SetActivePage(i);
				break;
			}
		}
	}
}

CPPageSheet::~CPPageSheet()
{
}

CTreeCtrl* CPPageSheet::CreatePageTreeObject()
{
	return new CTreePropSheetTreeCtrl();
}

BEGIN_MESSAGE_MAP(CPPageSheet, CTreePropSheet)
END_MESSAGE_MAP()

BOOL CPPageSheet::OnInitDialog()
{
	BOOL bResult = __super::OnInitDialog();

	if(CTreeCtrl* pTree = GetPageTreeControl())
	{
		for(HTREEITEM node = pTree->GetRootItem(); node; node = pTree->GetNextSiblingItem(node))
			pTree->Expand(node, TVE_EXPAND);
	}

	return bResult;
}

// CTreePropSheetTreeCtrl

IMPLEMENT_DYNAMIC(CTreePropSheetTreeCtrl, CTreeCtrl)
CTreePropSheetTreeCtrl::CTreePropSheetTreeCtrl()
{
}

CTreePropSheetTreeCtrl::~CTreePropSheetTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CTreePropSheetTreeCtrl, CTreeCtrl)
END_MESSAGE_MAP()

// CTreePropSheetTreeCtrl message handlers


BOOL CTreePropSheetTreeCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_CLIENTEDGE;
//	cs.style &= ~TVS_LINESATROOT;

	return __super::PreCreateWindow(cs);
}

