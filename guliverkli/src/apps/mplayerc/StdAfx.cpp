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

// stdafx.cpp : source file that includes just the standard includes
//	mplayerc.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// A few php-like functions implemented in C++ with MFC. Makes life much easier :)

CString Explode(CString str, CList<CString>& sl, TCHAR sep, int limit)
{
	sl.RemoveAll();

	if(limit == 1) {sl.AddTail(str); return "";}

	if(!str.IsEmpty() && str[str.GetLength()-1] != sep)
		str += sep;

	for(int i = 0, j = 0; (j = str.Find(sep, i)) >= 0; i = j+1)
	{
		CString tmp = str.Mid(i, j-i);
		tmp.TrimLeft(sep); tmp.TrimRight(sep);
		tmp.TrimLeft(); tmp.TrimRight();
		sl.AddTail(tmp);
		if(limit > 0 && sl.GetCount() == limit-1)
		{
			if(j+1 < str.GetLength()) 
			{
				CString tmp = str.Mid(j+1);
				tmp.TrimLeft(sep); tmp.TrimRight(sep);
				tmp.TrimLeft(); tmp.TrimRight();
				sl.AddTail(tmp);
			}
			break;
		}
	}

	if(sl.IsEmpty())
	{
		str.TrimLeft(sep); str.TrimRight(sep);
		str.TrimLeft(); str.TrimRight();
		sl.AddTail(str);
	}

	return sl.GetHead();
}

CString ExplodeMin(CString str, CList<CString>& sl, TCHAR sep, int limit)
{
	Explode(str, sl, sep, limit);
	POSITION pos = sl.GetHeadPosition();
	while(pos) 
	{
		POSITION tmp = pos;
		if(sl.GetNext(pos).IsEmpty())
			sl.RemoveAt(tmp);
	}
	if(sl.IsEmpty()) sl.AddTail(CString()); // eh

	return sl.GetHead();
}

CString UrlEncode(CString str, bool fRaw)
{
	CString urlstr;

	for(int i = 0; i < str.GetLength(); i++)
	{
		TCHAR c = str[i];
		if(c > 0x20 && c < 0x7f) urlstr += c;
		else if(c == 0x20) urlstr += fRaw ? ' ' : '+';
		else {CString tmp; tmp.Format(_T("%%%02x"), (BYTE)c); urlstr += tmp;}
	}

	return urlstr;
}

CString UrlDecode(CString str, bool fRaw)
{
	str.Replace(_T("&amp;"), _T("&"));

	TCHAR* s = str.GetBuffer(str.GetLength());
	TCHAR* e = s + str.GetLength();
	TCHAR* s1 = s;
	TCHAR* s2 = s;
	while(s1 < e)
	{
		if(*s1 == '%' && s1 < e-2
		&& (s1[1] >= '0' && s1[1] <= '9' || _tolower(s1[1]) >= 'a' && _tolower(s1[1]) <= 'f')
		&& (s1[2] >= '0' && s1[2] <= '9' || _tolower(s1[2]) >= 'a' && _tolower(s1[2]) <= 'f'))
		{
			s1[1] = tolower(s1[1]);
			s1[2] = tolower(s1[2]);
			*s2 = 0;
			if(s1[1] >= '0' && s1[1] <= '9') *s2 |= s1[1]-'0';
			else if(s1[1] >= 'a' && s1[1] <= 'f') *s2 |= s1[1]-'a'+10;
			*s2 <<= 4;
			if(s1[2] >= '0' && s1[2] <= '9') *s2 |= s1[2]-'0';
			else if(s1[2] >= 'a' && s1[2] <= 'f') *s2 |= s1[2]-'a'+10;
			s1 += 2;
		}
		else 
		{
			*s2 = *s1 == '+' && !fRaw ? ' ' : *s1;
		}

		s1++;
		s2++;
	}

	str.ReleaseBuffer(s2 - s);

	return str;
}
