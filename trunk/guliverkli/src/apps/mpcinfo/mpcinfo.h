// mpcinfo.h : main header file for the mpcinfo DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CmpcinfoApp
// See mpcinfo.cpp for the implementation of this class
//

class CmpcinfoApp : public CWinApp
{
public:
	CmpcinfoApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
