/* 
 *	Copyright (C) 2003-2004 Gabest
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
#include "GSState.h"

void GSState::WriteStep()
{
//	if(m_y == m_rs.TRXREG.RRH && m_x == m_rs.TRXPOS.DSAX) ASSERT(0);

	if(++m_x == m_rs.TRXREG.RRW)
	{
		m_x = m_rs.TRXPOS.DSAX;
		m_y++;
	}
}

void GSState::ReadStep()
{
//	if(m_y == m_rs.TRXREG.RRH && m_x == m_rs.TRXPOS.SSAX) ASSERT(0);

	if(++m_x == m_rs.TRXREG.RRW)
	{
		m_x = m_rs.TRXPOS.SSAX;
		m_y++;
	}
}

void GSState::WriteTransfer(BYTE* pMem, int len)
{
/*	CComPtr<IDirect3DTexture9> pRT;
	if(m_pRenderTargets.Lookup(m_rs.BITBLTBUF.DBP, pRT))
*/	{
		m_tc.InvalidateByTBP(m_rs.BITBLTBUF.DBP);
		m_tc.InvalidateByCBP(m_rs.BITBLTBUF.DBP);
	}

	if((m_rs.BITBLTBUF.DBP == m_de.CTXT[m_de.PRIM.CTXT].TEX0.TBP0
	|| m_rs.BITBLTBUF.DBP == m_de.CTXT[m_de.PRIM.CTXT].TEX0.CBP)
	&& m_de.PRIM.TME)
	{
		FlushPrim();
	}

	BYTE* pb = (BYTE*)pMem;
	WORD* pw = (WORD*)pMem;
	DWORD* pd = (DWORD*)pMem;

	// if(m_y >= (int)m_rs.TRXREG.RRH) {ASSERT(0); return;}

	switch(m_rs.BITBLTBUF.DPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; WriteStep(), pd++)
			m_lm.writePixel32(m_x, m_y, *pd, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; WriteStep(), pb+=3)
			m_lm.writePixel24(m_x, m_y, *(DWORD*)pb, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; WriteStep(), pw++)
			m_lm.writePixel16(m_x, m_y, *pw, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; WriteStep(), pw++)
			m_lm.writePixel16S(m_x, m_y, *pw, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; WriteStep(), pb++)
			m_lm.writePixel8(m_x, m_y, *pb, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; WriteStep(), WriteStep(), pb++)
			m_lm.writePixel4(m_x, m_y, *pb&0xf, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW),
			m_lm.writePixel4(m_x+1, m_y, *pb>>4, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; WriteStep(), pb++)
			m_lm.writePixel8H(m_x, m_y, *pb, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; WriteStep(), WriteStep(), pb++)
			m_lm.writePixel4HL(m_x, m_y, *pb&0xf, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW),
			m_lm.writePixel4HL(m_x+1, m_y, *pb>>4, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; WriteStep(), WriteStep(), pb++)
			m_lm.writePixel4HH(m_x, m_y, *pb&0xf, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW),
			m_lm.writePixel4HH(m_x+1, m_y, *pb>>4, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; WriteStep(), pd++)
			m_lm.writePixel32Z(m_x, m_y, *pd, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; WriteStep(), pb+=3)
			m_lm.writePixel24Z(m_x, m_y, *(DWORD*)pb, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; WriteStep(), pw++)
			m_lm.writePixel16Z(m_x, m_y, *pw, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; WriteStep(), pw++)
			m_lm.writePixel16SZ(m_x, m_y, *pw, m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
		break;
	}
}

void GSState::ReadTransfer(BYTE* pMem, int len)
{
	BYTE* pb = (BYTE*)pMem;
	WORD* pw = (WORD*)pMem;
	DWORD* pd = (DWORD*)pMem;

	if(m_y >= (int)m_rs.TRXREG.RRH) {ASSERT(0); return;}

	switch(m_rs.BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_lm.readPixel32(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_lm.readPixel24(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16S(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_lm.readPixel8(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4(m_x+1, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_lm.readPixel8H(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4HL(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4HL(m_x+1, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4HH(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4HH(m_x+1, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_lm.readPixel32Z(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_lm.readPixel24Z(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16Z(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16SZ(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	}
}

void GSState::MoveTransfer()
{
	GSLocalMemory::readPixel rp = m_lm.GetReadPixel(m_rs.BITBLTBUF.SPSM);
	GSLocalMemory::writePixel wp = m_lm.GetWritePixel(m_rs.BITBLTBUF.DPSM);
	for(int y = 0, sy = m_rs.TRXPOS.SSAY, dy = m_rs.TRXPOS.DSAY; y < m_rs.TRXREG.RRH; y++, sy++, dy++)
		for(int x = 0, sx = m_rs.TRXPOS.SSAX, dx = m_rs.TRXPOS.DSAX; x < m_rs.TRXREG.RRW; x++, sx++, dx++)
			(m_lm.*wp)(dx, dy, (m_lm.*rp)(sx, sy, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW), m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
}

bool GSState::CreateTexture(GSTexture& t)
{
	DrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	int tw = 1<<ctxt->TEX0.TW;
	int th = 1<<ctxt->TEX0.TH;
/*
	CComPtr<IDirect3DTexture9> pRT;
	if(!m_pRenderTargets.Lookup(ctxt->TEX0.TBP0, pRT))
	{
		if(m_lm.IsDirty(ctxt->TEX0))
		{
			LOG((_T("TBP0 dirty: %08x\n"), ctxt->TEX0.TBP0));
			m_tc.InvalidateByTBP(ctxt->TEX0.TBP0);
		}

		if(m_lm.IsPalDirty(ctxt->TEX0))
		{
			LOG((_T("CBP dirty: %08x\n"), ctxt->TEX0.CBP));
			m_tc.InvalidateByCBP(ctxt->TEX0.CBP);
		}
	}
*/
	if(m_tc.Lookup(ctxt->TEX0, ctxt->CLAMP, m_de.TEXA, t))
		return(true);

//	LOG((_T("TBP0/CBP: %08x/%08x\n"), ctxt->TEX0.TBP0, ctxt->TEX0.CBP));

	m_lm.setupCLUT(ctxt->TEX0, m_de.TEXCLUT, m_de.TEXA);

	GSLocalMemory::readTexel rt = m_lm.GetReadTexel(ctxt->TEX0.PSM);

	CComPtr<IDirect3DTexture9> pTexture;
	HRESULT hr = m_pD3DDev->CreateTexture(tw, th, 0, 0 , D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
	if(FAILED(hr) || !pTexture) return(false);

	D3DLOCKED_RECT r;
	if(FAILED(hr = pTexture->LockRect(0, &r, NULL, 0)))
		return(false);

	BYTE* dst = (BYTE*)r.pBits;

	if((ctxt->CLAMP.WMS&2) || (ctxt->CLAMP.WMT&2))
	{
		int tx, ty;

		for(int y = 0, diff = r.Pitch - tw*4; y < th; y++, dst += diff)
		{
			for(int x = 0; x < tw; x++, dst += 4)
			{
				switch(ctxt->CLAMP.WMS)
				{
				default: tx = x; break;
				case 2: tx = x < ctxt->CLAMP.MINU ? ctxt->CLAMP.MINU : x > ctxt->CLAMP.MAXU ? ctxt->CLAMP.MAXU : x; break;
				case 3: tx = (x & ctxt->CLAMP.MINU) | ctxt->CLAMP.MAXU; break;
				}

				switch(ctxt->CLAMP.WMT)
				{
				default: ty = y; break;
				case 2: ty = y < ctxt->CLAMP.MINV ? ctxt->CLAMP.MINV : y > ctxt->CLAMP.MAXV ? ctxt->CLAMP.MAXV : y; break;
				case 3: ty = (y & ctxt->CLAMP.MINV) | ctxt->CLAMP.MAXV; break;
				}

				*(DWORD*)dst = (m_lm.*rt)(tx, ty, ctxt->TEX0.TBP0, ctxt->TEX0.TBW, ctxt->TEX0.TCC, m_de.TEXA);
			}
		}
	}
	else
	{
		for(int y = 0, diff = r.Pitch - tw*4; y < th; y++, dst += diff)
			for(int x = 0; x < tw; x++, dst += 4)
				*(DWORD*)dst = (m_lm.*rt)(x, y, ctxt->TEX0.TBP0, ctxt->TEX0.TBW, ctxt->TEX0.TCC, m_de.TEXA);
	}

	pTexture->UnlockRect(0);

	m_tc.Add(ctxt->TEX0, ctxt->CLAMP, m_de.TEXA, 1, 1, pTexture);
	if(!m_tc.Lookup(ctxt->TEX0, ctxt->CLAMP, m_de.TEXA, t)) // ehe
		ASSERT(0);
/*	
	t.m_pTexture = pTexture;
	t.m_TEX0 = ctxt->TEX0;
	t.m_CLAMP = ctxt->CLAMP;
	t.m_TEXA = m_de.TEXA;
	t.m_xscale = t.m_yscale = 1;
*/
#ifdef DEBUG_SAVETEXTURES
	CString fn;
	fn.Format(_T("c:\\%08I64x_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d-%I64d_%I64d-%I64d.bmp"), 
		ctxt->TEX0.TBP0, ctxt->TEX0.PSM, ctxt->TEX0.TBW, 
		ctxt->TEX0.TW, ctxt->TEX0.TH,
		ctxt->CLAMP.WMS, ctxt->CLAMP.WMT, ctxt->CLAMP.MINU, ctxt->CLAMP.MAXU, ctxt->CLAMP.MINV, ctxt->CLAMP.MAXV);
	D3DXSaveTextureToFile(fn, D3DXIFF_BMP, pTexture, NULL);
#endif

	return(true);
}