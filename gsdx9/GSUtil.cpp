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

bool CompileTFX(IDirect3DDevice9* dev, IDirect3DPixelShader9** ps, CString target, DWORD flags, int tfx, int bpp, int tcc, int aem, int fog, int rt, int fst, int clamp)
{
	DWORD size = 0;

	BYTE* buff = LoadResource(IDR_HLSL_TFX, size);

	if(!buff || size == 0)
	{
		return false;
	}

	CString str[8];

	str[0].Format(_T("%d"), tfx);
	str[1].Format(_T("%d"), bpp);
	str[2].Format(_T("%d"), tcc);
	str[3].Format(_T("%d"), aem);
	str[4].Format(_T("%d"), fog);
	str[5].Format(_T("%d"), rt);
	str[6].Format(_T("%d"), fst);
	str[7].Format(_T("%d"), clamp);

	D3DXMACRO macro[] =
    {
        {"TFX", str[0]},
		{"BPP", str[1]},
        {"TCC", str[2]},
        {"AEM", str[3]},
        {"FOG", str[4]},
        {"RT", str[5]},
        {"FST", str[6]},
        {"CLAMP", str[7]},
        {NULL, NULL},
    };

	HRESULT hr;

	CComPtr<ID3DXBuffer> pShader, pErrorMsgs;

	hr = D3DXCompileShader((LPCSTR)buff, size, (D3DXMACRO*)macro, NULL, _T("main"), target, flags, &pShader, &pErrorMsgs, NULL);

	if(pErrorMsgs)
	{
		TRACE(_T("%s\n"), CString((LPCSTR)pErrorMsgs->GetBufferPointer()));
	}

	if(SUCCEEDED(hr))
	{
		hr = dev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ps);
	}

	delete [] buff; 

	return SUCCEEDED(hr);
}

bool CompileTFX(CString fn, CString target, DWORD flags)
{
	DWORD size = 0;

	BYTE* buff = LoadResource(IDR_HLSL_TFX, size);

	if(!buff || size == 0)
	{
		return false;
	}

	D3DXMACRO macro[] =
    {
        {"TFX", NULL},
		{"BPP", NULL},
        {"TCC", NULL},
        {"AEM", NULL},
        {"FOG", NULL},
        {"RT", NULL},
        {"FST", NULL},
        {"CLAMP", NULL},
        {NULL, NULL},
    };

	if(FILE* fp = _tfopen(fn + _T(".cpp"), _T("wb")))
	{
		_ftprintf(fp, _T("#include \"stdafx.h\"\n\n"));
		fclose(fp);
	}

	HRESULT hr;

	CAtlMap<CString, DWORD, CStringElementTraits<CString> > map;

	for(int tfx = 0; tfx < 5; tfx++)
	{
		CStringA str;
		str.Format("%d", tfx);
		macro[0].Definition = str;

		for(int bpp = 0; bpp < 4; bpp++)
		{
			CStringA str;
			str.Format("%d", bpp);
			macro[1].Definition = str;

			for(int tcc = 0; tcc < 2; tcc++)
			{
				CStringA str;
				str.Format("%d", tcc);
				macro[2].Definition = str;

				for(int aem = 0; aem < 2; aem++)
				{
					CStringA str;
					str.Format("%d", aem);
					macro[3].Definition = str;

					for(int fog = 0; fog < 2; fog++)
					{
						CStringA str;
						str.Format("%d", fog);
						macro[4].Definition = str;

						for(int rt = 0; rt < 2; rt++)
						{
							CStringA str;
							str.Format("%d", rt);
							macro[5].Definition = str;

							for(int fst = 0; fst < 2; fst++)
							{
								CStringA str;
								str.Format("%d", fst);
								macro[6].Definition = str;

								for(int clamp = 0; clamp < 2; clamp++)
								{
									CStringA str;
									str.Format("%d", clamp);
									macro[7].Definition = str;

									CComPtr<ID3DXBuffer> pShader, pErrorMsgs;

									hr = D3DXCompileShader((LPCSTR)buff, size, (D3DXMACRO*)macro, NULL, _T("main"), target, flags, &pShader, &pErrorMsgs, NULL);

									if(pErrorMsgs)
									{
										TRACE(_T("%s\n"), CString((LPCSTR)pErrorMsgs->GetBufferPointer()));
									}

									if(FAILED(hr)) {delete [] buff; return false;}

									DWORD id = (tfx << 8) | (bpp << 6) | (tcc << 5) | (aem << 4) | (fog << 3) | (rt << 2) | (fst << 1) | clamp;

									DWORD* buff = (DWORD*)pShader->GetBufferPointer();
									DWORD size = pShader->GetBufferSize() / 4;

									DWORD dup_id = ~0;

									CString hash;

									TCHAR* s = hash.GetBufferSetLength(size*8 + 1);

									for(int i = 0; i < size; i++)
									{
										_stprintf(&s[i*8], _T("%08x"), buff[i]);
									}

									if(CAtlMap<CString, DWORD, CStringElementTraits<CString> >::CPair* p = map.Lookup(hash))
									{
										dup_id = p->m_value;
									}
									else
									{
										map[hash] = id;
									}

									if(FILE* fp = _tfopen(fn + _T(".cpp"), _T("ab+")))
									{
										if(dup_id != ~0)
										{
											_ftprintf(fp, _T("static const DWORD* %s_%04x = %s_%04x;\n\n"), target, id, target, dup_id);
										}
										else
										{
											_ftprintf(fp, _T("static const DWORD %s_%04x[] = \n{\n"), target, id);

											for(int i = 0; i < size; i++)
											{
												if((i & 7) == 0) _ftprintf(fp, _T("\t"));

												_ftprintf(fp, _T("0x%08x, "), buff[i]);
												
												if(((i + 1) & 7) == 0 || i == size - 1) _ftprintf(fp, _T("\n"));
											}

											_ftprintf(fp, _T("};\n\n"));
										}

										fclose(fp);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	delete [] buff;

	if(FILE* fp = _tfopen(fn + _T(".cpp"), _T("ab+")))
	{
		_ftprintf(fp, _T("const DWORD* %s_tfx[] = \n{\n"), target);

		for(int i = 0; i < 0x500; i++)
		{
			if((i & 7) == 0) _ftprintf(fp, _T("\t"));

			_ftprintf(fp, _T("%s_%04x, "), target, i);
			
			if(((i + 1) & 7) == 0 || i == size - 1) _ftprintf(fp, _T("\n"));
		}

		_ftprintf(fp, _T("};\n\n"));

		fclose(fp);
	}

	if(FILE* fp = _tfopen(fn + _T(".h"), _T("wb")))
	{
		_ftprintf(fp, _T("#pragma once\n\nextern const DWORD* %s_tfx[];\n"), target);

		fclose(fp);
	}

	return true;
}
