#pragma once

#include "afxwin.h"
#include <afxtempl.h>
#include <atlcoll.h>

// COpenCapDeviceDlg dialog

class COpenCapDeviceDlg : public CDialog
{
	DECLARE_DYNAMIC(COpenCapDeviceDlg)

private:
	CStringArray m_vidnames, m_audnames;
//	CInterfaceArray<IMoniker> m_vidmonikers, m_audmonikers;

public:
	COpenCapDeviceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COpenCapDeviceDlg();

	CComboBox m_vidctrl;
	CComboBox m_audctrl;

//	CString m_vidfrstr, m_audfrstr;
	CString m_vidstr, m_audstr;
//	CComPtr<IBaseFilter> m_pVidCap, m_pAudCap;

// Dialog Data
	enum { IDD = IDD_OPENCAPDEVICEDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedOk();
};
