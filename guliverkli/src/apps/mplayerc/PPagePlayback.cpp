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

// PPagePlayback.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPagePlayback.h"
#include "MainFrm.h"
#include "..\..\DSUtil\DSUtil.h"

// CPPagePlayback dialog

IMPLEMENT_DYNAMIC(CPPagePlayback, CPPageBase)
CPPagePlayback::CPPagePlayback()
	: CPPageBase(CPPagePlayback::IDD, CPPagePlayback::IDD)
	, m_iLoopForever(0)
	, m_nLoops(0)
	, m_fRewind(FALSE)
	, m_iZoomLevel(0)
	, m_iRememberZoomLevel(FALSE)
	, m_nVolume(0)
	, m_nBalance(0)
	, m_iVideoRendererType(0)
	, m_iAudioRendererType(0)
	, m_fAutoloadAudio(FALSE)
	, m_fAutoloadSubtitles(FALSE)
	, m_fEnableWorkerThreadForOpening(FALSE)
	, m_fReportFailedPins(FALSE)
{
}

CPPagePlayback::~CPPagePlayback()
{
}

void CPPagePlayback::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER2, m_volumectrl);
	DDX_Control(pDX, IDC_SLIDER3, m_balancectrl);
	DDX_Slider(pDX, IDC_SLIDER2, m_nVolume);
	DDX_Slider(pDX, IDC_SLIDER3, m_nBalance);
	DDX_Radio(pDX, IDC_RADIO1, m_iLoopForever);
	DDX_Control(pDX, IDC_EDIT1, m_loopnumctrl);
	DDX_Text(pDX, IDC_EDIT1, m_nLoops);
	DDX_Check(pDX, IDC_CHECK1, m_fRewind);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iZoomLevel);
	DDX_Check(pDX, IDC_CHECK5, m_iRememberZoomLevel);
	DDX_CBIndex(pDX, IDC_COMBO2, m_iVideoRendererType);
	DDX_Control(pDX, IDC_COMBO2, m_iVideoRendererTypeCtrl);
	DDX_CBIndex(pDX, IDC_COMBO13, m_iAudioRendererType);
	DDX_Control(pDX, IDC_COMBO13, m_iAudioRendererTypeCtrl);
	DDX_Check(pDX, IDC_CHECK2, m_fAutoloadAudio);
	DDX_Check(pDX, IDC_CHECK3, m_fAutoloadSubtitles);
	DDX_Check(pDX, IDC_CHECK9, m_fEnableWorkerThreadForOpening);
	DDX_Check(pDX, IDC_CHECK6, m_fReportFailedPins);
}



BEGIN_MESSAGE_MAP(CPPagePlayback, CPPageBase)
	ON_WM_HSCROLL()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnBnClickedRadio12)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateLoopNum)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateLoopNum)
END_MESSAGE_MAP()


// CPPagePlayback message handlers

BOOL CPPagePlayback::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_volumectrl.SetRange(1, 100);
	m_volumectrl.SetTicFreq(10);
	m_balancectrl.SetRange(0, 200);
	m_balancectrl.SetTicFreq(20);
	m_nVolume = s.nVolume;
	m_nBalance = s.nBalance+100;
	m_iLoopForever = s.fLoopForever?1:0;
	m_nLoops = s.nLoops;
	m_fRewind = s.fRewind;
	m_iZoomLevel = s.iZoomLevel;
	m_iRememberZoomLevel = s.fRememberZoomLevel;

	m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("System Default")), VIDRNDT_DEFAULT);

	if(s.iVideoRendererAvailable&VIDRNDT_OLDRENDERER)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Old Video Renderer")), VIDRNDT_OLDRENDERER);
	if(s.iVideoRendererAvailable&VIDRNDT_OVERLAYMIXER)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Overlay Mixer")), VIDRNDT_OVERLAYMIXER);
	if(s.iVideoRendererAvailable&VIDRNDT_VMR7RENDERLESS)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Video Mixing Renderer 7 (Windowed)")), VIDRNDT_VMR7WINDOWED);
	if(s.iVideoRendererAvailable&VIDRNDT_VMR9RENDERLESS)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Video Mixing Renderer 9 (Windowed)")), VIDRNDT_VMR9WINDOWED);
	if(s.iVideoRendererAvailable&VIDRNDT_VMR7WINDOWED)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Video Mixing Renderer 7 (Renderless)")), VIDRNDT_VMR7RENDERLESS);
	if(s.iVideoRendererAvailable&VIDRNDT_VMR9WINDOWED)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Video Mixing Renderer 9 (Renderless)")), VIDRNDT_VMR9RENDERLESS);
	if(s.iVideoRendererAvailable&VIDRNDT_NULL_COMP)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Null Video Renderer (Any)")), VIDRNDT_NULL_COMP);
	if(s.iVideoRendererAvailable&VIDRNDT_NULL_UNCOMP)
		m_iVideoRendererTypeCtrl.SetItemData(m_iVideoRendererTypeCtrl.AddString(_T("Null Video Renderer (Uncompressed)")), VIDRNDT_NULL_UNCOMP);

	m_iVideoRendererType = 0;
	for(int i = 0; i < m_iVideoRendererTypeCtrl.GetCount(); i++)
	{
		if(m_iVideoRendererTypeCtrl.GetItemData(i) == s.iVideoRendererType)
		{
			m_iVideoRendererType = i;
			break;
		}
	}

	m_AudioRendererDisplayNames.Add(_T(""));
	m_iAudioRendererTypeCtrl.AddString(_T("System Default"));
	m_iAudioRendererType = 0;

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
	{
		LPOLESTR olestr = NULL;
		if(FAILED(pMoniker->GetDisplayName(0, 0, &olestr)))
			continue;
		CStringW str(olestr);
		CoTaskMemFree(olestr);
		m_AudioRendererDisplayNames.Add(CString(str));

		CComPtr<IPropertyBag> pPB;
		if(SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
		{
			CComVariant var;
			pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);

			CString fstr(var.bstrVal);

			var.Clear();
			if(SUCCEEDED(pPB->Read(CComBSTR(_T("FilterData")), &var, NULL)))
			{			
				BSTR* pbstr;
				if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pbstr)))
				{
					fstr.Format(_T("%s (%08x)"), CString(fstr), *((DWORD*)pbstr + 1));
					SafeArrayUnaccessData(var.parray);
				}
			}

			m_iAudioRendererTypeCtrl.AddString(fstr);
		}
		else
		{
			m_iAudioRendererTypeCtrl.AddString(CString(str));
		}

		if(s.AudioRendererDisplayName == str && m_iAudioRendererType == 0)
		{
			m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;
		}
	}
	EndEnumSysDev

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	m_iAudioRendererTypeCtrl.AddString(AUDRNDT_NULL_COMP);
	if(s.AudioRendererDisplayName == AUDRNDT_NULL_COMP && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);
	m_iAudioRendererTypeCtrl.AddString(AUDRNDT_NULL_UNCOMP);
	if(s.AudioRendererDisplayName == AUDRNDT_NULL_UNCOMP && m_iAudioRendererType == 0)
		m_iAudioRendererType = m_iAudioRendererTypeCtrl.GetCount()-1;

	CorrectComboListWidth(m_iVideoRendererTypeCtrl, GetFont());
	CorrectComboListWidth(m_iAudioRendererTypeCtrl, GetFont());

	m_fAutoloadAudio = s.fAutoloadAudio;
	m_fAutoloadSubtitles = s.fAutoloadSubtitles;
	m_fEnableWorkerThreadForOpening = s.fEnableWorkerThreadForOpening;
	m_fReportFailedPins = s.fReportFailedPins;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPagePlayback::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.nVolume = m_nVolume;
	s.nBalance = m_nBalance-100;
	s.fLoopForever = !!m_iLoopForever;
	s.nLoops = m_nLoops;
	s.fRewind = !!m_fRewind;
	s.iZoomLevel = m_iZoomLevel;
	s.fRememberZoomLevel = !!m_iRememberZoomLevel;
	s.iVideoRendererType = m_iVideoRendererType >= 0 ? m_iVideoRendererTypeCtrl.GetItemData(m_iVideoRendererType) : 0;
	s.AudioRendererDisplayName = m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.fAutoloadAudio = !!m_fAutoloadAudio;
	s.fAutoloadSubtitles = !!m_fAutoloadSubtitles;
	s.fEnableWorkerThreadForOpening = !!m_fEnableWorkerThreadForOpening;
	s.fReportFailedPins = !!m_fReportFailedPins;

	return __super::OnApply();
}

LRESULT CPPagePlayback::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_HSCROLL || message == WM_VSCROLL)
	{
		SetModified();
	}

	return __super::DefWindowProc(message, wParam, lParam);
}

void CPPagePlayback::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(*pScrollBar == m_volumectrl)
	{
		UpdateData();
		((CMainFrame*)GetParentFrame())->m_wndToolBar.Volume = m_nVolume; // nice shortcut...
	}
	else if(*pScrollBar == m_balancectrl)
	{
		UpdateData();
		((CMainFrame*)GetParentFrame())->SetBalance(m_nBalance-100); // see prev note...
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPagePlayback::OnBnClickedRadio12(UINT nID)
{
	SetModified();
}

void CPPagePlayback::OnUpdateLoopNum(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_iLoopForever == 0);
}
