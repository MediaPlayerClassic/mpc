#pragma once

#include <afxtempl.h>

// extern CString ExplodeMin(CString str, CList<CString>& sl, TCHAR sep, int limit = 0);
// extern CString Explode(CString str, CList<CString>& sl, TCHAR sep, int limit = 0);
// extern CString Implode(CList<CString>& sl, TCHAR sep);

template<class T, typename SEP>
T Explode(T str, CList<T>& sl, SEP sep, int limit = 0)
{
	sl.RemoveAll();

	for(int i = 0, j = 0; ; i = j+1)
	{
		j = str.Find(sep, i);

		if(j < 0 || sl.GetCount() == limit-1)
		{
			sl.AddTail(str.Mid(i).Trim());
			break;
		}
		else
		{
			sl.AddTail(str.Mid(i, j-i).Trim());
		}		
	}

	return sl.GetHead();
}

template<class T, typename SEP>
T ExplodeMin(T str, CList<T>& sl, SEP sep, int limit = 0)
{
	Explode(str, sl, sep, limit);
	POSITION pos = sl.GetHeadPosition();
	while(pos) 
	{
		POSITION tmp = pos;
		if(sl.GetNext(pos).IsEmpty())
			sl.RemoveAt(tmp);
	}
	if(sl.IsEmpty()) sl.AddTail(T()); // eh

	return sl.GetHead();
}

template<class T, typename SEP>
T Implode(CList<T>& sl, SEP sep)
{
	T ret;
	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		ret += sl.GetNext(pos);
		if(pos) ret += sep;
	}
	return(ret);
}

extern CString ExtractTag(CString tag, CMapStringToString& attribs, bool& fClosing);
extern CStringA ConvertMBCS(CStringA str, DWORD SrcCharSet, DWORD DstCharSet);
extern CStringA UrlEncode(CStringA str, bool fRaw = false);
extern CStringA UrlDecode(CStringA str, bool fRaw = false);
extern DWORD CharSetToCodePage(DWORD dwCharSet);
extern CList<CString>& MakeLower(CList<CString>& sl);
extern CList<CString>& MakeUpper(CList<CString>& sl);
extern CList<CString>& RemoveStrings(CList<CString>& sl, int minlen, int maxlen);

