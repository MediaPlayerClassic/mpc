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

// StatusLabel.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "StatusLabel.h"


// CStatusLabel

IMPLEMENT_DYNAMIC(CStatusLabel, CStatic)
CStatusLabel::CStatusLabel(bool fRightAlign) : m_fRightAlign(fRightAlign)
{
	HDC hdc = ::GetDC(NULL);
	double s = 1.0*GetDeviceCaps(hdc, LOGPIXELSY) / 96.0;
	m_font.CreateFont(int(15.0 * GetDeviceCaps(hdc, LOGPIXELSY) / 96.0), 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY/*ANTIALIASED_QUALITY*/, DEFAULT_PITCH|FF_DONTCARE, 
		_T("MS Sans Serif"));
	::ReleaseDC(0, hdc);
}

CStatusLabel::~CStatusLabel()
{
}

BEGIN_MESSAGE_MAP(CStatusLabel, CStatic)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CStatusLabel message handlers


void CStatusLabel::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
		CString str;
		GetWindowText(str);
		CRect r;
		GetClientRect(&r);
		CFont* old = dc.SelectObject(&m_font);
		dc.SetTextColor(0xffffff);
		dc.SetBkColor(0);
		CSize size = dc.GetTextExtent(str);
		CPoint p = CPoint(m_fRightAlign ? r.Width() - size.cx : 0, (r.Height()-size.cy)/2);
		dc.TextOut(p.x, p.y, str);
		dc.ExcludeClipRect(CRect(p, size));
		dc.SelectObject(&old);
		dc.FillSolidRect(&r, 0);
	dc.Detach();
}

BOOL CStatusLabel::OnEraseBkgnd(CDC* pDC)
{
	CRect r;
	GetClientRect(&r);
	pDC->FillSolidRect(&r, 0);
	return TRUE;
}

