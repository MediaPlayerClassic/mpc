#pragma once

namespace ssf
{
	class Exception
	{
		CString m_msg;

	public:
		Exception(LPCTSTR fmt, ...);
		
		CString ToString() {return m_msg;}
	};
}