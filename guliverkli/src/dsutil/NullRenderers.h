#pragma once

#include <atlcoll.h>

class CNullRenderer : public CBaseRenderer
{
protected:
	HRESULT DoRenderSample(IMediaSample* pSample) {return S_OK;}
public:
	CNullRenderer(REFCLSID clsid, TCHAR* pName)
		: CBaseRenderer(clsid, pName, NULL, NULL) {}
};

[uuid("579883A0-4E2D-481F-9436-467AAFAB7DE8")]
class CNullVideoRenderer : public CNullRenderer
{
protected:
	HRESULT CheckMediaType(const CMediaType* pmt);
public:
	CNullVideoRenderer()
		: CNullRenderer(__uuidof(this), NAME("Null Video Renderer")) {}
};

[uuid("DD9ED57D-6ABF-42E8-89A2-11D04798DC58")]
class CNullUVideoRenderer : public CNullRenderer
{
protected:
	HRESULT CheckMediaType(const CMediaType* pmt);
public:
	CNullUVideoRenderer()
		: CNullRenderer(__uuidof(this), NAME("Null Video Renderer (Uncompressed)")) {}
};

[uuid("0C38BDFD-8C17-4E00-A344-F89397D3E22A")]
class CNullAudioRenderer : public CNullRenderer
{
protected:
	HRESULT CheckMediaType(const CMediaType* pmt);
public:
	CNullAudioRenderer()
		: CNullRenderer(__uuidof(this), NAME("Null Audio Renderer")) {}
};

[uuid("64A45125-7343-4772-9DA4-179FAC9D462C")]
class CNullUAudioRenderer : public CNullRenderer
{
protected:
	HRESULT CheckMediaType(const CMediaType* pmt);
public:
	CNullUAudioRenderer()
		: CNullRenderer(__uuidof(this), NAME("Null Audio Renderer (Uncompressed)")) {}
};

[uuid("655D7613-C26C-4A25-BBBD-3C9C516122CC")]
class CNullTextRenderer : public CBaseFilter, public CCritSec
{
	class CTextInputPin : public CBaseInputPin
	{
	public:
		CTextInputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr) 
			: CBaseInputPin(NAME("CTextInputPin"), pFilter, pLock, phr, L"In") {}
	    HRESULT CheckMediaType(const CMediaType* pmt);
	};
	CAutoPtr<CTextInputPin> m_pInput;
public:
	CNullTextRenderer(LPUNKNOWN pUnk, HRESULT* phr);
	int GetPinCount() {return (int)!!m_pInput;}
	CBasePin* GetPin(int n) {return n == 0 ? (CBasePin*)m_pInput : NULL;}
};

