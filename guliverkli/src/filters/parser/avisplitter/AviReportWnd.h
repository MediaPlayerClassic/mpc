#pragma once

#include "AviFile.h"

class CAviPlotterWnd : public CStatic
{
	DECLARE_DYNCREATE(CAviPlotterWnd)

private:
	CDC m_dc;
	CBitmap m_bm;

public:
	CAviPlotterWnd();
	bool Create(CAviFile* pAF, CRect r, CWnd* pParentWnd);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
};

class CAviReportWnd : public CWnd
{
	DECLARE_DYNCREATE(CAviReportWnd)

protected:
	CFont m_font;
	CStatic m_message;
	CButton m_checkbox;
	CAviPlotterWnd m_graph;

public:
	CAviReportWnd();
	bool DoModal(CAviFile* pAF, bool fHideChecked, bool fShowWarningText);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
};


