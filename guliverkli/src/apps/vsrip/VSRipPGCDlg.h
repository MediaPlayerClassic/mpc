#pragma once
#include "VSRipPage.h"
#include "afxwin.h"


// CVSRipPGCDlg dialog

class CVSRipPGCDlg : public CVSRipPage
{
	DECLARE_DYNAMIC(CVSRipPGCDlg)

private:
	VSFRipperData m_rd;
	void SetupPGCList();
	void SetupAngleList();
	void SetupVCList();
	void SetupLangList();

public:
	CVSRipPGCDlg(IVSFRipper* pVSFRipper, CWnd* pParent = NULL);   // standard constructor
	virtual ~CVSRipPGCDlg();

	virtual void OnPrev();
	virtual void OnNext();
	virtual bool CanGoPrev() {return(true);}
	virtual bool CanGoNext();
	virtual CString GetHeaderText() {return(_T("Extraction settings"));}
	virtual CString GetDescText() {return(_T("Select the program chain and angle you did or ")
										_T("will do in the dvd ripper. Optionally, remove any not ")
										_T("needed language streams and vob/cell ids."));}

// Dialog Data
	enum { IDD = IDD_DIALOG_PGC };
	CListBox m_pgclist;
	CListBox m_anglelist;
	CListBox m_vclist;
	CListBox m_langlist;
	BOOL m_bResetTime;
	BOOL m_bClosedCaption;
	BOOL m_bForcedOnly;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnLbnSelchangeList2();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
