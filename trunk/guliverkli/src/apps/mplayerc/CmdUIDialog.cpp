// CmdUIDialog.cpp : implementation file
//

#include "stdafx.h"
#include <afxpriv.h>
#include "mplayerc.h"
#include "CmdUIDialog.h"


// CCmdUIDialog dialog

IMPLEMENT_DYNAMIC(CCmdUIDialog, CDialog)
CCmdUIDialog::CCmdUIDialog(UINT nIDTemplate, CWnd* pParent /*=NULL*/)
	: CDialog(nIDTemplate, pParent)
{
}

CCmdUIDialog::~CCmdUIDialog()
{
}

LRESULT CCmdUIDialog::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = __super::DefWindowProc(message, wParam, lParam);

	if(message == WM_INITDIALOG)
	{
		SendMessage(WM_KICKIDLE);
	}

	return(ret);
}

BEGIN_MESSAGE_MAP(CCmdUIDialog, CDialog)
	ON_MESSAGE_VOID(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()


// CCmdUIDialog message handlers

void CCmdUIDialog::OnKickIdle()
{
	UpdateDialogControls(this, false);

	// TODO: maybe we should send this call to modeless child cdialogs too
}

// CCmdUIPropertyPage

IMPLEMENT_DYNAMIC(CCmdUIPropertyPage, CPropertyPage)
CCmdUIPropertyPage::CCmdUIPropertyPage(UINT nIDTemplate, UINT nIDCaption)
	: CPropertyPage(nIDTemplate, nIDCaption)
{
}

CCmdUIPropertyPage::~CCmdUIPropertyPage()
{
}

LRESULT CCmdUIPropertyPage::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_COMMAND)
	{
		switch(HIWORD(wParam))
		{
			case BN_CLICKED: case CBN_SELCHANGE: case EN_CHANGE:
				SetModified();
			default:;
		}
	}

	LRESULT ret = __super::DefWindowProc(message, wParam, lParam);

	if(message == WM_INITDIALOG)
	{
		SendMessage(WM_KICKIDLE);
	}

	return(ret);
}

BEGIN_MESSAGE_MAP(CCmdUIPropertyPage, CPropertyPage)
	ON_MESSAGE_VOID(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()


// CCmdUIPropertyPage message handlers

void CCmdUIPropertyPage::OnKickIdle()
{
	UpdateDialogControls(this, false);

	// TODO: maybe we should send this call to modeless child cPropertyPages too
}
