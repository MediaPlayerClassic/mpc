#pragma once

#include "ComPropertyPage.h"

interface IComPropertyPageDirty
{
	virtual void OnSetDirty(bool fDirty) = 0;
};

// CComPropertySheet

class CComPropertySheet : public CPropertySheet, public IComPropertyPageDirty
{
	DECLARE_DYNAMIC(CComPropertySheet)

	CComPtr<IPropertyPageSite> m_pSite;
	CInterfaceList<ISpecifyPropertyPages> m_spp;
	CAutoPtrList<CComPropertyPage> m_pages;
	CSize m_size;

public:
	CComPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CComPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CComPropertySheet();

	int AddPages(CComPtr<ISpecifyPropertyPages> pSPP);

	// CPropertySheet
	void OnSetDirty(bool fDirty) {if(CPropertyPage* p = GetActivePage()) p->SetModified(fDirty);}

	// IComPropertyPageDirty
	void OnActivated(CPropertyPage* pPage);

	virtual BOOL OnInitDialog();

protected:
	DECLARE_MESSAGE_MAP()
};


