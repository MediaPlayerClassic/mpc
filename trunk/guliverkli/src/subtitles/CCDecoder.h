#pragma once

#include "STS.h"

class CCDecoder
{
	CSimpleTextSubtitle m_sts;
	CString m_fn, m_rawfn;
	__int64 m_time;
	bool m_fEndOfCaption;
	WCHAR m_buff[16][33], m_disp[16][33];
	CPoint m_cursor;

	void SaveDisp(__int64 time);
	void MoveCursor(int x, int y);
	void OffsetCursor(int x, int y);
	void PutChar(WCHAR c);

public:
	CCDecoder(CString fn, CString rawfn);
	virtual ~CCDecoder();
	void DecodeCC(BYTE* buff, int len, __int64 time);
	void ExtractCC(BYTE* buff /*2048bytes*/, __int64 time);
};


