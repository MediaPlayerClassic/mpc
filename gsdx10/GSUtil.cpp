/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "GSdx10.h"
#include "GS.h"

bool HasSharedBits(DWORD spsm, DWORD dpsm)
{
	switch(spsm)
	{
	case PSM_PSMCT32:
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMT8:
	case PSM_PSMT4:
	case PSM_PSMZ32:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:
		return true;
	case PSM_PSMCT24:
	case PSM_PSMZ24:
		return !(dpsm == PSM_PSMT8H || dpsm == PSM_PSMT4HL || dpsm == PSM_PSMT4HH);
	case PSM_PSMT8H:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMZ24);
	case PSM_PSMT4HL:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMZ24 || dpsm == PSM_PSMT4HH);
	case PSM_PSMT4HH:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMZ24 || dpsm == PSM_PSMT4HL);
	}

	return true;
}

bool HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm)
{
	if(sbp != dbp) return false;

	return HasSharedBits(spsm, dpsm);
}

bool IsRectInRect(const CRect& inner, const CRect& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right && outer.top <= inner.top && inner.bottom <= outer.bottom;
}

bool IsRectInRectH(const CRect& inner, const CRect& outer)
{
	return outer.top <= inner.top && inner.bottom <= outer.bottom;
}

bool IsRectInRectV(const CRect& inner, const CRect& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right;
}

BYTE* LoadResource(UINT id, DWORD& size)
{
	if(HRSRC hRes = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(id), RT_RCDATA)) 
	{
		if(HGLOBAL hGlobal = ::LoadResource(AfxGetResourceHandle(), hRes))
		{
			size = SizeofResource(AfxGetResourceHandle(), hRes);

			if(size > 0)
			{
				BYTE* buff = new BYTE[size+1];
				memcpy(buff, LockResource(hGlobal), size);
				buff[size] = 0;
				return buff;
			}
		}
	}

	return NULL;
}

HRESULT CompileShader(ID3D10Device* dev, ID3D10VertexShader** ps, UINT id, LPCTSTR entry, D3D10_INPUT_ELEMENT_DESC* layout, int count, ID3D10InputLayout** pl, D3D10_SHADER_MACRO* macro)
{
	HRESULT hr;

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX10CompileFromResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), NULL, macro, NULL, entry, _T("vs_4_0"), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		TRACE(_T("%s\n"), CString((LPCSTR)error->GetBufferPointer()));
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = dev->CreateVertexShader((DWORD*)shader->GetBufferPointer(), shader->GetBufferSize(), ps);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = dev->CreateInputLayout(layout, count, shader->GetBufferPointer(), shader->GetBufferSize(), pl);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT CompileShader(ID3D10Device* dev, ID3D10GeometryShader** gs, UINT id, LPCTSTR entry, D3D10_SHADER_MACRO* macro)
{
	HRESULT hr;

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX10CompileFromResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), NULL, macro, NULL, entry, _T("gs_4_0"), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		TRACE(_T("%s\n"), CString((LPCSTR)error->GetBufferPointer()));
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = dev->CreateGeometryShader((DWORD*)shader->GetBufferPointer(), shader->GetBufferSize(), gs);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}

HRESULT CompileShader(ID3D10Device* dev, ID3D10PixelShader** ps, UINT id, LPCTSTR entry, D3D10_SHADER_MACRO* macro)
{
	HRESULT hr;

	CComPtr<ID3D10Blob> shader, error;

    hr = D3DX10CompileFromResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(id), NULL, macro, NULL, entry, _T("ps_4_0"), 0, 0, NULL, &shader, &error, NULL);
	
	if(error)
	{
		TRACE(_T("%s\n"), CString((LPCSTR)error->GetBufferPointer()));
	}

	if(FAILED(hr))
	{
		return hr;
	}

	hr = dev->CreatePixelShader((DWORD*)shader->GetBufferPointer(), shader->GetBufferSize(), ps);

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;
}
