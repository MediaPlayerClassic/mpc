

/* 
 *	Copyright (C) 2003-2005 Gabest
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

// PPageFilters.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFilters.h"
#include "..\..\filters\filters.h"
#include ".\ppagefilters.h"

static struct filter_t {LPCTSTR label; int type; int flag; UINT nHintID;} s_filters[] = 
{
	{_T("AVI"), 0, SRC_AVI, IDS_SRC_AVI},
	{_T("CDDA (Audio CD)"), 0, SRC_CDDA, IDS_SRC_CDDA},
	{_T("CDXA (VCD/SVCD/XCD)"), 0, SRC_CDXA, IDS_SRC_CDXA},
	{_T("Dirac"), 0, SRC_DIRAC, IDS_SRC_DIRAC},
	{_T("DSM"), 0, SRC_DSM, 0/*IDS_SRC_DSM*/},
	{_T("DTS/AC3"), 0, SRC_DTSAC3, IDS_SRC_DTSAC3},
	{_T("DVD Video Title Set"), 0, SRC_VTS, IDS_SRC_VTS},
	{_T("DVD2AVI Project File"), 0, SRC_D2V, IDS_SRC_D2V},
	{_T("FLI/FLC"), 0, SRC_FLIC, IDS_SRC_FLIC},
	{_T("Matroska"), 0, SRC_MATROSKA, IDS_SRC_MATROSKA},
	{_T("MPEG Audio"), 0, SRC_MPA, 0/*IDS_SRC_MPA*/},
	{_T("MPEG PS/TS/PVA"), 0, SRC_MPEG, 0},
	{_T("Nut"), 0, SRC_NUT, IDS_SRC_NUT},
	{_T("Ogg"), 0, SRC_OGG, IDS_SRC_OGG},
	{_T("RealMedia"), 0, SRC_REALMEDIA, IDS_SRC_REALMEDIA},
	{_T("RoQ"), 0, SRC_ROQ, IDS_SRC_ROQ},
	{_T("SHOUTcast"), 0, SRC_SHOUTCAST, IDS_SRC_SHOUTCAST},
	__if_exists(CRadGtSplitterFilter) {{_T("Smacker/Bink"), 0, SRC_RADGT, IDS_SRC_RADGT},}
	{_T("AAC"), 1, TRA_AAC, IDS_TRA_AAC},
	{_T("AC3"), 1, TRA_AC3, IDS_TRA_AC3},
	{_T("DTS"), 1, TRA_DTS, IDS_TRA_DTS},
	{_T("Dirac"), 1, TRA_DIRAC, IDS_TRA_DIRAC},
	{_T("LPCM"), 1, TRA_LPCM, IDS_TRA_LPCM},
	{_T("MPEG Audio"), 1, TRA_MPA, IDS_TRA_MPA},
	{_T("MPEG-1 Video"), 1, TRA_MPEG1, IDS_TRA_MPEG1},
	{_T("MPEG-2 Video"), 1, TRA_MPEG2, IDS_TRA_MPEG2},
	{_T("PS2 Audio (PCM/ADPCM)"), 1, TRA_PS2AUD, IDS_TRA_PS2AUD},
	{_T("RealVideo"), 1, TRA_RV, IDS_TRA_RV},
	{_T("RealAudio"), 1, TRA_RA, IDS_TRA_RA},
};

IMPLEMENT_DYNAMIC(CPPageFiltersListBox, CCheckListBox)
CPPageFiltersListBox::CPPageFiltersListBox()
	: CCheckListBox()
{
}

void CPPageFiltersListBox::PreSubclassWindow()
{
	__super::PreSubclassWindow();
	EnableToolTips(TRUE);
}

INT_PTR CPPageFiltersListBox::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	BOOL b = FALSE;
	int row = ItemFromPoint(point, b);
	if(row < 0) return -1;

	CRect r;
	GetItemRect(row, r);
	pTI->rect = r;
	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)row;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->uFlags |= TTF_ALWAYSTIP;

	return pTI->uId;
}

BEGIN_MESSAGE_MAP(CPPageFiltersListBox, CCheckListBox)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
END_MESSAGE_MAP()

BOOL CPPageFiltersListBox::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	filter_t* f = (filter_t*)GetItemDataPtr(pNMHDR->idFrom);
	if(f->nHintID == 0) return FALSE;

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);

	static CStringA m_strTipTextA;
	static CStringW m_strTipTextW;
	
	m_strTipTextA = CString(MAKEINTRESOURCE(f->nHintID));
	m_strTipTextW = CString(MAKEINTRESOURCE(f->nHintID));

	if(pNMHDR->code == TTN_NEEDTEXTA) pTTTA->lpszText = (LPSTR)(LPCSTR)m_strTipTextA;
	else pTTTW->lpszText = (LPWSTR)(LPCWSTR)m_strTipTextW;

	*pResult = 0;

	return TRUE;    // message was handled
}

// CPPageFilters dialog

IMPLEMENT_DYNAMIC(CPPageFilters, CPPageBase)
CPPageFilters::CPPageFilters()
	: CPPageBase(CPPageFilters::IDD, CPPageFilters::IDD)
{
}

CPPageFilters::~CPPageFilters()
{
}


void CPPageFilters::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listSrc);
	DDX_Control(pDX, IDC_LIST2, m_listTra);
}

BEGIN_MESSAGE_MAP(CPPageFilters, CPPageBase)
END_MESSAGE_MAP()

// CPPageFilters message handlers

BOOL CPPageFilters::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	for(int i = 0; i < countof(s_filters); i++)
	{
		CCheckListBox* l = 
			s_filters[i].type == 0 ? &m_listSrc : 
			s_filters[i].type == 1 ? &m_listTra : 
			NULL; 

		UINT* pflags = 
			s_filters[i].type == 0 ? &s.SrcFilters : 
			s_filters[i].type == 1 ? &s.TraFilters : 
			NULL; 

		if(l && pflags)
		{
			int Index = l->AddString(s_filters[i].label);
			l->SetCheck(Index, !!(*pflags & s_filters[i].flag));
			l->SetItemDataPtr(Index, &s_filters[i]);
		}
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageFilters::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.SrcFilters = s.TraFilters = 0;

	CList<filter_t*> fl;
	for(int i = 0; i < m_listSrc.GetCount(); i++)
		if(m_listSrc.GetCheck(i))
			fl.AddTail((filter_t*)m_listSrc.GetItemDataPtr(i));
	for(int i = 0; i < m_listTra.GetCount(); i++)
		if(m_listTra.GetCheck(i))
			fl.AddTail((filter_t*)m_listTra.GetItemDataPtr(i));

	POSITION pos = fl.GetHeadPosition();
	while(pos)
	{
		filter_t* f = fl.GetNext(pos);

		UINT* pflags = 
			f->type == 0 ? &s.SrcFilters : 
			f->type == 1 ? &s.TraFilters : 
			NULL; 

		if(pflags)
			*pflags |= f->flag;
	}

	return __super::OnApply();
}

