#pragma once

extern BOOL IsDepthFormatOk(IDirect3D9* pD3D, D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat);
extern HRESULT CompileShaderFromResource(IDirect3DDevice9* pD3DDev, UINT id, CString entry, CString target, UINT flags, IDirect3DPixelShader9** ppPixelShader);
extern HRESULT AssembleShaderFromResource(IDirect3DDevice9* pD3DDev, UINT id, UINT flags, IDirect3DPixelShader9** ppPixelShader);
