#include "StdAfx.h"
#include "NullRenderers.h"
#include "..\..\include\moreuuids.h"
#include "..\..\include\matroska\matroska.h"

//
// CNullVideoRenderer
//

HRESULT CNullVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		|| pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO
		? S_OK
		: E_FAIL;
}

//
// CNullUVideoRenderer
//

HRESULT CNullUVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Video
		&& (pmt->subtype == MEDIASUBTYPE_YV12
		|| pmt->subtype == MEDIASUBTYPE_I420
		|| pmt->subtype == MEDIASUBTYPE_YUYV
		|| pmt->subtype == MEDIASUBTYPE_IYUV
		|| pmt->subtype == MEDIASUBTYPE_YVU9
		|| pmt->subtype == MEDIASUBTYPE_Y411
		|| pmt->subtype == MEDIASUBTYPE_Y41P
		|| pmt->subtype == MEDIASUBTYPE_YUY2
		|| pmt->subtype == MEDIASUBTYPE_YVYU
		|| pmt->subtype == MEDIASUBTYPE_UYVY
		|| pmt->subtype == MEDIASUBTYPE_Y211
		|| pmt->subtype == MEDIASUBTYPE_RGB1
		|| pmt->subtype == MEDIASUBTYPE_RGB4
		|| pmt->subtype == MEDIASUBTYPE_RGB8
		|| pmt->subtype == MEDIASUBTYPE_RGB565
		|| pmt->subtype == MEDIASUBTYPE_RGB555
		|| pmt->subtype == MEDIASUBTYPE_RGB24
		|| pmt->subtype == MEDIASUBTYPE_RGB32
		|| pmt->subtype == MEDIASUBTYPE_ARGB1555
		|| pmt->subtype == MEDIASUBTYPE_ARGB4444
		|| pmt->subtype == MEDIASUBTYPE_ARGB32
		|| pmt->subtype == MEDIASUBTYPE_A2R10G10B10
		|| pmt->subtype == MEDIASUBTYPE_A2B10G10R10)
		? S_OK
		: E_FAIL;
}

//
// CNullAudioRenderer
//

HRESULT CNullAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		|| pmt->majortype == MEDIATYPE_Midi
		|| pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO
		|| pmt->subtype == MEDIASUBTYPE_DOLBY_AC3
		|| pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
		|| pmt->subtype == MEDIASUBTYPE_DTS
		|| pmt->subtype == MEDIASUBTYPE_SDDS
		|| pmt->subtype == MEDIASUBTYPE_MPEG1AudioPayload
		|| pmt->subtype == MEDIASUBTYPE_MPEG1Audio
		|| pmt->subtype == MEDIASUBTYPE_MPEG1Audio
		? S_OK
		: E_FAIL;
}

//
// CNullUAudioRenderer
//

HRESULT CNullUAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Audio
		&& (pmt->subtype == MEDIASUBTYPE_PCM
		|| pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT
		|| pmt->subtype == MEDIASUBTYPE_DRM_Audio
		|| pmt->subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF
		|| pmt->subtype == MEDIASUBTYPE_RAW_SPORT
		|| pmt->subtype == MEDIASUBTYPE_SPDIF_TAG_241h)
		? S_OK
		: E_FAIL;
}

//
// CNullTextRenderer
//

HRESULT CNullTextRenderer::CTextInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Text || pmt->majortype == MEDIATYPE_Subtitle ? S_OK : E_FAIL;
}

CNullTextRenderer::CNullTextRenderer(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseFilter(NAME("CNullTextRenderer"), pUnk, this, __uuidof(this), phr) 
{
	m_pInput.Attach(new CTextInputPin(this, this, phr));
}
