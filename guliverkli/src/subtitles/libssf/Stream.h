#pragma once

#include "Exception.h"

namespace ssf
{
	class Stream
	{
	public:
		enum {EOS = -1};
		enum {none, unknown, utf8, utf16le, utf16be, tchar} m_encoding;

	private:
		int m_line, m_col;

		CAtlList<int> m_queue;
		int PushChar(), PopChar();

		int NextChar();

	protected:
		virtual int NextByte() = 0;

	public:
		Stream();
		virtual ~Stream();

		int PeekChar(), GetChar();

		static bool IsWhiteSpace(int c, LPCTSTR morechars = NULL);
		int SkipWhiteSpace(LPCTSTR morechars = NULL);

		void ThrowError(LPCTSTR fmt, ...);
	};

	class FileStream : public Stream
	{
		FILE* m_file;

	protected:
		int NextByte();

	public:
		FileStream(const TCHAR* fn);
		~FileStream();
	};

	class MemoryStream : public Stream
	{
		BYTE* m_pBytes;
		int m_pos, m_len;
		bool m_fFree;

	protected:
		int NextByte();

	public:
		MemoryStream(BYTE* pBytes, int len, bool fCopy, bool fFree);
		~MemoryStream();
	};

	class CharacterStream : public Stream
	{
		CString m_str;
		int m_pos;

	protected:
		int NextByte();

	public:
		CharacterStream(CString str);
	};
}