#pragma once

namespace ssf
{
	class Split : public CAtlArray<CString>
	{
	public:
		enum SplitType {Min, Def, Max};
		Split(LPCTSTR sep, CString str, size_t limit = 0, SplitType type = Def);
		operator size_t() {return GetCount();}
	};
}