#pragma once
#include "..\..\DSUtil\DSMPropertyBag.h"
#include "afxwin.h"
#include "afxcmn.h"

// CConvertPropsDlg dialog

class CConvertPropsDlg : public CResizableDialog
{
private:
	bool m_fPin;
	void SetItem(CString key, CString value);

public:
	CConvertPropsDlg(bool fPin, CWnd* pParent = NULL);   // standard constructor
	virtual ~CConvertPropsDlg();

	CAtlMap<CString, CString, CStringElementTraits<CString> > m_props;

// Dialog Data
	enum { IDD = IDD_CONVERTPROPSDIALOG };
	CComboBox m_fcc;
	CEdit m_text;
	CListCtrl m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnCbnEditchangeCombo1();
	afx_msg void OnCbnSelchangeCombo1();
};
