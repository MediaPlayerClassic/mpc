#pragma once
#include "PPageBase.h"
#include "afxwin.h"

// CPPageFormats dialog

class CPPageFormats : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageFormats)

private:
	CImageList m_onoff;

	int GetChecked(int iItem);
	void SetChecked(int iItem, int fChecked);

	bool IsRegistered(CString ext);
	bool RegisterExt(CString ext, bool fRegister);

	bool m_fXpOrBetter;
	typedef enum {AP_VIDEO=0,AP_MUSIC,AP_AUDIOCD,AP_DVDMOVIE} autoplay_t;
	void AddAutoPlayToRegistry(autoplay_t ap, bool fRegister);
	bool IsAutoPlayRegistered(autoplay_t ap);

	void SetListItemState(int nItem);

public:
	CPPageFormats();
	virtual ~CPPageFormats();

	CListCtrl m_list;
	CString m_exts;
	CStatic m_autoplay;
	CButton m_apvideo;
	CButton m_apmusic;
	CButton m_apaudiocd;
	CButton m_apdvd;

// Dialog Data
	enum { IDD = IDD_PPAGEFORMATS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton14();
	afx_msg void OnBnClickedButton13();
	afx_msg void OnBnClickedButton12();
	afx_msg void OnBnClickedButton11();
};
