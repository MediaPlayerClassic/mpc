// PPageMouse.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageMouse.h"


// CPPageMouse dialog

IMPLEMENT_DYNAMIC(CPPageMouse, CPPageBase)
CPPageMouse::CPPageMouse()
	: CPPageBase(CPPageMouse::IDD, CPPageMouse::IDD)
	, m_list(0)
{
}

CPPageMouse::~CPPageMouse()
{
}

void CPPageMouse::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

CString CPPageMouse::GetCmdName(UINT cmd)
{
	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.Find(wmcmd(cmd));
	return pos ? s.wmcmds.GetAt(pos).name : _T("No Action");
}

BEGIN_MESSAGE_MAP(CPPageMouse, CPPageBase)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST1, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, OnEndlabeleditList)
END_MESSAGE_MAP()


// CPPageMouse message handlers

BOOL CPPageMouse::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_MouseCmds.RemoveAll();
	m_MouseCmds.AddTail(&s.MouseCmds);

	UpdateData(FALSE);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);

	for(int i = 0, j = m_list.GetHeaderCtrl()->GetItemCount(); i < j; i++) m_list.DeleteColumn(0);
	m_list.InsertColumn(COL_BTN, _T("Button"), LVCFMT_LEFT, 80);
	m_list.InsertColumn(COL_1CLK, _T("Single Click"), LVCFMT_LEFT, 120);
	m_list.InsertColumn(COL_2CLK, _T("Double Click"), LVCFMT_LEFT, 120);

	POSITION pos = m_MouseCmds.GetHeadPosition(), pos2;
    for(int i = 0; pos; i++)
	{
		MouseCmd& mc = m_MouseCmds.GetAt(pos);
		int row = m_list.InsertItem(m_list.GetItemCount(), mc.name, COL_BTN);
		m_list.SetItemData(row, (DWORD_PTR)pos);
		m_list.SetItemText(row, COL_1CLK, GetCmdName(mc.cmd1clk));
		m_list.SetItemText(row, COL_2CLK, GetCmdName(mc.cmd2clk));
		m_MouseCmds.GetNext(pos);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageMouse::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.MouseCmds.RemoveAll();
	for(int i = 0; i < m_list.GetItemCount(); i++)
		s.MouseCmds.AddTail(m_MouseCmds.GetAt((POSITION)m_list.GetItemData(i)));

	return __super::OnApply();
}

void CPPageMouse::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if(pItem->iItem < 0) 
		return;

	if(pItem->iSubItem == COL_1CLK || pItem->iSubItem == COL_2CLK)
	{
		*pResult = TRUE;
	}
}

void CPPageMouse::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if(pItem->iItem < 0) 
		return;

	MouseCmd& mc = m_MouseCmds.GetAt((POSITION)m_list.GetItemData(pItem->iItem));

	CStringList sl;
	int nSel = 0;

	sl.AddTail(GetCmdName(0));

	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.GetHeadPosition();
	for(int i = 1; pos; i++)
	{
		wmcmd& wc = s.wmcmds.GetNext(pos);
		sl.AddTail(wc.name);
		if(pItem->iSubItem == COL_1CLK && wc.cmd == mc.cmd1clk
		|| pItem->iSubItem == COL_2CLK && wc.cmd == mc.cmd2clk)
			nSel = i;
	}

	m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, sl, nSel);

	*pResult = TRUE;
}

void CPPageMouse::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if(!m_list.m_fInPlaceDirty)
		return;

	if(pItem->iItem < 0) 
		return;

	CUIntArray cmds;
	cmds.Add(0);

	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.GetHeadPosition();
	for(int i = 1; pos; i++)
		cmds.Add(s.wmcmds.GetNext(pos).cmd);

	MouseCmd& mc = m_MouseCmds.GetAt((POSITION)m_list.GetItemData(pItem->iItem));

	if(pItem->lParam >= 0 && pItem->lParam < cmds.GetCount())
	{
		UINT cmd = cmds[pItem->lParam];
		if(pItem->iSubItem == COL_1CLK) mc.cmd1clk = cmd;
		else if(pItem->iSubItem == COL_2CLK) mc.cmd2clk = cmd;
		m_list.SetItemText(pItem->iItem, pItem->iSubItem, pItem->pszText);
		*pResult = TRUE;
	}

	if(*pResult)
		SetModified();
}
