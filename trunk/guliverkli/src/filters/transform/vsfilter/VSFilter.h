#pragma once

#include "resource.h"

class CVSFilterApp : public CWinApp
{
public:
	CVSFilterApp();

	CString m_AppName;

protected:
	HINSTANCE LoadAppLangResourceDLL();

public:
	BOOL InitInstance();
	BOOL ExitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CVSFilterApp theApp;

#define ResStr(id) CString(MAKEINTRESOURCE(id))
