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

static __m128i _000000ff = _mm_set1_epi32(0x000000ff);
static __m128i _00003fff = _mm_set1_epi32(0x00003fff);

// GIFPackedRegHandler*

void GSState::GIFPackedRegHandlerNull(GIFPackedReg* r)
{
	// ASSERT(0);
}

void GSState::GIFPackedRegHandlerPRIM(GIFPackedReg* r)
{
	ASSERT(r->PRIM.PRIM < 7);

	GIFReg r2;
	r2.PRIM.i64 = r->PRIM.PRIM;
	GIFRegHandlerPRIM(&r2);
}

void GSState::GIFPackedRegHandlerRGBA(GIFPackedReg* r)
{
#if defined(_M_AMD64) || _M_IX86_FP >= 2

	__m128i r0 = _mm_loadu_si128((__m128i*)r);
	r0 = _mm_and_si128(r0, _000000ff);
	r0 = _mm_packs_epi32(r0, r0);
	r0 = _mm_packus_epi16(r0, r0);
	m_v.RGBAQ.ai32[0] = _mm_cvtsi128_si32(r0);

#else

	m_v.RGBAQ.R = r->RGBA.R;
	m_v.RGBAQ.G = r->RGBA.G;
	m_v.RGBAQ.B = r->RGBA.B;
	m_v.RGBAQ.A = r->RGBA.A;

#endif

	m_v.RGBAQ.Q = m_q;
}

void GSState::GIFPackedRegHandlerSTQ(GIFPackedReg* r)
{
#if defined(_M_AMD64)

	m_v.ST.i64 = r->ai64[0];

#elif _M_IX86_FP >= 2

	_mm_storel_epi64((__m128i*)&m_v.ST.i64, _mm_loadl_epi64((__m128i*)r));

#else

	m_v.ST.S = r->STQ.S;
	m_v.ST.T = r->STQ.T;

#endif

	m_q = r->STQ.Q;
}

void GSState::GIFPackedRegHandlerUV(GIFPackedReg* r)
{
#if defined(_M_AMD64) || _M_IX86_FP >= 2

	__m128i r0 = _mm_loadu_si128((__m128i*)r);
	r0 = _mm_and_si128(r0, _00003fff);
	r0 = _mm_packs_epi32(r0, r0);
	m_v.UV.ai32[0] = _mm_cvtsi128_si32(r0);

#else

	m_v.UV.U = r->UV.U;
	m_v.UV.V = r->UV.V;

#endif
}

void GSState::GIFPackedRegHandlerXYZF2(GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZF2.X;
	m_v.XYZ.Y = r->XYZF2.Y;
	m_v.XYZ.Z = r->XYZF2.Z;
	m_v.FOG.F = r->XYZF2.F;

	VertexKick(r->XYZF2.ADC);
}

void GSState::GIFPackedRegHandlerXYZ2(GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZ2.X;
	m_v.XYZ.Y = r->XYZ2.Y;
	m_v.XYZ.Z = r->XYZ2.Z;

	VertexKick(r->XYZ2.ADC);
}

template<int i> void GSState::GIFPackedRegHandlerTEX0(GIFPackedReg* r)
{
	GIFRegHandlerTEX0<i>((GIFReg*)&r->ai64[0]);
}

template<int i> void GSState::GIFPackedRegHandlerCLAMP(GIFPackedReg* r)
{
	GIFRegHandlerCLAMP<i>((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerFOG(GIFPackedReg* r)
{
	m_v.FOG.F = r->FOG.F;
}

void GSState::GIFPackedRegHandlerXYZF3(GIFPackedReg* r)
{
	GIFRegHandlerXYZF3((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerXYZ3(GIFPackedReg* r)
{
	GIFRegHandlerXYZ3((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerA_D(GIFPackedReg* r)
{
	(this->*m_fpGIFRegHandlers[(BYTE)r->A_D.ADDR])(&r->r);
}

void GSState::GIFPackedRegHandlerNOP(GIFPackedReg* r)
{
}

// GIFRegHandler*

void GSState::GIFRegHandlerNull(GIFReg* r)
{
	// ASSERT(0);
}

void GSState::GIFRegHandlerPRIM(GIFReg* r)
{
	// ASSERT(r->PRIM.PRIM < 7);

	if(m_env.PRIM.i64 != r->PRIM.i64)
	{
		Flush();
	}

	m_env.PRIM = r->PRIM;
	m_env.PRMODE._PRIM = r->PRIM.PRIM;

	if(m_env.PRMODECONT.AC)
	{
		m_context = &m_env.CTXT[m_env.PRIM.CTXT];
	}

	NewPrim();
}

void GSState::GIFRegHandlerRGBAQ(GIFReg* r)
{
	m_v.RGBAQ = r->RGBAQ;
}

void GSState::GIFRegHandlerST(GIFReg* r)
{
	m_v.ST = r->ST;
}

void GSState::GIFRegHandlerUV(GIFReg* r)
{
	m_v.UV = r->UV;
}

void GSState::GIFRegHandlerXYZF2(GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.ai32[0] = r->XYZF.ai32[0];
	m_v.XYZ.ai32[1] = r->XYZF.ai32[1] & 0x00ffffff;
	m_v.FOG.ai32[1] = r->XYZF.ai32[1] & 0xff000000;

	VertexKick(false);
}

void GSState::GIFRegHandlerXYZ2(GIFReg* r)
{
	m_v.XYZ = r->XYZ;

	VertexKick(false);
}

template<int i> void GSState::GIFRegHandlerTEX0(GIFReg* r)
{
	// even if TEX0 did not change, a new palette may have been uploaded and will overwrite the currently queued for drawing

	if(m_pPRIM->CTXT == i && m_env.CTXT[i].TEX0.i64 != r->TEX0.i64
	|| r->TEX0.CLD >= 1 && r->TEX0.CLD <= 3 && m_mem.IsCLUTDirty(r->TEX0, m_env.TEXCLUT))
	{
		Flush(); 
	}

	m_env.CTXT[i].TEX0 = r->TEX0;

	// ASSERT(m_env.CTXT[i].TEX0.TW <= 10 && m_env.CTXT[i].TEX0.TH <= 10 && (m_env.CTXT[i].TEX0.CPSM & ~0xa) == 0);

	if(m_env.CTXT[i].TEX0.TW > 10) m_env.CTXT[i].TEX0.TW = 10;
	if(m_env.CTXT[i].TEX0.TH > 10) m_env.CTXT[i].TEX0.TH = 10;

	m_env.CTXT[i].TEX0.CPSM &= 0xa; // 1010b

	m_env.CTXT[i].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[i].TEX0.PSM];

	FlushWriteTransfer();

	m_mem.WriteCLUT(r->TEX0, m_env.TEXCLUT);
}

template<int i> void GSState::GIFRegHandlerCLAMP(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].CLAMP.i64 != r->CLAMP.i64)
	{
		Flush();
	}

	m_env.CTXT[i].CLAMP = r->CLAMP;
}

void GSState::GIFRegHandlerFOG(GIFReg* r)
{
	m_v.FOG = r->FOG;
}

void GSState::GIFRegHandlerXYZF3(GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.ai32[0] = r->XYZF.ai32[0];
	m_v.XYZ.ai32[1] = r->XYZF.ai32[1] & 0x00ffffff;
	m_v.FOG.ai32[1] = r->XYZF.ai32[1] & 0xff000000;

	VertexKick(true);
}

void GSState::GIFRegHandlerXYZ3(GIFReg* r)
{
	m_v.XYZ = r->XYZ;

	VertexKick(true);
}

void GSState::GIFRegHandlerNOP(GIFReg* r)
{
}

template<int i> void GSState::GIFRegHandlerTEX1(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].TEX1.i64 != r->TEX1.i64)
	{
		Flush();
	}

	m_env.CTXT[i].TEX1 = r->TEX1;
}

template<int i> void GSState::GIFRegHandlerTEX2(GIFReg* r)
{
	// m_env.CTXT[i].TEX2 = r->TEX2; // not used

	UINT64 mask = 0xFFFFFFE003F00000ui64; // TEX2 bits

	r->i64 = (r->i64 & mask) | (m_env.CTXT[i].TEX0.i64 & ~mask);

	GIFRegHandlerTEX0<i>(r);
}

template<int i> void GSState::GIFRegHandlerXYOFFSET(GIFReg* r)
{
	if(m_env.CTXT[i].XYOFFSET.i64 != r->XYOFFSET.i64)
	{
		Flush();
	}

	m_env.CTXT[i].XYOFFSET = r->XYOFFSET;

	m_env.CTXT[i].UpdateScissor();
}

void GSState::GIFRegHandlerPRMODECONT(GIFReg* r)
{
	if(m_env.PRMODECONT.i64 != r->PRMODECONT.i64)
	{
		Flush();
	}

	m_env.PRMODECONT = r->PRMODECONT;

	m_pPRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	ASSERT(m_pPRIM->PRIM < 7);

	m_context = &m_env.CTXT[m_pPRIM->CTXT];
}

void GSState::GIFRegHandlerPRMODE(GIFReg* r)
{
	if(!m_env.PRMODECONT.AC)
	{
		Flush();
	}

	UINT32 _PRIM = m_env.PRMODE._PRIM;
	m_env.PRMODE = r->PRMODE;
	m_env.PRMODE._PRIM = _PRIM;

	m_context = &m_env.CTXT[m_pPRIM->CTXT];
}

void GSState::GIFRegHandlerTEXCLUT(GIFReg* r)
{
	if(m_env.TEXCLUT.i64 != r->TEXCLUT.i64)
	{
		Flush();
	}

	m_env.TEXCLUT = r->TEXCLUT;
}

void GSState::GIFRegHandlerSCANMSK(GIFReg* r)
{
	if(m_env.SCANMSK.i64 != r->SCANMSK.i64)
	{
		Flush();
	}

	m_env.SCANMSK = r->SCANMSK;
}

template<int i> void GSState::GIFRegHandlerMIPTBP1(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].MIPTBP1.i64 != r->MIPTBP1.i64)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP1 = r->MIPTBP1;
}

template<int i> void GSState::GIFRegHandlerMIPTBP2(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].MIPTBP2.i64 != r->MIPTBP2.i64)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP2 = r->MIPTBP2;
}

void GSState::GIFRegHandlerTEXA(GIFReg* r)
{
	if(m_env.TEXA.i64 != r->TEXA.i64)
	{
		Flush();
	}

	m_env.TEXA = r->TEXA;
}

void GSState::GIFRegHandlerFOGCOL(GIFReg* r)
{
	if(m_env.FOGCOL.i64 != r->FOGCOL.i64)
	{
		Flush();
	}

	m_env.FOGCOL = r->FOGCOL;
}

void GSState::GIFRegHandlerTEXFLUSH(GIFReg* r)
{
	// what should we do here?
}

template<int i> void GSState::GIFRegHandlerSCISSOR(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].SCISSOR.i64 != r->SCISSOR.i64)
	{
		Flush();
	}

	m_env.CTXT[i].SCISSOR = r->SCISSOR;

	m_env.CTXT[i].UpdateScissor();
}

template<int i> void GSState::GIFRegHandlerALPHA(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].ALPHA.i64 != r->ALPHA.i64)
	{
		Flush();
	}

	m_env.CTXT[i].ALPHA = r->ALPHA;
}

void GSState::GIFRegHandlerDIMX(GIFReg* r)
{
	if(m_env.DIMX.i64 != r->DIMX.i64)
	{
		Flush();
	}

	m_env.DIMX = r->DIMX;
}

void GSState::GIFRegHandlerDTHE(GIFReg* r)
{
	if(m_env.DTHE.i64 != r->DTHE.i64)
	{
		Flush();
	}

	m_env.DTHE = r->DTHE;
}

void GSState::GIFRegHandlerCOLCLAMP(GIFReg* r)
{
	if(m_env.COLCLAMP.i64 != r->COLCLAMP.i64)
	{
		Flush();
	}

	m_env.COLCLAMP = r->COLCLAMP;
}

template<int i> void GSState::GIFRegHandlerTEST(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].TEST.i64 != r->TEST.i64)
	{
		Flush();
	}

	m_env.CTXT[i].TEST = r->TEST;
}

void GSState::GIFRegHandlerPABE(GIFReg* r)
{
	if(m_env.PABE.i64 != r->PABE.i64)
	{
		Flush();
	}

	m_env.PABE = r->PABE;
}

template<int i> void GSState::GIFRegHandlerFBA(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].FBA.i64 != r->FBA.i64)
	{
		Flush();
	}

	m_env.CTXT[i].FBA = r->FBA;
}

template<int i> void GSState::GIFRegHandlerFRAME(GIFReg* r)
{
	if(m_pPRIM->CTXT == i && m_env.CTXT[i].FRAME.i64 != r->FRAME.i64)
	{
		Flush();
	}

	m_env.CTXT[i].FRAME = r->FRAME;

	m_env.CTXT[i].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[i].FRAME.PSM];
}

template<int i> void GSState::GIFRegHandlerZBUF(GIFReg* r)
{
	r->ZBUF.PSM |= 0x30;

	if(m_pPRIM->CTXT == i && m_env.CTXT[i].ZBUF.i64 != r->ZBUF.i64)
	{
		Flush();
	}

	m_env.CTXT[i].ZBUF = r->ZBUF;

	if(m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ32
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ24
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ16
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ16S)
	{
		m_env.CTXT[i].ZBUF.PSM = PSM_PSMZ32;
	}

	m_env.CTXT[i].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[i].ZBUF.PSM];
}

void GSState::GIFRegHandlerBITBLTBUF(GIFReg* r)
{
	if(m_env.BITBLTBUF.i64 != r->BITBLTBUF.i64)
	{
		FlushWriteTransfer();
	}

	m_env.BITBLTBUF = r->BITBLTBUF;
}

void GSState::GIFRegHandlerTRXPOS(GIFReg* r)
{
	if(m_env.TRXPOS.i64 != r->TRXPOS.i64)
	{
		FlushWriteTransfer();
	}

	m_env.TRXPOS = r->TRXPOS;
}

void GSState::GIFRegHandlerTRXREG(GIFReg* r)
{
	if(m_env.TRXREG.i64 != r->TRXREG.i64 || m_env.TRXREG2.i64 != r->TRXREG.i64)
	{
		FlushWriteTransfer();
	}

	m_env.TRXREG = m_env.TRXREG2 = r->TRXREG;
}

void GSState::GIFRegHandlerTRXDIR(GIFReg* r)
{
	Flush();

	m_env.TRXDIR = r->TRXDIR;

	switch(m_env.TRXDIR.XDIR)
	{
	case 0: // host -> local
		m_x = m_env.TRXPOS.DSAX;
		m_y = m_env.TRXPOS.DSAY;
		m_env.TRXREG.RRW = m_x + m_env.TRXREG2.RRW;
		m_env.TRXREG.RRH = m_y + m_env.TRXREG2.RRH;
		break;
	case 1: // local -> host
		m_x = m_env.TRXPOS.SSAX;
		m_y = m_env.TRXPOS.SSAY;
		m_env.TRXREG.RRW = m_x + m_env.TRXREG2.RRW;
		m_env.TRXREG.RRH = m_y + m_env.TRXREG2.RRH;
		break;
	case 2: // local -> local
		MoveTransfer();
		break;
	case 3: 
		ASSERT(0);
		break;
	}
}

void GSState::GIFRegHandlerHWREG(GIFReg* r)
{
	// TODO

	ASSERT(0);
}

void GSState::GIFRegHandlerSIGNAL(GIFReg* r)
{
	if(m_mt) return;

	m_regs.pSIGLBLID->SIGID = (m_regs.pSIGLBLID->SIGID & ~r->SIGNAL.IDMSK) | (r->SIGNAL.ID & r->SIGNAL.IDMSK);

	if(m_regs.pCSR->wSIGNAL) m_regs.pCSR->rSIGNAL = 1;
	if(!m_regs.pIMR->SIGMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerFINISH(GIFReg* r)
{
	if(m_mt) return;

	if(m_regs.pCSR->wFINISH) m_regs.pCSR->rFINISH = 1;
	if(!m_regs.pIMR->FINISHMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerLABEL(GIFReg* r)
{
	if(m_mt) return;

	m_regs.pSIGLBLID->LBLID = (m_regs.pSIGLBLID->LBLID & ~r->LABEL.IDMSK) | (r->LABEL.ID & r->LABEL.IDMSK);
}

//

void GSState::ResetHandlers()
{
	for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
	{
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;
	}

	m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerPRIM;
	m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerRGBA;
	m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerSTQ;
	m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerUV;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = &GSState::GIFPackedRegHandlerTEX0<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = &GSState::GIFPackedRegHandlerTEX0<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GSState::GIFPackedRegHandlerCLAMP<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GSState::GIFPackedRegHandlerCLAMP<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerXYZF3;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerXYZ3;
	m_fpGIFPackedRegHandlers[GIF_REG_A_D] = &GSState::GIFPackedRegHandlerA_D;
	m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

	for(int i = 0; i < countof(m_fpGIFRegHandlers); i++)
	{
		m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNull;
	}

	m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
	m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
	m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
	m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerXYZF2;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerXYZ2;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GSState::GIFRegHandlerTEX0<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GSState::GIFRegHandlerTEX0<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GSState::GIFRegHandlerCLAMP<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GSState::GIFRegHandlerCLAMP<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOG] = &GSState::GIFRegHandlerFOG;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerXYZF3;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerXYZ3;
	m_fpGIFRegHandlers[GIF_A_D_REG_NOP] = &GSState::GIFRegHandlerNOP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_1] = &GSState::GIFRegHandlerTEX1<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_2] = &GSState::GIFRegHandlerTEX1<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_1] = &GSState::GIFRegHandlerTEX2<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_2] = &GSState::GIFRegHandlerTEX2<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_1] = &GSState::GIFRegHandlerXYOFFSET<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_2] = &GSState::GIFRegHandlerXYOFFSET<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerPRMODECONT;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerPRMODE;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXCLUT] = &GSState::GIFRegHandlerTEXCLUT;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCANMSK] = &GSState::GIFRegHandlerSCANMSK;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_1] = &GSState::GIFRegHandlerMIPTBP1<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_2] = &GSState::GIFRegHandlerMIPTBP1<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_1] = &GSState::GIFRegHandlerMIPTBP2<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_2] = &GSState::GIFRegHandlerMIPTBP2<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXA] = &GSState::GIFRegHandlerTEXA;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOGCOL] = &GSState::GIFRegHandlerFOGCOL;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXFLUSH] = &GSState::GIFRegHandlerTEXFLUSH;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_1] = &GSState::GIFRegHandlerSCISSOR<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_2] = &GSState::GIFRegHandlerSCISSOR<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_1] = &GSState::GIFRegHandlerALPHA<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_2] = &GSState::GIFRegHandlerALPHA<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_DIMX] = &GSState::GIFRegHandlerDIMX;
	m_fpGIFRegHandlers[GIF_A_D_REG_DTHE] = &GSState::GIFRegHandlerDTHE;
	m_fpGIFRegHandlers[GIF_A_D_REG_COLCLAMP] = &GSState::GIFRegHandlerCOLCLAMP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_1] = &GSState::GIFRegHandlerTEST<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_2] = &GSState::GIFRegHandlerTEST<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_PABE] = &GSState::GIFRegHandlerPABE;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_1] = &GSState::GIFRegHandlerFBA<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_2] = &GSState::GIFRegHandlerFBA<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_1] = &GSState::GIFRegHandlerFRAME<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_2] = &GSState::GIFRegHandlerFRAME<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_1] = &GSState::GIFRegHandlerZBUF<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_2] = &GSState::GIFRegHandlerZBUF<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_BITBLTBUF] = &GSState::GIFRegHandlerBITBLTBUF;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXPOS] = &GSState::GIFRegHandlerTRXPOS;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXREG] = &GSState::GIFRegHandlerTRXREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXDIR] = &GSState::GIFRegHandlerTRXDIR;
	m_fpGIFRegHandlers[GIF_A_D_REG_HWREG] = &GSState::GIFRegHandlerHWREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GSState::GIFRegHandlerSIGNAL;
	m_fpGIFRegHandlers[GIF_A_D_REG_FINISH] = &GSState::GIFRegHandlerFINISH;
	m_fpGIFRegHandlers[GIF_A_D_REG_LABEL] = &GSState::GIFRegHandlerLABEL;
}

