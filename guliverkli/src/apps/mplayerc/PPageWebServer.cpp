/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// PPageWebServer.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "MainFrm.h"
#include "PPageWebServer.h"
#include ".\ppagewebserver.h"

// CPPageWebServer dialog

IMPLEMENT_DYNAMIC(CPPageWebServer, CPPageBase)
CPPageWebServer::CPPageWebServer()
	: CPPageBase(CPPageWebServer::IDD, CPPageWebServer::IDD)
	, m_fEnableWebServer(FALSE)
	, m_nWebServerPort(0)
	, m_launch(_T("http://localhost:13579/"))
{
}

CPPageWebServer::~CPPageWebServer()
{
}

void CPPageWebServer::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_fEnableWebServer);
	DDX_Text(pDX, IDC_EDIT1, m_nWebServerPort);
	DDX_Control(pDX, IDC_EDIT1, m_nWebServerPortCtrl);
	DDX_Control(pDX, IDC_STATIC1, m_launch);
}

BOOL CPPageWebServer::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_LBUTTONDOWN && pMsg->hwnd == m_launch.m_hWnd)
	{
		UpdateData();

		AppSettings& s = AfxGetAppSettings();

		if(CMainFrame* pWnd = (CMainFrame*)AfxGetMainWnd())
		{
			if(m_fEnableWebServer)
			{
				if(s.nWebServerPort != m_nWebServerPort)
				{
					AfxMessageBox(_T("Press apply first, before testing the new settings!"), MB_OK);
					return TRUE;
				}
			}
		}
	}

	return CPPageBase::PreTranslateMessage(pMsg);
}

BOOL CPPageWebServer::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_fEnableWebServer = s.fEnableWebServer;
	m_nWebServerPort = s.nWebServerPort;

	UpdateData(FALSE);

	OnEnChangeEdit1();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageWebServer::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	if(CMainFrame* pWnd = (CMainFrame*)AfxGetMainWnd())
	{
		if(m_fEnableWebServer)
		{
			if(s.nWebServerPort != m_nWebServerPort) pWnd->StopWebServer(); // force restart, even if it already running
			pWnd->StartWebServer(m_nWebServerPort);
		}
		else
		{
			pWnd->StopWebServer();
		}
	}

	s.fEnableWebServer = !!m_fEnableWebServer;
	s.nWebServerPort = m_nWebServerPort;

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageWebServer, CPPageBase)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
END_MESSAGE_MAP()


// CPPageWebServer message handlers


void CPPageWebServer::OnEnChangeEdit1()
{
	UpdateData();

	CString link;
	link.Format(_T("http://localhost:%d/"), m_nWebServerPort);
	m_launch.m_link = link;

	SetModified();
}
