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

