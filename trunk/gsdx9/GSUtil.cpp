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
#include "GSdx9.h"
#include "GSState.h"

bool IsDepthFormatOk(IDirect3D9* pD3D, D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat)
{
    // Verify that the depth format exists.
	HRESULT hr = pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DepthFormat);
    if(FAILED(hr)) return false;

    // Verify that the depth format is compatible.
    hr = pD3D->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, AdapterFormat, BackBufferFormat, DepthFormat);
    return SUCCEEDED(hr);
}

bool IsRenderTarget(IDirect3DTexture9* pTexture)
{
	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	return pTexture && S_OK == pTexture->GetLevelDesc(0, &desc) && (desc.Usage & D3DUSAGE_RENDERTARGET);
}

HRESULT CompileShaderFromResource(IDirect3DDevice9* dev, UINT id, CString entry, CString target, UINT flags, IDirect3DVertexShader9** ppVertexShader, ID3DXConstantTable** ppConstantTable)
{
	CheckPointer(dev, E_POINTER);
	CheckPointer(ppVertexShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;

	HRESULT hr = D3DXCompileShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		entry, target, flags, 
		&pShader, &pErrorMsgs, ppConstantTable);

	if(SUCCEEDED(hr))
	{
		hr = dev->CreateVertexShader((DWORD*)pShader->GetBufferPointer(), ppVertexShader);
	}
	else
	{
		LPCSTR msg = (LPCSTR)pErrorMsgs->GetBufferPointer();

		TRACE(_T("%s\n"), CString(msg));
	}

	ASSERT(SUCCEEDED(hr));

	return hr;
}

HRESULT CompileShaderFromResource(IDirect3DDevice9* dev, UINT id, CString entry, CString target, UINT flags, IDirect3DPixelShader9** ppPixelShader, ID3DXConstantTable** ppConstantTable)
{
	CheckPointer(dev, E_POINTER);
	CheckPointer(ppPixelShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;

	HRESULT hr = D3DXCompileShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		entry, target, flags, 
		&pShader, &pErrorMsgs, ppConstantTable);

	if(SUCCEEDED(hr))
	{
		hr = dev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
/*
		CComPtr<ID3DXBuffer> pDisAsm;

		hr = D3DXDisassembleShader((DWORD*)pShader->GetBufferPointer(), FALSE, NULL, &pDisAsm);

		if(SUCCEEDED(hr) && pDisAsm)
		{
			CString str = CString(CStringA((const char*)pDisAsm->GetBufferPointer()));

			TRACE(_T("%s\n"), str);
		}
*/
	}
	else
	{
		LPCSTR msg = (LPCSTR)pErrorMsgs->GetBufferPointer();

		TRACE(_T("%s\n"), CString(msg));
	}

	ASSERT(SUCCEEDED(hr));

	return hr;
}

HRESULT AssembleShaderFromResource(IDirect3DDevice9* dev, UINT id, UINT flags, IDirect3DPixelShader9** ppPixelShader)
{
	CheckPointer(dev, E_POINTER);
	CheckPointer(ppPixelShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;

	HRESULT hr = D3DXAssembleShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		flags, 
		&pShader, &pErrorMsgs);

	if(SUCCEEDED(hr))
	{
		hr = dev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
	}

	ASSERT(SUCCEEDED(hr));

	return hr;
}

bool HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm)
{
	if(sbp != dbp) return false;

	switch(spsm)
	{
	case PSM_PSMCT32:
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMT8:
	case PSM_PSMT4:
		return true;
	case PSM_PSMCT24:
		return !(dpsm == PSM_PSMT8H || dpsm == PSM_PSMT4HL || dpsm == PSM_PSMT4HH);
	case PSM_PSMT8H:
		return !(dpsm == PSM_PSMCT24);
	case PSM_PSMT4HL:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMT4HH);
	case PSM_PSMT4HH:
		return !(dpsm == PSM_PSMCT24 || dpsm == PSM_PSMT4HL);
	}

	return true;
}

