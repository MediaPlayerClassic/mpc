#pragma once

#include "resource.h"

class CGSSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CGSSettingsDlg)

public:
	CGSSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGSSettingsDlg();

// Dialog Data
	enum { IDD = IDD_CONFIG };
	BOOL m_fDisableShaders;
	BOOL m_fHalfVRes;
	BOOL m_fSoftRenderer;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
