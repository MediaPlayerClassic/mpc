#include "stdafx.h"
#include "GSdx9.h"
#include "GSSettingsDlg.h"

IMPLEMENT_DYNAMIC(CGSSettingsDlg, CDialog)
CGSSettingsDlg::CGSSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGSSettingsDlg::IDD, pParent)
	, m_fDisableShaders(FALSE)
	, m_fHalfVRes(FALSE)
{
}

CGSSettingsDlg::~CGSSettingsDlg()
{
}

void CGSSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_fDisableShaders);
	DDX_Check(pDX, IDC_CHECK3, m_fHalfVRes);
}

BEGIN_MESSAGE_MAP(CGSSettingsDlg, CDialog)
END_MESSAGE_MAP()

// CGSSettingsDlg message handlers

BOOL CGSSettingsDlg::OnInitDialog()
{
	__super::OnInitDialog();

    CWinApp* pApp = AfxGetApp();

	m_fDisableShaders = !!pApp->GetProfileInt(_T("Settings"), _T("DisableShaders"), FALSE);
	m_fHalfVRes = !!pApp->GetProfileInt(_T("Settings"), _T("HalfVRes"), FALSE);
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGSSettingsDlg::OnOK()
{
	CWinApp* pApp = AfxGetApp();

	UpdateData();
	pApp->WriteProfileInt(_T("Settings"), _T("DisableShaders"), m_fDisableShaders);
	pApp->WriteProfileInt(_T("Settings"), _T("HalfVRes"), m_fHalfVRes);

	__super::OnOK();
}
