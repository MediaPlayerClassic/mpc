#include "stdafx.h"
#include "Split.h"

namespace ssf
{
	Split::Split(LPCTSTR sep, CString str, size_t limit, SplitType type)
	{
		RemoveAll();

		if(size_t seplen = _tcslen(sep))
		{
			for(int i = 0, j = 0, len = str.GetLength(); 
				i <= len && (limit == 0 || GetCount() < limit); 
				i = j + (int)seplen)
			{
				j = str.Find(sep, i);
				if(j < 0) j = len;

				CString s = i < j ? str.Mid(i, j - i) : _T("");

				switch(type)
				{
				case Min: s.Trim(); // fall through
				case Def: if(s.IsEmpty()) break; // else fall through
				case Max: Add(s); break;
				}
			}
		}
	}
}