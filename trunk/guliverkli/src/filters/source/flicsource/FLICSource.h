#pragma once

#include <atlbase.h>
#include <afxtempl.h>

#pragma pack(push, 1)
enum
{
	FLIC_256_COLOR = 4,
	FLIC_DELTA = 7,
	FLIC_64_COLOR = 11,
	FLIC_LC = 12,
	FLIC_BLACK = 13,
	FLIC_BRUN = 15,
	FLIC_COPY = 16,
	FLIC_MINI = 18
};
	
struct FLIC
{
	DWORD size;
	WORD id; // 0xaf11 or 0xaf12
	WORD frames, x, y, bpp;
	WORD flags, ticks;
	DWORD next, frit;
	BYTE reserved[102];
};

struct FLIC_PREFIX
{
	DWORD size;
	WORD id; // 0xf100
	WORD chunks;
	BYTE reserved[8];
};

struct FLIC_FRAME
{
	DWORD size;
	WORD id; // 0xf1fa
	WORD chunks;
	BYTE reserved[8];
};

struct FLIC_CHUNK
{
	DWORD size;
	WORD type;
};
#pragma pack(pop)

struct FLIC_FRAME_ENTRY
{
	__int64 pos;
	bool fKeyframe;
	FLIC_FRAME hdr;
};

[uuid("17DB5CF6-39BB-4d5b-B0AA-BEBA44673AD4")]
class CFLICSource
	: public CSource
	, public IFileSourceFilter
	, public IAMFilterMiscFlags
{
	CStringW m_fn;

public:
	CFLICSource(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CFLICSource();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IAMFilterMiscFlags
	STDMETHODIMP_(ULONG) GetMiscFlags();
};

class CFLICStream 
	: public CSourceStream
	, public CSourceSeeking
{
	CFile m_flic;
	FLIC m_hdr;
	CArray<FLIC_FRAME_ENTRY> m_frames;

	CCritSec m_cSharedState;

	REFERENCE_TIME m_AvgTimePerFrame;
	REFERENCE_TIME m_rtSampleTime, m_rtPosition;

	BOOL m_bDiscontinuity, m_bFlushing;

	HRESULT OnThreadStartPlay();
	HRESULT OnThreadCreate();

	void UpdateFromSeek();
	STDMETHODIMP SetRate(double dRate);

	HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate() {return S_OK;}

private:
	int m_nLastFrameNum;
	DWORD m_pPalette[256];
	CAutoVectorPtr<BYTE> m_pFrameBuffer;

	void SeekToNearestKeyFrame(int nFrame);
	void ExtractFrame(int nFrame);
	void _blackchunk();
	void _copychunk();
	bool _colorchunk(bool f64);
	void _brunchunk();
	void _lcchunk();
	void _deltachunk();

public:
    CFLICStream(const WCHAR* wfn, CFLICSource* pParent, HRESULT* phr);
	virtual ~CFLICStream();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT FillBuffer(IMediaSample* pSample);
	HRESULT CheckConnect(IPin* pPin);
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	
	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};

