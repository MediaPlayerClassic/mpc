#pragma once

/*  AC3 audio

    wFormatTag          WAVE_FORMAT_DOLBY_AC3
    nChannels           1 -6 channels valid
    nSamplesPerSec      48000, 44100, 32000
    nAvgByesPerSec      4000 to 80000
    nBlockAlign         128 - 3840
    wBitsPerSample      Up to 24 bits - (in the original)

*/

typedef struct tagDOLBYAC3WAVEFORMAT
{
    WAVEFORMATEX     wfx;
    BYTE             bBigEndian;       // TRUE = Big Endian, FALSE little endian
    BYTE             bsid;
    BYTE             lfeon;
    BYTE             copyrightb;
    BYTE             nAuxBitsCode;  //  Aux bits per frame
} DOLBYAC3WAVEFORMAT;

//
// CAVI2AC3Filter
//

[uuid("93230DD0-7B3C-4efb-AFBB-DC380FEC9E6B")]
class CAVI2AC3Filter : public CTransformFilter
{
	bool CheckAC3(const CMediaType* pmt);
	bool CheckDTS(const CMediaType* pmt);
	bool CheckWAVEAC3(const CMediaType* pmt);
	bool CheckWAVEDTS(const CMediaType* pmt);

public:
	CAVI2AC3Filter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CAVI2AC3Filter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
};
