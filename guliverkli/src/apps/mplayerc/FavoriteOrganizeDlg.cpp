// FavoriteOrganizeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "FavoriteOrganizeDlg.h"


// CFavoriteOrganizeDlg dialog

IMPLEMENT_DYNAMIC(CFavoriteOrganizeDlg, CDialog)
CFavoriteOrganizeDlg::CFavoriteOrganizeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFavoriteOrganizeDlg::IDD, pParent)
{
}

CFavoriteOrganizeDlg::~CFavoriteOrganizeDlg()
{
}

void CFavoriteOrganizeDlg::SetupList(bool fSave)
{
	int i = m_tab.GetCurSel();

	if(fSave)
	{
		CStringList sl;

		for(int j = 0; j < m_list.GetItemCount(); j++)
		{
			CString desc = m_list.GetItemText(j, 0);
			desc.Remove(';');
			CString str = m_sl[i].GetAt((POSITION)m_list.GetItemData(j));
			sl.AddTail(desc + str.Mid(str.Find(';')));
		}

		m_sl[i].RemoveAll();
		m_sl[i].AddTail(&sl);
	}
	else
	{
		m_list.DeleteAllItems();

		POSITION pos = m_sl[i].GetHeadPosition(), tmp;
		while(pos) 
		{
			tmp = pos;
			CString s = m_sl[i].GetNext(pos);
			int i = s.Find(';');
			if(i >= 0) s = s.Left(i);
			m_list.SetItemData(m_list.InsertItem(m_list.GetItemCount(), s), (DWORD_PTR)tmp);
		}

		m_list.SetRedraw(FALSE);

		CRect r;
		m_list.GetClientRect(r);
		m_list.SetColumnWidth(0, -1);
		m_list.SetColumnWidth(0, max(m_list.GetColumnWidth(0), r.Size().cx));

		m_list.SetRedraw(TRUE);
	}
}

void CFavoriteOrganizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tab);
	DDX_Control(pDX, IDC_LIST2, m_list);
}


BEGIN_MESSAGE_MAP(CFavoriteOrganizeDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnTcnSelchangeTab1)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON7, OnBnClickedButton7)
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB1, OnTcnSelchangingTab1)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST2, OnLvnEndlabeleditList2)
END_MESSAGE_MAP()


// CFavoriteOrganizeDlg message handlers

BOOL CFavoriteOrganizeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_tab.InsertItem(0, _T("Files"));
	m_tab.InsertItem(1, _T("DVDs"));
//	m_tab.InsertItem(2, _T("Devices"));
	m_tab.SetCurSel(0);

	m_list.InsertColumn(0, _T(""));
	m_list.SetExtendedStyle(m_list.GetExtendedStyle()|LVS_EX_FULLROWSELECT);

	AfxGetAppSettings().GetFav(FAV_FILE, m_sl[0]);
	AfxGetAppSettings().GetFav(FAV_DVD, m_sl[1]);
	AfxGetAppSettings().GetFav(FAV_DEVICE, m_sl[2]);
	
	SetupList(false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFavoriteOrganizeDlg::OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
	SetupList(false);

	m_list.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

	*pResult = 0;
}

void CFavoriteOrganizeDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if(nIDCtl != IDC_LIST2) return;

	int nItem = lpDrawItemStruct->itemID;
	CRect rcItem = lpDrawItemStruct->rcItem;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	if(!!m_list.GetItemState(nItem, LVIS_SELECTED))
	{
		FillRect(pDC->m_hDC, rcItem, CBrush(0xf1dacc));
		FrameRect(pDC->m_hDC, rcItem, CBrush(0xc56a31));
	}
	else
	{
		CBrush b;
		b.CreateSysColorBrush(COLOR_WINDOW);
		FillRect(pDC->m_hDC, rcItem, b);
	}

	CString str = m_list.GetItemText(nItem, 0);

	pDC->SetTextColor(0);
	pDC->TextOut(rcItem.left+3, (rcItem.top+rcItem.bottom - pDC->GetTextExtent(str).cy)/2, str);
}

void CFavoriteOrganizeDlg::OnBnClickedButton1()
{
	if(POSITION pos = m_list.GetFirstSelectedItemPosition())
	{
		m_list.SetFocus();
		m_list.EditLabel(m_list.GetNextSelectedItem(pos));
	}
}

void CFavoriteOrganizeDlg::OnLvnEndlabeleditList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if(pDispInfo->item.iItem >= 0 && pDispInfo->item.pszText)
		m_list.SetItemText(pDispInfo->item.iItem, 0, pDispInfo->item.pszText);
	*pResult = 0;
}

void CFavoriteOrganizeDlg::OnBnClickedButton2()
{
	if(POSITION pos = m_list.GetFirstSelectedItemPosition())
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		if(nItem < 0 || nItem >= m_list.GetItemCount()) return;

		m_list.DeleteItem(nItem);

		nItem = min(nItem, m_list.GetItemCount()-1);

		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CFavoriteOrganizeDlg::OnBnClickedButton3()
{
	if(POSITION pos = m_list.GetFirstSelectedItemPosition())
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		if(nItem <= 0) return;

		DWORD_PTR data = m_list.GetItemData(nItem);
		CString str = m_list.GetItemText(nItem, 0);

		m_list.DeleteItem(nItem);

		nItem--;

		m_list.InsertItem(nItem, str);
		m_list.SetItemData(nItem, data);
		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CFavoriteOrganizeDlg::OnBnClickedButton7()
{
	if(POSITION pos = m_list.GetFirstSelectedItemPosition())
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		if(nItem < 0 || nItem >= m_list.GetItemCount()-1) return;

		DWORD_PTR data = m_list.GetItemData(nItem);
		CString str = m_list.GetItemText(nItem, 0);

		m_list.DeleteItem(nItem);

		nItem++;

		m_list.InsertItem(nItem, str);
		m_list.SetItemData(nItem, data);
		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CFavoriteOrganizeDlg::OnTcnSelchangingTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	SetupList(true);

	*pResult = 0;
}

void CFavoriteOrganizeDlg::OnBnClickedOk()
{
	SetupList(true);

	AfxGetAppSettings().SetFav(FAV_FILE, m_sl[0]);
	AfxGetAppSettings().SetFav(FAV_DVD, m_sl[1]);
	AfxGetAppSettings().SetFav(FAV_DEVICE, m_sl[2]);

	OnOK();
}

void CFavoriteOrganizeDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	if(nState == WA_ACTIVE)
		m_list.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
}
