/* 
 *	Copyright (C) 2003-2004 Gabest
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

// mplayerc.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "mplayerc.h"
#include <atlsync.h>
#include <Tlhelp32.h>
#include "MainFrm.h"
#include "..\..\DSUtil\DSUtil.h"

/////////

void CorrectComboListWidth(CComboBox& box, CFont* pWndFont)
{
	int cnt = box.GetCount();
	if(cnt <= 0) return;

	CDC* pDC = box.GetDC();
	pDC->SelectObject(pWndFont);

	int maxw = box.GetDroppedWidth();

	for(int i = 0; i < cnt; i++)
	{
		CString str;
		box.GetLBText(i, str);
		int w = pDC->GetTextExtent(str).cx + 22;
		if(maxw < w) maxw = w;
	}

	box.ReleaseDC(pDC);

	box.SetDroppedWidth(maxw);
}

HICON LoadIcon(CString fn, bool fSmall)
{
	if(fn.IsEmpty()) return(NULL);

	CString ext = fn.Left(fn.Find(_T("://"))+1).TrimRight(':');
	if(ext.IsEmpty() || !ext.CompareNoCase(_T("file")))
		ext = _T(".") + fn.Mid(fn.ReverseFind('.')+1);

	CSize size(fSmall?16:32,fSmall?16:32);

	if(!ext.CompareNoCase(_T(".ifo")))
	{
		if(HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DVD), IMAGE_ICON, size.cx, size.cy, 0))
			return(hIcon);
	}

	if(!ext.CompareNoCase(_T(".cda")))
	{
		if(HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUDIOCD), IMAGE_ICON, size.cx, size.cy, 0))
			return(hIcon);
	}

	do
	{
		CRegKey key;

		TCHAR buff[256];
		ULONG len;

		if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + _T("\\DefaultIcon"), KEY_READ))
		{
			if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext, KEY_READ))
				break;

			len = sizeof(buff);
			memset(buff, 0, len);
			if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (ext = buff).Trim().IsEmpty())
				break;

			if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + _T("\\DefaultIcon"), KEY_READ))
				break;
		}

		CString icon;

		len = sizeof(buff);
		memset(buff, 0, len);
		if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (icon = buff).Trim().IsEmpty())
			break;

		int i = icon.ReverseFind(',');
		if(i < 0) break;
		
		int id = 0;
		if(_stscanf(icon.Mid(i+1), _T("%d"), &id) != 1)
			break;

		icon = icon.Left(i);

		HICON hIcon = NULL;
		UINT cnt = fSmall 
			? ExtractIconEx(icon, id, NULL, &hIcon, 1)
			: ExtractIconEx(icon, id, &hIcon, NULL, 1);
		if(hIcon) return hIcon;
	}
	while(0);

	return((HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_UNKNOWN), IMAGE_ICON, size.cx, size.cy, 0));
}

bool LoadType(CString fn, CString& type)
{
	CRegKey key;

	TCHAR buff[256];
	ULONG len;

	if(fn.IsEmpty()) return(NULL);

	CString ext = fn.Left(fn.Find(_T("://"))+1).TrimRight(':');
	if(ext.IsEmpty() || !ext.CompareNoCase(_T("file")))
		ext = _T(".") + fn.Mid(fn.ReverseFind('.')+1);

	if(ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext))
		return(false);

	CString tmp = ext;

    while(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, tmp))
	{
		len = sizeof(buff);
		memset(buff, 0, len);
		if(ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (tmp = buff).Trim().IsEmpty())
			return(false);
	}

	type = tmp;

	return(true);
}

bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype)
{
	str.Empty();
	HRSRC hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(resid), restype);
	if(!hrsrc) return(false);
	HGLOBAL hGlobal = LoadResource(AfxGetResourceHandle(), hrsrc);
	if(!hGlobal) return(false);
	DWORD size = SizeofResource(AfxGetResourceHandle(), hrsrc);
	if(!size) return(false);
	memcpy(str.GetBufferSetLength(size), LockResource(hGlobal), size);
	return(true);
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_appname;
	virtual BOOL OnInitDialog()
	{
#ifdef UNICODE
		UpdateData();
		m_appname += _T(" (unicode build)");
		UpdateData(FALSE);
#endif
		return TRUE;
	}
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD), m_appname(_T(""))
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_STATIC1, m_appname);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp

BEGIN_MESSAGE_MAP(CMPlayerCApp, CWinApp)
	//{{AFX_MSG_MAP(CMPlayerCApp)
	ON_COMMAND(ID_HELP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_SHOWCOMMANDLINESWITCHES, OnHelpShowcommandlineswitches)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp construction

CMPlayerCApp::CMPlayerCApp()
//	: m_hMutexOneInstance(NULL)
{
}

void CMPlayerCApp::ShowCmdlnSwitches()
{
	CString s;

	if(m_s.nCLSwitches&CLSW_UNRECOGNIZEDSWITCH)
	{
		CList<CString> sl;
		for(int i = 0; i < __argc; i++) sl.AddTail(__targv[i]);
		s += "Unrecognized switch(es) found in command line string: \n\n" + Implode(sl, ' ') + "\n\n";
	}

	s +=
		_T("Usage: mplayerc.exe \"pathname\" [switches]\n\n")
		_T("\"pathname\"\tThe main file or directory to be loaded. (wildcards allowed)\n")
		_T("/dub \"dubname\"\tLoad an additional audio file.\n")
		_T("/sub \"subname\"\tLoad an additional subtitle file.\n")
		_T("/filter \"filtername\"\tLoad DirectShow filters from a dynamic link library. (wildcards allowed)\n")
		_T("/dvd\t\tRun in dvd mode, \"pathname\" means the dvd folder (optional).\n")
		_T("/cd\t\tLoad all the tracks of an audio cd or (s)vcd, \"pathname\" means the drive path (optional).\n")
		_T("/open\t\tOpen the file, don't automatically start playing.\n")
		_T("/play\t\tStart playing the file as soon the player is launched.\n")
		_T("/close\t\tClose the player after playback (only works when used with /play).\n")
		_T("/shutdown\tShutdown the operating system after playback\n")
		_T("/fullscreen\tStart in full-screen mode.\n")
		_T("/minimized\tStart in minimized mode.\n")
		_T("/new\t\tUse a new instance of the player.\n")
		_T("/add\t\tAdd \"pathname\" to playlist, can be combined with /open and /play.\n")
		_T("/regvid\t\tRegister video formats\n")
		_T("/regaud\t\tRegister audio formats\n")
		_T("/unregvid\t\tUnregister video formats\n")
		_T("/unregaud\tUnregister audio formats\n")
		_T("/start ms\t\tStart playing at \"ms\" (= milliseconds)\n")
		_T("/help /h /?\tShow help about command line switches. (this message box)\n");

	AfxMessageBox(s);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMPlayerCApp object

CMPlayerCApp theApp;

HWND g_hWnd = NULL;

bool CMPlayerCApp::StoreSettingsToIni()
{
	CString ini = GetIniPath();
/*
	FILE* f;
	if(!(f = _tfopen(ini, _T("r+"))) && !(f = _tfopen(ini, _T("w"))))
		return StoreSettingsToRegistry();
	fclose(f);
*/
	if(m_pszRegistryKey) free((void*)m_pszRegistryKey);
	m_pszRegistryKey = NULL;
	if(m_pszProfileName) free((void*)m_pszProfileName);
	m_pszProfileName = _tcsdup(ini);

	return(true);
}

bool CMPlayerCApp::StoreSettingsToRegistry()
{
	_tremove(GetIniPath());

	if(m_pszRegistryKey) free((void*)m_pszRegistryKey);
	m_pszRegistryKey = NULL;

	SetRegistryKey(_T("Gabest"));

	return(true);
}

CString CMPlayerCApp::GetIniPath()
{
	CString path;
	GetModuleFileName(AfxGetInstanceHandle(), path.GetBuffer(MAX_PATH), MAX_PATH);
	path.ReleaseBuffer();
	path = path.Left(path.ReverseFind('.')+1) + _T("ini");
	return(path);
}

bool CMPlayerCApp::IsIniValid()
{
	CFileStatus fs;
	return CFile::GetStatus(GetIniPath(), fs) && fs.m_size > 0;
}

void CMPlayerCApp::PreProcessCommandLine()
{
	m_cmdln.RemoveAll();
	for(int i = 1; i < __argc; i++)
	{
		CString str = CString(__targv[i]).Trim(_T(" \""));

		if(str[0] != '/' && str[0] != '-' && str.Find(_T(":")) < 0)
		{
			LPTSTR p = NULL;
			CString str2;
			str2.ReleaseBuffer(GetFullPathName(str, MAX_PATH, str2.GetBuffer(MAX_PATH), &p));
			CFileStatus fs;
			if(!str2.IsEmpty() && CFile::GetStatus(str2, fs)) str = str2;
		}

		m_cmdln.AddTail(str);
	}
}

void CMPlayerCApp::SendCommandLine(HWND hWnd)
{
	if(m_cmdln.IsEmpty())
		return;

	int bufflen = sizeof(DWORD);

	POSITION pos = m_cmdln.GetHeadPosition();
	while(pos) bufflen += (m_cmdln.GetNext(pos).GetLength()+1)*sizeof(TCHAR);

	CAutoVectorPtr<BYTE> buff;
	if(!buff.Allocate(bufflen))
		return;

	BYTE* p = buff;

	*(DWORD*)p = m_cmdln.GetCount(); 
	p += sizeof(DWORD);

	pos = m_cmdln.GetHeadPosition();
	while(pos)
	{
		CString s = m_cmdln.GetNext(pos); 
		int len = (s.GetLength()+1)*sizeof(TCHAR);
		memcpy(p, s, len);
		p += len;
	}

	COPYDATASTRUCT cds;
	cds.dwData = 0x6ABE51;
	cds.cbData = bufflen;
	cds.lpData = (void*)(BYTE*)buff;
	SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp initialization

#include "..\..\..\include\detours\detours.h"

DETOUR_TRAMPOLINE(BOOL WINAPI Real_IsDebuggerPresent(), IsDebuggerPresent);
BOOL WINAPI Mine_IsDebuggerPresent()
{
	TRACE(_T("Oops, somebody was trying to be naughty! (called IsDebuggerPresent)\n")); 
	return FALSE;
}

DETOUR_TRAMPOLINE(LONG WINAPI Real_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam), ChangeDisplaySettingsExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam), ChangeDisplaySettingsExW);
LONG WINAPI Mine_ChangeDisplaySettingsEx(LONG ret, DWORD dwFlags, LPVOID lParam)
{
	if(dwFlags&CDS_VIDEOPARAMETERS)
	{
		VIDEOPARAMETERS* vp = (VIDEOPARAMETERS*)lParam;

		if(vp->Guid == GUIDFromCString(_T("{02C62061-1097-11d1-920F-00A024DF156E}"))
		&& (vp->dwFlags&VP_FLAGS_COPYPROTECT))
		{
			if(vp->dwCommand == VP_COMMAND_GET)
			{
				if((vp->dwTVStandard&VP_TV_STANDARD_WIN_VGA)
				&& vp->dwTVStandard != VP_TV_STANDARD_WIN_VGA)
				{
					TRACE(_T("Ooops, tv-out enabled? macrovision checks suck..."));
					vp->dwTVStandard = VP_TV_STANDARD_WIN_VGA;
				}
			}
			else if(vp->dwCommand == VP_COMMAND_SET)
			{
				TRACE(_T("Ooops, as I already told ya, no need for any macrovision bs here"));
				return 0;
			}
		}
	}

	return ret;
}
LONG WINAPI Mine_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
		Real_ChangeDisplaySettingsExA(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), 
		dwFlags, 
		lParam);
}
LONG WINAPI Mine_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
		Real_ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), 
		dwFlags,
		lParam);
}

DETOUR_TRAMPOLINE(HANDLE WINAPI Real_CreateFileA(LPCSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7), CreateFileA);
DETOUR_TRAMPOLINE(HANDLE WINAPI Real_CreateFileW(LPCWSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7), CreateFileW);
HANDLE WINAPI Mine_CreateFileA(LPCSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	CStringA fn(p1);
	fn.MakeLower();
	int i = fn.Find(".part");
	if(i > 0 && i == fn.GetLength() - 5)
		p3 |= FILE_SHARE_WRITE;

	return Real_CreateFileA(p1, p2, p3, p4, p5, p6, p7);
}
HANDLE WINAPI Mine_CreateFileW(LPCWSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	CStringW fn(p1);
	fn.MakeLower();
	int i = fn.Find(L".part");
	if(i > 0 && i == fn.GetLength() - 5)
		p3 |= FILE_SHARE_WRITE;

	return Real_CreateFileW(p1, p2, p3, p4, p5, p6, p7);
}

BOOL CMPlayerCApp::InitInstance()
{
	DetourFunctionWithTrampoline((PBYTE)Real_IsDebuggerPresent, (PBYTE)Mine_IsDebuggerPresent);
	DetourFunctionWithTrampoline((PBYTE)Real_ChangeDisplaySettingsExA, (PBYTE)Mine_ChangeDisplaySettingsExA);
	DetourFunctionWithTrampoline((PBYTE)Real_ChangeDisplaySettingsExW, (PBYTE)Mine_ChangeDisplaySettingsExW);
	DetourFunctionWithTrampoline((PBYTE)Real_CreateFileA, (PBYTE)Mine_CreateFileA);
	DetourFunctionWithTrampoline((PBYTE)Real_CreateFileW, (PBYTE)Mine_CreateFileW);
	CFilterMapper2::Init();

	HRESULT hr;
    if(FAILED(hr = OleInitialize(0)))
	{
        AfxMessageBox(_T("OleInitialize failed!"));
		return FALSE;
	}

    WNDCLASS wndcls;
    memset(&wndcls, 0, sizeof(WNDCLASS));
    wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc = ::DefWindowProc; 
    wndcls.hInstance = AfxGetInstanceHandle();
    wndcls.hIcon = LoadIcon(IDR_MAINFRAME);
    wndcls.hCursor = LoadCursor(IDC_ARROW);
    wndcls.hbrBackground = 0;//(HBRUSH)(COLOR_WINDOW + 1); // no bkg brush, the view and the bars should always fill the whole client area
    wndcls.lpszMenuName = NULL;
    wndcls.lpszClassName = MPC_WND_CLASS_NAME;

	if(!AfxRegisterClass(&wndcls))
    {
		AfxMessageBox(_T("MainFrm class registration failed!"));
		return FALSE;
    }

	if(!AfxSocketInit(NULL))
	{
        AfxMessageBox(_T("AfxSocketInit failed!"));
		return FALSE;
	}

	PreProcessCommandLine();

	if(IsIniValid()) StoreSettingsToIni();
	else StoreSettingsToRegistry();

	m_s.ParseCommandLine(m_cmdln);

	if(m_s.nCLSwitches&(CLSW_HELP|CLSW_UNRECOGNIZEDSWITCH))
	{
		ShowCmdlnSwitches();
		return FALSE;
	}

	if((m_s.nCLSwitches&CLSW_CLOSE) && m_s.slFiles.IsEmpty())
	{
		return FALSE;
	}

	m_s.UpdateData(false);

	if((m_s.nCLSwitches&CLSW_REGEXTVID) || (m_s.nCLSwitches&CLSW_UNREGEXTVID)
	|| (m_s.nCLSwitches&CLSW_REGEXTAUD) || (m_s.nCLSwitches&CLSW_UNREGEXTAUD))
	{
		CMediaFormats& mf = m_s.Formats;

		for(int i = 0; i < mf.GetCount(); i++)
		{
			bool fAudioOnly = mf[i].IsAudioOnly();

			int j = 0;
			CString str = mf[i].GetExtsWithPeriod();
			for(CString ext = str.Tokenize(_T(" "), j); !ext.IsEmpty(); ext = str.Tokenize(_T(" "), j))
			{
				if((m_s.nCLSwitches&CLSW_REGEXTVID) && !fAudioOnly
				|| (m_s.nCLSwitches&CLSW_REGEXTAUD) && fAudioOnly)
					CPPageFormats::RegisterExt(ext, true);
				
				if((m_s.nCLSwitches&CLSW_UNREGEXTVID) && !fAudioOnly
				|| (m_s.nCLSwitches&CLSW_UNREGEXTAUD) && fAudioOnly)
					CPPageFormats::RegisterExt(ext, false);
			}
		}

		return FALSE;
	}

	m_mutexOneInstance.Create(NULL, TRUE, MPC_WND_CLASS_NAME);

	if(GetLastError() == ERROR_ALREADY_EXISTS
	&& (!(m_s.fAllowMultipleInst || (m_s.nCLSwitches&CLSW_NEW) || m_cmdln.IsEmpty())
		|| (m_s.nCLSwitches&CLSW_ADD)))
	{
		if(HWND hWnd = ::FindWindow(MPC_WND_CLASS_NAME, NULL))
		{
			SetForegroundWindow(hWnd);

			if(!(m_s.nCLSwitches&CLSW_MINIMIZED) && IsIconic(hWnd))
				ShowWindow(hWnd, SW_RESTORE);

			SendCommandLine(hWnd);

			return FALSE;
		}
	}

	if(!__super::InitInstance())
	{
		AfxMessageBox(_T("InitInstance failed!"));
		return FALSE;
	}

	AfxEnableControlContainer();

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;
	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW|FWS_ADDTOTITLE, NULL, NULL);
	pFrame->SetDefaultWindowRect();
	pFrame->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	pFrame->DragAcceptFiles();
	pFrame->ShowWindow((m_s.nCLSwitches&CLSW_MINIMIZED)?SW_SHOWMINIMIZED:SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->m_hAccelTable = m_s.hAccel;

	m_s.WinLircClient.SetHWND(m_pMainWnd->m_hWnd);
	if(m_s.fWinLirc) m_s.WinLircClient.Connect(m_s.WinLircAddr);
	m_s.UIceClient.SetHWND(m_pMainWnd->m_hWnd);
	if(m_s.fUIce) m_s.UIceClient.Connect(m_s.UIceAddr);

	SendCommandLine(m_pMainWnd->m_hWnd);

	pFrame->SetFocus();

	return TRUE;
}

int CMPlayerCApp::ExitInstance()
{
	m_s.UpdateData(true);

    OleUninitialize();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp message handlers
// App command to run the dialog

void CMPlayerCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CMPlayerCApp::OnFileExit()
{
	OnAppExit();
}

// CRemoteCtrlClient

CRemoteCtrlClient::CRemoteCtrlClient() 
	: m_pWnd(NULL)
	, m_nStatus(DISCONNECTED)
{
}

void CRemoteCtrlClient::SetHWND(HWND hWnd)
{
	CAutoLock cAutoLock(&m_csLock);

	m_pWnd = CWnd::FromHandle(hWnd);
}

void CRemoteCtrlClient::Connect(CString addr)
{
	CAutoLock cAutoLock(&m_csLock);

	if(m_nStatus == CONNECTING && m_addr == addr)
	{
		TRACE(_T("CRemoteCtrlClient (Connect): already connecting to %s\n"), addr);
		return;
	}

	if(m_nStatus == CONNECTED && m_addr == addr)
	{
		TRACE(_T("CRemoteCtrlClient (Connect): already connected to %s\n"), addr);
		return;
	}

	m_nStatus = CONNECTING;

	TRACE(_T("CRemoteCtrlClient (Connect): connecting to %s\n"), addr);

	Close();

	Create();

	CString ip = addr.Left(addr.Find(':')+1).TrimRight(':');
	int port = _tcstol(addr.Mid(addr.Find(':')+1), NULL, 10);

	__super::Connect(ip, port);

	m_addr = addr;
}

void CRemoteCtrlClient::OnConnect(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	m_nStatus = (nErrorCode == 0 ? CONNECTED : DISCONNECTED);

	TRACE(_T("CRemoteCtrlClient (OnConnect): %d\n"), nErrorCode);
}

void CRemoteCtrlClient::OnClose(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	if(m_hSocket != INVALID_SOCKET && m_nStatus == CONNECTED)
	{
		TRACE(_T("CRemoteCtrlClient (OnClose): connection lost\n"));
	}

	m_nStatus = DISCONNECTED;

	TRACE(_T("CRemoteCtrlClient (OnClose): %d\n"), nErrorCode);
}

void CRemoteCtrlClient::OnReceive(int nErrorCode)
{
	if(nErrorCode != 0 || !m_pWnd) return;

	CStringA str;
	int ret = Receive(str.GetBuffer(256), 255, 0);
	if(ret <= 0) return;
	str.ReleaseBuffer(ret);

	TRACE(_T("CRemoteCtrlClient (OnReceive): %s\n"), CString(str));

	OnCommand(str);

	__super::OnReceive(nErrorCode);
}

void CRemoteCtrlClient::ExecuteCommand(CStringA cmd, int repcnt)
{
	cmd.Trim();
	if(cmd.IsEmpty()) return;
	cmd.Replace(' ', '_');

	AppSettings& s = AfxGetAppSettings();

	POSITION pos = s.wmcmds.GetHeadPosition();
	while(pos)
	{
		wmcmd wc = s.wmcmds.GetNext(pos);
		CStringA name = CString(wc.name);
		name.Replace(' ', '_');
		if((repcnt == 0 && wc.rmrepcnt == 0 || wc.rmrepcnt > 0 && (repcnt%wc.rmrepcnt) == 0)
		&& (!name.CompareNoCase(cmd) || !wc.rmcmd.CompareNoCase(cmd) || wc.cmd == (WORD)strtol(cmd, NULL, 10)))
		{
			CAutoLock cAutoLock(&m_csLock);
			TRACE(_T("CRemoteCtrlClient (calling command): %s\n"), wc.name);
			m_pWnd->SendMessage(WM_COMMAND, wc.cmd);
			break;
		}
	}
}

// CWinLircClient

CWinLircClient::CWinLircClient()
{
}

void CWinLircClient::OnCommand(CStringA str)
{
	TRACE(_T("CWinLircClient (OnCommand): %s\n"), CString(str));

	int i = 0, j = 0, repcnt = 0;
	for(CStringA token = str.Tokenize(" ", i); 
		!token.IsEmpty();
		token = str.Tokenize(" ", i), j++)
	{
		if(j == 1)
			repcnt = strtol(token, NULL, 16);
		else if(j == 2)
			ExecuteCommand(token, repcnt);
	}
}

// CUIceClient

CUIceClient::CUIceClient()
{
}

void CUIceClient::OnCommand(CStringA str)
{
	TRACE(_T("CUIceClient (OnCommand): %s\n"), CString(str));

	CStringA cmd;
	int i = 0, j = 0;
	for(CStringA token = str.Tokenize("|", i); 
		!token.IsEmpty(); 
		token = str.Tokenize("|", i), j++)
	{
		if(j == 0)
			cmd = token;
		else if(j == 1)
			ExecuteCommand(cmd, strtol(token, NULL, 16));
	}
}

// CMPlayerCApp::Settings

CMPlayerCApp::Settings::Settings() 
	: fInitialized(false)
	, MRU(0, _T("Recent File List"), _T("File%d"), 20)
	, MRUDub(0, _T("Recent Dub List"), _T("Dub%d"), 20)
	, hAccel(NULL)
{
#define ADDCMD(cmd) wmcmds.AddTail(wmcmd##cmd)
	ADDCMD((ID_FILE_OPENMEDIA, 'O', FVIRTKEY|FCONTROL|FNOINVERT, _T("Open File")));
	ADDCMD((ID_FILE_OPENDVD, 'D', FVIRTKEY|FCONTROL|FNOINVERT, _T("Open DVD")));
	ADDCMD((ID_FILE_OPENDEVICE, 'V', FVIRTKEY|FCONTROL|FNOINVERT, _T("Open Device")));
	ADDCMD((ID_FILE_SAVEAS, 0, FVIRTKEY|FNOINVERT, _T("Save As")));
	ADDCMD((ID_FILE_LOADSUBTITLE, 'L', FVIRTKEY|FCONTROL|FNOINVERT, _T("Load Subtitle")));
	ADDCMD((ID_FILE_SAVESUBTITLES, 'S', FVIRTKEY|FCONTROL|FNOINVERT, _T("Save Subtitle")));
	ADDCMD((ID_FILE_CLOSEPLAYLIST, 'C', FVIRTKEY|FCONTROL|FNOINVERT, _T("Close")));
	ADDCMD((ID_FILE_PROPERTIES, VK_F10, FVIRTKEY|FSHIFT|FNOINVERT, _T("Properties")));
	ADDCMD((ID_FILE_EXIT, 'X', FVIRTKEY|FALT|FNOINVERT, _T("Exit")));
	ADDCMD((ID_PLAY_PLAYPAUSE, VK_SPACE, FVIRTKEY|FNOINVERT, _T("Play/Pause"), APPCOMMAND_MEDIA_PLAY_PAUSE, wmcmd::LDOWN));
	ADDCMD((ID_PLAY_PLAY, 0, FVIRTKEY|FNOINVERT, _T("Play")));
	ADDCMD((ID_PLAY_PAUSE, 0, FVIRTKEY|FNOINVERT, _T("Pause")));
	ADDCMD((ID_PLAY_STOP, VK_OEM_PERIOD, FVIRTKEY|FNOINVERT, _T("Stop"), APPCOMMAND_MEDIA_STOP));
	ADDCMD((ID_PLAY_FRAMESTEP, VK_RIGHT, FVIRTKEY|FNOINVERT, _T("Framestep")));
	ADDCMD((ID_PLAY_FRAMESTEPCANCEL, VK_LEFT, FVIRTKEY|FNOINVERT, _T("Framestep back")));
	ADDCMD((ID_PLAY_GOTO, 'G', FVIRTKEY|FCONTROL|FNOINVERT, _T("Go To")));
	ADDCMD((ID_PLAY_INCRATE, VK_UP, FVIRTKEY|FCONTROL|FNOINVERT, _T("Increase Rate")));
	ADDCMD((ID_PLAY_DECRATE, VK_DOWN, FVIRTKEY|FCONTROL|FNOINVERT, _T("Decrease Rate")));
	ADDCMD((ID_PLAY_INCAUDDELAY, VK_ADD, FVIRTKEY|FNOINVERT, _T("Audio Delay +10ms")));
	ADDCMD((ID_PLAY_DECAUDDELAY, VK_SUBTRACT, FVIRTKEY|FNOINVERT, _T("Audio Delay -10ms")));
	ADDCMD((ID_PLAY_SEEKFORWARDSMALL, 0, FVIRTKEY|FNOINVERT, _T("Jump Forward (small)")));
	ADDCMD((ID_PLAY_SEEKBACKWARDSMALL, 0, FVIRTKEY|FNOINVERT, _T("Jump Backward (small)")));
	ADDCMD((ID_PLAY_SEEKFORWARDMED, VK_RIGHT, FVIRTKEY|FCONTROL|FNOINVERT, _T("Jump Forward (medium)")));
	ADDCMD((ID_PLAY_SEEKBACKWARDMED, VK_LEFT, FVIRTKEY|FCONTROL|FNOINVERT, _T("Jump Backward (medium)")));
	ADDCMD((ID_PLAY_SEEKFORWARDLARGE, 0, FVIRTKEY|FNOINVERT, _T("Jump Forward (large)")));
	ADDCMD((ID_PLAY_SEEKBACKWARDLARGE, 0, FVIRTKEY|FNOINVERT, _T("Jump Backward (large)")));
	ADDCMD((ID_PLAY_SEEKKEYFORWARD, VK_RIGHT, FVIRTKEY|FSHIFT|FNOINVERT, _T("Jump Forward (keyframe)")));
	ADDCMD((ID_PLAY_SEEKKEYBACKWARD, VK_LEFT, FVIRTKEY|FSHIFT|FNOINVERT, _T("Jump Backward (keyframe)")));
	ADDCMD((ID_NAVIGATE_SKIPFORWARD, VK_NEXT, FVIRTKEY|FNOINVERT, _T("Next"), APPCOMMAND_MEDIA_NEXTTRACK, wmcmd::X2DOWN));
	ADDCMD((ID_NAVIGATE_SKIPBACK, VK_PRIOR, FVIRTKEY|FNOINVERT, _T("Previous"), APPCOMMAND_MEDIA_PREVIOUSTRACK, wmcmd::X1DOWN));
	ADDCMD((ID_NAVIGATE_SKIPFORWARDPLITEM, VK_NEXT, FVIRTKEY|FCONTROL|FNOINVERT, _T("Next Playlist Item")));
	ADDCMD((ID_NAVIGATE_SKIPBACKPLITEM, VK_PRIOR, FVIRTKEY|FCONTROL|FNOINVERT, _T("Previous Playlist Item")));
	ADDCMD((ID_VIEW_CAPTIONMENU, '0', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Caption&Menu")));
	ADDCMD((ID_VIEW_SEEKER, '1', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Seeker")));
	ADDCMD((ID_VIEW_CONTROLS, '2', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Controls")));
	ADDCMD((ID_VIEW_INFORMATION, '3', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Information")));
	ADDCMD((ID_VIEW_STATISTICS, '4', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Statistics")));
	ADDCMD((ID_VIEW_STATUS, '5', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Status")));
	ADDCMD((ID_VIEW_SUBRESYNC, '6', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Subresync Bar")));
	ADDCMD((ID_VIEW_PLAYLIST, '7', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Playlist Bar")));
	ADDCMD((ID_VIEW_CAPTURE, '8', FVIRTKEY|FCONTROL|FNOINVERT, _T("Toggle Capture Bar")));
	ADDCMD((ID_VIEW_PRESETS_MINIMAL, '0', FVIRTKEY|FSHIFT|FCONTROL|FNOINVERT, _T("View Minimal")));
	ADDCMD((ID_VIEW_PRESETS_COMPACT, '1', FVIRTKEY|FSHIFT|FCONTROL|FNOINVERT, _T("View Compact")));
	ADDCMD((ID_VIEW_PRESETS_NORMAL, '2', FVIRTKEY|FSHIFT|FCONTROL|FNOINVERT, _T("View Normal")));
	ADDCMD((ID_VIEW_FULLSCREEN, VK_RETURN, FVIRTKEY|FALT|FNOINVERT, _T("Fullscreen"), 0, wmcmd::LDBLCLK));
	ADDCMD((ID_VIEW_FULLSCREEN_SECONDARY, VK_RETURN, FVIRTKEY|FCONTROL|FNOINVERT, _T("Fullscreen (w/o res.change)")));
	ADDCMD((ID_VIEW_ZOOM_50, '1', FVIRTKEY|FALT|FNOINVERT, _T("Zoom 50%")));
	ADDCMD((ID_VIEW_ZOOM_100, '2', FVIRTKEY|FALT|FNOINVERT, _T("Zoom 100%")));
	ADDCMD((ID_VIEW_ZOOM_200, '3', FVIRTKEY|FALT|FNOINVERT, _T("Zoom 200%")));
	ADDCMD((ID_VIEW_VF_HALF, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Half")));
	ADDCMD((ID_VIEW_VF_NORMAL, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Normal")));
	ADDCMD((ID_VIEW_VF_DOUBLE, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Double")));
	ADDCMD((ID_VIEW_VF_STRETCH, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Stretch")));
	ADDCMD((ID_VIEW_VF_FROMINSIDE, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Inside")));
	ADDCMD((ID_VIEW_VF_FROMOUTSIDE, 0, FVIRTKEY|FNOINVERT, _T("VidFrm Outside")));
	ADDCMD((ID_ONTOP_ALWAYS, 'T', FVIRTKEY|FCONTROL|FNOINVERT, _T("Always On Top")));
	ADDCMD((ID_VIEW_RESET, VK_NUMPAD5, FVIRTKEY|FNOINVERT, _T("PnS Reset")));
	ADDCMD((ID_VIEW_INCSIZE, VK_NUMPAD9, FVIRTKEY|FNOINVERT, _T("PnS Inc Size")));
	ADDCMD((ID_VIEW_INCWIDTH, VK_NUMPAD6, FVIRTKEY|FNOINVERT, _T("PnS Inc Width")));
	ADDCMD((ID_VIEW_INCHEIGHT, VK_NUMPAD8, FVIRTKEY|FNOINVERT, _T("PnS Inc Height")));
	ADDCMD((ID_VIEW_DECSIZE, VK_NUMPAD1, FVIRTKEY|FNOINVERT, _T("PnS Dec Size")));
	ADDCMD((ID_VIEW_DECWIDTH, VK_NUMPAD4, FVIRTKEY|FNOINVERT, _T("PnS Dec Width")));
	ADDCMD((ID_VIEW_DECHEIGHT, VK_NUMPAD2, FVIRTKEY|FNOINVERT, _T("PnS Dec Height")));
	ADDCMD((ID_PANSCAN_CENTER, VK_NUMPAD5, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Center")));
	ADDCMD((ID_PANSCAN_MOVELEFT, VK_NUMPAD4, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Left")));
	ADDCMD((ID_PANSCAN_MOVERIGHT, VK_NUMPAD6, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Right")));
	ADDCMD((ID_PANSCAN_MOVEUP, VK_NUMPAD8, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Up")));
	ADDCMD((ID_PANSCAN_MOVEDOWN, VK_NUMPAD2, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Down")));
	ADDCMD((ID_PANSCAN_MOVEUPLEFT, VK_NUMPAD7, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Up/Left")));
	ADDCMD((ID_PANSCAN_MOVEUPRIGHT, VK_NUMPAD9, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Up/Right")));
	ADDCMD((ID_PANSCAN_MOVEDOWNLEFT, VK_NUMPAD1, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Down/Left")));
	ADDCMD((ID_PANSCAN_MOVEDOWNRIGHT, VK_NUMPAD3, FVIRTKEY|FCONTROL|FNOINVERT, _T("PnS Down/Right")));
	ADDCMD((ID_VOLUME_UP, VK_UP, FVIRTKEY|FNOINVERT, _T("Volume Up"), APPCOMMAND_VOLUME_UP, wmcmd::WUP));
	ADDCMD((ID_VOLUME_DOWN, VK_DOWN, FVIRTKEY|FNOINVERT, _T("Volume Down"), APPCOMMAND_VOLUME_DOWN, wmcmd::WDOWN));
	ADDCMD((ID_VOLUME_MUTE, 'M', FVIRTKEY|FCONTROL|FNOINVERT, _T("Volume Mute"), APPCOMMAND_VOLUME_MUTE));
	ADDCMD((ID_NAVIGATE_TITLEMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Title Menu")));
	ADDCMD((ID_NAVIGATE_ROOTMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Root Menu")));
	ADDCMD((ID_NAVIGATE_SUBPICTUREMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Subtitle Menu")));
	ADDCMD((ID_NAVIGATE_AUDIOMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Audio Menu")));
	ADDCMD((ID_NAVIGATE_ANGLEMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Angle Menu")));
	ADDCMD((ID_NAVIGATE_CHAPTERMENU, 0, FVIRTKEY|FNOINVERT, _T("DVD Chapter Menu")));
	ADDCMD((ID_NAVIGATE_MENU_LEFT, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Left")));
	ADDCMD((ID_NAVIGATE_MENU_RIGHT, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Right")));
	ADDCMD((ID_NAVIGATE_MENU_UP, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Up")));
	ADDCMD((ID_NAVIGATE_MENU_DOWN, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Down")));
	ADDCMD((ID_NAVIGATE_MENU_ACTIVATE, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Activate")));
	ADDCMD((ID_NAVIGATE_MENU_BACK, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Back")));
	ADDCMD((ID_NAVIGATE_MENU_LEAVE, 0, FVIRTKEY|FNOINVERT, _T("DVD Menu Leave")));
	ADDCMD((ID_BOSS, 'B', FVIRTKEY|FNOINVERT, _T("Boss key")));
	ADDCMD((ID_MENU_PLAYER_SHORT, 0, FVIRTKEY|FNOINVERT, _T("Player Menu (short)"), 0, wmcmd::RUP));
	ADDCMD((ID_MENU_PLAYER_LONG, 0, FVIRTKEY|FNOINVERT, _T("Player Menu (long)")));
	ADDCMD((ID_MENU_FILTERS, 0, FVIRTKEY|FNOINVERT, _T("Filters Menu")));
	ADDCMD((ID_VIEW_OPTIONS, 'O', FVIRTKEY|FNOINVERT, _T("Options")));
	ADDCMD((ID_STREAM_AUDIO_NEXT, 'A', FVIRTKEY|FNOINVERT, _T("Next Audio")));
	ADDCMD((ID_STREAM_AUDIO_PREV, 'A', FVIRTKEY|FSHIFT|FNOINVERT, _T("Prev Audio")));
	ADDCMD((ID_STREAM_SUB_NEXT, 'S', FVIRTKEY|FNOINVERT, _T("Next Subtitle")));
	ADDCMD((ID_STREAM_SUB_PREV, 'S', FVIRTKEY|FSHIFT|FNOINVERT, _T("Prev Subtitle")));
	ADDCMD((ID_STREAM_SUB_ONOFF, 'W', FVIRTKEY|FNOINVERT, _T("On/Off Subtitle")));
	ADDCMD((ID_OGM_AUDIO_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Audio (OGM)")));
	ADDCMD((ID_OGM_AUDIO_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Audio (OGM)")));
	ADDCMD((ID_OGM_SUB_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Subtitle (OGM)")));
	ADDCMD((ID_OGM_SUB_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Subtitle (OGM)")));
	ADDCMD((ID_DVD_ANGLE_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Angle (DVD)")));
	ADDCMD((ID_DVD_ANGLE_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Angle (DVD)")));
	ADDCMD((ID_DVD_AUDIO_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Audio (DVD)")));
	ADDCMD((ID_DVD_AUDIO_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Audio (DVD)")));
	ADDCMD((ID_DVD_SUB_NEXT, 0, FVIRTKEY|FNOINVERT, _T("Next Subtitle (DVD)")));
	ADDCMD((ID_DVD_SUB_PREV, 0, FVIRTKEY|FNOINVERT, _T("Prev Subtitle (DVD)")));
	ADDCMD((ID_DVD_SUB_ONOFF, 0, FVIRTKEY|FNOINVERT, _T("On/Off Subtitle (DVD)")));
#undef ADDCMD
}

CMPlayerCApp::Settings::~Settings()
{
	if(hAccel)
		DestroyAcceleratorTable(hAccel);
}

void CMPlayerCApp::Settings::UpdateData(bool fSave)
{
	CWinApp* pApp = AfxGetApp();
	ASSERT(pApp);

	UINT len;
	BYTE* ptr = NULL;

	if(fSave)
	{
		if(!fInitialized) return;

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECAPTIONMENU), fHideCaptionMenu);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CONTROLSTATE), nCS);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DEFAULTVIDEOFRAME), iDefaultVideoSize);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPASPECTRATIO), fKeepAspectRatio);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COMPMONDESKARDIFF), fCompMonDeskARDiff);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VOLUME), nVolume);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_BALANCE), nBalance);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MUTE), fMute);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOPNUM), nLoops);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOP), fLoopForever);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REWIND), fRewind);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ZOOM), iZoomLevel);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MULTIINST), fAllowMultipleInst);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TITLEBARTEXTSTYLE), iTitleBarTextStyle);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ONTOP), iOnTop);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TRAYICON), fTrayIcon);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOZOOM), fRememberZoomLevel);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLS), fShowBarsWhenFullScreen);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLSTIMEOUT), nShowBarsWhenFullScreenTimeOut);
		pApp->WriteProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENRES), (BYTE*)&dmFullscreenRes, sizeof(dmFullscreenRes));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_EXITFULLSCREENATTHEEND), fExitFullScreenAtTheEnd);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWPOS), fRememberWindowPos);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWSIZE), fRememberWindowSize);
		pApp->WriteProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWRECT), (BYTE*)&rcLastWindowPos, sizeof(rcLastWindowPos));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWTYPE), lastWindowType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPHISTORY), fKeepHistory);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DSVIDEORENDERERTYPE), iDSVideoRendererType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_RMVIDEORENDERERTYPE), iRMVideoRendererType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QTVIDEORENDERERTYPE), iQTVideoRendererType);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_APSURACEFUSAGE), iAPSurfaceUsage);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIORENDERERTYPE), CString(AudioRendererDisplayName));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADAUDIO), fAutoloadAudio);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADSUBTITLES), fAutoloadSubtitles);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWORKERTHREADFOROPENING), fEnableWorkerThreadForOpening);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REPORTFAILEDPINS), fReportFailedPins);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DVDPATH), sDVDPath);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEDVDPATH), fUseDVDPath);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MENULANG), idMenuLang);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOLANG), idAudioLang);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SUBTITLESLANG), idSubtitlesLang);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOSPEAKERCONF), fAutoSpeakerConf);
		CString style;
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPLOGFONT), style <<= subdefstyle);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPOVERRIDEPLACEMENT), fOverridePlacement);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPHORPOS), nHorPos);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPVERPOS), nVerPos);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCSIZE), nSPCSize);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCMAXRES), nSPCMaxRes);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOSWITCHER), fEnableAudioSwitcher);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOTIMESHIFT), fAudioTimeShift);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOTIMESHIFT), tAudioTimeShift);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DOWNSAMPLETO441), fDownSampleTo441);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CUSTOMCHANNELMAPPING), fCustomChannelMapping);
		pApp->WriteProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPEAKERTOCHANNELMAPPING), (BYTE*)pSpeakerToChannelMap, sizeof(pSpeakerToChannelMap));

		{
			for(int i = 0; ; i++)
			{
				CString key;
				key.Format(_T("%s\\%04d"), ResStr(IDS_R_FILTERS), i);
				int j = pApp->GetProfileInt(key, _T("Enabled"), -1); 
				pApp->WriteProfileString(key, NULL, NULL);
				if(j < 0) break;
			}
			pApp->WriteProfileString(ResStr(IDS_R_FILTERS), NULL, NULL);

			POSITION pos = filters.GetHeadPosition();
			for(int i = 0; pos; i++)
			{
				Filter* f = filters.GetNext(pos);

				if(f->fTemporary)
					continue;

				CString key;
				key.Format(_T("%s\\%04d"), ResStr(IDS_R_FILTERS), i);

				pApp->WriteProfileInt(key, _T("SourceType"), (int)f->type);
				pApp->WriteProfileInt(key, _T("Enabled"), (int)!f->fDisabled);
				if(f->type == Filter::REGISTERED)
				{
					pApp->WriteProfileString(key, _T("DisplayName"), CString(f->dispname));
					pApp->WriteProfileString(key, _T("Name"), f->name);
				}
				else if(f->type == Filter::EXTERNAL)
				{
					pApp->WriteProfileString(key, _T("Path"), f->path);
					pApp->WriteProfileString(key, _T("Name"), f->name);
					pApp->WriteProfileString(key, _T("CLSID"), CStringFromGUID(f->clsid));
				}
				POSITION pos2 = f->backup.GetHeadPosition();
				for(int i = 0; pos2; i++)
				{
					CString val;
					val.Format(_T("org%04d"), i);
					pApp->WriteProfileString(key, val, CStringFromGUID(f->backup.GetNext(pos2)));
				}
				pos2 = f->guids.GetHeadPosition();
				for(int i = 0; pos2; i++)
				{
					CString val;
					val.Format(_T("mod%04d"), i);
					pApp->WriteProfileString(key, val, CStringFromGUID(f->guids.GetNext(pos2)));
				}
				pApp->WriteProfileInt(key, _T("LoadType"), f->iLoadType);
				pApp->WriteProfileInt(key, _T("Merit"), f->dwMerit);
			}
		}

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_INTREALMEDIA), fIntRealMedia);
		// pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIARENDERLESS), fRealMediaRenderless);
		// pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QUICKTIMERENDERER), iQuickTimeRenderer);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIAFPS), *((DWORD*)&RealMediaQuickTimeFPS));

		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS) + _T("\\") + ResStr(IDS_RS_PNSPRESETS), NULL, NULL);
		for(int i = 0, j = m_pnspresets.GetCount(); i < j; i++)
		{
			CString str;
			str.Format(_T("Preset%d"), i);
			pApp->WriteProfileString(ResStr(IDS_R_SETTINGS) + _T("\\") + ResStr(IDS_RS_PNSPRESETS), str, m_pnspresets[i]);
		}

		pApp->WriteProfileString(ResStr(IDS_R_COMMANDS), NULL, NULL);
		POSITION pos = wmcmds.GetHeadPosition();
		for(int i = 0; pos; )
		{
			wmcmd& wc = wmcmds.GetNext(pos);
			if(wc.IsModified())
			{
				CString str;
				str.Format(_T("CommandMod%d"), i);
				CString str2;
				str2.Format(_T("%d %x %x %s %d %d %d"), 
					wc.cmd, wc.fVirt, wc.key, 
					_T("\"") + CString(wc.rmcmd) +  _T("\""), wc.rmrepcnt,
					wc.mouse, wc.appcmd);
				pApp->WriteProfileString(ResStr(IDS_R_COMMANDS), str, str2);
				i++;
			}
		}

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRC), fWinLirc);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRCADDR), WinLircAddr);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICE), fUIce);
		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICEADDR), UIceAddr);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DISABLEXPTOOLBARS), fDisabeXPToolbars);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEWMASFREADER), fUseWMASFReader);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTS), nJumpDistS);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTM), nJumpDistM);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTL), nJumpDistL);

		Formats.UpdateData(true);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SRCFILTERS), SrcFilters|~(SRC_LAST-1));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TRAFILTERS), TraFilters|~(TRA_LAST-1));

		pApp->WriteProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOFILE), logofn);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOID), logoid);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOEXT), logoext);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGDI), mpegdi);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGBRIGHT), (int)(mpegbright+255));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGCONT), (int)(mpegcont*100));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGHUE), (int)(mpeghue+180));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGSAT), (int)(mpegsat*100));
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGFORCEDSUBS), mpegforcedsubs);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGPLANARYUV), mpegplanaryuv);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPASF), mpasf);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPASC), mpasc);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPADRC), mpadrc);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPANORMALIZE), mpanormalize);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECDROMSSUBMENU), fHideCDROMsSubMenu);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWEBSERVER), fEnableWebServer);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPORT), nWebServerPort);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPRINTDEBUGINFO), fWebServerPrintDebugInfo);
		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERUSECOMPRESSION), fWebServerUseCompression);
	}
	else
	{
		if(fInitialized) return;

		OSVERSIONINFO vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		GetVersionEx(&vi);
		fXpOrBetter = (vi.dwMajorVersion >= 5 && vi.dwMinorVersion >= 1 || vi.dwMajorVersion >= 6);

		iDXVer = 0;
		CRegKey dxver;
		if(ERROR_SUCCESS == dxver.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\DirectX"), KEY_READ))
		{
			CString str;
			ULONG len = 64;
			if(ERROR_SUCCESS == dxver.QueryStringValue(_T("Version"), str.GetBuffer(len), &len))
			{
				str.ReleaseBuffer(len);
				int ver[4];
				_stscanf(str, _T("%d.%d.%d.%d"), ver+0, ver+1, ver+2, ver+3);
				iDXVer = ver[1];
			}
		}

		fHideCaptionMenu = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECAPTIONMENU), 0);
		nCS = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CONTROLSTATE), CS_SEEKBAR|CS_TOOLBAR|CS_INFOBAR|CS_STATUSBAR);
		iDefaultVideoSize = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DEFAULTVIDEOFRAME), DVS_FROMINSIDE);
		fKeepAspectRatio = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPASPECTRATIO), TRUE);
		fCompMonDeskARDiff = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_COMPMONDESKARDIFF), FALSE);
		nVolume = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_VOLUME), 100);
		nBalance = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_BALANCE), 0);
		fMute = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MUTE), 0);
		nLoops = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOPNUM), 1);
		fLoopForever = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOOP), 1);
		fRewind = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REWIND), TRUE);
		iZoomLevel = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ZOOM), 1);
		iDSVideoRendererType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DSVIDEORENDERERTYPE), VIDRNDT_DS_DEFAULT);
		iRMVideoRendererType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_RMVIDEORENDERERTYPE), VIDRNDT_RM_DEFAULT);
		iQTVideoRendererType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QTVIDEORENDERERTYPE), VIDRNDT_QT_DEFAULT);
		iAPSurfaceUsage = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_APSURACEFUSAGE), VIDRNDT_AP_TEXTURE2D);
		AudioRendererDisplayName = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIORENDERERTYPE), _T(""));
		fAutoloadAudio = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADAUDIO), TRUE);
		fAutoloadSubtitles = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOLOADSUBTITLES), TRUE);
		fEnableWorkerThreadForOpening = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWORKERTHREADFOROPENING), TRUE);
		fReportFailedPins = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REPORTFAILEDPINS), TRUE);
		fAllowMultipleInst = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MULTIINST), 0);
		iTitleBarTextStyle = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TITLEBARTEXTSTYLE), 1);
		iOnTop = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ONTOP), 0);
		fTrayIcon = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TRAYICON), 0);
		fRememberZoomLevel = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOZOOM), 1);
		fShowBarsWhenFullScreen = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLS), 1);
		nShowBarsWhenFullScreenTimeOut = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENCTRLSTIMEOUT), 0);
		if(pApp->GetProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_FULLSCREENRES), &ptr, &len))
		{
			memcpy(&dmFullscreenRes, ptr, sizeof(dmFullscreenRes));
			delete [] ptr;
		}
		else
		{
			dmFullscreenRes.fValid = false;
		}
		fExitFullScreenAtTheEnd = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_EXITFULLSCREENATTHEEND), 0);
		fRememberWindowPos = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWPOS), 0);
		fRememberWindowSize = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REMEMBERWINDOWSIZE), 0);
		fKeepHistory = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_KEEPHISTORY), 1);
		if(pApp->GetProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWRECT), &ptr, &len))
		{
			memcpy(&rcLastWindowPos, ptr, sizeof(rcLastWindowPos));
			delete [] ptr;
		}
		else
		{
			fRememberWindowPos = false;
		}
		lastWindowType = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LASTWINDOWTYPE), SIZE_RESTORED);
		sDVDPath = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DVDPATH), _T(""));
		fUseDVDPath = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEDVDPATH), 0);
		idMenuLang = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MENULANG), ::GetUserDefaultLCID());
		idAudioLang = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOLANG), ::GetUserDefaultLCID());
		idSubtitlesLang = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SUBTITLESLANG), ::GetUserDefaultLCID());
		fAutoSpeakerConf = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUTOSPEAKERCONF), 1);
		// TODO: rename subdefstyle -> defStyle, IDS_RS_SPLOGFONT -> IDS_RS_SPSTYLE
		subdefstyle <<= pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPLOGFONT), _T(""));
		fOverridePlacement = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPOVERRIDEPLACEMENT), 0);
		nHorPos = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPHORPOS), 50);
		nVerPos = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPVERPOS), 90);
		nSPCSize = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCSIZE), 3);
		nSPCMaxRes = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPCMAXRES), 2);
		fEnableAudioSwitcher = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOSWITCHER), TRUE);
		fAudioTimeShift = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEAUDIOTIMESHIFT), 0);
		tAudioTimeShift = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_AUDIOTIMESHIFT), 0);
		fDownSampleTo441 = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DOWNSAMPLETO441), 0);
		fCustomChannelMapping = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_CUSTOMCHANNELMAPPING), 0);
		if(pApp->GetProfileBinary(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SPEAKERTOCHANNELMAPPING), &ptr, &len))
		{
			memcpy(pSpeakerToChannelMap, ptr, sizeof(pSpeakerToChannelMap));
			delete [] ptr;
		}
		else
		{
			memset(pSpeakerToChannelMap, 0, sizeof(pSpeakerToChannelMap));
			for(int j = 0; j < 18; j++)
				for(int i = 0; i <= j; i++)
					pSpeakerToChannelMap[j][i] = 1<<i;

			pSpeakerToChannelMap[0][0] = 1<<0;
			pSpeakerToChannelMap[0][1] = 1<<0;

			pSpeakerToChannelMap[3][0] = 1<<0;
			pSpeakerToChannelMap[3][1] = 1<<1;
			pSpeakerToChannelMap[3][2] = 0;
			pSpeakerToChannelMap[3][3] = 0;
			pSpeakerToChannelMap[3][4] = 1<<2;
			pSpeakerToChannelMap[3][5] = 1<<3;

			pSpeakerToChannelMap[4][0] = 1<<0;
			pSpeakerToChannelMap[4][1] = 1<<1;
			pSpeakerToChannelMap[4][2] = 1<<2;
			pSpeakerToChannelMap[4][3] = 0;
			pSpeakerToChannelMap[4][4] = 1<<3;
			pSpeakerToChannelMap[4][5] = 1<<4;
		}

		{
			for(int i = 0; ; i++)
			{
				CString key;
				key.Format(_T("%s\\%04d"), ResStr(IDS_R_FILTERS), i);

				CAutoPtr<Filter> f(new Filter);

				f->fDisabled = !pApp->GetProfileInt(key, _T("Enabled"), 0);

				UINT j = pApp->GetProfileInt(key, _T("SourceType"), -1);
				if(j == 0)
				{
					f->type = Filter::REGISTERED;
					f->dispname = CStringW(pApp->GetProfileString(key, _T("DisplayName"), _T("")));
					f->name = pApp->GetProfileString(key, _T("Name"), _T(""));
				}
				else if(j == 1)
				{
					f->type = Filter::EXTERNAL;
					f->path = pApp->GetProfileString(key, _T("Path"), _T(""));
					f->name = pApp->GetProfileString(key, _T("Name"), _T(""));
					f->clsid = GUIDFromCString(pApp->GetProfileString(key, _T("CLSID"), _T("")));
				}
				else
				{
					pApp->WriteProfileString(key, NULL, 0);
					break;
				}

				f->backup.RemoveAll();
				for(int i = 0; ; i++)
				{
					CString val;
					val.Format(_T("org%04d"), i);
					CString guid = pApp->GetProfileString(key, val, _T(""));
					if(guid.IsEmpty()) break;
					f->backup.AddTail(GUIDFromCString(guid));
				}

				f->guids.RemoveAll();
				for(int i = 0; ; i++)
				{
					CString val;
					val.Format(_T("mod%04d"), i);
					CString guid = pApp->GetProfileString(key, val, _T(""));
					if(guid.IsEmpty()) break;
					f->guids.AddTail(GUIDFromCString(guid));
				}

				f->iLoadType = (int)pApp->GetProfileInt(key, _T("LoadType"), -1);
				if(f->iLoadType < 0) break;

				f->dwMerit = pApp->GetProfileInt(key, _T("Merit"), MERIT_DO_NOT_USE+1);

				filters.AddTail(f);
			}
		}

		fIntRealMedia = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_INTREALMEDIA), 0);
		//fRealMediaRenderless = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIARENDERLESS), 0);
		//iQuickTimeRenderer = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_QUICKTIMERENDERER), 2);
		RealMediaQuickTimeFPS = 25.0;
		*((DWORD*)&RealMediaQuickTimeFPS) = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_REALMEDIAFPS), *((DWORD*)&RealMediaQuickTimeFPS));

		m_pnspresets.RemoveAll();
		for(int i = 0; i < (ID_PANNSCAN_PRESETS_END - ID_PANNSCAN_PRESETS_START); i++)
		{
			CString str;
			str.Format(_T("Preset%d"), i);
			str = pApp->GetProfileString(ResStr(IDS_R_SETTINGS) + _T("\\") + ResStr(IDS_RS_PNSPRESETS), str, _T(""));
			if(str.IsEmpty()) break;
			m_pnspresets.Add(str);
		}
		if(m_pnspresets.IsEmpty())
		{
			double _4p3 = 4.0/3.0;
			double _16p9 = 16.0/9.0;
			double _185p1 = 1.85/1.0;
			double _235p1 = 2.35/1.0;

			CString str;
			str.Format(_T("Scale to 16:9 TV,%.3f,%.3f,%.3f,%.3f"), 0.5, 0.5, _4p3/_4p3, _16p9/_4p3);
			m_pnspresets.Add(str);
			str.Format(_T("Zoom To Widescreen,%.3f,%.3f,%.3f,%.3f"), 0.5, 0.5, _16p9/_4p3, _16p9/_4p3);
			m_pnspresets.Add(str);
			str.Format(_T("Zoom To Ultra-Widescreen,%.3f,%.3f,%.3f,%.3f"), 0.5, 0.5, _235p1/_4p3, _235p1/_4p3);
			m_pnspresets.Add(str);
		}

		for(int i = 0; i < wmcmds.GetCount(); i++)
		{
			CString str;
			str.Format(_T("CommandMod%d"), i);
			str = pApp->GetProfileString(ResStr(IDS_R_COMMANDS), str, _T(""));
			if(str.IsEmpty()) break;
			int cmd, fVirt, key, repcnt, mouse, appcmd;
			TCHAR buff[128];
			int n;
			if(5 > (n = _stscanf(str, _T("%d %x %x %s %d %d %d"), &cmd, &fVirt, &key, buff, &repcnt, &mouse, &appcmd)))
				break;
			if(POSITION pos = wmcmds.Find(cmd))
			{
				wmcmd& wc = wmcmds.GetAt(pos);
                wc.cmd = cmd;
				wc.fVirt = fVirt;
				wc.key = key;
				if(n >= 6) wc.mouse = (UINT)mouse;
				if(n >= 7) wc.appcmd = (UINT)appcmd;
				wc.rmcmd = CStringA(buff).Trim('\"');
				wc.rmrepcnt = repcnt;
			}
		}

		CArray<ACCEL> pAccel;
		pAccel.SetSize(wmcmds.GetCount());
		POSITION pos = wmcmds.GetHeadPosition();
		for(int i = 0; pos; i++) pAccel[i] = wmcmds.GetNext(pos);
		hAccel = CreateAcceleratorTable(pAccel.GetData(), pAccel.GetCount());

		WinLircAddr = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRCADDR), _T("127.0.0.1:8765"));
		fWinLirc = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WINLIRC), 0);
		UIceAddr = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICEADDR), _T("127.0.0.1:1234"));
		fUIce = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_UICE), 0);

		fDisabeXPToolbars = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_DISABLEXPTOOLBARS), 0);
		fUseWMASFReader = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_USEWMASFREADER), TRUE);
		nJumpDistS = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTS), 1000);
		nJumpDistM = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTM), 5000);
		nJumpDistL = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_JUMPDISTL), 20000);

		Formats.UpdateData(false);

		SrcFilters = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_SRCFILTERS), ~0);
		TraFilters = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_TRAFILTERS), ~0^TRA_MPEG1);

		logofn = pApp->GetProfileString(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOFILE), _T(""));
		logoid = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOID), IDB_LOGO2);
		logoext = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_LOGOEXT), 0);

		mpegdi = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGDI), 0);
		mpegbright = (double)pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGBRIGHT), 255) - 255;
		mpegcont = (double)pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGCONT), 100) / 100;
		mpeghue = (double)pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGHUE), 180) - 180;
		mpegsat = (double)pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGSAT), 100) / 100;
		mpegforcedsubs = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGFORCEDSUBS), 1);
		mpegplanaryuv = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPEGPLANARYUV), 1);

		mpasf = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPASF), 0);
		mpasc = (int)pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPASC), 2);
		mpadrc = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPADRC), FALSE);
		mpanormalize = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_MPANORMALIZE), TRUE);

		fHideCDROMsSubMenu = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_HIDECDROMSSUBMENU), 0);		

		fEnableWebServer = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_ENABLEWEBSERVER), FALSE);
		nWebServerPort = pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPORT), 13579);
		fWebServerPrintDebugInfo = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERPRINTDEBUGINFO), FALSE);
		fWebServerUseCompression = !!pApp->GetProfileInt(ResStr(IDS_R_SETTINGS), ResStr(IDS_RS_WEBSERVERUSECOMPRESSION), TRUE);

		pApp->WriteProfileInt(ResStr(IDS_R_SETTINGS), _T("LastUsedPage"), 0);

		fInitialized = true;
	}
}

void CMPlayerCApp::Settings::ParseCommandLine(CStringList& cmdln)
{
	nCLSwitches = 0;
	slFiles.RemoveAll();
	slDubs.RemoveAll();
	slSubs.RemoveAll();
	slFilters.RemoveAll();
	rtStart = 0;

	POSITION pos = cmdln.GetHeadPosition();
	while(pos)
	{
		CString param = cmdln.GetNext(pos);
		if(param.IsEmpty()) continue;

		if((param[0] == '-' || param[0] == '/') && param.GetLength() > 1)
		{
			CString sw = param.Mid(1).MakeLower();
			if(sw == _T("open")) nCLSwitches |= CLSW_OPEN;
			else if(sw == _T("play")) nCLSwitches |= CLSW_PLAY;
			else if(sw == _T("close")) nCLSwitches |= CLSW_CLOSE;
			else if(sw == _T("fullscreen")) nCLSwitches |= CLSW_FULLSCREEN;
			else if(sw == _T("minimized")) nCLSwitches |= CLSW_MINIMIZED;
			else if(sw == _T("new")) nCLSwitches |= CLSW_NEW;
			else if(sw == _T("help") || sw == _T("h") || sw == _T("?")) nCLSwitches |= CLSW_HELP;
			else if(sw == _T("dub") && pos) slDubs.AddTail(cmdln.GetNext(pos));
			else if(sw == _T("sub") && pos) slSubs.AddTail(cmdln.GetNext(pos));
			else if(sw == _T("filter") && pos) slFilters.AddTail(cmdln.GetNext(pos));
			else if(sw == _T("dvd")) nCLSwitches |= CLSW_DVD;
			else if(sw == _T("cd")) nCLSwitches |= CLSW_CD;
			else if(sw == _T("add")) nCLSwitches |= CLSW_ADD;
			else if(sw == _T("shutdown")) nCLSwitches |= CLSW_SHUTDOWN;
			else if(sw == _T("regvid")) nCLSwitches |= CLSW_REGEXTVID;
			else if(sw == _T("regaud")) nCLSwitches |= CLSW_REGEXTAUD;
			else if(sw == _T("unregvid")) nCLSwitches |= CLSW_UNREGEXTVID;
			else if(sw == _T("unregaud")) nCLSwitches |= CLSW_UNREGEXTAUD;
			else if(sw == _T("start") && pos) {rtStart = 10000i64*_tcstol(cmdln.GetNext(pos), NULL, 10); nCLSwitches |= CLSW_STARTVALID;}
			else nCLSwitches |= CLSW_HELP|CLSW_UNRECOGNIZEDSWITCH;
		}
		else
		{
			slFiles.AddTail(param);
		}
	}
}

void CMPlayerCApp::Settings::GetFav(favtype ft, CStringList& sl)
{
	sl.RemoveAll();

	CString root;

	switch(ft)
	{
	case FAV_FILE: root = ResStr(IDS_R_FAVFILES); break;
	case FAV_DVD: root = ResStr(IDS_R_FAVDVDS); break;
	case FAV_DEVICE: root = ResStr(IDS_R_FAVDEVICES); break;
	default: return;
	}

	for(int i = 0; ; i++)
	{
		CString s;
		s.Format(_T("Name%d"), i);
		s = AfxGetApp()->GetProfileString(root, s, NULL);
		if(s.IsEmpty()) break;
		sl.AddTail(s);
	}
}

void CMPlayerCApp::Settings::SetFav(favtype ft, CStringList& sl)
{
	CString root;

	switch(ft)
	{
	case FAV_FILE: root = ResStr(IDS_R_FAVFILES); break;
	case FAV_DVD: root = ResStr(IDS_R_FAVDVDS); break;
	case FAV_DEVICE: root = ResStr(IDS_R_FAVDEVICES); break;
	default: return;
	}

	AfxGetApp()->WriteProfileString(root, NULL, NULL);

	int i = 0;
	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		CString s;
		s.Format(_T("Name%d"), i++);
		AfxGetApp()->WriteProfileString(root, s, sl.GetNext(pos));
	}
}

void CMPlayerCApp::Settings::AddFav(favtype ft, CString s)
{
	CStringList sl;
	GetFav(ft, sl);
	if(sl.Find(s)) return;
	sl.AddTail(s);
	SetFav(ft, sl);
}

// CMPlayerCApp::Settings::CRecentFileAndURLList

CMPlayerCApp::Settings::CRecentFileAndURLList::CRecentFileAndURLList(UINT nStart, LPCTSTR lpszSection,
															LPCTSTR lpszEntryFormat, int nSize,	
															int nMaxDispLen) 
	: CRecentFileList(nStart, lpszSection, lpszEntryFormat, nSize, nMaxDispLen)	
{
}

//#include <afximpl.h>
extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
extern BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

void CMPlayerCApp::Settings::CRecentFileAndURLList::Add(LPCTSTR lpszPathName)
{
	ASSERT(m_arrNames != NULL);
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	if(CString(lpszPathName).MakeLower().Find(_T("@device:")) >= 0)
		return;

	bool fURL = (CString(lpszPathName).Find(_T("://")) >= 0);

	// fully qualify the path name
	TCHAR szTemp[_MAX_PATH];
	if(fURL) _tcscpy(szTemp, lpszPathName);
	else AfxFullPath(szTemp, lpszPathName);

	// update the MRU list, if an existing MRU string matches file name
	int iMRU;
	for (iMRU = 0; iMRU < m_nSize-1; iMRU++)
	{
		if((fURL && !_tcscmp(m_arrNames[iMRU], szTemp))
		|| AfxComparePath(m_arrNames[iMRU], szTemp))
			break;      // iMRU will point to matching entry
	}
	// move MRU strings before this one down
	for (; iMRU > 0; iMRU--)
	{
		ASSERT(iMRU > 0);
		ASSERT(iMRU < m_nSize);
		m_arrNames[iMRU] = m_arrNames[iMRU-1];
	}
	// place this one at the beginning
	m_arrNames[0] = szTemp;
}


void CMPlayerCApp::OnHelpShowcommandlineswitches()
{
	ShowCmdlnSwitches();
}

//

void GetCurDispMode(dispmode& dm)
{
	if(HDC hDC = ::GetDC(0))
	{
		dm.fValid = true;
		dm.size = CSize(GetDeviceCaps(hDC, HORZRES), GetDeviceCaps(hDC, VERTRES));
		dm.bpp = GetDeviceCaps(hDC, BITSPIXEL);
		dm.freq = GetDeviceCaps(hDC, VREFRESH);
		::ReleaseDC(0, hDC);
	}
}

bool GetDispMode(int i, dispmode& dm)
{
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	if(!EnumDisplaySettings(0, i, &devmode))
		return(false);

	dm.fValid = true;
	dm.size = CSize(devmode.dmPelsWidth, devmode.dmPelsHeight);
	dm.bpp = devmode.dmBitsPerPel;
	dm.freq = devmode.dmDisplayFrequency;

	return(true);
}

void SetDispMode(dispmode& dm)
{
	if(!dm.fValid) return;

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = dm.size.cx;
	dmScreenSettings.dmPelsHeight = dm.size.cy;
	dmScreenSettings.dmBitsPerPel = dm.bpp;
	dmScreenSettings.dmDisplayFrequency = dm.freq;
	dmScreenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
	ChangeDisplaySettings(&dmScreenSettings, 0);
}

#include <afxsock.h>
#include <atlsync.h>
#include <atlutil.h> // put this before the first detours macro above to see an ICE with vc71 :)

CStringA GetContentType(CString fn)
{
	if(fn.Find(_T("://")) < 0)
		return "";

	CUrl url;
	url.CrackUrl(fn);

	if(_tcsicmp(url.GetSchemeName(), _T("pnm")) == 0)
		return "audio/x-pn-realaudio";

	if(_tcsicmp(url.GetSchemeName(), _T("mms")) == 0)
		return "video/x-ms-asf";

	if(_tcsicmp(url.GetSchemeName(), _T("http")) != 0)
		return "";

	CSocket s;
	s.Create();
	if(s.Connect(url.GetHostName(), url.GetPortNumber()))
	{
		CStringA hdr = 
			"GET " + CStringA(url.GetUrlPath()) + CStringA(url.GetExtraInfo()) + " HTTP/1.0\r\n"
			"User-Agent: Media Player Classic\r\n"
			"Host: %s\r\n"
			"Accept: */*\r\n"
			"\r\n";
		if(s.Send((LPCSTR)hdr, hdr.GetLength()) < hdr.GetLength()) return "";
		hdr.Empty();
		while(1)
		{
			CStringA str;
			str.ReleaseBuffer(s.Receive(str.GetBuffer(256), 256)); // SOCKET_ERROR == -1, also suitable for ReleaseBuffer
			if(str.IsEmpty()) break;
			hdr += str;
			int hdrend = hdr.Find("\r\n\r\n");
			if(hdrend >= 0) {hdr = hdr.Left(hdrend); break;}
		}
		CList<CStringA> sl;
		Explode(hdr, sl, '\n');
		POSITION pos = sl.GetHeadPosition();
		while(pos)
		{
			CStringA& hdrline = sl.GetNext(pos);
			CList<CStringA> sl2;
			Explode(hdrline, sl2, ':', 2);
			if(sl2.RemoveHead().MakeLower() != "content-type" || sl2.IsEmpty())
				continue;
			return sl2.GetHead();
		}
	}

	return "";
}
