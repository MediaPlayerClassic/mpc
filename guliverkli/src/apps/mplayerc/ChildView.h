#pragma once

#include <atlimage.h>

class CChildView : public CWnd
{
	CRect m_vrect;

	DWORD m_lastlmdowntime;
	CPoint m_lastlmdownpoint;

	CImage m_logo;

public:
	CChildView();
	virtual ~CChildView();

	DECLARE_DYNAMIC(CChildView)

public:
	void SetVideoRect(CRect r = CRect(0,0,0,0));
	CRect GetVideoRect() {return(m_vrect);}

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnPlayPlayPauseStop(UINT nID);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();

	DECLARE_MESSAGE_MAP()
};
