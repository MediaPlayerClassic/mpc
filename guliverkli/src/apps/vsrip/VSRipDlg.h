#pragma once

#include <afxwin.h>
#include <afxtempl.h>
#include "VSRipFileDlg.h"
#include "VSRipPGCDlg.h"
#include "VSRipIndexingDlg.h"
#include "afxwin.h"

// CVSRipDlg dialog
class CVSRipDlg : public CDialog
{
// Construction
public:
	CVSRipDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CVSRipDlg();

// Dialog Data
	enum { IDD = IDD_VSRIP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();

// Implementation
protected:
	HICON m_hIcon;

	CStatic m_dlgrect;
	CStatic m_hdrline;

	CAutoPtrList<CVSRipPage> m_dlgs;
	POSITION m_dlgpos;
	void ShowNext(), ShowPrev();
	POSITION GetNext(), GetPrev();

	CComPtr<IVSFRipper> m_pVSFRipper;

	// Generated message map functions
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnPaint();
	afx_msg void OnKickIdle();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnPrev();
	afx_msg void OnUpdatePrev(CCmdUI* pCmdUI);
	afx_msg void OnNext();
	afx_msg void OnUpdateNext(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnUpdateClose(CCmdUI* pCmdUI);
	CStatic m_ftrline;
};
