#pragma once

// CCmdUIDialog dialog

class CCmdUIDialog : public CDialog
{
	DECLARE_DYNAMIC(CCmdUIDialog)

public:
	CCmdUIDialog(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
	virtual ~CCmdUIDialog();

protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnKickIdle();
};


// CCmdUIPropertyPage

class CCmdUIPropertyPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CCmdUIPropertyPage)

public:
	CCmdUIPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);   // standard constructor
	virtual ~CCmdUIPropertyPage();

protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnKickIdle();
};

