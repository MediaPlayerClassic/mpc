// subresync.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "subresync.h"
#include "subresyncDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSubresyncApp

BEGIN_MESSAGE_MAP(CSubresyncApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CSubresyncApp construction

CSubresyncApp::CSubresyncApp()
{
}


// The one and only CSubresyncApp object

CSubresyncApp theApp;


// CSubresyncApp initialization

BOOL CSubresyncApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	SetRegistryKey(_T("Gabest"));

	CoInitialize(NULL);

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	CSubresyncDlg dlg(cmdInfo.m_strFileName);
	m_pMainWnd = &dlg;

	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CSubresyncApp::ExitInstance()
{
	CoUninitialize();

	return CWinApp::ExitInstance();
}
