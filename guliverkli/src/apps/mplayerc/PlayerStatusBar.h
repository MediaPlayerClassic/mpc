#pragma once

#include "StatusLabel.h"

// CPlayerStatusBar

class CPlayerStatusBar : public CDialogBar
{
	DECLARE_DYNAMIC(CPlayerStatusBar)

	CStatic m_type;
	CStatusLabel m_status, m_time;
	CBitmap m_bm;
	UINT m_bmid;
	HICON m_hIcon;

	void Relayout();

public:
	CPlayerStatusBar();
	virtual ~CPlayerStatusBar();

	void Clear();

	void SetStatusBitmap(UINT id);
	void SetStatusTypeIcon(HICON hIcon);
	void SetStatusMessage(CString str);
	void SetStatusTimer(CString str);

	void ShowTimer(bool fShow);

// Overrides
	virtual BOOL Create(CWnd* pParentWnd);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	DECLARE_MESSAGE_MAP()

protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
