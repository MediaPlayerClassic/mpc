// PlayerCaptureBar.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "mainfrm.h"
#include "PlayerCaptureBar.h"


// CPlayerCaptureBar

IMPLEMENT_DYNAMIC(CPlayerCaptureBar, baseCPlayerCaptureBar)
CPlayerCaptureBar::CPlayerCaptureBar()
{
}

CPlayerCaptureBar::~CPlayerCaptureBar()
{
}

BOOL CPlayerCaptureBar::Create(CWnd* pParentWnd)
{
	if(!baseCPlayerCaptureBar::Create(_T("Capture Settings"), pParentWnd, 50))
		return FALSE;

	m_capdlg.Create(this);
	m_capdlg.ShowWindow(SW_SHOWNORMAL);

	CRect r;
	m_capdlg.GetWindowRect(r);
	m_szMinVert = m_szVert = r.Size();
	m_szMinHorz = m_szHorz = r.Size();
	m_szMinFloat = m_szFloat = r.Size();
	m_bFixedFloat = true;
	m_szFixedFloat = r.Size();

	return TRUE;
}

BOOL CPlayerCaptureBar::PreTranslateMessage(MSG* pMsg)
{
	if(IsWindow(pMsg->hwnd) && IsVisible() && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		if(IsDialogMessage(pMsg))
			return TRUE;
	}

	return baseCPlayerCaptureBar::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CPlayerCaptureBar, baseCPlayerCaptureBar)
END_MESSAGE_MAP()

// CPlayerCaptureBar message handlers
