#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <afxtempl.h>
#include "..\BaseSplitter\BaseSplitter.h"

class CMpegSplitterFile : public CBaseSplitterFile
{
	int m_tslen;

	CMap<WORD,WORD,BYTE,BYTE> m_pid2pes;

	HRESULT Init();

public:
	CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr);

	using CBaseSplitterFile::Read;
	bool Next(BYTE& b, __int64 len = 65536);
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
		operator == (const struct stream& s) const {return (DWORD)*this == (DWORD)s;}
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

#pragma pack(push, 1)

	enum mpeg_t {unk, mpeg1, mpeg2};

	struct pshdr
	{
		mpeg_t type;
		UINT64 scr, bitrate;
	};

	struct pssyshdr
	{
		DWORD rate_bound;
		BYTE video_bound, audio_bound;
		bool fixed_rate, csps;
		bool sys_video_loc_flag, sys_audio_loc_flag;
	};

	struct peshdr
	{
		WORD len;

		BYTE type:2, fpts:1, fdts:1;
		REFERENCE_TIME pts, dts;

		// mpeg1 stuff
		UINT64 std_buff_size;

		// mpeg2 stuff
		BYTE scrambling:2, priority:1, alignment:1, copyright:1, original:1;
		BYTE escr:1, esrate:1, dsmtrickmode:1, morecopyright:1, crc:1, extension:1;
		BYTE hdrlen;

		struct peshdr() {memset(this, 0, sizeof(*this));}
	};

	class seqhdr
	{
	public:
		WORD width;
		WORD height;
		BYTE ar:4;
		DWORD ifps;
		DWORD bitrate;
		DWORD vbv;
		BYTE constrained:1;
		BYTE fiqm:1;
		BYTE iqm[64];
		BYTE fniqm:1;
		BYTE niqm[64];
		// ext
		BYTE startcodeid:4;
		BYTE profile_levelescape:1;
		BYTE profile:3;
		BYTE level:4;
		BYTE progressive:1;
		BYTE chroma:2;
		BYTE lowdelay:1;
		// misc
		int arx, ary;
	};

	class mpahdr
	{
	public:
		WORD sync:11;
		WORD version:2;
		WORD layer:2;
		WORD crc:1;
		WORD bitrate:4;
		WORD freq:2;
		WORD padding:1;
		WORD privatebit:1;
		WORD channels:2;
		WORD modeext:2;
		WORD copyright:1;
		WORD original:1;
		WORD emphasis:2;
	};

	class aachdr
	{
	public:
		WORD sync:12;
		WORD version:1;
		WORD layer:2;
		WORD fcrc:1;
		WORD profile:2;
		WORD freq:4;
		WORD privatebit:1;
		WORD channels:3;
		WORD original:1;
		WORD home:1; // ?

		WORD copyright_id_bit:1;
		WORD copyright_id_start:1;
		WORD aac_frame_length:13;
		WORD adts_buffer_fullness:11;
		WORD no_raw_data_blocks_in_frame:2;
	};

	class ac3hdr
	{
	public:
		WORD sync;
		WORD crc1;
		BYTE fscod:2;
		BYTE frmsizecod:6;
		BYTE bsid:5;
		BYTE bsmod:3;
		BYTE acmod:3;
		BYTE cmixlev:2;
		BYTE surmixlev:2;
		BYTE dsurmod:2;
		BYTE lfeon:1;
		// the rest is unimportant for us
	};

	class dtshdr
	{
	public:
		DWORD sync;
		BYTE frametype:1;
		BYTE deficitsamplecount:5;
        BYTE fcrc:1;
		BYTE nblocks:7;
		WORD framebytes;
		BYTE amode:6;
		BYTE sfreq:4;
		BYTE rate:5;
	};

	class lpcmhdr
	{
	public:
		BYTE emphasis:1;
		BYTE mute:1;
		BYTE reserved1:1;
		BYTE framenum:5;
		BYTE quantwordlen:2;
		BYTE freq:2; // 48, 96, 44.1, 32
		BYTE reserved2:1;
		BYTE channels:3; // +1
		BYTE drc; // 0x80: off
	};

	class dvdspuhdr
	{
	public:
		// nothing ;)
	};
	
	class svcdspuhdr
	{
	public:
		// nothing ;)
	};

	class cvdspuhdr
	{
	public:
		// nothing ;)
	};

	class ps2audhdr
	{
	public:
		// 'SShd' + len (0x18)
		DWORD unk1;
		DWORD freq;
		DWORD channels;
		DWORD interleave; // bytes per channel
		// padding: FF .. FF
		// 'SSbd' + len
		// pcm or adpcm data
	};

	class ps2subhdr
	{
	public:
		// nothing ;)
	};

	struct trhdr
	{
		BYTE sync; // 0x47
		BYTE error:1;
		BYTE payloadstart:1;
		BYTE transportpriority:1;
		WORD pid:13;
		BYTE scrambling:2;
		BYTE adapfield:1;
		BYTE payload:1;
		BYTE counter:4;
		// if adapfield set
		BYTE length;
		BYTE discontinuity:1;
		BYTE randomaccess:1;
		BYTE priority:1;
		BYTE PCR:1;
		BYTE OPCR:1;
		BYTE splicingpoint:1;
		BYTE privatedata:1;
		BYTE extension:1;
		// TODO: add more fields here when the flags above are set (they aren't very interesting...)

		int bytes;
		__int64 next;
	};

	// http://www.technotrend.de/download/av_format_v1.pdf

	struct pvahdr
	{
		WORD sync; // 'VA'
		BYTE streamid; // 1 - video, 2 - audio
		BYTE counter;
		BYTE res1; // 0x55
		BYTE res2:3;
		BYTE fpts:1;
		BYTE postbytes:2;
		BYTE prebytes:2;
		WORD length;
		REFERENCE_TIME pts;
	};

#pragma pack(pop)

	bool Read(pshdr& h);
	bool Read(pssyshdr& h);
	bool Read(peshdr& h, BYTE code);
	bool Read(seqhdr& h, int len, CMediaType* pmt = NULL);
	bool Read(mpahdr& h, int len, CMediaType* pmt = NULL);
	bool Read(aachdr& h, int len, CMediaType* pmt = NULL);
	bool Read(ac3hdr& h, int len, CMediaType* pmt = NULL);
	bool Read(dtshdr& h, int len, CMediaType* pmt = NULL);
	bool Read(lpcmhdr& h, CMediaType* pmt = NULL);
	bool Read(dvdspuhdr& h, CMediaType* pmt = NULL);
	bool Read(svcdspuhdr& h, CMediaType* pmt = NULL);
	bool Read(cvdspuhdr& h, CMediaType* pmt = NULL);
	bool Read(ps2audhdr& h, CMediaType* pmt = NULL);
	bool Read(ps2subhdr& h, CMediaType* pmt = NULL);
	bool Read(trhdr& h, bool fSync = true);
	bool Read(pvahdr& h, bool fSync = true);
};
