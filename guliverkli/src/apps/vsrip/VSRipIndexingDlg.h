#pragma once
#include "VSRipPage.h"

// CVSRipIndexingDlg dialog

class CVSRipIndexingDlg : public CVSRipPage
{
	DECLARE_DYNAMIC(CVSRipIndexingDlg)

private:
	BOOL m_bBeep, m_bExit;
	bool m_fAuto;

protected:
	// IVSFRipperCallback
	STDMETHODIMP OnMessage(LPCTSTR msg);
	STDMETHODIMP OnProgress(double progress /*0->1*/);
	STDMETHODIMP OnFinished(bool fSucceeded);

public:
	CVSRipIndexingDlg(IVSFRipper* pVSFRipper, CWnd* pParent = NULL);   // standard constructor
	virtual ~CVSRipIndexingDlg();

	virtual bool CanGoPrev() {return(S_OK != m_pVSFRipper->IsIndexing() && !m_fAuto);}
	virtual bool CanGoNext() {return(false);}
	virtual bool CanClose() {return(S_OK != m_pVSFRipper->IsIndexing());}
	virtual CString GetCloseText() {return(m_fFinished ? _T("&Close") : _T("&Cancel"));}
	virtual CString GetHeaderText() {return(_T("Extracting subtitles"));}
	virtual CString GetDescText() {return(_T("This may take a while, please sit back and relax... ")
										_T("If you wish you can abort the process and go back to ")
										_T("adjust the settings again."));}

	bool m_fFinished;

// Dialog Data
	enum { IDD = IDD_DIALOG_INDEXING };

	CEdit m_log;
	CProgressCtrl m_progress;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnIndex();
	afx_msg void OnUpdateIndex(CCmdUI* pCmdUI);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedCheck1();
};
