#pragma once

#include "resource.h"
#include "afxwin.h"

// GSCaptureDlg dialog

class GSCaptureDlg : public CDialog
{
	DECLARE_DYNAMIC(GSCaptureDlg)

private:
	struct Codec
	{
		CComPtr<IMoniker> pMoniker;
		CComPtr<IBaseFilter> pBF;
		CString FriendlyName;
		CComBSTR DisplayName;
	};

	CList<Codec> m_codecs;

	int GetSelCodec(Codec& c);

public:
	GSCaptureDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~GSCaptureDlg();

	CComPtr<IBaseFilter> m_pVidEnc;

// Dialog Data
	enum { IDD = IDD_CAPTURE };
	CString m_filename;
	CComboBox m_codeclist;

protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKickIdle();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedOk();
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
};
