#pragma once

#include "Stream.h"
#include "NodeFactory.h"

namespace ssf
{
	class File : public NodeFactory
	{
	public:
		File();
		virtual ~File();

		void Parse(Stream& s, LPCTSTR predef = NULL);

		void ParseDefs(Stream& s, Reference* pParentRef);
		void ParseTypes(Stream& s, CAtlList<CString>& types);
		void ParseName(Stream& s, CString& name);
		void ParseQuotedString(Stream& s, Definition* pDef);
		void ParseNumber(Stream& s, Definition* pDef);
		void ParseBlock(Stream& s, Definition* pDef);
		void ParseRefs(Stream& s, Definition* pParentDef, LPCTSTR term = _T(";}]"));
	};
}