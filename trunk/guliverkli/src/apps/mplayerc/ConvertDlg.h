#pragma once
#include "afxcmn.h"
#include "..\..\DSUtil\DSMPropertyBag.h"

// CConvertDlg dialog

class CConvertDlg : public CResizableDialog
{
private:
	DWORD m_dwRegister;
	CComPtr<ICaptureGraphBuilder2> m_pCGB;
	CComPtr<IGraphBuilder> m_pGB;
	CComPtr<IBaseFilter> m_pMux;
	CComQIPtr<IMediaControl> m_pMC;
	CComQIPtr<IMediaEventEx> m_pME;
	CComQIPtr<IMediaSeeking> m_pMS;

	CString m_title;
	UINT m_nIDEventStatus;

	CBitmap m_streamtypesbm;
	CImageList m_streamtypes;

	void AddFile(CString fn);
	void AddFilter(HTREEITEM hTI, IBaseFilter* pBF);
	void DeleteFilter(IBaseFilter* pBF);
	void DeleteTreeNode(HTREEITEM hTI);
	void UpdateTreeNode(HTREEITEM hTI);

	HTREEITEM HitTest(CPoint& sp, CPoint& cp);

	void ShowPopup(CPoint p);
	void ShowFilePopup(HTREEITEM hTI, CPoint p);
	void ShowPinPopup(HTREEITEM hTI, CPoint p);

	void EditProperties(IDSMPropertyBag* pPB);

public:
	CConvertDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConvertDlg();

// Dialog Data
	enum { IDD = IDD_CONVERT_DIALOG };
	CTreeCtrl m_tree;
	CString m_fn;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnClose();
	afx_msg void OnNMClickTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateButton1(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton3(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButton4(CCmdUI* pCmdUI);
};
