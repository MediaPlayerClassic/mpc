#pragma once


// CComPropertyPage dialog

class CComPropertyPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CComPropertyPage)

	CComPtr<IPropertyPage> m_pPage;

public:
	CComPropertyPage(IPropertyPage* pPage);
	virtual ~CComPropertyPage();

// Dialog Data
	enum { IDD = IDD_COMPROPERTYPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
};

