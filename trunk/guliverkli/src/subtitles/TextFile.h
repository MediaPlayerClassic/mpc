#pragma once

#include <afx.h>

class CTextFile : protected CStdioFile
{
public:
	typedef enum {ASCII, UTF8, LE16, BE16} enc;

private:
	enc m_encoding;
	int m_offset;

public:
	CTextFile();

	virtual bool Open(LPCTSTR lpszFileName);
	virtual bool Save(LPCTSTR lpszFileName, enc e /*= ASCII*/);

	enc GetEncoding();
	bool IsUnicode();

	// CFile

	CString GetFilePath() const {return __super::GetFilePath();}

	// CStdioFile

    ULONGLONG GetPosition() const;
	ULONGLONG GetLength() const;
	ULONGLONG Seek(LONGLONG lOff, UINT nFrom);

	void WriteString(LPCSTR lpsz/*CStringA str*/);
	void WriteString(LPCWSTR lpsz/*CStringW str*/);
	BOOL ReadString(CStringA& str);
	BOOL ReadString(CStringW& str);
};

class CWebTextFile : public CTextFile
{
	LONGLONG m_llMaxSize;
	CString m_tempfn;

public:
	CWebTextFile(LONGLONG llMaxSize = 1024*1024);

	bool Open(LPCTSTR lpszFileName);
	bool Save(LPCTSTR lpszFileName, enc e /*= ASCII*/);
	void Close();
};

extern CStringW AToW(CStringA str);
extern CStringA WToA(CStringW str);
extern CString AToT(CStringA str);
extern CString WToT(CStringW str);
extern CStringA TToA(CString str);
extern CStringW TToW(CString str);
