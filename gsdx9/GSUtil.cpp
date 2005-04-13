#include "stdafx.h"
#include "GSState.h"

BOOL IsDepthFormatOk(IDirect3D9* pD3D, D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat)
{
    // Verify that the depth format exists.
    HRESULT hr = pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DepthFormat);
    if(FAILED(hr)) return FALSE;

    // Verify that the depth format is compatible.
    hr = pD3D->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, AdapterFormat, BackBufferFormat, DepthFormat);
    return SUCCEEDED(hr);
}

HRESULT CompileShaderFromResource(IDirect3DDevice9* pD3DDev, UINT id, CString entry, CString target, UINT flags, IDirect3DPixelShader9** ppPixelShader)
{
	CheckPointer(pD3DDev, E_POINTER);
	CheckPointer(ppPixelShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
	HRESULT hr = D3DXCompileShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		entry, target, flags, 
		&pShader, &pErrorMsgs, NULL);
	ASSERT(SUCCEEDED(hr));

	if(SUCCEEDED(hr))
	{
		hr = pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
		ASSERT(SUCCEEDED(hr));
	}

	return hr;
}

HRESULT AssembleShaderFromResource(IDirect3DDevice9* pD3DDev, UINT id, UINT flags, IDirect3DPixelShader9** ppPixelShader)
{
	CheckPointer(pD3DDev, E_POINTER);
	CheckPointer(ppPixelShader, E_POINTER);

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;
	HRESULT hr = D3DXAssembleShaderFromResource(
		AfxGetResourceHandle(), MAKEINTRESOURCE(id),
		NULL, NULL, 
		flags, 
		&pShader, &pErrorMsgs);
	ASSERT(SUCCEEDED(hr));

	if(SUCCEEDED(hr))
	{
		hr = pD3DDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
		ASSERT(SUCCEEDED(hr));
	}

	return hr;
}
