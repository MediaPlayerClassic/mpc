#pragma once
#include "PPageBase.h"

// CPPageDVD dialog

class CPPageDVD : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageDVD)

private:
	void UpdateLCIDList();

public:
	CPPageDVD();
	virtual ~CPPageDVD();

	CListBox m_lcids;
	CString m_dvdpath;
	CEdit m_dvdpathctrl;
	CButton m_dvdpathselctrl;
	int m_iDVDLocation;
	int m_iDVDLangType;

	LCID m_idMenuLang;
	LCID m_idAudioLang;
	LCID m_idSubtitlesLang;

	BOOL m_fAutoSpeakerConf;

// Dialog Data
	enum { IDD = IDD_PPAGEDVD};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedLangradio123(UINT nID);
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnUpdateDVDPath(CCmdUI* pCmdUI);
};
