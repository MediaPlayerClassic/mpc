#pragma once

#include <afxtempl.h>

// extern CString ExplodeMin(CString str, CStringList& sl, TCHAR sep, int limit = 0);
// extern CString Explode(CString str, CStringList& sl, TCHAR sep, int limit = 0);
// extern CString Implode(CStringList& sl, TCHAR sep);

template<class T, typename SEP>
T Explode(T str, CList<T>& sl, SEP sep, int limit = 0)
{
	sl.RemoveAll();

	if(limit == 1) {sl.AddTail(str); return T();}

	if(!str.IsEmpty() && str[str.GetLength()-1] != sep)
		str += sep;

	for(int i = 0, j = 0; (j = str.Find(sep, i)) >= 0; i = j+1)
	{
		sl.AddTail(str.Mid(i, j-i).Trim(sep).Trim());
		if(limit > 0 && sl.GetCount() == limit-1)
		{
			if(j+1 < str.GetLength()) 
				sl.AddTail(str.Mid(j+1).Trim(sep).Trim());
			break;
		}
	}

	if(sl.IsEmpty())
		sl.AddTail(str.Trim(sep).Trim());

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
extern CStringList& MakeLower(CStringList& sl);
extern CStringList& MakeUpper(CStringList& sl);
extern CStringList& RemoveStrings(CStringList& sl, int minlen, int maxlen);

