#pragma once
#include "afxwin.h"

// CPPageFileInfoClip dialog

class CPPageFileInfoClip : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPageFileInfoClip)

private:
	CComPtr<IFilterGraph> m_pFG;

	HICON m_hIcon;

public:
	CPPageFileInfoClip(CString fn, IFilterGraph* pFG);
	virtual ~CPPageFileInfoClip();

// Dialog Data
	enum { IDD = IDD_FILEPROPCLIP };

	CStatic m_icon;
	CString m_fn;
	CString m_clip;
	CString m_author;
	CString m_copyright;
	CString m_rating;
	CString m_location;
	CEdit m_desc;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
};
