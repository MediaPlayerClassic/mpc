#pragma once

#include "CmdUIDialog.h"

// CPPageBase dialog

class CPPageBase : public CCmdUIPropertyPage
{
	DECLARE_DYNAMIC(CPPageBase)

public:
	CPPageBase(UINT nIDTemplate, UINT nIDCaption = 0);
	virtual ~CPPageBase();

// Dialog Data

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnSetActive();
};
