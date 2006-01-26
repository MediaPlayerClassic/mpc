/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include "..\BaseSplitter\BaseSplitter.h"

class CMpegSplitterFile : public CBaseSplitterFileEx
{
	CMap<WORD,WORD,BYTE,BYTE> m_pid2pes;

	HRESULT Init();

public:
	CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	REFERENCE_TIME NextPTS(DWORD TrackNum);

	enum {us, ps, ts, es, pva} m_type;

	REFERENCE_TIME m_rtMin, m_rtMax;
	__int64 m_posMin, m_posMax;
	int m_rate; // byte/sec

	struct stream
	{
		CMediaType mt;
		WORD pid;
		BYTE pesid, ps1id;
		struct stream() {pid = pesid = ps1id = 0;}
		operator DWORD() const {return pid ? pid : ((pesid<<8)|ps1id);}
		bool operator == (const struct stream& s) const {return (DWORD)*this == (DWORD)s;}
	};

	enum {video, audio, subpic, unknown};

	class CStreamList : public CList<stream>
	{
	public:
		void Insert(stream& s)
		{
			for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
			{
				stream& s2 = GetAt(pos);
				if(s < s2) {InsertBefore(pos, s); return;}
			}

			AddTail(s);
		}

		static CStringW ToString(int type)
		{
			return 
				type == video ? L"Video" : 
				type == audio ? L"Audio" : 
				type == subpic ? L"Subtitle" : 
				L"Unknown";
		}
	} m_streams[unknown];

	HRESULT SearchStreams(__int64 start, __int64 stop);
	DWORD AddStream(WORD pid, BYTE pesid, DWORD len);
};
