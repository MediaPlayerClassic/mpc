/* 
 *	Copyright (C) 2003 Gabest
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

namespace RealMedia
{
#pragma pack(push, 1)
	typedef struct {union {char id[4]; UINT32 object_id;}; UINT32 size; UINT16 object_version;} ChunkHdr;
	typedef struct {UINT32 version, nHeaders;} FileHdr;
	typedef struct 
	{
		UINT32 maxBitRate, avgBitRate;
		UINT32 maxPacketSize, avgPacketSize, nPackets;
		UINT32 tDuration, tPreroll;
		UINT32 ptrIndex, ptrData;
		UINT16 nStreams;
		enum flags_t {PN_SAVE_ENABLED=1, PN_PERFECT_PLAY_ENABLED=2, PN_LIVE_BROADCAST=4} flags; 
	} Properies;
	typedef struct 
	{
		UINT16 stream;
		UINT32 maxBitRate, avgBitRate;
		UINT32 maxPacketSize, avgPacketSize;
		UINT32 tStart, tPreroll, tDuration;
		CStringA name, mime;
		CArray<BYTE> typeSpecData;
	} MediaProperies;
	typedef struct {CStringA title, author, copyright, comment;} ContentDesc;
	typedef struct {UINT64 pos; UINT32 nPackets, ptrNext;} DataChunk;
	typedef struct 
	{
		UINT16 len, stream;
		UINT32 tStart;
		UINT8 reserved;
		enum flag_t {PN_RELIABLE_FLAG=1, PN_KEYFRAME_FLAG=2} flags; // UINT8
		CArray<BYTE> pData;
	} MediaPacketHeader;
	typedef struct {UINT32 nIndices; UINT16 stream; UINT32 ptrNext;} IndexChunkHeader;
	typedef struct {UINT32 tStart, ptrFilePos, packet;} IndexRecord;
#pragma pack(pop)
}

#pragma pack(push, 1)
struct rvinfo 
{
	DWORD dwSize, fcc1, fcc2; 
	WORD w, h, bpp; 
	DWORD unk1, fps, type1, type2;
	BYTE w2, h2, w3, h3;
};
/*
struct RealMedia_AudioHeader {
	UINT32 unknown1; // No clue
	UINT16 unknown2; // just need to skip 6 bytes
	UINT16 header_version;
	UINT16 unknown3; // 00 00
	UINT32 format; //? .ra4 or .ra5
	UINT32 unknown4; // ???
	UINT16 format_version; // version (4 or 5)
	UINT32 header_size; // header size == 0x4E 
	UINT16 codec_flavor; // codec flavor id, Also called the codec_audience_number
	UINT32 coded_frame_size; // coded frame size, needed by codec
	UINT32 unknown5; // big number
	UINT32 unknown6; // bigger number
	UINT32 unknown7; // 2 || -''-
	UINT16 sub_packet_h; //Sub packet header???
	UINT16 frame_size;
	UINT16 sub_packet_size;
	UINT16 unknown8; //Two bytes of 0's

	//Here if format_version == 5
	UINT32 unknown9;
	UINT16 unknown10;
	//End if format_version == 5

	UINT16 sample_rate;
	UINT16 unknown11;
	UINT16 sample_size;
	UINT16 channels;

	//Here if format_version == 5
	UINT32 unknown12;
	UINT32 codec_name;
	//End if format_version == 5

	//If codec_name == "cook"
	UINT16 unknown13; // Skip 3 unknown bytes 
	UINT8 unknown14;
	UINT8 unknown15;  // Skip 1 additional unknown byte If format_version == 5
	UINT32 codecdata_length;
	UINT8 *codecdata; //This is the size codecdata_length
	//End if codec_name == "cook"
};
*/
#pragma pack(pop)

#define MAXBUFFERS 2
#define MAXPACKETS 100

typedef struct
{
	DWORD TrackNumber;
	CArray<BYTE> pData;
	REFERENCE_TIME rtStart, rtStop;
	BOOL bSyncPoint;
} RMBlock;

class CRMFile
{
	CComPtr<IAsyncReader> m_pReader;
	UINT64 m_pos, m_len;

	HRESULT Init();

public:
	CRMFile(IAsyncReader* pReader, HRESULT& hr);
	UINT64 GetPos() {return m_pos;}
	UINT64 GetLength() {return m_len;}
	void Seek(UINT64 pos) {m_pos = pos;}
	HRESULT Read(BYTE* pData, LONG len);
	template<typename T> HRESULT Read(T& var);
	HRESULT Read(RealMedia::ChunkHdr& hdr);
	HRESULT Read(RealMedia::MediaPacketHeader& mph, bool fFull = true);

	RealMedia::FileHdr m_fh;
	RealMedia::ContentDesc m_cd;
	RealMedia::Properies m_p;
	CAutoPtrList<RealMedia::MediaProperies> m_mps;
	CAutoPtrList<RealMedia::DataChunk> m_dcs;
	CAutoPtrList<RealMedia::IndexRecord> m_irs;
};

class CRealMediaSplitterInputPin : public CBasePin
{
	CComQIPtr<IAsyncReader> m_pAsyncReader;

public:
	CRealMediaSplitterInputPin(TCHAR* pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CRealMediaSplitterInputPin();

	HRESULT GetAsyncReader(IAsyncReader** ppAsyncReader);

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT CheckMediaType(const CMediaType* pmt);

    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pPin);

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
};

class CRealMediaSplitterOutputPin : public CBaseOutputPin, protected CAMThread
{
	CArray<CMediaType> m_mts;
	void DontGoWild();

public:
	enum {EMPTY, EOS, BLOCK};
	typedef struct {int type; CAutoPtr<RMBlock> b; BOOL bDiscontinuity;} packet;
	HRESULT DeliverBlock(CAutoPtr<RMBlock> b, BOOL bDiscontinuity);

private:
	CCritSec m_csQueueLock;
	CAutoPtrList<packet> m_packets;
	HRESULT m_hrDeliver;
	enum {CMD_EXIT};
    DWORD ThreadProc();

public:
	CRealMediaSplitterOutputPin(CArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CRealMediaSplitterOutputPin();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);

    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	CMediaType& CurrentMediaType() {return m_mt;}

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

	// Queueing

	HRESULT Active();
    HRESULT Inactive();

    HRESULT DeliverEndOfStream();
    HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

[uuid("765035B3-5944-4A94-806B-20EE3415F26F")]
class CRealMediaSourceFilter 
	: public CBaseFilter
	, public CCritSec
	, protected CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IAMOpenProgress
{
	class CFileReader : public CUnknown, public IAsyncReader
	{
		CFile m_file;

	public:
		CFileReader(CString fn, HRESULT& hr);

		DECLARE_IUNKNOWN;
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		// IAsyncReader

		STDMETHODIMP RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) {return E_NOTIMPL;}
        STDMETHODIMP Request(IMediaSample* pSample, DWORD_PTR dwUser) {return E_NOTIMPL;}
        STDMETHODIMP WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) {return E_NOTIMPL;}
		STDMETHODIMP SyncReadAligned(IMediaSample* pSample) {return E_NOTIMPL;}
		STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer);
		STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable);
		STDMETHODIMP BeginFlush() {return E_NOTIMPL;}
		STDMETHODIMP EndFlush() {return E_NOTIMPL;}
	};

	CStringW m_fn;
	CAMEvent m_eEndFlush;

	LONGLONG m_nOpenProgress;
	bool m_fAbort;

protected:
	CAutoPtr<CRealMediaSplitterInputPin> m_pInput;
	CAutoPtrList<CRealMediaSplitterOutputPin> m_pOutputs;

	CAutoPtr<CRMFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	CMap<DWORD, DWORD, CRealMediaSplitterOutputPin*, CRealMediaSplitterOutputPin*> m_mapTrackToPin;

	CCritSec m_csSend;

	REFERENCE_TIME m_rtStart, m_rtStop, m_rtCurrent, m_rtNewStart, m_rtNewStop;
	double m_dRate;

	CList<UINT64> m_bDiscontinuitySent;
	CList<CBaseOutputPin*> m_pActivePins;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	HRESULT DeliverBlock(CAutoPtr<RMBlock> b);

protected:
	enum {CMD_EXIT, CMD_SEEK};
    DWORD ThreadProc();

public:
	CRealMediaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CRealMediaSourceFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT BreakConnect(PIN_DIRECTION dir, CBasePin* pPin);
	HRESULT CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IMediaSeeking

	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	// IAMOpenProgress

	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
	STDMETHODIMP AbortOperation();
};

[uuid("E21BE468-5C18-43EB-B0CC-DB93A847D769")]
class CRealMediaSplitterFilter : public CRealMediaSourceFilter
{
public:
	CRealMediaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};

////////////

[uuid("238D0F23-5DC9-45A6-9BE2-666160C324DD")]
class CRealVideoDecoder : public CTransformFilter
{
	typedef HRESULT (WINAPI *PRVCustomMessage)(void*, DWORD);
	typedef HRESULT (WINAPI *PRVFree)(DWORD);
	typedef HRESULT (WINAPI *PRVHiveMessage)(DWORD, DWORD);
	typedef HRESULT (WINAPI *PRVInit)(void*, DWORD* dwCookie);
	typedef HRESULT (WINAPI *PRVTransform)(BYTE*, BYTE*, void*, void*, DWORD);

	PRVCustomMessage RVCustomMessage;
	PRVFree RVFree;
	PRVHiveMessage RVHiveMessage;
	PRVInit RVInit;
	PRVTransform RVTransform;

	HMODULE m_hDrvDll;
	DWORD m_dwCookie;

	HRESULT InitRV(const CMediaType* pmt);
	void FreeRV();

	REFERENCE_TIME m_rtLast, m_tStart;
	DWORD m_packetlen;
	typedef struct {CByteArray data; DWORD offset;} chunk;
	CAutoPtrList<chunk> m_data;

	HRESULT Decode(bool fPreroll);
	void Copy(BYTE* pIn, BYTE* pOut, int w, int h);

	CAutoVectorPtr<BYTE> m_pI420FrameBuff;

public:
	CRealVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CRealVideoDecoder();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
	HRESULT Receive(IMediaSample* pIn);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

    HRESULT EndOfStream();
    HRESULT BeginFlush();
    HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};



