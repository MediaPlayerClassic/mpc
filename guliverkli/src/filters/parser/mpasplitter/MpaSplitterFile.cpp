/* 
 *	Copyright (C) 2003-2004 Gabest
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

#include "StdAfx.h"
#include <mmreg.h>
#include "MpaSplitterFile.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

//

static const LPCTSTR s_genre[] = 
{
	_T("Blues"), _T("Classic Rock"), _T("Country"), _T("Dance"),
	_T("Disco"), _T("Funk"), _T("Grunge"), _T("Hip-Hop"),
	_T("Jazz"), _T("Metal"), _T("New Age"), _T("Oldies"), 
	_T("Other"), _T("Pop"), _T("R&B"), _T("Rap"),
	_T("Reggae"), _T("Rock"), _T("Techno"), _T("Industrial"), 
	_T("Alternative"), _T("Ska"), _T("Death Metal"), _T("Pranks"),
	_T("Soundtrack"), _T("Euro-Techno"), _T("Ambient"), _T("Trip-Hop"),
	_T("Vocal"), _T("Jazz+Funk"), _T("Fusion"), _T("Trance"),
	_T("Classical"), _T("Instrumental"), _T("Acid"), _T("House"), 
	_T("Game"), _T("Sound Clip"), _T("Gospel"), _T("Noise"),
	_T("Alternative Rock"), _T("Bass"), _T("Soul"), _T("Punk"), 
	_T("Space"), _T("Meditative"), _T("Instrumental Pop"), _T("Instrumental Rock"),
	_T("Ethnic"), _T("Gothic"), _T("Darkwave"), _T("Techno-Industrial"),
	_T("Electronic"), _T("Pop-Folk"), _T("Eurodance"), _T("Dream"),
	_T("Southern Rock"), _T("Comedy"), _T("Cult"), _T("Gangsta"),
	_T("Top 40"), _T("Christian Rap"), _T("Pop/Funk"), _T("Jungle"),
	_T("Native US"), _T("Cabaret"), _T("New Wave"), _T("Psychadelic"), 
	_T("Rave"), _T("Showtunes"), _T("Trailer"), _T("Lo-Fi"),
	_T("Tribal"), _T("Acid Punk"), _T("Acid Jazz"), _T("Polka"),
	_T("Retro"), _T("Musical"), _T("Rock & Roll"), _T("Hard Rock"),
	_T("Folk"), _T("Folk-Rock"), _T("National Folk"), _T("Swing"),
	_T("Fast Fusion"), _T("Bebob"), _T("Latin"), _T("Revival"),
	_T("Celtic"), _T("Bluegrass"), _T("Avantgarde"), _T("Gothic Rock"), 
	_T("Progressive Rock"), _T("Psychedelic Rock"), _T("Symphonic Rock"), _T("Slow Rock"),
	_T("Big Band"), _T("Chorus"), _T("Easy Listening"), _T("Acoustic"),
	_T("Humour"), _T("Speech"), _T("Chanson"), _T("Opera"), 
	_T("Chamber Music"), _T("Sonata"), _T("Symphony"), _T("Booty Bass"), 
	_T("Primus"), _T("Porn Groove"), _T("Satire"), _T("Slow Jam"),
	_T("Club"), _T("Tango"), _T("Samba"), _T("Folklore"), 
	_T("Ballad"), _T("Power Ballad"), _T("Rhytmic Soul"), _T("Freestyle"),
	_T("Duet"), _T("Punk Rock"), _T("Drum Solo"), _T("Acapella"), 
	_T("Euro-House"), _T("Dance Hall"), _T("Goa"), _T("Drum & Bass"),
	_T("Club-House"), _T("Hardcore"), _T("Terror"), _T("Indie"),
	_T("BritPop"), _T("Negerpunk"), _T("Polsk Punk"), _T("Beat"),
	_T("Christian Gangsta"), _T("Heavy Metal"), _T("Black Metal"), 
	_T("Crossover"), _T("Contemporary C"), _T("Christian Rock"), _T("Merengue"), _T("Salsa"),
	_T("Thrash Metal"), _T("Anime"), _T("JPop"), _T("SynthPop"),
};

//

CMpaSplitterFile::CMpaSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFileEx(pAsyncReader, hr)
	, m_mode(none)
	, m_rtDuration(0)
	, m_startpos(0)
	, m_endpos(0)
	, m_totalbps(0)
{
	if(SUCCEEDED(hr)) hr = Init();
}

HRESULT CMpaSplitterFile::Init()
{
	m_startpos = 0;
	m_endpos = GetLength();

	if(m_endpos > 128)
	{
		Seek(m_endpos - 128);

		if(BitRead(24) == 'TAG')
		{
			m_endpos -= 128;

			CStringA str;
			
			// title
			Read((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['TIT2'] = CString(str).Trim();

			// artist
			Read((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['TPE1'] = CString(str).Trim();

			// album
			Read((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['TALB'] = CString(str).Trim();

			// year
			Read((BYTE*)str.GetBufferSetLength(4), 4);
			m_tags['TYER'] = CString(str).Trim();

			// comment
			Read((BYTE*)str.GetBufferSetLength(30), 30);
			m_tags['COMM'] = CString(str).Trim(); 

			// track
			LPCSTR s = str;
			if(s[28] == 0 && s[29] != 0)
				m_tags['TRCK'].Format(_T("%d"), s[29]); 

			// genre
			BYTE genre = (BYTE)BitRead(8);
			if(genre < countof(s_genre))
				m_tags['TCON'] = s_genre[genre];
		}
	}

	Seek(0);

	if(BitRead(24, true) == 'ID3')
	{
		Seek(3);

		BYTE major = (BYTE)BitRead(8);
		BYTE revision = (BYTE)BitRead(8);
		BYTE flags = (BYTE)BitRead(8);
		DWORD size = 0;
		if(BitRead(1) != 0) return E_FAIL;
		size |= BitRead(7) << 21;
		if(BitRead(1) != 0) return E_FAIL;
		size |= BitRead(7) << 14;
		if(BitRead(1) != 0) return E_FAIL;
		size |= BitRead(7) << 7;
		if(BitRead(1) != 0) return E_FAIL;
		size |= BitRead(7);

		m_startpos = GetPos() + size;

		// TODO: read tags
	}

	__int64 startpos;
	int nBytesPerSec = 0;

	Seek(m_startpos);

	if(m_mode == none && Read(m_mpahdr, min(m_endpos - GetPos(), 0x2000), true, &m_mt))
	{
		m_mode = mpa;

		startpos = GetPos() - 4;
		nBytesPerSec = m_mpahdr.nBytesPerSec;
		
		// make sure the first frame is followed by another of the same kind (validates m_mpahdr basically)
		Seek(startpos + m_mpahdr.FrameSize);
		if(!Sync(4)) m_mode = none;
	}

	Seek(m_startpos);

	if(m_mode == none && Read(m_aachdr, min(m_endpos - GetPos(), 0x2000), &m_mt))
	{
		m_mode = mp4a;

		startpos = GetPos() - (m_aachdr.fcrc?7:9);
		nBytesPerSec = ((WAVEFORMATEX*)m_mt.Format())->nAvgBytesPerSec;

		// make sure the first frame is followed by another of the same kind (validates m_aachdr basically)
		Seek(startpos + m_aachdr.aac_frame_length);
		if(!Sync(9)) m_mode = none;
	}

	if(m_mode == none)
		return E_FAIL;

	m_startpos = startpos;

	// initial duration, may not be correct (VBR files...)
	m_rtDuration = 10000000i64 * (m_endpos - m_startpos) / nBytesPerSec;

	return S_OK;
}

bool CMpaSplitterFile::Sync(int limit)
{
	int FrameSize;
	REFERENCE_TIME rtDuration;
	return Sync(FrameSize, rtDuration, limit);
}

bool CMpaSplitterFile::Sync(int& FrameSize, REFERENCE_TIME& rtDuration, int limit)
{
	__int64 endpos = min(m_endpos, GetPos() + limit);

	if(m_mode == mpa)
	{
		while(GetPos() <= endpos - 4)
		{
			mpahdr h;

			if(Read(h, endpos - GetPos(), true)
			&& m_mpahdr.version == h.version
			&& m_mpahdr.layer == h.layer
			&& m_mpahdr.channels == h.channels)
			{
				Seek(GetPos() - 4);
				AdjustDuration(h.nBytesPerSec);

				FrameSize = h.FrameSize;
				rtDuration = h.rtDuration;

				return true;
			}
		}
	}
	else if(m_mode == mp4a)
	{
		while(GetPos() <= endpos - 9)
		{
			aachdr h;

			if(Read(h, endpos - GetPos())
			&& m_aachdr.version == h.version
			&& m_aachdr.layer == h.layer
			&& m_aachdr.channels == h.channels)
			{
				Seek(GetPos() - (h.fcrc?7:9));
				AdjustDuration(h.nBytesPerSec);
				Seek(GetPos() + (h.fcrc?7:9));

				FrameSize = h.FrameSize;
				rtDuration = h.rtDuration;

				return true;
			}
		}
	}

	return false;
}

void CMpaSplitterFile::AdjustDuration(int nBytesPerSec)
{
	int rValue;
	if(!m_pos2bps.Lookup(GetPos(), rValue))
	{
		m_totalbps += nBytesPerSec;
		m_pos2bps.SetAt(GetPos(), nBytesPerSec);
		__int64 avgbps = m_totalbps / m_pos2bps.GetCount();
		m_rtDuration = 10000000i64 * (m_endpos - m_startpos) / avgbps;
	}
}
