#pragma once
#include "CmdUIDialog.h"
#include "FloatEdit.h"

// CPnSPresetsDlg dialog

class CPnSPresetsDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CPnSPresetsDlg)

private:
	void StringToParams(CString str, CString& label, double& PosX, double& PosY, double& ZoomX, double& ZoomY);
	CString ParamsToString(CString label, double PosX, double PosY, double ZoomX, double ZoomY);

public:
	CPnSPresetsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPnSPresetsDlg();

	CStringArray m_pnspresets;

// Dialog Data
	enum { IDD = IDD_PNSPRESETDIALOG };
	CFloatEdit m_PosX;
	CFloatEdit m_PosY;
	CFloatEdit m_ZoomX;
	CFloatEdit m_ZoomY;
	CString m_label;
	CListBox m_list;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton6();
	afx_msg void OnUpdateButton6(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton9();
	afx_msg void OnUpdateButton9(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton10();
	afx_msg void OnUpdateButton10(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateButton1(CCmdUI* pCmdUI);
};
