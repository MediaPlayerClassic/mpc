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

#include "..\..\ui\TreePropSheet\TreePropSheet.h"
using namespace TreePropSheet;

#include "PPagePlayer.h"
#include "PPageFormats.h"
#include "PPageAccelTbl.h"
#include "PPagePlayback.h"
#include "PPageDVD.h"
#include "PPageRealMediaQuickTime.h"
#include "PPageFilters.h"
#include "PPageSource.h"
#include "PPageAudioSwitcher.h"
#include "PPageSubtitles.h"
#include "PPageSubStyle.h"
#include "PPageTweaks.h"

// CTreePropSheetTreeCtrl

class CTreePropSheetTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CTreePropSheetTreeCtrl)

public:
	CTreePropSheetTreeCtrl();
	virtual ~CTreePropSheetTreeCtrl();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};

// CPPageSheet

class CPPageSheet : public CTreePropSheet
{
	DECLARE_DYNAMIC(CPPageSheet)

private:
	CPPagePlayer m_player;
	CPPageFormats m_formats;
	CPPageAccelTbl m_acceltbl;
	CPPagePlayback m_playback;
	CPPageDVD m_dvd;
	CPPageRealMediaQuickTime m_realmediaquicktime;
	CPPageSubtitles m_subtitles;
	CPPageSubStyle m_substyle;
	CPPageFilters m_filters;
	CPPageSource m_source;
	CPPageAudioSwitcher m_audioswitcher;
	CPPageTweaks m_tweaks;


	CTreeCtrl* CreatePageTreeObject();

public:
	CPPageSheet(LPCTSTR pszCaption, IUnknown* pAudioSwitcher, CWnd* pParentWnd, UINT idPage = 0);
	virtual ~CPPageSheet();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};