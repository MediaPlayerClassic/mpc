/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

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
