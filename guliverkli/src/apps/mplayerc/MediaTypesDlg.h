#pragma once

#include <afxwin.h>
#include <atlcoll.h>
#include "CmdUIDialog.h"
#include "GraphBuilder.h"

// CMediaTypesDlg dialog

class CMediaTypesDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CMediaTypesDlg)

private:
	CArray<CGraphBuilder::DeadEnd*> m_DeadEnds;
	enum {UNKNOWN, VIDEO, AUDIO} m_type;
	GUID m_subtype;
	void AddLine(CString str = _T("\n"));
	void AddMediaType(AM_MEDIA_TYPE* pmt);

public:
	CMediaTypesDlg(CGraphBuilder& gb, CWnd* pParent = NULL);   // standard constructor
	virtual ~CMediaTypesDlg();

// Dialog Data
	enum { IDD = IDD_MEDIATYPESDIALOG };
	CComboBox m_pins;
	CEdit m_report;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateButton1(CCmdUI* pCmdUI);
};
