#pragma once
#include "afxwin.h"

// CPPageFileInfoDetails dialog

class CPPageFileInfoDetails : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPageFileInfoDetails)

private:
	CComPtr<IFilterGraph> m_pFG;

	HICON m_hIcon;

public:
	CPPageFileInfoDetails(CString fn, IFilterGraph* pFG);
	virtual ~CPPageFileInfoDetails();

// Dialog Data
	enum { IDD = IDD_FILEPROPDETAILS };

	CStatic m_icon;
	CString m_fn;
	CString m_type;
	CString m_size;
	CString m_time;
	CString m_res;
	CString m_created;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
};
