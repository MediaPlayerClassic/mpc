#pragma once

#include "..\..\ui\sizecbar\scbarg.h"
#include "PlayerCaptureDialog.h"

#ifndef baseCPlayerCaptureBar
#define baseCPlayerCaptureBar CSizingControlBarG
#endif

// CPlayerCaptureBar

class CPlayerCaptureBar : public baseCPlayerCaptureBar
{
	DECLARE_DYNAMIC(CPlayerCaptureBar)

public:
	CPlayerCaptureBar();
	virtual ~CPlayerCaptureBar();

	BOOL Create(CWnd* pParentWnd);

public:
	CPlayerCaptureDialog m_capdlg;

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
};
