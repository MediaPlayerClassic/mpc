/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  Mpeg2DecFilter.ax is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  Mpeg2DecFilter.ax is distributed in the hope that it will be useful,
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

#include <atlcoll.h>
#include <afxtempl.h>

class CSubpicInputPin;
class CClosedCaptionOutputPin;
class CMpeg2Dec;

[uuid("39F498AF-1A09-4275-B193-673B0BA3D478")]
class CMpeg2DecFilter : public CTransformFilter
{
	CSubpicInputPin* m_pSubpicInput;
	CClosedCaptionOutputPin* m_pClosedCaptionOutput;
	CAutoPtr<CMpeg2Dec> m_dec;
	REFERENCE_TIME m_AvgTimePerFrame;
	CCritSec m_csReceive;
	bool m_fWaitForKeyFrame;
	struct framebuf 
	{
		int w, h, pw, ph;
		BYTE* buf[6];
		REFERENCE_TIME rtFrame, rtOffset, rtStart, rtStop;
		DWORD flags;
        framebuf()
		{
			w = h = pw = ph = 0;
			memset(&buf, 0, sizeof(buf));
			rtFrame = rtOffset = rtStart = rtStop = 0;
			flags = 0;
		}
        ~framebuf() {free();}
		void alloc(int w, int h, int pw, int ph)
		{
			this->w = w; this->h = h;
			this->pw = pw; this->ph = ph;
			buf[0] = (BYTE*)_aligned_malloc(w*h, 16); buf[3] = (BYTE*)_aligned_malloc(w*h, 16);
			buf[1] = (BYTE*)_aligned_malloc(w*h/4, 16); buf[4] = (BYTE*)_aligned_malloc(w*h/4, 16);
			buf[2] = (BYTE*)_aligned_malloc(w*h/4, 16); buf[5] = (BYTE*)_aligned_malloc(w*h/4, 16);
		}
		void free() {for(int i = 0; i < 6; i++) {_aligned_free(buf[i]); buf[i] = NULL;}}
	} m_fb;

	void Copy(BYTE* pOut, BYTE** ppIn, DWORD w, DWORD h, DWORD pitchIn);
	void ResetMpeg2Decoder();
	HRESULT ReconnectOutput(int w, int h);

public:
	CMpeg2DecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMpeg2DecFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	HRESULT Deliver(bool fRepeatLast);
	HRESULT CheckOutputMediaType(const CMediaType& mtOut);

	int GetPinCount();
	CBasePin* GetPin(int n);

    HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT Receive(IMediaSample* pIn);

    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

	HRESULT AlterQuality(Quality q);
};

class CMpeg2DecInputPin : public CTransformInputPin, public IKsPropertySet
{
	int m_varient;
	BYTE m_Challenge[10], m_KeyCheck[5], m_Key[10];
	BYTE m_DiscKey[6], m_TitleKey[6];

protected:
	virtual HRESULT Transform(IMediaSample* pSample) {return S_OK;}

public:
    CMpeg2DecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IMemInputPin
    STDMETHODIMP Receive(IMediaSample* pSample);

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CSubpicInputPin : public CMpeg2DecInputPin
{
	CCritSec m_csReceive;

	AM_PROPERTY_COMPOSIT_ON m_spon;
	AM_DVD_YUV m_sppal[16];
	CAutoPtr<AM_PROPERTY_SPHLI> m_sphli; // temp
	struct sp_t
	{
		REFERENCE_TIME rtStart, rtStop; 
		CArray<BYTE> pData;
		CAutoPtr<AM_PROPERTY_SPHLI> sphli; // hli
		bool fForced;
	};
	CAutoPtrList<sp_t> m_sps;

	bool DecodeSubpic(sp_t* sp, AM_PROPERTY_SPHLI& sphli, DWORD& offset1, DWORD& offset2);
	void RenderSubpic(sp_t* sp, BYTE** p, int w, int h, AM_PROPERTY_SPHLI* sphli_hli = NULL);

protected:
	HRESULT Transform(IMediaSample* pSample);

public:
	CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr);

	bool HasAnythingToRender(REFERENCE_TIME rt);
	void RenderSubpics(REFERENCE_TIME rt, BYTE** p, int w, int h);

    HRESULT CheckMediaType(const CMediaType* mtIn);
	HRESULT SetMediaType(const CMediaType* mtIn);

	// we shouldn't pass these to the filter from this pin
	STDMETHODIMP EndOfStream() {return S_OK;}
    STDMETHODIMP BeginFlush() {return S_OK;}
    STDMETHODIMP EndFlush();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {return S_OK;}

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CMpeg2DecOutputPin : public CTransformOutputPin
{
public:
	CMpeg2DecOutputPin(CTransformFilter* pTransformFilter, HRESULT* phr);

    HRESULT CheckMediaType(const CMediaType* mtOut);
};

class CClosedCaptionOutputPin : public CBaseOutputPin
{
public:
	CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

    HRESULT CheckMediaType(const CMediaType* mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);

	CMediaType& CurrentMediaType() {return m_mt;}
};

