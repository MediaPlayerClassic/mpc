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

#pragma once

#include <atlbase.h>

#include "ChildView.h"
#include "PlayerSeekBar.h"
#include "PlayerToolBar.h"
#include "PlayerInfoBar.h"
#include "PlayerStatusBar.h"
#include "PlayerSubresyncBar.h"
#include "PlayerPlaylistBar.h"
#include "PlayerCaptureBar.h"
#include "PPageSheet.h"
#include "PPageFileInfoSheet.h"
#include "OpenCapDeviceDlg.h"

#include "FileDropTarget.h"

#include "KeyProvider.h"

#include "..\..\subpic\ISubPic.h"

#include "RealMediaGraph.h"
#include "QuicktimeGraph.h"
#include "ShockwaveGraph.h"

#include "..\..\..\include\IChapterInfo.h"
#include "..\..\..\include\IKeyFrameInfo.h"

#include "WebServer.h"

enum {PM_NONE, PM_FILE, PM_DVD, PM_CAPTURE};

class OpenMediaData
{
public:
//	OpenMediaData() {}
	virtual ~OpenMediaData() {} // one virtual funct is needed to enable rtti
	CString title;
	CStringList subs;
};

class OpenFileData : public OpenMediaData 
{
public:
	OpenFileData() : rtStart(0) {}
	CStringList fns; 
	REFERENCE_TIME rtStart;
};

class OpenDVDData : public OpenMediaData 
{
public: 
//	OpenDVDData() {}
	CString path; 
	CComPtr<IDvdState> pDvdState;
};

class OpenDeviceData : public OpenMediaData
{
public: 
//	OpenDeviceData() {}
	CStringW DisplayName[2];
};

class CMainFrame;

class CGraphThread : public CWinThread
{
	CMainFrame* m_pMainFrame;

	DECLARE_DYNCREATE(CGraphThread);

public:
	CGraphThread() : m_pMainFrame(NULL) {}

	void SetMainFrame(CMainFrame* pMainFrame) {m_pMainFrame = pMainFrame;}

	BOOL InitInstance();
	int ExitInstance();

	enum {TM_EXIT=WM_APP, TM_OPEN, TM_CLOSE};
	DECLARE_MESSAGE_MAP()
	afx_msg void OnExit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnOpen(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose(WPARAM wParam, LPARAM lParam);
};
/*
class CKeyFrameFinderThread : public CWinThread, public CCritSec
{
	DECLARE_DYNCREATE(CKeyFrameFinderThread);

public:
	CKeyFrameFinderThread() {}
	
	CUIntArray m_kfs; // protected by (CCritSec*)this

	BOOL InitInstance();
	int ExitInstance();

	enum {TM_EXIT=WM_APP, TM_INDEX, TM_BREAK};
	DECLARE_MESSAGE_MAP()
	afx_msg void OnExit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnIndex(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBreak(WPARAM wParam, LPARAM lParam);
};
*/
interface ISubClock;

class CMainFrame : public CFrameWnd, public CDropTarget
{
	enum
	{
		TIMER_STREAMPOSPOLLER = 1, 
		TIMER_STREAMPOSPOLLER2, 
		TIMER_FULLSCREENCONTROLBARHIDER, 
		TIMER_FULLSCREENMOUSEHIDER, 
		TIMER_STATS,
		TIMER_LEFTCLICK,
		TIMER_STATUSERASER
	};

	friend class CPPageFileInfoSheet;
	friend class CPPageLogo;

	// TODO: wrap these graph objects into a class to make it look cleaner

	DWORD m_dwRegister;

	CComPtr<IGraphBuilder> pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaEventEx> pME;
	CComQIPtr<IVideoWindow> pVW;
	CComQIPtr<IBasicVideo> pBV;
	CComQIPtr<IBasicAudio> pBA;
	CComQIPtr<IMediaSeeking> pMS;
	CComQIPtr<IVideoFrameStep> pFS;
	CComQIPtr<IQualProp, &IID_IQualProp> pQP;
	CComQIPtr<IAMOpenProgress> pAMOP;

	CComQIPtr<IDvdControl2> pDVDC;
	CComQIPtr<IDvdInfo2> pDVDI;

	CComQIPtr<IBaseFilter> pVMR;

	CComPtr<ICaptureGraphBuilder2> pCGB;
	CStringW m_VidDispName, m_AudDispName;
	CComPtr<IBaseFilter> pVidCap, pAudCap;
	CComPtr<IAMVideoCompression> pAMVCCap, pAMVCPrev;
	CComPtr<IAMStreamConfig> pAMVSCCap, pAMVSCPrev, pAMASC;
	CComPtr<IAMCrossbar> pAMXBar;
	CComPtr<IAMTVTuner> pAMTuner;
	CComPtr<IAMDroppedFrames> pAMDF;

	CComPtr<ISubPicAllocatorPresenter> m_pCAP;

	void SetBalance(int balance);

	// subtitles

	CCritSec m_csSubLock;
	CInterfaceList<ISubStream> m_pSubStreams;
	int m_iSubtitleSel; // if(m_iSubtitleSel&(1<<31)): disabled
	DWORD_PTR m_nSubtitleId;

	friend class CTextPassThruFilter;

	// windowing

	CRect m_lastWindowRect;
	CPoint m_lastMouseMove;

	void ShowControls(int nCS, bool fSave = true);

	void SetDefaultWindowRect();
	void RestoreDefaultWindowRect();
	void ZoomVideoWindow(double scale = -1);

	void SetAlwaysOnTop(int i);

	// dynamic menus

	void SetupOpenCDSubMenu();
	void SetupFiltersSubMenu();
	void SetupAudioSwitcherSubMenu();
	void SetupSubtitlesSubMenu();
	void SetupNavAudioSubMenu();
	void SetupNavSubtitleSubMenu();
	void SetupNavAngleSubMenu();
	void SetupNavChaptersSubMenu();
	void SetupFavoritesSubMenu();

	void SetupNavOgmSubMenu(CMenu* pSub, UINT id, CString type);
	void OnNavOgmSubMenu(UINT id, CString type);

	CMenu m_popupmain, m_popup;
	CMenu m_opencds;
	CMenu m_filters, m_subtitles, m_audios;
	CAutoPtrArray<CMenu> m_filterpopups;
	CMenu m_navaudio, m_navsubtitle, m_navangle;
	CMenu m_navchapters, m_navtitles;
	CMenu m_favorites;

	CInterfaceArray<ISpecifyPropertyPages> m_spparray;
	CInterfaceArray<IAMStreamSelect> m_ssarray;

	// chapters (file mode)

	typedef struct {REFERENCE_TIME rtStart, rtStop; CString name;} chapter_t;
	CArray<chapter_t> m_chapters;

	//

	void SetupIViAudReg();

	void AddTextPassThruFilter();

	int m_nLoops;

	bool m_fCustomGraph;
	bool m_fRealMediaGraph, m_fShockwaveGraph, m_fQuicktimeGraph;

	CComPtr<ISubClock> m_pSubClock;

	int m_fFrameSteppingActive;
	int m_VolumeBeforeFrameStepping;

	bool m_fEndOfStream;

	bool m_fBuffering;

	bool m_fLiveWM;

	void SendStatusMessage(CString msg, int nTimeOut);
	CString m_playingmsg, m_closingmsg;

	REFERENCE_TIME m_rtDurationOverride;

	CComPtr<IUnknown> m_pProv;

	void CleanGraph();

	CComPtr<IBaseFilter> pAudioDubSrc;

	void ShowOptions(int idPage = 0);

	void SaveImage(LPCTSTR fn);

	//

	friend class CWebServer;
	CAutoPtr<CWebServer> m_pWebServer;

public:
	void StartWebServer(int nPort);
	void StopWebServer();

public:
	CMainFrame();

	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:
	int m_iPlaybackMode;

	bool m_fFullScreen;
	bool m_fHideCursor;

	bool IsFrameLessWindow() {return(m_fFullScreen || AfxGetAppSettings().fHideCaptionMenu);}
	bool IsCaptionMenuHidden() {return(!m_fFullScreen && AfxGetAppSettings().fHideCaptionMenu);}
	bool IsSomethingLoaded() {return(m_iMediaLoadState == MLS_LOADING || m_iMediaLoadState == MLS_LOADED);}
	bool IsPlaylistEmpty() {return(m_wndPlaylistBar.GetCount() == 0);}
	bool IsInteractiveVideo() {return(AfxGetAppSettings().fIntRealMedia && m_fRealMediaGraph || m_fShockwaveGraph);}

	CControlBar* m_pLastBar;

protected: 
	enum {MLS_CLOSED, MLS_LOADING, MLS_LOADED, MLS_CLOSING};
	int m_iMediaLoadState;

	bool m_fAudioOnly;
	dispmode m_dmBeforeFullscreen;

	DVD_DOMAIN m_iDVDDomain;
	int m_iSpeedLevel;

	double m_ZoomX, m_ZoomY, m_PosX, m_PosY;

// Operations
	bool OpenMediaPrivate(CAutoPtr<OpenMediaData> pOMD);
	void CloseMediaPrivate();

	void OpenCreateGraphObject(OpenMediaData* pOMD);
	void OpenFile(OpenFileData* pOFD);
	void OpenDVD(OpenDVDData* pODD);
	void OpenCapture(OpenDeviceData* pODD);
	void OpenCustomizeGraph();
	void OpenSetupVideo();
	void OpenSetupAudio();
	void OpenSetupInfoBar();
	void OpenSetupStatsBar();
	void OpenSetupStatusBar();
	// void OpenSetupToolBar();
	void OpenSetupCaptureBar();
	void OpenSetupWindowTitle(CString fn = _T(""));

	friend class CGraphThread;
	CGraphThread* m_pGraphThread;

	CArray<REFERENCE_TIME> m_kfs;

	bool m_fOpeningAborted;

	CWnd m_owner;

public:
	void OpenCurPlaylistItem(REFERENCE_TIME rtStart = 0);
	void OpenMedia(CAutoPtr<OpenMediaData> pOMD);
	void CloseMedia();

	void ShowTrayIcon(bool fShow);
	void SetTrayTip(CString str);

	CSize GetVideoSize();
	void ToggleFullscreen(bool fToNearest, bool fSwitchScreenResWhenHasTo);
	void MoveVideoWindow(bool fShowStats = false);
	void RepaintVideo();

	OAFilterState GetMediaState();
	REFERENCE_TIME GetPos(), GetDur();
	void SeekTo(REFERENCE_TIME rt, bool fSeekToKeyFrame = false);

	bool LoadSubtitle(CString fn);
	void UpdateSubtitle(bool fApplyDefStyle = false);
	void SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle = false);
	void InvalidateSubtitle(DWORD_PTR nSubtitleId = -1, REFERENCE_TIME rtInvalidate = -1);
	void ReloadSubtitle();

	// capturing
	bool m_fCapturing;
	HRESULT BuildCapture(IPin* pPin, IBaseFilter* pBF[3], const GUID& majortype, AM_MEDIA_TYPE* pmt); // pBF: 0 buff, 1 enc, 2 mux, pmt is for 1 enc
	bool BuildToCapturePreviewPin(
		IBaseFilter* pVidCap, IPin** pVidCapPin, IPin** pVidPrevPin, 
		IBaseFilter* pAudCap, IPin** pAudCapPin, IPin** pAudPrevPin);
	bool BuildGraphVideoAudio(bool fVPreview, bool fVCapture, bool fAPreview, bool fACapture);
	bool DoCapture(), StartCapture(), StopCapture();

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void RecalcLayout(BOOL bNotify = TRUE);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members

	CChildView m_wndView;

	CPlayerSeekBar m_wndSeekBar;
	CPlayerToolBar m_wndToolBar;
	CPlayerInfoBar m_wndInfoBar;
	CPlayerInfoBar m_wndStatsBar;
	CPlayerStatusBar m_wndStatusBar;
	CList<CControlBar*> m_bars;

	CPlayerSubresyncBar m_wndSubresyncBar;
	CPlayerPlaylistBar m_wndPlaylistBar;
	CPlayerCaptureBar m_wndCaptureBar;
	CList<CControlBar*> m_dockingbars;

	CFileDropTarget m_fileDropTarget;
	// TODO
	DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	DROPEFFECT OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
	void OnDragLeave();
	DROPEFFECT OnDragScroll(DWORD dwKeyState, CPoint point);

	friend class CPPagePlayback; // TODO
	friend class CMPlayerCApp; // TODO

	void LoadControlBar(CControlBar* pBar, CString section, UINT defDockBarID);
	void SaveControlBar(CControlBar* pBar, CString section);

// Generated message map functions

	DECLARE_MESSAGE_MAP()

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();

	afx_msg LRESULT OnTaskBarRestart(WPARAM, LPARAM);
	afx_msg LRESULT OnNotifyIcon(WPARAM, LPARAM);

	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDisplayChange();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg LRESULT OnAppCommand(WPARAM wParam, LPARAM lParam);

	afx_msg void OnTimer(UINT nIDEvent);

	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRepaintRenderLess(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResumeFromState(WPARAM wParam, LPARAM lParam);

	BOOL OnButton(UINT id, UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg LRESULT OnXButtonDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXButtonUp(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXButtonDblClk(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg UINT OnNcHitTest(CPoint point);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);

	BOOL OnMenu(CMenu* pMenu);
	afx_msg void OnMenuPlayerShort();
	afx_msg void OnMenuPlayerLong();
	afx_msg void OnMenuFilters();

	afx_msg void OnUpdatePlayerStatus(CCmdUI* pCmdUI);

	afx_msg void OnFilePostOpenmedia();
	afx_msg void OnUpdateFilePostOpenmedia(CCmdUI* pCmdUI);
	afx_msg void OnFilePostClosemedia();
	afx_msg void OnUpdateFilePostClosemedia(CCmdUI* pCmdUI);

	afx_msg void OnBossKey();

	afx_msg void OnStreamAudio(UINT nID);
	afx_msg void OnStreamSub(UINT nID);
	afx_msg void OnStreamSubOnOff();
	afx_msg void OnOgmAudio(UINT nID);
	afx_msg void OnOgmSub(UINT nID);
	afx_msg void OnDvdAngle(UINT nID);
	afx_msg void OnDvdAudio(UINT nID);
	afx_msg void OnDvdSub(UINT nID);
	afx_msg void OnDvdSubOnOff();


	// menu item handlers

	afx_msg void OnFileOpenmedia();
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnFileOpendvd();
	afx_msg void OnFileOpendevice();
	afx_msg void OnFileOpenCD(UINT nID);
	afx_msg void OnDropFiles(HDROP hDropInfo); // no menu item
	afx_msg void OnFileSaveas();
	afx_msg void OnUpdateFileSaveas(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveImage();
	afx_msg void OnFileSaveImageAuto();
	afx_msg void OnUpdateFileSaveImage(CCmdUI *pCmdUI);
	afx_msg void OnFileLoadsubtitles();
	afx_msg void OnUpdateFileLoadsubtitles(CCmdUI* pCmdUI);
	afx_msg void OnFileSavesubtitles();
	afx_msg void OnUpdateFileSavesubtitles(CCmdUI* pCmdUI);
	afx_msg void OnSubtitledatabaseSearch();
	afx_msg void OnUpdateSubtitledatabaseSearch(CCmdUI *pCmdUI);
	afx_msg void OnSubtitledatabaseUpload();
	afx_msg void OnUpdateSubtitledatabaseUpload(CCmdUI *pCmdUI);
	afx_msg void OnFileProperties();
	afx_msg void OnUpdateFileProperties(CCmdUI* pCmdUI);
	afx_msg void OnFileClosePlaylist();
	afx_msg void OnFileCloseMedia(); // no menu item
	afx_msg void OnUpdateFileClose(CCmdUI* pCmdUI);

	afx_msg void OnViewCaptionmenu();
	afx_msg void OnUpdateViewCaptionmenu(CCmdUI *pCmdUI);
	afx_msg void OnViewControlBar(UINT nID);
	afx_msg void OnUpdateViewControlBar(CCmdUI* pCmdUI);
	afx_msg void OnViewSubresync();
	afx_msg void OnUpdateViewSubresync(CCmdUI* pCmdUI);
	afx_msg void OnViewPlaylist();
	afx_msg void OnUpdateViewPlaylist(CCmdUI* pCmdUI);
	afx_msg void OnViewCapture();
	afx_msg void OnUpdateViewCapture(CCmdUI* pCmdUI);
	afx_msg void OnViewMinimal();
	afx_msg void OnUpdateViewMinimal(CCmdUI* pCmdUI);
	afx_msg void OnViewCompact();
	afx_msg void OnUpdateViewCompact(CCmdUI* pCmdUI);
	afx_msg void OnViewNormal();
	afx_msg void OnUpdateViewNormal(CCmdUI* pCmdUI);
	afx_msg void OnViewFullscreen();
	afx_msg void OnViewFullscreenSecondary();
	afx_msg void OnUpdateViewFullscreen(CCmdUI* pCmdUI);
	afx_msg void OnViewZoom(UINT nID);
	afx_msg void OnUpdateViewZoom(CCmdUI* pCmdUI);
	afx_msg void OnViewDefaultVideoFrame(UINT nID);
	afx_msg void OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI);
	afx_msg void OnViewKeepaspectratio();
	afx_msg void OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI);
	afx_msg void OnViewCompMonDeskARDiff();
	afx_msg void OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI);
	afx_msg void OnViewPanNScan(UINT nID);
	afx_msg void OnUpdateViewPanNScan(CCmdUI* pCmdUI);
	afx_msg void OnViewPanNScanPresets(UINT nID);
	afx_msg void OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI);
	afx_msg void OnViewOntop(UINT nID);
	afx_msg void OnUpdateViewOntop(CCmdUI* pCmdUI);
	afx_msg void OnViewOptions();

	afx_msg void OnPlayPlay();
	afx_msg void OnPlayPause();
	afx_msg void OnPlayPlaypause();
	afx_msg void OnPlayStop();
	afx_msg void OnUpdatePlayPauseStop(CCmdUI* pCmdUI);
	afx_msg void OnPlayFramestep(UINT nID);
	afx_msg void OnUpdatePlayFramestep(CCmdUI* pCmdUI);
	afx_msg void OnPlaySeek(UINT nID);
	afx_msg void OnPlaySeekKey(UINT nID); // no menu item
	afx_msg void OnUpdatePlaySeek(CCmdUI* pCmdUI);
	afx_msg void OnPlayGoto();
	afx_msg void OnUpdateGoto(CCmdUI* pCmdUI);
	afx_msg void OnPlayChangeRate(UINT nID);
	afx_msg void OnUpdatePlayChangeRate(CCmdUI* pCmdUI);
	afx_msg void OnPlayChangeAudDelay(UINT nID);
	afx_msg void OnUpdatePlayChangeAudDelay(CCmdUI* pCmdUI);
	afx_msg void OnPlayFilters(UINT nID);
	afx_msg void OnUpdatePlayFilters(CCmdUI* pCmdUI);
	afx_msg void OnPlayAudio(UINT nID);
	afx_msg void OnUpdatePlayAudio(CCmdUI* pCmdUI);
	afx_msg void OnPlaySubtitles(UINT nID);
	afx_msg void OnUpdatePlaySubtitles(CCmdUI* pCmdUI);
	afx_msg void OnPlayLanguage(UINT nID);
	afx_msg void OnUpdatePlayLanguage(CCmdUI* pCmdUI);
	afx_msg void OnPlayVolume(UINT nID);
	afx_msg void OnAfterplaybackClose();
	afx_msg void OnUpdateAfterplaybackClose(CCmdUI *pCmdUI);
	afx_msg void OnAfterplaybackShutdown();
	afx_msg void OnUpdateAfterplaybackShutdown(CCmdUI *pCmdUI);

	afx_msg void OnNavigateSkip(UINT nID);
	afx_msg void OnUpdateNavigateSkip(CCmdUI* pCmdUI);
	afx_msg void OnNavigateSkipPlaylistItem(UINT nID);
	afx_msg void OnUpdateNavigateSkipPlaylistItem(CCmdUI* pCmdUI);
	afx_msg void OnNavigateMenu(UINT nID);
	afx_msg void OnUpdateNavigateMenu(CCmdUI* pCmdUI);
	afx_msg void OnNavigateAudio(UINT nID);
	afx_msg void OnNavigateSubpic(UINT nID);
	afx_msg void OnNavigateAngle(UINT nID);
	afx_msg void OnNavigateChapters(UINT nID);
	afx_msg void OnNavigateMenuItem(UINT nID);
	afx_msg void OnUpdateNavigateMenuItem(CCmdUI* pCmdUI);

	afx_msg void OnFavoritesAdd();
	afx_msg void OnUpdateFavoritesAdd(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesOrganize();
	afx_msg void OnUpdateFavoritesOrganize(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesFile(UINT nID);
	afx_msg void OnUpdateFavoritesFile(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesDVD(UINT nID);
	afx_msg void OnUpdateFavoritesDVD(CCmdUI* pCmdUI);
	afx_msg void OnFavoritesDevice(UINT nID);
	afx_msg void OnUpdateFavoritesDevice(CCmdUI* pCmdUI);

	afx_msg void OnHelpHomepage();
	afx_msg void OnHelpDonate();
public:
	afx_msg void OnClose();
};
