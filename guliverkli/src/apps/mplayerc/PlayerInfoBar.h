#pragma once

#include <atlcoll.h>
#include <afxtempl.h>
#include "StatusLabel.h"

// CPlayerInfoBar

class CPlayerInfoBar : public CDialogBar
{
	DECLARE_DYNAMIC(CPlayerInfoBar)

private:
	CAutoPtrArray<CStatusLabel> m_label;
	CAutoPtrArray<CStatusLabel> m_info;

	int m_nFirstColWidth;

	void Relayout();

public:
	CPlayerInfoBar(int nFirstColWidth = 100);
	virtual ~CPlayerInfoBar();

	BOOL Create(CWnd* pParentWnd);

	void SetLine(CString label, CString info);
	void RemoveLine(CString label);
	void RemoveAllLines();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
