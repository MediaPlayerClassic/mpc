#pragma once

#include "PPageFileInfoClip.h"
#include "PPageFileInfoDetails.h"

interface IFilterGraph;

// CPPageFileInfoSheet

class CPPageFileInfoSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPPageFileInfoSheet)

private:
	CPPageFileInfoClip m_clip;
	CPPageFileInfoDetails m_details;

public:
	CPPageFileInfoSheet(CString fn, IFilterGraph* pFG, CWnd* pParentWnd = NULL);
	virtual ~CPPageFileInfoSheet();

protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};


