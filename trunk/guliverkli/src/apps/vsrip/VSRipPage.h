#pragma once

#include <afxtempl.h>
#include "..\..\subtitles\VobSubFileRipper.h"

// CVSRipPage dialog

class CVSRipPage : public CDialog, public IVSFRipperCallbackImpl
{
	DECLARE_DYNAMIC(CVSRipPage)

protected:
	CComPtr<IVSFRipper> m_pVSFRipper;

public:
	CVSRipPage(IVSFRipper* pVSFRipper, UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
	virtual ~CVSRipPage();

//	static bool ParseParamFile(CString fn);

	virtual void OnPrev() {}
	virtual void OnNext() {}
	virtual void OnClose() {}
	virtual bool CanGoPrev() {return(false);}
	virtual bool CanGoNext() {return(false);}
	virtual bool CanClose() {return(true);}
	virtual CString GetPrevText() {return(_T("< &Back"));}
	virtual CString GetNextText() {return(_T("&Next >"));}
	virtual CString GetCloseText() {return(_T("&Cancel"));}
	virtual CString GetHeaderText() {return(_T("Header Text"));}
	virtual CString GetDescText() {return(_T("Hello World"));}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
