#pragma once

#include "..\..\subpic\ISubPic.h"
#include "afxwin.h"


// Q174667

class CShaderLabelComboBox : public CComboBox
{
public:
    CEdit m_edit;

	DECLARE_MESSAGE_MAP()
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();
};

// CShaderEditorDlg dialog

class CShaderEditorDlg : public CResizableDialog
{
private:
	CString m_label;
	CComPtr<ISubPicAllocatorPresenter> m_pCAP;

	struct shader_t {CString target, srcdata;};
	CMap<CString, LPCTSTR, shader_t, shader_t&> m_shaders;

	UINT m_nIDEventShader;

	bool m_fSplitterGrabbed;
	bool HitTestSplitter(CPoint p);

public:
	CShaderEditorDlg(CString label, ISubPicAllocatorPresenter* pCAP, CWnd* pParent = NULL);   // standard constructor
	virtual ~CShaderEditorDlg();

// Dialog Data
	enum { IDD = IDD_SHADEREDITORDIALOG };
	CShaderLabelComboBox m_labels;
	CComboBox m_targets;
	CEdit m_srcdata;
	CEdit m_output;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
