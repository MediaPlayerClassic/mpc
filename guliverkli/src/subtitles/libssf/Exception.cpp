#include "stdafx.h"
#include "Exception.h"

namespace ssf
{
	Exception::Exception(LPCTSTR fmt, ...) 
	{
		va_list args;
		va_start(args, fmt);
		int len = _vsctprintf(fmt, args) + 1;
		if(len > 0) _vstprintf_s(m_msg.GetBufferSetLength(len), len, fmt, args);
		va_end(args);
	}
}