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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mplayerc.h"

#include "MainFrm.h"

#include <math.h>

#include <afxpriv.h>

#include <atlconv.h>
#include <atlrx.h>
#include <atlsync.h>

#include "OpenDlg.h"
#include "SaveDlg.h"
#include "GoToDlg.h"
#include "PnSPresetsDlg.h"
#include "MediaTypesDlg.h"
#include "SaveTextFileDialog.h"
#include "FavoriteAddDlg.h"
#include "FavoriteOrganizeDlg.h"

#include <mtype.h>
#include <Mpconfig.h>
#include <ks.h>
#include <ksmedia.h>
#include <dvdevcod.h>
#include <dsound.h>

#include <initguid.h>
#include <uuids.h>
#include "..\..\..\include\moreuuids.h"
#include "..\..\..\include\Ogg\OggDS.h"
#include <Qnetwork.h>
#include <qedit.h>

#include "..\..\DSUtil\DSUtil.h"

#include "GraphBuilder.h"

#include "textpassthrufilter.h"
#include "..\..\filters\filters.h"

#include "DX7AllocatorPresenter.h"
#include "DX9AllocatorPresenter.h"

#include "..\..\..\include\matroska\matroska.h"

#define DEFCLIENTW 292
#define DEFCLIENTH 200

static UINT s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
static UINT WM_NOTIFYICON = RegisterWindowMessage(TEXT("MYWM_NOTIFYICON"));

#include "..\..\filters\transform\vsfilter\IDirectVobSub.h"

class CSubClock : public CUnknown, public ISubClock
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return 
			QI(ISubClock)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	REFERENCE_TIME m_rt;

public:
	CSubClock() : CUnknown(NAME("CSubClock"), NULL) {m_rt = 0;}

	DECLARE_IUNKNOWN;

	// ISubClock
	STDMETHODIMP SetTime(REFERENCE_TIME rt) {m_rt = rt; return S_OK;}
	STDMETHODIMP_(REFERENCE_TIME) GetTime() {return(m_rt);}
};

//

#define SaveMediaState \
	OAFilterState __fs = GetMediaState(); \
 \
	REFERENCE_TIME __rt = 0; \
	if(m_iMediaLoadState == MLS_LOADED) __rt = GetPos(); \
 \
	if(__fs != State_Stopped) \
		SendMessage(WM_COMMAND, ID_PLAY_STOP); \


#define RestoreMediaState \
	if(m_iMediaLoadState == MLS_LOADED) \
	{ \
		SeekTo(__rt); \
 \
		if(__fs == State_Stopped) \
			SendMessage(WM_COMMAND, ID_PLAY_STOP); \
		else if(__fs == State_Paused) \
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE); \
		else if(__fs == State_Running) \
			SendMessage(WM_COMMAND, ID_PLAY_PLAY); \
	} \


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()

	ON_REGISTERED_MESSAGE(s_uTaskbarRestart, OnTaskBarRestart)
	ON_REGISTERED_MESSAGE(WM_NOTIFYICON, OnNotifyIcon)

	ON_WM_SETFOCUS()
	ON_WM_GETMINMAXINFO()
	ON_WM_MOVE()
	ON_WM_SIZE()
	ON_MESSAGE_VOID(WM_DISPLAYCHANGE, OnDisplayChange)

	ON_WM_SYSCOMMAND()
	ON_WM_ACTIVATEAPP()
	ON_MESSAGE(WM_APPCOMMAND, OnAppCommand)

	ON_WM_TIMER()

	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_MESSAGE(WM_REARRANGERENDERLESS, OnRepaintRenderLess)
	ON_MESSAGE(WM_RESUMEFROMSTATE, OnResumeFromState)

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_MESSAGE(WM_XBUTTONDOWN, OnXButtonDown)
	ON_MESSAGE(WM_XBUTTONUP, OnXButtonUp)
	ON_MESSAGE(WM_XBUTTONDBLCLK, OnXButtonDblClk)
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()

	ON_WM_NCHITTEST()

	ON_WM_HSCROLL()

	ON_WM_INITMENU()
	ON_WM_INITMENUPOPUP()
	ON_WM_CONTEXTMENU()

	ON_COMMAND(ID_MENU_PLAYER_SHORT, OnMenuPlayerShort)
	ON_COMMAND(ID_MENU_PLAYER_LONG, OnMenuPlayerLong)
	ON_COMMAND(ID_MENU_FILTERS, OnMenuFilters)

	ON_UPDATE_COMMAND_UI(ID_PLAYERSTATUS, OnUpdatePlayerStatus)

	ON_COMMAND(ID_FILE_POST_OPENMEDIA, OnFilePostOpenmedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_POST_OPENMEDIA, OnUpdateFilePostOpenmedia)
	ON_COMMAND(ID_FILE_POST_CLOSEMEDIA, OnFilePostClosemedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_POST_CLOSEMEDIA, OnUpdateFilePostClosemedia)

	ON_COMMAND(ID_BOSS, OnBossKey)

	ON_COMMAND_RANGE(ID_STREAM_AUDIO_NEXT, ID_STREAM_AUDIO_PREV, OnStreamAudio)
	ON_COMMAND_RANGE(ID_STREAM_SUB_NEXT, ID_STREAM_SUB_PREV, OnStreamSub)
	ON_COMMAND_RANGE(ID_OGM_AUDIO_NEXT, ID_OGM_AUDIO_PREV, OnOgmAudio)
	ON_COMMAND_RANGE(ID_OGM_SUB_NEXT, ID_OGM_SUB_PREV, OnOgmSub)
	ON_COMMAND_RANGE(ID_DVD_ANGLE_NEXT, ID_DVD_ANGLE_PREV, OnDvdAngle)
	ON_COMMAND_RANGE(ID_DVD_AUDIO_NEXT, ID_DVD_AUDIO_PREV, OnDvdAudio)
	ON_COMMAND_RANGE(ID_DVD_SUB_NEXT, ID_DVD_SUB_PREV, OnDvdSub)

	ON_COMMAND(ID_FILE_OPENMEDIA, OnFileOpenmedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENMEDIA, OnUpdateFileOpen)
	ON_WM_COPYDATA()
	ON_COMMAND(ID_FILE_OPENDVD, OnFileOpendvd)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDVD, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENDEVICE, OnFileOpendevice)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDEVICE, OnUpdateFileOpen)
	ON_COMMAND_RANGE(ID_FILE_OPEN_CD_START, ID_FILE_OPEN_CD_END, OnFileOpenCD)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_OPEN_CD_START, ID_FILE_OPEN_CD_END, OnUpdateFileOpen)
	ON_WM_DROPFILES()
	ON_COMMAND(ID_FILE_SAVEAS, OnFileSaveas)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEAS, OnUpdateFileSaveas)
	ON_COMMAND(ID_FILE_LOADSUBTITLE, OnFileLoadsubtitles)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOADSUBTITLE, OnUpdateFileLoadsubtitles)
	ON_COMMAND(ID_FILE_SAVESUBTITLES, OnFileSavesubtitles)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVESUBTITLES, OnUpdateFileSavesubtitles)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_CLOSEPLAYLIST, OnFileClosePlaylist)
	ON_COMMAND(ID_FILE_CLOSEMEDIA, OnFileCloseMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEMEDIA, OnUpdateFileClose)

	ON_COMMAND(ID_VIEW_CAPTIONMENU, OnViewCaptionmenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTIONMENU, OnUpdateViewCaptionmenu)
	ON_COMMAND_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnViewControlBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnUpdateViewControlBar)
	ON_COMMAND(ID_VIEW_SUBRESYNC, OnViewSubresync)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SUBRESYNC, OnUpdateViewSubresync)
	ON_COMMAND(ID_VIEW_PLAYLIST, OnViewPlaylist)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PLAYLIST, OnUpdateViewPlaylist)
	ON_COMMAND(ID_VIEW_CAPTURE, OnViewCapture)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTURE, OnUpdateViewCapture)
	ON_COMMAND(ID_VIEW_PRESETS_MINIMAL, OnViewMinimal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_MINIMAL, OnUpdateViewMinimal)
	ON_COMMAND(ID_VIEW_PRESETS_COMPACT, OnViewCompact)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_COMPACT, OnUpdateViewCompact)
	ON_COMMAND(ID_VIEW_PRESETS_NORMAL, OnViewNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_NORMAL, OnUpdateViewNormal)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_COMMAND(ID_VIEW_FULLSCREEN_SECONDARY, OnViewFullscreenSecondary)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FULLSCREEN, OnUpdateViewFullscreen)
	ON_COMMAND_RANGE(ID_VIEW_ZOOM_50, ID_VIEW_ZOOM_200, OnViewZoom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ZOOM_50, ID_VIEW_ZOOM_200, OnUpdateViewZoom)
	ON_COMMAND_RANGE(ID_VIEW_VF_HALF, ID_VIEW_VF_FROMOUTSIDE, OnViewDefaultVideoFrame)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_VF_HALF, ID_VIEW_VF_FROMOUTSIDE, OnUpdateViewDefaultVideoFrame)
	ON_COMMAND(ID_VIEW_VF_KEEPASPECTRATIO, OnViewKeepaspectratio)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VF_KEEPASPECTRATIO, OnUpdateViewKeepaspectratio)
	ON_COMMAND(ID_VIEW_VF_COMPMONDESKARDIFF, OnViewCompMonDeskARDiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VF_COMPMONDESKARDIFF, OnUpdateViewCompMonDeskARDiff)
	ON_COMMAND_RANGE(ID_VIEW_INCSIZE, ID_VIEW_RESET, OnViewPanNScan)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_INCSIZE, ID_VIEW_RESET, OnUpdateViewPanNScan)
	ON_COMMAND_RANGE(ID_PANSCAN_MOVELEFT, ID_PANSCAN_CENTER, OnViewPanNScan)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PANSCAN_MOVELEFT, ID_PANSCAN_CENTER, OnUpdateViewPanNScan)
	ON_COMMAND_RANGE(ID_PANNSCAN_PRESETS_START, ID_PANNSCAN_PRESETS_END, OnViewPanNScanPresets)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PANNSCAN_PRESETS_START, ID_PANNSCAN_PRESETS_END, OnUpdateViewPanNScanPresets)
	ON_COMMAND(ID_VIEW_ALWAYSONTOP, OnViewAlwaysontop)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ALWAYSONTOP, OnUpdateViewAlwaysontop)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)

	ON_COMMAND(ID_PLAY_PLAY, OnPlayPlay)
	ON_COMMAND(ID_PLAY_PAUSE, OnPlayPause)
	ON_COMMAND(ID_PLAY_PLAYPAUSE, OnPlayPlaypause)
	ON_COMMAND(ID_PLAY_STOP, OnPlayStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PLAY, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PAUSE, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PLAYPAUSE, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_STOP, OnUpdatePlayPauseStop)
	ON_COMMAND_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEPCANCEL, OnPlayFramestep)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEPCANCEL, OnUpdatePlayFramestep)
	ON_COMMAND_RANGE(ID_PLAY_SEEKBACKWARDSMALL, ID_PLAY_SEEKFORWARDLARGE, OnPlaySeek)
	ON_COMMAND_RANGE(ID_PLAY_SEEKKEYBACKWARD, ID_PLAY_SEEKKEYFORWARD, OnPlaySeekKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_SEEKBACKWARDSMALL, ID_PLAY_SEEKFORWARDLARGE, OnUpdatePlaySeek)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_SEEKKEYBACKWARD, ID_PLAY_SEEKKEYFORWARD, OnUpdatePlaySeek)
	ON_COMMAND(ID_PLAY_GOTO, OnPlayGoto)
	ON_UPDATE_COMMAND_UI(ID_PLAY_GOTO, OnUpdateGoto)
	ON_COMMAND_RANGE(ID_PLAY_DECRATE, ID_PLAY_INCRATE, OnPlayChangeRate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_DECRATE, ID_PLAY_INCRATE, OnUpdatePlayChangeRate)
	ON_COMMAND_RANGE(ID_PLAY_INCAUDDELAY, ID_PLAY_DECAUDDELAY, OnPlayChangeAudDelay)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_INCAUDDELAY, ID_PLAY_DECAUDDELAY, OnUpdatePlayChangeAudDelay)
	ON_COMMAND_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnPlayFilters)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnUpdatePlayFilters)
	ON_COMMAND_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnPlayAudio)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnUpdatePlayAudio)
	ON_COMMAND_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnPlaySubtitles)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnUpdatePlaySubtitles)
	ON_COMMAND_RANGE(ID_FILTERSTREAMS_SUBITEM_START, ID_FILTERSTREAMS_SUBITEM_END, OnPlayLanguage)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILTERSTREAMS_SUBITEM_START, ID_FILTERSTREAMS_SUBITEM_END, OnUpdatePlayLanguage)
	ON_COMMAND_RANGE(ID_VOLUME_UP, ID_VOLUME_MUTE, OnPlayVolume)

	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnNavigateSkip)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnUpdateNavigateSkip)
	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACKPLITEM, ID_NAVIGATE_SKIPFORWARDPLITEM, OnNavigateSkipPlaylistItem)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACKPLITEM, ID_NAVIGATE_SKIPFORWARDPLITEM, OnUpdateNavigateSkipPlaylistItem)
	ON_COMMAND_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnNavigateMenu)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnUpdateNavigateMenu)
	ON_COMMAND_RANGE(ID_NAVIGATE_AUDIO_SUBITEM_START, ID_NAVIGATE_AUDIO_SUBITEM_END, OnNavigateAudio)
	ON_COMMAND_RANGE(ID_NAVIGATE_SUBP_SUBITEM_START, ID_NAVIGATE_SUBP_SUBITEM_END, OnNavigateSubpic)
	ON_COMMAND_RANGE(ID_NAVIGATE_ANGLE_SUBITEM_START, ID_NAVIGATE_ANGLE_SUBITEM_END, OnNavigateAngle)
	ON_COMMAND_RANGE(ID_NAVIGATE_CHAP_SUBITEM_START, ID_NAVIGATE_CHAP_SUBITEM_END, OnNavigateChapters)
	ON_COMMAND_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnNavigateMenuItem)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnUpdateNavigateMenuItem)

	ON_COMMAND(ID_FAVORITES_ADD, OnFavoritesAdd)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ADD, OnUpdateFavoritesAdd)
	ON_COMMAND(ID_FAVORITES_ORGANIZE, OnFavoritesOrganize)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ORGANIZE, OnUpdateFavoritesOrganize)
	ON_COMMAND_RANGE(ID_FAVORITES_FILE_START, ID_FAVORITES_FILE_END, OnFavoritesFile)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_FILE_START, ID_FAVORITES_FILE_END, OnUpdateFavoritesFile)
	ON_COMMAND_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnFavoritesDVD)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnUpdateFavoritesDVD)
	ON_COMMAND_RANGE(ID_FAVORITES_DEVICE_START, ID_FAVORITES_DEVICE_END, OnFavoritesDevice)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_DEVICE_START, ID_FAVORITES_DEVICE_END, OnUpdateFavoritesDevice)

	ON_COMMAND(ID_HELP_HOMEPAGE, OnHelpHomepage)
	ON_COMMAND(ID_HELP_DONATE, OnHelpDonate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

#pragma warning(disable : 4355)

CMainFrame::CMainFrame() : 
	m_dwRegister(0),
	m_iMediaLoadState(MLS_CLOSED),
	m_iPlaybackMode(PM_NONE),
	m_iSpeedLevel(0),
	m_rtDurationOverride(-1),
	m_fFullScreen(false),
	m_fHideCursor(false),
	m_lastMouseMove(-1, -1),
	m_pLastBar(NULL),
	m_nLoops(0),
	m_iSubtitleSel(-1),
	m_ZoomX(1), m_ZoomY(1), m_PosX(0.5), m_PosY(0.5),
	m_fCustomGraph(false),
	m_fRealMediaGraph(false), m_fShockwaveGraph(false), m_fQuicktimeGraph(false),
	m_fFrameSteppingActive(false),
	m_fEndOfStream(false),
	m_fCapturing(false),
	m_fLiveWM(false),
	m_fOpeningAborted(false),
	m_fBuffering(false),
	m_fileDropTarget(this)
{
}

CMainFrame::~CMainFrame()
{
	m_owner.DestroyWindow();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_popup.LoadMenu(IDR_POPUP);
	m_popupmain.LoadMenu(IDR_POPUPMAIN);

	// create a view to occupy the client area of the frame
	if(!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	// static bars

	if(!m_wndStatusBar.Create(this)
	|| !m_wndStatsBar.Create(this)
	|| !m_wndInfoBar.Create(this)
	|| !m_wndToolBar.Create(this)
	|| !m_wndSeekBar.Create(this))
	{
		TRACE0("Failed to create all control bars\n");
		return -1;      // fail to create
	}

	m_bars.AddTail(&m_wndSeekBar);
	m_bars.AddTail(&m_wndToolBar);
	m_bars.AddTail(&m_wndInfoBar);
	m_bars.AddTail(&m_wndStatsBar);
	m_bars.AddTail(&m_wndStatusBar);

	m_wndSeekBar.Enable(false);

	// dockable bars

	EnableDocking(CBRS_ALIGN_ANY);

	m_wndSubresyncBar.Create(this, &m_csSubLock);
	m_wndSubresyncBar.SetBarStyle(m_wndSubresyncBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSubresyncBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndSubresyncBar.SetHeight(200);
	LoadControlBar(&m_wndSubresyncBar, _T("ToolBars\\Subresync"), AFX_IDW_DOCKBAR_TOP);

	m_wndPlaylistBar.Create(this);
	m_wndPlaylistBar.SetBarStyle(m_wndPlaylistBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndPlaylistBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndPlaylistBar.SetHeight(100);
	LoadControlBar(&m_wndPlaylistBar, _T("ToolBars\\Playlist"), AFX_IDW_DOCKBAR_BOTTOM);

	m_wndCaptureBar.Create(this);
	m_wndCaptureBar.SetBarStyle(m_wndCaptureBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndCaptureBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	LoadControlBar(&m_wndCaptureBar, _T("ToolBars\\Capture"), AFX_IDW_DOCKBAR_LEFT);

	m_dockingbars.AddTail(&m_wndSubresyncBar);
	m_dockingbars.AddTail(&m_wndPlaylistBar);
	m_dockingbars.AddTail(&m_wndCaptureBar);

	m_fileDropTarget.Register(this);

	AppSettings& s = AfxGetAppSettings();

	ShowControls(s.nCS);

	SetAlwaysOnTop(s.fAlwaysOnTop);

	ShowTrayIcon(s.fTrayIcon);

	SetFocus();

	m_pGraphThread = (CGraphThread*)AfxBeginThread(RUNTIME_CLASS(CGraphThread));

	if(m_pGraphThread)
		m_pGraphThread->SetMainFrame(this);

	if(s.fEnableWebServer)
		StartWebServer(s.nWebServerPort);

	return 0;
}

void CMainFrame::OnDestroy()
{
	ShowTrayIcon(false);

	m_fileDropTarget.Revoke();

	if(m_pGraphThread)
	{
		CAMEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_EXIT, 0, (LPARAM)&e);
		if(!e.Wait(5000))
		{
			TRACE(_T("ERROR: Must call TerminateThread() on CMainFrame::m_pGraphThread->m_hThread\n")); 
			TerminateThread(m_pGraphThread->m_hThread, -1);
		}
	}

	CFrameWnd::OnDestroy();
}

void CMainFrame::OnClose()
{
	SaveControlBar(&m_wndSubresyncBar, _T("ToolBars\\Subresync"));
	SaveControlBar(&m_wndPlaylistBar, _T("ToolBars\\Playlist"));
	SaveControlBar(&m_wndCaptureBar, _T("ToolBars\\Capture"));

	ShowWindow(SW_HIDE);

	CloseMedia();

	__super::OnClose();
}

DROPEFFECT CMainFrame::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}
DROPEFFECT CMainFrame::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	UINT CF_URL = RegisterClipboardFormat(_T("UniformResourceLocator"));
	return pDataObject->IsDataAvailable(CF_URL) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}
BOOL CMainFrame::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	UINT CF_URL = RegisterClipboardFormat(_T("UniformResourceLocator"));

	BOOL bResult = FALSE;

	if(pDataObject->IsDataAvailable(CF_URL)) 
	{
		FORMATETC fmt = {CF_URL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		if(HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_URL, &fmt))
		{
			LPCSTR pText = (LPCSTR)GlobalLock(hGlobal);
			if(AfxIsValidString(pText))
			{
				CStringA url(pText);

				SetForegroundWindow();

				CStringList sl;
				sl.AddTail(CString(url));

				if(m_wndPlaylistBar.IsWindowVisible())
				{
					m_wndPlaylistBar.Append(sl, true);
				}
				else
				{
					m_wndPlaylistBar.Open(sl, true);
					OpenCurPlaylistItem();
				}

				GlobalUnlock(hGlobal);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}
DROPEFFECT CMainFrame::OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point)
{
	return (DROPEFFECT)-1;
}
void CMainFrame::OnDragLeave()
{
}
DROPEFFECT CMainFrame::OnDragScroll(DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}


void CMainFrame::LoadControlBar(CControlBar* pBar, CString section, UINT defDockBarID)
{
	if(!pBar || section.IsEmpty()) return;

	CWinApp* pApp = AfxGetApp();

	UINT nID = pApp->GetProfileInt(section, _T("DockState"), defDockBarID);

	if(nID != AFX_IDW_DOCKBAR_FLOAT)
	{
		DockControlBar(pBar, nID);
	}
	else
	{
		CRect r;
		GetWindowRect(r);
		CPoint p;
		p.x = r.left + pApp->GetProfileInt(section, _T("DockPosX"), r.Width());
		p.y = r.top + pApp->GetProfileInt(section, _T("DockPosY"), 0);
		FloatControlBar(pBar, p);
	}

	pBar->ShowWindow(
		pApp->GetProfileInt(section, _T("Visible"), FALSE) && pBar != &m_wndSubresyncBar && pBar != &m_wndCaptureBar
		? SW_SHOW
		: SW_HIDE);

	if(pBar->IsKindOf(RUNTIME_CLASS(CSizingControlBar)))
	{
		((CSizingControlBar*)(pBar))->LoadState(section + _T("\\State"));
	}
}

void CMainFrame::SaveControlBar(CControlBar* pBar, CString section)
{
	if(!pBar || section.IsEmpty()) return;

	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt(section, _T("Visible"), pBar->IsWindowVisible());

	if(pBar->IsKindOf(RUNTIME_CLASS(CSizingControlBar)))
		((CSizingControlBar*)(pBar))->SaveState(section + _T("\\State"));

	UINT nID = pBar->GetParent()->GetDlgCtrlID();

	if(nID == AFX_IDW_DOCKBAR_FLOAT)
	{
return;
		// FIXME
		CRect r1, r2;
		GetWindowRect(r1);
		pBar->GetWindowRect(r2);
		pApp->WriteProfileInt(section, _T("DockPosX"), r2.left - r1.left);
		pApp->WriteProfileInt(section, _T("DockPosY"), r2.top - r1.top);
	}

	pApp->WriteProfileInt(section, _T("DockState"), nID);

}

LRESULT CMainFrame::OnTaskBarRestart(WPARAM, LPARAM)
{
	ShowTrayIcon(AfxGetAppSettings().fTrayIcon);

	return 0;
}

LRESULT CMainFrame::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
    if((UINT)wParam != IDR_MAINFRAME)
		return -1;

	switch((UINT)lParam)
	{
		case WM_LBUTTONDOWN:
			ShowWindow(SW_SHOW);
			MoveVideoWindow();
			SetForegroundWindow();
			break;

		case WM_LBUTTONDBLCLK:
			PostMessage(WM_COMMAND, ID_FILE_OPENMEDIA);
			break;

		case WM_RBUTTONDOWN:
		{
			POINT p;
			GetCursorPos(&p);
			SetForegroundWindow();
			m_popupmain.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NOANIMATION, p.x, p.y, this);
			PostMessage(WM_NULL);
			break; 
		}

		case WM_MOUSEMOVE:
		{
			CString str;
			GetWindowText(str);
			SetTrayTip(str);
			break;
		}

		default: 
			break; 
	}

	return 0;
}
	
void CMainFrame::ShowTrayIcon(bool fShow)
{
    BOOL bWasVisible = ShowWindow(SW_HIDE);

	if(fShow)
	{
		if(GetExStyle()&WS_EX_APPWINDOW)
		{
			NOTIFYICONDATA tnid; 
			tnid.cbSize = sizeof(NOTIFYICONDATA); 
			tnid.hWnd = m_hWnd; 
			tnid.uID = IDR_MAINFRAME; 
//			tnid.hIcon = (HICON)LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
			tnid.hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			tnid.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP; 
			tnid.uCallbackMessage = WM_NOTIFYICON; 
			lstrcpyn(tnid.szTip, TEXT("Media Player Classic"), sizeof(tnid.szTip)); 
			Shell_NotifyIcon(NIM_ADD, &tnid);
			
			ModifyStyleEx(WS_EX_APPWINDOW, 0);
		}
	}
	else
	{
		if(!(GetExStyle()&WS_EX_APPWINDOW))
		{
			NOTIFYICONDATA tnid; 
			tnid.cbSize = sizeof(NOTIFYICONDATA); 
			tnid.hWnd = m_hWnd;
			tnid.uID = IDR_MAINFRAME; 
			Shell_NotifyIcon(NIM_DELETE, &tnid); 

			ModifyStyleEx(0, WS_EX_APPWINDOW); 
		}
	}

	if(bWasVisible)
		ShowWindow(SW_SHOW);
}

void CMainFrame::SetTrayTip(CString str)
{
	NOTIFYICONDATA tnid; 
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = m_hWnd; 
	tnid.uID = IDR_MAINFRAME; 
	tnid.uFlags = NIF_TIP; 
	lstrcpyn(tnid.szTip, str, sizeof(tnid.szTip)); 
	Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.dwExStyle |= WS_EX_APPWINDOW;
	cs.lpszClass = MPC_WND_CLASS_NAME; //AfxRegisterWndClass(0);

	if(!IsWindow(m_owner.m_hWnd))
	{
		m_owner.CreateEx(0, 
			AfxRegisterWndClass(0, 0, 0, LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME))),
			_T(""), WS_OVERLAPPEDWINDOW, CRect(0,0,0,0), NULL, 0);
	}

	cs.hwndParent = m_owner.GetSafeHwnd();

	return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
/*		if(m_fShockwaveGraph
		&& (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT
			|| pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN))
			return FALSE;
*/
		if(pMsg->wParam == VK_ESCAPE && m_iMediaLoadState == MLS_LOADED && m_fFullScreen)
		{
			OnViewFullscreen();
			PostMessage(WM_COMMAND, ID_PLAY_PAUSE);
			return TRUE;
		}
		else if(pMsg->wParam == VK_ESCAPE && (IsCaptionMenuHidden()))
		{
			PostMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
			return TRUE;
		}
		else if(pMsg->wParam == VK_LEFT && pAMTuner)
		{
			PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
			return TRUE;
		}
		else if(pMsg->wParam == VK_RIGHT && pAMTuner)
		{
			PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			return TRUE;
		}
	}

	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	__super::RecalcLayout(bNotify);

	CRect r;
	GetWindowRect(&r);
	MINMAXINFO mmi;
	memset(&mmi, 0, sizeof(mmi));
	SendMessage(WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
	r |= CRect(r.TopLeft(), CSize(r.Width(), mmi.ptMinTrackSize.y));
	MoveWindow(&r);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	SetAlwaysOnTop(AfxGetAppSettings().fAlwaysOnTop);

	// forward focus to the view window
	if(IsWindow(m_wndView.m_hWnd))
		m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if(m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	POSITION pos = m_bars.GetHeadPosition();
	while(pos) 
	{
		if(m_bars.GetNext(pos)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	pos = m_dockingbars.GetHeadPosition();
	while(pos) 
	{
		if(m_dockingbars.GetNext(pos)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	DWORD style = GetStyle();

	MENUBARINFO mbi;
	memset(&mbi, 0, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

	lpMMI->ptMinTrackSize.x = 10;
	CRect r;
	for(int i = 0; ::GetMenuItemRect(m_hWnd, mbi.hMenu, i, &r); i++)
		lpMMI->ptMinTrackSize.x += r.Width();
	lpMMI->ptMinTrackSize.x = max(DEFCLIENTW, lpMMI->ptMinTrackSize.x);
	if(style&WS_THICKFRAME) lpMMI->ptMinTrackSize.x += GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2;

	memset(&mbi, 0, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

	lpMMI->ptMinTrackSize.y = 0;
	if(style&WS_CAPTION) lpMMI->ptMinTrackSize.y += GetSystemMetrics(SM_CYCAPTION);
	if(style&WS_THICKFRAME) lpMMI->ptMinTrackSize.y += GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2;
	lpMMI->ptMinTrackSize.y += (mbi.rcBar.bottom - mbi.rcBar.top);
	if(!AfxGetAppSettings().fHideCaptionMenu) lpMMI->ptMinTrackSize.y += 3;
	POSITION pos = m_bars.GetHeadPosition();
	while(pos) 
	{
		CControlBar* pCB = m_bars.GetNext(pos);
		if(!IsWindow(pCB->m_hWnd) || !pCB->IsVisible()) continue;

		lpMMI->ptMinTrackSize.y += pCB->CalcFixedLayout(TRUE, TRUE).cy;
	}

	if(IsWindow(m_wndSubresyncBar.m_hWnd) && m_wndSubresyncBar.IsWindowVisible() && !m_wndSubresyncBar.IsFloating())
		lpMMI->ptMinTrackSize.y += m_wndSubresyncBar.CalcFixedLayout(TRUE, TRUE).cy-2;

	if(IsWindow(m_wndPlaylistBar.m_hWnd) && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating())
		lpMMI->ptMinTrackSize.y += m_wndPlaylistBar.CalcFixedLayout(TRUE, TRUE).cy-2;

	if(IsWindow(m_wndCaptureBar.m_hWnd) && m_wndCaptureBar.IsWindowVisible() && !m_wndCaptureBar.IsFloating())
		lpMMI->ptMinTrackSize.y += m_wndCaptureBar.CalcFixedLayout(TRUE, TRUE).cy-2;

	CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

void CMainFrame::OnMove(int x, int y)
{
	CFrameWnd::OnMove(x, y);

	MoveVideoWindow();

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if(!m_fFullScreen && wp.flags != WPF_RESTORETOMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED)
		GetWindowRect(AfxGetAppSettings().rcLastWindowPos);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if(nType == SIZE_RESTORED && !(GetWindowLong(m_hWnd, GWL_EXSTYLE)&WS_EX_APPWINDOW))
	{
		ShowWindow(SW_SHOW);
	}

	if(!m_fFullScreen)
	{
		AppSettings& s = AfxGetAppSettings();
		if(nType != SIZE_MAXIMIZED && nType != SIZE_MINIMIZED)
			GetWindowRect(s.rcLastWindowPos);
		s.lastWindowType = nType;
	}
}

void CMainFrame::OnDisplayChange() // untested, not sure if it's working...
{
	TRACE(_T("*** CMainFrame::OnDisplayChange()\n"));
/*
	if(m_iMediaLoadState == MLS_LOADED && m_pCAP) 
		m_pCAP->OnDisplayChange();
*/
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	if((nID & 0xFFF0) == SC_SCREENSAVE)
	{
		TRACE(_T("SC_SCREENSAVE, nID = %d, lParam = %d\n"), nID, lParam);
	}
	else if((nID & 0xFFF0) == SC_MINIMIZE && !(GetWindowLong(m_hWnd, GWL_EXSTYLE)&WS_EX_APPWINDOW))
	{
		ShowWindow(SW_HIDE);
		return;
	}

	CFrameWnd::OnSysCommand(nID, lParam);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	CFrameWnd::OnActivateApp(bActive, dwThreadID);

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

	if(m_iMediaLoadState == MLS_LOADED && m_fFullScreen && !bActive && (mi.dwFlags&MONITORINFOF_PRIMARY))
	{
		OnViewFullscreen();
	}
}

LRESULT CMainFrame::OnAppCommand(WPARAM wParam, LPARAM lParam)
{
	UINT cmd  = GET_APPCOMMAND_LPARAM(lParam);
	UINT uDevice = GET_DEVICE_LPARAM(lParam);
	UINT dwKeys = GET_KEYSTATE_LPARAM(lParam);

	if(uDevice != FAPPCOMMAND_OEM)
	{
		AppSettings& s = AfxGetAppSettings();

		BOOL fRet = FALSE;

		POSITION pos = s.wmcmds.GetHeadPosition();
		while(pos)
		{
			wmcmd& wc = s.wmcmds.GetNext(pos);
			if(wc.appcmd == cmd && TRUE == SendMessage(WM_COMMAND, wc.cmd)) 
				fRet = TRUE;
		}

		if(fRet) return TRUE;
	}

	return Default();
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == TIMER_STREAMPOSPOLLER && m_iMediaLoadState == MLS_LOADED)
	{
		REFERENCE_TIME rtNow = 0, rtDur = 0;

		if(m_iPlaybackMode == PM_FILE)
		{
			pMS->GetCurrentPosition(&rtNow);
			pMS->GetDuration(&rtDur);

			if(m_rtDurationOverride >= 0) rtDur = m_rtDurationOverride;

			m_wndSeekBar.Enable(rtDur > 0);
			m_wndSeekBar.SetRange(0, rtDur);
			m_wndSeekBar.SetPos(rtNow);
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{
			if(m_fCapturing && m_wndCaptureBar.m_capdlg.m_pMux)
			{
				CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux;
				if(!pMuxMS || FAILED(pMuxMS->GetCurrentPosition(&rtNow))) rtNow = 0;
			}

			if(m_rtDurationOverride >= 0) rtDur = m_rtDurationOverride;
            
			m_wndSeekBar.Enable(false);
			m_wndSeekBar.SetRange(0, rtDur);
			m_wndSeekBar.SetPos(rtNow);
		}

		if(m_pCAP) m_pCAP->SetTime(/*rtNow*/m_wndSeekBar.GetPos());
	}
	else if(nIDEvent == TIMER_STREAMPOSPOLLER2 && m_iMediaLoadState == MLS_LOADED)
	{
		__int64 start, stop, pos;
		m_wndSeekBar.GetRange(start, stop);
		pos = m_wndSeekBar.GetPosReal();

		GUID tf;
		pMS->GetTimeFormat(&tf);

		if(m_iPlaybackMode == PM_CAPTURE && !m_fCapturing)
		{
			CString str = _T("Live");

			long lChannel = 0, lVivSub = 0, lAudSub = 0;
			if(pAMTuner 
			&& m_wndCaptureBar.m_capdlg.IsTunerActive()
			&& SUCCEEDED(pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub)))
			{
				CString ch;
				ch.Format(_T(" (ch%d)"), lChannel);
				str += ch;
			}

			m_wndStatusBar.SetStatusTimer(str);
		}
		else
		{
			m_wndStatusBar.SetStatusTimer(pos, stop, !!m_wndSubresyncBar.IsWindowVisible(), &tf);
		}

		m_wndSubresyncBar.SetTime(pos);

		if(m_pCAP && GetMediaState() == State_Paused) m_pCAP->Paint(true);
	}
	else if(nIDEvent == TIMER_FULLSCREENCONTROLBARHIDER)
	{
		CPoint p;
		GetCursorPos(&p);

		CRect r;
		GetWindowRect(r);
		bool fCursorOutside = !r.PtInRect(p);

		CWnd* pWnd = WindowFromPoint(p);
		if(pWnd && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside))
		{
			if(AfxGetAppSettings().nShowBarsWhenFullScreenTimeOut >= 0)
				ShowControls(CS_NONE, false);
		}
	}
	else if(nIDEvent == TIMER_FULLSCREENMOUSEHIDER)
	{
		CPoint p;
		GetCursorPos(&p);

		CRect r;
		GetWindowRect(r);
		bool fCursorOutside = !r.PtInRect(p);

		CWnd* pWnd = WindowFromPoint(p);
		if(pWnd && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside))
		{
			m_fHideCursor = true;
			SetCursor(NULL);
		}
	}
	else if(nIDEvent == TIMER_STATS)
	{
		if(pQP)
		{
			CString rate;
			if(m_iSpeedLevel >= -11 && m_iSpeedLevel <= 3 && m_iSpeedLevel != -4)
			{
				CString speeds[] = {_T("1/8"),_T("1/4"),_T("1/2"),_T("1"),_T("2"),_T("4"),_T("8")};
				rate = speeds[(m_iSpeedLevel >= -3 ? m_iSpeedLevel : (-m_iSpeedLevel - 8)) + 3];
				if(m_iSpeedLevel < -4) rate = _T("-") + rate;
				if(!rate.IsEmpty()) rate = _T("(") + rate + _T("X)");
			}

			CString info;
			int val;
			pQP->get_AvgFrameRate(&val);
			info.Format(_T("%d.%02d %s"), val/100, val%100, rate);
			m_wndStatsBar.SetLine(_T("Frame-rate"), info);
			pQP->get_AvgSyncOffset(&val);
			info.Format(_T("%d ms"), val);
			m_wndStatsBar.SetLine(_T("Sync Offset (avg)"), info);
			pQP->get_DevSyncOffset(&val);
			info.Format(_T("%d ms"), val);
			m_wndStatsBar.SetLine(_T("Sync Offset (dev)"), info);
			pQP->get_FramesDrawn(&val);
			info.Format(_T("%d"), val);
			m_wndStatsBar.SetLine(_T("Frames Drawn"), info);
			pQP->get_FramesDroppedInRenderer(&val);
			info.Format(_T("%d"), val);
			m_wndStatsBar.SetLine(_T("Frames Dropped"), info);
			pQP->get_Jitter(&val);
			info.Format(_T("%d ms"), val);
			m_wndStatsBar.SetLine(_T("Jitter"), info);
		}

		if(m_iPlaybackMode == PM_DVD) // we also use this timer to update the info panel for dvd playback
		{
			ULONG ulAvailable, ulCurrent;

			// Domain

			CString Domain('-');

			DVD_DOMAIN dom;

			if(SUCCEEDED(pDVDI->GetCurrentDomain(&dom)))
			{
				switch(dom)
				{
				case DVD_DOMAIN_FirstPlay: Domain = _T("First Play"); break;
				case DVD_DOMAIN_VideoManagerMenu: Domain = _T("Video Manager Menu"); break;
				case DVD_DOMAIN_VideoTitleSetMenu: Domain = _T("Video Title Set Menu"); break;
				case DVD_DOMAIN_Title: Domain = _T("Title"); break;
				case DVD_DOMAIN_Stop: Domain = _T("Stop"); break;
				default: Domain = _T("-"); break;
				}
			}

			m_wndInfoBar.SetLine(_T("Domain"), Domain);

			// Location

			CString Location('-');

			DVD_PLAYBACK_LOCATION2 loc;
			ULONG ulNumOfVolumes, ulVolume;
			DVD_DISC_SIDE Side;
			ULONG ulNumOfTitles;
			ULONG ulNumOfChapters;

			if(SUCCEEDED(pDVDI->GetCurrentLocation(&loc))
			&& SUCCEEDED(pDVDI->GetNumberOfChapters(loc.TitleNum, &ulNumOfChapters))
			&& SUCCEEDED(pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles)))
			{
				Location.Format(_T("Volume: %02d/%02d, Title: %02d/%02d, Chapter: %02d/%02d"), 
					ulVolume, ulNumOfVolumes, 
					loc.TitleNum, ulNumOfTitles, 
					loc.ChapterNum, ulNumOfChapters);
			}

			m_wndInfoBar.SetLine(_T("Location"), Location);

			// Video

			CString Video('-');

			DVD_VideoAttributes VATR;

			if(SUCCEEDED(pDVDI->GetCurrentAngle(&ulAvailable, &ulCurrent))
			&& SUCCEEDED(pDVDI->GetCurrentVideoAttributes(&VATR)))
			{
				Video.Format(_T("Angle: %02d/%02d, %dx%d %dHz %d:%d"), 
					ulAvailable, ulCurrent,
					VATR.ulSourceResolutionX, VATR.ulSourceResolutionY, VATR.ulFrameRate,
					VATR.ulAspectX, VATR.ulAspectY);
			}

			m_wndInfoBar.SetLine(_T("Video"), Video);

			// Audio

			CString Audio('-');

			DVD_AudioAttributes AATR;

			if(SUCCEEDED(pDVDI->GetCurrentAudio(&ulAvailable, &ulCurrent))
			&& SUCCEEDED(pDVDI->GetAudioAttributes(ulCurrent, &AATR)))
			{
				CString lang;
				int len = GetLocaleInfo(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
				lang.ReleaseBufferSetLength(max(len-1, 0));

				switch(AATR.LanguageExtension)
				{
				case DVD_AUD_EXT_NotSpecified:
				default: break;
				case DVD_AUD_EXT_Captions: lang += _T(" (Captions)"); break;
				case DVD_AUD_EXT_VisuallyImpaired: lang += _T(" (Visually Impaired)"); break;
				case DVD_AUD_EXT_DirectorComments1: lang += _T(" (Director Comments 1)"); break;
				case DVD_AUD_EXT_DirectorComments2: lang += _T(" (Director Comments 2)"); break;
				}

				CString format;
				switch(AATR.AudioFormat)
				{
				case DVD_AudioFormat_AC3: format = _T("AC3"); break;
				case DVD_AudioFormat_MPEG1: 
				case DVD_AudioFormat_MPEG1_DRC: format = _T("MPEG1"); break;
				case DVD_AudioFormat_MPEG2: 
				case DVD_AudioFormat_MPEG2_DRC: format = _T("MPEG2"); break;
				case DVD_AudioFormat_LPCM: format = _T("LPCM"); break;
				case DVD_AudioFormat_DTS: format = _T("DTS"); break;
				case DVD_AudioFormat_SDDS: format = _T("SDDS"); break;
				case DVD_AudioFormat_Other: 
				default: format = _T("Unknown format"); break;
				}

				Audio.Format(_T("%s, %s %dHz %dbits %d channel(s)"), 
					lang, 
					format,
					AATR.dwFrequency,
					AATR.bQuantization,
					AATR.bNumberOfChannels);

				m_wndStatusBar.SetStatusBitmap(
					AATR.bNumberOfChannels == 1 ? IDB_MONO 
					: AATR.bNumberOfChannels >= 2 ? IDB_STEREO 
					: IDB_NOAUDIO);
			}

			m_wndInfoBar.SetLine(_T("Audio"), Audio);

			// Subtitles

			CString Subtitles('-');

			BOOL bIsDisabled;
			DVD_SubpictureAttributes SATR;

			if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulAvailable, &ulCurrent, &bIsDisabled))
			&& SUCCEEDED(pDVDI->GetSubpictureAttributes(ulCurrent, &SATR)))
			{
				CString lang;
				int len = GetLocaleInfo(SATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
				lang.ReleaseBufferSetLength(max(len-1, 0));

				switch(SATR.LanguageExtension)
				{
				case DVD_SP_EXT_NotSpecified:
				default: break;
				case DVD_SP_EXT_Caption_Normal: lang += _T(""); break;
				case DVD_SP_EXT_Caption_Big: lang += _T(" (Big)"); break;
				case DVD_SP_EXT_Caption_Children: lang += _T(" (Children)"); break;
				case DVD_SP_EXT_CC_Normal: lang += _T(" (CC)"); break;
				case DVD_SP_EXT_CC_Big: lang += _T(" (CC Big)"); break;
				case DVD_SP_EXT_CC_Children: lang += _T(" (CC Children)"); break;
				case DVD_SP_EXT_Forced: lang += _T(" (Forced)"); break;
				case DVD_SP_EXT_DirectorComments_Normal: lang += _T(" (Director Comments)"); break;
				case DVD_SP_EXT_DirectorComments_Big: lang += _T(" (Director Comments, Big)"); break;
				case DVD_SP_EXT_DirectorComments_Children: lang += _T(" (Director Comments, Children)"); break;
				}

				if(bIsDisabled) lang = _T("-");

				Subtitles.Format(_T("%s"), 
					lang);
			}

			m_wndInfoBar.SetLine(_T("Subtitles"), Subtitles);
		}

		if(GetMediaState() == State_Running)
		{
			UINT fSaverActive = 0;
			if(SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&fSaverActive, 0))
			{
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, 0, SPIF_SENDWININICHANGE); // this might not be needed at all...
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, fSaverActive, 0, SPIF_SENDWININICHANGE);
			}

			fSaverActive = 0;
			if(SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, (PVOID)&fSaverActive, 0))
			{
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE, 0, 0, SPIF_SENDWININICHANGE); // this might not be needed at all...
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE, fSaverActive, 0, SPIF_SENDWININICHANGE);
			}
		}
	}
	else if(nIDEvent == TIMER_STATUSERASER)
	{
		KillTimer(TIMER_STATUSERASER);
		m_playingmsg.Empty();
	}

	CFrameWnd::OnTimer(nIDEvent);
}

static bool Shutdown()
{
   HANDLE hToken; 
   TOKEN_PRIVILEGES tkp; 
 
   // Get a token for this process. 
 
   if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	   return(false);
 
   // Get the LUID for the shutdown privilege. 
 
   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
 
   tkp.PrivilegeCount = 1;  // one privilege to set    
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
   // Get the shutdown privilege for this process. 
 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
 
   if(GetLastError() != ERROR_SUCCESS)
	   return(false); 
 
   // Shut down the system and force all applications to close. 
 
   if(!ExitWindowsEx(EWX_SHUTDOWN|EWX_POWEROFF|EWX_FORCEIFHUNG, 0))
	   return(false);

   return(true);
}

//
// our WM_GRAPHNOTIFY handler
//

LRESULT CMainFrame::OnGraphNotify(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

	LONG evCode, evParam1, evParam2;
    while(pME && SUCCEEDED(pME->GetEvent(&evCode, (LONG_PTR*)&evParam1, (LONG_PTR*)&evParam2, 0)))
    {
		CString str;

		if(m_fCustomGraph)
		{
			if(EC_BG_ERROR == evCode)
			{
				str = CString((char*)evParam1);
			}
		}

		hr = pME->FreeEventParams(evCode, evParam1, evParam2);

        if(EC_COMPLETE == evCode)
        {
			AppSettings& s = AfxGetAppSettings();

			if(m_wndPlaylistBar.GetCount() <= 1)
			{
				m_nLoops++;

				if(s.nCLSwitches&CLSW_SHUTDOWN)
				{
					SendMessage(WM_COMMAND, ID_FILE_EXIT);
					Shutdown();
					return hr;
				}

				if(s.nCLSwitches&CLSW_CLOSE)
				{
					SendMessage(WM_COMMAND, ID_FILE_EXIT);
					return hr;
				}

				if(s.fLoopForever || m_nLoops < s.nLoops)
				{
					if(GetMediaState() == State_Stopped)
					{
						SendMessage(WM_COMMAND, ID_PLAY_PLAY);
					}
					else
					{
						LONGLONG pos = 0;
						pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

						if(GetMediaState() == State_Paused)
						{
							SendMessage(WM_COMMAND, ID_PLAY_PLAY);
						}
					}
				}
				else 
				{
					if(s.fRewind) SendMessage(WM_COMMAND, ID_PLAY_STOP);
					else m_fEndOfStream = true;
					SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	
					if(m_fFullScreen && s.fExitFullScreenAtTheEnd) 
						OnViewFullscreen();
				}
			}
			else if(m_wndPlaylistBar.GetCount() > 1)
			{
				if(m_wndPlaylistBar.IsAtEnd())
				{
					if(s.nCLSwitches&CLSW_SHUTDOWN)
					{
						SendMessage(WM_COMMAND, ID_FILE_EXIT);
						Shutdown();
						return hr;
					}

					if(s.nCLSwitches&CLSW_CLOSE)
					{
						SendMessage(WM_COMMAND, ID_FILE_EXIT);
						return hr;
					}

					m_nLoops++;
				}

				if(s.fLoopForever || m_nLoops < s.nLoops)
				{
					int nLoops = m_nLoops;
					PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
					m_nLoops = nLoops;
				}
				else 
				{
					if(m_fFullScreen && s.fExitFullScreenAtTheEnd) 
						OnViewFullscreen();

					if(s.fRewind)
					{
						AfxGetAppSettings().nCLSwitches |= CLSW_OPEN; // HACK
						PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
					}
					else
					{
						m_fEndOfStream = true;
						PostMessage(WM_COMMAND, ID_PLAY_PAUSE);
					}
				}
			}
        }
		else if(EC_ERRORABORT == evCode)
		{
			TRACE(_T("EC_ERRORABORT, hr = %08x\n"), (HRESULT)evParam1);
		}
		else if(EC_REPAINT == evCode)
		{
			TRACE(_T("EC_REPAINT\n"));
		}
		else if(EC_BUFFERING_DATA == evCode)
		{
			TRACE(_T("EC_BUFFERING_DATA, %d, %d\n"), (HRESULT)evParam1, evParam2);

			m_fBuffering = ((HRESULT)evParam1 != S_OK);
		}
		else if(EC_STEP_COMPLETE == evCode)
		{
			if(m_fFrameSteppingActive)
			{
				m_fFrameSteppingActive = false;
				pBA->put_Volume(m_VolumeBeforeFrameStepping);
			}
		}
		else if(EC_DEVICE_LOST == evCode)
		{
			CComQIPtr<IBaseFilter> pBF;
			if(m_iPlaybackMode == PM_CAPTURE 
			&& (!pVidCap && pVidCap == (pBF = (IUnknown*)evParam1) 
				|| !pAudCap && pAudCap == (pBF = (IUnknown*)evParam1))
			&& evParam2 == 0)
			{
				SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			}
		}
		else if(EC_DVD_DOMAIN_CHANGE == evCode)
		{
			m_iDVDDomain = (DVD_DOMAIN)evParam1;
			TRACE(_T("EC_DVD_DOMAIN_CHANGE %d\n"), m_iDVDDomain);
			MoveVideoWindow(); // AR might have changed
		}
		else if(EC_DVD_CURRENT_HMSF_TIME == evCode)
		{
			REFERENCE_TIME rtNow = 0, rtDur = 0;

			double fps = evParam2 == DVD_TC_FLAG_25fps ? 25.0
				: evParam2 == DVD_TC_FLAG_30fps ? 30.0
				: evParam2 == DVD_TC_FLAG_DropFrame ? 29.97
				: 25.0;

			rtNow = HMSF2RT(*((DVD_HMSF_TIMECODE*)&evParam1), fps);

			DVD_HMSF_TIMECODE tcDur;
			ULONG ulFlags;
			if(SUCCEEDED(pDVDI->GetTotalTitleTime(&tcDur, &ulFlags)))
			{
				rtDur = HMSF2RT(tcDur, fps);
			}

			m_wndSeekBar.Enable(rtDur > 0);
			m_wndSeekBar.SetRange(0, rtDur);
			m_wndSeekBar.SetPos(rtNow);

			if(m_pSubClock) m_pSubClock->SetTime(rtNow);
		}
		else if(EC_DVD_ERROR == evCode)
		{
			TRACE(_T("EC_DVD_ERROR %d %d\n"), evParam1, evParam2);

			CString err;

			switch(evParam1)
			{
			case DVD_ERROR_Unexpected: default: err = _T("DVD: Unexpected error"); break;
			case DVD_ERROR_CopyProtectFail: err = _T("DVD: Copy-Protect Fail"); break;
			case DVD_ERROR_InvalidDVD1_0Disc: err = _T("DVD: Invalid DVD 1.x Disc"); break;
			case DVD_ERROR_InvalidDiscRegion: err = _T("DVD: Invalid Disc Region"); break;
			case DVD_ERROR_LowParentalLevel: err = _T("DVD: Low Parental Level"); break;
			case DVD_ERROR_MacrovisionFail: err = _T("DVD: Macrovision Fail"); break;
			case DVD_ERROR_IncompatibleSystemAndDecoderRegions: err = _T("DVD: Incompatible System And Decoder Regions"); break;
			case DVD_ERROR_IncompatibleDiscAndDecoderRegions: err = _T("DVD: Incompatible Disc And Decoder Regions"); break;
			}

			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

			m_closingmsg = err;
		}
		else if(EC_DVD_WARNING == evCode)
		{
			TRACE(_T("EC_DVD_WARNING %d %d\n"), evParam1, evParam2);
		}
		else if(EC_VIDEO_SIZE_CHANGED == evCode)
		{
			TRACE(_T("EC_VIDEO_SIZE_CHANGED %dx%d\n"), CSize(evParam1));

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CSize size(evParam1);
			m_fAudioOnly = (size.cx <= 0 || size.cy <= 0);

			if(AfxGetAppSettings().fRememberZoomLevel
			&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED))
			{
				ZoomVideoWindow();
			}
			else
			{
				MoveVideoWindow();
			}

			if(m_iMediaLoadState == MLS_LOADED
			&& !m_fAudioOnly && (AfxGetAppSettings().nCLSwitches&CLSW_FULLSCREEN))
			{
				PostMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
				AfxGetAppSettings().nCLSwitches &= ~CLSW_FULLSCREEN;
			}
		}
		else if(EC_LENGTH_CHANGED == evCode)
		{
			__int64 rtDur = 0;
			pMS->GetDuration(&rtDur);
			m_wndPlaylistBar.SetCurTime(rtDur);
		}
		else if(!m_fCustomGraph)
		{
			TRACE(_T("%d\n"), evCode);
		}
		else if(EC_BG_AUDIO_CHANGED == evCode)
		{
			int nAudioChannels = evParam1;

			m_wndStatusBar.SetStatusBitmap(nAudioChannels == 1 ? IDB_MONO 
										: nAudioChannels >= 2 ? IDB_STEREO 
										: IDB_NOAUDIO);
		}
		else if(EC_BG_ERROR == evCode)
		{
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			m_closingmsg = !str.IsEmpty() ? str : _T("Unspecified graph error");
			m_wndPlaylistBar.SetCurValid(false);
			break;
		}
	}

    return hr;
}

LRESULT CMainFrame::OnRepaintRenderLess(WPARAM wParam, LPARAM lParam)
{
	MoveVideoWindow();
	return TRUE;
}

LRESULT CMainFrame::OnResumeFromState(WPARAM wParam, LPARAM lParam)
{
	int iPlaybackMode = (int)wParam;

	if(iPlaybackMode == PM_FILE)
	{
		SeekTo(10000i64*int(lParam));
	}
	else if(iPlaybackMode == PM_DVD)
	{
		CComPtr<IDvdState> pDvdState;
		pDvdState.Attach((IDvdState*)lParam);
		if(pDVDC) pDVDC->SetState(pDvdState, DVD_CMD_FLAG_Block, NULL);
	}
	else if(iPlaybackMode == PM_CAPTURE)
	{
		// not implemented
	}
	else
	{
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrame::OnButton(UINT id, UINT nFlags, CPoint point)
{
	SetFocus();

	CRect r;
	m_wndView.GetClientRect(r);
	m_wndView.MapWindowPoints(this, &r);

	if(id != wmcmd::WDOWN && id != wmcmd::WUP && !r.PtInRect(point)) return FALSE;

	BOOL ret = FALSE;

	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.GetHeadPosition();
	while(pos)
	{
		wmcmd& wc = s.wmcmds.GetNext(pos);
		if(wc.mouse == id)
		{
			SendMessage(WM_COMMAND, wc.cmd);
			ret = true;
		}
	}

	return ret;
}

static bool s_fLDown = false;

void CMainFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	bool fClicked = false;

	if(m_iPlaybackMode == PM_DVD)
	{
		CPoint p = point - m_wndView.GetVideoRect().TopLeft();

		if(SUCCEEDED(pDVDC->ActivateAtPosition(p))
		|| m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu 
		|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu)
			fClicked = true;
	}

	if(!fClicked)
	{
		bool fLeftMouseBtnUnassigned = true;
		AppSettings& s = AfxGetAppSettings();
		POSITION pos = s.wmcmds.GetHeadPosition();
		while(pos && fLeftMouseBtnUnassigned)
			if(s.wmcmds.GetNext(pos).mouse == wmcmd::LDOWN)
				fLeftMouseBtnUnassigned = false;

		if(IsCaptionMenuHidden() || fLeftMouseBtnUnassigned)
		{
			PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		}
		else
{
s_fLDown = true;
			OnButton(wmcmd::LDOWN, nFlags, point);
}
	}

	CFrameWnd::OnLButtonDown(nFlags, point);
}

void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnButton(wmcmd::LUP, nFlags, point);
	CFrameWnd::OnLButtonUp(nFlags, point);
}

void CMainFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{
if(s_fLDown)
{
	SendMessage(WM_LBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
s_fLDown = false;
}
	OnButton(wmcmd::LDBLCLK, nFlags, point);
	CFrameWnd::OnLButtonDblClk(nFlags, point);
}

void CMainFrame::OnMButtonDown(UINT nFlags, CPoint point)
{
	SendMessage(WM_CANCELMODE);
	OnButton(wmcmd::MDOWN, nFlags, point);
	CFrameWnd::OnMButtonDown(nFlags, point);
}

void CMainFrame::OnMButtonUp(UINT nFlags, CPoint point)
{
	OnButton(wmcmd::MUP, nFlags, point);
	CFrameWnd::OnMButtonUp(nFlags, point);
}

void CMainFrame::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessage(WM_MBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	OnButton(wmcmd::MDBLCLK, nFlags, point);
	CFrameWnd::OnMButtonDblClk(nFlags, point);
}

void CMainFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
	OnButton(wmcmd::RDOWN, nFlags, point);
	CFrameWnd::OnRButtonDown(nFlags, point);
}

void CMainFrame::OnRButtonUp(UINT nFlags, CPoint point)
{
	OnButton(wmcmd::RUP, nFlags, point);
	CFrameWnd::OnRButtonUp(nFlags, point);
}

void CMainFrame::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessage(WM_RBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	OnButton(wmcmd::RDBLCLK, nFlags, point);
	CFrameWnd::OnRButtonDblClk(nFlags, point);
}

LRESULT CMainFrame::OnXButtonDown(WPARAM wParam, LPARAM lParam)
{
	SendMessage(WM_CANCELMODE);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam); 
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DOWN : fwButton == XBUTTON2 ? wmcmd::X2DOWN : wmcmd::NONE,
		GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

LRESULT CMainFrame::OnXButtonUp(WPARAM wParam, LPARAM lParam)
{
	UINT fwButton = GET_XBUTTON_WPARAM(wParam); 
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1UP : fwButton == XBUTTON2 ? wmcmd::X2UP : wmcmd::NONE,
		GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

LRESULT CMainFrame::OnXButtonDblClk(WPARAM wParam, LPARAM lParam)
{
	SendMessage(WM_XBUTTONDOWN, wParam, lParam);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam); 
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DBLCLK : fwButton == XBUTTON2 ? wmcmd::X2DBLCLK : wmcmd::NONE,
		GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	ScreenToClient(&point);

	BOOL fRet = 
		zDelta > 0 ? OnButton(wmcmd::WUP, nFlags, point) :
		zDelta < 0 ? OnButton(wmcmd::WDOWN, nFlags, point) : 
		FALSE;

	return fRet;
}

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_iPlaybackMode == PM_DVD)
	{
		CPoint p = point - m_wndView.GetVideoRect().TopLeft();
		pDVDC->SelectAtPosition(p);
	}

	CSize diff = m_lastMouseMove - point;

	if(m_fFullScreen && (abs(diff.cx)+abs(diff.cy)) >= 1)
	{
		int nTimeOut = AfxGetAppSettings().nShowBarsWhenFullScreenTimeOut;

		if(nTimeOut < 0)
		{
			m_fHideCursor = false;
			if(AfxGetAppSettings().fShowBarsWhenFullScreen)
				ShowControls(AfxGetAppSettings().nCS);

			KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
		}
		else if(nTimeOut == 0)
		{
			CRect r;
			GetClientRect(r);
			r.top = r.bottom;

			POSITION pos = m_bars.GetHeadPosition();
			for(int i = 1; pos; i <<= 1)
			{
				CControlBar* pNext = m_bars.GetNext(pos);
				CSize s = pNext->CalcFixedLayout(FALSE, TRUE);
				if(AfxGetAppSettings().nCS&i) r.top -= s.cy;
			}

			// HACK: the controls would cover the menu too early hiding some buttons
			if(m_iPlaybackMode == PM_DVD
			&& (m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu
			|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu))
				r.top = r.bottom - 10;

			m_fHideCursor = false;

			if(r.PtInRect(point))
			{
				if(AfxGetAppSettings().fShowBarsWhenFullScreen)
					ShowControls(AfxGetAppSettings().nCS);
			}
			else
			{
				if(AfxGetAppSettings().fShowBarsWhenFullScreen)
					ShowControls(CS_NONE, false);
			}

			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
		}
		else
		{
			m_fHideCursor = false;
			if(AfxGetAppSettings().fShowBarsWhenFullScreen)
				ShowControls(AfxGetAppSettings().nCS);

			SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut*1000, NULL);
			SetTimer(TIMER_FULLSCREENMOUSEHIDER, max(nTimeOut*1000, 2000), NULL);
		}
	}

	m_lastMouseMove = point;

	CFrameWnd::OnMouseMove(nFlags, point);
}


UINT CMainFrame::OnNcHitTest(CPoint point)
{
	UINT nHitTest = CFrameWnd::OnNcHitTest(point);
	return ((IsCaptionMenuHidden()) && nHitTest == HTCLIENT) ? HTCAPTION : nHitTest;
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(pScrollBar->IsKindOf(RUNTIME_CLASS(CVolumeCtrl)))
	{
		OnPlayVolume(0);
	}
	else if(pScrollBar->IsKindOf(RUNTIME_CLASS(CPlayerSeekBar)) && m_iMediaLoadState == MLS_LOADED)
	{
		SeekTo(m_wndSeekBar.GetPos(), !!(::GetKeyState(VK_SHIFT)&0x8000));
	}

	CFrameWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	CFrameWnd::OnInitMenu(pMenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for(UINT i = 0, j = pMenu->GetMenuItemCount(); i < j; i++)
	{
		CString str;
		pMenu->GetMenuString(i, str, MF_BYPOSITION);
		str.Remove('&');

		CMenu* pSubMenu = NULL;

		if(str == _T("Favorites"))
		{
			SetupFavoritesSubMenu();
			pSubMenu = &m_favorites;
		}

		if(pSubMenu)
		{
			mii.fMask = MIIM_STATE|MIIM_SUBMENU;
			mii.fType = MF_POPUP;
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED|MF_GRAYED));
			pMenu->SetMenuItemInfo(i, &mii, TRUE);
		}
	}
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for(UINT i = 0, j = pPopupMenu->GetMenuItemCount(); i < j; i++)
	{
		CString str;
		pPopupMenu->GetMenuString(i, str, MF_BYPOSITION);
		str.Remove('&');

		CMenu* pSubMenu = NULL;

		if(str == _T("Navigate"))
		{
			UINT fState = (m_iMediaLoadState == MLS_LOADED 
				&& (1/*m_iPlaybackMode == PM_DVD *//*|| (m_iPlaybackMode == PM_FILE && m_PlayList.GetCount() > 0)*/)) 
				? MF_ENABLED 
				: (MF_DISABLED|MF_GRAYED);

			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
		}
		else if(str == _T("Video Frame"))
		{
			UINT fState = (m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly) 
				? MF_ENABLED 
				: (MF_DISABLED|MF_GRAYED);

			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
		}
		else if(str == _T("PanScan"))
		{
			UINT fState = (m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly) 
				? MF_ENABLED 
				: (MF_DISABLED|MF_GRAYED);

			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
		}
		else if(str == _T("Zoom"))
		{
			UINT fState = (m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly) 
				? MF_ENABLED 
				: (MF_DISABLED|MF_GRAYED);

			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
		}
		else if(str == _T("Open CD-ROM"))
		{
			SetupOpenCDSubMenu();
			pSubMenu = &m_opencds;
		}
		else if(str == _T("Filters"))
		{
			SetupFiltersSubMenu();
			pSubMenu = &m_filters;
		}
		else if(str == _T("Audio"))
		{
			SetupAudioSwitcherSubMenu();
			pSubMenu = &m_audios;
		}
		else if(str == _T("Subtitles"))
		{
			SetupSubtitlesSubMenu();
			pSubMenu = &m_subtitles;
		}
		else if(str == _T("Audio Language"))
		{
			SetupNavAudioSubMenu();
			pSubMenu = &m_navaudio;
		}
		else if(str == _T("Subtitle Language"))
		{
			SetupNavSubtitleSubMenu();
			pSubMenu = &m_navsubtitle;
		}
		else if(str == _T("Video Angle"))
		{
			SetupNavAngleSubMenu();
			pSubMenu = &m_navangle;
		}
		else if(str == _T("Jump To..."))
		{
			SetupNavChaptersSubMenu();
			pSubMenu = &m_navchapters;
		}
		else if(str == _T("Favorites"))
		{
			SetupFavoritesSubMenu();
			pSubMenu = &m_favorites;
		}

		if(pSubMenu)
		{
			mii.fMask = MIIM_STATE|MIIM_SUBMENU;
			mii.fType = MF_POPUP;
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED|MF_GRAYED));
			pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);
		}
	}

	//

	for(UINT i = 0, j = pPopupMenu->GetMenuItemCount(); i < j; i++)
	{
		UINT nID = pPopupMenu->GetMenuItemID(i);
		if(nID == ID_SEPARATOR || nID == -1) continue;

		CString str;
		pPopupMenu->GetMenuString(i, str, MF_BYPOSITION);
		int k = str.Find('\t');
		if(k > 0) str = str.Left(k);

		CString key = CPPageAccelTbl::MakeAccelShortcutLabel(nID);
		if(!key.IsEmpty()) str += _T("\t") + key;

		if(key.IsEmpty() && i < 0) continue;

		// BUG(?): this disables menu item update ui calls for some reason...
//		pPopupMenu->ModifyMenu(i, MF_BYPOSITION|MF_STRING, nID, str);

		// this works fine
		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = (LPTSTR)(LPCTSTR)str;
		pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);

	}

	//

	bool fPnSPresets = false;

	for(UINT i = 0, j = pPopupMenu->GetMenuItemCount(); i < j; i++)
	{
		UINT nID = pPopupMenu->GetMenuItemID(i);

		if(nID >= ID_PANNSCAN_PRESETS_START && nID < ID_PANNSCAN_PRESETS_END)
		{
			do
			{
				nID = pPopupMenu->GetMenuItemID(i);
				pPopupMenu->DeleteMenu(i, MF_BYPOSITION);
				j--;
			}
			while(i < j && nID >= ID_PANNSCAN_PRESETS_START && nID < ID_PANNSCAN_PRESETS_END);

			nID = pPopupMenu->GetMenuItemID(i);
		}

		if(nID == ID_VIEW_RESET)
		{
			fPnSPresets = true;
		}
	}

	if(fPnSPresets)
	{
		AppSettings& s = AfxGetAppSettings();
		int i = 0, j = s.m_pnspresets.GetCount();
		for(; i < j; i++)
		{
			int k = 0;
			CString label = s.m_pnspresets[i].Tokenize(_T(","), k);
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND, ID_PANNSCAN_PRESETS_START+i, label);
		}
//		if(j > 0)
		{
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND, ID_PANNSCAN_PRESETS_START+i, _T("Edit..."));
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND|MF_SEPARATOR);
		}
	}
}

void CMainFrame::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
/*
	KillTimer(TIMER_FULLSCREENMOUSEHIDER);
	m_fHideCursor = false;

	CRect r;
	m_wndView.GetWindowRect(r);
	if(r.PtInRect(point))
	{
		if(IsCaptionMenuHidden())
		{
			m_popupmain.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NOANIMATION, point.x, point.y, this);
		}
		else
		{
			m_popup.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NOANIMATION, point.x, point.y, this);
		}
	}
*/
}

BOOL CMainFrame::OnMenu(CMenu* pMenu)
{
	if(!pMenu) return FALSE;

	KillTimer(TIMER_FULLSCREENMOUSEHIDER);
	m_fHideCursor = false;

	CPoint point;
	GetCursorPos(&point);

	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NOANIMATION, point.x, point.y, this);

	return TRUE;
}

void CMainFrame::OnMenuPlayerShort()
{
	if(IsCaptionMenuHidden())
	{
		OnMenu(m_popupmain.GetSubMenu(0));
	}
	else
	{
		OnMenu(m_popup.GetSubMenu(0));
	}
}

void CMainFrame::OnMenuPlayerLong()
{
	OnMenu(m_popupmain.GetSubMenu(0));
}

void CMainFrame::OnMenuFilters()
{
	SetupFiltersSubMenu();
	OnMenu(&m_filters);
}

void CMainFrame::OnUpdatePlayerStatus(CCmdUI* pCmdUI)
{
	if(m_iMediaLoadState == MLS_LOADING)
	{
		pCmdUI->SetText(_T("Opening..."));
	}
	else if(m_iMediaLoadState == MLS_LOADED)
	{
		CString msg;

		if(!m_playingmsg.IsEmpty())
		{
			msg = m_playingmsg;
		}
		else if(m_fCapturing)
		{
			msg = _T("Capturing...");

			if(pAMDF)
			{
				long lDropped = 0;
				pAMDF->GetNumDropped(&lDropped);
				long lNotDropped = 0;
				pAMDF->GetNumNotDropped(&lNotDropped);

				if((lDropped + lNotDropped) > 0)
				{
					CString str;
					str.Format(_T(", Total: %d, Dropped: %d"), lDropped + lNotDropped, lDropped);
					msg += str;
				}
			}

			CComPtr<IPin> pPin;
			if(SUCCEEDED(pCGB->FindPin(m_wndCaptureBar.m_capdlg.m_pDst, PINDIR_INPUT, NULL, NULL, FALSE, 0, &pPin)))
			{
				LONGLONG size = 0;
				if(CComQIPtr<IStream> pStream = pPin)
				{
					pStream->Commit(STGC_DEFAULT);

					WIN32_FIND_DATA findFileData;
					HANDLE h = FindFirstFile(m_wndCaptureBar.m_capdlg.m_file, &findFileData);
					if(h != INVALID_HANDLE_VALUE)
					{
						size = ((LONGLONG)findFileData.nFileSizeHigh << 32) | findFileData.nFileSizeLow;

						CString str;
						if(size < 1024i64*1024)
							str.Format(_T(", Size: %I64dKB"), size/1024);
						else //if(size < 1024i64*1024*1024)
							str.Format(_T(", Size: %I64dMB"), size/1024/1024);
						msg += str;
						
						FindClose(h);
					}
				}

				ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
				if(GetDiskFreeSpaceEx(
					m_wndCaptureBar.m_capdlg.m_file.Left(m_wndCaptureBar.m_capdlg.m_file.ReverseFind('\\')+1), 
					&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
				{
					CString str;
					if(FreeBytesAvailable.QuadPart < 1024i64*1024)
						str.Format(_T(", Free: %I64dKB"), FreeBytesAvailable.QuadPart/1024);
					else //if(FreeBytesAvailable.QuadPart < 1024i64*1024*1024)
						str.Format(_T(", Free: %I64dMB"), FreeBytesAvailable.QuadPart/1024/1024);
					msg += str;
				}

				if(m_wndCaptureBar.m_capdlg.m_pMux)
				{
					__int64 pos = 0;
					CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux;
					if(pMuxMS && SUCCEEDED(pMuxMS->GetCurrentPosition(&pos)) && pos > 0)
					{
						double bytepersec = 10000000.0 * size / pos;
						if(bytepersec > 0)
							m_rtDurationOverride = __int64(10000000.0 * (FreeBytesAvailable.QuadPart+size) / bytepersec);
					}
				}

				if(m_wndCaptureBar.m_capdlg.m_pVidBuffer
				|| m_wndCaptureBar.m_capdlg.m_pAudBuffer)
				{
					int nFreeVidBuffers = 0, nFreeAudBuffers = 0;
					if(CComQIPtr<IBufferFilter> pVB = m_wndCaptureBar.m_capdlg.m_pVidBuffer)
						nFreeVidBuffers = pVB->GetFreeBuffers();
					if(CComQIPtr<IBufferFilter> pAB = m_wndCaptureBar.m_capdlg.m_pAudBuffer)
						nFreeAudBuffers = pAB->GetFreeBuffers();

					CString str;
					str.Format(_T(", Free V/A Buffers: %03d/%03d"), nFreeVidBuffers, nFreeAudBuffers);
					msg += str;
				}
			}
		}
		else if(m_fBuffering)
		{
			BeginEnumFilters(pGB, pEF, pBF)
			{
				if(CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF)
				{
					long BufferingProgress = 0;
					if(SUCCEEDED(pAMNS->get_BufferingProgress(&BufferingProgress)) && BufferingProgress > 0)
					{
						msg.Format(_T("Buffering... (%d%%)"), BufferingProgress);

						__int64 start = 0, stop = 0;
						m_wndSeekBar.GetRange(start, stop);
						m_fLiveWM = (stop == start);
					}
					break;
				}
			}
			EndEnumFilters
		}
		else if(pAMOP)
		{
			__int64 t, c;
			if(SUCCEEDED(pAMOP->QueryProgress(&t, &c)) && t > 0 && c < t)
			{
				msg.Format(_T("Buffering... (%d%%)"), c*100/t);
			}

			if(FindFilter(__uuidof(CShoutcastSource), pGB))
				OpenSetupInfoBar();
		}

		OAFilterState fs = GetMediaState();
		pCmdUI->SetText(
			!msg.IsEmpty() ? msg : 
			fs == State_Stopped ? _T("Stopped") :
			(fs == State_Paused || m_fFrameSteppingActive) ? _T("Paused") :
			fs == State_Running ? _T("Playing") :
			_T(""));
	}
	else if(m_iMediaLoadState == MLS_CLOSING)
	{
		pCmdUI->SetText(_T("Closing..."));
	}
	else
	{
		pCmdUI->SetText(m_closingmsg);
	}
}

void CMainFrame::OnFilePostOpenmedia()
{
	OpenSetupInfoBar();

	OpenSetupStatsBar();

	OpenSetupStatusBar();

	OpenSetupToolBar();

	OpenSetupCaptureBar();

	__int64 rtDur = 0;
	pMS->GetDuration(&rtDur);
	m_wndPlaylistBar.SetCurTime(rtDur);

	if(m_iPlaybackMode == PM_CAPTURE)
	{
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE);
		ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
		ShowControlBar(&m_wndCaptureBar, TRUE, TRUE);
	}

	m_iMediaLoadState = MLS_LOADED;

	// IMPORTANT: must not call any windowing msgs before
	// this point, it will deadlock when OpenMediaPrivate is
	// still running and the renderer window was created on
	// the same worker-thread

	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(&wp);

		// restore magnification
		if(IsWindowVisible() && AfxGetAppSettings().fRememberZoomLevel
		&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED))
		{
			ZoomVideoWindow();
		}
	}

	if(!m_fAudioOnly && (AfxGetAppSettings().nCLSwitches&CLSW_FULLSCREEN))
	{
		SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
		AfxGetAppSettings().nCLSwitches &= ~CLSW_FULLSCREEN;
	}
}

void CMainFrame::OnUpdateFilePostOpenmedia(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADING);
}

void CMainFrame::OnFilePostClosemedia()
{
	m_wndView.SetVideoRect();
	m_wndSeekBar.Enable(false);
	m_wndSeekBar.SetPos(0);
	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();		
	m_wndStatusBar.Clear();
	m_wndStatusBar.ShowTimer(false);

	if(IsWindow(m_wndSubresyncBar.m_hWnd))
	{
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE); 
		SetSubtitle(NULL);
	}

	if(IsWindow(m_wndCaptureBar.m_hWnd))
	{
		ShowControlBar(&m_wndCaptureBar, FALSE, TRUE);
		m_wndCaptureBar.m_capdlg.SetupVideoControls(_T(""), NULL, NULL, NULL);
		m_wndCaptureBar.m_capdlg.SetupAudioControls(_T(""), NULL, CInterfaceArray<IAMAudioInputMixer>());
	}

	RecalcLayout();

	SetWindowText(ResStr(IDR_MAINFRAME));

	// this will prevent any further UI updates on the dynamically added menu items
	SetupFiltersSubMenu();
	SetupAudioSwitcherSubMenu();
	SetupSubtitlesSubMenu();
	SetupNavAudioSubMenu();
	SetupNavSubtitleSubMenu();
	SetupNavAngleSubMenu();
	SetupNavChaptersSubMenu();
	SetupFavoritesSubMenu();
}

void CMainFrame::OnUpdateFilePostClosemedia(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_hWnd && m_iMediaLoadState == MLS_CLOSING);
}

void CMainFrame::OnBossKey()
{
	SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	if(m_fFullScreen) SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);
}

void CMainFrame::OnStreamAudio(UINT nID)
{
	nID -= ID_STREAM_AUDIO_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

	DWORD cStreams = 0;
	if(pSS && SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1)
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(pSS->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				return;
			if(pmt) DeleteMediaType(pmt);
			if(pszName) CoTaskMemFree(pszName);
			
			if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE))
			{
				pSS->Enable((i+(nID==0?1:cStreams-1))%cStreams, AMSTREAMSELECTENABLE_ENABLE);
				break;
			}
		}
	}
	else if(m_iPlaybackMode == PM_FILE) SendMessage(WM_COMMAND, ID_OGM_AUDIO_NEXT+nID);
	else if(m_iPlaybackMode == PM_DVD) SendMessage(WM_COMMAND, ID_DVD_AUDIO_NEXT+nID);
}

void CMainFrame::OnStreamSub(UINT nID)
{
	nID -= ID_STREAM_SUB_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	int cnt = 0;
	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos) cnt += m_pSubStreams.GetNext(pos)->GetStreamCount();

	if(cnt > 1)
	{
		int i = ((m_iSubtitleSel&0x7fffffff)+(nID==0?1:cnt-1))%cnt;
		m_iSubtitleSel = i | (m_iSubtitleSel&0x80000000);

		UpdateSubtitle();

		SetFocus();
	}
	else if(m_iPlaybackMode == PM_FILE) SendMessage(WM_COMMAND, ID_OGM_SUB_NEXT+nID);
	else if(m_iPlaybackMode == PM_DVD) SendMessage(WM_COMMAND, ID_DVD_SUB_NEXT+nID);
}

void CMainFrame::OnOgmAudio(UINT nID)
{
	nID -= ID_OGM_AUDIO_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB);
	if(!pSS) return;

    CUIntArray snds;
	int iSel = -1;

	DWORD cStreams = 0;
	if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1)
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(pSS->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				return;

			CStringW name(pszName), sound(L"sound");
			name.MakeLower();
			if(name.GetLength() >= sound.GetLength() && name.Left(sound.GetLength()) == sound)
			{
				if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE))
					iSel = snds.GetCount();
				snds.Add(i);
			}

			if(pmt) DeleteMediaType(pmt);
			if(pszName) CoTaskMemFree(pszName);
		
		}

		int cnt = snds.GetCount();
		if(cnt > 1 && iSel >= 0)
			pSS->Enable(snds[(iSel+(nID==0?1:cnt-1))%cnt], AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnOgmSub(UINT nID)
{
	nID -= ID_OGM_SUB_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB);
	if(!pSS) return;

    CUIntArray subs;
	int iSel = -1;

	DWORD cStreams = 0;
	if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1)
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(pSS->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				return;

			CStringW name(pszName), subtitle(L"subtitle");
			name.MakeLower();
			if(name.GetLength() >= subtitle.GetLength() && name.Left(subtitle.GetLength()) == subtitle)
			{
				if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE))
					iSel = subs.GetCount();
				subs.Add(i);
			}

			if(pmt) DeleteMediaType(pmt);
			if(pszName) CoTaskMemFree(pszName);
		
		}

		int cnt = subs.GetCount();
		if(cnt > 1 && iSel >= 0)
			pSS->Enable(subs[(iSel+(nID==0?1:cnt-1))%cnt], AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnDvdAngle(UINT nID)
{
	nID -= ID_DVD_ANGLE_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG ulAnglesAvailable, ulCurrentAngle;
		if(SUCCEEDED(pDVDI->GetCurrentAngle(&ulAnglesAvailable, &ulCurrentAngle))
		&& ulAnglesAvailable > 1)
		{
			pDVDC->SelectAngle(
				(ulCurrentAngle+(nID==0?1:ulAnglesAvailable-1))%ulAnglesAvailable, 
				DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnDvdAudio(UINT nID)
{
	nID -= ID_DVD_AUDIO_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG nStreamsAvailable, nCurrentStream;
		if(SUCCEEDED(pDVDI->GetCurrentAudio(&nStreamsAvailable, &nCurrentStream))
		&& nStreamsAvailable > 1)
		{
			pDVDC->SelectAudioStream(
				(nCurrentStream+(nID==0?1:nStreamsAvailable-1))%nStreamsAvailable, 
				DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnDvdSub(UINT nID)
{
	nID -= ID_DVD_SUB_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
		&& ulStreamsAvailable > 1)
		{
			pDVDC->SelectSubpictureStream(
				(ulCurrentStream+(nID==0?1:ulStreamsAvailable-1))%ulStreamsAvailable, 
				DVD_CMD_FLAG_Block, NULL);
		}
	}
}

//
// menu item handlers
//

// file

void CMainFrame::OnFileOpenmedia()
{
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) return;

	COpenDlg dlg;
	if(dlg.DoModal() != IDOK || dlg.m_fns.GetCount() == 0) return;

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	m_wndPlaylistBar.Open(dlg.m_fns, dlg.m_fMultipleFiles);
	OpenCurPlaylistItem();
}

void CMainFrame::OnUpdateFileOpen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState != MLS_LOADING);
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS)
{
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar))
		return FALSE;

	if(pCDS->dwData != 0x6ABE51 || pCDS->cbData < sizeof(DWORD))
		return FALSE;

	DWORD len = *((DWORD*)pCDS->lpData);
	TCHAR* pBuff = (TCHAR*)((DWORD*)pCDS->lpData + 1);
	TCHAR* pBuffEnd = (TCHAR*)((BYTE*)pBuff + pCDS->cbData - sizeof(DWORD));

	CStringList cmdln;

	while(len-- > 0)
	{
		CString str;
		while(pBuff < pBuffEnd && *pBuff) str += *pBuff++;
		pBuff++;
		cmdln.AddTail(str);
	}

	AppSettings& s = AfxGetAppSettings();

	s.ParseCommandLine(cmdln);

	POSITION pos = s.slFilters.GetHeadPosition();
	while(pos)
	{
		CString fullpath = MakeFullPath(s.slFilters.GetNext(pos));

		CPath tmp(fullpath);
		tmp.RemoveFileSpec();
		tmp.AddBackslash();
		CString path = tmp;

		WIN32_FIND_DATA fd = {0};
		HANDLE hFind = FindFirstFile(fullpath, &fd);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;

				CFilterMapper2 fm2(false);
				fm2.Register(path + fd.cFileName);
				while(!fm2.m_filters.IsEmpty())
				{
					if(Filter* f = fm2.m_filters.RemoveTail())
					{
						f->fTemporary = true;

						bool fFound = false;

						POSITION pos2 = s.filters.GetHeadPosition();
						while(pos2)
						{
							Filter* f2 = s.filters.GetNext(pos2);
							if(f2->type == Filter::EXTERNAL && !f2->path.CompareNoCase(f->path))
							{
								fFound = true;
								break;
							}
						}

						if(!fFound)
						{
							CAutoPtr<Filter> p(f);
							s.filters.AddHead(p);
						}
					}
				}
			}
			while(FindNextFile(hFind, &fd));
			
			FindClose(hFind);
		}
	}

	if((s.nCLSwitches&CLSW_DVD) && !s.slFiles.IsEmpty())
	{
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		CAutoPtr<OpenDVDData> p(new OpenDVDData());
		if(p) {p->path = s.slFiles.GetHead(); p->subs.AddTail(&s.slSubs);}
		OpenMedia(p);
	}
	else if(s.nCLSwitches&CLSW_CD)
	{
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		CStringList sl;

		if(!s.slFiles.IsEmpty())
		{
			GetCDROMType(s.slFiles.GetHead()[0], sl);
		}
		else
		{
			for(TCHAR drive = 'C'; sl.IsEmpty() && drive <= 'Z'; drive++)
			{
				sl.RemoveAll();
				GetCDROMType(drive, sl);
			}
		}

		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	}
	else if(!s.slFiles.IsEmpty())
	{
		bool fMulti = s.slFiles.GetCount() > 1;

		CStringList sl;
		sl.AddTail((CStringList*)&s.slFiles);
		if(!fMulti) sl.AddTail((CStringList*)&s.slDubs);

		if((s.nCLSwitches&CLSW_ADD) && m_wndPlaylistBar.GetCount() > 0)
		{
			m_wndPlaylistBar.Append(sl, fMulti, &s.slSubs);

 			if(s.nCLSwitches&(CLSW_OPEN|CLSW_PLAY))
			{
				m_wndPlaylistBar.SetLast();
				OpenCurPlaylistItem();
			}
		}
		else
		{
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

			m_wndPlaylistBar.Open(sl, fMulti, &s.slSubs);
			OpenCurPlaylistItem((s.nCLSwitches&CLSW_STARTVALID) ? s.rtStart : 0);

			s.nCLSwitches &= ~CLSW_STARTVALID;
			s.rtStart = 0;
		}
	}
	else
	{
		s.nCLSwitches = CLSW_NONE;
	}

	return TRUE;
}

void CMainFrame::OnFileOpendvd()
{
	if(m_iMediaLoadState == MLS_LOADING) return;

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	CAutoPtr<OpenDVDData> p(new OpenDVDData());
	if(p)
	{
        AppSettings& s = AfxGetAppSettings();
		if(s.fUseDVDPath && !s.sDVDPath.IsEmpty())
		{
			p->path = s.sDVDPath;
			p->path.Replace('/', '\\');
			if(p->path[p->path.GetLength()-1] != '\\') p->path += '\\';
		}
	}
	OpenMedia(p);
}

void CMainFrame::OnFileOpendevice()
{
	if(m_iMediaLoadState == MLS_LOADING) return;

	COpenCapDeviceDlg capdlg;
	if(capdlg.DoModal() != IDOK)
		return;

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	CAutoPtr<OpenDeviceData> p(new OpenDeviceData());
	if(p) {p->DisplayName[0] = capdlg.m_vidstr; p->DisplayName[1] = capdlg.m_audstr;}
	OpenMedia(p);
}

void CMainFrame::OnFileOpenCD(UINT nID)
{
	nID -= ID_FILE_OPEN_CD_START;

	nID++;
	for(TCHAR drive = 'C'; drive <= 'Z'; drive++)
	{
		CStringList sl;

		switch(GetCDROMType(drive, sl))
		{
		case CDROM_Audio:
		case CDROM_VideoCD:
		case CDROM_DVDVideo:
			nID--;
			break;
		default:
			break;
		}

		if(nID == 0)
		{
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

			ShowWindow(SW_SHOW);
			SetForegroundWindow();

			m_wndPlaylistBar.Open(sl, true);
			OpenCurPlaylistItem();

			break;
		}
	}
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	SetForegroundWindow();

	if(m_wndPlaylistBar.IsWindowVisible())
	{
		m_wndPlaylistBar.OnDropFiles(hDropInfo);
		return;
	}

	CStringList sl;

	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	for(UINT iFile = 0; iFile < nFiles; iFile++)
	{
		CString fn;
		fn.ReleaseBuffer(::DragQueryFile(hDropInfo, iFile, fn.GetBuffer(MAX_PATH), MAX_PATH));
		sl.AddTail(fn);
	}
	::DragFinish(hDropInfo);

	if(!sl.IsEmpty())
	{
		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	}
}

void CMainFrame::OnFileSaveas()
{
	CString in = m_wndPlaylistBar.GetCur(), out = in;

	if(out.Find(_T("://")) < 0)
	{
		CString ext = out.Mid(out.ReverseFind('.')+1).MakeLower();
		if(ext == _T("cda")) out = out.Left(out.GetLength()-3) + _T("wav");
		else if(ext == _T("ifo")) out = out.Left(out.GetLength()-3) + _T("vob");
	}
	else
	{
		out.Empty();
	}

	CFileDialog fd(FALSE, 0, out, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
		_T("All files (*.*)|*.*||"), this, 0); 
	if(fd.DoModal() != IDOK || !in.CompareNoCase(fd.GetPathName())) return;

	OAFilterState fs = State_Stopped;
	pMC->GetState(0, &fs);
	if(fs == State_Running) pMC->Pause();

	CSaveDlg dlg(in, fd.GetPathName());
	dlg.DoModal();

	if(fs == State_Running) pMC->Run();
}

void CMainFrame::OnUpdateFileSaveas(CCmdUI* pCmdUI)
{
	if(m_iMediaLoadState != MLS_LOADED || m_iPlaybackMode != PM_FILE)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

    CString fn = m_wndPlaylistBar.GetCur();
	CString ext = fn.Mid(fn.ReverseFind('.')+1).MakeLower();

	if(fn.Find(_T("://")) >= 0)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if((GetVersion()&0x80000000) && (ext == _T("cda") || ext == _T("ifo")))
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnFileLoadsubtitles()
{
	if(!m_pCAP)
	{
		AfxMessageBox(_T("To load subtitles you have change the video renderer type and reopen the file.\n")
					_T("- DirectShow: VMR7 or VMR9 renderless\n")
					_T("- RealMedia: Special renderer for RealMedia, or open it through DirectShow\n")
					_T("- Quicktime: DX7 or DX9 renderer for QuickTime\n")
					_T("- ShockWave: n/a\n")
					, MB_OK);
		return;
	}

	static TCHAR BASED_CODE szFilter[] = 
		_T(".srt .sub .ssa .ass .smi .psb .txt .idx .usf .xss|")
		_T("*.srt;*.sub;*.ssa;*.ass;*smi;*.psb;*.txt;*.idx;*.usf;*.xss||");

	CFileDialog fd(TRUE, NULL, NULL, 
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY, 
		szFilter, this, 0);

	if(fd.DoModal() != IDOK) return;

	if(LoadSubtitle(fd.GetPathName()))
		SetSubtitle(m_pSubStreams.GetTail());
}

void CMainFrame::OnUpdateFileLoadsubtitles(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && /*m_pCAP &&*/ !m_fAudioOnly);
}

void CMainFrame::OnFileSavesubtitles()
{
	int i = m_iSubtitleSel;

	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos && i >= 0)
	{
		CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

		if(i < pSubStream->GetStreamCount())
		{
			CLSID clsid;
			if(FAILED(pSubStream->GetClassID(&clsid)))
				continue;

			if(clsid == __uuidof(CVobSubFile))
			{
				CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;

				CFileDialog fd(FALSE, NULL, NULL, 
					OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
					_T("VobSub (*.idx, *.sub)|*.idx;*.sub||"), this, 0);
		
				if(fd.DoModal() == IDOK)
				{
					CAutoLock cAutoLock(&m_csSubLock);
					pVSF->Save(fd.GetPathName());
				}
	            
				return;
			}
			else if(clsid == __uuidof(CRenderedTextSubtitle))
			{
				CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

				CString filter;
				filter += _T("Subripper (*.srt)|*.srt|");
				filter += _T("Microdvd (*.sub)|*.sub|");
				filter += _T("Sami (*.smi)|*.smi|");
				filter += _T("Psb (*.psb)|*.psb|");
				filter += _T("Sub Station Alpha (*.ssa)|*.ssa|");
				filter += _T("Advanced Sub Station Alpha (*.ass)|*.ass|");
				filter += _T("|");

				CSaveTextFileDialog fd(pRTS->m_encoding, NULL, NULL, filter, this);
		
				if(fd.DoModal() == IDOK)
				{
					CAutoLock cAutoLock(&m_csSubLock);
					pRTS->SaveAs(fd.GetPathName(), (exttype)(fd.m_ofn.nFilterIndex-1), m_pCAP->GetFPS(), fd.GetEncoding());
				}
	            
				return;
			}
		}

		i -= pSubStream->GetStreamCount();
	}
}

void CMainFrame::OnUpdateFileSavesubtitles(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iSubtitleSel >= 0);
}

void CMainFrame::OnFileProperties()
{
	CPPageFileInfoSheet m_fileinfo(m_wndPlaylistBar.GetCur(), this);
	m_fileinfo.DoModal();
}

void CMainFrame::OnUpdateFileProperties(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_FILE);
}

void CMainFrame::OnFileCloseMedia()
{
	CloseMedia();
}

void CMainFrame::OnFileClosePlaylist()
{
	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	RestoreDefaultWindowRect();
}

void CMainFrame::OnUpdateFileClose(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED || m_iMediaLoadState == MLS_LOADING);
}

// view

void CMainFrame::OnViewCaptionmenu()
{
	bool fHideCaptionMenu = AfxGetAppSettings().fHideCaptionMenu;

	AfxGetAppSettings().fHideCaptionMenu = !fHideCaptionMenu;

	if(m_fFullScreen) return;

	DWORD dwRemove = 0, dwAdd = 0;
	HMENU hMenu;

	if(!fHideCaptionMenu)
	{
		dwRemove = WS_CAPTION;
		hMenu = NULL;
	}
	else
	{
		dwAdd = WS_CAPTION;
		hMenu = m_hMenuDefault;
	}

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	::SetMenu(m_hWnd, hMenu);
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);

	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewCaptionmenu(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!AfxGetAppSettings().fHideCaptionMenu);
}

void CMainFrame::OnViewControlBar(UINT nID)
{
	nID -= ID_VIEW_SEEKER;
	ShowControls(AfxGetAppSettings().nCS ^ (1<<nID));
}

void CMainFrame::OnUpdateViewControlBar(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_VIEW_SEEKER;
	pCmdUI->SetCheck(!!(AfxGetAppSettings().nCS & (1<<nID)));
}

void CMainFrame::OnViewSubresync()
{
	ShowControlBar(&m_wndSubresyncBar, !m_wndSubresyncBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewSubresync(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndSubresyncBar.IsWindowVisible());
	pCmdUI->Enable(m_pCAP && m_iSubtitleSel >= 0);
}

void CMainFrame::OnViewPlaylist()
{
	ShowControlBar(&m_wndPlaylistBar, !m_wndPlaylistBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewPlaylist(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPlaylistBar.IsWindowVisible());
	pCmdUI->Enable(m_iMediaLoadState == MLS_CLOSED && m_iMediaLoadState != MLS_LOADED
		|| m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_FILE);
}

void CMainFrame::OnViewCapture()
{
	ShowControlBar(&m_wndCaptureBar, !m_wndCaptureBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewCapture(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndCaptureBar.IsWindowVisible());
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_CAPTURE);
}

void CMainFrame::OnViewMinimal()
{
	if(!AfxGetAppSettings().fHideCaptionMenu)
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	ShowControls(0);
}

void CMainFrame::OnUpdateViewMinimal(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewCompact()
{
	if(AfxGetAppSettings().fHideCaptionMenu)
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	ShowControls(CS_TOOLBAR);
}

void CMainFrame::OnUpdateViewCompact(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewNormal()
{
	if(AfxGetAppSettings().fHideCaptionMenu)
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	ShowControls(CS_SEEKBAR|CS_TOOLBAR|CS_STATUSBAR|CS_INFOBAR);
}

void CMainFrame::OnUpdateViewNormal(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewFullscreen()
{
	ToggleFullscreen(true, true);
}

void CMainFrame::OnViewFullscreenSecondary()
{
	ToggleFullscreen(true, false);
}

void CMainFrame::OnUpdateViewFullscreen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly || m_fFullScreen);
	pCmdUI->SetCheck(m_fFullScreen);
}

void CMainFrame::OnViewZoom(UINT nID)
{
	ZoomVideoWindow(nID == ID_VIEW_ZOOM_50 ? 0.5 : nID == ID_VIEW_ZOOM_200 ? 2.0 : 1.0);
}

void CMainFrame::OnUpdateViewZoom(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
}

void CMainFrame::OnViewDefaultVideoFrame(UINT nID)
{
	AfxGetAppSettings().iDefaultVideoSize = nID - ID_VIEW_VF_HALF;
	m_ZoomX = m_ZoomY = 1; 
	m_PosX = m_PosY = 0.5;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
	pCmdUI->SetRadio(AfxGetAppSettings().iDefaultVideoSize == (pCmdUI->m_nID - ID_VIEW_VF_HALF));
}

void CMainFrame::OnViewKeepaspectratio()
{
	AfxGetAppSettings().fKeepAspectRatio = !AfxGetAppSettings().fKeepAspectRatio;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
	pCmdUI->SetCheck(AfxGetAppSettings().fKeepAspectRatio);
}

void CMainFrame::OnViewCompMonDeskARDiff()
{
	AfxGetAppSettings().fCompMonDeskARDiff = !AfxGetAppSettings().fCompMonDeskARDiff;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
	pCmdUI->SetCheck(AfxGetAppSettings().fCompMonDeskARDiff);
}

void CMainFrame::OnViewPanNScan(UINT nID)
{
	if(m_iMediaLoadState != MLS_LOADED) return;

	int x = 0, y = 0;
	int dx = 0, dy = 0;

	switch(nID)
	{
	case ID_VIEW_RESET: m_ZoomX = m_ZoomY = 1.0; m_PosX = m_PosY = 0.5; break;
	case ID_VIEW_INCSIZE: x = y = 1; break;
	case ID_VIEW_DECSIZE: x = y = -1; break;
	case ID_VIEW_INCWIDTH: x = 1; break;
	case ID_VIEW_DECWIDTH: x = -1; break;
	case ID_VIEW_INCHEIGHT: y = 1; break;
	case ID_VIEW_DECHEIGHT: y = -1; break;
	case ID_PANSCAN_CENTER: m_PosX = m_PosY = 0.5; break;
	case ID_PANSCAN_MOVELEFT: dx = -1; break;
	case ID_PANSCAN_MOVERIGHT: dx = 1; break;
	case ID_PANSCAN_MOVEUP: dy = -1; break;
	case ID_PANSCAN_MOVEDOWN: dy = 1; break;
	case ID_PANSCAN_MOVEUPLEFT: dx = dy = -1; break;
	case ID_PANSCAN_MOVEUPRIGHT: dx = 1; dy = -1; break;
	case ID_PANSCAN_MOVEDOWNLEFT: dx = -1; dy = 1; break;
	case ID_PANSCAN_MOVEDOWNRIGHT: dx = dy = 1; break;
	default: break;
	}

	if(x > 0 && m_ZoomX < 3) m_ZoomX *= 1.02;
	if(x < 0 && m_ZoomX > 0.2) m_ZoomX /= 1.02;
	if(y > 0 && m_ZoomY < 3) m_ZoomY *= 1.02;
	if(y < 0 && m_ZoomY > 0.2) m_ZoomY /= 1.02;

	if(dx < 0 && m_PosX > 0) m_PosX = max(m_PosX - 0.005*m_ZoomX, 0);
	if(dx > 0 && m_PosX < 1) m_PosX = min(m_PosX + 0.005*m_ZoomX, 1);
	if(dy < 0 && m_PosY > 0) m_PosY = max(m_PosY - 0.005*m_ZoomY, 0);
	if(dy > 0 && m_PosY < 1) m_PosY = min(m_PosY + 0.005*m_ZoomY, 1);

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScan(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
}

void CMainFrame::OnViewPanNScanPresets(UINT nID)
{
	if(m_iMediaLoadState != MLS_LOADED) return;

	AppSettings& s = AfxGetAppSettings();

	nID -= ID_PANNSCAN_PRESETS_START;

	if(nID == s.m_pnspresets.GetCount())
	{
		CPnSPresetsDlg dlg;
		dlg.m_pnspresets.Copy(s.m_pnspresets);
		if(dlg.DoModal() == IDOK)
		{
			s.m_pnspresets.Copy(dlg.m_pnspresets);
			s.UpdateData(true);
		}
		return;
	}

	m_PosX = 0.5;
	m_PosY = 0.5;
	m_ZoomX = 1.0;
	m_ZoomY = 1.0;
	
	CString str = s.m_pnspresets[nID];

	int i = 0, j = 0;
	for(CString token = str.Tokenize(_T(","), i); !token.IsEmpty(); token = str.Tokenize(_T(","), i), j++)
	{
		float f = 0;
		if(_stscanf(token, _T("%f"), &f) != 1) continue;

		switch(j)
		{
		case 0: break;
		case 1: m_PosX = f; break;
		case 2: m_PosY = f; break;
		case 3: m_ZoomX = f; break;
		case 4: m_ZoomY = f; break;
		default: break;
		}
	}

	if(j != 5) return;

	m_PosX = min(max(m_PosX, 0), 1);
	m_PosY = min(max(m_PosY, 0), 1);
	m_ZoomX = min(max(m_ZoomX, 0.2), 3);
	m_ZoomY = min(max(m_ZoomY, 0.2), 3);

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI)
{
	int nID = pCmdUI->m_nID - ID_PANNSCAN_PRESETS_START;
	AppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && nID >= 0 && nID <= s.m_pnspresets.GetCount());
}

void CMainFrame::OnViewAlwaysontop()
{
	SetAlwaysOnTop(!AfxGetAppSettings().fAlwaysOnTop);
}

void CMainFrame::OnUpdateViewAlwaysontop(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(AfxGetAppSettings().fAlwaysOnTop);
}

void CMainFrame::OnViewOptions()
{
	ShowOptions();
}

// play

void CMainFrame::OnPlayPlay()
{
	if(m_iMediaLoadState == MLS_LOADED)
	{
		if(GetMediaState() == State_Stopped) m_iSpeedLevel = 0;

		if(m_iPlaybackMode == PM_FILE)
		{
			if(m_fEndOfStream) SendMessage(WM_COMMAND, ID_PLAY_STOP);
			pMC->Run();
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			double dRate = 1.0;
			if(m_iSpeedLevel != -4 && m_iSpeedLevel != 0)
				dRate = pow(2.0, m_iSpeedLevel >= -3 ? m_iSpeedLevel : (-m_iSpeedLevel - 8));

			pDVDC->PlayForwards(dRate, DVD_CMD_FLAG_Block, NULL);
			pDVDC->Pause(FALSE);
			pMC->Run();
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{			
			pMC->Stop(); // audio preview won't be in sync if we run it from paused state
			pMC->Run();
		}

		SetTimer(TIMER_STREAMPOSPOLLER, 40, NULL);
		SetTimer(TIMER_STREAMPOSPOLLER2, 500, NULL);
		SetTimer(TIMER_STATS, 1000, NULL);

		if(m_fFrameSteppingActive) // FIXME
		{
			m_fFrameSteppingActive = false;
			pBA->put_Volume(m_VolumeBeforeFrameStepping);
		}
	}

	MoveVideoWindow();
}

void CMainFrame::OnPlayPause()
{
	if(m_iMediaLoadState == MLS_LOADED)
	{
		if(m_iPlaybackMode == PM_FILE)
		{
			pMC->Pause();
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			pMC->Pause();
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{
			pMC->Pause();
		} 

		SetTimer(TIMER_STREAMPOSPOLLER, 40, NULL);
		SetTimer(TIMER_STREAMPOSPOLLER2, 500, NULL);
		SetTimer(TIMER_STATS, 1000, NULL);
	}

	MoveVideoWindow();
}

void CMainFrame::OnPlayPlaypause()
{
	OAFilterState fs = GetMediaState();
	if(fs == State_Running) SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	else if(fs == State_Stopped || fs == State_Paused) SendMessage(WM_COMMAND, ID_PLAY_PLAY);
}

void CMainFrame::OnPlayStop()
{
	if(m_iMediaLoadState == MLS_LOADED)
	{
		if(m_iPlaybackMode == PM_FILE)
		{
			LONGLONG pos = 0;
			pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			pMC->Stop();

			// BUG: after pause or stop the netshow url source filter won't continue 
			// on the next play command, unless we cheat it by setting the file name again.
			// 
			// Note: WMPx may be using some undocumented interface to restart streaming.

			BeginEnumFilters(pGB, pEF, pBF)
			{
				CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF;
				CComQIPtr<IFileSourceFilter> pFSF = pBF;
				if(pAMNS && pFSF)
				{
					WCHAR* pFN = NULL;
					AM_MEDIA_TYPE mt;
					if(SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN)
					{
						pFSF->Load(pFN, NULL);
						CoTaskMemFree(pFN);
					}
					break;
				}
			}
			EndEnumFilters
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			pDVDC->SetOption(DVD_ResetOnStop, TRUE);
			pMC->Stop();
			pDVDC->SetOption(DVD_ResetOnStop, FALSE);
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{
			pMC->Stop();
		}

		m_iSpeedLevel = 0;

		if(m_fFrameSteppingActive) // FIXME
		{
			m_fFrameSteppingActive = false;
			pBA->put_Volume(m_VolumeBeforeFrameStepping);
		}

		m_fEndOfStream = false;
	}

	m_nLoops = 0;

	if(m_hWnd) 
	{
		KillTimer(TIMER_STREAMPOSPOLLER2);
		KillTimer(TIMER_STREAMPOSPOLLER);
		KillTimer(TIMER_STATS);
		MoveVideoWindow();

		if(m_iMediaLoadState == MLS_LOADED)
		{
			__int64 start, stop;
			m_wndSeekBar.GetRange(start, stop);
			GUID tf;
			pMS->GetTimeFormat(&tf);
			m_wndStatusBar.SetStatusTimer(m_wndSeekBar.GetPosReal(), stop, !!m_wndSubresyncBar.IsWindowVisible(), &tf);
		}
	}
}

void CMainFrame::OnUpdatePlayPauseStop(CCmdUI* pCmdUI)
{
	OAFilterState fs = m_fFrameSteppingActive ? State_Paused : GetMediaState();

	pCmdUI->SetCheck(fs == State_Running && pCmdUI->m_nID == ID_PLAY_PLAY
		|| fs == State_Paused && pCmdUI->m_nID == ID_PLAY_PAUSE
		|| fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_STOP
		|| (fs == State_Paused || fs == State_Running) && pCmdUI->m_nID == ID_PLAY_PLAYPAUSE);

	bool fEnable = false;

	if(fs >= 0)
	{
		if(m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_CAPTURE)
		{
			fEnable = true;

			if(fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_PAUSE && m_fRealMediaGraph) fEnable = false; // can't go into paused state from stopped with rm
			else if(m_fCapturing) fEnable = false;
			else if(m_fLiveWM && pCmdUI->m_nID == ID_PLAY_PAUSE) fEnable = false;
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			fEnable = m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu 
				&& m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu;

			if(fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_PAUSE) fEnable = false;
		}
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayFramestep(UINT nID)
{
	REFERENCE_TIME rt;

	if(pFS && m_fQuicktimeGraph)
	{
		if(GetMediaState() != State_Paused)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		pFS->Step(nID == ID_PLAY_FRAMESTEP ? 1 : -1, NULL);
	}
	else if(pFS && nID == ID_PLAY_FRAMESTEP)
	{
		if(GetMediaState() != State_Paused)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		m_fFrameSteppingActive = true;

		m_VolumeBeforeFrameStepping = m_wndToolBar.Volume;
		pBA->put_Volume(-10000);

		pFS->Step(1, NULL);
	}
	else if(S_OK == pMS->IsFormatSupported(&TIME_FORMAT_FRAME))
	{
		if(GetMediaState() != State_Paused)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		pMS->SetTimeFormat(&TIME_FORMAT_FRAME);
		pMS->GetCurrentPosition(&rt);
		if(nID == ID_PLAY_FRAMESTEP) rt++;
		else if(nID == ID_PLAY_FRAMESTEPCANCEL) rt--;
		pMS->SetPositions(&rt, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
	}
}

void CMainFrame::OnUpdatePlayFramestep(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly
	&& (m_iPlaybackMode != PM_DVD || m_iDVDDomain == DVD_DOMAIN_Title)
	&& m_iPlaybackMode != PM_CAPTURE
	&& !m_fLiveWM)
	{
		REFTIME AvgTimePerFrame = 0;
        if(S_OK == pMS->IsFormatSupported(&TIME_FORMAT_FRAME)
		|| pCmdUI->m_nID == ID_PLAY_FRAMESTEP && pFS && pFS->CanStep(0, NULL) == S_OK
		|| m_fQuicktimeGraph && pFS)
		{
			fEnable = true;
		}
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlaySeek(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();
	
	REFERENCE_TIME dt = 
		nID == ID_PLAY_SEEKBACKWARDSMALL ? -10000i64*s.nJumpDistS : 
		nID == ID_PLAY_SEEKFORWARDSMALL ? +10000i64*s.nJumpDistS : 
		nID == ID_PLAY_SEEKBACKWARDMED ? -10000i64*s.nJumpDistM : 
		nID == ID_PLAY_SEEKFORWARDMED ? +10000i64*s.nJumpDistM : 
		nID == ID_PLAY_SEEKBACKWARDLARGE ? -10000i64*s.nJumpDistL : 
		nID == ID_PLAY_SEEKFORWARDLARGE ? +10000i64*s.nJumpDistL : 
		0;

	if(!dt) return;

	// HACK: the custom graph should support frame based seeking instead
	if(m_fShockwaveGraph) dt /= 10000i64*100;

	SeekTo(m_wndSeekBar.GetPos() + dt);
}

static int rangebsearch(REFERENCE_TIME val, CArray<REFERENCE_TIME>& rta)
{
	int i = 0, j = rta.GetSize() - 1, ret = -1;

	if(j >= 0 && val >= rta[j]) return(j);

	while(i < j)
	{
		int mid = (i + j) >> 1;
		REFERENCE_TIME midt = rta[mid];
		if(val == midt) {ret = mid; break;}
		else if(val < midt) {ret = -1; if(j == mid) mid--; j = mid;}
		else if(val > midt) {ret = mid; if(i == mid) mid++; i = mid;}
	}

	return(ret);
}

void CMainFrame::OnPlaySeekKey(UINT nID)
{
	if(m_kfs.GetCount() > 0)
	{
		HRESULT hr;

		if(GetMediaState() == State_Stopped)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		REFERENCE_TIME rtCurrent, rtDur;
		hr = pMS->GetCurrentPosition(&rtCurrent);
		hr = pMS->GetDuration(&rtDur);

		int dec = 1;
		int i = rangebsearch(rtCurrent, m_kfs);
		if(i > 0) dec = (UINT)max(min(rtCurrent - m_kfs[i-1], 10000000), 0);

		rtCurrent = 
			nID == ID_PLAY_SEEKKEYBACKWARD ? max(rtCurrent - dec, 0) : 
			nID == ID_PLAY_SEEKKEYFORWARD ? rtCurrent : 0;

		i = rangebsearch(rtCurrent, m_kfs);

		if(nID == ID_PLAY_SEEKKEYBACKWARD)
			rtCurrent = m_kfs[max(i, 0)];
		else if(nID == ID_PLAY_SEEKKEYFORWARD && i < m_kfs.GetCount()-1)
			rtCurrent = m_kfs[i+1];
		else
			return;

		hr = pMS->SetPositions(
			&rtCurrent, AM_SEEKING_AbsolutePositioning|AM_SEEKING_SeekToKeyFrame, 
			NULL, AM_SEEKING_NoPositioning);
	}
}

void CMainFrame::OnUpdatePlaySeek(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	OAFilterState fs = GetMediaState();

	if(m_iMediaLoadState == MLS_LOADED && (fs == State_Paused || fs == State_Running))
	{
		fEnable = true;
		if(m_iPlaybackMode == PM_DVD && (m_iDVDDomain != DVD_DOMAIN_Title || fs != State_Running)) fEnable = false;
		else if(m_iPlaybackMode == PM_CAPTURE) fEnable = false;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayGoto()
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	REFTIME atpf = 0;
	if(FAILED(pBV->get_AvgTimePerFrame(&atpf)) || atpf < 0)
	{
		atpf = 0;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(atpf > 0) break;

			BeginEnumPins(pBF, pEP, pPin)
			{
				if(atpf > 0) break;

				AM_MEDIA_TYPE mt;
				pPin->ConnectionMediaType(&mt);

				if(mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo)
				{
					atpf = (REFTIME)((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
				}
				else if(mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo2)
				{
					atpf = (REFTIME)((VIDEOINFOHEADER2*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
				}
			}
			EndEnumPins
		}
		EndEnumFilters
	}

	CGoToDlg dlg((int)(m_wndSeekBar.GetPos()/10000), atpf > 0 ? (float)(1.0/atpf) : 0);
	if(IDOK != dlg.DoModal() || dlg.m_time < 0) return;

	SeekTo(10000i64 * dlg.m_time);
}

void CMainFrame::OnUpdateGoto(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if(m_iMediaLoadState == MLS_LOADED)
	{
		fEnable = true;
		if(m_iPlaybackMode == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) fEnable = false;
		else if(m_iPlaybackMode == PM_CAPTURE) fEnable = false;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayChangeRate(UINT nID)
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	int iNewSpeedLevel;

	if(nID == ID_PLAY_INCRATE) iNewSpeedLevel = m_iSpeedLevel+1;
	else if(nID == ID_PLAY_DECRATE) iNewSpeedLevel = m_iSpeedLevel-1;
	else return;

	HRESULT hr = E_FAIL;

	if(iNewSpeedLevel == -4)
	{
		if(GetMediaState() != State_Paused)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		if(GetMediaState() == State_Paused) hr = S_OK;
	}
	else
	{
		double dRate = pow(2.0, iNewSpeedLevel >= -3 ? iNewSpeedLevel : (-iNewSpeedLevel - 8));
		if(fabs(dRate - 1.0) < 0.01) dRate = 1.0;

		if(GetMediaState() != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		if(m_iPlaybackMode == PM_FILE)
		{
			hr = pMS->SetRate(dRate);
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			if(iNewSpeedLevel >= -3)
				hr = pDVDC->PlayForwards(dRate, DVD_CMD_FLAG_Block, NULL);
			else
				hr = pDVDC->PlayBackwards(dRate, DVD_CMD_FLAG_Block, NULL);
		}
	}

	if(SUCCEEDED(hr))
		m_iSpeedLevel = iNewSpeedLevel;
}

void CMainFrame::OnUpdatePlayChangeRate(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if(m_iMediaLoadState == MLS_LOADED)
	{
		bool fInc = pCmdUI->m_nID == ID_PLAY_INCRATE;

		fEnable = true;
		if(fInc && m_iSpeedLevel >= 3) fEnable = false;
		else if(!fInc && m_iPlaybackMode == PM_FILE && m_iSpeedLevel <= -4) fEnable = false;
		else if(!fInc && m_iPlaybackMode == PM_DVD && m_iSpeedLevel <= -11) fEnable = false;
		else if(m_iPlaybackMode == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) fEnable = false;
		else if(m_fRealMediaGraph || m_fShockwaveGraph) fEnable = false;
		else if(m_iPlaybackMode == PM_CAPTURE) fEnable = false;
		else if(m_fLiveWM) fEnable = false;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayChangeAudDelay(UINT nID)
{
	if(CComQIPtr<IAudioSwitcherFilter> pAS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB))
	{
		REFERENCE_TIME rtShift = pAS->GetAudioTimeShift();
		rtShift += 
			nID == ID_PLAY_INCAUDDELAY ? 100000 :
			nID == ID_PLAY_DECAUDDELAY ? -100000 : 
			0;
		pAS->SetAudioTimeShift(rtShift);

		CString str;
		str.Format(_T("Audio Delay: %I64dms"), rtShift/10000);
		SendStatusMessage(str, 3000);
	}
}

void CMainFrame::OnUpdatePlayChangeAudDelay(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(pGB && !!FindFilter(__uuidof(CAudioSwitcherFilter), pGB));
}

#include "ComPropertySheet.h"

void CMainFrame::OnPlayFilters(UINT nID)
{
//	ShowPPage(m_spparray[nID - ID_FILTERS_SUBITEM_START], m_hWnd);

	CComPtr<ISpecifyPropertyPages> pSPP = m_spparray[nID - ID_FILTERS_SUBITEM_START];

	CComPropertySheet ps(_T("Properties"), this);
	ps.AddPages(pSPP);
	ps.DoModal();

	OpenSetupStatusBar();
}

void CMainFrame::OnUpdatePlayFilters(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_fCapturing);
}

void CMainFrame::OnPlayAudio(UINT nID)
{
	int i = (int)nID - (1 + ID_AUDIO_SUBITEM_START);

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

	if(i == -1)
	{
		ShowOptions(CPPageAudioSwitcher::IDD);
	}
	else if(i >= 0 && pSS)
	{
		pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnUpdatePlayAudio(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (1 + ID_AUDIO_SUBITEM_START);

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

	/*if(i == -1)
	{
		// TODO****
	}
	else*/ if(i >= 0 && pSS)
	{
		DWORD flags = 0;

		if(SUCCEEDED(pSS->Info(i, NULL, &flags, NULL, NULL, NULL, NULL, NULL)))
		{
			if(flags&AMSTREAMSELECTINFO_EXCLUSIVE) pCmdUI->SetRadio(TRUE);
			else if(flags&AMSTREAMSELECTINFO_ENABLED) pCmdUI->SetCheck(TRUE);	
			else pCmdUI->SetCheck(FALSE);
		}
		else
		{
			pCmdUI->Enable(FALSE);
		}
	}
}

void CMainFrame::OnPlaySubtitles(UINT nID)
{
	int i = (int)nID - (3 + ID_SUBTITLES_SUBITEM_START);

	if(i == -3)
	{
		ShowOptions(CPPageSubtitles::IDD);
	}
	else if(i == -2)
	{
		int i = m_iSubtitleSel;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(pos && i >= 0)
		{
			CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

			if(i < pSubStream->GetStreamCount())
			{
				CLSID clsid;
				if(FAILED(pSubStream->GetClassID(&clsid)))
					continue;

				if(clsid == __uuidof(CRenderedTextSubtitle))
				{
					CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

					CAutoPtrArray<CPPageSubStyle> pages;
					CArray<STSStyle*> styles;

					POSITION pos = pRTS->m_styles.GetStartPosition();
					for(int i = 0; pos; i++)
					{
						CString key;
						void* val;
						pRTS->m_styles.GetNextAssoc(pos, key, val);

						CAutoPtr<CPPageSubStyle> page(new CPPageSubStyle());
						page->InitStyle(key, *(STSStyle*)val);
						pages.Add(page);
						styles.Add((STSStyle*)val);
					}

					CPropertySheet dlg(_T("Styles..."), this);
					for(int i = 0; i < (int)pages.GetCount(); i++) 
						dlg.AddPage(pages[i]);

					if(dlg.DoModal() == IDOK)
					{
						for(int j = 0; j < (int)pages.GetCount(); j++)
							pages[j]->GetStyle(*styles[j]);
						UpdateSubtitle(false);
					}

					return;
				}
			}

			i -= pSubStream->GetStreamCount();
		}
	}
	else if(i == -1)
	{
		m_iSubtitleSel ^= (1<<31);
		UpdateSubtitle();
	}
	else if(i >= 0)
	{
		m_iSubtitleSel = i;
		UpdateSubtitle();
	}
}

void CMainFrame::OnUpdatePlaySubtitles(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (3 + ID_SUBTITLES_SUBITEM_START);

	pCmdUI->Enable(m_pCAP && !m_fAudioOnly);

	if(i == -2)
	{
		pCmdUI->Enable(FALSE);

		int i = m_iSubtitleSel;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(pos && i >= 0)
		{
			CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

			if(i < pSubStream->GetStreamCount())
			{
				CLSID clsid;
				if(FAILED(pSubStream->GetClassID(&clsid)))
					continue;

				if(clsid == __uuidof(CRenderedTextSubtitle))
				{
					pCmdUI->Enable(TRUE);
					break;
				}
			}

			i -= pSubStream->GetStreamCount();
		}
	}
	else if(i == -1)
	{
		pCmdUI->SetCheck(m_iSubtitleSel >= 0);
	}
	else if(i >= 0)
	{
		pCmdUI->SetRadio(i == abs(m_iSubtitleSel));
	}
}

void CMainFrame::OnPlayLanguage(UINT nID)
{
	nID -= ID_FILTERSTREAMS_SUBITEM_START;
	CComPtr<IAMStreamSelect> pAMSS = m_ssarray[nID];
	UINT i = nID;
	while(i > 0 && pAMSS == m_ssarray[i-1]) i--;
	pAMSS->Enable(nID-i, AMSTREAMSELECTENABLE_ENABLE);

	OpenSetupStatusBar();
}

void CMainFrame::OnUpdatePlayLanguage(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	nID -= ID_FILTERSTREAMS_SUBITEM_START;
	CComPtr<IAMStreamSelect> pAMSS = m_ssarray[nID];
	UINT i = nID;
	while(i > 0 && pAMSS == m_ssarray[i-1]) i--;
	DWORD flags = 0;
	pAMSS->Info(nID-i, NULL, &flags, NULL, NULL, NULL, NULL, NULL);
	if(flags&AMSTREAMSELECTINFO_EXCLUSIVE) pCmdUI->SetRadio(TRUE);
	else if(flags&AMSTREAMSELECTINFO_ENABLED) pCmdUI->SetCheck(TRUE);	
	else pCmdUI->SetCheck(FALSE);
}

void CMainFrame::OnPlayVolume(UINT nID)
{
	if(m_iMediaLoadState == MLS_LOADED) 
		pBA->put_Volume(m_wndToolBar.Volume);
}

// navigate

void CMainFrame::OnNavigateSkip(UINT nID)
{
	if(m_iPlaybackMode == PM_FILE)
	{
		if(m_chapters.GetCount())
		{
			REFERENCE_TIME rtNow, rtDur;
			pMS->GetCurrentPosition(&rtNow);
			pMS->GetDuration(&rtDur);

			if(nID == ID_NAVIGATE_SKIPBACK)
			{
				for(int i = m_chapters.GetCount()-1; i >= 0; i--)
				{
					if(rtNow >= m_chapters[i].rtStart)
					{
						if(rtNow < m_chapters[i].rtStart + 10000000 && i > 0) i--;
						SeekTo(m_chapters[i].rtStart);
						SendStatusMessage(_T("Chapter: ") + m_chapters[i].name, 3000);
						return;
					}
				}
			}
			else if(nID == ID_NAVIGATE_SKIPFORWARD)
			{
				for(int i = 0; i < m_chapters.GetCount(); i++)
				{
					if(rtNow < m_chapters[i].rtStart)
					{
						SeekTo(m_chapters[i].rtStart);
						SendStatusMessage(_T("Chapter: ") + m_chapters[i].name, 3000);
						return;
					}
				}
			}
		}

		if(nID == ID_NAVIGATE_SKIPBACK)
		{
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACKPLITEM);
		}
		else if(nID == ID_NAVIGATE_SKIPFORWARD)
		{
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARDPLITEM);
		}
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		m_iSpeedLevel = 0;

		if(GetMediaState() != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		if(nID == ID_NAVIGATE_SKIPBACK)
		{
			if(Location.ChapterNum == 1 && Location.TitleNum > 1)
			{
				pDVDI->GetNumberOfChapters(Location.TitleNum-1, &ulNumOfChapters);
				pDVDC->PlayChapterInTitle(Location.TitleNum-1, ulNumOfChapters, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
			else
			{
				pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
		}
		else if(nID == ID_NAVIGATE_SKIPFORWARD)
		{
			if(Location.ChapterNum == ulNumOfChapters && Location.TitleNum < ulNumOfTitles)
			{
				pDVDC->PlayChapterInTitle(Location.TitleNum+1, 1, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
			else
			{
				pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
		}
/*
        if(nID == ID_NAVIGATE_SKIPBACK)
			pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block, NULL);
		else if(nID == ID_NAVIGATE_SKIPFORWARD)
			pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block, NULL);
*/
	}
	else if(m_iPlaybackMode == PM_CAPTURE) // TODO
	{
		if(GetMediaState() != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		long lChannelMin = 0, lChannelMax = 0;
		pAMTuner->ChannelMinMax(&lChannelMin, &lChannelMax);
		long lChannel = 0, lVivSub = 0, lAudSub = 0;
		pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);

		long lFreqOrg = 0, lFreqNew = -1;
		pAMTuner->get_VideoFrequency(&lFreqOrg);

//		long lSignalStrength;
		do
		{
			if(nID == ID_NAVIGATE_SKIPBACK) lChannel--;
			else if(nID == ID_NAVIGATE_SKIPFORWARD) lChannel++;

//			if(lChannel < lChannelMin) lChannel = lChannelMax;
//			if(lChannel > lChannelMax) lChannel = lChannelMin;

			if(lChannel < lChannelMin || lChannel > lChannelMax) 
				break;

			if(FAILED(pAMTuner->put_Channel(lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT)))
				break;
		
			long flFoundSignal;
			pAMTuner->AutoTune(lChannel, &flFoundSignal);

			pAMTuner->get_VideoFrequency(&lFreqNew);

			int i = 0;
		}
		while(FALSE);
/*			SUCCEEDED(pAMTuner->SignalPresent(&lSignalStrength)) 
			&& (lSignalStrength != AMTUNER_SIGNALPRESENT || lFreqNew == lFreqOrg));*/
	}
}

void CMainFrame::OnUpdateNavigateSkip(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
		&& ((m_iPlaybackMode == PM_DVD 
				&& m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu 
				&& m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu)
			|| (m_iPlaybackMode == PM_FILE && (m_wndPlaylistBar.GetCount() > 1/*0*/ || m_chapters.GetCount() > 1))
			|| (m_iPlaybackMode == PM_CAPTURE && pAMTuner && !m_fCapturing))); // TODO
}

void CMainFrame::OnNavigateSkipPlaylistItem(UINT nID)
{
	if(m_iPlaybackMode == PM_FILE)
	{
		if(m_wndPlaylistBar.GetCount() == 1)
		{
			SendMessage(WM_COMMAND, ID_PLAY_STOP); // do not remove this, unless you want a circular call with OnPlayPlay()
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);
		}
		else
		{
			if(nID == ID_NAVIGATE_SKIPBACKPLITEM)
			{
				m_wndPlaylistBar.SetPrev();
			}
			else if(nID == ID_NAVIGATE_SKIPFORWARDPLITEM)
			{
				m_wndPlaylistBar.SetNext();
			}

			OpenCurPlaylistItem();
		}
	}
}

void CMainFrame::OnUpdateNavigateSkipPlaylistItem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
		&& (m_iPlaybackMode == PM_FILE && m_wndPlaylistBar.GetCount() > 1/*0*/));
}

void CMainFrame::OnNavigateMenu(UINT nID)
{
	nID -= ID_NAVIGATE_TITLEMENU;

	if(m_iMediaLoadState != MLS_LOADED || m_iPlaybackMode != PM_DVD)
		return;

	m_iSpeedLevel = 0;

	if(GetMediaState() != State_Running)
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);

	pDVDC->ShowMenu((DVD_MENU_ID)(nID+2), DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
}

void CMainFrame::OnUpdateNavigateMenu(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_NAVIGATE_TITLEMENU;
	ULONG ulUOPs;

	if(m_iMediaLoadState != MLS_LOADED || m_iPlaybackMode != PM_DVD 
		|| FAILED(pDVDI->GetCurrentUOPS(&ulUOPs)))
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(!(ulUOPs & (UOP_FLAG_ShowMenu_Title << nID)));
}

void CMainFrame::OnNavigateAudio(UINT nID)
{
	nID -= ID_NAVIGATE_AUDIO_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		OnNavOgmSubMenu(nID, _T("sound"));
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		pDVDC->SelectAudioStream(nID, DVD_CMD_FLAG_Block, NULL);
	}
}

void CMainFrame::OnNavigateSubpic(UINT nID)
{
	if(m_iPlaybackMode == PM_FILE)
	{
		OnNavOgmSubMenu(nID - ID_NAVIGATE_SUBP_SUBITEM_START, _T("subtitle"));
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		int i = (int)nID - (1 + ID_NAVIGATE_SUBP_SUBITEM_START);

		if(i == -1)
		{
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled)))
				pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, NULL);
		}
		else
		{
			pDVDC->SelectSubpictureStream(i, DVD_CMD_FLAG_Block, NULL);
			pDVDC->SetSubpictureState(TRUE, DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnNavigateAngle(UINT nID)
{
	nID -= ID_NAVIGATE_ANGLE_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		OnNavOgmSubMenu(nID, _T("video"));
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		pDVDC->SelectAngle(nID, DVD_CMD_FLAG_Block, NULL);
	}
}

void CMainFrame::OnNavigateChapters(UINT nID)
{
	nID -= ID_NAVIGATE_CHAP_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		if((int)nID >= 0 && (int)nID < m_chapters.GetCount())
		{
			SeekTo(m_chapters[nID].rtStart);
			SendStatusMessage(_T("Chapter: ") + m_chapters[nID].name, 3000);
			return;
		}
		nID -= m_chapters.GetCount();

		if((int)nID >= 0 && (int)nID < m_wndPlaylistBar.GetCount()
		&& m_wndPlaylistBar.GetSelIdx() != (int)nID)
		{
			m_wndPlaylistBar.SetSelIdx(nID);
			OpenCurPlaylistItem();
		}
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		nID++;

		if(nID > 0 && nID <= ulNumOfTitles)
		{
			pDVDC->PlayTitle(nID, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); // sometimes this does not do anything ... 
			pDVDC->PlayChapterInTitle(nID, 1, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); // ... but this does!
			return;
		}

		nID -= ulNumOfTitles;

		if(nID > 0 && nID <= ulNumOfChapters)
		{
			pDVDC->PlayChapter(nID, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			return;
		}
	}
}

void CMainFrame::OnNavigateMenuItem(UINT nID)
{
	nID -= ID_NAVIGATE_MENU_LEFT;

	switch(nID)
	{
	case 0: pDVDC->SelectRelativeButton(DVD_Relative_Left); break;
	case 1: pDVDC->SelectRelativeButton(DVD_Relative_Right); break;
	case 2: pDVDC->SelectRelativeButton(DVD_Relative_Upper); break;
	case 3: pDVDC->SelectRelativeButton(DVD_Relative_Lower); break;
	case 4: pDVDC->ActivateButton(); break;
	case 5: pDVDC->ReturnFromSubmenu(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); break;
	case 6: pDVDC->Resume(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); break;
	default: break;
	}

}

void CMainFrame::OnUpdateNavigateMenuItem(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_NAVIGATE_MENU_LEFT;
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_DVD);
}

// favorites

class CDVDStateStream : public CUnknown, public IStream
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return 
			QI(IStream)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	__int64 m_pos;

public:
	CDVDStateStream() : CUnknown(NAME("CDVDStateStream"), NULL) {m_pos = 0;}

	DECLARE_IUNKNOWN;

	CByteArray m_data;

    // ISequentialStream
	STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead)
	{
		__int64 cbRead = min(m_data.GetCount() - m_pos, (__int64)cb);
		cbRead = max(cbRead, 0);
		memcpy(pv, &m_data[(INT_PTR)m_pos], (int)cbRead);
		if(pcbRead) *pcbRead = (ULONG)cbRead;
		m_pos += cbRead;
		return S_OK;
	}
	STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten)
	{
		BYTE* p = (BYTE*)pv;
		ULONG cbWritten = -1;
		while(++cbWritten < cb)
			m_data.Add(*p++);
		if(pcbWritten) *pcbWritten = cbWritten;
		return S_OK;
	}

	// IStream
	STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) {return E_NOTIMPL;}
	STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) {return E_NOTIMPL;}
	STDMETHODIMP CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {return E_NOTIMPL;}
	STDMETHODIMP Commit(DWORD grfCommitFlags) {return E_NOTIMPL;}
	STDMETHODIMP Revert() {return E_NOTIMPL;}
	STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {return E_NOTIMPL;}
	STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {return E_NOTIMPL;}
	STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) {return E_NOTIMPL;}
	STDMETHODIMP Clone(IStream** ppstm) {return E_NOTIMPL;}
};


void CMainFrame::OnFavoritesAdd()
{
	AppSettings& s = AfxGetAppSettings();

	if(m_iPlaybackMode == PM_FILE)
	{
		CString fn =  m_wndPlaylistBar.GetCur();
		if(fn.IsEmpty()) return;

		CString desc = fn;
		desc.Replace('\\', '/');
		int i = desc.Find(_T("://")), j = desc.Find(_T("?")), k = desc.ReverseFind('/');
		if(i >= 0) desc = j >= 0 ? desc.Left(j) : desc;
		else if(k >= 0) desc = desc.Mid(k+1);

		CFavoriteAddDlg dlg(desc, fn);
		if(dlg.DoModal() != IDOK) return;

		CString str = dlg.m_name;
		str.Remove(';');

		CString pos(_T("0"));
		if(dlg.m_fRememberPos)
			pos.Format(_T("%I64d"), GetPos());

		str += ';';
		str += pos;

		CPlaylistItem pli;
		if(m_wndPlaylistBar.GetCur(pli))
		{
			POSITION pos = pli.m_fns.GetHeadPosition();
			while(pos) str += _T(";") + pli.m_fns.GetNext(pos);
		}

		s.AddFav(FAV_FILE, str);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		WCHAR path[MAX_PATH];
		ULONG len = 0;
		pDVDI->GetDVDDirectory(path, MAX_PATH, &len);
		CString fn = path;
		fn.TrimRight(_T("/\\"));

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);
		CString desc;
		desc.Format(_T("%s - T%02d C%02d - %02d:%02d:%02d"), fn, Location.TitleNum, Location.ChapterNum, 
			Location.TimeCode.bHours, Location.TimeCode.bMinutes, Location.TimeCode.bSeconds);

		CFavoriteAddDlg dlg(fn, desc);
		if(dlg.DoModal() != IDOK) return;

		CString str = dlg.m_name;
		str.Remove(';');

		CString pos(_T("0"));
		if(dlg.m_fRememberPos)
		{
			CDVDStateStream stream;
			stream.AddRef();

			CComPtr<IDvdState> pStateData;
			CComQIPtr<IPersistStream> pPersistStream;
			if(SUCCEEDED(pDVDI->GetState(&pStateData))
			&& (pPersistStream = pStateData)
			&& SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream)))
			{
				pos = BinToString(stream.m_data.GetData(), stream.m_data.GetCount());
			}
		}

		str += ';';
		str += pos;

		str += ';';
		str += fn;

		s.AddFav(FAV_DVD, str);
	}
	else if(m_iPlaybackMode == PM_CAPTURE)
	{
		// TODO
	}
}

void CMainFrame::OnUpdateFavoritesAdd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_DVD);
}

void CMainFrame::OnFavoritesOrganize()
{
	CFavoriteOrganizeDlg dlg;
	dlg.DoModal();
}

void CMainFrame::OnUpdateFavoritesOrganize(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
}

void CMainFrame::OnFavoritesFile(UINT nID)
{
	nID -= ID_FAVORITES_FILE_START;

	CStringList sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	if(POSITION pos = sl.FindIndex(nID))
	{
		CStringList fns;
		REFERENCE_TIME rtStart = 0;

		int i = 0, j = 0;
		for(CString s1 = sl.GetAt(pos), s2 = s1.Tokenize(_T(";"), i); 
			!s2.IsEmpty(); 
			s2 = s1.Tokenize(_T(";"), i), j++)
		{
			if(j == 0) ; // desc
			else if(j == 1) _stscanf(s2, _T("%I64d"), &rtStart); // pos
			else fns.AddTail(s2); // subs
		}

		m_wndPlaylistBar.Open(fns, false);
		OpenCurPlaylistItem(max(rtStart, 0));
	}
}

void CMainFrame::OnUpdateFavoritesFile(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_FILE_START;
}

void CMainFrame::OnFavoritesDVD(UINT nID)
{
	nID -= ID_FAVORITES_DVD_START;

	CStringList sl;
	AfxGetAppSettings().GetFav(FAV_DVD, sl);

	if(POSITION pos = sl.FindIndex(nID))
	{
		CString fn;

		CDVDStateStream stream;
		stream.AddRef();

		int i = 0, j = 0;
		for(CString s1 = sl.GetAt(pos), s2 = s1.Tokenize(_T(";"), i); 
			!s2.IsEmpty(); 
			s2 = s1.Tokenize(_T(";"), i), j++)
		{
			if(j == 0) ; // desc
			else if(j == 1 && s2 != _T("0")) // state
			{
				StringToBin(s2, stream.m_data);
			}
			else if(j == 2) fn = s2; // path
		}

		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		CComPtr<IDvdState> pDvdState;
		HRESULT hr = OleLoadFromStream((IStream*)&stream, IID_IDvdState, (void**)&pDvdState);

		CAutoPtr<OpenDVDData> p(new OpenDVDData());
		if(p) {p->path = fn; p->pDvdState = pDvdState;}
		OpenMedia(p);
	}
}

void CMainFrame::OnUpdateFavoritesDVD(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_DVD_START;
}

void CMainFrame::OnFavoritesDevice(UINT nID)
{
	nID -= ID_FAVORITES_DEVICE_START;
}

void CMainFrame::OnUpdateFavoritesDevice(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_DEVICE_START;
}

// help

void CMainFrame::OnHelpHomepage()
{
	ShellExecute(m_hWnd, _T("open"), _T("http://gabest.org/"), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnHelpDonate()
{
	const TCHAR URL[] = _T("http://order.kagi.com/?N4A");
	if(CString(URL).Find(CString(_T("A4N")).MakeReverse()) >= 0)
		ShellExecute(m_hWnd, _T("open"), URL, NULL, NULL, SW_SHOWDEFAULT);
}

//////////////////////////////////


void CMainFrame::SetDefaultWindowRect()
{
	AppSettings& s = AfxGetAppSettings();

	{
		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		CSize logosize = m_wndView.GetLogoSize();
		int _DEFCLIENTW = max(logosize.cx, DEFCLIENTW);
		int _DEFCLIENTH = max(logosize.cy, DEFCLIENTH);

		DWORD style = GetStyle();

		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		int w = _DEFCLIENTW + GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2
			+ r1.Width() - r2.Width();
		int h = _DEFCLIENTH + GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2
			+ (mbi.rcBar.bottom - mbi.rcBar.top)
			+ r1.Height() - r2.Height()
			+ 2; // ???
		if(style&WS_CAPTION) h += GetSystemMetrics(SM_CYCAPTION);

		if(s.fRememberWindowSize)
		{
			w = s.rcLastWindowPos.Width();
			h = s.rcLastWindowPos.Height();
		}

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		int x = (mi.rcWork.left+mi.rcWork.right-w)/2;
		int y = (mi.rcWork.top+mi.rcWork.bottom-h)/2;

		if(s.fRememberWindowPos)
		{
			CRect r = s.rcLastWindowPos;
//			x = r.CenterPoint().x - w/2;
//			y = r.CenterPoint().y - h/2;
			x = r.TopLeft().x;
			y = r.TopLeft().y;
		}

		UINT lastWindowType = s.lastWindowType;

		MoveWindow(x, y, w, h);

		WINDOWPLACEMENT wp;
		memset(&wp, 0, sizeof(wp));
		wp.length = sizeof(WINDOWPLACEMENT);
		if(lastWindowType == SIZE_MAXIMIZED)
			ShowWindow(SW_MAXIMIZE);
		else if(lastWindowType == SIZE_MINIMIZED)
			ShowWindow(SW_MAXIMIZE);
	}

	if(s.fHideCaptionMenu)
	{
		ModifyStyle(WS_CAPTION, 0, SWP_NOZORDER);
		::SetMenu(m_hWnd, NULL);
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
	}
}

void CMainFrame::RestoreDefaultWindowRect()
{
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if(!m_fFullScreen && wp.showCmd != SW_SHOWMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED
	&& (GetExStyle()&WS_EX_APPWINDOW)
	&& !AfxGetAppSettings().fRememberWindowSize)
	{
		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		CSize logosize = m_wndView.GetLogoSize();
		int _DEFCLIENTW = max(logosize.cx, DEFCLIENTW);
		int _DEFCLIENTH = max(logosize.cy, DEFCLIENTH);

		DWORD style = GetStyle();

		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		int w = _DEFCLIENTW + GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2
			+ r1.Width() - r2.Width();
		int h = _DEFCLIENTH + GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2
			+ (mbi.rcBar.bottom - mbi.rcBar.top)
			+ r1.Height() - r2.Height()
			+ 2; // ???
		if(style&WS_CAPTION) h += GetSystemMetrics(SM_CYCAPTION);

		CRect r;
		GetWindowRect(r);

		int x = r.CenterPoint().x - w/2;
		int y = r.CenterPoint().y - h/2;

		if(AfxGetAppSettings().fRememberWindowPos)
		{
			CRect r = AfxGetAppSettings().rcLastWindowPos;

			x = r.TopLeft().x;
			y = r.TopLeft().y;
		}

		MoveWindow(x, y, w, h);
	}
}

OAFilterState CMainFrame::GetMediaState()
{
	OAFilterState ret = -1;
	if(m_iMediaLoadState == MLS_LOADED) pMC->GetState(0, &ret);
	return(ret);
}

CSize CMainFrame::GetVideoSize()
{
	bool fKeepAspectRatio = AfxGetAppSettings().fKeepAspectRatio;
	bool fCompMonDeskARDiff = AfxGetAppSettings().fCompMonDeskARDiff;

	CSize ret(0,0);
	if(m_iMediaLoadState != MLS_LOADED || m_fAudioOnly)
		return ret;

	CSize wh(0, 0), arxy(0, 0);

	if(m_pCAP)
	{
		wh = m_pCAP->GetVideoSize(false);
		arxy = m_pCAP->GetVideoSize(fKeepAspectRatio);
	}
	else
	{
		pBV->GetVideoSize(&wh.cx, &wh.cy);

		long arx = 0, ary = 0;
		CComQIPtr<IBasicVideo2> pBV2 = pBV;
		if(pBV2 && SUCCEEDED(pBV2->GetPreferredAspectRatio(&arx, &ary)) && arx > 0 && ary > 0)
			arxy.SetSize(arx, ary);
	}

	if(wh.cx <= 0 || wh.cy <= 0)
		return ret;

	// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
	DVD_VideoAttributes VATR;
	if(m_iPlaybackMode == PM_DVD && SUCCEEDED(pDVDI->GetCurrentVideoAttributes(&VATR)))
		arxy.SetSize(VATR.ulAspectX, VATR.ulAspectY);

	ret = (!fKeepAspectRatio || arxy.cx <= 0 || arxy.cy <= 0)
		? wh
		: CSize(MulDiv(wh.cy, arxy.cx, arxy.cy), wh.cy);

	if(fCompMonDeskARDiff)
	if(HDC hDC = ::GetDC(0))
	{
		int _HORZSIZE = GetDeviceCaps(hDC, HORZSIZE);
		int _VERTSIZE = GetDeviceCaps(hDC, VERTSIZE);
		int _HORZRES = GetDeviceCaps(hDC, HORZRES);
		int _VERTRES = GetDeviceCaps(hDC, VERTRES);

		if(_HORZSIZE > 0 && _VERTSIZE > 0 && _HORZRES > 0 && _VERTRES > 0)
		{
			double a = 1.0*_HORZSIZE/_VERTSIZE;
			double b = 1.0*_HORZRES/_VERTRES;

			if(b < a)
				ret.cy = (DWORD)(1.0*ret.cy * a / b);
			else if(a < b)
				ret.cx = (DWORD)(1.0*ret.cx * b / a);
		}

		::ReleaseDC(0, hDC);
	}

	return ret;
}

void CMainFrame::ToggleFullscreen(bool fToNearest, bool fSwitchScreenResWhenHasTo)
{
	CRect r;
	const CWnd* pWndInsertAfter;
	DWORD dwRemove = 0, dwAdd = 0;
	DWORD dwRemoveEx = 0, dwAddEx = 0;
	HMENU hMenu;

	if(!m_fFullScreen)
	{
		GetWindowRect(&m_lastWindowRect);
		m_fOnTopBeforeFullScreen = !!(GetWindowLong(m_hWnd, GWL_EXSTYLE)&WS_EX_TOPMOST);

		dispmode& dm = AfxGetAppSettings().dmFullscreenRes;
		m_dmBeforeFullscreen.fValid = false;
		if(dm.fValid && fSwitchScreenResWhenHasTo)
		{
			GetCurDispMode(m_dmBeforeFullscreen);
			SetDispMode(dm);
		}

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		dwRemove = WS_CAPTION|WS_THICKFRAME;
		if(fToNearest) r = mi.rcMonitor;
		else GetDesktopWindow()->GetWindowRect(&r);
		pWndInsertAfter = &wndTopMost;
		hMenu = NULL;
	}
	else
	{
		if(m_dmBeforeFullscreen.fValid)
			SetDispMode(m_dmBeforeFullscreen);

		dwAdd = (AfxGetAppSettings().fHideCaptionMenu ? 0 : WS_CAPTION) | WS_THICKFRAME;
		r = m_lastWindowRect;
		pWndInsertAfter = m_fOnTopBeforeFullScreen?&wndTopMost:&wndNoTopMost;
		hMenu = AfxGetAppSettings().fHideCaptionMenu ? NULL : m_hMenuDefault;
	}

	m_lastMouseMove.x = m_lastMouseMove.y = -1;

	bool fAudioOnly = m_fAudioOnly;
	m_fAudioOnly = true;

	m_fFullScreen = !m_fFullScreen;

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	ModifyStyleEx(dwRemoveEx, dwAddEx, SWP_NOZORDER);
	::SetMenu(m_hWnd, hMenu);
	SetWindowPos(pWndInsertAfter, r.left, r.top, r.Width(), r.Height(), SWP_NOSENDCHANGING /*SWP_FRAMECHANGED*/);

	if(m_fFullScreen)
	{
		m_fHideCursor = true;
        ShowControls(CS_NONE, false);
	}
	else
	{
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		m_fHideCursor = false;
        ShowControls(AfxGetAppSettings().nCS);
	}

	m_wndView.SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);

	m_fAudioOnly = fAudioOnly;

	MoveVideoWindow();
}

void CMainFrame::MoveVideoWindow(bool fShowStats)
{
	if(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && IsWindowVisible())
	{
		CRect wr;
		if(!m_fFullScreen)
		{
			m_wndView.GetClientRect(wr);
			if(!AfxGetAppSettings().fHideCaptionMenu)
				wr.DeflateRect(2, 2);
		}
		else
		{
			GetWindowRect(&wr);

			// HACK
			CRect r;
			m_wndView.GetWindowRect(&r);
			wr -= r.TopLeft();
		}

		CRect vr = CRect(0,0,0,0);

		OAFilterState fs = GetMediaState();
		if(fs == State_Paused || fs == State_Running || fs == State_Stopped && (m_fShockwaveGraph || m_fQuicktimeGraph))
		{
			CSize arxy = GetVideoSize();

			int iDefaultVideoSize = AfxGetAppSettings().iDefaultVideoSize;

			CSize ws = 
				iDefaultVideoSize == DVS_HALF ? CSize(arxy.cx/2, arxy.cy/2) :
				iDefaultVideoSize == DVS_NORMAL ? arxy :
				iDefaultVideoSize == DVS_DOUBLE ? CSize(arxy.cx*2, arxy.cy*2) :
				wr.Size();

			int w = ws.cx;
			int h = ws.cy;

			if(!m_fShockwaveGraph) //&& !m_fQuicktimeGraph)
			{
				if(iDefaultVideoSize == DVS_FROMINSIDE || iDefaultVideoSize == DVS_FROMOUTSIDE)
				{
					h = ws.cy;
					w = MulDiv(h, arxy.cx, arxy.cy);

					if(iDefaultVideoSize == DVS_FROMINSIDE && w > ws.cx
					|| iDefaultVideoSize == DVS_FROMOUTSIDE && w < ws.cx)
					{
						w = ws.cx;
						h = MulDiv(w, arxy.cy, arxy.cx);
					}
				}
			}

			CSize size(
				(int)(m_ZoomX*w), 
				(int)(m_ZoomY*h));

			CPoint pos(
				(int)(m_PosX*(wr.Width()*3 - m_ZoomX*w) - wr.Width()), 
				(int)(m_PosY*(wr.Height()*3 - m_ZoomY*h) - wr.Height()));

/*			CPoint pos(
				(int)(m_PosX*(wr.Width() - size.cx)), 
				(int)(m_PosY*(wr.Height() - size.cy)));

*/
			vr = CRect(pos, size);
		}

		wr |= CRect(0,0,0,0);
		vr |= CRect(0,0,0,0);

		if(m_pCAP)
		{
			m_pCAP->SetPosition(wr, vr);
		}
		else
		{
			HRESULT hr;
			hr = pBV->SetDefaultSourcePosition();
			hr = pBV->SetDestinationPosition(vr.left, vr.top, vr.Width(), vr.Height());
			hr = pVW->SetWindowPosition(wr.left, wr.top, wr.Width(), wr.Height());
		}

		m_wndView.SetVideoRect(wr);

		if(fShowStats && vr.Height() > 0)
		{
			CString info;
			info.Format(_T("Pos %.2f %.2f, Zoom %.2f %.2f, AR %.2f"), m_PosX, m_PosY, m_ZoomX, m_ZoomY, (float)vr.Width()/vr.Height());
			SendStatusMessage(info, 3000);
		}
	}
	else
	{
		m_wndView.SetVideoRect();
	}
}

void CMainFrame::ZoomVideoWindow(double scale)
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	if(scale <= 0)
	{
		int iZoomLevel = AfxGetAppSettings().iZoomLevel;
		scale = iZoomLevel == 0 ? 0.5 : iZoomLevel == 2 ? 2.0 : 1.0;
	}

	if(m_fFullScreen)
	{
		OnViewFullscreen();
	}

	MINMAXINFO mmi;
	OnGetMinMaxInfo(&mmi);

	CRect r;
	int w = 0, h = 0;

	if(!m_fAudioOnly)
	{
		CSize arxy = GetVideoSize();

		long lWidth = int(arxy.cx * scale + 0.5);
		long lHeight = int(arxy.cy * scale + 0.5);

		DWORD style = GetStyle();

		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		w = GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2
				+ r1.Width() - r2.Width()
				+ lWidth;

		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		h = GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2
				+ (mbi.rcBar.bottom - mbi.rcBar.top)
				+ r1.Height() - r2.Height()
				+ lHeight;

		if(style&WS_CAPTION)
		{
			h += GetSystemMetrics(SM_CYCAPTION);
			w += 2; h += 2; // for the 1 pixel wide sunken frame
			w += 2; h += 3; // for the inner black border
		}

		GetWindowRect(r);

		w = max(w, mmi.ptMinTrackSize.x);
		h = max(h, mmi.ptMinTrackSize.y);
	}
	else
	{
		GetWindowRect(r);

		w = r.Width(); // mmi.ptMinTrackSize.x;
		h = mmi.ptMinTrackSize.y;
	}

	// center window
	if(!AfxGetAppSettings().fRememberWindowPos)
	{
		CPoint cp = r.CenterPoint();
		r.left = cp.x - w/2;
		r.top = cp.y - h/2;
	}

	r.right = r.left + w;
	r.bottom = r.top + h;

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
	if(r.right > mi.rcWork.right) r.OffsetRect(mi.rcWork.right-r.right, 0);
	if(r.left < mi.rcWork.left) r.OffsetRect(mi.rcWork.left-r.left, 0);
	if(r.bottom > mi.rcWork.bottom) r.OffsetRect(0, mi.rcWork.bottom-r.bottom);
	if(r.top < mi.rcWork.top) r.OffsetRect(0, mi.rcWork.top-r.top);

	MoveWindow(r);
//	ShowWindow(SW_SHOWNORMAL);

	MoveVideoWindow();
}

void CMainFrame::RepaintVideo()
{
	if(m_pCAP) m_pCAP->Paint(false);
}

void CMainFrame::SetBalance(int balance)
{
	AfxGetAppSettings().nBalance = balance;

	int sign = balance>0?-1:1;
	balance = max(100-abs(balance), 1);
	balance = (int)((log10(1.0*balance)-2)*5000*sign);
	balance = max(min(balance, 10000), -10000);

	if(m_iMediaLoadState == MLS_LOADED) 
		pBA->put_Balance(balance);
}

void CMainFrame::SetupIViAudReg()
{
	if(!AfxGetAppSettings().fAutoSpeakerConf) return;

	DWORD spc = 0, defchnum = 0;

	if(AfxGetAppSettings().fAutoSpeakerConf)
	{
		CComPtr<IDirectSound> pDS;
		if(SUCCEEDED(DirectSoundCreate(NULL, &pDS, NULL))
		&& SUCCEEDED(pDS->SetCooperativeLevel(m_hWnd, DSSCL_NORMAL)))
		{
			if(SUCCEEDED(pDS->GetSpeakerConfig(&spc)))
			{
				switch(spc)
				{
				case DSSPEAKER_DIRECTOUT: defchnum = 6; break;
				case DSSPEAKER_HEADPHONE: defchnum = 2; break;
				case DSSPEAKER_MONO: defchnum = 1; break;
				case DSSPEAKER_QUAD: defchnum = 4; break;
				default:
				case DSSPEAKER_STEREO: defchnum = 2; break;
				case DSSPEAKER_SURROUND: defchnum = 2; break;
				case DSSPEAKER_5POINT1: defchnum = 5; break;
				case DSSPEAKER_7POINT1: defchnum = 5; break;
				}
			}
		}
	}
	else
	{
		defchnum = 2;
	}

	CRegKey iviaud;
	if(ERROR_SUCCESS == iviaud.Create(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\InterVideo\\Common\\AudioDec")))
	{
		DWORD chnum = 0;
		if(FAILED(iviaud.QueryDWORDValue(_T("AUDIO"), chnum))) chnum = 0;
		if(chnum <= defchnum) // check if the user has already set it..., but we won't skip if it's lower than sensible :P
			iviaud.SetDWORDValue(_T("AUDIO"), defchnum);
	}
}

//
// Open/Close
//

void CMainFrame::OpenCreateGraphObject(OpenMediaData* pOMD)
{
	pGB = NULL;

	m_fCustomGraph = false;
	m_fRealMediaGraph = m_fShockwaveGraph = m_fQuicktimeGraph = false;

	if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD))
	{
		AppSettings& s = AfxGetAppSettings();

		HRESULT hr;
		CComPtr<IUnknown> pUnk;

		engine_t engine = s.Formats.GetEngine(p->fns.GetHead());

		if(engine == RealMedia)
		{
			if(!(pUnk = (IUnknown*)(INonDelegatingUnknown*)new CRealMediaGraph(m_wndView.m_hWnd, hr)))
				throw _T("Out of memory");

			m_pCAP = CComQIPtr<ISubPicAllocatorPresenter>(pUnk);

//			if(FAILED(hr) || !(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
//				throw _T("RealMedia files require RealPlayer/RealOne to be installed");
			if(SUCCEEDED(hr) && !!(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
				m_fRealMediaGraph = true;
		}
		else if(engine == ShockWave)
		{
			if(!(pUnk = (IUnknown*)(INonDelegatingUnknown*)new CShockwaveGraph(m_wndView.m_hWnd, hr)))
				throw _T("Out of memory");

			if(FAILED(hr) || !(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
				throw _T("Can't create shockwave control");

			m_fShockwaveGraph = true;
		}
		else if(engine == QuickTime)
		{
			if(!(pUnk = (IUnknown*)(INonDelegatingUnknown*)new CQuicktimeGraph(m_wndView.m_hWnd, hr)))
				throw _T("Out of memory");

			m_pCAP = CComQIPtr<ISubPicAllocatorPresenter>(pUnk);

//			if(FAILED(hr) || !(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
//				throw _T("Can't initialize QuickTime");
			if(SUCCEEDED(hr) && !!(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
                m_fQuicktimeGraph = true;
		}

		m_fCustomGraph = m_fRealMediaGraph || m_fShockwaveGraph || m_fQuicktimeGraph;
	}

	if(!m_fCustomGraph)
	{
		if(FAILED(pGB.CoCreateInstance(CLSID_FilterGraph)))
		{
			throw _T("Failed to create the filter graph object");
		}
	}

	pMC = pGB; pME = pGB; pMS = pGB; // general
	pVW = pGB; pBV = pGB; // video
	pBA = pGB; // audio
	pFS = pGB;

	if(!(pMC && pME && pMS)
	|| !(pVW && pBV)
	|| !(pBA))
	{
		throw _T("Failed to query the needed interfaces for playback");
	}

	if(FAILED(pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0)))
	{
		throw _T("Could not set target window for graph notification");
	}

	m_pProv = (IUnknown*)new CKeyProvider();
	
	if(CComQIPtr<IObjectWithSite> pObjectWithSite = pGB)
		pObjectWithSite->SetSite(m_pProv);
}

void CMainFrame::OpenFile(OpenFileData* pOFD)
{
	if(pOFD->fns.IsEmpty())
		throw _T("Invalid argument");

	AppSettings& s = AfxGetAppSettings();

    CGraphBuilderFile gb(pGB, m_wndView.m_hWnd);

	bool fFirst = true;

	POSITION pos = pOFD->fns.GetHeadPosition();
	while(pos)
	{
		CString fn = pOFD->fns.GetNext(pos);

		fn.Trim();
		if(fn.IsEmpty() && !fFirst)
			break;

		HRESULT hr = gb.Render(fn);

		if(FAILED(hr))
		{
			if(fFirst)
			{
				if(s.fReportFailedPins && gb.GetDeadEnd(0))
				{
					CMediaTypesDlg(gb, this).DoModal();

					if(gb.GetStreamCount() == 0)
						throw _T("Cannot render any of the streams");
				}

				CString err;

				switch(hr)
				{
				case E_ABORT: err = _T("Opening aborted"); break;
				case E_FAIL: case E_POINTER: default: err = _T("Failed to render the file"); break;
				case E_INVALIDARG: err = _T("Invalid file name"); break;
				case E_OUTOFMEMORY: err = _T("Out of memory"); break;
				case VFW_E_CANNOT_CONNECT: err = _T("Cannot connect the filters"); break;
				case VFW_E_CANNOT_LOAD_SOURCE_FILTER: err = _T("Cannot load any source filter"); break;
				case VFW_E_CANNOT_RENDER: err = _T("Cannot render the file"); break;
				case VFW_E_INVALID_FILE_FORMAT: err = _T("Invalid file format"); break;
				case VFW_E_NOT_FOUND: err = _T("File not found"); break;
				case VFW_E_UNKNOWN_FILE_TYPE: err = _T("Unknown file type"); break;
				case VFW_E_UNSUPPORTED_STREAM: err = _T("Unsupported stream"); break;
				}

				throw err;
			}
		}

		if(s.fKeepHistory)
		{
			CRecentFileList* pMRU = fFirst ? &s.MRU : &s.MRUDub;
			pMRU->ReadList();
			pMRU->Add(fn);
			pMRU->WriteList();
		}

		if(fFirst)
		{
			pOFD->title = fn;
		}

		fFirst = false;

		if(m_fCustomGraph) break;
	}

	if(s.fReportFailedPins && gb.GetDeadEnd(0))
		CMediaTypesDlg(gb, this).DoModal();

	if(!m_fCustomGraph)
		gb.FindInterface(__uuidof(ISubPicAllocatorPresenter), (void**)&m_pCAP);

	if(!(pAMOP = pGB))
	{
		BeginEnumFilters(pGB, pEF, pBF)
			if(pAMOP = pBF) break;
		EndEnumFilters
	}

	if(m_chapters.IsEmpty())
	{
		CComQIPtr<IChapterInfo> pCI;
		BeginEnumFilters(pGB, pEF, pBF)
			if(pCI = pBF) break;
		EndEnumFilters
		if(pCI)
		{
			REFERENCE_TIME rtDur = 0;
			pMS->GetDuration(&rtDur);

			CHAR iso6391[3];
			::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, iso6391, 3);
			CStringA iso6392 = ISO6391To6392(iso6391);
			if(iso6392.GetLength() < 3) iso6392 = "eng";

			UINT cnt = pCI->GetChapterCount(CHAPTER_ROOT_ID);
			for(UINT i = 1; i <= cnt; i++)
			{
				UINT cid = pCI->GetChapterId(CHAPTER_ROOT_ID, i);
				ChapterElement ce;
				CComBSTR bstr;
				CHAR pl[3] = {iso6392[0], iso6392[1], iso6392[2]};
				CHAR cc[] = "  ";
				if(pCI->GetChapterInfo(cid, &ce))
				{
					chapter_t c;
					c.rtStart = ce.rtStart;
					c.rtStop = max(rtDur, ce.rtStop); // TODO

					bstr.Attach(pCI->GetChapterStringInfo(cid, pl, cc));
					c.name = CString(bstr);

					if(m_chapters.GetCount() > 0)
						m_chapters[m_chapters.GetCount()-1].rtStop = c.rtStart;

					m_chapters.Add(c);
				}
			}
		}
	}

	if(m_chapters.IsEmpty())
	{
		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(GetCLSID(pBF) != CLSID_OggSplitter)
				continue;

			REFERENCE_TIME rtDur = 0;
			pMS->GetDuration(&rtDur);

			BeginEnumPins(pBF, pEP, pPin)
			{
				if(CComQIPtr<IPropertyBag> pPB = pPin)
				{
            		for(int i = 1; ; i++)
					{
						CStringW str;
						CComVariant var;

						var.Clear();
						str.Format(L"CHAPTER%02d", i);
						if(S_OK != pPB->Read(str, &var, NULL)) 
							break;

						int h, m, s, ms;
						WCHAR wc;
						if(7 != swscanf(CStringW(var), L"%d%c%d%c%d%c%d", &h, &wc, &m, &wc, &s, &wc, &ms)) 
							break;

						chapter_t c;
						c.rtStart = 10000i64*(((h*60 + m)*60 + s)*1000 + ms);
						c.rtStop = rtDur ? rtDur : c.rtStart;

						var.Clear();
                        str += L"NAME";
						if(S_OK == pPB->Read(str, &var, NULL))
							c.name = var;

						if(m_chapters.GetCount() > 0)
							m_chapters[m_chapters.GetCount()-1].rtStop = c.rtStart;

						m_chapters.Add(c);
					}
				}

				if(!m_chapters.IsEmpty())
					break;
			}
			EndEnumPins

			break;
		}
		EndEnumFilters
	}

	CComQIPtr<IKeyFrameInfo> pKFI;
	BeginEnumFilters(pGB, pEF, pBF)
		if(pKFI = pBF) break;
	EndEnumFilters
	UINT nKFs = 0, nKFsTmp = 0;
	if(pKFI && S_OK == pKFI->GetKeyFrameCount(nKFs) && nKFs > 0)
	{
		m_kfs.SetSize(nKFsTmp = nKFs);
		if(S_OK != pKFI->GetKeyFrames(&TIME_FORMAT_MEDIA_TIME, m_kfs.GetData(), nKFsTmp) || nKFsTmp != nKFs)
			m_kfs.SetSize(0);
	}

	m_iPlaybackMode = PM_FILE;
}

void CMainFrame::OpenDVD(OpenDVDData* pODD)
{
	CGraphBuilderDVD gb(pGB, m_wndView.m_hWnd);

	HRESULT hr = gb.Render(pODD->path, pODD->title);

	AppSettings& s = AfxGetAppSettings();

	if(hr == VFW_E_CANNOT_LOAD_SOURCE_FILTER)
		throw _T("Can't find DVD directory");
	else if(hr == VFW_E_CANNOT_RENDER)
		throw _T("Failed to render all pins of the DVD Navigator filter");
	else if(hr == VFW_S_PARTIAL_RENDER)
	{
		if(s.fReportFailedPins && gb.GetDeadEnd(0))
			CMediaTypesDlg(gb, this).DoModal();

		throw _T("Failed to render some of the pins of the DVD Navigator filter");
	}
	else if(hr == E_NOINTERFACE)
		throw _T("Failed to query the needed interfaces for DVD playback");
	else if(FAILED(hr))
		throw _T("Can't create the DVD Navigator filter");

	gb.FindInterface(__uuidof(ISubPicAllocatorPresenter), (void**)&m_pCAP);
	gb.FindInterface(__uuidof(IDvdControl2), (void**)&pDVDC);
	gb.FindInterface(__uuidof(IDvdInfo2), (void**)&pDVDI);

	if(!pDVDC || !pDVDI)
		throw _T("Failed to query the needed interfaces for DVD playback");

	if(s.idMenuLang)
		pDVDC->SelectDefaultMenuLanguage(s.idMenuLang);
	if(s.idAudioLang)
		pDVDC->SelectDefaultAudioLanguage(s.idAudioLang, DVD_AUD_EXT_NotSpecified);
	if(s.idSubtitlesLang)
		pDVDC->SelectDefaultSubpictureLanguage(s.idSubtitlesLang, DVD_SP_EXT_NotSpecified);

	m_iDVDDomain = DVD_DOMAIN_Stop;

	m_iPlaybackMode = PM_DVD;
}

void CMainFrame::OpenCapture(OpenDeviceData* pODD)
{
	CStringW vidfrname, audfrname;
	CComPtr<IBaseFilter> pVidCapTmp, pAudCapTmp;

	m_VidDispName = pODD->DisplayName[0];

	if(!m_VidDispName.IsEmpty())
	{
		if(!CreateFilter(m_VidDispName, &pVidCapTmp, vidfrname))
			throw _T("Can't create video capture filter");
	}

	m_AudDispName = pODD->DisplayName[1];

	if(!m_AudDispName.IsEmpty())
	{
		if(!CreateFilter(m_AudDispName, &pAudCapTmp, audfrname))
			throw _T("Can't create video capture filter");
	}

	if(!pVidCapTmp && !pAudCapTmp)
	{
		throw _T("No capture filters");
	}

	pCGB = NULL;
	pVidCap = NULL;
	pAudCap = NULL;

	if(FAILED(pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2)))
	{
		throw _T("Can't create capture graph builder object");
	}

	HRESULT hr;

	pCGB->SetFiltergraph(pGB);

	if(pVidCapTmp)
	{
		if(FAILED(hr = pGB->AddFilter(pVidCapTmp, vidfrname)))
		{
			throw _T("Can't add video capture filter to the graph");
		}

		pVidCap = pVidCapTmp;

		if(!pAudCapTmp)
		{
			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap))
			|| FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));

			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))
			|| FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
		}
		else
		{
			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));

			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
		}

		if(FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidCap, IID_IAMCrossbar, (void**)&pAMXBar)))
			TRACE(_T("Warning: No IAMCrossbar interface was found\n"));

		if(FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidCap, IID_IAMTVTuner, (void**)&pAMTuner)))
			TRACE(_T("Warning: No IAMTVTuner interface was found\n"));
/*
		if(pAMVSCCap) 
		{
//DumpStreamConfig(_T("c:\\mpclog.txt"), pAMVSCCap);
            CComQIPtr<IAMVfwCaptureDialogs> pVfwCD = pVidCap;
			if(!pAMXBar && pVfwCD)
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(viddispname, pAMVSCCap, pVfwCD);
			}
			else
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(viddispname, pAMVSCCap, pAMXBar, pAMTuner);
			}
		}
*/
	}

	if(pAudCapTmp)
	{
		if(FAILED(hr = pGB->AddFilter(pAudCapTmp, CStringW(audfrname))))
		{
			throw _T("Can't add audio capture filter to the graph");
		}

		pAudCap = pAudCapTmp;

		if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pAudCap, IID_IAMStreamConfig, (void **)&pAMASC))
		&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, pAudCap, IID_IAMStreamConfig, (void **)&pAMASC)))
		{
			TRACE(_T("Warning: No IAMStreamConfig interface for vidcap"));
		}
/*
		CInterfaceArray<IAMAudioInputMixer> pAMAIM;

		BeginEnumPins(pAudCap, pEP, pPin)
		{
			PIN_DIRECTION dir;
			if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_INPUT)
				continue;

			if(CComQIPtr<IAMAudioInputMixer> pAIM = pPin) 
				pAMAIM.Add(pAIM);
		}
		EndEnumPins

		if(pAMASC)
		{
			m_wndCaptureBar.m_capdlg.SetupAudioControls(auddispname, pAMASC, pAMAIM);
		}
*/
	}

	if(!(pVidCap || pAudCap))
	{
		throw _T("Couldn't open any device");
	}

	pODD->title = _T("Live");
	
	m_iPlaybackMode = PM_CAPTURE;
}

void CMainFrame::OpenCustomizeGraph()
{
	if(m_iPlaybackMode == PM_CAPTURE)
		return;

	CleanGraph();

	if(m_iPlaybackMode == PM_FILE)
	{
		if(m_pCAP)
			AddTextPassThruFilter();
	}

	if(m_iPlaybackMode == PM_DVD)
	{
		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(CComQIPtr<IDirectVobSub2> pDVS2 = pBF)
			{
//				pDVS2->AdviseSubClock(m_pSubClock = new CSubClock);
//				break;

				// TODO: test multiple dvobsub instances with one clock
				if(!m_pSubClock) m_pSubClock = new CSubClock;
				pDVS2->AdviseSubClock(m_pSubClock);
			}
		}
		EndEnumFilters
	}

	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(GetCLSID(pBF) == CLSID_OggSplitter)
		{
			if(CComQIPtr<IAMStreamSelect> pSS = pBF)
			{
				LCID idAudio = AfxGetAppSettings().idAudioLang;
				if(!idAudio) idAudio = GetUserDefaultLCID();
				LCID idSub = AfxGetAppSettings().idSubtitlesLang;
				if(!idSub) idSub = GetUserDefaultLCID();

				DWORD cnt = 0;
				pSS->Count(&cnt);
				for(DWORD i = 0; i < cnt; i++)
				{
					AM_MEDIA_TYPE* pmt = NULL;
					DWORD dwFlags = 0;
					LCID lcid = 0;
					DWORD dwGroup = 0;
					WCHAR* pszName = NULL;
					if(SUCCEEDED(pSS->Info((long)i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
					{
						CStringW name(pszName), sound(L"Sound"), subtitle(L"Subtitle");

						if(idAudio != -1 && (idAudio&0x3ff) == (lcid&0x3ff) // sublang seems to be zeroed out in ogm...
						&& name.GetLength() > sound.GetLength()
						&& !name.Left(sound.GetLength()).CompareNoCase(sound))
						{
							if(SUCCEEDED(pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE)))
								idAudio = -1;
						}

						if(idSub != -1 && (idSub&0x3ff) == (lcid&0x3ff) // sublang seems to be zeroed out in ogm...
						&& name.GetLength() > subtitle.GetLength()
						&& !name.Left(subtitle.GetLength()).CompareNoCase(subtitle)
						&& name.Mid(subtitle.GetLength()).Trim().CompareNoCase(L"off"))
						{
							if(SUCCEEDED(pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE)))
								idSub = -1;
						}

						if(pmt) DeleteMediaType(pmt);
						if(pszName) CoTaskMemFree(pszName);
					}
				}
			}
		}
	}
	EndEnumFilters

	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(CComQIPtr<IMpeg2DecFilter> m_pMpeg2DecFilter = pBF)
		{
			AppSettings& s = AfxGetAppSettings();
			m_pMpeg2DecFilter->SetDeinterlaceMethod((ditype)s.mpegdi);
			m_pMpeg2DecFilter->SetBrightness(s.mpegbright);
			m_pMpeg2DecFilter->SetContrast(s.mpegcont);
			m_pMpeg2DecFilter->SetHue(s.mpeghue);
			m_pMpeg2DecFilter->SetSaturation(s.mpegsat);
			m_pMpeg2DecFilter->EnableForcedSubtitles(s.mpegforcedsubs);
			m_pMpeg2DecFilter->EnablePlanarYUV(s.mpegplanaryuv);
		}
	}
	EndEnumFilters

	CleanGraph();
}

void CMainFrame::OpenSetupVideoWindow()
{
	m_fAudioOnly = true;

	if(m_pCAP)
	{
		CSize vs = m_pCAP->GetVideoSize();
		m_fAudioOnly = (vs.cx <= 0 || vs.cy <= 0);
	}
	else
	{
		{
			long w = 0, h = 0;

			if(CComQIPtr<IBasicVideo> pBV = pGB)
			{
				pBV->GetVideoSize(&w, &h);
			}

			if(w > 0 && h > 0)
			{
				m_fAudioOnly = false;
			}
		}

		if(m_fAudioOnly)
		{
			BeginEnumFilters(pGB, pEF, pBF)
			{
				long w = 0, h = 0;

				if(CComQIPtr<IVideoWindow> pVW = pBF)
				{
					long lVisible;
					if(FAILED(pVW->get_Visible(&lVisible)))
						continue;

					pVW->get_Width(&w);
					pVW->get_Height(&h);
				}

				if(w > 0 && h > 0)
				{
					m_fAudioOnly = false;
					break;
				}
			}
			EndEnumFilters
		}
	}

	if(m_fShockwaveGraph)
	{
		m_fAudioOnly = false;
	}

	if(!m_pCAP)
	{
		pVW->put_Owner((OAHWND)m_wndView.m_hWnd);
		pVW->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
		pVW->put_MessageDrain((OAHWND)m_hWnd);

		for(CWnd* pWnd = m_wndView.GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow())
			pWnd->EnableWindow(FALSE); // little trick to let WM_SETCURSOR thru
	}
}

void CMainFrame::OpenSetupToolBar()
{
	m_wndToolBar.Volume = AfxGetAppSettings().nVolume;
	SetBalance(AfxGetAppSettings().nBalance);
}

void CMainFrame::OpenSetupCaptureBar()
{
	if(m_iPlaybackMode == PM_CAPTURE)
	{
		if(pVidCap && pAMVSCCap) 
		{
            CComQIPtr<IAMVfwCaptureDialogs> pVfwCD = pVidCap;

			if(!pAMXBar && pVfwCD)
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, pAMVSCCap, pVfwCD);
			}
			else
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, pAMVSCCap, pAMXBar, pAMTuner);
			}
		}

		if(pAudCap && pAMASC)
		{
			CInterfaceArray<IAMAudioInputMixer> pAMAIM;

			BeginEnumPins(pAudCap, pEP, pPin)
			{
				PIN_DIRECTION dir;
				if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_INPUT)
					continue;

				if(CComQIPtr<IAMAudioInputMixer> pAIM = pPin) 
					pAMAIM.Add(pAIM);
			}
			EndEnumPins

			m_wndCaptureBar.m_capdlg.SetupAudioControls(m_AudDispName, pAMASC, pAMAIM);
		}
	}

	BuildGraphVideoAudio(
		!!m_wndCaptureBar.m_capdlg.m_fVidPreview, false, 
		!!m_wndCaptureBar.m_capdlg.m_fAudPreview, false);
}

void CMainFrame::OpenSetupInfoBar()
{
	if(m_iPlaybackMode == PM_FILE)
	{
		bool fEmpty = true;
		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF)
			{
				CComBSTR bstr;
				if(SUCCEEDED(pAMMC->get_Title(&bstr))) {m_wndInfoBar.SetLine(_T("Title"), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_AuthorName(&bstr))) {m_wndInfoBar.SetLine(_T("Author"), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_Copyright(&bstr))) {m_wndInfoBar.SetLine(_T("Copyright"), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_Rating(&bstr))) {m_wndInfoBar.SetLine(_T("Rating"), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_Description(&bstr))) {m_wndInfoBar.SetLine(_T("Description"), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(!fEmpty)
				{
					RecalcLayout();
					break;
				}
			}
		}
		EndEnumFilters
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		CString info('-');
		m_wndInfoBar.SetLine(_T("Domain"), info);
		m_wndInfoBar.SetLine(_T("Location"), info);
		m_wndInfoBar.SetLine(_T("Video"), info);
		m_wndInfoBar.SetLine(_T("Audio"), info);
		m_wndInfoBar.SetLine(_T("Subtitles"), info);
		RecalcLayout();
	}
}

void CMainFrame::OpenSetupStatsBar()
{
	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(pQP = pBF)
		{
			CString info('-');
			m_wndStatsBar.SetLine(_T("Frame-rate"), info);
			m_wndStatsBar.SetLine(_T("Sync Offset (avg)"), info);
			m_wndStatsBar.SetLine(_T("Sync Offset (dev)"), info);
			m_wndStatsBar.SetLine(_T("Frames Drawn"), info);
			m_wndStatsBar.SetLine(_T("Frames Dropped"), info);
			m_wndStatsBar.SetLine(_T("Jitter"), info);
			RecalcLayout();
			break;
		}
	}
	EndEnumFilters
}

void CMainFrame::OpenSetupStatusBar()
{
	m_wndStatusBar.ShowTimer(true);

	//

	if(!m_fCustomGraph)
	{
		UINT id = IDB_NOAUDIO;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			CComQIPtr<IBasicAudio> pBA = pBF;
			if(!pBA) continue;

			BeginEnumPins(pBF, pEP, pPin)
			{
				PIN_DIRECTION dir;
				CComPtr<IPin> pPinTo;
				if(S_OK == pPin->QueryDirection(&dir) && dir == PINDIR_INPUT
				&& S_OK == pPin->ConnectedTo(&pPinTo) && pPinTo)
				{
					AM_MEDIA_TYPE mt;
					memset(&mt, 0, sizeof(mt));
					pPin->ConnectionMediaType(&mt);

					if(mt.majortype == MEDIATYPE_Audio 
					&& mt.formattype == FORMAT_WaveFormatEx)
					{
						switch(((WAVEFORMATEX*)mt.pbFormat)->nChannels)
						{
						case 1: id = IDB_MONO; break;
						case 2: default: id = IDB_STEREO; break;
						}
						break;
					}
					else if(mt.majortype == MEDIATYPE_Midi)
					{
						id = NULL;
						break;
					}
				}
			}
			EndEnumPins

			if(id != IDB_NOAUDIO)
			{
				break;
			}
		}
		EndEnumFilters

		m_wndStatusBar.SetStatusBitmap(id);
	}

	//

	HICON hIcon = NULL;

	if(m_iPlaybackMode == PM_FILE)
	{
		CString fn = m_wndPlaylistBar.GetCur();
		CString ext = fn.Mid(fn.ReverseFind('.')+1);
		hIcon = LoadIcon(ext, true);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		hIcon = LoadIcon(_T(".ifo"), true);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		// hIcon = ; // TODO
	}

	m_wndStatusBar.SetStatusTypeIcon(hIcon);
}

void CMainFrame::OpenSetupWindowTitle(CString fn)
{
	CString title(MAKEINTRESOURCE(IDR_MAINFRAME));

	int i = AfxGetAppSettings().iTitleBarTextStyle;

	if(!fn.IsEmpty() && (i == 0 || i == 1))
	{
		if(i == 1)
		{
			if(m_iPlaybackMode == PM_FILE)
			{
				fn.Replace('\\', '/');
				CString fn2 = fn.Mid(fn.ReverseFind('/')+1);
				if(!fn2.IsEmpty()) fn = fn2;
			}
			else if(m_iPlaybackMode == PM_DVD)
			{
				fn = _T("DVD");
			}
			else if(m_iPlaybackMode == PM_CAPTURE)
			{
				fn = _T("Live");
			}
		}

		title = fn + _T(" - ") + title;
	}

	SetWindowText(title);
}

bool CMainFrame::OpenMediaPrivate(CAutoPtr<OpenMediaData> pOMD)
{
	if(m_iMediaLoadState != MLS_CLOSED)
	{
		ASSERT(0);
		return(false);
	}

	if(!dynamic_cast<OpenFileData*>(pOMD.m_p)
	&& !dynamic_cast<OpenDVDData*>(pOMD.m_p)
	&& !dynamic_cast<OpenDeviceData*>(pOMD.m_p))
	{
		ASSERT(0);
		return(false);
	}

	if(OpenFileData* pOFD = dynamic_cast<OpenFileData*>(pOMD.m_p))
	{
		if(pOFD->fns.IsEmpty())
			return(false);

		CString fn = pOFD->fns.GetHead();

		int i = fn.Find(_T(":\\"));
		if(i > 0)
		{
			CString drive = fn.Left(i+2);
			UINT type = GetDriveType(drive);
			CStringList sl;
			if(type == DRIVE_REMOVABLE || type == DRIVE_CDROM && GetCDROMType(drive[0], sl) != CDROM_Audio)
			{
				int ret = IDRETRY;
				while(ret == IDRETRY)
				{
					WIN32_FIND_DATA findFileData;
					HANDLE h = FindFirstFile(fn, &findFileData);
					if(h != INVALID_HANDLE_VALUE)
					{
						FindClose(h);
						ret = IDOK;
					}
					else
					{
						CString msg;
						msg.Format(_T("%s was not found, please insert media containing this file."), fn);
						ret = AfxMessageBox(msg, MB_RETRYCANCEL);
					}
				}

				if(ret != IDOK)
					return(false);
			}
		}
	}

	m_iMediaLoadState = MLS_LOADING;

	// FIXME: Don't show "Closed" initially
	PostMessage(WM_KICKIDLE);

	CString err, aborted(_T("Aborted"));

	try
	{
		if(m_fOpeningAborted) throw aborted;

		OpenCreateGraphObject(pOMD);
AddToRot(pGB, &m_dwRegister);

		if(m_fOpeningAborted) throw aborted;

		SetupIViAudReg();

		if(m_fOpeningAborted) throw aborted;

		if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p))
		{
			OpenFile(p);
		}
		else if(OpenDVDData* p = dynamic_cast<OpenDVDData*>(pOMD.m_p))
		{
			OpenDVD(p);
		}
		else if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p))
		{
			OpenCapture(p);
		}
		else
		{
			throw _T("Can't open, invalid input parameters");
		}

		if(m_fOpeningAborted) throw aborted;

		OpenCustomizeGraph();

		if(m_fOpeningAborted) throw aborted;

		OpenSetupVideoWindow();

		if(m_fOpeningAborted) throw aborted;

		if(m_pCAP && (!m_fAudioOnly || m_fRealMediaGraph))
		{
			POSITION pos = pOMD->subs.GetHeadPosition();
			while(pos) LoadSubtitle(pOMD->subs.GetNext(pos));

			if(m_pSubStreams.GetCount() > 0)
				SetSubtitle(m_pSubStreams.GetHead());
		}

		if(m_fOpeningAborted) throw aborted;

		OpenSetupWindowTitle(pOMD->title);

		if(::GetCurrentThreadId() == AfxGetApp()->m_nThreadID)
		{
			OnFilePostOpenmedia();
		}
		else
		{
			PostMessage(WM_COMMAND, ID_FILE_POST_OPENMEDIA);
		}

		while(m_iMediaLoadState != MLS_LOADED)
		{
			Sleep(50);
		}

		// PostMessage instead of SendMessage because the user might call CloseMedia and then we would deadlock

		PostMessage(WM_COMMAND, ID_PLAY_PAUSE);

		if(!(AfxGetAppSettings().nCLSwitches&CLSW_OPEN))
			PostMessage(WM_COMMAND, ID_PLAY_PLAY);

		AfxGetAppSettings().nCLSwitches &= ~CLSW_OPEN;

		if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p))
		{
			if(p->rtStart > 0)
				PostMessage(WM_RESUMEFROMSTATE, (WPARAM)PM_FILE, (LPARAM)(p->rtStart/10000)); // REFERENCE_TIME doesn't fit in LPARAM under a 32bit env.
		}
		else if(OpenDVDData* p = dynamic_cast<OpenDVDData*>(pOMD.m_p))
		{
			if(p->pDvdState)
				PostMessage(WM_RESUMEFROMSTATE, (WPARAM)PM_DVD, (LPARAM)(CComPtr<IDvdState>(p->pDvdState).Detach())); // must be released by the called message handler
		}
	}
	catch(LPCTSTR msg)
	{
		err = msg;
	}
	catch(CString msg)
	{
		err = msg;
	}

	if(!err.IsEmpty())
	{
		CloseMediaPrivate();
		m_closingmsg = err;

		OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p);
		if(p && err != aborted)
		{
			m_wndPlaylistBar.SetCurValid(false);
			if(m_wndPlaylistBar.GetCount() > 1)
			{
				CPlaylistItem pli[2];
				m_wndPlaylistBar.GetCur(pli[0]);
				m_wndPlaylistBar.SetNext();
				m_wndPlaylistBar.GetCur(pli[1]);
				if(pli[0].m_id != pli[1].m_id)
				{
					p->fns.RemoveAll();
					p->fns.AddTail((CStringList*)&pli[1].m_fns);
					p->rtStart = 0;
					OpenMediaPrivate(pOMD);
				}
			}
		}
	}
	else
	{
		m_wndPlaylistBar.SetCurValid(true);
	}

	PostMessage(WM_KICKIDLE); // calls main thread to update things

	return(err.IsEmpty());
}

void CMainFrame::CloseMediaPrivate()
{
	m_iMediaLoadState = MLS_CLOSING;

    OnPlayStop(); // SendMessage(WM_COMMAND, ID_PLAY_STOP);

	m_iPlaybackMode = PM_NONE;
	m_iSpeedLevel = 0;

	m_fLiveWM = false;

	m_rtDurationOverride = -1;

	m_kfs.RemoveAll();

RemoveFromRot(m_dwRegister);

//	if(pVW) pVW->put_Visible(OAFALSE);
//	if(pVW) pVW->put_MessageDrain((OAHWND)NULL), pVW->put_Owner((OAHWND)NULL);

	m_pCAP = NULL; // IMPORTANT: IVMRSurfaceAllocatorNotify/IVMRSurfaceAllocatorNotify9 has to be released before the VMR/VMR9, otherwise it will crash in Release()

	pAMXBar.Release(); pAMTuner.Release(); pAMDF.Release();
	pAMVCCap.Release(); pAMVCPrev.Release(); pAMVSCCap.Release(); pAMVSCPrev.Release(); pAMASC.Release();
	pVidCap.Release(); pAudCap.Release();
	pCGB.Release();
	pVMR.Release();
	pDVDC.Release(); pDVDI.Release();
	pQP.Release(); pAMOP.Release(); pFS.Release();
	pMC.Release(); pME.Release(); pMS.Release();
	pVW.Release(); pBV.Release();
	pBA.Release();
	pGB.Release();

	m_fRealMediaGraph = m_fShockwaveGraph = m_fQuicktimeGraph = false;

	m_pSubClock = NULL;

	m_pProv.Release();

	{
		CAutoLock cAutoLock(&m_csSubLock);
		m_pSubStreams.RemoveAll();
	}

	m_chapters.RemoveAll();

	m_closingmsg = _T("Closed");

	AfxGetAppSettings().nCLSwitches &= CLSW_OPEN|CLSW_PLAY|CLSW_SHUTDOWN|CLSW_CLOSE;

	m_iMediaLoadState = MLS_CLOSED;
}

// dynamic menus

void CMainFrame::SetupOpenCDSubMenu()
{
	CMenu* pSub = &m_opencds;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState == MLS_LOADING) return;

	if(AfxGetAppSettings().fHideCDROMsSubMenu) return;

	UINT id = ID_FILE_OPEN_CD_START;

	for(TCHAR drive = 'C'; drive <= 'Z'; drive++)
	{
		CString label = GetDriveLabel(drive), str;

		CStringList files;
		switch(GetCDROMType(drive, files))
		{
		case CDROM_Audio:
			if(label.IsEmpty()) label = _T("Audio CD");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		case CDROM_VideoCD:
			if(label.IsEmpty()) label = _T("(S)VCD");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		case CDROM_DVDVideo:
			if(label.IsEmpty()) label = _T("DVD Video");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		default:
			break;
		}

		if(!str.IsEmpty())
			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, str);
	}
}

void CMainFrame::SetupFiltersSubMenu()
{
	CMenu* pSub = &m_filters;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	m_filterpopups.RemoveAll();

	m_spparray.RemoveAll();
	m_ssarray.RemoveAll();

	if(m_iMediaLoadState == MLS_LOADED)
	{
		UINT idf = 0;
		UINT ids = ID_FILTERS_SUBITEM_START;
		UINT idl = ID_FILTERSTREAMS_SUBITEM_START;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			CString name(GetFilterName(pBF));
			if(name.GetLength() >= 43) name = name.Left(40) + _T("...");

			CLSID clsid = GetCLSID(pBF);
			if(clsid == CLSID_AVIDec)
			{
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
				{
					DWORD c = ((VIDEOINFOHEADER*)mt.pbFormat)->bmiHeader.biCompression;
					switch(c)
					{
					case BI_RGB: name += _T(" (RGB)"); break;
					case BI_RLE4: name += _T(" (RLE4)"); break;
					case BI_RLE8: name += _T(" (RLE8)"); break;
					case BI_BITFIELDS: name += _T(" (BITF)"); break;
					default: name.Format(_T("%s (%c%c%c%c)"), 
								 CString(name), (TCHAR)((c>>0)&0xff), (TCHAR)((c>>8)&0xff), (TCHAR)((c>>16)&0xff), (TCHAR)((c>>24)&0xff)); break;
					}
				}
			}
			else if(clsid == CLSID_ACMWrapper)
			{
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
				{
					WORD c = ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag;
					name.Format(_T("%s (0x%04x)"), CString(name), (int)c);
				}
			}
			else if(clsid == __uuidof(CTextPassThruFilter) || clsid == __uuidof(CNullTextRenderer)
				|| clsid == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) // ISCR
			{
				// hide these
				continue;
			}

			CAutoPtr<CMenu> pSubSub(new CMenu);
			pSubSub->CreatePopupMenu();

			int nPPages = 0;

			CComQIPtr<ISpecifyPropertyPages> pSPP = pBF;
			if(pSPP)
			{
				CAUUID caGUID;
				caGUID.pElems = NULL;
				if(SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0)
				{
					m_spparray.Add(pSPP);
					pSubSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ids, _T("&Properties..."));

					if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);

					nPPages++;
				}
			}

			BeginEnumPins(pBF, pEP, pPin)
			{
				CString name = GetPinName(pPin);

				if(pSPP = pPin)
				{
					CAUUID caGUID;
					caGUID.pElems = NULL;
					if(SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0)
					{
						m_spparray.Add(pSPP);
						pSubSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ids+nPPages, name + _T(" (pin) properties..."));

						if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
						
						nPPages++;
					}
				}
			}
			EndEnumPins

			CComQIPtr<IAMStreamSelect> pSS = pBF;
			if(pSS)
			{
				DWORD nStreams = 0, flags, group, prevgroup = -1;
				LCID lcid;
				WCHAR* wname = NULL;
				CComPtr<IUnknown> pObj, pUnk;

				pSS->Count(&nStreams);

				if(nStreams > 0 && nPPages > 0) pSubSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

				UINT idlstart = idl;

				for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
				{
					m_ssarray.Add(pSS);

					flags = group = 0;
					wname = NULL;
					pSS->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);

					if(group != prevgroup && idl > idlstart)
						pSubSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);
					prevgroup = group;

					if(flags & AMSTREAMSELECTINFO_EXCLUSIVE)
					{
					}
					else if(flags & AMSTREAMSELECTINFO_ENABLED)
					{
					}

					if(!wname) 
					{
						CStringW stream(L"Unknown Stream");
						wname = (WCHAR*)CoTaskMemAlloc((stream.GetLength()+3+1)*sizeof(WCHAR));
						swprintf(wname, L"%s %d", stream, min(i+1,999));
					}

					pSubSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, idl++, CString(wname));

					CoTaskMemFree(wname);
				}

				if(nStreams == 0) pSS.Release();
			}

			if(nPPages == 1 && !pSS)
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ids, name);
			}
			else 
			{
				pSub->AppendMenu(MF_BYPOSITION|MF_STRING|MF_DISABLED|MF_GRAYED, idf, name);

				if(nPPages > 0 || pSS)
				{
					MENUITEMINFO mii;
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_STATE|MIIM_SUBMENU;
					mii.fType = MF_POPUP;
					mii.hSubMenu = pSubSub->m_hMenu;
					mii.fState = (pSPP || pSS) ? MF_ENABLED : (MF_DISABLED|MF_GRAYED);
					pSub->SetMenuItemInfo(idf, &mii, TRUE);

					m_filterpopups.Add(pSubSub);
				}
			}

			ids += nPPages;
			idf++;
		}
		EndEnumFilters
	}
}

void CMainFrame::SetupAudioSwitcherSubMenu()
{
	CMenu* pSub = &m_audios;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState == MLS_LOADED)
	{
		UINT id = ID_AUDIO_SUBITEM_START;

		CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
		if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

		if(pSS)
		{
			DWORD cStreams = 0;
			if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0)
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, _T("&Options..."));
				pSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

				for(int i = 0; i < (int)cStreams; i++)
				{
					WCHAR* pName = NULL;
					if(FAILED(pSS->Info(i, NULL, NULL, NULL, NULL, &pName, NULL, NULL)))
						break;

					CString name(pName);
					CoTaskMemFree(pName);
					pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, name);
				}
			}
		}
	}
}

void CMainFrame::SetupSubtitlesSubMenu()
{
	CMenu* pSub = &m_subtitles;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED || m_fAudioOnly || !m_pCAP)
		return;

	UINT id = ID_SUBTITLES_SUBITEM_START;

	POSITION pos = m_pSubStreams.GetHeadPosition();

	if(pos)
	{
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, _T("&Options..."));
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, _T("&Styles..."));
		pSub->AppendMenu(MF_SEPARATOR);

		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, _T("&Enable"));
		pSub->AppendMenu(MF_SEPARATOR);
	}

	while(pos)
	{
		CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);
		if(!pSubStream) continue;

		for(int i = 0, j = pSubStream->GetStreamCount(); i < j; i++)
		{
			WCHAR* pName = NULL;
			if(SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL)))
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, CString(pName));
				CoTaskMemFree(pName);
			}
			else
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, _T("<Unknown>"));
			}
		}

		// TODO: find a better way to group these entries
		if(pos && m_pSubStreams.GetAt(pos))
		{
			CLSID cur, next;
			pSubStream->GetClassID(&cur);
			m_pSubStreams.GetAt(pos)->GetClassID(&next);

			if(cur != next)
				pSub->AppendMenu(MF_SEPARATOR);
		}
	}
}

void CMainFrame::SetupNavAudioSubMenu()
{
	CMenu* pSub = &m_navaudio;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED) return;

	UINT id = ID_NAVIGATE_AUDIO_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupNavOgmSubMenu(pSub, id, _T("sound"));
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		if(FAILED(pDVDI->GetCurrentAudio(&ulStreamsAvailable, &ulCurrentStream)))
			return;

		LCID DefLanguage;
		DVD_AUDIO_LANG_EXT ext;
		if(FAILED(pDVDI->GetDefaultAudioLanguage(&DefLanguage, &ext)))
			return;

        for(ULONG i = 0; i < ulStreamsAvailable; i++)
		{
			LCID Language;
			if(FAILED(pDVDI->GetAudioLanguage(i, &Language)))
				continue;

			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(Language == DefLanguage) flags |= MF_DEFAULT;
            if(i == ulCurrentStream) flags |= MF_CHECKED;

			CString str(_T("Unknown"));
			if(Language)
			{
				int len = GetLocaleInfo(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(max(len-1, 0));
			}

			DVD_AudioAttributes ATR;
			if(SUCCEEDED(pDVDI->GetAudioAttributes(i, &ATR)))
			{
				switch(ATR.LanguageExtension)
				{
				case DVD_AUD_EXT_NotSpecified:
				default: break;
				case DVD_AUD_EXT_Captions: str += _T(" (Captions)"); break;
				case DVD_AUD_EXT_VisuallyImpaired: str += _T(" (Visually Impaired)"); break;
				case DVD_AUD_EXT_DirectorComments1: str += _T(" (Director Comments 1)"); break;
				case DVD_AUD_EXT_DirectorComments2: str += _T(" (Director Comments 2)"); break;
				}
			}

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavSubtitleSubMenu()
{
	CMenu* pSub = &m_navsubtitle;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED) return;

	UINT id = ID_NAVIGATE_SUBP_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupNavOgmSubMenu(pSub, id, _T("subtitle"));
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if(FAILED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
		|| ulStreamsAvailable == 0)
			return;

		LCID DefLanguage;
		DVD_SUBPICTURE_LANG_EXT ext;
		if(FAILED(pDVDI->GetDefaultSubpictureLanguage(&DefLanguage, &ext)))
			return;

		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|(bIsDisabled?0:MF_CHECKED), id++, _T("Enabled"));
		pSub->AppendMenu(MF_BYCOMMAND|MF_SEPARATOR|MF_ENABLED);

        for(ULONG i = 0; i < ulStreamsAvailable; i++)
		{
			LCID Language;
			if(FAILED(pDVDI->GetSubpictureLanguage(i, &Language)))
				continue;

			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(Language == DefLanguage) flags |= MF_DEFAULT;
            if(i == ulCurrentStream) flags |= MF_CHECKED;

			CString str(_T("Unknown"));
			if(Language)
			{
				int len = GetLocaleInfo(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(max(len-1, 0));
			}

			DVD_SubpictureAttributes ATR;
			if(SUCCEEDED(pDVDI->GetSubpictureAttributes(i, &ATR)))
			{
				switch(ATR.LanguageExtension)
				{
				case DVD_SP_EXT_NotSpecified:
				default: break;
				case DVD_SP_EXT_Caption_Normal: str += _T(""); break;
				case DVD_SP_EXT_Caption_Big: str += _T(" (Big)"); break;
				case DVD_SP_EXT_Caption_Children: str += _T(" (Children)"); break;
				case DVD_SP_EXT_CC_Normal: str += _T(" (CC)"); break;
				case DVD_SP_EXT_CC_Big: str += _T(" (CC Big)"); break;
				case DVD_SP_EXT_CC_Children: str += _T(" (CC Children)"); break;
				case DVD_SP_EXT_Forced: str += _T(" (Forced)"); break;
				case DVD_SP_EXT_DirectorComments_Normal: str += _T(" (Director Comments)"); break;
				case DVD_SP_EXT_DirectorComments_Big: str += _T(" (Director Comments, Big)"); break;
				case DVD_SP_EXT_DirectorComments_Children: str += _T(" (Director Comments, Children)"); break;
				}
			}

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavAngleSubMenu()
{
	CMenu* pSub = &m_navangle;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED) return;

	UINT id = ID_NAVIGATE_ANGLE_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupNavOgmSubMenu(pSub, id, _T("video"));
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		if(FAILED(pDVDI->GetCurrentAngle(&ulStreamsAvailable, &ulCurrentStream)))
			return;

		if(ulStreamsAvailable < 2) return; // one choice is not a choice...

        for(ULONG i = 0; i < ulStreamsAvailable; i++)
		{
			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
            if(i == ulCurrentStream) flags |= MF_CHECKED;

			CString str;
			str.Format(_T("Angle %d"), i+1);

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavChaptersSubMenu()
{
	CMenu* pSub = &m_navchapters;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED)
		return;

	UINT id = ID_NAVIGATE_CHAP_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		if(m_chapters.GetCount())
		{
			REFERENCE_TIME rt = GetPos();

			for(int i = 0; i < m_chapters.GetCount(); i++)
			{
				chapter_t& c = m_chapters[i];

				int s = (int)((c.rtStart/10000000)%60);
				int m = (int)((c.rtStart/10000000/60)%60);
				int h = (int)((c.rtStart/10000000/60/60));

				CString t;
				t.Format(_T("[%02d:%02d:%02d] "), h, m, s);

				UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
				if(c.rtStart <= rt && rt < c.rtStop) flags |= MF_CHECKED;
				if(id != ID_NAVIGATE_CHAP_SUBITEM_START && i == 0) pSub->AppendMenu(MF_SEPARATOR);
				pSub->AppendMenu(flags, id++, t + c.name);
			}
		}

		if(m_wndPlaylistBar.GetCount() > 1)
		{
			POSITION pos = m_wndPlaylistBar.m_pl.GetHeadPosition();
			while(pos)
			{
				UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
				if(pos == m_wndPlaylistBar.m_pl.GetPos()) flags |= MF_CHECKED;
				if(id != ID_NAVIGATE_CHAP_SUBITEM_START && pos == m_wndPlaylistBar.m_pl.GetHeadPosition())
					pSub->AppendMenu(MF_SEPARATOR);
				CPlaylistItem& pli = m_wndPlaylistBar.m_pl.GetNext(pos);
				pSub->AppendMenu(flags, id++, pli.m_fns.GetHead());
			}
		}
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		ULONG ulUOPs = 0;
		pDVDI->GetCurrentUOPS(&ulUOPs);

		for(ULONG i = 1; i <= ulNumOfTitles; i++)
		{
			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(i == Location.TitleNum) flags |= MF_CHECKED;
			if(ulUOPs&UOP_FLAG_Play_Title) flags |= MF_DISABLED|MF_GRAYED;

			CString str;
			str.Format(_T("Title %d"), i);

			pSub->AppendMenu(flags, id++, str);
		}

		for(ULONG i = 1; i <= ulNumOfChapters; i++)
		{
			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(i == Location.ChapterNum) flags |= MF_CHECKED;
			if(ulUOPs&UOP_FLAG_Play_Chapter) flags |= MF_DISABLED|MF_GRAYED;
			if(i == 1) flags |= MF_MENUBARBREAK;

			CString str;
			str.Format(_T("Chapter %d"), i);

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavOgmSubMenu(CMenu* pSub, UINT id, CString type)
{
	UINT baseid = id;

	if(CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB))
	{
		DWORD cStreams;
		if(FAILED(pSS->Count(&cStreams)))
			return;

		DWORD dwPrevGroup = -1;

		for(int i = 0, j = cStreams; i < j; i++)
		{
			DWORD dwFlags, dwGroup;
			LCID lcid;
			WCHAR* pszName = NULL;

			if(FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
			|| !pszName)
				continue;

			CString name(pszName);
			name.MakeLower();

			if(pszName) CoTaskMemFree(pszName);

			if(name.Find(type) != 0)
				continue;

			if(dwPrevGroup != -1 && dwPrevGroup != dwGroup)
				pSub->AppendMenu(MF_SEPARATOR);
			dwPrevGroup = dwGroup;

			CString str;

			if(name.Find(type + _T(" off")) == 0)
			{
				str = _T("Disabled");
			}
			else if(lcid == 0)
			{
				str.Format(_T("Unknown %d"), id-baseid);
			}
			else
			{
				int len = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, str.GetBuffer(64), 64);
				str.ReleaseBufferSetLength(max(len-1, 0));
			}

			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(dwFlags) flags |= MF_CHECKED;
			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::OnNavOgmSubMenu(UINT id, CString type)
{
	if(CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB))
	{
		DWORD cStreams;
		if(FAILED(pSS->Count(&cStreams)))
			return;

		for(int i = 0, j = cStreams; i < j; i++)
		{
			DWORD dwFlags, dwGroup;
			LCID lcid;
			WCHAR* pszName = NULL;

			if(FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
			|| !pszName)
				continue;

			CString name(pszName);
			name.MakeLower();

			if(pszName) CoTaskMemFree(pszName);

			if(name.Find(type) != 0)
				continue;

			if(id == 0)
			{
				pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
				break;
			}

			id--;
		}
	}
}

void CMainFrame::SetupFavoritesSubMenu()
{
	CMenu* pSub = &m_favorites;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	AppSettings& s = AfxGetAppSettings();

	pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_FAVORITES_ADD, _T("&Add to Favorites..."));
	pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_FAVORITES_ORGANIZE, _T("&Organize Favorites..."));

	int nLastGroupStart = pSub->GetMenuItemCount();

	UINT id = ID_FAVORITES_FILE_START;

	CStringList sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;

		CString str = sl.GetNext(pos);

		int i = str.Find(';');
		if(i >= 0)
			pSub->AppendMenu(flags, id, str.Left(i));

		id++;
	}

	if(id > ID_FAVORITES_FILE_START)
		pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR|MF_ENABLED|MF_BYPOSITION);

	nLastGroupStart = pSub->GetMenuItemCount();

	id = ID_FAVORITES_DVD_START;

	AfxGetAppSettings().GetFav(FAV_DVD, sl);

	pos = sl.GetHeadPosition();
	while(pos)
	{
		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;

		CString str = sl.GetNext(pos);

		int i = str.Find(';');
		if(i >= 0)
			pSub->AppendMenu(flags, id, str.Left(i));

		id++;
	}

	if(id > ID_FAVORITES_DVD_START)
		pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR|MF_ENABLED|MF_BYPOSITION);

	nLastGroupStart = pSub->GetMenuItemCount();

	id = ID_FAVORITES_DEVICE_START;

	AfxGetAppSettings().GetFav(FAV_DEVICE, sl);

	pos = sl.GetHeadPosition();
	while(pos)
	{
		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;

		CString str = sl.GetNext(pos);

		int i = str.Find(';');
		if(i >= 0)
			pSub->AppendMenu(flags, id, str.Left(i));

		id++;
	}
}

/////////////

void CMainFrame::ShowControls(int nCS, bool fSave)
{
	int nCSprev = AfxGetAppSettings().nCS;
	int hbefore = 0, hafter = 0;

	m_pLastBar = NULL;

	POSITION pos = m_bars.GetHeadPosition();
	for(int i = 1; pos; i <<= 1)
	{
		CControlBar* pNext = m_bars.GetNext(pos);
		ShowControlBar(pNext, !!(nCS&i), TRUE);
		if(nCS&i) m_pLastBar = pNext;

		CSize s = pNext->CalcFixedLayout(FALSE, TRUE);
		if(nCSprev&i) hbefore += s.cy;
		if(nCS&i) hafter += s.cy;
	}

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);

	if(wp.showCmd != SW_SHOWMAXIMIZED && !m_fFullScreen)
	{
		CRect r;
		GetWindowRect(r);
		MoveWindow(r.left, r.top, r.Width(), r.Height()+(hafter-hbefore));
	}

    if(fSave)
		AfxGetAppSettings().nCS = nCS;

	RecalcLayout();
}


void CMainFrame::SetAlwaysOnTop(bool f)
{
	if(!m_fFullScreen) SetWindowPos(f ? &wndTopMost : &wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	AfxGetAppSettings().fAlwaysOnTop = f;
}

void CMainFrame::AddTextPassThruFilter()
{
	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(!IsSplitter(pBF)) continue;

		BeginEnumPins(pBF, pEP, pPin)
		{
			CComPtr<IPin> pPinTo;
			AM_MEDIA_TYPE mt;
			if(FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo 
			|| FAILED(pPin->ConnectionMediaType(&mt)) 
			|| mt.majortype != MEDIATYPE_Text && mt.majortype != MEDIATYPE_Subtitle)
				continue;

			CComQIPtr<IBaseFilter> pTPTF = new CTextPassThruFilter(this);
			CStringW name;
			name.Format(L"TextPassThru%08x", pTPTF);
			if(FAILED(pGB->AddFilter(pTPTF, name)))
				continue;

			HRESULT hr;

			hr = pPinTo->Disconnect();
			hr = pPin->Disconnect();

			if(FAILED(hr = pGB->ConnectDirect(pPin, GetFirstPin(pTPTF, PINDIR_INPUT), NULL))
			|| FAILED(hr = pGB->ConnectDirect(GetFirstPin(pTPTF, PINDIR_OUTPUT), pPinTo, NULL)))
				hr = pGB->ConnectDirect(pPin, pPinTo, NULL);
			else
				m_pSubStreams.AddTail(CComQIPtr<ISubStream>(pTPTF));
		}
		EndEnumPins
	}
	EndEnumFilters
}

bool CMainFrame::LoadSubtitle(CString fn)
{
	CComPtr<ISubStream> pSubStream;

	if(!pSubStream)
	{
		CAutoPtr<CVobSubFile> pVSF(new CVobSubFile(&m_csSubLock));
		if(pVSF && pVSF->Open(fn) && pVSF->GetStreamCount() > 0)
			pSubStream = pVSF.Detach();
	}

	if(!pSubStream)
	{
		CAutoPtr<CRenderedTextSubtitle> pRTS(new CRenderedTextSubtitle(&m_csSubLock));
		if(pRTS && pRTS->Open(fn, DEFAULT_CHARSET) && pRTS->GetStreamCount() > 0)
			pSubStream = pRTS.Detach();
	}

	if(pSubStream)
	{
		m_pSubStreams.AddTail(pSubStream);
	}

	return(!!pSubStream);
}

void CMainFrame::UpdateSubtitle(bool fApplyDefStyle)
{
	if(!m_pCAP) return;

	int i = m_iSubtitleSel;

	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos && i >= 0)
	{
		CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

		if(i < pSubStream->GetStreamCount()) 
		{
			CAutoLock cAutoLock(&m_csSubLock);
			pSubStream->SetStream(i);
			SetSubtitle(pSubStream, fApplyDefStyle);
			return;
		}

		i -= pSubStream->GetStreamCount();
	}

	m_pCAP->SetSubPicProvider(NULL);
}

void CMainFrame::SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle)
{
	AppSettings& s = AfxGetAppSettings();

	if(pSubStream)
	{
		CLSID clsid;
		pSubStream->GetClassID(&clsid);

		if(clsid == __uuidof(CVobSubFile))
		{
			CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSF->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos, 1, 1);
			}
		}
		if(clsid == __uuidof(CVobSubStream))
		{
			CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSS->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos, 1, 1);
			}
		}
		else if(clsid == __uuidof(CRenderedTextSubtitle))
		{
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

			if(fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle)
			{
				STSStyle style = s.subdefstyle;

				if(s.fOverridePlacement)
				{
					style.scrAlignment = 2;
					int w = pRTS->m_dstScreenSize.cx;
					int h = pRTS->m_dstScreenSize.cy;
					int mw = w - style.marginRect.left - style.marginRect.right;
					style.marginRect.bottom = h - MulDiv(h, s.nVerPos, 100);
					style.marginRect.left = MulDiv(w, s.nHorPos, 100) - mw/2;
					style.marginRect.right = w - (style.marginRect.left + mw);
				}

				pRTS->SetDefaultStyle(style);
			}

			pRTS->Deinit();
		}
	}

	if(!fApplyDefStyle)
	{
		m_iSubtitleSel = -1;

		if(pSubStream)
		{

		int i = 0;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(pos)
		{
			CComPtr<ISubStream> pSubStream2 = m_pSubStreams.GetNext(pos);

			if(pSubStream == pSubStream2)
			{
				m_iSubtitleSel = i + pSubStream2->GetStream();
				break;
			}

			i += pSubStream2->GetStreamCount();
		}

		}
	}

	m_nSubtitleId = (DWORD_PTR)pSubStream;

	if(m_pCAP)
	{
		m_pCAP->SetSubPicProvider(CComQIPtr<ISubPicProvider>(pSubStream));
		m_wndSubresyncBar.SetSubtitle(pSubStream, m_pCAP->GetFPS());
	}
}

void CMainFrame::InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate)
{
	if(m_pCAP)
	{
		if(nSubtitleId == -1 || nSubtitleId == m_nSubtitleId)
			m_pCAP->Invalidate(rtInvalidate);
	}
}

REFERENCE_TIME CMainFrame::GetPos()
{
	return(m_iMediaLoadState == MLS_LOADED ? m_wndSeekBar.GetPos() : 0);
}

void CMainFrame::SeekTo(REFERENCE_TIME rtPos, bool fSeekToKeyFrame)
{
	OAFilterState fs = GetMediaState();

	if(rtPos < 0) rtPos = 0;

	if(m_iPlaybackMode == PM_FILE)
	{
		if(fs == State_Stopped)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		HRESULT hr;

		if(fSeekToKeyFrame)
		{
			if(!m_kfs.IsEmpty())
			{
				int i = rangebsearch(rtPos, m_kfs);
				if(i >= 0 && i < m_kfs.GetCount())
					rtPos = m_kfs[i];
			}
		}

		hr = pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
	}
	else if(m_iPlaybackMode == PM_DVD && m_iDVDDomain == DVD_DOMAIN_Title)
	{
		if(fs != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		DVD_HMSF_TIMECODE tc = RT2HMSF(rtPos);
		pDVDC->PlayAtTime(&tc, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);

//		if(fs != State_Running)
//			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	}
	else if(m_iPlaybackMode == PM_CAPTURE)
	{
		TRACE(_T("Warning (CMainFrame::SeekTo): Trying to seek in capture mode"));
	}
}

void CMainFrame::CleanGraph()
{
	if(!pGB) return;

	BeginEnumFilters(pGB, pEF, pBF)
	{
		CComQIPtr<IAMFilterMiscFlags> pAMMF(pBF);
		if(pAMMF && (pAMMF->GetMiscFlags()&AM_FILTER_MISC_FLAGS_IS_SOURCE))
			continue;

		// some capture filters forget to set AM_FILTER_MISC_FLAGS_IS_SOURCE 
		// or to implement the IAMFilterMiscFlags interface
		if(pBF == pVidCap || pBF == pAudCap) 
			continue;

		if(CComQIPtr<IFileSourceFilter>(pBF))
			continue;

		int nIn, nOut, nInC, nOutC;
		if(CountPins(pBF, nIn, nOut, nInC, nOutC) > 0 && (nInC+nOutC) == 0)
		{
			TRACE(CStringW(L"Removing: ") + GetFilterName(pBF) + '\n');

			pGB->RemoveFilter(pBF);
			pEF->Reset();
		}
	}
	EndEnumFilters
}

#define AUDIOBUFFERLEN 500

static void SetLatency(IBaseFilter* pBF, int cbBuffer)
{
	BeginEnumPins(pBF, pEP, pPin)
	{
		if(CComQIPtr<IAMBufferNegotiation> pAMBN = pPin)
		{
			ALLOCATOR_PROPERTIES ap;
			ap.cbAlign = -1;  // -1 means no preference.
			ap.cbBuffer = cbBuffer;
			ap.cbPrefix = -1;
			ap.cBuffers = -1;
			pAMBN->SuggestAllocatorProperties(&ap);
		}
	}
	EndEnumPins
}

HRESULT CMainFrame::BuildCapture(IPin* pPin, IBaseFilter* pBF[3], const GUID& majortype, AM_MEDIA_TYPE* pmt)
{
	IBaseFilter* pBuff = pBF[0];
	IBaseFilter* pEnc = pBF[1];
	IBaseFilter* pMux = pBF[2];

	if(!pPin || !pMux) return E_FAIL;

	CString err;

	HRESULT hr = S_OK;

	AddFilterToGraph(pGB, pMux, L"Multiplexer");

	IUnknown* pPrev = pPin;

	CStringW prefix, prefixl;
	if(majortype == MEDIATYPE_Video) prefix = L"Video ";
	else if(majortype == MEDIATYPE_Audio) prefix = L"Audio ";
	prefixl = prefix;
	prefixl.MakeLower();

	if(pBuff)
	{
		hr = pGB->AddFilter(pBuff, prefix + L"Buffer");
		if(FAILED(hr))
		{
			err = _T("Can't add ") + CString(prefixl) + _T("buffer filter");
			AfxMessageBox(err);
			return hr;
		}

		hr = pCGB->RenderStream(NULL, &majortype, pPrev, NULL, pBuff);
		if(FAILED(hr))
		{
			err = _T("Error connecting the ") + CString(prefixl) + _T("buffer filter");
			AfxMessageBox(err);
			return(hr);
		}

		pPrev = pBuff;
	}

	if(pEnc)
	{
		hr = pGB->AddFilter(pEnc, prefix + L"Encoder");
		if(FAILED(hr))
		{
			err = _T("Can't add ") + CString(prefixl) + _T("encoder filter");
			AfxMessageBox(err);
			return hr;
		}

		hr = pCGB->RenderStream(NULL, &majortype, pPrev, NULL, pEnc);
		if(FAILED(hr))
		{
			err = _T("Error connecting the ") + CString(prefixl) + _T("encoder filter");
			AfxMessageBox(err);
			return(hr);
		}

		pPrev = pEnc;

		if(CComQIPtr<IAMStreamConfig> pAMSC = GetFirstPin(pEnc, PINDIR_OUTPUT))
		{
			if(pmt->majortype == majortype)
			{
				hr = pAMSC->SetFormat(pmt);
				if(FAILED(hr))
				{
					err = _T("Can't set compression format on the ") + CString(prefixl) + _T("encoder filter");
					AfxMessageBox(err);
					return(hr);
				}
			}
		}

	}

//	if(pMux)
	{
		hr = pCGB->RenderStream(NULL, &majortype, pPrev, NULL, pMux);
		if(FAILED(hr))
		{
			err = _T("Error connecting ") + CString(prefixl) + _T(" to the muliplexer filter");
			AfxMessageBox(err);
			return(hr);
		}
	}

	CleanGraph();

	return S_OK;
}

bool CMainFrame::BuildToCapturePreviewPin(
	IBaseFilter* pVidCap, IPin** ppVidCapPin, IPin** ppVidPrevPin, 
	IBaseFilter* pAudCap, IPin** ppAudCapPin, IPin** ppAudPrevPin)
{
	HRESULT hr;

	*ppVidCapPin = *ppVidPrevPin = NULL; 
	*ppAudCapPin = *ppAudPrevPin = NULL;

	CComPtr<IPin> pDVAudPin;

	if(pVidCap)
	{
		CComPtr<IPin> pPin;
		if(!pAudCap // only look for interleaved stream when we don't use any other audio capture source
		&& SUCCEEDED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, TRUE, 0, &pPin)))
		{
			CComPtr<IBaseFilter> pDVSplitter;
			hr = pDVSplitter.CoCreateInstance(CLSID_DVSplitter);
			hr = pGB->AddFilter(pDVSplitter, L"DV Splitter");

			hr = pCGB->RenderStream(NULL, &MEDIATYPE_Interleaved, pPin, NULL, pDVSplitter);

			pPin = NULL;
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pPin);
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pDVAudPin);

			CComPtr<IBaseFilter> pDVDec;
			hr = pDVDec.CoCreateInstance(CLSID_DVVideoCodec);
			hr = pGB->AddFilter(pDVDec, L"DV Video Decoder");

			hr = pCGB->RenderStream(NULL, &MEDIATYPE_Video, pPin, NULL, pDVDec);

			pPin = NULL;
			hr = pCGB->FindPin(pDVDec, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pPin);
		}
		else if(SUCCEEDED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, TRUE, 0, &pPin)))
		{
		}
		else
		{
			AfxMessageBox(_T("No video capture pin found"));
			return(false);
		}

		CComPtr<IBaseFilter> pSmartTee;
		hr = pSmartTee.CoCreateInstance(CLSID_SmartTee);
		hr = pGB->AddFilter(pSmartTee, L"Smart Tee (video)");

		hr = pCGB->RenderStream(NULL, &MEDIATYPE_Video, pPin, NULL, pSmartTee);

		hr = pSmartTee->FindPin(L"Preview", ppVidPrevPin);
		hr = pSmartTee->FindPin(L"Capture", ppVidCapPin);
	}

	if(pAudCap || pDVAudPin)
	{
		CComPtr<IPin> pPin;
		if(pDVAudPin)
		{
			pPin = pDVAudPin;
		}
		else if(SUCCEEDED(pCGB->FindPin(pAudCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, TRUE, 0, &pPin)))
		{
		}
		else
		{
			AfxMessageBox(_T("No audio capture pin found"));
			return(false);
		}

		CComPtr<IBaseFilter> pSmartTee;
		hr = pSmartTee.CoCreateInstance(CLSID_SmartTee);
		hr = pGB->AddFilter(pSmartTee, L"Smart Tee (audio)");

		hr = pCGB->RenderStream(NULL, &MEDIATYPE_Audio, pPin, NULL, pSmartTee);

		hr = pSmartTee->FindPin(L"Preview", ppAudPrevPin);
		hr = pSmartTee->FindPin(L"Capture", ppAudCapPin);
	}

	return(true);
}

bool CMainFrame::BuildGraphVideoAudio(bool fVPreview, bool fVCapture, bool fAPreview, bool fACapture)
{
	if(!pCGB) return(false);

	SaveMediaState;

	HRESULT hr;

	NukeDownstream(pVidCap, pGB);
	NukeDownstream(pAudCap, pGB);

	CleanGraph();

	if(pAMVSCCap) hr = pAMVSCCap->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	if(pAMVSCPrev) hr = pAMVSCPrev->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	if(pAMASC) hr = pAMASC->SetFormat(&m_wndCaptureBar.m_capdlg.m_mta);

	CComPtr<IBaseFilter> pVidBuffer = m_wndCaptureBar.m_capdlg.m_pVidBuffer;
	CComPtr<IBaseFilter> pAudBuffer = m_wndCaptureBar.m_capdlg.m_pAudBuffer;
	CComPtr<IBaseFilter> pVidEnc = m_wndCaptureBar.m_capdlg.m_pVidEnc;
	CComPtr<IBaseFilter> pAudEnc = m_wndCaptureBar.m_capdlg.m_pAudEnc;
	CComPtr<IBaseFilter> pMux = m_wndCaptureBar.m_capdlg.m_pMux;
	CComPtr<IBaseFilter> pDst = m_wndCaptureBar.m_capdlg.m_pDst;
	CComPtr<IBaseFilter> pAudMux = m_wndCaptureBar.m_capdlg.m_pAudMux;
	CComPtr<IBaseFilter> pAudDst = m_wndCaptureBar.m_capdlg.m_pAudDst;

	bool fFileOutput = (pMux && pDst) || (pAudMux && pAudDst);
	bool fCapture = (fVCapture || fACapture);

	if(pAudCap)
	{
		AM_MEDIA_TYPE* pmt = &m_wndCaptureBar.m_capdlg.m_mta;
		int ms = (fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput) ? AUDIOBUFFERLEN : 60;
		if(pMux != pAudMux && fACapture) SetLatency(pAudCap, -1);
		else SetLatency(pAudCap, ((WAVEFORMATEX*)pmt->pbFormat)->nAvgBytesPerSec * ms / 1000);
	}

	CComPtr<IPin> pVidCapPin, pVidPrevPin, pAudCapPin, pAudPrevPin;
	BuildToCapturePreviewPin(pVidCap, &pVidCapPin, &pVidPrevPin, pAudCap, &pAudCapPin, &pAudPrevPin);

	CGraphBuilderCapture gb(pGB, m_wndView.m_hWnd);

//	if(pVidCap)
	{
		bool fVidPrev = pVidPrevPin && fVPreview;
		bool fVidCap = pVidCapPin && fVCapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fVidOutput;

		if(fVidPrev)
		{
			m_pCAP = NULL;
			gb.Render(pVidPrevPin);
			gb.FindInterface(__uuidof(ISubPicAllocatorPresenter), (void**)&m_pCAP);
		}

		if(fVidCap)
		{
			IBaseFilter* pBF[3] = {pVidBuffer, pVidEnc, pMux};
			HRESULT hr = BuildCapture(pVidCapPin, pBF, MEDIATYPE_Video, &m_wndCaptureBar.m_capdlg.m_mtcv);
		}

		pAMDF = NULL;
		pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMDroppedFrames, (void**)&pAMDF);
	}

//	if(pAudCap)
	{
		bool fAudPrev = pAudPrevPin && fAPreview;
		bool fAudCap = pAudCapPin && fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput;

		if(fAudPrev)
		{
			gb.Render(fAudCap ? pAudPrevPin : pAudCapPin);
		}

		if(fAudCap)
		{
			IBaseFilter* pBF[3] = {pAudBuffer, pAudEnc, pAudMux ? pAudMux : pMux};
			HRESULT hr = BuildCapture(pAudCapPin, pBF, MEDIATYPE_Audio, &m_wndCaptureBar.m_capdlg.m_mtca);
		}
	}

	if((pVidCap || pAudCap) && fCapture && fFileOutput)
	{
		if(pMux != pDst)
		{
			hr = pGB->AddFilter(pDst, L"File Writer V/A");
			hr = pCGB->RenderStream(NULL, NULL, pMux, NULL, pDst);
		}

		if(CComQIPtr<IConfigAviMux> pCAM = pMux)
		{
			int nIn, nOut, nInC, nOutC;
			CountPins(pMux, nIn, nOut, nInC, nOutC);
			pCAM->SetMasterStream(nInC-1);
//			pCAM->SetMasterStream(-1);
			pCAM->SetOutputCompatibilityIndex(FALSE);
		}

		if(CComQIPtr<IConfigInterleaving> pCI = pMux)
		{
//			if(FAILED(pCI->put_Mode(INTERLEAVE_CAPTURE)))
			if(FAILED(pCI->put_Mode(INTERLEAVE_NONE_BUFFERED)))
				pCI->put_Mode(INTERLEAVE_NONE);

			REFERENCE_TIME rtInterleave = 10000i64*AUDIOBUFFERLEN, rtPreroll = 0;//10000i64*500
			pCI->put_Interleaving(&rtInterleave, &rtPreroll);
		}

		if(pMux != pAudMux && pAudMux != pAudDst)
		{
			hr = pGB->AddFilter(pAudDst, L"File Writer A");
			hr = pCGB->RenderStream(NULL, NULL, pAudMux, NULL, pAudDst);
		}
	}

	REFERENCE_TIME stop = MAX_TIME;
	hr = pCGB->ControlStream(&PIN_CATEGORY_CAPTURE, NULL, NULL, NULL, &stop, 0, 0); // stop in the infinite

	CleanGraph();

	OpenSetupVideoWindow();
	OpenSetupStatsBar();
	OpenSetupStatusBar();

	RestoreMediaState;

	return(true);
}

bool CMainFrame::StartCapture()
{
	if(!pCGB || m_fCapturing) return(false);

	if(!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) return(false);

	HRESULT hr;

	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// rare to see two capture filters to support IAMPushSource at the same time...
//	hr = CComQIPtr<IAMGraphStreams>(pGB)->SyncUsingStreamOffset(TRUE); // TODO:

	BuildGraphVideoAudio(
		!!m_wndCaptureBar.m_capdlg.m_fVidPreview, true, 
		!!m_wndCaptureBar.m_capdlg.m_fAudPreview, true);

	hr = pME->CancelDefaultHandling(EC_REPAINT);

	SendMessage(WM_COMMAND, ID_PLAY_PLAY);

	m_fCapturing = true;

	return(true);
}

bool CMainFrame::StopCapture()
{
	if(!pCGB || !m_fCapturing) return(false);

	if(!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) return(false);

	HRESULT hr;

	m_wndStatusBar.SetStatusMessage(_T("Completing..."));

	m_fCapturing = false;

	BuildGraphVideoAudio(
		!!m_wndCaptureBar.m_capdlg.m_fVidPreview, false, 
		!!m_wndCaptureBar.m_capdlg.m_fAudPreview, false);

	hr = pME->RestoreDefaultHandling(EC_REPAINT);

	::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	m_rtDurationOverride = -1;

	return(true);
}

//

void CMainFrame::ShowOptions(int idPage)
{
	AppSettings& s = AfxGetAppSettings();

	CPPageSheet options(_T("Options"), pGB, this, idPage);

	if(options.DoModal() == IDOK)
	{
		if(!m_fFullScreen)
			SetAlwaysOnTop(s.fAlwaysOnTop);

		m_wndView.LoadLogo();

		s.UpdateData(true);
	}
}

void CMainFrame::StartWebServer(int nPort)
{
	if(!m_pWebServer)
		m_pWebServer.Attach(new CWebServer(this, nPort));
}

void CMainFrame::StopWebServer()
{
	if(m_pWebServer)
		m_pWebServer.Free();
}

void CMainFrame::SendStatusMessage(CString msg, int nTimeOut)
{
	KillTimer(TIMER_STATUSERASER);

	m_playingmsg.Empty();
	if(nTimeOut <= 0) return;

	m_playingmsg = msg;
	SetTimer(TIMER_STATUSERASER, nTimeOut, NULL);
}

void CMainFrame::OpenCurPlaylistItem(REFERENCE_TIME rtStart)
{
	if(m_wndPlaylistBar.GetCount() == 0)
		return;

	CPlaylistItem pli;
	if(!m_wndPlaylistBar.GetCur(pli)) m_wndPlaylistBar.SetFirst();
	if(!m_wndPlaylistBar.GetCur(pli)) return;

	if(CString(pli.m_fns.GetHead()).MakeLower().Find(_T("video_ts.ifo")) >= 0)
	{
		CAutoPtr<OpenDVDData> p(new OpenDVDData());
		if(p) {p->path = pli.m_fns.GetHead(); p->subs.AddTail(&pli.m_subs);}
		OpenMedia(p);
		return;
	}

	CAutoPtr<OpenFileData> p(new OpenFileData());
	if(p)
	{
		p->fns.AddTail((CStringList*)&pli.m_fns);
		p->subs.AddTail((CStringList*)&pli.m_subs);
		p->rtStart = rtStart;
	}
	OpenMedia(p);
}

static int s_fOpenedThruThread = false;

void CMainFrame::OpenMedia(CAutoPtr<OpenMediaData> pOMD)
{
	if(m_iMediaLoadState != MLS_CLOSED)
		CloseMedia();

	AppSettings& s = AfxGetAppSettings();

	bool fUseThread = true;

	if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p))
	{
		if(p->fns.GetCount() > 0)
		{
			engine_t e = s.Formats.GetEngine(p->fns.GetHead());
			fUseThread = e == DirectShow || /*e == RealMedia ||*/ e == QuickTime;
		}
	}
	else if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p))
	{
		fUseThread = false;
	}

	if(m_pGraphThread && fUseThread
	&& AfxGetAppSettings().fEnableWorkerThreadForOpening)
	{
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_OPEN, 0, (LPARAM)pOMD.Detach());
		s_fOpenedThruThread = true;
	}
	else
	{
		OpenMediaPrivate(pOMD);
		s_fOpenedThruThread = false;
	}
}

void CMainFrame::CloseMedia()
{
	if(m_iMediaLoadState == MLS_CLOSING)
	{
		TRACE(_T("WARNING: CMainFrame::CloseMedia() called twice or more\n"));
		return;
	}

	int nTimeWaited = 0;

	while(m_iMediaLoadState == MLS_LOADING)
	{
		m_fOpeningAborted = true;

		if(pGB) pGB->Abort(); // TODO: lock on graph objects somehow, this is not thread safe

		if(nTimeWaited > 5*1000 && m_pGraphThread)
		{
			MessageBeep(MB_ICONEXCLAMATION);
			TRACE(_T("CRITICAL ERROR: !!! Must kill opener thread !!!"));
			TerminateThread(m_pGraphThread->m_hThread, -1);
			m_pGraphThread = (CGraphThread*)AfxBeginThread(RUNTIME_CLASS(CGraphThread));
			s_fOpenedThruThread = false;
			break;
		}

		Sleep(50);

		nTimeWaited += 50;
	}

	m_fOpeningAborted = false;

	m_closingmsg.Empty();

	m_iMediaLoadState = MLS_CLOSING;

	OnFilePostClosemedia();

	if(m_pGraphThread && s_fOpenedThruThread)
	{
		CAMEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_CLOSE, 0, (LPARAM)&e);
		e.Wait(); // either opening or closing has to be blocked to prevent reentering them, closing is the better choice
	}
	else
	{
		CloseMediaPrivate();
	}

	UnloadExternalObjects();
}

//
// CGraphThread
//

IMPLEMENT_DYNCREATE(CGraphThread, CWinThread)

BOOL CGraphThread::InitInstance()
{
	return SUCCEEDED(CoInitialize(0)) ? TRUE : FALSE;
}

int CGraphThread::ExitInstance()
{
	CoUninitialize();
	return __super::ExitInstance();
}

BEGIN_MESSAGE_MAP(CGraphThread, CWinThread)
	ON_THREAD_MESSAGE(TM_EXIT, OnExit)
	ON_THREAD_MESSAGE(TM_OPEN, OnOpen)
	ON_THREAD_MESSAGE(TM_CLOSE, OnClose)
END_MESSAGE_MAP()

void CGraphThread::OnExit(WPARAM wParam, LPARAM lParam)
{
	PostQuitMessage(0);
	if(CAMEvent* e = (CAMEvent*)lParam) e->Set();
}

void CGraphThread::OnOpen(WPARAM wParam, LPARAM lParam)
{
	if(m_pMainFrame)
	{
		CAutoPtr<OpenMediaData> pOMD((OpenMediaData*)lParam);
		m_pMainFrame->OpenMediaPrivate(pOMD);
	}
}

void CGraphThread::OnClose(WPARAM wParam, LPARAM lParam)
{
	if(m_pMainFrame) m_pMainFrame->CloseMediaPrivate();
	if(CAMEvent* e = (CAMEvent*)lParam) e->Set();
}

