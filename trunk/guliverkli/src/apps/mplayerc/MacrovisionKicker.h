/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
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

