#pragma once

#include "CmdUIDialog.h"
#include "afxcmn.h"
#include "afxwin.h"

// CSaveDlg dialog

class CSaveDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CSaveDlg)

private:
	CString m_in, m_out;
	CComPtr<IGraphBuilder> pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaEventEx> pME;
	CComQIPtr<IMediaSeeking> pMS;
	UINT_PTR m_nIDTimerEvent;

public:
	CSaveDlg(CString in, CString out, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSaveDlg();

// Dialog Data
	enum { IDD = IDD_SAVEDIALOG };
	CAnimateCtrl m_anim;
	CProgressCtrl m_progress;
	CStatic m_report;
	CStatic m_fromto;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
};
