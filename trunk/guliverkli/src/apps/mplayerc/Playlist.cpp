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
#include "mplayerc.h"
#include "playlist.h"

//
// CPlaylistItem
//

UINT CPlaylistItem::m_globalid  = 0;

CPlaylistItem::CPlaylistItem()
	: m_duration(0)
	, m_fInvalid(false)
{
	m_id = m_globalid++;
}

CPlaylistItem::~CPlaylistItem()
{
}

CPlaylistItem::CPlaylistItem(const CPlaylistItem& pli)
{
	*this = pli;
}

CPlaylistItem& CPlaylistItem::operator = (const CPlaylistItem& pli)
{
	m_id = pli.m_id;
	m_fns.RemoveAll();
	m_fns.AddTail((CStringList*)&pli.m_fns);
	m_subs.RemoveAll();
	m_subs.AddTail((CStringList*)&pli.m_subs);
	m_duration = pli.m_duration;
	m_fInvalid = pli.m_fInvalid;
	return(*this);
}

POSITION CPlaylistItem::FindFile(CString path)
{
	POSITION pos = m_fns.GetHeadPosition();
	while(pos && !m_fns.GetAt(pos).CompareNoCase(path)) m_fns.GetNext(pos);
	return(NULL);
}

//
// CPlaylist
//

CPlaylist::CPlaylist()
	: m_pos(NULL)
{
}

CPlaylist::~CPlaylist()
{
}

void CPlaylist::RemoveAll()
{
	__super::RemoveAll();
	m_pos = NULL;
}

bool CPlaylist::RemoveAt(POSITION pos)
{
	if(pos)
	{
		__super::RemoveAt(pos);
		if(m_pos == pos) {m_pos = NULL; return(true);}
	}

	return(false);
}

typedef struct {UINT n; POSITION pos;} plsort_t;

static int compare(const void* arg1, const void* arg2)
{
	UINT a1 = ((plsort_t*)arg1)->n;
	UINT a2 = ((plsort_t*)arg2)->n;
	return a1 > a2 ? 1 : a1 < a2 ? -1 : 0;
}

typedef struct {LPCTSTR str; POSITION pos;} plsort2_t;

int compare2(const void* arg1, const void* arg2)
{
	return _tcsicmp(((plsort2_t*)arg1)->str, ((plsort2_t*)arg2)->str);
}

void CPlaylist::SortById()
{
	CArray<plsort_t> a;
	a.SetSize(GetCount());
	POSITION pos = GetHeadPosition();
	for(int i = 0; pos; i++, GetNext(pos))
		a[i].n = GetAt(pos).m_id, a[i].pos = pos;
	qsort(a.GetData(), a.GetCount(), sizeof(plsort_t), compare);
	for(int i = 0; i < a.GetCount(); i++)
	{
		AddTail(GetAt(a[i].pos));
		RemoveAt(a[i].pos);
		if(m_pos == a[i].pos) m_pos = GetTailPosition(); 
	}
}

void CPlaylist::SortByName()
{
	CArray<plsort2_t> a;
	a.SetSize(GetCount());
	POSITION pos = GetHeadPosition();
	for(int i = 0; pos; i++, GetNext(pos))
	{
		CString& fn = GetAt(pos).m_fns.GetHead();
		a[i].str = (LPCTSTR)fn + max(fn.ReverseFind('/'), fn.ReverseFind('\\')) + 1;
		a[i].pos = pos;
	}
	qsort(a.GetData(), a.GetCount(), sizeof(plsort2_t), compare2);
	for(int i = 0; i < a.GetCount(); i++)
	{
		AddTail(GetAt(a[i].pos));
		RemoveAt(a[i].pos);
		if(m_pos == a[i].pos) m_pos = GetTailPosition(); 
	}
}

void CPlaylist::SortByPath()
{
	CArray<plsort2_t> a;
	a.SetSize(GetCount());
	POSITION pos = GetHeadPosition();
	for(int i = 0; pos; i++, GetNext(pos))
		a[i].str = GetAt(pos).m_fns.GetHead(), a[i].pos = pos;
	qsort(a.GetData(), a.GetCount(), sizeof(plsort2_t), compare2);
	for(int i = 0; i < a.GetCount(); i++)
	{
		AddTail(GetAt(a[i].pos));
		RemoveAt(a[i].pos);
		if(m_pos == a[i].pos) m_pos = GetTailPosition(); 
	}
}

void CPlaylist::Randomize()
{
	CArray<plsort_t> a;
	a.SetSize(GetCount());
	srand((unsigned int)time(NULL));
	POSITION pos = GetHeadPosition();
	for(int i = 0; pos; i++, GetNext(pos))
		a[i].n = rand(), a[i].pos = pos;
	qsort(a.GetData(), a.GetCount(), sizeof(plsort_t), compare);
	CList<CPlaylistItem> pl;
	for(int i = 0; i < a.GetCount(); i++)
	{
		AddTail(GetAt(a[i].pos));
		RemoveAt(a[i].pos);
		if(m_pos == a[i].pos) m_pos = GetTailPosition(); 
	}
}

POSITION CPlaylist::GetPos()
{
	return(m_pos);
}

void CPlaylist::SetPos(POSITION pos)
{
	m_pos = pos;
}

CPlaylistItem& CPlaylist::GetNextWrap(POSITION& pos)
{
	GetNext(pos);
	if(!pos) pos = GetHeadPosition();
	return(GetAt(pos));
}

CPlaylistItem& CPlaylist::GetPrevWrap(POSITION& pos)
{
	GetPrev(pos);
	if(!pos) pos = GetTailPosition();
	return(GetAt(pos));
}

