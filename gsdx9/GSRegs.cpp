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

// GIFPackedRegHandler*

void GSState::GIFPackedRegHandlerNull(GIFPackedReg* r)
{
	// ASSERT(0);
}

void GSState::GIFPackedRegHandlerPRIM(GIFPackedReg* r)
{
	GIFReg r2;
	r2.PRIM.i64 = r->PRIM.PRIM;
	GIFRegHandlerPRIM(&r2);
}

void GSState::GIFPackedRegHandlerRGBA(GIFPackedReg* r)
{
	m_v.RGBAQ.R = r->RGBA.R;
	m_v.RGBAQ.G = r->RGBA.G;
	m_v.RGBAQ.B = r->RGBA.B;
	m_v.RGBAQ.A = r->RGBA.A;
/*
	__m128i r0 = _mm_loadu_si128((__m128i*)r);
	r0 = _mm_packs_epi32(r0, r0);
	r0 = _mm_packus_epi16(r0, r0);
	m_v.RGBAQ.ai32[0] = _mm_cvtsi128_si32(r0);
*/
	m_v.RGBAQ.Q = m_q;
}

void GSState::GIFPackedRegHandlerSTQ(GIFPackedReg* r)
{
	m_v.ST.S = r->STQ.S;
	m_v.ST.T = r->STQ.T;
/*	
	_mm_storel_epi64((__m128i*)&m_v.ST.i64, _mm_loadl_epi64((__m128i*)r));
*/
	m_q = r->STQ.Q;
}

void GSState::GIFPackedRegHandlerUV(GIFPackedReg* r)
{
	m_v.UV.U = r->UV.U;
	m_v.UV.V = r->UV.V;
/*
	__m128i r0 = _mm_loadu_si128((__m128i*)r);
	r0 = _mm_packs_epi32(r0, r0);
	m_v.UV.ai32[0] = _mm_cvtsi128_si32(r0);
*/
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

void GSState::GIFPackedRegHandlerTEX0_1(GIFPackedReg* r)
{
	GIFRegHandlerTEX0_1((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerTEX0_2(GIFPackedReg* r)
{
	GIFRegHandlerTEX0_2((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerCLAMP_1(GIFPackedReg* r)
{
	GIFRegHandlerCLAMP_1((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerCLAMP_2(GIFPackedReg* r)
{
	GIFRegHandlerCLAMP_2((GIFReg*)&r->ai64[0]);
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
	ASSERT(0);
}

void GSState::GIFRegHandlerPRIM(GIFReg* r)
{
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

void GSState::GIFRegHandlerTEX0_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].TEX0.i64 != r->TEX0.i64)
	{
		Flush();
	}

	m_env.CTXT[0].TEX0 = r->TEX0;

	//ASSERT(m_env.CTXT[0].TEX0.TW <= 10 && m_env.CTXT[0].TEX0.TH <= 10);

	if(m_env.CTXT[0].TEX0.TW > 10) m_env.CTXT[0].TEX0.TW = 10;
	if(m_env.CTXT[0].TEX0.TH > 10) m_env.CTXT[0].TEX0.TH = 10;

	m_env.CTXT[0].TEX0.CPSM &= 0xa; // 1010b

	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].TEX0.PSM];

	FlushWriteTransfer();

	m_mem.WriteCLUT(r->TEX0, m_env.TEXCLUT);
}

void GSState::GIFRegHandlerTEX0_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].TEX0.i64 != r->TEX0.i64)
	{
		Flush();
	}

	m_env.CTXT[1].TEX0 = r->TEX0;

	//ASSERT(m_env.CTXT[1].TEX0.TW <= 10 && m_env.CTXT[1].TEX0.TH <= 10);

	if(m_env.CTXT[1].TEX0.TW > 10) m_env.CTXT[1].TEX0.TW = 10;
	if(m_env.CTXT[1].TEX0.TH > 10) m_env.CTXT[1].TEX0.TH = 10;

	m_env.CTXT[1].TEX0.CPSM &= 0xa; // 1010b

	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].TEX0.PSM];

	FlushWriteTransfer();

	m_mem.WriteCLUT(r->TEX0, m_env.TEXCLUT);
}

void GSState::GIFRegHandlerCLAMP_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].CLAMP.i64 != r->CLAMP.i64)
	{
		Flush();
	}

	m_env.CTXT[0].CLAMP = r->CLAMP;
}

void GSState::GIFRegHandlerCLAMP_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].CLAMP.i64 != r->CLAMP.i64)
	{
		Flush();
	}

	m_env.CTXT[1].CLAMP = r->CLAMP;
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

void GSState::GIFRegHandlerTEX1_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].TEX1.i64 != r->TEX1.i64)
	{
		Flush();
	}

	m_env.CTXT[0].TEX1 = r->TEX1;
}

void GSState::GIFRegHandlerTEX1_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].TEX1.i64 != r->TEX1.i64)
	{
		Flush();
	}

	m_env.CTXT[1].TEX1 = r->TEX1;
}

void GSState::GIFRegHandlerTEX2_1(GIFReg* r)
{
	// if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].TEX2.i64 != r->TEX2.i64)
	{
		Flush(); // FIXME: must flush here, CLUT may be overwritten for the other context (not happening when writing TEX0, strange)
	}

	m_env.CTXT[0].TEX2 = r->TEX2;

	FlushWriteTransfer();

	m_mem.WriteCLUT(*(GIFRegTEX0*)&r->TEX2, m_env.TEXCLUT);
}

void GSState::GIFRegHandlerTEX2_2(GIFReg* r)
{
	// if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].TEX2.i64 != r->TEX2.i64)
	{
		Flush(); // FIXME: must flush here, CLUT may be overwritten for the other context (not happening when writing TEX0, strange)
	}

	m_env.CTXT[1].TEX2 = r->TEX2;

	FlushWriteTransfer();

	m_mem.WriteCLUT(*(GIFRegTEX0*)&r->TEX2, m_env.TEXCLUT);
}

void GSState::GIFRegHandlerXYOFFSET_1(GIFReg* r)
{
	if(m_env.CTXT[0].XYOFFSET.i64 != r->XYOFFSET.i64)
	{
		Flush();
	}

	m_env.CTXT[0].XYOFFSET = r->XYOFFSET;
}

void GSState::GIFRegHandlerXYOFFSET_2(GIFReg* r)
{
	if(m_env.CTXT[1].XYOFFSET.i64 != r->XYOFFSET.i64)
	{
		Flush();
	}

	m_env.CTXT[1].XYOFFSET = r->XYOFFSET;
}

void GSState::GIFRegHandlerPRMODECONT(GIFReg* r)
{
	if(m_env.PRMODECONT.i64 != r->PRMODECONT.i64)
	{
		Flush();
	}

	m_env.PRMODECONT = r->PRMODECONT;

	m_pPRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

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

void GSState::GIFRegHandlerMIPTBP1_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].MIPTBP1.i64 != r->MIPTBP1.i64)
	{
		Flush();
	}

	m_env.CTXT[0].MIPTBP1 = r->MIPTBP1;
}

void GSState::GIFRegHandlerMIPTBP1_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].MIPTBP1.i64 != r->MIPTBP1.i64)
	{
		Flush();
	}

	m_env.CTXT[1].MIPTBP1 = r->MIPTBP1;
}

void GSState::GIFRegHandlerMIPTBP2_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].MIPTBP2.i64 != r->MIPTBP2.i64)
	{
		Flush();
	}

	m_env.CTXT[0].MIPTBP2 = r->MIPTBP2;
}

void GSState::GIFRegHandlerMIPTBP2_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].MIPTBP2.i64 != r->MIPTBP2.i64)
	{
		Flush();
	}

	m_env.CTXT[1].MIPTBP2 = r->MIPTBP2;
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

void GSState::GIFRegHandlerSCISSOR_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].SCISSOR.i64 != r->SCISSOR.i64)
	{
		Flush();
	}

	m_env.CTXT[0].SCISSOR = r->SCISSOR;
}

void GSState::GIFRegHandlerSCISSOR_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].SCISSOR.i64 != r->SCISSOR.i64)
	{
		Flush();
	}

	m_env.CTXT[1].SCISSOR = r->SCISSOR;
}

void GSState::GIFRegHandlerALPHA_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].ALPHA.i64 != r->ALPHA.i64)
	{
		Flush();
	}

	m_env.CTXT[0].ALPHA = r->ALPHA;
}

void GSState::GIFRegHandlerALPHA_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].ALPHA.i64 != r->ALPHA.i64)
	{
		Flush();
	}

	m_env.CTXT[1].ALPHA = r->ALPHA;
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

void GSState::GIFRegHandlerTEST_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].TEST.i64 != r->TEST.i64)
	{
		Flush();
	}

	m_env.CTXT[0].TEST = r->TEST;
}

void GSState::GIFRegHandlerTEST_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].TEST.i64 != r->TEST.i64)
	{
		Flush();
	}

	m_env.CTXT[1].TEST = r->TEST;
}

void GSState::GIFRegHandlerPABE(GIFReg* r)
{
	if(m_env.PABE.i64 != r->PABE.i64)
	{
		Flush();
	}

	m_env.PABE = r->PABE;
}

void GSState::GIFRegHandlerFBA_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].FBA.i64 != r->FBA.i64)
	{
		Flush();
	}

	m_env.CTXT[0].FBA = r->FBA;
}

void GSState::GIFRegHandlerFBA_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].FBA.i64 != r->FBA.i64)
	{
		Flush();
	}

	m_env.CTXT[1].FBA = r->FBA;
}

void GSState::GIFRegHandlerFRAME_1(GIFReg* r)
{
	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].FRAME.i64 != r->FRAME.i64)
	{
		Flush();
	}

	m_env.CTXT[0].FRAME = r->FRAME;

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].FRAME.PSM];
}

void GSState::GIFRegHandlerFRAME_2(GIFReg* r)
{
	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].FRAME.i64 != r->FRAME.i64)
	{
		Flush();
	}

	m_env.CTXT[1].FRAME = r->FRAME;

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].FRAME.PSM];
}

void GSState::GIFRegHandlerZBUF_1(GIFReg* r)
{
	r->ZBUF.PSM |= 0x30;

	if(m_pPRIM->CTXT == 0 && m_env.CTXT[0].ZBUF.i64 != r->ZBUF.i64)
	{
		Flush();
	}

	m_env.CTXT[0].ZBUF = r->ZBUF;

	if(m_env.CTXT[0].ZBUF.PSM != PSM_PSMZ32
	&& m_env.CTXT[0].ZBUF.PSM != PSM_PSMZ24
	&& m_env.CTXT[0].ZBUF.PSM != PSM_PSMZ16
	&& m_env.CTXT[0].ZBUF.PSM != PSM_PSMZ16S)
	{
		m_env.CTXT[0].ZBUF.PSM = PSM_PSMZ32;
	}

	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[0].ZBUF.PSM];
}

void GSState::GIFRegHandlerZBUF_2(GIFReg* r)
{
	r->ZBUF.PSM |= 0x30;

	if(m_pPRIM->CTXT == 1 && m_env.CTXT[1].ZBUF.i64 != r->ZBUF.i64)
	{
		Flush();
	}

	m_env.CTXT[1].ZBUF = r->ZBUF;

	if(m_env.CTXT[1].ZBUF.PSM != PSM_PSMZ32
	&& m_env.CTXT[1].ZBUF.PSM != PSM_PSMZ24
	&& m_env.CTXT[1].ZBUF.PSM != PSM_PSMZ16
	&& m_env.CTXT[1].ZBUF.PSM != PSM_PSMZ16S)
	{
		m_env.CTXT[1].ZBUF.PSM = PSM_PSMZ32;
	}

	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psmtbl[m_env.CTXT[1].ZBUF.PSM];
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

