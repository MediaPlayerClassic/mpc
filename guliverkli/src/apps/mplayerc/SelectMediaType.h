#pragma once

#include "CmdUIDialog.h"
#include "afxwin.h"

// CSelectMediaType dialog

class CSelectMediaType : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CSelectMediaType)

private:
	CArray<GUID>& m_guids;

public:
	CSelectMediaType(CArray<GUID>& guids, GUID guid, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectMediaType();

	GUID m_guid;

// Dialog Data
	enum { IDD = IDD_SELECTMEDIATYPE };
	CString m_guidstr;
	CComboBox m_guidsctrl;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnEditchangeCombo1();
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
};
