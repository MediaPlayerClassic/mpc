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
#include "GSState.h"

void GSState::WriteStep()
{
//	if(m_y == m_env.TRXREG.RRH && m_x == m_env.TRXPOS.DSAX) ASSERT(0);

	if(++m_x == m_env.TRXREG.RRW)
	{
		m_x = m_env.TRXPOS.DSAX;
		m_y++;
	}
}

void GSState::ReadStep()
{
//	if(m_y == m_env.TRXREG.RRH && m_x == m_env.TRXPOS.SSAX) ASSERT(0);

	if(++m_x == m_env.TRXREG.RRW)
	{
		m_x = m_env.TRXPOS.SSAX;
		m_y++;
	}
}

void GSState::WriteTransfer(BYTE* pMem, int len)
{
	if(len == 0) return;

	// TODO: hmmmm
	if(m_pPRIM->TME && (m_env.BITBLTBUF.DBP == m_context->TEX0.TBP0 || m_env.BITBLTBUF.DBP == m_context->TEX0.CBP))
	{
		FlushPrim();
	}

	int bpp = GSLocalMemory::m_psmtbl[m_env.BITBLTBUF.DPSM].trbpp;
	int pitch = (m_env.TRXREG.RRW - m_env.TRXPOS.DSAX)*bpp>>3;

	if(pitch <= 0) {ASSERT(0); return;}

	int height = len / pitch;

	if(m_nTransferBytes > 0 || height < m_env.TRXREG.RRH - m_env.TRXPOS.DSAY)
	{
		ASSERT(len <= m_nTrMaxBytes); // transferring more than 4mb into a 4mb local mem doesn't make any sense

		len = min(m_nTrMaxBytes, len);

		if(m_nTransferBytes + len > m_nTrMaxBytes)
		{
			FlushWriteTransfer();
		}

		memcpy(&m_pTransferBuffer[m_nTransferBytes], pMem, len);

		m_nTransferBytes += len;
	}
	else
	{
		int x = m_x, y = m_y;

		(m_mem.*GSLocalMemory::m_psmtbl[m_env.BITBLTBUF.DPSM].st)(m_x, m_y, pMem, len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

		m_perfmon.Put(GSPerfMon::Swizzle, len);

		//ASSERT(m_env.TRXREG.RRH >= m_y - y);

		CRect r(m_env.TRXPOS.DSAX, y, m_env.TRXREG.RRW, min(m_x == m_env.TRXPOS.DSAX ? m_y : m_y+1, m_env.TRXREG.RRH));

		InvalidateTexture(m_env.BITBLTBUF, r);

		m_mem.InvalidateCLUT();
	}
}

void GSState::FlushWriteTransfer()
{
	if(!m_nTransferBytes) return;

	int x = m_x, y = m_y;

	(m_mem.*GSLocalMemory::m_psmtbl[m_env.BITBLTBUF.DPSM].st)(m_x, m_y, m_pTransferBuffer, m_nTransferBytes, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

	m_perfmon.Put(GSPerfMon::Swizzle, m_nTransferBytes);

	m_nTransferBytes = 0;

	//ASSERT(m_env.TRXREG.RRH >= m_y - y);

	CRect r(m_env.TRXPOS.DSAX, y, m_env.TRXREG.RRW, min(m_x == m_env.TRXPOS.DSAX ? m_y : m_y+1, m_env.TRXREG.RRH));

	InvalidateTexture(m_env.BITBLTBUF, r);

	m_mem.InvalidateCLUT();
}

void GSState::ReadTransfer(BYTE* pMem, int len)
{
	BYTE* pb = (BYTE*)pMem;
	WORD* pw = (WORD*)pMem;
	DWORD* pd = (DWORD*)pMem;

	if(m_y >= (int)m_env.TRXREG.RRH) {ASSERT(0); return;}

	if(m_x == m_env.TRXPOS.SSAX && m_y == m_env.TRXPOS.SSAY)
	{
		CRect r(m_env.TRXPOS.SSAX, m_env.TRXPOS.SSAY, m_env.TRXREG.RRW, m_env.TRXREG.RRH);
		InvalidateLocalMem(m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW, m_env.BITBLTBUF.SPSM, r);
	}

	switch(m_env.BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_mem.readPixel32(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_mem.readPixel24(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_mem.readPixel16(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_mem.readPixel16S(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_mem.readPixel8(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_mem.readPixel4(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_mem.readPixel4(m_x+1, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_mem.readPixel8H(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_mem.readPixel4HL(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_mem.readPixel4HL(m_x+1, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_mem.readPixel4HH(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_mem.readPixel4HH(m_x+1, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_mem.readPixel32Z(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_mem.readPixel24Z(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_mem.readPixel16Z(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_mem.readPixel16SZ(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	}
}

void GSState::MoveTransfer()
{
	GSLocalMemory::readPixel rp = GSLocalMemory::m_psmtbl[m_env.BITBLTBUF.SPSM].rp;
	GSLocalMemory::writePixel wp = GSLocalMemory::m_psmtbl[m_env.BITBLTBUF.DPSM].wp;

	int sx = m_env.TRXPOS.SSAX;
	int dx = m_env.TRXPOS.DSAX;
	int sy = m_env.TRXPOS.SSAY;
	int dy = m_env.TRXPOS.DSAY;
	int w = m_env.TRXREG.RRW;
	int h = m_env.TRXREG.RRH;
	int xinc = 1;
	int yinc = 1;

	if(sx < dx) sx += w-1, dx += w-1, xinc = -1;
	if(sy < dy) sy += h-1, dy += h-1, yinc = -1;

	for(int y = 0; y < h; y++, sy += yinc, dy += yinc, sx -= xinc*w, dx -= xinc*w)
		for(int x = 0; x < w; x++, sx += xinc, dx += xinc)
			(m_mem.*wp)(dx, dy, (m_mem.*rp)(sx, sy, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW), m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW);
}


