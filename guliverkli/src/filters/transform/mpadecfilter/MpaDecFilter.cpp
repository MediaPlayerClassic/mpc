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

#include "stdafx.h"
#include <atlbase.h>
#include <mmreg.h>
#include "MpaDecFilter.h"

#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Payload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DOLBY_AC3},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DTS},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      countof(sudPinTypesIn), // Number of types
      sudPinTypesIn		// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      countof(sudPinTypesOut), // Number of types
      sudPinTypesOut		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMpaDecFilter), L"MPEG/AC3/LPCM Audio Decoder", 0x40000001, countof(sudpPins), sudpPins},
};

CFactoryTemplate g_Templates[] =
{
    {L"MPEG/AC3/LPCM Audio Decoder", &__uuidof(CMpaDecFilter), CMpaDecFilter::CreateInstance, NULL, &sudFilter[0]},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CMpaDecFilter
//

CUnknown* WINAPI CMpaDecFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMpaDecFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

static struct scmap_t
{
	WORD nChannels;
	BYTE ch[6];
	DWORD dwChannelMask;
}
s_scmap[2*11] =
{
	{2, {0, 1,-1,-1,-1,-1}, 0},	// A52_CHANNEL
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_MONO
	{2, {0, 1,-1,-1,-1,-1}, 0}, // A52_STEREO
	{3, {0, 2, 1,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // A52_3F
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER}, // A52_2F1R
	{4, {0, 2, 1, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER}, // A52_3F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_2F2R
	{5, {0, 2, 1, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_3F2R
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_CHANNEL1
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_CHANNEL2
	{2, {0, 1,-1,-1,-1,-1}, 0}, // A52_DOLBY

	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY},	// A52_CHANNEL|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_MONO|A52_LFE
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // A52_STEREO|A52_LFE
	{4, {1, 3, 2, 0,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_3F|A52_LFE
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // A52_2F1R|A52_LFE
	{5, {1, 3, 2, 0, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // A52_3F1R|A52_LFE
	{5, {1, 2, 0, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_2F2R|A52_LFE
	{6, {1, 3, 2, 0, 4, 5}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_3F2R|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_CHANNEL1|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_CHANNEL2|A52_LFE
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // A52_DOLBY|A52_LFE
};

CMpaDecFilter::CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CMpaDecFilter"), lpunk, __uuidof(this))
	, m_iSampleFormat(SF_PCM16)
	, m_fNormalize(false)
	, m_iSpeakerConfig(A52_STEREO)
	, m_fDynamicRangeControl(false)
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CMpaDecInputPin(this, phr, L"In"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CTransformOutputPin(NAME("CTransformOutputPin"), this, phr, L"Out"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}
}

CMpaDecFilter::~CMpaDecFilter()
{
}

STDMETHODIMP CMpaDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMpaDecFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMpaDecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);
	return __super::EndOfStream();
}

HRESULT CMpaDecFilter::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMpaDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	m_sample_max = 0.1;
	return __super::EndFlush();
}

HRESULT CMpaDecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	m_sample_max = 0.1;
	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMpaDecFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
		m_sample_max = 0.1;
	}

	const GUID& subtype = m_pInput->CurrentMediaType().subtype;

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;

	long len = pIn->GetActualDataLength();

	if(len <= 0) return S_OK;

	if(*(DWORD*)pDataIn == 0xBA010000) // MEDIATYPE_*_PACK
	{
		len -= 14; pDataIn += 14;
		if(int stuffing = (pDataIn[-1]&7)) {len -= stuffing; pDataIn += stuffing;}
	}

	if(len <= 0) return S_OK;

	if(*(DWORD*)pDataIn == 0xBB010000)
	{
		len -= 4; pDataIn += 4;
		int hdrlen = ((pDataIn[0]<<8)|pDataIn[1]) + 2;
		len -= hdrlen; pDataIn += hdrlen;
	}

	if(len <= 0) return S_OK;

	if((*(DWORD*)pDataIn&0xE0FFFFFF) == 0xC0010000 || *(DWORD*)pDataIn == 0xBD010000)
	{
		if(subtype == MEDIASUBTYPE_MPEG1Packet)
		{
			len -= 4+2+7; pDataIn += 4+2+7; // is it always ..+7 ?
		}
		else if(subtype == MEDIASUBTYPE_MPEG2_AUDIO) // can this be after 0x000001BD too?
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1; pDataIn += *pDataIn+1;
		}
		else if(subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1; pDataIn += *pDataIn+1;
			len -= 7; pDataIn += 7;
		}
		else if(subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3
			|| subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_WAVE_DTS)
		{
			len -= 8; pDataIn += 8;
			len -= *pDataIn+1; pDataIn += *pDataIn+1;
			len -= 4; pDataIn += 4;
		}
	}

	if(len <= 0) return S_OK;

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);

	if(pIn->IsDiscontinuity() == S_OK)
	{
		m_fDiscontinuity = true;
		m_buff.RemoveAll();
		m_sample_max = 0.1;
		ASSERT(SUCCEEDED(hr)); // what to do if not?
		if(FAILED(hr)) return S_OK; // lets wait then...
		m_rtStart = rtStart;
	}

	if(SUCCEEDED(hr) && abs(m_rtStart - rtStart) > 1000000) // +-100ms jitter is allowed for now
	{
		m_buff.RemoveAll();
		m_rtStart = rtStart;
	}

	int tmp = m_buff.GetSize();
	m_buff.SetSize(m_buff.GetSize() + len);
	memcpy(m_buff.GetData() + tmp, pDataIn, len);
	len += tmp;

	if(subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
		hr = ProcessLPCM();
	else if(subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
		hr = ProcessAC3();
	else if(subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_WAVE_DTS)
		hr = ProcessDTS();
	else // if(.. the rest ..)
		hr = ProcessMPA();

	return hr;
}

HRESULT CMpaDecFilter::ProcessLPCM()
{
	WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();

	ASSERT(wfein->nChannels == 2);
	ASSERT(wfein->wBitsPerSample == 16);

	BYTE* pDataIn = m_buff.GetData();
	int len = m_buff.GetSize() & ~(wfein->nChannels*wfein->wBitsPerSample/8-1);

	CArray<float> pBuff;
	pBuff.SetSize(len*8/wfein->wBitsPerSample);

	float* pDataOut = pBuff.GetData();
	for(int i = 0; i < len; i += 2, pDataIn += 2, pDataOut++)
		*pDataOut = (float)(short)((pDataIn[0]<<8)|pDataIn[1]) / 0x8000; // FIXME: care about 20/24 bps too

	memmove(m_buff.GetData(), pDataIn, m_buff.GetSize() - len);
	m_buff.SetSize(m_buff.GetSize() - len);

	return Deliver(pBuff, wfein->nSamplesPerSec, wfein->nChannels);
}

HRESULT CMpaDecFilter::ProcessAC3()
{
	BYTE* p = m_buff.GetData();
	BYTE* base = p;
	BYTE* end = p + m_buff.GetSize();

	while(end - p >= 7)
	{
		int size = 0, flags, sample_rate, bit_rate;

		if((size = a52_syncinfo(p, &flags, &sample_rate, &bit_rate)) > 0)
		{
			TRACE(_T("size=%d, flags=%08x, sample_rate=%d, bit_rate=%d\n"), size, flags, sample_rate, bit_rate);

			bool fEnoughData = p + size <= end;

			if(fEnoughData)
			{
				int iSpeakerConfig = GetSpeakerConfig();

				if(iSpeakerConfig < 0)
				{
					CArray<BYTE> pBuff;
					pBuff.SetSize(0x1800); // sizeof(WORD)*4 + size

					WORD* pDataOut = (WORD*)pBuff.GetData();
					pDataOut[0] = 0xf872;
					pDataOut[1] = 0x4e1f;
					pDataOut[2] = 0x0001;
					pDataOut[3] = size*8;
					_swab((char*)p, (char*)&pDataOut[4], size);

					REFERENCE_TIME rtDur = 10000000i64 * size*8 / bit_rate; // should be 320000 * 100ns

					HRESULT hr;
					if(S_OK != (hr = Deliver(pBuff, sample_rate, rtDur)))
						return hr;
				}
				else
				{
					flags = iSpeakerConfig&(A52_CHANNEL_MASK|A52_LFE);
					flags |= A52_ADJUST_LEVEL;

					sample_t level = 1, gain = 1, bias = 0;
					level *= gain;

					if(a52_frame(m_a52_state, p, &flags, &level, bias) == 0)
					{
						if(GetDynamicRangeControl())
							a52_dynrng(m_a52_state, NULL, NULL);

						int scmapidx = min(flags&A52_CHANNEL_MASK, countof(s_scmap)/2);
                        scmap_t& scmap = s_scmap[scmapidx + ((flags&A52_LFE)?(countof(s_scmap)/2):0)];

						CArray<float> pBuff;
						pBuff.SetSize(6*256*scmap.nChannels);
						float* p = pBuff.GetData();

						int i = 0;

						for(; i < 6 && a52_block(m_a52_state) == 0; i++)
						{
							sample_t* samples = a52_samples(m_a52_state);

							for(int j = 0; j < 256; j++, samples++)
							{
								for(int ch = 0; ch < scmap.nChannels; ch++)
								{
									ASSERT(scmap.ch[ch] != -1);
									*p++ = (float)(*(samples + 256*scmap.ch[ch]) / level);
								}
							}
						}

						if(i == 6)
						{
							HRESULT hr;
							if(S_OK != (hr = Deliver(pBuff, sample_rate, scmap.nChannels, scmap.dwChannelMask)))
								return hr;
						}
					}
				}

				p += size;
			}

			memmove(base, p, end - p);
			end = base + (end - p);
			p = base;

			if(!fEnoughData)
				break;
		}
		else
		{
			p++;
		}
	}

	m_buff.SetSize(end - p);

	return S_OK;
}

static int dts_syncinfo(BYTE* p, int* sample_rate, int* bit_rate)
{
	if(*(DWORD*)p != 0x0180FE7F)
		return 0;

	p += 4;

	int frametype = (p[0]>>7); // 1
	int deficitsamplecount = (p[0]>>2)&31; // 5
	int crcpresent = (p[0]>>1)&1; // 1
	int npcmsampleblocks = ((p[0]&1)<<6)|(p[1]>>2); // 7
	int framebytes = (((p[1]&3)<<12)|(p[2]<<4)|(p[3]>>4)) + 1; // 14
	int audiochannelarrangement = (p[3]&15)<<2|(p[4]>>6); // 6
	int freq = (p[4]>>2)&15; // 4
	int transbitrate = ((p[4]&3)<<3)|(p[5]>>5); // 5

	int freqtbl[] = 
	{
		0,8000,16000,32000,
		0,0,
		11025,22050,44100,
		0,0,
		12000,24000,48000,
		0,0
	};

	int bitratetbl[] = 
	{
		32000,56000,64000,96000,112000,128000,192000,224000,
		256000,320000,384000,448000,512000,576000,640000,754500,
		960000,1024000,1152000,1280000,1344000,1408000,1411200,1472000,
		1509750,1920000,2048000,3072000,3840000,0,0,0
	};

	*sample_rate = freqtbl[freq];
	*bit_rate = bitratetbl[transbitrate];

	return framebytes;
}

HRESULT CMpaDecFilter::ProcessDTS()
{
	BYTE* p = m_buff.GetData();
	BYTE* base = p;
	BYTE* end = p + m_buff.GetSize();

	while(end - p >= 10) // ?
	{
		int size = 0, sample_rate, bit_rate;

		if((size = dts_syncinfo(p, &sample_rate, &bit_rate)) > 0)
		{
			TRACE(_T("size=%d, sample_rate=%d, bit_rate=%d\n"), size, sample_rate, bit_rate);

			bool fEnoughData = p + size <= end;

			if(fEnoughData)
			{
				CArray<BYTE> pBuff;
				pBuff.SetSize(0x800); // sizeof(WORD)*4 + size

				WORD* pDataOut = (WORD*)pBuff.GetData();
				pDataOut[0] = 0xf872;
				pDataOut[1] = 0x4e1f;
				pDataOut[2] = 0x000b;
				pDataOut[3] = size*8;
				_swab((char*)p, (char*)&pDataOut[4], size);

				REFERENCE_TIME rtDur = 10000000i64 * size*8 / bit_rate; // should be 106667 * 100 ns

				HRESULT hr;
				if(S_OK != (hr = Deliver(pBuff, sample_rate, rtDur)))
					return hr;

				p += size;
			}

			memmove(base, p, end - p);
			end = base + (end - p);
			p = base;

			if(!fEnoughData)
				break;
		}
		else
		{
			p++;
		}
	}

	m_buff.SetSize(end - p);

	return S_OK;
}

static inline float fscale(mad_fixed_t sample)
{
	if(sample >= MAD_F_ONE) sample = MAD_F_ONE - 1;
	else if(sample < -MAD_F_ONE) sample = -MAD_F_ONE;

	return (float)sample / (1 << MAD_F_FRACBITS);
}

HRESULT CMpaDecFilter::ProcessMPA()
{
	mad_stream_buffer(&m_stream, m_buff.GetData(), m_buff.GetSize());

	while(1)
	{
		if(mad_frame_decode(&m_frame, &m_stream) == -1)
		{
			if(m_stream.error == MAD_ERROR_BUFLEN)
			{
				memmove(m_buff.GetData(), m_stream.this_frame, m_stream.bufend - m_stream.this_frame);
				m_buff.SetSize(m_stream.bufend - m_stream.this_frame);
				break;
			}

			TRACE(_T("*m_stream.error == %d\n"), m_stream.error);

			if(!MAD_RECOVERABLE(m_stream.error))
				return E_FAIL;

			m_fDiscontinuity = true;
			continue;
		}

		mad_synth_frame(&m_synth, &m_frame);

		WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
		if(wfein->nChannels != m_synth.pcm.channels || wfein->nSamplesPerSec != m_synth.pcm.samplerate)
			continue;

		const mad_fixed_t* left_ch   = m_synth.pcm.samples[0];
		const mad_fixed_t* right_ch  = m_synth.pcm.samples[1];

		CArray<float> pBuff;
		pBuff.SetSize(m_synth.pcm.length*m_synth.pcm.channels);

		float* pDataOut = pBuff.GetData();
		for(unsigned short i = 0; i < m_synth.pcm.length; i++)
		{
			*pDataOut++ = fscale(*left_ch++);
			if(m_synth.pcm.channels == 2) *pDataOut++ = fscale(*right_ch++);
		}

		HRESULT hr;
		if(S_OK != (hr = Deliver(pBuff, m_synth.pcm.samplerate, m_synth.pcm.channels)))
			return hr;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData)
{
	HRESULT hr;

	*pData = NULL;
	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(pSample, NULL, NULL, 0))
	|| FAILED(hr = (*pSample)->GetPointer(pData)))
		return hr;

	AM_MEDIA_TYPE* pmt = NULL;
	if(SUCCEEDED((*pSample)->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::Deliver(CArray<float>& pBuff, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	HRESULT hr;

	SampleFormat iSampleFormat = GetSampleFormat();

	CMediaType mt;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = iSampleFormat == SF_FLOAT32 ? MEDIASUBTYPE_IEEE_FLOAT : MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEXTENSIBLE wfex;
	memset(&wfex, 0, sizeof(wfex));
	WAVEFORMATEX* wfe = &wfex.Format;
	wfe->wFormatTag = (WORD)mt.subtype.Data1;
	wfe->nChannels = nChannels;
	wfe->nSamplesPerSec = nSamplesPerSec;
	switch(iSampleFormat)
	{
	default:
	case SF_PCM16: wfe->wBitsPerSample = 16; break;
	case SF_PCM24: wfe->wBitsPerSample = 24; break;
	case SF_PCM32: case SF_FLOAT32: wfe->wBitsPerSample = 32; break;
	}
	wfe->nBlockAlign = wfe->nChannels*wfe->wBitsPerSample/8;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec*wfe->nBlockAlign;

	// FIXME: 24/32 bit only seems to work with WAVE_FORMAT_EXTENSIBLE
	if(dwChannelMask == 0 && (iSampleFormat == SF_PCM24 || iSampleFormat == SF_PCM32))
		dwChannelMask = nChannels == 2 ? (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT) : SPEAKER_FRONT_CENTER;

	if(dwChannelMask)
	{
		wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfex.Format.cbSize = sizeof(wfex) - sizeof(wfex.Format);
		wfex.dwChannelMask = dwChannelMask;
		wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;
		wfex.SubFormat = mt.subtype;
	}

	mt.SetFormat((BYTE*)&wfex, sizeof(wfex.Format) + wfex.Format.cbSize);

	int nSamples = pBuff.GetSize()/wfe->nChannels;

	if(FAILED(hr = ReconnectOutput(nSamples, mt)))
		return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(GetDeliveryBuffer(&pOut, &pDataOut)))
		return E_FAIL;

	REFERENCE_TIME rtDur = 10000000i64*nSamples/wfe->nSamplesPerSec;
	REFERENCE_TIME rtStart = m_rtStart, rtStop = m_rtStart + rtDur;
	m_rtStart += rtDur;
TRACE(_T("CMpaDecFilter: %I64d - %I64d\n"), rtStart/10000, rtStop/10000);
	if(rtStart < 200000 /* < 0, FIXME: 0 makes strange noises */)
		return S_OK;

	if(hr == S_OK)
	{
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_fDiscontinuity); m_fDiscontinuity = false;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(pBuff.GetSize()*wfe->wBitsPerSample/8);

WAVEFORMATEX* wfeout = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();
ASSERT(wfeout->nChannels == wfe->nChannels);
ASSERT(wfeout->nSamplesPerSec == wfe->nSamplesPerSec);

	float* pDataIn = pBuff.GetData();

	// TODO: move this into the audio switcher
	float sample_mul = 1;
	if(m_fNormalize)
	{
		for(int i = 0, len = pBuff.GetSize(); i < len; i++)
		{
			float f = *pDataIn++;
			if(f < 0) f = -f;
			if(m_sample_max < f) m_sample_max = f;
		}
		sample_mul = 1.0 / m_sample_max;
		pDataIn = pBuff.GetData();
	}

	for(int i = 0, len = pBuff.GetSize(); i < len; i++)
	{
		float f = *pDataIn++;

		ASSERT(f >= -1 && f <= 1);

		// TODO: move this into the audio switcher
		if(m_fNormalize) 
			f *= sample_mul;

		if(f < -1) f = -1;
		else if(f > 1) f = 1;

		switch(iSampleFormat)
		{
		default:
		case SF_PCM16:
			*(short*)pDataOut = (short)(f * SHRT_MAX);
			pDataOut += sizeof(short);
			break;
		case SF_PCM24:
			{DWORD i24 = (DWORD)(int)(f * ((1<<23)-1));
			*pDataOut++ = (BYTE)(i24);
			*pDataOut++ = (BYTE)(i24>>8);
			*pDataOut++ = (BYTE)(i24>>16);}
			break;
		case SF_PCM32:
			*(int*)pDataOut = (int)(f * INT_MAX);
			pDataOut += sizeof(int);
			break;
		case SF_FLOAT32:
			*(float*)pDataOut = f;
			pDataOut += sizeof(float);
			break;
		}
	}

	hr = m_pOutput->Deliver(pOut);
	return hr;
}

HRESULT CMpaDecFilter::Deliver(CArray<BYTE>& pBuff, DWORD nSamplesPerSec, REFERENCE_TIME rtDur)
{
	HRESULT hr;

	CMediaType mt;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX wfe;
	memset(&wfe, 0, sizeof(wfe));
	wfe.wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
	wfe.nSamplesPerSec = nSamplesPerSec;
	wfe.nChannels = 2;
	wfe.wBitsPerSample = 16;
	wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;
	wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
	wfe.cbSize = 0;

	mt.SetFormat((BYTE*)&wfe, sizeof(wfe) + wfe.cbSize);

	if(FAILED(hr = ReconnectOutput(pBuff.GetSize() / wfe.nBlockAlign, mt)))
		return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(GetDeliveryBuffer(&pOut, &pDataOut)))
		return E_FAIL;

	REFERENCE_TIME rtStart = m_rtStart, rtStop = m_rtStart + rtDur;
	m_rtStart += rtDur;
//TRACE(_T("CMpaDecFilter: %I64d - %I64d\n"), rtStart/10000, rtStop/10000);
	if(rtStart < 0)
		return S_OK;

	if(hr == S_OK)
	{
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_fDiscontinuity); m_fDiscontinuity = false;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(pBuff.GetSize());
	memcpy(pDataOut, pBuff.GetData(), pBuff.GetSize());

	return m_pOutput->Deliver(pOut);
}

HRESULT CMpaDecFilter::ReconnectOutput(int nSamples, CMediaType& mt)
{
	HRESULT hr;

	CComQIPtr<IMemInputPin> pPin = m_pOutput->GetConnected();
	if(!pPin) return E_NOINTERFACE;

	CComPtr<IMemAllocator> pAllocator;
	if(FAILED(hr = pPin->GetAllocator(&pAllocator)) || !pAllocator) 
		return hr;

	ALLOCATOR_PROPERTIES props, actual;
	if(FAILED(hr = pAllocator->GetProperties(&props)))
		return hr;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
	long cbBuffer = nSamples * wfe->nBlockAlign;

	if(mt != m_pOutput->CurrentMediaType() || cbBuffer > props.cbBuffer)
	{
		if(cbBuffer > props.cbBuffer)
		{
		props.cBuffers = 4;
		props.cbBuffer = cbBuffer*3/2;

		if(FAILED(hr = m_pOutput->DeliverBeginFlush())
		|| FAILED(hr = m_pOutput->DeliverEndFlush())
		|| FAILED(hr = pAllocator->Decommit())
		|| FAILED(hr = pAllocator->SetProperties(&props, &actual))
		|| FAILED(hr = pAllocator->Commit()))
			return hr;
		}

		return S_OK;
	}

	return S_FALSE;
}

HRESULT CMpaDecFilter::CheckInputType(const CMediaType* mtIn)
{
	// TODO: remove this limitation
	if(mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();
		if(wfe->nChannels != 2 || wfe->wBitsPerSample != 16)
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return (mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MP3
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG1AudioPayload
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG1Payload
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG1Packet
			|| mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_MPEG2_AUDIO
			|| mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_DOLBY_AC3
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_DOLBY_AC3
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_DOLBY_AC3
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_DOLBY_AC3
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3
			|| mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_DTS
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_DTS
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_DTS
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_DTS
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_WAVE_DTS
			|| mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			|| mtIn->majortype == MEDIATYPE_Audio && mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
			)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM
		|| mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_IEEE_FLOAT
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CMediaType& mt = m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	pProperties->cBuffers = 4;
	pProperties->cbBuffer = 1;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

HRESULT CMpaDecFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_PCM;
	pmt->formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfe, 0, pmt->FormatLength());
	wfe->cbSize = 0;
	wfe->wFormatTag = WAVE_FORMAT_PCM;
	wfe->nChannels = 2;
	wfe->wBitsPerSample = 16;
	wfe->nSamplesPerSec = 44100;
	wfe->nBlockAlign = wfe->nChannels*wfe->wBitsPerSample/8;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec*wfe->nBlockAlign;

	return S_OK;
}

HRESULT CMpaDecFilter::StartStreaming()
{
	HRESULT hr = __super::StartStreaming();
	if(FAILED(hr)) return hr;

	m_a52_state = a52_init(0);

	mad_stream_init(&m_stream);
	mad_frame_init(&m_frame);
	mad_synth_init(&m_synth);
	mad_stream_options(&m_stream, 0/*options*/);

	m_fDiscontinuity = false;

	m_sample_max = 0.1;

	return S_OK;
}

HRESULT CMpaDecFilter::StopStreaming()
{
	a52_free(m_a52_state);

	mad_synth_finish(&m_synth);
	mad_frame_finish(&m_frame);
	mad_stream_finish(&m_stream);

	return __super::StopStreaming();
}

// IMpaDecFilter

STDMETHODIMP CMpaDecFilter::SetSampleFormat(SampleFormat sf)
{
	CAutoLock cAutoLock(&m_csProps);
	m_iSampleFormat = sf;
	return S_OK;
}

STDMETHODIMP_(SampleFormat) CMpaDecFilter::GetSampleFormat()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_iSampleFormat;
}

STDMETHODIMP CMpaDecFilter::SetNormalize(bool fNormalize)
{
	CAutoLock cAutoLock(&m_csProps);
	if(m_fNormalize != fNormalize) m_sample_max = 0.1;
	m_fNormalize = fNormalize;
	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetNormalize()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fNormalize;
}

STDMETHODIMP CMpaDecFilter::SetSpeakerConfig(int sc)
{
	CAutoLock cAutoLock(&m_csProps);
	m_iSpeakerConfig = sc;
	return S_OK;
}

STDMETHODIMP_(int) CMpaDecFilter::GetSpeakerConfig()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_iSpeakerConfig;
}

STDMETHODIMP CMpaDecFilter::SetDynamicRangeControl(bool fDRC)
{
	CAutoLock cAutoLock(&m_csProps);
	m_fDynamicRangeControl = fDRC;
	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetDynamicRangeControl()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fDynamicRangeControl;
}

//
// CMpaDecInputPin
//

CMpaDecInputPin::CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMpaDecInputPin"), pFilter, phr, pName)
{
}
