#pragma once

#include <Videoacc.h>

class CMacrovisionKicker
	: public CUnknown
	, public IKsPropertySet
	, public IAMVideoAccelerator
{
	CComPtr<IUnknown> m_pInner;

public:
	CMacrovisionKicker(const TCHAR* pName, LPUNKNOWN pUnk);
	virtual ~CMacrovisionKicker();

	void SetInner(CComPtr<IUnknown> pUnk);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);

	// IAMVideoAccelerator
    STDMETHODIMP GetVideoAcceleratorGUIDs(LPDWORD pdwNumGuidsSupported, LPGUID pGuidsSupported);
    STDMETHODIMP GetUncompFormatsSupported(const GUID* pGuid, LPDWORD pdwNumFormatsSupported, LPDDPIXELFORMAT pFormatsSupported);
    STDMETHODIMP GetInternalMemInfo(const GUID* pGuid, const AMVAUncompDataInfo* pamvaUncompDataInfo, LPAMVAInternalMemInfo pamvaInternalMemInfo);
    STDMETHODIMP GetCompBufferInfo(const GUID* pGuid, const AMVAUncompDataInfo* pamvaUncompDataInfo, LPDWORD pdwNumTypesCompBuffers, LPAMVACompBufferInfo pamvaCompBufferInfo);
    STDMETHODIMP GetInternalCompBufferInfo(LPDWORD pdwNumTypesCompBuffers, LPAMVACompBufferInfo pamvaCompBufferInfo);
    STDMETHODIMP BeginFrame(const AMVABeginFrameInfo* amvaBeginFrameInfo);
    STDMETHODIMP EndFrame(const AMVAEndFrameInfo* pEndFrameInfo);
    STDMETHODIMP GetBuffer(DWORD dwTypeIndex, DWORD dwBufferIndex, BOOL bReadOnly, LPVOID* ppBuffer, LONG* lpStride);
    STDMETHODIMP ReleaseBuffer(DWORD dwTypeIndex, DWORD dwBufferIndex);
    STDMETHODIMP Execute(DWORD dwFunction, LPVOID lpPrivateInputData, DWORD cbPrivateInputData, LPVOID lpPrivateOutputDat, DWORD cbPrivateOutputData, DWORD dwNumBuffers, const AMVABUFFERINFO* pamvaBufferInfo);
    STDMETHODIMP QueryRenderStatus(DWORD dwTypeIndex, DWORD dwBufferIndex, DWORD dwFlags);
    STDMETHODIMP DisplayFrame(DWORD dwFlipToIndex, IMediaSample* pMediaSample);
};

