// PPageFileInfoClip.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPageFileInfoClip.h"
#include <atlbase.h>
#include <qnetwork.h>
#include "..\..\DSUtil\DSUtil.h"

// CPPageFileInfoClip dialog

IMPLEMENT_DYNAMIC(CPPageFileInfoClip, CPropertyPage)
CPPageFileInfoClip::CPPageFileInfoClip(CString fn, IFilterGraph* pFG)
	: CPropertyPage(CPPageFileInfoClip::IDD, CPPageFileInfoClip::IDD)
	, m_fn(fn)
	, m_pFG(pFG)
	, m_clip(_T("None"))
	, m_author(_T("None"))
	, m_copyright(_T("None"))
	, m_rating(_T("None"))
	, m_location(_T("None"))
	, m_hIcon(NULL)
{
}

CPPageFileInfoClip::~CPPageFileInfoClip()
{
	if(m_hIcon) DestroyIcon(m_hIcon);
}

void CPPageFileInfoClip::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEFAULTICON, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_fn);
	DDX_Text(pDX, IDC_EDIT4, m_clip);
	DDX_Text(pDX, IDC_EDIT3, m_author);
	DDX_Text(pDX, IDC_EDIT2, m_copyright);
	DDX_Text(pDX, IDC_EDIT5, m_rating);
	DDX_Text(pDX, IDC_EDIT6, m_location);
	DDX_Control(pDX, IDC_EDIT7, m_desc);
}

BEGIN_MESSAGE_MAP(CPPageFileInfoClip, CPropertyPage)
END_MESSAGE_MAP()


// CPPageFileInfoClip message handlers

BOOL CPPageFileInfoClip::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	if(m_hIcon = LoadIcon(m_fn, false))
		m_icon.SetIcon(m_hIcon);

	m_fn.TrimRight('/');
	int i = max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));
	if(i >= 0 && i < m_fn.GetLength()-1)
	{
		m_location = m_fn.Left(i);
		m_fn = m_fn.Mid(i+1);

		if(m_location.GetLength() == 2 && m_location[1] == ':')
			m_location += '\\';
	}

	bool fEmpty = true;
	BeginEnumFilters(m_pFG, pEF, pBF)
	{
		if(CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF)
		{
			CComBSTR bstr;
			if(SUCCEEDED(pAMMC->get_Title(&bstr)) && wcslen(bstr.m_str) > 0) {m_clip = bstr.m_str; fEmpty = false;}
			if(SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && wcslen(bstr.m_str) > 0) {m_author = bstr.m_str; fEmpty = false;}
			if(SUCCEEDED(pAMMC->get_Copyright(&bstr)) && wcslen(bstr.m_str) > 0) {m_copyright = bstr.m_str; fEmpty = false;}
			if(SUCCEEDED(pAMMC->get_Rating(&bstr)) && wcslen(bstr.m_str) > 0) {m_rating = bstr.m_str; fEmpty = false;}
			if(SUCCEEDED(pAMMC->get_Description(&bstr)) && wcslen(bstr.m_str) > 0) {m_desc.SetWindowText(CString(bstr.m_str)); fEmpty = false;}
			if(!fEmpty) break;
		}
	}
	EndEnumFilters

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
