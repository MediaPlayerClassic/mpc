/* 
 *	Copyright (C) 2003 Gabest
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
#include <io.h>
#include "TextFile.h"
#include "GFN.h"

TCHAR* exttypestr[9] = 
{
	_T("srt"), _T("sub"), _T("smi"), _T("psb"), _T("ssa"), _T("ass"), _T("idx"), _T("usf"), _T("xss")
};

static TCHAR* ext[2][9] = 
{
	{_T(".srt"), _T(".sub"), _T(".smi"), _T(".psb"), _T(".ssa"), _T(".ass"), _T(".idx"), _T(".usf"), _T(".xss")},
	{_T(".*.srt"), _T(".*.sub"), _T(".*.smi"), _T(".*.psb"), _T(".*.ssa"), _T(".*.ass"), _T(".*.dummyidx"), _T(".*.usf"), _T(".*.xss")}, 
};

#define WEBSUBEXT _T(".wse")

static int SubFileCompare(const void* elem1, const void* elem2)
{
	return(((SubFile*)elem1)->fn.CompareNoCase(((SubFile*)elem2)->fn));
}

void GetSubFileNames(CString fn, CStringArray& paths, SubFiles& ret)
{
	ret.RemoveAll();

	int extlistnum = sizeof(ext) / sizeof(ext[0]);
	int extsubnum = sizeof(ext[0]) / sizeof(ext[0][0]);

	fn.Replace('\\', '/');

	bool fWeb = false;
	{
		int i = fn.Find(_T("://"));
		if(i > 0) {fn = _T("http") + fn.Mid(i); fWeb = true;}
	}

	int	l = fn.GetLength(), l2 = l;
	l2 = fn.ReverseFind('.');
	l = fn.ReverseFind('/') + 1;
	if(l2 < l) l2 = l;

	CString orgpath = fn.Left(l);
	CString title = fn.Mid(l, l2-l);

	if(!fWeb)
	{
		struct _tfinddata_t file, file2;
		long hFile, hFile2 = 0;

		for(int k = 0; k < paths.GetSize(); k++)
		{
			CString path = paths[k];
			path.Replace('\\', '/');

			l = path.GetLength();
			if(l > 0 && path[l-1] != '/') path += '/';

			if(path.Find(':') == -1 && path.Find(_T("\\\\")) != 0) path = orgpath + path;

			path.Replace(_T("/./"), _T("/"));
			path.Replace('/', '\\');

			for(int j = 0; j < extlistnum; j++)
			{
				for(int i = 0; i < extsubnum; i++)
				{
					CString fn2 = path + title + ext[j][i];

					if((hFile = _tfindfirst((LPTSTR)(LPCTSTR)fn2, &file)) != -1L)
					{
						do
						{
							CString fn = path + file.name;

							hFile2 = -1;
							if(j == 0 || (hFile2 = _tfindfirst((LPTSTR)(LPCTSTR)(fn.Left(fn.ReverseFind('.')) + _T(".avi")), &file2)) == -1L)
							{
								SubFile f;
								f.fn = fn;
								ret.Add(f);
							}
							
							if(hFile2 >= 0) _findclose(hFile2);
						}
						while(_tfindnext(hFile, &file) != -1);
						
						_findclose(hFile);
					}
				}
			}
		}
	}
	else if(l > 7)
	{
		CWebTextFile wtf; // :)
		if(wtf.Open(orgpath + title + WEBSUBEXT))
		{
			CString fn;
			while(wtf.ReadString(fn) && fn.Find(_T("://")) >= 0)
			{
				SubFile f;
				f.fn = fn;
				ret.Add(f);
			}
		}
	}

	// sort files, this way the user can define the order (movie.00.English.srt, movie.01.Hungarian.srt, etc)

	qsort(ret.GetData(), ret.GetSize(), sizeof(SubFile), SubFileCompare);
}
