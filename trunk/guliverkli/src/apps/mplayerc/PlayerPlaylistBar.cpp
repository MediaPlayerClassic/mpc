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

#include "stdafx.h"
#include <math.h>
#include <afxinet.h>
#include <atlrx.h>
#include <atlutil.h>
#include "mplayerc.h"
#include "mainfrm.h"
#include "playerplaylistbar.h"
#include "..\..\DSUtil\DSUtil.h"
#include "SaveTextFileDialog.h"
#include ".\playerplaylistbar.h"

IMPLEMENT_DYNAMIC(CPlayerPlaylistBar, CSizingControlBarG)
CPlayerPlaylistBar::CPlayerPlaylistBar()
	: m_list(0)
	, m_nTimeColWidth(0)
{
	m_bDragging = FALSE;
}

CPlayerPlaylistBar::~CPlayerPlaylistBar()
{
}

BOOL CPlayerPlaylistBar::Create(CWnd* pParentWnd)
{
	if(!CSizingControlBarG::Create(_T("Playlist"), pParentWnd, 50))
		return FALSE;

	m_list.CreateEx(
		WS_EX_DLGMODALFRAME|WS_EX_CLIENTEDGE, 
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_TABSTOP
			|LVS_OWNERDRAWFIXED
			|LVS_NOCOLUMNHEADER
			|LVS_REPORT|LVS_SINGLESEL|LVS_AUTOARRANGE|LVS_NOSORTHEADER, // TODO: remove LVS_SINGLESEL and implement multiple item repositioning (dragging is ready)
		CRect(0,0,100,100), this, IDC_PLAYLIST);

	m_list.SetExtendedStyle(m_list.GetExtendedStyle()|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);

	m_list.InsertColumn(COL_NAME, _T("Name"), LVCFMT_LEFT, 380);

	CDC* pDC = m_list.GetDC();
	CFont* old = pDC->SelectObject(GetFont());
	m_nTimeColWidth = pDC->GetTextExtent(_T("000:00:00")).cx + 5;
	pDC->SelectObject(old);
	m_list.ReleaseDC(pDC);
	m_list.InsertColumn(COL_TIME, _T("Time"), LVCFMT_RIGHT, m_nTimeColWidth);

    m_fakeImageList.Create(1, 16, ILC_COLOR4, 10, 10);
	m_list.SetImageList(&m_fakeImageList, LVSIL_SMALL);

	return TRUE;
}

BOOL CPlayerPlaylistBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CSizingControlBarG::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_ACCEPTFILES;

	return TRUE;
}

BOOL CPlayerPlaylistBar::PreTranslateMessage(MSG* pMsg)
{
	if(IsWindow(pMsg->hwnd) && IsVisible() && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		if(IsDialogMessage(pMsg))
			return TRUE;
	}

	return CSizingControlBarG::PreTranslateMessage(pMsg);
}

bool FindFileInList(CStringList& sl, CString fn)
{
	bool fFound = false;
	POSITION pos = sl.GetHeadPosition();
	while(pos && !fFound) {if(!sl.GetNext(pos).CompareNoCase(fn)) fFound = true;}
	return(fFound);
}

void CPlayerPlaylistBar::AddItem(CString fn, CStringList* subs)
{
	CStringList sl;
	sl.AddTail(fn);
	AddItem(sl, subs);
}

void CPlayerPlaylistBar::AddItem(CStringList& fns, CStringList* subs)
{
	CPlaylistItem pli;

	POSITION pos = fns.GetHeadPosition();
	while(pos)
	{
		CString fn = fns.GetNext(pos);
		if(!fn.Trim().IsEmpty()) pli.m_fns.AddTail(fn);
	}

	if(subs)
	{
		POSITION pos = subs->GetHeadPosition();
		while(pos)
		{
			CString fn = subs->GetNext(pos);
			if(!fn.Trim().IsEmpty()) pli.m_subs.AddTail(fn);
		}
	}

	if(pli.m_fns.IsEmpty()) return;

	CString fn = pli.m_fns.GetHead();

	if(AfxGetAppSettings().fAutoloadAudio && fn.Find(_T("://")) < 0)
	{
		int i = fn.ReverseFind('.');
		if(i > 0)
		{
			CMediaFormats& mf = AfxGetAppSettings().Formats;

			CString ext = fn.Mid(i+1).MakeLower();

			if(!mf.FindExt(ext, true))
			{
				CString path = fn;
				path.Replace('/', '\\');
				path = path.Left(path.ReverseFind('\\')+1);

				WIN32_FIND_DATA fd = {0};
				HANDLE hFind = FindFirstFile(fn.Left(i) + _T("*.*"), &fd);
				if(hFind != INVALID_HANDLE_VALUE)
				{
					do
					{
						if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;

						CString fullpath = path + fd.cFileName;
						CString ext2 = fullpath.Mid(fullpath.ReverseFind('.')+1).MakeLower();
						if(!FindFileInList(pli.m_fns, fullpath) && ext != ext2 
						&& mf.FindExt(ext2, true) && mf.IsUsingEngine(fullpath, DirectShow))
						{
							pli.m_fns.AddTail(fullpath);
						}
					}
					while(FindNextFile(hFind, &fd));
					
					FindClose(hFind);
				}
			}
		}
	}

	if(AfxGetAppSettings().fAutoloadSubtitles)
	{
		CStringArray paths;
		paths.Add(_T("."));
		paths.Add(_T(".\\subtitles"));
		paths.Add(_T("c:\\subtitles"));

		SubFiles ret;
		GetSubFileNames(fn, paths, ret);

		for(int i = 0; i < ret.GetCount(); i++)
		{
			if(!FindFileInList(pli.m_subs, ret[i].fn))
				pli.m_subs.AddTail(ret[i].fn);
		}
	}

	m_pl.AddTail(pli);
}

static bool SearchFiles(CString mask, CStringList& sl)
{
	if(mask.Find(_T("://")) >= 0) 
		return(false);

	mask.Trim();
	sl.RemoveAll();

	CMediaFormats& mf = AfxGetAppSettings().Formats;

	bool fFilterKnownExts;
	WIN32_FILE_ATTRIBUTE_DATA fad;
	mask = (fFilterKnownExts = (GetFileAttributesEx(mask, GetFileExInfoStandard, &fad) 
							&& (fad.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)))
		? CString(mask).TrimRight(_T("\\/")) + _T("\\*.*")
		: mask;

	{
		CString dir = mask.Left(max(mask.ReverseFind('\\'), mask.ReverseFind('/'))+1);

		WIN32_FIND_DATA fd;
		HANDLE h = FindFirstFile(mask, &fd);
		if(h != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;

				CString fn = fd.cFileName;
				CString ext = fn.Mid(fn.ReverseFind('.')+1).MakeLower();
				CString path = dir + fd.cFileName;

				if(!fFilterKnownExts || mf.FindExt(ext))
					sl.AddTail(path);
			}
			while(FindNextFile(h, &fd));
			
			FindClose(h);

			if(sl.GetCount() == 0 && mask.Find(_T(":\\")) == 1)
			{
				GetCDROMType(mask[0], sl);
			}
		}
	}

	return(sl.GetCount() > 1
		|| sl.GetCount() == 1 && sl.GetHead().CompareNoCase(mask)
		|| sl.GetCount() == 0 && mask.FindOneOf(_T("?*")) >= 0);
}

void CPlayerPlaylistBar::ParsePlayList(CString fn, CStringList* subs)
{
	CStringList sl;
	sl.AddTail(fn);
	ParsePlayList(sl, subs);
}

void CPlayerPlaylistBar::ParsePlayList(CStringList& fns, CStringList* subs)
{
	if(fns.IsEmpty()) return;

	CString fn = fns.GetHead();

	CStringList sl;
	if(SearchFiles(fn, sl))
	{
		if(sl.GetCount() > 1) subs = NULL;
		POSITION pos = sl.GetHeadPosition();
		while(pos) ParsePlayList(sl.GetNext(pos), subs);
		return;
	}

	CString ext = fn.Mid(fn.ReverseFind('.')+1).MakeLower();
	CString dir = fn.Left(max(fn.ReverseFind('/'), fn.ReverseFind('\\'))+1); // "ReverseFindOneOf"

	if(CString(fn).MakeLower().Find(_T("http://")) >= 0)
	{
		if(!(ext == _T("pls") || ext == _T("m3u") || ext == _T("asx") /*|| ext == _T("asf")*/))
		{
			CUrl url;
			url.CrackUrl(fn);
			if(url.GetPortNumber() == ATL_URL_SCHEME_UNKNOWN) url.SetPortNumber(ATL_URL_SCHEME_HTTP);

			CStringA str;
			str.Format(
				"GET %s HTTP/1.0\r\n"
				"User-Agent: Media Player Classic\r\n"
				"Host: %s\r\n"
				"Accept: */*\r\n"
				"Connection: Keep-Alive\r\n"
				"\r\n", CStringA(url.GetUrlPath())+CStringA(url.GetExtraInfo()), CStringA(url.GetHostName()));

			float ver = 0;
			CStringA contenttype;

			CSocket s;
			if(s.Create() && s.Connect(url.GetHostName(), url.GetPortNumber())
			&& s.Send((BYTE*)(LPCSTR)str, str.GetLength()) > 0)
			{
				str.Empty();
				BYTE cur = 0, prev = 0;
				while(s.Receive(&cur, 1) == 1 && cur && !(cur == '\n' && prev == '\n'))
				{
					if(cur == '\r')
						continue;

					if(cur == '\n')
					{
						str.MakeLower();
						str.Trim();
						if(1 == sscanf(str, "http/%f 200 ok", &ver)) ver = ver;
						else if(str.Find("content-type:") == 0) contenttype = str.Mid(13).Trim();
						str.Empty();
					}
					else
					{
						str += cur;
					}

					prev = cur;
					cur = 0;
				}
			}

			if(ver != 0)
			{
				if(contenttype == "video/x-ms-asf")
					ext = _T("asx");
				else if(contenttype == "audio/x-mpegurl")
					ext = _T("m3u");
			}
		}
	}
/*
	if(CString(fn).MakeLower().Find(_T("http://")) >= 0 
	|| (fn.Find(_T("://")) < 0 && (ext == _T("pls") || ext == _T("m3u") || ext == _T("asx") || ext == _T("asf"))))
	{
		CComPtr<IGraphBuilder> pGB;
		pGB.CoCreateInstance(CLSID_FilterGraph);
		if(!pGB) return;

		CComPtr<IBaseFilter> pBF;
		if(SUCCEEDED(pGB->AddSourceFilter(CStringW(fn), NULL, &pBF)) && pBF)
		{
			IPin* pPin = GetFirstPin(pBF, PINDIR_OUTPUT);
			BeginEnumMediaTypes(pPin, pEMT, pmt)
			{
				CString subtype = CStringFromGUID(pmt->subtype);

				if(!subtype.CompareNoCase(_T("{D51BD5AE-7548-11CF-A520-0080C77EF58A}")))
				{
					ext = _T("asx");
					break;
				}
				else if(!subtype.CompareNoCase(_T("{A98C8400-4181-11D1-A520-00A0D10129C0}")))
				{
					ext = _T("m3u");
					break;
				}
			}
			EndEnumMediaTypes(pmt)
		}
	}
*/
	CAutoPtrList<CAtlRegExp<> > res;
	CAutoPtr<CAtlRegExp<> > re;

	if(ext == _T("pls"))
	{
		// File1=...\n
		re.Attach(new CAtlRegExp<>());
		if(re && REPARSE_ERROR_OK == re->Parse(_T("file\\z\\b*=\\b*[\"]*{[^\n\"]+}"), FALSE))
			res.AddTail(re);
	}
	else if(ext == _T("m3u"))
	{
		// #comment
		// ...
		re.Attach(new CAtlRegExp<>());
		if(re && REPARSE_ERROR_OK == re->Parse(_T("(^|\n){[^#][^\n]+}"), FALSE))
			res.AddTail(re);
	}
	else if(ext == _T("asx") || ext == _T("asf") || ext == _T("wmx") || ext == _T("wvx") || ext == _T("wax"))
	{
		// <Ref href = "..."/>
		re.Attach(new CAtlRegExp<>());
		if(re && REPARSE_ERROR_OK == re->Parse(_T("<[ \\t\n]*Ref[ \\t\n]+href[ \\t\n]*=[ \\t\n\"]*{[^\">]*}"), FALSE))
			res.AddTail(re);
		// Ref#n= ...\n
		re.Attach(new CAtlRegExp<>());
		if(re && REPARSE_ERROR_OK == re->Parse(_T("Ref\\z\\b*=\\b*[\"]*{[^\n\"]+}"), FALSE))
			res.AddTail(re);
	}
	else
	{
		AddItem(fns, subs);
		return;
	}

	CString str('\n'); // FIXME: m3u regexp skips the first line for some reason

	CWebTextFile f;
	if(f.Open(fn)) 
		for(CString tmp; f.ReadString(tmp); str += tmp + '\n');

	bool fFound = false;

	POSITION pos = res.GetHeadPosition();
	while(pos)
	{
		CAtlRegExp<>* re = res.GetNext(pos);

		CAtlREMatchContext<> mc;
		const CAtlREMatchContext<>::RECHAR* s = str.GetBuffer();
		const CAtlREMatchContext<>::RECHAR* e = NULL;
		for(; s && re->Match(s, &mc, &e); s = e)
		{
			fFound = true;

			const CAtlREMatchContext<>::RECHAR* szStart = 0;
			const CAtlREMatchContext<>::RECHAR* szEnd = 0;
			mc.GetMatch(0, &szStart, &szEnd);

			fn.Format(_T("%.*s"), szEnd - szStart, szStart);
			fn.Trim();
			if(fn.CompareNoCase(_T("asf path")) == 0) continue;
			if(fn.Find(_T(":")) < 0 && fn.Find(_T("\\\\")) != 0 && fn.Find(_T("//")) != 0)
				fn = dir + fn;

			ParsePlayList(fn, NULL);
		}
	}

	if(!fFound)
		AddItem(fns, subs);
}

void CPlayerPlaylistBar::Empty()
{
	m_pl.RemoveAll();
	m_list.DeleteAllItems();
}

void CPlayerPlaylistBar::Open(CStringList& fns, bool fMulti, CStringList* subs)
{
	Empty();
	Append(fns, fMulti, subs);
}

void CPlayerPlaylistBar::Append(CStringList& fns, bool fMulti, CStringList* subs)
{
	if(fMulti)
	{
		ASSERT(subs == NULL || subs->GetCount() == 0);
		POSITION pos = fns.GetHeadPosition();
		while(pos) ParsePlayList(fns.GetNext(pos), NULL);
	}
	else
	{
		ParsePlayList(fns, subs);
	}

	SetupList();
	ResizeListColumn();
}

void CPlayerPlaylistBar::SetupList()
{
	m_list.DeleteAllItems();

	POSITION pos = m_pl.GetHeadPosition();
	for(int i = 0; pos; i++)
	{
		CPlaylistItem& pli = m_pl.GetAt(pos);
		m_list.SetItemData(m_list.InsertItem(i, StripPath(pli.m_fns.GetHead())), (DWORD_PTR)pos);
		if(pli.m_fInvalid) m_list.SetItemText(i, COL_TIME, _T("Invalid"));
		else SetTime(i, pli.m_duration);
		m_pl.GetNext(pos);
	}
}

void CPlayerPlaylistBar::EnsureVisible(POSITION pos)
{
	int i = FindItem(m_pl.GetPos());
	if(i < 0) return;
	m_list.EnsureVisible(i, TRUE);
	m_list.Invalidate();
}

int CPlayerPlaylistBar::FindItem(POSITION pos)
{
	for(int i = 0; i < m_list.GetItemCount(); i++)
		if((POSITION)m_list.GetItemData(i) == pos)
			return(i);
	return(-1);
}

POSITION CPlayerPlaylistBar::FindPos(int i)
{
	if(i < 0) return(NULL);
	return((POSITION)m_list.GetItemData(i));
}

int CPlayerPlaylistBar::GetCount()
{
	return(m_pl.GetCount()); // TODO: n - .fInvalid
}

int CPlayerPlaylistBar::GetSelIdx()
{
	return(FindItem(m_pl.GetPos()));
}

void CPlayerPlaylistBar::SetSelIdx(int i)
{
	m_pl.SetPos(FindPos(i));
}

bool CPlayerPlaylistBar::IsAtEnd()
{
	return(m_pl.GetPos() && m_pl.GetPos() == m_pl.GetTailPosition());
}

bool CPlayerPlaylistBar::GetCur(CPlaylistItem& pli)
{
	if(!m_pl.GetPos()) return(false);
	pli = m_pl.GetAt(m_pl.GetPos());
	return(true);
}

CString CPlayerPlaylistBar::GetCur()
{
	CString fn;
	CPlaylistItem pli;
	if(GetCur(pli) && !pli.m_fns.IsEmpty()) fn = pli.m_fns.GetHead();
	return(fn);
}

void CPlayerPlaylistBar::SetNext()
{
	POSITION pos = m_pl.GetPos(), org = pos;
	while(m_pl.GetNextWrap(pos).m_fInvalid && pos != org);
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetPrev()
{
	POSITION pos = m_pl.GetPos(), org = pos;
	while(m_pl.GetPrevWrap(pos).m_fInvalid && pos != org);
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetFirst()
{
	POSITION pos = m_pl.GetTailPosition(), org = pos;
	while(m_pl.GetNextWrap(pos).m_fInvalid && pos != org);
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetLast()
{
	POSITION pos = m_pl.GetHeadPosition(), org = pos;
	while(m_pl.GetPrevWrap(pos).m_fInvalid && pos != org);
	m_pl.SetPos(pos);
	EnsureVisible(pos);
}

void CPlayerPlaylistBar::SetCurValid(bool fValid)
{
	if(POSITION pos = m_pl.GetPos())
	{
		if(m_pl.GetAt(pos).m_fInvalid = !fValid)
		{
			int i = FindItem(pos);
			m_list.RedrawItems(i, i);
		}
	}
}

CString CPlayerPlaylistBar::StripPath(CString path)
{
	CString p = path;
	p.Replace('\\', '/');
	p = p.Mid(p.ReverseFind('/')+1);
	return(p.IsEmpty() ? path : p);
}

void CPlayerPlaylistBar::SetTime(int i, REFERENCE_TIME rt)
{
	if(i < 0) return;

	CString t;

	if(rt > 0)
	{
		int hh = int(rt/10000000/60/60);
		int mm = int((rt/10000000/60)%60);
		int ss = int((rt/10000000)%60);
		t.Format(_T("%02d:%02d:%02d"), hh, mm, ss);
	}

	m_list.SetItemText(i, COL_TIME, t);
}

void CPlayerPlaylistBar::SetCurTime(REFERENCE_TIME rt)
{
	if(POSITION pos = m_pl.GetPos())
	{
		m_pl.GetAt(pos).m_duration = rt;
		SetTime(FindItem(pos), rt);
	}
}

BEGIN_MESSAGE_MAP(CPlayerPlaylistBar, CSizingControlBarG)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_KEYDOWN, IDC_PLAYLIST, OnLvnKeyDown)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_PLAYLIST, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_PLAYLIST, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_PLAYLIST, OnEndlabeleditList)
	ON_NOTIFY(NM_DBLCLK, IDC_PLAYLIST, OnNMDblclkList)
//	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PLAYLIST, OnCustomdrawList)
	ON_WM_DRAWITEM()
	ON_COMMAND_EX(ID_FILE_CLOSEPLAYLIST, OnFileClosePlaylist)
	ON_COMMAND_EX(ID_PLAY_PLAY, OnPlayPlay)
	ON_WM_DROPFILES()
	ON_NOTIFY(LVN_BEGINDRAG, IDC_PLAYLIST, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


// CPlayerPlaylistBar message handlers

void CPlayerPlaylistBar::ResizeListColumn()
{
	if(::IsWindow(m_list.m_hWnd))
	{
		CRect r;
		GetClientRect(r);
		r.DeflateRect(2, 2);
		m_list.SetRedraw(FALSE);
		m_list.MoveWindow(r);
		m_list.GetClientRect(r);
		m_list.SetColumnWidth(COL_NAME, r.Width()-m_nTimeColWidth); //LVSCW_AUTOSIZE_USEHEADER
		m_list.SetRedraw(TRUE);
	}
}

void CPlayerPlaylistBar::OnSize(UINT nType, int cx, int cy)
{
	CSizingControlBarG::OnSize(nType, cx, cy);

	ResizeListColumn();
}

void CPlayerPlaylistBar::OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	*pResult = FALSE;

	CList<int> items;
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while(pos) items.AddHead(m_list.GetNextSelectedItem(pos));

	if(pLVKeyDown->wVKey == VK_DELETE && items.GetCount() > 0) 
	{
		pos = items.GetHeadPosition();
		while(pos) 
		{
			int i = items.GetNext(pos);
			if(m_pl.RemoveAt(FindPos(i))) ((CMainFrame*)AfxGetMainWnd())->CloseMedia();
			m_list.DeleteItem(i);
		}

		m_list.SetItemState(-1, 0, LVIS_SELECTED);
		m_list.SetItemState(
			max(min(items.GetTail(), m_list.GetItemCount()-1), 0), 
			LVIS_SELECTED, LVIS_SELECTED);

		ResizeListColumn();

		*pResult = TRUE;
	}
	else if(pLVKeyDown->wVKey == VK_SPACE && items.GetCount() == 1) 
	{
		m_pl.SetPos(FindPos(items.GetHead()));

		((CMainFrame*)AfxGetMainWnd())->OpenCurPlaylistItem();

		*pResult = TRUE;
	}
}

void CPlayerPlaylistBar::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if(pItem->iItem >= 0)
	{
	}
}

void CPlayerPlaylistBar::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if(pItem->iItem >= 0)
	{
	}
}

void CPlayerPlaylistBar::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if(!m_list.m_fInPlaceDirty)
		return;

	if(pItem->iItem >= 0)
	{
	}
}

void CPlayerPlaylistBar::OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;

	if(lpnmlv->iItem >= 0 && lpnmlv->iSubItem >= 0)
	{
		m_pl.SetPos(FindPos(lpnmlv->iItem));
		m_list.Invalidate();
		((CMainFrame*)AfxGetMainWnd())->OpenCurPlaylistItem();
	}

	*pResult = 0;
}
/*
void CPlayerPlaylistBar::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = CDRF_DODEFAULT;

	if(CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYPOSTPAINT|CDRF_NOTIFYITEMDRAW;
	}
	else if(CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		pLVCD->nmcd.uItemState &= ~CDIS_SELECTED;
		pLVCD->nmcd.uItemState &= ~CDIS_FOCUS;

		pLVCD->clrText = (pLVCD->nmcd.dwItemSpec == m_playList.m_idx) ? 0x0000ff : CLR_DEFAULT;
		pLVCD->clrTextBk = m_list.GetItemState(pLVCD->nmcd.dwItemSpec, LVIS_SELECTED) ? 0xf1dacc : CLR_DEFAULT;

		*pResult = CDRF_NOTIFYPOSTPAINT;
	}
	else if(CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage)
	{
        int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

		if(m_list.GetItemState(pLVCD->nmcd.dwItemSpec, LVIS_SELECTED))
		{
			CRect r, r2;
			m_list.GetItemRect(nItem, &r, LVIR_BOUNDS);
			m_list.GetItemRect(nItem, &r2, LVIR_LABEL);
			r.left = r2.left;
			FrameRect(pLVCD->nmcd.hdc, &r, CBrush(0xc56a31));
		}

		*pResult = CDRF_SKIPDEFAULT;
	}
	else if(CDDS_POSTPAINT == pLVCD->nmcd.dwDrawStage)
	{
	}
}
*/

void CPlayerPlaylistBar::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if(nIDCtl != IDC_PLAYLIST) return;

	int nItem = lpDrawItemStruct->itemID;
	CRect rcItem = lpDrawItemStruct->rcItem;
	POSITION pos = FindPos(nItem);
	bool fSelected = pos == m_pl.GetPos();
	CPlaylistItem& pli = m_pl.GetAt(pos);

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	if(!!m_list.GetItemState(nItem, LVIS_SELECTED))
	{
		FillRect(pDC->m_hDC, rcItem, CBrush(0xf1dacc));
		FrameRect(pDC->m_hDC, rcItem, CBrush(0xc56a31));
	}

	COLORREF textcolor = fSelected?0xff:0;
	if(pli.m_fInvalid) textcolor |= 0xA0A0A0;

	CString time = !pli.m_fInvalid ? m_list.GetItemText(nItem, COL_TIME) : _T("Invalid");
	CSize timesize(0, 0);
	CPoint timept(rcItem.right, 0);
	if(time.GetLength() > 0)
	{
		timesize = pDC->GetTextExtent(time);
		if((3+timesize.cx+3) < rcItem.Width()/2)
		{
			timept = CPoint(rcItem.right-(3+timesize.cx+3), (rcItem.top+rcItem.bottom-timesize.cy)/2);

			pDC->SetTextColor(textcolor);
			pDC->TextOut(timept.x, timept.y, time);
		}
	}

	CString fmt, file;
	fmt.Format(_T("%%0%dd. %%s"), (int)log10(0.1+m_pl.GetCount())+1);
	file.Format(fmt, nItem+1, m_list.GetItemText(nItem, COL_NAME));
	CSize filesize = pDC->GetTextExtent(file);
	while(3+filesize.cx+6 > timept.x && file.GetLength() > 3)
	{
		file = file.Left(file.GetLength()-4) + _T("...");
		filesize = pDC->GetTextExtent(file);
	}

	if(file.GetLength() > 3)
	{
		pDC->SetTextColor(textcolor);
		pDC->TextOut(rcItem.left+3, (rcItem.top+rcItem.bottom-filesize.cy)/2, file);
	}
}

BOOL CPlayerPlaylistBar::OnFileClosePlaylist(UINT nID)
{
	Empty();
	return FALSE;
}

BOOL CPlayerPlaylistBar::OnPlayPlay(UINT nID)
{
	m_list.Invalidate();
	return FALSE;
}

void CPlayerPlaylistBar::OnDropFiles(HDROP hDropInfo)
{
	SetActiveWindow();

	CStringList sl;

	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	for(UINT iFile = 0; iFile < nFiles; iFile++)
	{
		TCHAR szFileName[_MAX_PATH];
		::DragQueryFile(hDropInfo, iFile, szFileName, _MAX_PATH);
		sl.AddTail(szFileName);
	}
	::DragFinish(hDropInfo);

	Append(sl, true);
}

void CPlayerPlaylistBar::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	ModifyStyle(WS_EX_ACCEPTFILES, 0);

	m_nDragIndex = ((LPNMLISTVIEW)pNMHDR)->iItem;

	CPoint p(0, 0);
	m_pDragImage = m_list.CreateDragImageEx(&p);

	CPoint p2 = ((LPNMLISTVIEW)pNMHDR)->ptAction;

	m_pDragImage->BeginDrag(0, p2 - p);
	m_pDragImage->DragEnter(GetDesktopWindow(), ((LPNMLISTVIEW)pNMHDR)->ptAction);

	m_bDragging = TRUE;
	m_nDropIndex = -1;

	SetCapture();
}

void CPlayerPlaylistBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bDragging)
	{
		m_ptDropPoint = point;
		ClientToScreen(&m_ptDropPoint);
		
		m_pDragImage->DragMove(m_ptDropPoint);
		m_pDragImage->DragShowNolock(FALSE);

		WindowFromPoint(m_ptDropPoint)->ScreenToClient(&m_ptDropPoint);
		
		m_pDragImage->DragShowNolock(TRUE);

		{
			int iOverItem = m_list.HitTest(m_ptDropPoint);
			int iTopItem = m_list.GetTopIndex();
			int iBottomItem = m_list.GetBottomIndex();

			if(iOverItem == iTopItem && iTopItem != 0) // top of list
				SetTimer(1, 100, NULL); 
			else
				KillTimer(1); 

			if(iOverItem >= iBottomItem && iBottomItem != (m_list.GetItemCount() - 1)) // bottom of list
				SetTimer(2, 100, NULL); 
			else 
				KillTimer(2); 
		}
	}

	__super::OnMouseMove(nFlags, point);
}

void CPlayerPlaylistBar::OnTimer(UINT nIDEvent)
{
	int iTopItem = m_list.GetTopIndex();
	int iBottomItem = iTopItem + m_list.GetCountPerPage() - 1;

	if(m_bDragging)
	{
		m_pDragImage->DragShowNolock(FALSE);

		if(nIDEvent == 1)
		{
			m_list.EnsureVisible(iTopItem - 1, false);
			m_list.UpdateWindow();
			if(m_list.GetTopIndex() == 0) KillTimer(1); 
		}
		else if(nIDEvent == 2)
		{
			m_list.EnsureVisible(iBottomItem + 1, false);
			m_list.UpdateWindow();
			if(m_list.GetBottomIndex() == (m_list.GetItemCount() - 1)) KillTimer(2); 
		} 

		m_pDragImage->DragShowNolock(TRUE);
	}

	__super::OnTimer(nIDEvent);
}

void CPlayerPlaylistBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(m_bDragging)
	{
		::ReleaseCapture();

		m_bDragging = FALSE;
		m_pDragImage->DragLeave(GetDesktopWindow());
		m_pDragImage->EndDrag();

		delete m_pDragImage;
		m_pDragImage = NULL;

		KillTimer(1);
		KillTimer(2);

		CPoint pt(point);
		ClientToScreen(&pt);

		if(WindowFromPoint(pt) == &m_list)
			DropItemOnList();
	}

	ModifyStyle(0, WS_EX_ACCEPTFILES); 

	__super::OnLButtonUp(nFlags, point);
}

void CPlayerPlaylistBar::DropItemOnList()
{
	m_ptDropPoint.y += 10; //
	m_nDropIndex = m_list.HitTest(CPoint(10, m_ptDropPoint.y));

	TCHAR szLabel[MAX_PATH];
	LV_ITEM lvi;
	ZeroMemory(&lvi, sizeof(LV_ITEM));
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.stateMask = LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
	lvi.pszText = szLabel;
	lvi.iItem = m_nDragIndex;
	lvi.cchTextMax = MAX_PATH;
	m_list.GetItem(&lvi);

	if(m_nDropIndex < 0) m_nDropIndex = m_list.GetItemCount();
	lvi.iItem = m_nDropIndex;
	m_list.InsertItem(&lvi);

	CHeaderCtrl* pHeader = (CHeaderCtrl*)m_list.GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	lvi.mask = LVIF_TEXT;
	lvi.iItem = m_nDropIndex;
	//INDEX OF DRAGGED ITEM WILL CHANGE IF ITEM IS DROPPED ABOVE ITSELF
	if(m_nDropIndex < m_nDragIndex) m_nDragIndex++;
	for(int col=1; col < nColumnCount; col++)
	{
		_tcscpy(lvi.pszText, (LPCTSTR)(m_list.GetItemText(m_nDragIndex, col)));
		lvi.iSubItem = col;
		m_list.SetItem(&lvi);
	}

	m_list.DeleteItem(m_nDragIndex);

	CList<CPlaylistItem> tmp;
	UINT id = -1;
	for(int i = 0; i < m_list.GetItemCount(); i++)
	{
		POSITION pos = (POSITION)m_list.GetItemData(i);
		CPlaylistItem& pli = m_pl.GetAt(pos);
		tmp.AddTail(pli);
		if(pos == m_pl.GetPos()) id = pli.m_id;
	}
	m_pl.RemoveAll();
	POSITION pos = tmp.GetHeadPosition();
	for(int i = 0; pos; i++)
	{
		CPlaylistItem& pli = tmp.GetNext(pos);
		m_pl.AddTail(pli);
		if(pli.m_id == id) m_pl.SetPos(m_pl.GetTailPosition());
		m_list.SetItemData(i, (DWORD_PTR)m_pl.GetTailPosition());
	}

	ResizeListColumn();
}

BOOL CPlayerPlaylistBar::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	if((pNMHDR->code == TTN_NEEDTEXTA && (HWND)pTTTA->lParam != m_list.m_hWnd)
	|| (pNMHDR->code == TTN_NEEDTEXTW && (HWND)pTTTW->lParam != m_list.m_hWnd))
		return FALSE;

	int row = ((pNMHDR->idFrom-1) >> 10) & 0x3fffff;
	int col = (pNMHDR->idFrom-1) & 0x3ff;

	if(row < 0 || row >= m_pl.GetCount())
		return FALSE;

	CPlaylistItem& pli = m_pl.GetAt(FindPos(row));

	CString strTipText;

	if(col == COL_NAME)
	{
		POSITION pos = pli.m_fns.GetHeadPosition();
		while(pos) strTipText += _T("\n") + pli.m_fns.GetNext(pos);
		strTipText.Trim();

		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)1000);
	}
	else if(col == COL_TIME)
	{
		return FALSE;
	}

	if(pNMHDR->code == TTN_NEEDTEXTA)
	{
		m_strTipTextA = strTipText;
		pTTTA->lpszText = (LPSTR)(LPCSTR)m_strTipTextA;
	}
	else
	{
		m_strTipTextW = strTipText;
		pTTTW->lpszText = (LPWSTR)(LPCWSTR)m_strTipTextW;
	}

	*pResult = 0;

	return TRUE;    // message was handled
}

void CPlayerPlaylistBar::OnContextMenu(CWnd* /*pWnd*/, CPoint p)
{
	LVHITTESTINFO lvhti;
	lvhti.pt = p;
	m_list.ScreenToClient(&lvhti.pt);
	m_list.SubItemHitTest(&lvhti);

	POSITION pos = FindPos(lvhti.iItem);
//	bool fSelected = (pos == m_pl.GetPos());
	bool fOnItem = !!(lvhti.flags&LVHT_ONITEM);

	enum {M_OPEN=1, M_REMOVE, M_CLIPBOARD, M_SAVEAS, M_SORTBYNAME, M_SORTBYPATH, M_RANDOMIZE, M_SORTBYID};

	CMenu m;
	m.CreatePopupMenu();
	m.AppendMenu(MF_STRING|(!fOnItem?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_OPEN, _T("&Open"));
	m.AppendMenu(MF_STRING|(/*fSelected||*/!fOnItem?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_REMOVE, _T("&Remove from list"));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING|(!fOnItem?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_CLIPBOARD, _T("&Copy path to clipboard"));
	m.AppendMenu(MF_STRING|(!m_pl.GetCount()?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_SAVEAS, _T("&Save playlist..."));
	m.AppendMenu(MF_SEPARATOR);
	m.AppendMenu(MF_STRING|(!m_pl.GetCount()?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_SORTBYNAME, _T("Sort by &label"));
	m.AppendMenu(MF_STRING|(!m_pl.GetCount()?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_SORTBYPATH, _T("Sort by &path"));
	m.AppendMenu(MF_STRING|(!m_pl.GetCount()?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_RANDOMIZE, _T("R&andomize"));
	m.AppendMenu(MF_STRING|(!m_pl.GetCount()?(MF_DISABLED|MF_GRAYED):MF_ENABLED), M_SORTBYID, _T("R&estore"));

	switch(m.TrackPopupMenu(TPM_LEFTBUTTON|TPM_RETURNCMD, p.x, p.y, this))
	{
	case M_OPEN:
		m_pl.SetPos(pos);
		m_list.Invalidate();
		((CMainFrame*)AfxGetMainWnd())->OpenCurPlaylistItem();
		break;
	case M_REMOVE:
		if(m_pl.RemoveAt(pos)) ((CMainFrame*)AfxGetMainWnd())->CloseMedia();
		m_list.DeleteItem(lvhti.iItem);
		break;
	case M_SORTBYID:
		m_pl.SortById();
		SetupList();
		break;
	case M_SORTBYNAME:
		m_pl.SortByName();
		SetupList();
		break;
	case M_SORTBYPATH:
		m_pl.SortByPath();
		SetupList();
		break;
	case M_RANDOMIZE:
		m_pl.Randomize();
		SetupList();
		break;
	case M_CLIPBOARD:
		if(OpenClipboard() && EmptyClipboard())
		{
			CString str;

			CPlaylistItem& pli = m_pl.GetAt(pos);
			POSITION pos = pli.m_fns.GetHeadPosition();
			while(pos) str += _T("\r\n") + pli.m_fns.GetNext(pos);
			str.Trim();

			if(HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (str.GetLength()+1)*sizeof(TCHAR)))
			{
				if(TCHAR* s = (TCHAR*)GlobalLock(h))
				{
					_tcscpy(s, str);
					GlobalUnlock(h);
#ifdef UNICODE
			        SetClipboardData(CF_UNICODETEXT, h);
#else
			        SetClipboardData(CF_TEXT, h);
#endif
				}
			}
			CloseClipboard(); 
		}
		break;
	case M_SAVEAS:
		{
			CSaveTextFileDialog fd(
				CTextFile::ASCII, NULL, NULL,
				_T("Playlist (*.pls)|*.pls|WinAmp playlist (*.m3u)|*.m3u|Windows Media Playlist (*.asx)|*.asx||"), 
				this);
	
			if(fd.DoModal() != IDOK)
				break;

			int idx = fd.m_pOFN->nFilterIndex;

			CPath path(fd.GetPathName());

			switch(idx)
			{
			case 1: path.AddExtension(_T(".pls")); break;
			case 2: path.AddExtension(_T(".m3u")); break;
			case 3: path.AddExtension(_T(".asx")); break;
			default: break;
			}

			CTextFile f;
			if(!f.Save(path, fd.GetEncoding()))
				break;

			if(idx == 3)
			{
				f.WriteString(_T("<ASX version = \"3.0\">\n"));
			}

			pos = m_pl.GetHeadPosition();
			for(int i = 0; pos; i++)
			{
				CPlaylistItem& pli = m_pl.GetNext(pos);
				CString fn = pli.m_fns.GetHead();

				CString str;
				switch(idx)
				{
				case 1: str.Format(_T("File%d=%s\n"), i+1, fn); break;
				case 2: str.Format(_T("%s\n"), fn); break;
				case 3: str.Format(_T("<Entry><Ref href = \"%s\"/></Entry>\n"), fn); break;
				default: break;
				}
				f.WriteString(str);
			}

			if(idx == 3)
			{
				f.WriteString(_T("</ASX>\n"));
			}
		}
		break;
	default:
		break;
	}
}
