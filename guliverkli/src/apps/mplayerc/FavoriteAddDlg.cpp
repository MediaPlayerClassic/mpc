// FavoritAddDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "FavoriteAddDlg.h"


// CFavoriteAddDlg dialog

IMPLEMENT_DYNAMIC(CFavoriteAddDlg, CCmdUIDialog)
CFavoriteAddDlg::CFavoriteAddDlg(CString shortname, CString fullname, CWnd* pParent /*=NULL*/)
	: CCmdUIDialog(CFavoriteAddDlg::IDD, pParent)
	, m_shortname(shortname)
	, m_fullname(fullname)
	, m_fRememberPos(FALSE)
{
}

CFavoriteAddDlg::~CFavoriteAddDlg()
{
}

void CFavoriteAddDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_namectrl);
	DDX_CBString(pDX, IDC_COMBO1, m_name);	
	DDX_Check(pDX, IDC_CHECK1, m_fRememberPos);
}

BOOL CFavoriteAddDlg::OnInitDialog()
{
	__super::OnInitDialog();

	if(!m_shortname.IsEmpty()) m_namectrl.AddString(m_shortname);
	if(!m_fullname.IsEmpty()) m_namectrl.AddString(m_fullname);
	m_namectrl.SetCurSel(0);

	::CorrectComboListWidth(m_namectrl, GetFont());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(CFavoriteAddDlg, CCmdUIDialog)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
END_MESSAGE_MAP()


// CFavoriteAddDlg message handlers

void CFavoriteAddDlg::OnUpdateOk(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(!m_name.IsEmpty());
}
