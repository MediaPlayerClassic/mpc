// SaveTextFileDialog.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "SaveTextFileDialog.h"


// CSaveTextFileDialog

IMPLEMENT_DYNAMIC(CSaveTextFileDialog, CFileDialog)
CSaveTextFileDialog::CSaveTextFileDialog(
	CTextFile::enc e,
	LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
	LPCTSTR lpszFilter, CWnd* pParentWnd) :
		CFileDialog(FALSE, lpszDefExt, lpszFileName, 
			OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
			lpszFilter, pParentWnd, 0),
		m_e(e)
{
	if(m_ofn.lStructSize == sizeof(OPENFILENAME))
	{
		SetTemplate(0, IDD_SAVETEXTFILEDIALOGTEMPL);
	}
	else /*if(m_ofn.lStructSize == OPENFILENAME_SIZE_VERSION_400)*/
	{
		SetTemplate(0, IDD_SAVETEXTFILEDIALOGTEMPL_400);
	}
}

CSaveTextFileDialog::~CSaveTextFileDialog()
{
}

void CSaveTextFileDialog::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_COMBO1, m_encoding);
	CFileDialog::DoDataExchange(pDX);
}

BOOL CSaveTextFileDialog::OnInitDialog()
{
	CFileDialog::OnInitDialog();

	m_encoding.AddString(_T("ANSI"));
	m_encoding.AddString(_T("Unicode 16-LE"));
	m_encoding.AddString(_T("Unicode 16-BE"));
	m_encoding.AddString(_T("UTF-8"));

	switch(m_e)
	{
	default:
	case CTextFile::ASCII: m_encoding.SetCurSel(0); break;
	case CTextFile::LE16: m_encoding.SetCurSel(1); break;
	case CTextFile::BE16: m_encoding.SetCurSel(2); break;
	case CTextFile::UTF8: m_encoding.SetCurSel(3); break;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CSaveTextFileDialog, CFileDialog)
END_MESSAGE_MAP()

// CSaveTextFileDialog message handlers

BOOL CSaveTextFileDialog::OnFileNameOK()
{
	switch(m_encoding.GetCurSel())
	{
	case 0: m_e = CTextFile::ASCII; break;
	case 1: m_e = CTextFile::LE16; break;
	case 2: m_e = CTextFile::BE16; break;
	case 3: m_e = CTextFile::UTF8; break;
	default: break;
	}

	return CFileDialog::OnFileNameOK();
}
