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

#include "stdafx.h"
#include "MacrovisionKicker.h"

//
// CMacrovisionKicker
//

CMacrovisionKicker::CMacrovisionKicker(const TCHAR* pName, LPUNKNOWN pUnk)
	: CUnknown(pName, pUnk)
{
}

CMacrovisionKicker::~CMacrovisionKicker()
{
}

void CMacrovisionKicker::SetInner(CComPtr<IUnknown> pUnk)
{
	m_pInner = pUnk;
}

STDMETHODIMP CMacrovisionKicker::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	if(riid == __uuidof(IKsPropertySet) && CComQIPtr<IKsPropertySet>(m_pInner))
		return GetInterface((IKsPropertySet*)this, ppv);
	if(riid == __uuidof(IAMVideoAccelerator) && CComQIPtr<IAMVideoAccelerator>(m_pInner))
		return GetInterface((IAMVideoAccelerator*)this, ppv);

	HRESULT hr = m_pInner ? m_pInner->QueryInterface(riid, ppv) : E_NOINTERFACE;

	return SUCCEEDED(hr) ? hr : __super::NonDelegatingQueryInterface(riid, ppv);
}

// IKsPropertySet

STDMETHODIMP CMacrovisionKicker::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(CComQIPtr<IKsPropertySet> pKsPS = m_pInner)
	{
		if(PropSet == AM_KSPROPSETID_CopyProt && Id == AM_PROPERTY_COPY_MACROVISION
		/*&& DataLength == 4 && *(DWORD*)pPropertyData*/)
		{
			TRACE(_T("Oops, no-no-no, no macrovision please\n"));
			return S_OK;
		}

		return pKsPS->Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CMacrovisionKicker::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(CComQIPtr<IKsPropertySet> pKsPS = m_pInner)
	{
		return pKsPS->Get(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength, pBytesReturned);
	}
	
	return E_UNEXPECTED;	
}

STDMETHODIMP CMacrovisionKicker::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(CComQIPtr<IKsPropertySet> pKsPS = m_pInner)
	{
		return pKsPS->QuerySupported(PropSet, Id, pTypeSupport);
	}
	
	return E_UNEXPECTED;
}

// IAMVideoAccelerator

STDMETHODIMP CMacrovisionKicker::GetVideoAcceleratorGUIDs(LPDWORD pdwNumGuidsSupported, LPGUID pGuidsSupported)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->GetVideoAcceleratorGUIDs(pdwNumGuidsSupported, pGuidsSupported);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::GetUncompFormatsSupported(const GUID* pGuid, LPDWORD pdwNumFormatsSupported, LPDDPIXELFORMAT pFormatsSupported)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->GetUncompFormatsSupported(pGuid, pdwNumFormatsSupported, pFormatsSupported);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::GetInternalMemInfo(const GUID* pGuid, const AMVAUncompDataInfo* pamvaUncompDataInfo, LPAMVAInternalMemInfo pamvaInternalMemInfo)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->GetInternalMemInfo(pGuid, pamvaUncompDataInfo, pamvaInternalMemInfo);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::GetCompBufferInfo(const GUID* pGuid, const AMVAUncompDataInfo* pamvaUncompDataInfo, LPDWORD pdwNumTypesCompBuffers, LPAMVACompBufferInfo pamvaCompBufferInfo)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->GetCompBufferInfo(pGuid, pamvaUncompDataInfo, pdwNumTypesCompBuffers, pamvaCompBufferInfo);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::GetInternalCompBufferInfo(LPDWORD pdwNumTypesCompBuffers, LPAMVACompBufferInfo pamvaCompBufferInfo)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->GetInternalCompBufferInfo(pdwNumTypesCompBuffers, pamvaCompBufferInfo);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::BeginFrame(const AMVABeginFrameInfo* amvaBeginFrameInfo)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->BeginFrame(amvaBeginFrameInfo);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::EndFrame(const AMVAEndFrameInfo* pEndFrameInfo)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->EndFrame(pEndFrameInfo);
	return hr;
}        
STDMETHODIMP CMacrovisionKicker::GetBuffer(DWORD dwTypeIndex, DWORD dwBufferIndex, BOOL bReadOnly, LPVOID* ppBuffer, LONG* lpStride)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->GetBuffer(dwTypeIndex, dwBufferIndex, bReadOnly, ppBuffer, lpStride);
	return hr;
}
STDMETHODIMP CMacrovisionKicker::ReleaseBuffer(DWORD dwTypeIndex, DWORD dwBufferIndex)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->ReleaseBuffer(dwTypeIndex, dwBufferIndex);
	return hr;
}
STDMETHODIMP CMacrovisionKicker::Execute(DWORD dwFunction, LPVOID lpPrivateInputData, DWORD cbPrivateInputData,
	LPVOID lpPrivateOutputDat, DWORD cbPrivateOutputData, DWORD dwNumBuffers, const AMVABUFFERINFO* pamvaBufferInfo)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->Execute(dwFunction, lpPrivateInputData, cbPrivateInputData, 
		lpPrivateOutputDat, cbPrivateOutputData, dwNumBuffers, pamvaBufferInfo);
	return hr;
}
STDMETHODIMP CMacrovisionKicker::QueryRenderStatus(DWORD dwTypeIndex, DWORD dwBufferIndex, DWORD dwFlags)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->QueryRenderStatus(dwTypeIndex, dwBufferIndex, dwFlags);
	return hr;
}
STDMETHODIMP CMacrovisionKicker::DisplayFrame(DWORD dwFlipToIndex, IMediaSample* pMediaSample)
{
	CComQIPtr<IAMVideoAccelerator> pVA = m_pInner;
	if(!pVA) return E_UNEXPECTED;
	HRESULT hr = pVA->DisplayFrame(dwFlipToIndex, pMediaSample);
	return hr;
}
