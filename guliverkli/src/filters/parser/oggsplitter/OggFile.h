#pragma once

#pragma pack(push, 1)
struct OggPageHeader
{
	DWORD capture_pattern;
	BYTE stream_structure_version;
	BYTE header_type_flag; enum {continued=1, first=2, last=4};
	__int64 granule_position;
	DWORD bitstream_serial_number;
	DWORD page_sequence_number;
	DWORD CRC_checksum;
	BYTE number_page_segments;
};
struct OggVorbisIdHeader
{
	DWORD vorbis_version;
	BYTE audio_channels;
	DWORD audio_sample_rate;
	DWORD bitrate_maximum;
	DWORD bitrate_nominal;
	DWORD bitrate_minimum;
	BYTE blocksize_0:4;
	BYTE blocksize_1:4;
	BYTE framing_flag;
};
struct OggVideoHeader
{
	DWORD w, h;
};
struct OggAudioHeader
{
	WORD nChannels, nBlockAlign;
	DWORD nAvgBytesPerSec;
};
struct OggStreamHeader
{
	char streamtype[8], subtype[4];

	DWORD size;
	__int64 time_unit, samples_per_unit;
	DWORD default_len;
    DWORD buffersize;
	DWORD bps;

    union 
	{
		OggVideoHeader v;
		OggAudioHeader a;
    };
};
#pragma pack(pop)

class OggPage : public CArray<BYTE>
{
public:
	OggPageHeader m_hdr;
	CList<int> m_lens;
	OggPage() {memset(&m_hdr, 0, sizeof(m_hdr));}
};

class COggFile
{
	CComPtr<IAsyncReader> m_pReader;
	UINT64 m_pos, m_len;

	HRESULT Init();

public:
	COggFile(IAsyncReader* pReader, HRESULT& hr);
	virtual ~COggFile();

	UINT64 GetPos() {return m_pos;}
	UINT64 GetLength() {return m_len;}
	void Seek(UINT64 pos) {m_pos = pos;}
	HRESULT Read(BYTE* pData, LONG len);

	bool Sync();
	bool Read(OggPageHeader& hdr);
	bool Read(OggPage& page);
};
