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

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include <afxadv.h>
#include <atlsync.h>
#include "..\..\subtitles\STS.h"
#include "MediaFormats.h"
#include "fakefiltermapper2.h"

#ifdef UNICODE
#define MPC_WND_CLASS_NAME L"MediaPlayerClassicW"
#else
#define MPC_WND_CLASS_NAME "MediaPlayerClassicA"
#endif

enum 
{
	WM_GRAPHNOTIFY = WM_APP+1,
	WM_REARRANGERENDERLESS,
	WM_RESUMEFROMSTATE
};

#define WM_MYMOUSELAST WM_XBUTTONDBLCLK

///////////////

extern void CorrectComboListWidth(CComboBox& box, CFont* pWndFont);
extern HICON LoadIcon(CString fn, bool fSmall);
extern bool LoadType(CString fn, CString& type);


/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp:
// See mplayerc.cpp for the implementation of this class
//

// flags for AppSettings::nCS
enum 
{
	CS_NONE=0, 
	CS_SEEKBAR=1, 
	CS_TOOLBAR=CS_SEEKBAR<<1, 
	CS_INFOBAR=CS_TOOLBAR<<1, 
	CS_STATSBAR=CS_INFOBAR<<1, 
	CS_STATUSBAR=CS_STATSBAR<<1, 
	CS_LAST=CS_STATUSBAR
};

enum
{
	CLSW_NONE=0,
	CLSW_OPEN=1,
	CLSW_PLAY=CLSW_OPEN<<1,
	CLSW_CLOSE=CLSW_PLAY<<1,
	CLSW_FULLSCREEN=CLSW_CLOSE<<1,
	CLSW_NEW=CLSW_FULLSCREEN<<1,
	CLSW_HELP=CLSW_NEW<<1,
	CLSW_DVD=CLSW_HELP<<1,
	CLSW_CD=CLSW_DVD<<1,
	CLSW_ADD=CLSW_CD<<1,
	CLSW_SHUTDOWN=CLSW_ADD<<1,
	CLSW_MINIMIZED=CLSW_SHUTDOWN<<1
};

enum
{
	VIDRNDT_DEFAULT,
	VIDRNDT_OLDRENDERER,
	VIDRNDT_OVERLAYMIXER,
	VIDRNDT_VMR7RENDERLESS,
	VIDRNDT_VMR9RENDERLESS,
	VIDRNDT_VMR7WINDOWED,
	VIDRNDT_VMR9WINDOWED
};

enum
{
	DVS_HALF, 
	DVS_NORMAL, 
	DVS_DOUBLE, 
	DVS_STRETCH, 
	DVS_FROMINSIDE, 
	DVS_FROMOUTSIDE
};

typedef enum 
{
	FAV_FILE,
	FAV_DVD,
	FAV_DEVICE
} favtype;

#pragma pack(push, 1)
typedef struct
{
	bool fValid;
	CSize size; 
	int bpp, freq;
} dispmode;

class wmcmd : public ACCEL
{
	ACCEL backup;
	UINT mouseorg;
public:
	CString name;
	enum {NONE,LDOWN,LUP,LDBLCLK,MDOWN,MUP,MDBLCLK,RDOWN,RUP,RDBLCLK,X1DOWN,X1UP,X1DBLCLK,X2DOWN,X2UP,X2DBLCLK,WUP,WDOWN,LAST};
	UINT mouse;
	CStringA rmcmd;
	int rmrepcnt;
	wmcmd(WORD cmd = 0) {this->cmd = cmd;}
	wmcmd(WORD cmd, WORD key, BYTE fVirt, LPCTSTR name, UINT mouse = NONE, LPCSTR rmcmd = "", int rmrepcnt = 5)
	{
		this->cmd = cmd;
		this->key = key;
		this->fVirt = fVirt;
		this->name = name;
		this->mouse = mouseorg = mouse;
		this->rmcmd = rmcmd;
		this->rmrepcnt = rmrepcnt;
		backup = *this;
	}
	operator == (const wmcmd& wc) const
	{
		return(cmd > 0 && cmd == wc.cmd);
	}
	void Restore() {*(ACCEL*)this = backup; mouse = mouseorg; rmcmd.Empty(); rmrepcnt = 5;}
	bool IsModified() {return(memcmp((const ACCEL*)this, &backup, sizeof(ACCEL)) || mouse != mouseorg || !rmcmd.IsEmpty() || rmrepcnt != 5);}
};
#pragma pack(pop)

#include <afxsock.h>

class CRemoteCtrlClient : public CAsyncSocket
{
protected:
	CCritSec m_csLock;
	CWnd* m_pWnd;
	enum {DISCONNECTED, CONNECTED, CONNECTING} m_nStatus;
	CString m_addr;

	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnReceive(int nErrorCode);

	virtual void OnCommand(CStringA str) = 0;

	void ExecuteCommand(CStringA cmd, int repcnt);

public:
	CRemoteCtrlClient();
	void SetHWND(HWND hWnd);
	void Connect(CString addr);
	int GetStatus() {return(m_nStatus);}
};

class CWinLircClient : public CRemoteCtrlClient
{
protected:
	virtual void OnCommand(CStringA str);

public:
	CWinLircClient();
};

class CUIceClient : public CRemoteCtrlClient
{
protected:
	virtual void OnCommand(CStringA str);

public:
	CUIceClient();
};

extern void GetCurDispMode(dispmode& dm);
extern bool GetDispMode(int i, dispmode& dm);
extern void SetDispMode(dispmode& dm);

class CMPlayerCApp : public CWinApp
{
	ATL::CMutex m_mutexOneInstance;

	CStringList m_cmdln;
	void PreProcessCommandLine();
	void SendCommandLine(HWND hWnd);

public:
	CMPlayerCApp();

	void ShowCmdlnSwitches();

	bool StoreSettingsToIni();
	bool StoreSettingsToRegistry();
	CString GetIniPath();
	bool IsIniValid();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMPlayerCApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	class Settings
	{
		friend class CMPlayerCApp;

		bool fInitialized;

		class CRecentFileAndURLList : public CRecentFileList
		{
		public:
			CRecentFileAndURLList(UINT nStart, LPCTSTR lpszSection,
				LPCTSTR lpszEntryFormat, int nSize,
				int nMaxDispLen = AFX_ABBREV_FILENAME_LEN);

			virtual void Add(LPCTSTR lpszPathName); // we have to override CRecentFileList::Add because the original version can't handle URLs
		};

	public:
		// cmdline params
		int nCLSwitches;
		CString strFile;
		CStringList slDubs, slSubs, slFilters;

		void ParseCommandLine(CStringList& cmdln);

		int nCS;
		bool fHideCaptionMenu;
		int iDefaultVideoSize;
		bool fKeepAspectRatio;

		CRecentFileAndURLList MRU;
		CRecentFileAndURLList MRUDub;

		int iVideoRendererAvailable;
		int iAudioRendererAvailable;

		CAutoPtrList<Filter> filters;

		int nVolume;
		int nBalance;
		bool fMute;
		int nLoops;
		bool fLoopForever;
		bool fRewind;
		int iZoomLevel;
		int iVideoRendererType; 
		CStringW AudioRendererDisplayName;
		bool fAutoloadAudio;
		bool fAutoloadSubtitles;
		bool fEnableWorkerThreadForOpening;
		bool fReportFailedPins;

		bool fAllowMultipleInst;
		int iTitleBarTextStyle;
		bool fAlwaysOnTop;
		bool fTrayIcon;
		bool fRememberZoomLevel;
		bool fShowBarsWhenFullScreen;
		int nShowBarsWhenFullScreenTimeOut;
		dispmode dmFullscreenRes;
		bool fExitFullScreenAtTheEnd;
		bool fRememberWindowPos;
		bool fRememberWindowSize;
		CRect rcLastWindowPos;

		CString sDVDPath;
		bool fUseDVDPath;
		LCID idMenuLang, idAudioLang, idSubtitlesLang;
		bool fAutoSpeakerConf;

		STSStyle subdefstyle;
		bool fOverridePlacement;
		int nHorPos, nVerPos;
		int nSPCSize;
		int nSPCMaxRes;

		bool fDisabeXPToolbars;
		bool fUseWMASFReader;
		int nJumpDistS;
		int nJumpDistM;
		int nJumpDistL;

		bool fEnableAudioSwitcher;
		bool fDownSampleTo441;
		bool fAudioTimeShift;
		int tAudioTimeShift;
		bool fCustomChannelMapping;
		DWORD pSpeakerToChannelMap[18][18];

		bool fIntRealMedia;
		bool fRealMediaRenderless;
		int iQuickTimeRenderer;
		float RealMediaQuickTimeFPS;

		CStringArray m_pnspresets;

		CList<wmcmd> wmcmds;
		HACCEL hAccel;

		bool fWinLirc;
		CString WinLircAddr;
		CWinLircClient WinLircClient;
		bool fUIce;
		CString UIceAddr;
		CUIceClient UIceClient;

		CMediaFormats Formats;

	public:
		Settings();
		virtual ~Settings();
		void UpdateData(bool fSave);

		void GetFav(favtype ft, CStringList& sl);
		void SetFav(favtype ft, CStringList& sl);
		void AddFav(favtype ft, CString s);
	} m_s;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();
	afx_msg void OnFileExit();
	afx_msg void OnHelpShowcommandlineswitches();
};

#define AfxGetMyApp() ((CMPlayerCApp*)AfxGetApp())
#define AfxGetAppSettings() ((CMPlayerCApp*)AfxGetApp())->m_s
#define AppSettings CMPlayerCApp::Settings
