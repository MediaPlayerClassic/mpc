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
#include "GSTextureCache.h"
#include "GSState.h"

GSTextureCache::GSRenderTarget::GSRenderTarget(GSTextureCache* tc)
	: GSSurface(tc)
	, m_used(true)
{
}

bool GSTextureCache::GSRenderTarget::Create(int w, int h)
{
	HRESULT hr;

	m_size = w * h * 4;

	hr = m_tc->CreateRenderTarget(w, h, &m_texture, &m_surface, &m_desc);
	
	if(FAILED(hr)) return false;

	hr = m_tc->m_state->m_dev->SetRenderTarget(0, m_surface);
	hr = m_tc->m_state->m_dev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	return true;
}

extern int s_n;
extern bool s_dump;
extern bool s_save;

void GSTextureCache::GSRenderTarget::Update()
{
	__super::Update();

	// FIXME: the union of the rects may also update wrong parts of the render target (but a lot faster :)

	CRect r = m_dirty.GetDirtyRect(m_TEX0);

	m_dirty.RemoveAll();

	if(r.IsRectEmpty()) return;

	TRACE(_T("%d,%d - %d,%d (%08x)\n"), r.left, r.top, r.right, r.bottom, m_TEX0.TBP0);

	HRESULT hr;

	int w = (r.Width() + 3) & ~3;
	int h = r.Height();

	CComPtr<IDirect3DTexture9> texture;
	CComPtr<IDirect3DSurface9> surface;
	
	hr = m_tc->CreateTexture(w, h, D3DFMT_A8R8G8B8, &texture, &surface);

	D3DLOCKED_RECT lr;

	if(texture && SUCCEEDED(texture->LockRect(0, &lr, NULL, 0)))
	{
		GIFRegTEXA TEXA;

		TEXA.AEM = 1;
		TEXA.TA0 = 0;
		TEXA.TA1 = 0x80;

		GIFRegCLAMP CLAMP;

		CLAMP.WMS = 0;
		CLAMP.WMT = 0;

		m_tc->m_state->m_mem.ReadTexture(r, (BYTE*)lr.pBits, lr.Pitch, m_TEX0, TEXA, CLAMP);
		
		m_tc->m_state->m_perfmon.Put(GSPerfMon::Unswizzle, r.Width() * r.Height() * 4);

		texture->UnlockRect(0);

		//

		GSState* s = m_tc->m_state;

		hr = s->m_dev->SetRenderTarget(0, m_surface);
		hr = s->m_dev->SetDepthStencilSurface(NULL);

		hr = s->m_dev->SetTexture(0, texture);
		hr = s->m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		hr = s->m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		hr = s->m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = s->m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		hr = s->m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		hr = s->m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		hr = s->m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

		hr = s->m_dev->SetVertexShader(NULL);
		hr = s->m_dev->SetPixelShader(m_tc->m_ps[0]);

		float left = m_scale.x * r.left;
		float top = m_scale.y * r.top;
		float right = m_scale.x * r.right;
		float bottom = m_scale.y * r.bottom;

		struct
		{
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] =
		{
			{(float)left, (float)top, 0.5f, 2.0f, 0, 0},
			{(float)right, (float)top, 0.5f, 2.0f, 1, 0},
			{(float)left, (float)bottom, 0.5f, 2.0f, 0, 1},
			{(float)right, (float)bottom, 0.5f, 2.0f, 1, 1},
		};

		hr = s->m_dev->BeginScene();
		hr = s->m_dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = s->m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
		hr = s->m_dev->EndScene();

		m_tc->Recycle(surface);
	}
/*
if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_dw_%05x.bmp"), s_n++, s->m_perfmon.GetFrame(), m_TEX0.TBP0);
	if(s_save) ::D3DXSaveTextureToFile(str, D3DXIFF_BMP, m_texture, NULL);
}
*/
}

void GSTextureCache::GSRenderTarget::Read(CRect r)
{
	if(!m_used) return;

	m_used = false;

	// TODO: 16 bit

	if(m_TEX0.PSM != PSM_PSMCT32 && m_TEX0.PSM != PSM_PSMCT24)
	{
		ASSERT(0);
		return;
	}

	HRESULT hr;

	GSState* s = m_tc->m_state;

	CComPtr<IDirect3DSurface9> offscreen;

	hr = m_tc->CreateOffscreenPlainSurface(r.Width(), r.Height(), D3DFMT_A8R8G8B8, &offscreen);

	if(FAILED(hr)) return;

	CComPtr<IDirect3DTexture9> texture;
	CComPtr<IDirect3DSurface9> surface;
	D3DSURFACE_DESC desc;

	hr = m_tc->CreateRenderTarget(r.Width(), r.Height(), &texture, &surface, &desc);

	if(FAILED(hr)) return;

	hr = s->m_dev->SetRenderTarget(0, surface);
	hr = s->m_dev->SetDepthStencilSurface(NULL);

	hr = s->m_dev->SetTexture(0, m_texture);
	hr = s->m_dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hr = s->m_dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	hr = s->m_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hr = s->m_dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hr = s->m_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	hr = s->m_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hr = s->m_dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RGBA);

	hr = s->m_dev->SetVertexShader(NULL);
	hr = s->m_dev->SetPixelShader(m_tc->m_ps[1]);

	float left = m_scale.x * r.left / m_desc.Width;
	float top = m_scale.y * r.top / m_desc.Height;
	float right = m_scale.x * r.right / m_desc.Width;
	float bottom = m_scale.y * r.bottom / m_desc.Height;

	struct
	{
		float x, y, z, rhw;
		float tu, tv;
	}
	pVertices[] =
	{
		{0, 0, 0.5f, 2.0f, left, top},
		{(float)desc.Width, 0, 0.5f, 2.0f, right, top},
		{0, (float)desc.Height, 0.5f, 2.0f, left, bottom},
		{(float)desc.Width, (float)desc.Height, 0.5f, 2.0f, right, bottom},
	};

	hr = s->m_dev->BeginScene();
	hr = s->m_dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
	hr = s->m_dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
	hr = s->m_dev->EndScene();

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_drsrc_%05x.bmp"), s_n++, s->m_perfmon.GetFrame(), m_TEX0.TBP0);
//	if(s_save) 
		::D3DXSaveTextureToFile(str, D3DXIFF_BMP, m_texture, NULL);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_dr_%05x.bmp"), s_n++, s->m_perfmon.GetFrame(), m_TEX0.TBP0);
//	if(s_save) 
		::D3DXSaveTextureToFile(str, D3DXIFF_BMP, texture, NULL);
}

	hr = s->m_dev->GetRenderTargetData(surface, offscreen);

	m_tc->Recycle(surface);

	if(SUCCEEDED(hr))
	{
		D3DLOCKED_RECT lr;

		if(SUCCEEDED(offscreen->LockRect(&lr, NULL, D3DLOCK_READONLY)))
		{
			// TODO: block level write

			DWORD bp = m_TEX0.TBP0;
			DWORD bw = m_TEX0.TBW;

			GSLocalMemory::pixelAddress pa = GSLocalMemory::m_psmtbl[m_TEX0.PSM].pa;
			GSLocalMemory::writeFrameAddr wfa = GSLocalMemory::m_psmtbl[m_TEX0.PSM].wfa;

			BYTE* bits = (BYTE*)lr.pBits;

			if(m_TEX0.PSM == PSM_PSMCT32)
			{
				for(int y = r.top; y < r.bottom; y++, bits += lr.Pitch)
				{
					DWORD addr = pa(r.left, y, bp, bw);
					int* offset = &GSLocalMemory::m_psmtbl[m_TEX0.PSM].rowOffset[y & 7][r.left];

					for(int x = r.left, i = 0; x < r.right; x++, i++)
					{
						s->m_mem.writePixel32(addr + offset[i], ((DWORD*)bits)[i]);
					}
				}
			}
			else if(m_TEX0.PSM != PSM_PSMCT24)
			{
				for(int y = r.top; y < r.bottom; y++, bits += lr.Pitch)
				{
					DWORD addr = pa(r.left, y, bp, bw);
					int* offset = &GSLocalMemory::m_psmtbl[m_TEX0.PSM].rowOffset[y & 7][r.left];

					for(int x = r.left, i = 0; x < r.right; x++, i++)
					{
						s->m_mem.writePixel24(addr + offset[i], ((DWORD*)bits)[i]);
					}
				}
			}
			else
			{
				for(int y = r.top; y < r.bottom; y++, bits += lr.Pitch)
				{
					DWORD addr = pa(r.left, y, bp, bw);
					int* offset = &GSLocalMemory::m_psmtbl[m_TEX0.PSM].rowOffset[y & 7][r.left];

					for(int x = r.left, i = 0; x < r.right; x++, i++)
					{
						(s->m_mem.*wfa)(addr + offset[i], ((DWORD*)bits)[i]);
					}
				}
			}

			offscreen->UnlockRect();
		}
	}

	m_tc->Recycle(offscreen);
}
