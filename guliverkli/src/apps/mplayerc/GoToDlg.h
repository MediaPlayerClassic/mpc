#pragma once
#include "afxwin.h"

// CGoToDlg dialog

class CGoToDlg : public CDialog
{
	DECLARE_DYNAMIC(CGoToDlg)

public:
	CGoToDlg(int time = -1, float fps = 0, CWnd* pParent = NULL);   // standard constructor
	virtual ~CGoToDlg();

	CString m_timestr;
	CString m_framestr;
	CEdit m_timeedit;
	CEdit m_frameedit;

	int m_time;
	float m_fps;

// Dialog Data
	enum { IDD = IDD_GOTODIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedOk2();
};
