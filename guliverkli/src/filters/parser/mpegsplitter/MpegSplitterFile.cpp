#include "StdAfx.h"
#include <mmreg.h>
#include "MpegSplitterFile.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

#define MEGABYTE 1024*10240

CMpegSplitterFile::CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr)
	, m_type(us)
{
	if(SUCCEEDED(hr)) hr = Init();
}

HRESULT CMpegSplitterFile::Init()
{
	HRESULT hr;

	// get the type first
	m_type = us;

	Seek(0);

	if(m_type == us)
	{
		int cnt = 0, limit = 4;
		for(trhdr h; cnt < limit && Read(h); cnt++) 
			Seek(GetPos() + h.bytes);
		if(cnt >= limit)
			m_type = ts;
	}

	Seek(0);

	if(m_type == us)
	{
		BYTE b;
		if(Next(b))
		{
			if(b == 0xba)
			{
				b = (BYTE)BitRead(8);
				if((b&0xf1) == 0x21) m_type = ps; // mpeg1
				else if((b&0xc4) == 0x44) m_type = ps; // mpeg2
			}
			else if((b&0xe0) == 0xc0 // audio, 110xxxxx, mpeg1/2/3
				|| (b&0xf0) == 0xe0 // video, 1110xxxx, mpeg1/2
				|| (b&0xbd) == 0xbd) // private stream 1, 0xbd, ac3/dts/lpcm/subpic
			{
				m_type = es;
			}
		}
	}

	Seek(0);

	if(m_type == us)
	{
		return E_FAIL;
	}

	// min/max pts & bitrate

	m_rtMin = m_posMin = _I64_MAX;
	m_rtMax = m_posMax = 0;

	CList<__int64> fps;
	for(int i = 0, j = 5; i <= j; i++)
		fps.AddTail(i*GetLength()/j);

	for(__int64 pfp = 0; fps.GetCount(); )
	{
		__int64 fp = fps.RemoveHead();
		fp = min(GetLength() - MEGABYTE/4, fp);
		fp = max(pfp, fp);
		__int64 nfp = fp + (pfp == 0 ? 5*MEGABYTE : MEGABYTE/8);
		if(FAILED(hr = SearchStreams(fp, nfp)))
			return hr;
		pfp = nfp;
	}

	if(m_posMax - m_posMin <= 0 || m_rtMax - m_rtMin <= 0)
		return E_FAIL;

	m_rate = 10000000i64 * (m_posMax - m_posMin) / (m_rtMax - m_rtMin);

//#ifndef DEBUG
	if(m_streams[video].GetCount() || m_streams[subpic].GetCount())
	{
		stream s;
		s.mt.majortype = MEDIATYPE_Video;
		s.mt.subtype = MEDIASUBTYPE_DVD_SUBPICTURE;
		s.mt.formattype = FORMAT_None;
		m_streams[subpic].Insert(s);
	}
//#endif

	Seek(0);

	return S_OK;
}

bool CMpegSplitterFile::Next(BYTE& code, __int64 len)
{
	BitByteAlign();
	DWORD dw = -1;
	do
	{
		if(len-- == 0 || GetPos() >= GetLength()) return(false);
		dw = (dw << 8) | (BYTE)BitRead(8);
	}
	while((dw&0xffffff00) != 0x00000100);
	code = (BYTE)(dw&0xff);
	return(true);
}

REFERENCE_TIME CMpegSplitterFile::NextPTS(DWORD TrackNum)
{
	REFERENCE_TIME rt = -1;
	__int64 rtpos = -1;

	BYTE b;

	while(GetPos() < GetLength())
	{
		if(m_type == ps || m_type == es)
		{
			if(!Next(b)) // continue;
				{ASSERT(0); break;}

			rtpos = GetPos()-4;

			if(b >= 0xbd && b < 0xf0)
			{
				peshdr h;
				if(!Read(h, b) || !h.len) continue;

				__int64 pos = GetPos();

				if(h.fpts && AddStream(0, b, h.len) == TrackNum)
				{
					ASSERT(h.pts >= m_rtMin && h.pts <= m_rtMax);
					rt = h.pts;
					break;
				}

				Seek(pos + h.len);
			}
		}
		else if(m_type == ts)
		{
			trhdr h;
			if(!Read(h)) continue;

			rtpos = GetPos()-4;

			__int64 pos = GetPos();

			if(h.payload && h.payloadstart && h.pid >= 16 && h.pid < 0x1fff)
			{
				peshdr h2;
				if(Next(b, 4) && Read(h2, b)) // pes packet
				{
					if(h2.fpts && AddStream(h.pid, b, h.bytes - (GetPos() - rtpos)) == TrackNum)
					{
						ASSERT(h2.pts >= m_rtMin && h2.pts <= m_rtMax);
						rt = h2.pts;
						break;
					}
				}
			}

			Seek(pos + h.bytes);
		}
	}

	if(rtpos >= 0) Seek(rtpos);
	if(rt >= 0) rt -= m_rtMin;

	return rt;
}

HRESULT CMpegSplitterFile::SearchStreams(__int64 start, __int64 stop)
{
	Seek(start);
	stop = min(stop, GetLength());

	while(GetPos() < stop)
	{
		BYTE b;

		if(m_type == ps || m_type == es)
		{
			if(!Next(b)) continue;

			if(b == 0xba) // program stream header
			{
				pshdr h;
				if(!Read(h)) continue;
			}
			else if(b == 0xbb) // program stream system header
			{
				pssyshdr h;
				if(!Read(h)) continue;
			}
			else if(b >= 0xbd && b < 0xf0) // pes packet
			{
				peshdr h;
				if(!Read(h, b)) continue;

				if(h.type == mpeg2 && h.scrambling) {ASSERT(0); return E_FAIL;}

				if(h.fpts)
				{
					if(m_rtMin == _I64_MAX) {m_rtMin = h.pts; m_posMin = GetPos();}
					if(h.pts > m_rtMin && m_rtMax < h.pts) {m_rtMax = h.pts; m_posMax = GetPos();}
				}

				__int64 pos = GetPos();
				AddStream(0, b, h.len);
				if(h.len) Seek(pos + h.len);
			}
		}
		else if(m_type == ts)
		{
			trhdr h;
			if(!Read(h)) continue;

			if(h.scrambling) {ASSERT(0); return E_FAIL;}

			__int64 pos = GetPos();

			if(h.payload && h.payloadstart && h.pid >= 16 && h.pid < 0x1fff)
			{
				peshdr h2;
				if(Next(b, 4) && Read(h2, b)) // pes packet
				{
					if(h2.type == mpeg2 && h2.scrambling) {ASSERT(0); return E_FAIL;}

					if(h2.fpts)
					{
						if(m_rtMin == _I64_MAX) {m_rtMin = h2.pts; m_posMin = GetPos();}
						if(h2.pts > m_rtMin && m_rtMax < h2.pts) {m_rtMax = h2.pts; m_posMax = GetPos();}
					}

					AddStream(h.pid, b, h.bytes - (GetPos() - pos));
				}
			}

			Seek(pos + h.bytes);
		}
	}

	return S_OK;
}

DWORD CMpegSplitterFile::AddStream(WORD pid, BYTE pesid, DWORD len)
{
	stream s;
	s.pid = pid;
	s.pesid = pesid;

	int type = unknown;

	if(pesid >= 0xe0 && pesid < 0xf0) // mpeg video
	{
		CMpegSplitterFile::seqhdr h;
		if(!m_streams[video].Find(s) && Read(h, len, &s.mt))
			type = video;
	}
	else if(pesid >= 0xc0 && pesid < 0xe0) // mpeg audio
	{
		__int64 pos = GetPos();

		CMpegSplitterFile::mpahdr h;
		if(!m_streams[audio].Find(s) && Read(h, len, &s.mt))
			type = audio;

		if(type == unknown)
		{
		Seek(pos);

		CMpegSplitterFile::aachdr h2;
		if(!m_streams[audio].Find(s) && Read(h2, len, &s.mt))
			type = audio;
		}
	}
	else if(pesid == 0xbd) // private stream 1
	{
		BYTE b = (BYTE)BitRead(8, true);
		WORD w = (WORD)BitRead(16, true);
		DWORD dw = (DWORD)BitRead(32, true);

		if(b >= 0x80 && b < 0x88 || w == 0x0b77) // ac3
		{
			s.ps1id = (b >= 0x80 && b < 0x88) ? (BYTE)(BitRead(32) >> 24) : 0x80;
	
			CMpegSplitterFile::ac3hdr h;
			if(!m_streams[audio].Find(s) && Read(h, len, &s.mt))
				type = audio;
		}
		else if(b >= 0x88 && b < 0x90 || dw == 0x7ffe8001) // dts
		{
			s.ps1id = (b >= 0x88 && b < 0x90) ? (BYTE)(BitRead(32) >> 24) : 0x88;

			CMpegSplitterFile::dtshdr h;
			if(!m_streams[audio].Find(s) && Read(h, len, &s.mt))
				type = audio;
		}
		else if(b >= 0xa0 && b < 0xa8) // lpcm
		{
			s.ps1id = (b >= 0xa0 && b < 0xa8) ? (BYTE)(BitRead(32) >> 24) : 0xa0;
			
			CMpegSplitterFile::lpcmhdr h;
			if(Read(h, &s.mt) && !m_streams[audio].Find(s)) // note the reversed order, the header should be stripped always even if it's not a new stream
				type = audio;
		}
		else if(b >= 0x20 && b < 0x40) // DVD subpic
		{
			s.ps1id = (BYTE)BitRead(8);

			CMpegSplitterFile::dvdspuhdr h;
			if(!m_streams[subpic].Find(s) && Read(h, &s.mt))
				type = subpic;
		}
		else if(b >= 0x70 && b < 0x80) // SVCD subpic
		{
			s.ps1id = (BYTE)BitRead(8);

			CMpegSplitterFile::svcdspuhdr h;
			if(!m_streams[subpic].Find(s) && Read(h, &s.mt))
				type = subpic;
		}
		else if(b >= 0x00 && b < 0x10) // CVD subpic
		{
			s.ps1id = (BYTE)BitRead(8);

			CMpegSplitterFile::cvdspuhdr h;
			if(!m_streams[subpic].Find(s) && Read(h, &s.mt))
				type = subpic;
		}
	}
	else if(pesid == 0xbe) // padding
	{
	}
	else if(pesid == 0xbf) // private stream 2
	{
	}

	if(type != unknown && !m_streams[type].Find(s))
	{
		if(s.pid)
		{
			for(int i = 0; i < unknown; i++)
			{
				if(m_streams[i].Find(s)) {/*ASSERT(0);*/ return s;}
			}
		}

		m_streams[type].Insert(s);
	}

	return s;
}

#define MARKER if(BitRead(1) != 1) {ASSERT(0); return(false);}

bool CMpegSplitterFile::Read(pshdr& h)
{
	memset(&h, 0, sizeof(h));

	BYTE b = (BYTE)BitRead(8, true);

	if((b&0xf1) == 0x21)
	{
		h.type = mpeg1;

		EXECUTE_ASSERT(BitRead(4) == 2);

		h.scr = 0;
		h.scr |= BitRead(3) << 30; MARKER; // 32..30
		h.scr |= BitRead(15) << 15; MARKER; // 29..15
		h.scr |= BitRead(15); MARKER; MARKER; // 14..0
		h.bitrate = BitRead(22); MARKER;
	}
	else if((b&0xc4) == 0x44)
	{
		h.type = mpeg2;

		EXECUTE_ASSERT(BitRead(2) == 1);

		h.scr = 0;
		h.scr |= BitRead(3) << 30; MARKER; // 32..30
		h.scr |= BitRead(15) << 15; MARKER; // 29..15
		h.scr |= BitRead(15); MARKER; // 14..0
		h.scr = h.scr*300 + BitRead(9); MARKER;
		h.bitrate = BitRead(22); MARKER; MARKER;
		BitRead(5); // reserved
		UINT64 stuffing = BitRead(3);
		while(stuffing-- > 0) EXECUTE_ASSERT(BitRead(8) == 0xff);
	}
	else
	{
		return(false);
	}

	h.bitrate *= 400;

	return(true);
}

bool CMpegSplitterFile::Read(pssyshdr& h)
{
	memset(&h, 0, sizeof(h));

	WORD len = (WORD)BitRead(16); MARKER;
	h.rate_bound = (DWORD)BitRead(22); MARKER;
	h.audio_bound = (BYTE)BitRead(6);
	h.fixed_rate = !!BitRead(1);
	h.csps = !!BitRead(1);
	h.sys_audio_loc_flag = !!BitRead(1);
	h.sys_video_loc_flag = !!BitRead(1); MARKER;
	h.video_bound = (BYTE)BitRead(5);

	EXECUTE_ASSERT((BitRead(8)&0x7f) == 0x7f); // reserved (should be 0xff, but not in reality)

	for(len -= 6; len > 3; len -= 3) // TODO: also store these, somewhere, if needed
	{
		UINT64 stream_id = BitRead(8);
		EXECUTE_ASSERT(BitRead(2) == 3);
		UINT64 p_std_buff_size_bound = (BitRead(1)?1024:128)*BitRead(13);
	}

	return(true);
}

bool CMpegSplitterFile::Read(peshdr& h, BYTE code)
{
	memset(&h, 0, sizeof(h));

	if(!(code >= 0xbd && code < 0xf0))
		return(false);

	h.len = (WORD)BitRead(16);

	if(code == 0xbe || code == 0xbf)
		return(true);

	// mpeg1 stuffing (ff ff .. , max 16x)
	for(int i = 0; i < 16 && BitRead(8, true) == 0xff; i++)
	{
		BitRead(8); 
		if(h.len) h.len--;
	}

	h.type = (BYTE)BitRead(2, true) == mpeg2 ? mpeg2 : mpeg1;

	if(h.type == mpeg1)
	{
		BYTE b = (BYTE)BitRead(2);

		if(b == 1)
		{
			h.std_buff_size = (BitRead(1)?1024:128)*BitRead(13);
			if(h.len) h.len -= 2;
			b = (BYTE)BitRead(2);
		}
		
		if(b == 0)
		{
			h.fpts = (BYTE)BitRead(1);
			h.fdts = (BYTE)BitRead(1);
		}
	}
	else if(h.type == mpeg2)
	{
		EXECUTE_ASSERT(BitRead(2) == mpeg2);
		h.scrambling = (BYTE)BitRead(2);
		h.priority = (BYTE)BitRead(1);
		h.alignment = (BYTE)BitRead(1);
		h.copyright = (BYTE)BitRead(1);
		h.original = (BYTE)BitRead(1);
		h.fpts = (BYTE)BitRead(1);
		h.fdts = (BYTE)BitRead(1);
		h.escr = (BYTE)BitRead(1);
		h.esrate = (BYTE)BitRead(1);
		h.dsmtrickmode = (BYTE)BitRead(1);
		h.morecopyright = (BYTE)BitRead(1);
		h.crc = (BYTE)BitRead(1);
		h.extension = (BYTE)BitRead(1);
		h.hdrlen = (BYTE)BitRead(8);
	}
	else
	{
		if(h.len) while(h.len-- > 0) BitRead(8);
		return(false);
	}

	if(h.fpts)
	{
		if(h.type == mpeg2)
		{
			BYTE b = (BYTE)BitRead(4);
			if(!(h.fdts && b == 3 || !h.fdts && b == 2)) {ASSERT(0); return(false);}
		}

		h.pts = 0;
		h.pts |= BitRead(3) << 30; MARKER; // 32..30
		h.pts |= BitRead(15) << 15; MARKER; // 29..15
		h.pts |= BitRead(15); MARKER; // 14..0
		h.pts = 10000*h.pts/90;
	}

	if(h.fdts)
	{
		if((BYTE)BitRead(4) != 1) {ASSERT(0); return(false);}

		h.dts = 0;
		h.dts |= BitRead(3) << 30; MARKER; // 32..30
		h.dts |= BitRead(15) << 15; MARKER; // 29..15
		h.dts |= BitRead(15); MARKER; // 14..0
		h.dts = 10000*h.dts/90;
	}

	// skip to the end of header

	if(h.type == mpeg1)
	{
		if(!h.fpts && !h.fdts && BitRead(4) != 0xf) {ASSERT(0); return(false);}

		if(h.len)
		{
			h.len--;
			if(h.pts) h.len -= 4;
			if(h.dts) h.len -= 5;
		}
	}

	if(h.type == mpeg2)
	{
		if(h.len) h.len -= 3+h.hdrlen;

		int left = h.hdrlen;
		if(h.fpts) left -= 5;
		if(h.fdts) left -= 5;
		while(left-- > 0) BitRead(8);
/*
		// mpeg2 stuffing (ff ff .. , max 32x)
		while(BitRead(8, true) == 0xff) {BitRead(8); if(h.len) h.len--;}
		Seek(GetPos()); // put last peeked byte back for Read()

		// FIXME: this doesn't seems to be here, 
		// infact there can be ff's as part of the data 
		// right at the beginning of the packet, which 
		// we should not skip...
*/
	}

	return(true);
}

bool CMpegSplitterFile::Read(seqhdr& h, int len, CMediaType* pmt)
{
	BYTE id;
	if(!(Next(id, len) && id == 0xb3))
		return(false);

	__int64 shpos = GetPos() - 4;

	h.width = (WORD)BitRead(12);
	h.height = (WORD)BitRead(12);
	h.ar = BitRead(4);
    static int ifps[16] = {0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000, 0, 0, 0, 0, 0, 0, 0};
	h.ifps = ifps[BitRead(4)];
	h.bitrate = (DWORD)BitRead(18); MARKER;
	h.vbv = (DWORD)BitRead(10);
	h.constrained = BitRead(1);

	if(h.fiqm = BitRead(1))
		for(int i = 0; i < countof(h.iqm); i++)
			h.iqm[i] = (BYTE)BitRead(8);

	if(h.fniqm = BitRead(1))
		for(int i = 0; i < countof(h.niqm); i++)
			h.niqm[i] = (BYTE)BitRead(8);

	__int64 shlen = GetPos() - shpos;

	static float ar[] = 
	{
		1.0000f,1.0000f,0.6735f,0.7031f,0.7615f,0.8055f,0.8437f,0.8935f,
		0.9157f,0.9815f,1.0255f,1.0695f,1.0950f,1.1575f,1.2015f,1.0000f
	};

	h.arx = (int)((float)h.width / ar[h.ar] + 0.5);
	h.ary = h.height;

	mpeg_t type = mpeg1;

	__int64 shextpos = 0, shextlen = 0;

	if(Next(id, 8) && id == 0xb5) // sequence header ext
	{
		shextpos = GetPos() - 4;

		h.startcodeid = BitRead(4);
		h.profile_levelescape = BitRead(1); // reserved, should be 0
		h.profile = BitRead(3);
		h.level = BitRead(4);
		h.progressive = BitRead(1);
		h.chroma = BitRead(2);
		h.width |= (BitRead(2)<<12);
		h.height |= (BitRead(2)<<12);
		h.bitrate |= (BitRead(12)<<18); MARKER;
		h.vbv |= (BitRead(8)<<10);
		h.lowdelay = BitRead(1);
		h.ifps = (DWORD)(h.ifps * (BitRead(2)+1) / (BitRead(5)+1));

		shextlen = GetPos() - shextpos;

		struct {DWORD x, y;} ar[] = {{h.width,h.height},{4,3},{16,9},{221,100},{h.width,h.height}};
		int i = min(max(h.ar, 1), 5)-1;
		h.arx = ar[i].x;
		h.ary = ar[i].y;

		type = mpeg2;
	}

	h.ifps = 10 * h.ifps / 27;
	h.bitrate *= 400;

	DWORD a = h.arx, b = h.ary;
    while(a) {DWORD tmp = a; a = b % tmp; b = tmp;}
	if(b) h.arx /= b, h.ary /= b;

	if(!pmt) return(true);

	pmt->majortype = MEDIATYPE_Video;

	if(type == mpeg1)
	{
		pmt->subtype = MEDIASUBTYPE_MPEG1Payload;
		pmt->formattype = FORMAT_MPEGVideo;
		int len = FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader) + shlen + shextlen;
		MPEG1VIDEOINFO* vi = (MPEG1VIDEOINFO*)new BYTE[len];
		memset(vi, 0, len);
		vi->hdr.dwBitRate = h.bitrate;
		vi->hdr.AvgTimePerFrame = h.ifps;
		vi->hdr.bmiHeader.biSize = sizeof(vi->hdr.bmiHeader);
		vi->hdr.bmiHeader.biWidth = h.width;
		vi->hdr.bmiHeader.biHeight = h.height;
		vi->hdr.bmiHeader.biXPelsPerMeter = h.width * h.ary;
		vi->hdr.bmiHeader.biYPelsPerMeter = h.height * h.arx;
		vi->cbSequenceHeader = shlen;
		Seek(shpos);
		Read((BYTE*)&vi->bSequenceHeader[0], shlen);
		if(shextpos && shextlen) Seek(shextpos);
		Read((BYTE*)&vi->bSequenceHeader[0] + shlen, shextlen);
		pmt->SetFormat((BYTE*)vi, len);
		delete [] vi;
	}
	else if(type == mpeg2)
	{
		pmt->subtype = MEDIASUBTYPE_MPEG2_VIDEO;
		pmt->formattype = FORMAT_MPEG2_VIDEO;
		int len = FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + shlen + shextlen;
		MPEG2VIDEOINFO* vi = (MPEG2VIDEOINFO*)new BYTE[len];
		memset(vi, 0, len);
		vi->hdr.dwBitRate = h.bitrate;
		vi->hdr.AvgTimePerFrame = h.ifps;
		vi->hdr.dwPictAspectRatioX = h.arx;
		vi->hdr.dwPictAspectRatioY = h.ary;
		vi->hdr.bmiHeader.biSize = sizeof(vi->hdr.bmiHeader);
		vi->hdr.bmiHeader.biWidth = h.width;
		vi->hdr.bmiHeader.biHeight = h.height;
		vi->dwProfile = h.profile;
		vi->dwLevel = h.level;
		vi->cbSequenceHeader = shlen;
		Seek(shpos);
		Read((BYTE*)&vi->dwSequenceHeader[0], shlen);
		if(shextpos && shextlen) Seek(shextpos);
		Read((BYTE*)&vi->dwSequenceHeader[0] + shlen, shextlen);
		pmt->SetFormat((BYTE*)vi, len);
		delete [] vi;
	}
	else
	{
		return(false);
	}

	return(true);
}

bool CMpegSplitterFile::Read(mpahdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

//	for(; len >= 4 && BitRead(11, true) != 0x7ff; len--) // this misdetects sometimes, lets hope no one will use v2.5 in mpegs... 
	for(; len >= 4 && BitRead(12, true) != 0xfff; len--)
		BitRead(8);

	if(len < 4)
		return(false);

	h.sync = BitRead(11);
	h.version = BitRead(2);
	h.layer = BitRead(2);
	h.crc = BitRead(1);
	h.bitrate = BitRead(4);
	h.freq = BitRead(2);
	h.padding = BitRead(1);
	h.privatebit = BitRead(1);
	h.channels = BitRead(2);
	h.modeext = BitRead(2);
	h.copyright = BitRead(1);
	h.original = BitRead(1);
	h.emphasis = BitRead(1);

	if(h.version == 1 || h.layer == 0 || h.freq == 3 || h.bitrate == 15)
		return(false);

	h.layer = 4 - h.layer;

	if(!pmt) return(true);

	/*int*/ len = h.layer == 3 
		? sizeof(WAVEFORMATEX/*MPEGLAYER3WAVEFORMAT*/) // no need to overcomplicate this...
		: sizeof(MPEG1WAVEFORMAT);
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)new BYTE[len];
	memset(wfe, 0, len);
	wfe->cbSize = len - sizeof(WAVEFORMATEX);

	static int brtbl[][5] = 
	{
		{0,0,0,0,0},
		{32,32,32,32,8},
		{64,48,40,48,16},
		{96,56,48,56,24},
		{128,64,56,64,32},
		{160,80,64,80,40},
		{192,96,80,96,48},
		{224,112,96,112,56},
		{256,128,112,128,64},
		{288,160,128,144,80},
		{320,192,160,160,96},
		{352,224,192,176,112},
		{384,256,224,192,128},
		{416,320,256,224,144},
		{448,384,320,256,160},
		{0,0,0,0,0},
	};

	static int brtblcol[][4] = {{0,3,4,4},{0,0,1,2}};
	int bitrate = 1000*brtbl[h.bitrate][brtblcol[h.version&1][h.layer]];

	if(h.layer == 3)
	{
		wfe->wFormatTag = WAVE_FORMAT_MP3;

/*		MPEGLAYER3WAVEFORMAT* f = (MPEGLAYER3WAVEFORMAT*)wfe;
		f->wfx.wFormatTag = WAVE_FORMAT_MP3;
		f->wID = MPEGLAYER3_ID_UNKNOWN;
		f->fdwFlags = h.padding ? MPEGLAYER3_FLAG_PADDING_ON : MPEGLAYER3_FLAG_PADDING_OFF; // _OFF or _ISO ?
*/
	}
	else
	{
		MPEG1WAVEFORMAT* f = (MPEG1WAVEFORMAT*)wfe;
		f->wfx.wFormatTag = WAVE_FORMAT_MPEG;
		f->fwHeadMode = 1 << h.channels;
		f->fwHeadModeExt = 1 << h.modeext;
		f->wHeadEmphasis = h.emphasis+1;
		if(h.privatebit) f->fwHeadFlags |= ACM_MPEG_PRIVATEBIT;
		if(h.copyright) f->fwHeadFlags |= ACM_MPEG_COPYRIGHT;
		if(h.original) f->fwHeadFlags |= ACM_MPEG_ORIGINALHOME;
		if(h.crc == 0) f->fwHeadFlags |= ACM_MPEG_PROTECTIONBIT;
		if(h.version == 3) f->fwHeadFlags |= ACM_MPEG_ID_MPEG1;
		f->fwHeadLayer = 1 << (h.layer-1);
		f->dwHeadBitrate = bitrate;
	}

	wfe->nChannels = h.channels == 3 ? 1 : 2;

	static int freq[][4] = {{11025,0,22050,44100},{12000,0,24000,48000},{8000,0,16000,32000}};
	wfe->nSamplesPerSec = freq[h.freq][h.version];

	wfe->nBlockAlign = h.layer == 1
		? (12 * bitrate / freq[h.freq][h.version] + h.padding) * 4
		: 144 * bitrate / freq[h.freq][h.version] + h.padding;

	wfe->nAvgBytesPerSec = bitrate / 8;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = FOURCCMap(wfe->wFormatTag);
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

	delete [] wfe;

	return(true);
}

bool CMpegSplitterFile::Read(aachdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	for(; len >= 7 && BitRead(12, true) != 0xfff; len--)
		BitRead(8);

	if(len < 7)
		return(false);

	h.sync = BitRead(12);
	h.version = BitRead(1);
	h.layer = BitRead(2);
	h.fcrc = BitRead(1);
	h.profile = BitRead(2);
	h.freq = BitRead(4);
	h.privatebit = BitRead(1);
	h.channels = BitRead(3);
	h.original = BitRead(1);
	h.home = BitRead(1);

	h.copyright_id_bit = BitRead(1);
	h.copyright_id_start = BitRead(1);
	h.aac_frame_length = BitRead(13);
	h.adts_buffer_fullness = BitRead(11);
	h.no_raw_data_blocks_in_frame = BitRead(2);

	if(h.layer != 0 || h.freq >= 12)
		return(false);

	if(!pmt) return(true);

	// TODO

	WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(wfe));
	wfe.wFormatTag = WAVE_FORMAT_AAC;
	wfe.nChannels = h.channels <= 6 ? h.channels : 2;
    static int freq[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};
	wfe.nSamplesPerSec = freq[h.freq];

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = FOURCCMap(wfe.wFormatTag);
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	return(true);
}

bool CMpegSplitterFile::Read(ac3hdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	for(; len >= 7 && BitRead(16, true) != 0x0b77; len--)
		BitRead(8);

	if(len < 7)
		return(false);

	h.sync = (WORD)BitRead(16);
	h.crc1 = (WORD)BitRead(16);
	h.fscod = BitRead(2);
	h.frmsizecod = BitRead(6);
	h.bsid = BitRead(5);
	h.bsmod = BitRead(3);
	h.acmod = BitRead(3);
	if((h.acmod & 1) && h.acmod != 1) h.cmixlev = BitRead(2);
	if(h.acmod & 4) h.surmixlev = BitRead(2);
	if(h.acmod == 2) h.dsurmod = BitRead(2);
	h.lfeon = BitRead(1);

	if(h.bsid >= 12 || h.fscod == 3 || h.frmsizecod >= 38)
		return(false);

	if(!pmt) return(true);

	WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(wfe));
	wfe.wFormatTag = WAVE_FORMAT_DOLBY_AC3;

	static int channels[] = {2, 1, 2, 3, 3, 4, 4, 5};
	wfe.nChannels = channels[h.acmod] + h.lfeon;

	static int freq[] = {48000, 44100, 32000, 0};
	wfe.nSamplesPerSec = freq[h.fscod];

	switch(h.bsid)
	{
	case 9: wfe.nSamplesPerSec >>= 1; break;
	case 10: wfe.nSamplesPerSec >>= 2; break;
	case 11: wfe.nSamplesPerSec >>= 3; break;
	default: break;
	}

	static int rate[] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};

	wfe.nAvgBytesPerSec = rate[h.frmsizecod>>1] * 1000 / 8;
	wfe.nBlockAlign = (WORD)(1536 * wfe.nAvgBytesPerSec / wfe.nSamplesPerSec);

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_DOLBY_AC3;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	return(true);
}

bool CMpegSplitterFile::Read(dtshdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	for(; len >= 10 && BitRead(32, true) != 0x7ffe8001; len--)
		BitRead(8);

	if(len < 10)
		return(false);

	h.sync = (DWORD)BitRead(32);
	h.frametype = BitRead(1);
	h.deficitsamplecount = BitRead(5);
	h.fcrc = BitRead(1);
	h.nblocks = BitRead(7);
	h.framebytes = (WORD)BitRead(14)+1;
	h.amode = BitRead(6);
	h.sfreq = BitRead(6);
	h.rate = BitRead(5);

	if(!pmt) return(true);

	WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(wfe));
	wfe.wFormatTag = WAVE_FORMAT_DVD_DTS;

	static int channels[] = {1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8};
	if(h.amode < countof(channels)) wfe.nChannels = channels[h.amode];

	static int freq[] = {0,8000,16000,32000,0,0,11025,22050,44100,0,0,12000,24000,48000,0,0};
	wfe.nSamplesPerSec = freq[h.sfreq];

	static int rate[] = 
	{
		32000,56000,64000,96000,112000,128000,192000,224000,
		256000,320000,384000,448000,512000,576000,640000,754500,
		960000,1024000,1152000,1280000,1344000,1408000,1411200,1472000,
		1509750,1920000,2048000,3072000,3840000,0,0,0
	};
	
	wfe.nAvgBytesPerSec = rate[h.rate] * 1000 / 8;
	wfe.nBlockAlign = h.framebytes;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_DTS;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	return(true);
}

bool CMpegSplitterFile::Read(lpcmhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	h.emphasis = BitRead(1);
	h.mute = BitRead(1);
	h.reserved1 = BitRead(1);
	h.framenum = BitRead(5);
	h.quantwordlen = BitRead(2);
	h.freq = BitRead(2);
	h.reserved2 = BitRead(1);
	h.channels = BitRead(3);
	h.drc = (BYTE)BitRead(8);

	if(!pmt) return(true);

	WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(wfe));
	wfe.wFormatTag = WAVE_FORMAT_PCM;
	wfe.nChannels = h.channels+1;
	static int freq[] = {48000, 96000, 44100, 32000};
	wfe.nSamplesPerSec = freq[h.freq];
	wfe.wBitsPerSample = 16;
	wfe.nBlockAlign = wfe.nChannels*wfe.wBitsPerSample>>3;
	wfe.nAvgBytesPerSec = wfe.nBlockAlign*wfe.nSamplesPerSec;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_DVD_LPCM_AUDIO;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));

	// TODO: what to do with dvd-audio lpcm?

	return(true);
}

bool CMpegSplitterFile::Read(dvdspuhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return(true);

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_DVD_SUBPICTURE;
	pmt->formattype = FORMAT_None;

	return(true);
}

bool CMpegSplitterFile::Read(svcdspuhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return(true);

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_SVCD_SUBPICTURE;
	pmt->formattype = FORMAT_None;

	return(true);
}

bool CMpegSplitterFile::Read(cvdspuhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if(!pmt) return(true);

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = MEDIASUBTYPE_CVD_SUBPICTURE;
	pmt->formattype = FORMAT_None;

	return(true);
}

bool CMpegSplitterFile::Read(trhdr& h, bool fSync)
{
	memset(&h, 0, sizeof(h));

	BitByteAlign();

	if(fSync)
	{
		bool fDoubleCheck = BitRead(8, true) != 0x47;

		for(int i = 0; i < 188; i++)
		{
			if(BitRead(8, true) == 0x47)
			{
				if(!fDoubleCheck)
					break;

				Seek(GetPos()+188);
				if(BitRead(8, true) == 0x47)
				{
					Seek(GetPos()-188);
					break;
				}
			}

			BitRead(8);

			if(i == 187)
				return(false);
		}
/*
		for(int i = 0; i < 188 && BitRead(8, true) != 0x47; i++)
			BitRead(8);

		if(BitRead(8, true) == 0x47 && fDoubleCheck)
		{
			Seek(GetPos()+188);
			if(BitRead(8) != 0x47) return(false);
			Seek(GetPos()-189);
		}
*/	}

	if(BitRead(8, true) != 0x47)
		return(false);
ASSERT(GetPos()!=0x59228);
	h.sync = (BYTE)BitRead(8);
	h.error = BitRead(1);
	h.payloadstart = BitRead(1);
	h.transportpriority = BitRead(1);
	h.pid = BitRead(13);
	h.scrambling = BitRead(2);
	h.adapfield = BitRead(1);
	h.payload = BitRead(1);
	h.counter = BitRead(4);

	h.bytes = 184;

	if(h.adapfield)
	{
		h.length = (BYTE)BitRead(8);
		h.discontinuity = BitRead(1);
		h.randomaccess = BitRead(1);
		h.priority = BitRead(1);
		h.PCR = BitRead(1);
		h.OPCR = BitRead(1);
		h.splicingpoint = BitRead(1);
		h.privatedata = BitRead(1);
		h.extension = BitRead(1);
		if(!(0 < h.length && h.length <= 183))
			return(false);
		for(int i = 1; i < h.length; i++)
			BitRead(8);

		h.bytes = 183 - h.length;
	}

	return(true);
}
