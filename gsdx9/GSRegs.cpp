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

// GIFPackedRegHandler*

void GSState::GIFPackedRegHandlerNull(GIFPackedReg* r)
{
	LOG((_T("GIFPackedRegHandlerNull(%016I64x%016I64x)\n"), r->ai64[0], r->ai64[1]));
}

void GSState::GIFPackedRegHandlerPRIM(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.PRIM.i64 = r->PRIM.PRIM;
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r2);
}

void GSState::GIFPackedRegHandlerRGBAQ(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.RGBAQ.R = r->RGBA.R;
	r2.RGBAQ.G = r->RGBA.G;
	r2.RGBAQ.B = r->RGBA.B;
	r2.RGBAQ.A = r->RGBA.A;
	r2.RGBAQ.Q = m_v.RGBAQ.Q;
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ])(&r2);
}

void GSState::GIFPackedRegHandlerST(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.ST.S = r->STQ.S;
	r2.ST.T = r->STQ.T;
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_ST])(&r2);
	// for some reason only ST would have to be saved here (eeuser_e.pdf p154), but not in reality...
	LOG((_T("Packed ")));
	r2.RGBAQ.R = m_v.RGBAQ.R;
	r2.RGBAQ.G = m_v.RGBAQ.G;
	r2.RGBAQ.B = m_v.RGBAQ.B;
	r2.RGBAQ.A = m_v.RGBAQ.A;
	r2.RGBAQ.Q = r->STQ.Q;
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ])(&r2);
}

void GSState::GIFPackedRegHandlerUV(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.UV.U = r->UV.U;
	r2.UV.V = r->UV.V;
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_UV])(&r2);
}

void GSState::GIFPackedRegHandlerXYZF2(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.XYZF.X = r->XYZF2.X;
	r2.XYZF.Y = r->XYZF2.Y;
	r2.XYZF.Z = r->XYZF2.Z;
	r2.XYZF.F = r->XYZF2.F;
	(this->*m_fpGIFRegHandlers[r->XYZF2.ADC ? GIF_A_D_REG_XYZF3 : GIF_A_D_REG_XYZF2])(&r2);
}

void GSState::GIFPackedRegHandlerXYZ2(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.XYZ.X = r->XYZ2.X;
	r2.XYZ.Y = r->XYZ2.Y;
	r2.XYZ.Z = r->XYZ2.Z;
	(this->*m_fpGIFRegHandlers[r->XYZ2.ADC ? GIF_A_D_REG_XYZ3 : GIF_A_D_REG_XYZ2])(&r2);
}

void GSState::GIFPackedRegHandlerTEX0_1(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_1])((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerTEX0_2(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_2])((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerCLAMP_1(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_1])((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerCLAMP_2(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_2])((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerFOG(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	GIFReg r2;
	r2.FOG.F = r->FOG.F;
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_FOG])(&r2);
}

void GSState::GIFPackedRegHandlerXYZF3(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3])((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerXYZ3(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3])((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerA_D(GIFPackedReg* r)
{
	LOG((_T("Packed ")));
	(this->*m_fpGIFRegHandlers[(BYTE)r->A_D.ADDR])(&r->r);
}

void GSState::GIFPackedRegHandlerNOP(GIFPackedReg* r)
{
	LOG((_T("Packed NOP(%016I64x%016I64x)\n"), r->ai64[0], r->ai64[1]));
}

// GIFRegHandler*

void GSState::GIFRegHandlerNull(GIFReg* r)
{
	LOG((_T("*** WARNING *** GIFRegHandlerNull(%016I64x)\n"), r->i64));
}

void GSState::GIFRegHandlerPRIM(GIFReg* r)
{
	LOG((_T("PRIM(PRIM=%x IIP=%x TME=%x FGE=%x ABE=%x AA1=%x FST=%x CTXT=%x FIX=%x)\n"), 
		r->PRIM.PRIM,
		r->PRIM.IIP,
		r->PRIM.TME,
		r->PRIM.FGE,
		r->PRIM.ABE,
		r->PRIM.AA1,
		r->PRIM.FST,
		r->PRIM.CTXT,
		r->PRIM.FIX));

	if(m_de.PRIM.i64 != r->PRIM.i64
#ifdef ENABLE_STRIPFAN
	|| m_vl.GetCount()
#endif
		)
	{
		FlushPrim();
	}

	m_de.PRIM.PRIM = r->PRIM.PRIM;

	if(m_de.PRMODECONT.AC)
	{
		m_de.PRIM.IIP = r->PRIM.IIP;
		m_de.PRIM.TME = r->PRIM.TME;
		m_de.PRIM.FGE = r->PRIM.FGE;
		m_de.PRIM.ABE = r->PRIM.ABE;
		m_de.PRIM.AA1 = r->PRIM.AA1;
		m_de.PRIM.FST = r->PRIM.FST;
		m_de.PRIM.CTXT = r->PRIM.CTXT;
		m_de.PRIM.FIX = r->PRIM.FIX;
	}

	NewPrim();
}

void GSState::GIFRegHandlerRGBAQ(GIFReg* r)
{
	LOG((_T("RGBAQ(R=%x G=%x B=%x A=%x Q=%.4f)\n"),
		r->RGBAQ.R,
		r->RGBAQ.G,
		r->RGBAQ.B,
		r->RGBAQ.A,
		r->RGBAQ.Q));

	m_v.RGBAQ = r->RGBAQ;
}

void GSState::GIFRegHandlerST(GIFReg* r)
{
	LOG((_T("ST(S=%.4f T=%.4f)\n"), 
		r->ST.S,
		r->ST.T));

	m_v.ST = r->ST;
}

void GSState::GIFRegHandlerUV(GIFReg* r)
{
	LOG((_T("UV(U=%.4f V=%.4f)\n"), 
		(float)r->UV.U/16,
		(float)r->UV.V/16));

	m_v.UV = r->UV;
}

void GSState::GIFRegHandlerXYZF2(GIFReg* r)
{
	LOG((_T("XYZF2(X=%.2f Y=%.2f Z=%d F=%d)\n"), 
		(float)r->XYZF.X/16,
		(float)r->XYZF.Y/16,
		r->XYZF.Z,
		r->XYZF.F));

	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;

	VertexKick(false);
}

void GSState::GIFRegHandlerXYZ2(GIFReg* r)
{
	LOG((_T("XYZ2(X=%.2f Y=%.2f Z=%d)\n"), 
		(float)r->XYZ.X/16,
		(float)r->XYZ.Y/16,
		r->XYZ.Z));

	m_v.XYZ = r->XYZ;

	VertexKick(false);
}

void GSState::GIFRegHandlerTEX0_1(GIFReg* r)
{
	LOG((_T("TEX0_1(TBP0=%I64x TBW=%I64d PSM=%I64x TW=%I64d TH=%I64d TCC=%I64x TFX=%I64x CBP=%I64x CPSM=%I64x CSM=%I64x CSA=%I64x CLD=%I64x)\n"),
		r->TEX0.TBP0,
		r->TEX0.TBW*64,
		r->TEX0.PSM,
		1i64<<r->TEX0.TW,
		1i64<<r->TEX0.TH,
		r->TEX0.TCC,
		r->TEX0.TFX,
		r->TEX0.CBP,
		r->TEX0.CPSM,
		r->TEX0.CSM,
		r->TEX0.CSA,
		r->TEX0.CLD));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].TEX0.i64 != r->TEX0.i64)
		FlushPrim();

	m_de.CTXT[0].TEX0 = r->TEX0;

	ASSERT(r->TEX0.TW <= 10 && r->TEX0.TH <= 10);
	if(r->TEX0.TW > 10) r->TEX0.TW = 10;
	if(r->TEX0.TH > 10) r->TEX0.TH = 10;

	m_de.CTXT[0].rt = m_lm.GetReadTexel(r->TEX0.PSM);
}

void GSState::GIFRegHandlerTEX0_2(GIFReg* r)
{
	LOG((_T("TEX0_2(TBP0=%I64x TBW=%I64d PSM=%I64x TW=%I64d TH=%I64d TCC=%I64x TFX=%I64x CBP=%I64x CPSM=%I64x CSM=%I64x CSA=%I64x CLD=%I64x)\n"),
		r->TEX0.TBP0,
		r->TEX0.TBW*64,
		r->TEX0.PSM,
		1i64<<r->TEX0.TW,
		1i64<<r->TEX0.TH,
		r->TEX0.TCC,
		r->TEX0.TFX,
		r->TEX0.CBP,
		r->TEX0.CPSM,
		r->TEX0.CSM,
		r->TEX0.CSA,
		r->TEX0.CLD));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].TEX0.i64 != r->TEX0.i64)
		FlushPrim();

	m_de.CTXT[1].TEX0 = r->TEX0;

	ASSERT(r->TEX0.TW <= 10 && r->TEX0.TH <= 10);
	if(r->TEX0.TW > 10) r->TEX0.TW = 10;
	if(r->TEX0.TH > 10) r->TEX0.TH = 10;

	m_de.CTXT[1].rt = m_lm.GetReadTexel(r->TEX0.PSM);
}

void GSState::GIFRegHandlerCLAMP_1(GIFReg* r)
{
	LOG((_T("CLAMP_1(WMS=%x WMT=%x MINU=%x MAXU=%x MINV=%x MAXV=%x)\n"),
		r->CLAMP.WMS,
		r->CLAMP.WMT,
		r->CLAMP.MINU,
		r->CLAMP.MAXU,
		r->CLAMP.MINV,
		r->CLAMP.MAXV));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].CLAMP.i64 != r->CLAMP.i64)
		FlushPrim();

	m_de.CTXT[0].CLAMP = r->CLAMP;
}

void GSState::GIFRegHandlerCLAMP_2(GIFReg* r)
{
	LOG((_T("CLAMP_2(WMS=%x WMT=%x MINU=%x MAXU=%x MINV=%x MAXV=%x)\n"),
		r->CLAMP.WMS,
		r->CLAMP.WMT,
		r->CLAMP.MINU,
		r->CLAMP.MAXU,
		r->CLAMP.MINV,
		r->CLAMP.MAXV));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].CLAMP.i64 != r->CLAMP.i64)
		FlushPrim();

	m_de.CTXT[1].CLAMP = r->CLAMP;
}

void GSState::GIFRegHandlerFOG(GIFReg* r)
{
	LOG((_T("FOG(F=%x)\n"),
		r->FOG.F));

	m_v.FOG = r->FOG;
}

void GSState::GIFRegHandlerXYZF3(GIFReg* r)
{
	LOG((_T("XYZF3(X=%.2f Y=%.2f Z=%d F=%d)\n"), 
		(float)r->XYZF.X/16,
		(float)r->XYZF.Y/16,
		r->XYZF.Z,
		r->XYZF.F));

	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;

	VertexKick(true);
}

void GSState::GIFRegHandlerXYZ3(GIFReg* r)
{
	LOG((_T("XYZ3(X=%.2f Y=%.2f Z=%d)\n"), 
		(float)r->XYZ.X/16,
		(float)r->XYZ.Y/16,
		r->XYZ.Z));

	m_v.XYZ = r->XYZ;

	VertexKick(true);
}

void GSState::GIFRegHandlerNOP(GIFReg* r)
{
	LOG((_T("NOP()\n")));
}

void GSState::GIFRegHandlerTEX1_1(GIFReg* r)
{
	LOG((_T("TEX1_1(LCM=%x MXL=%x MMAG=%x MMIN=%x MTBA=%x L=%x K=%x)\n"),
		r->TEX1.LCM,
		r->TEX1.MXL,
		r->TEX1.MMAG,
		r->TEX1.MMIN,
		r->TEX1.MTBA,
		r->TEX1.L,
		r->TEX1.K));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].TEX1.i64 != r->TEX1.i64)
		FlushPrim();

	m_de.CTXT[0].TEX1 = r->TEX1;
}

void GSState::GIFRegHandlerTEX1_2(GIFReg* r)
{
	LOG((_T("TEX1_2(LCM=%x MXL=%x MMAG=%x MMIN=%x MTBA=%x L=%x K=%x)\n"),
		r->TEX1.LCM,
		r->TEX1.MXL,
		r->TEX1.MMAG,
		r->TEX1.MMIN,
		r->TEX1.MTBA,
		r->TEX1.L,
		r->TEX1.K));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].TEX1.i64 != r->TEX1.i64)
		FlushPrim();

	m_de.CTXT[1].TEX1 = r->TEX1;
}

void GSState::GIFRegHandlerTEX2_1(GIFReg* r)
{
	LOG((_T("TEX2_1(PSM=%x CBP=%x CPSM=%x CSM=%x CSA=%x CLD=%x)\n"),
		r->TEX2.PSM,
		r->TEX2.CBP,
		r->TEX2.CPSM,
		r->TEX2.CSM,
		r->TEX2.CSA,
		r->TEX2.CLD));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].TEX2.i64 != r->TEX2.i64)
		FlushPrim();

	m_de.CTXT[0].TEX2 = r->TEX2;
}

void GSState::GIFRegHandlerTEX2_2(GIFReg* r)
{
	LOG((_T("TEX2_2(PSM=%x CBP=%x CPSM=%x CSM=%x CSA=%x CLD=%x)\n"),
		r->TEX2.PSM,
		r->TEX2.CBP,
		r->TEX2.CPSM,
		r->TEX2.CSM,
		r->TEX2.CSA,
		r->TEX2.CLD));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].TEX2.i64 != r->TEX2.i64)
		FlushPrim();

	m_de.CTXT[1].TEX2 = r->TEX2;
}

void GSState::GIFRegHandlerXYOFFSET_1(GIFReg* r)
{
	LOG((_T("XYOFFSET_1(OFX=%.2f OFY=%.2f)\n"), 
		(float)r->XYOFFSET.OFX/16,
		(float)r->XYOFFSET.OFY/16));

	m_de.CTXT[0].XYOFFSET = r->XYOFFSET;
}

void GSState::GIFRegHandlerXYOFFSET_2(GIFReg* r)
{
	LOG((_T("XYOFFSET_2(OFX=%.2f OFY=%.2f)\n"), 
		(float)r->XYOFFSET.OFX/16,
		(float)r->XYOFFSET.OFY/16));

	m_de.CTXT[1].XYOFFSET = r->XYOFFSET;
}

void GSState::GIFRegHandlerPRMODECONT(GIFReg* r)
{
	LOG((_T("PRMODECONT(AC=%x)\n"),
		r->PRMODECONT.AC));

	if(m_de.PRMODECONT.i64 != r->PRMODECONT.i64)
		FlushPrim();

	m_de.PRMODECONT = r->PRMODECONT;
}

void GSState::GIFRegHandlerPRMODE(GIFReg* r)
{
	LOG((_T("PRMODE(IIP=%x TME=%x FGE=%x ABE=%x AA1=%x FST=%x CTXT=%x FIX=%x)\n"),
		r->PRMODE.IIP,
		r->PRMODE.TME,
		r->PRMODE.FGE,
		r->PRMODE.ABE,
		r->PRMODE.AA1,
		r->PRMODE.FST,
		r->PRMODE.CTXT,
		r->PRMODE.FIX));

	if(!m_de.PRMODECONT.AC)
	{
		FlushPrim();

		m_de.PRIM.IIP = r->PRMODE.IIP;
		m_de.PRIM.TME = r->PRMODE.TME;
		m_de.PRIM.FGE = r->PRMODE.FGE;
		m_de.PRIM.ABE = r->PRMODE.ABE;
		m_de.PRIM.AA1 = r->PRMODE.AA1;
		m_de.PRIM.FST = r->PRMODE.FST;
		m_de.PRIM.CTXT = r->PRMODE.CTXT;
		m_de.PRIM.FIX = r->PRMODE.FIX;
	}
}

void GSState::GIFRegHandlerTEXCLUT(GIFReg* r)
{
	LOG((_T("TEXCLUT(CBW=%x COU=%x COV=%x)\n"),
		r->TEXCLUT.CBW,
		r->TEXCLUT.COU,
		r->TEXCLUT.COV));

	if(m_de.TEXCLUT.i64 != r->TEXCLUT.i64)
		FlushPrim();

	m_de.TEXCLUT = r->TEXCLUT;
}

void GSState::GIFRegHandlerSCANMSK(GIFReg* r)
{
	LOG((_T("SCANMSK(MSK=%x)\n"),
		r->SCANMSK.MSK));

	if(m_de.SCANMSK.i64 != r->SCANMSK.i64)
		FlushPrim();

	m_de.SCANMSK = r->SCANMSK;
}

void GSState::GIFRegHandlerMIPTBP1_1(GIFReg* r)
{
	LOG((_T("MIPTBP1_1(TBP1=%x TBW1=%x TBP2=%x TBW2=%x TBP3=%x TBW3=%x)\n"),
		r->MIPTBP1.TBP1,
		r->MIPTBP1.TBW1,
		r->MIPTBP1.TBP2,
		r->MIPTBP1.TBW2,
		r->MIPTBP1.TBP3,
		r->MIPTBP1.TBW3));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].MIPTBP1.i64 != r->MIPTBP1.i64)
		FlushPrim();

	m_de.CTXT[0].MIPTBP1 = r->MIPTBP1;
}

void GSState::GIFRegHandlerMIPTBP1_2(GIFReg* r)
{
	LOG((_T("MIPTBP1_2(TBP1=%x TBW1=%x TBP2=%x TBW2=%x TBP3=%x TBW3=%x)\n"),
		r->MIPTBP1.TBP1,
		r->MIPTBP1.TBW1,
		r->MIPTBP1.TBP2,
		r->MIPTBP1.TBW2,
		r->MIPTBP1.TBP3,
		r->MIPTBP1.TBW3));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].MIPTBP1.i64 != r->MIPTBP1.i64)
		FlushPrim();

	m_de.CTXT[1].MIPTBP1 = r->MIPTBP1;
}

void GSState::GIFRegHandlerMIPTBP2_1(GIFReg* r)
{
	LOG((_T("MIPTBP2_1(TBP4=%x TBW4=%x TBP5=%x TBW5=%x TBP6=%x TBW6=%x)\n"),
		r->MIPTBP2.TBP4,
		r->MIPTBP2.TBW4,
		r->MIPTBP2.TBP5,
		r->MIPTBP2.TBW5,
		r->MIPTBP2.TBP6,
		r->MIPTBP2.TBW6));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].MIPTBP2.i64 != r->MIPTBP2.i64)
		FlushPrim();

	m_de.CTXT[0].MIPTBP2 = r->MIPTBP2;
}

void GSState::GIFRegHandlerMIPTBP2_2(GIFReg* r)
{
	LOG((_T("MIPTBP2_2(TBP4=%x TBW4=%x TBP5=%x TBW5=%x TBP6=%x TBW6=%x)\n"),
		r->MIPTBP2.TBP4,
		r->MIPTBP2.TBW4,
		r->MIPTBP2.TBP5,
		r->MIPTBP2.TBW5,
		r->MIPTBP2.TBP6,
		r->MIPTBP2.TBW6));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].MIPTBP2.i64 != r->MIPTBP2.i64)
		FlushPrim();

	m_de.CTXT[1].MIPTBP2 = r->MIPTBP2;
}

void GSState::GIFRegHandlerTEXA(GIFReg* r)
{
	LOG((_T("TEXA(TA0=%x AEM=%x TA1=%x)\n"),
		r->TEXA.TA0,
		r->TEXA.AEM,
		r->TEXA.TA1));

	if(m_de.TEXA.i64 != r->TEXA.i64)
		FlushPrim();

	m_de.TEXA = r->TEXA;
}

void GSState::GIFRegHandlerFOGCOL(GIFReg* r)
{
	LOG((_T("FOGCOL(FCR=%x FCG=%x FCB=%x)\n"),
		r->FOGCOL.FCR,
		r->FOGCOL.FCG,
		r->FOGCOL.FCB));

	if(m_de.FOGCOL.i64 != r->FOGCOL.i64)
		FlushPrim();

	m_de.FOGCOL = r->FOGCOL;
}

void GSState::GIFRegHandlerTEXFLUSH(GIFReg* r)
{
	LOG((_T("TEXFLUSH()\n")));

	// what should we do here?
}

void GSState::GIFRegHandlerSCISSOR_1(GIFReg* r)
{
	LOG((_T("SCISSOR_1(SCAX0=%d SCAX1=%d SCAY0=%d SCAY1=%d)\n"),
		r->SCISSOR.SCAX0,
		r->SCISSOR.SCAX1,
		r->SCISSOR.SCAY0,
		r->SCISSOR.SCAY1));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].SCISSOR.i64 != r->SCISSOR.i64)
		FlushPrim();

	m_de.CTXT[0].SCISSOR = r->SCISSOR;
}

void GSState::GIFRegHandlerSCISSOR_2(GIFReg* r)
{
	LOG((_T("SCISSOR_2(SCAX0=%d SCAX1=%d SCAY0=%d SCAY1=%d)\n"),
		r->SCISSOR.SCAX0,
		r->SCISSOR.SCAX1,
		r->SCISSOR.SCAY0,
		r->SCISSOR.SCAY1));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].SCISSOR.i64 != r->SCISSOR.i64)
		FlushPrim();

	m_de.CTXT[1].SCISSOR = r->SCISSOR;
}

void GSState::GIFRegHandlerALPHA_1(GIFReg* r)
{
	LOG((_T("ALPHA_1(A=%x B=%x C=%x D=%x FIX=%x)\n"),
		r->ALPHA.A,
		r->ALPHA.B,
		r->ALPHA.C,
		r->ALPHA.D,
		r->ALPHA.FIX));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].ALPHA.i64 != r->ALPHA.i64)
		FlushPrim();

	m_de.CTXT[0].ALPHA = r->ALPHA;
}

void GSState::GIFRegHandlerALPHA_2(GIFReg* r)
{
	LOG((_T("ALPHA_2(A=%x B=%x C=%x D=%x FIX=%x)\n"),
		r->ALPHA.A,
		r->ALPHA.B,
		r->ALPHA.C,
		r->ALPHA.D,
		r->ALPHA.FIX));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].ALPHA.i64 != r->ALPHA.i64)
		FlushPrim();

	m_de.CTXT[1].ALPHA = r->ALPHA;
}

void GSState::GIFRegHandlerDIMX(GIFReg* r)
{
	LOG((_T("DIMX([%d,%d,%d,%d][%d,%d,%d,%d][%d,%d,%d,%d][%d,%d,%d,%d])\n"),
		r->DIMX.DM00,
		r->DIMX.DM01,
		r->DIMX.DM02,
		r->DIMX.DM03,
		r->DIMX.DM10,
		r->DIMX.DM11,
		r->DIMX.DM12,
		r->DIMX.DM13,
		r->DIMX.DM20,
		r->DIMX.DM21,
		r->DIMX.DM22,
		r->DIMX.DM23,
		r->DIMX.DM30,
		r->DIMX.DM31,
		r->DIMX.DM32,
		r->DIMX.DM33));

	if(m_de.DIMX.i64 != r->DIMX.i64)
		FlushPrim();

	m_de.DIMX = r->DIMX;
}

void GSState::GIFRegHandlerDTHE(GIFReg* r)
{
	LOG((_T("DTHE(DTHE=%x)\n"),
		r->DTHE.DTHE));

	if(m_de.DTHE.i64 != r->DTHE.i64)
		FlushPrim();

	m_de.DTHE = r->DTHE;
}

void GSState::GIFRegHandlerCOLCLAMP(GIFReg* r)
{
	LOG((_T("COLCLAMP(CLAMP=%x)\n"),
		r->COLCLAMP.CLAMP));

	if(m_de.COLCLAMP.i64 != r->COLCLAMP.i64)
		FlushPrim();

	m_de.COLCLAMP = r->COLCLAMP;
}

void GSState::GIFRegHandlerTEST_1(GIFReg* r)
{
	LOG((_T("TEST_1(ATE=%x ATST=%x AREF=%x AFAIL=%x DATE=%x DATM=%x ZTE=%x ZTST=%x)\n"),
		r->TEST.ATE,
		r->TEST.ATST,
		r->TEST.AREF,
		r->TEST.AFAIL,
		r->TEST.DATE,
		r->TEST.DATM,
		r->TEST.ZTE,
		r->TEST.ZTST));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].TEST.i64 != r->TEST.i64)
		FlushPrim();

	m_de.CTXT[0].TEST = r->TEST;
}

void GSState::GIFRegHandlerTEST_2(GIFReg* r)
{
	LOG((_T("TEST_2(ATE=%x ATST=%x AREF=%x AFAIL=%x DATE=%x DATM=%x ZTE=%x ZTST=%x)\n"),
		r->TEST.ATE,
		r->TEST.ATST,
		r->TEST.AREF,
		r->TEST.AFAIL,
		r->TEST.DATE,
		r->TEST.DATM,
		r->TEST.ZTE,
		r->TEST.ZTST));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].TEST.i64 != r->TEST.i64)
		FlushPrim();

	m_de.CTXT[1].TEST = r->TEST;
}

void GSState::GIFRegHandlerPABE(GIFReg* r)
{
	LOG((_T("PABE(PABE=%x)\n"), 
		r->PABE.PABE));

	if(m_de.PABE.i64 != r->PABE.i64)
		FlushPrim();

	m_de.PABE = r->PABE;
}

void GSState::GIFRegHandlerFBA_1(GIFReg* r)
{
	LOG((_T("FBA_1(FBA=%x)\n"), 
		r->FBA.FBA));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].FBA.i64 != r->FBA.i64)
		FlushPrim();

	m_de.CTXT[0].FBA = r->FBA;
}

void GSState::GIFRegHandlerFBA_2(GIFReg* r)
{
	LOG((_T("FBA_2(FBA=%x)\n"), 
		r->FBA.FBA));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].FBA.i64 != r->FBA.i64)
		FlushPrim();

	m_de.CTXT[1].FBA = r->FBA;
}

void GSState::GIFRegHandlerFRAME_1(GIFReg* r)
{
	LOG((_T("FRAME_1(FBP=%x FBW=%d PSM=%x FBMSK=%x)\n"),
		r->FRAME.Block(),
		r->FRAME.FBW*64,
		r->FRAME.PSM,
		r->FRAME.FBMSK));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].FRAME.i64 != r->FRAME.i64)
		FlushPrim();

	m_de.CTXT[0].FRAME = r->FRAME;

	m_de.CTXT[0].rp = m_lm.GetReadTexel(r->FRAME.PSM);
	m_de.CTXT[0].wf = m_lm.GetWriteFrame(r->FRAME.PSM);
}

void GSState::GIFRegHandlerFRAME_2(GIFReg* r)
{
	LOG((_T("FRAME_2(FBP=%x FBW=%d PSM=%x FBMSK=%x)\n"),
		r->FRAME.Block(),
		r->FRAME.FBW*64,
		r->FRAME.PSM,
		r->FRAME.FBMSK));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].FRAME.i64 != r->FRAME.i64)
		FlushPrim();

	m_de.CTXT[1].FRAME = r->FRAME;

	m_de.CTXT[1].rp = m_lm.GetReadTexel(r->FRAME.PSM);
	m_de.CTXT[1].wf = m_lm.GetWriteFrame(r->FRAME.PSM);
}

void GSState::GIFRegHandlerZBUF_1(GIFReg* r)
{
	LOG((_T("ZBUF_1(ZBP=%x PSM=%x ZMSK=%x)\n"),
		r->ZBUF.ZBP,
		r->ZBUF.PSM,
		r->ZBUF.ZMSK));

	if(m_de.PRIM.CTXT == 0 && m_de.CTXT[0].ZBUF.i64 != r->ZBUF.i64)
		FlushPrim();

	m_de.CTXT[0].ZBUF = r->ZBUF;

	m_de.CTXT[0].rz = m_lm.GetReadPixel(r->ZBUF.PSM);
	m_de.CTXT[0].wz = m_lm.GetWritePixel(r->ZBUF.PSM);
}

void GSState::GIFRegHandlerZBUF_2(GIFReg* r)
{
	LOG((_T("ZBUF_2(ZBP=%x PSM=%x ZMSK=%x)\n"),
		r->ZBUF.ZBP,
		r->ZBUF.PSM,
		r->ZBUF.ZMSK));

	if(m_de.PRIM.CTXT == 1 && m_de.CTXT[1].ZBUF.i64 != r->ZBUF.i64)
		FlushPrim();

	m_de.CTXT[1].ZBUF = r->ZBUF;

	m_de.CTXT[1].rz = m_lm.GetReadPixel(r->ZBUF.PSM);
	m_de.CTXT[1].wz = m_lm.GetWritePixel(r->ZBUF.PSM);
}

void GSState::GIFRegHandlerBITBLTBUF(GIFReg* r)
{
	LOG((_T("BITBLTBUF(SBP=%x SBW=%d SPSM=%x DBP=%x DBW=%d DPSM=%x)\n"),
		r->BITBLTBUF.SBP,
		r->BITBLTBUF.SBW*64,
		r->BITBLTBUF.SPSM,
		r->BITBLTBUF.DBP,
		r->BITBLTBUF.DBW*64,
		r->BITBLTBUF.DPSM));

	m_rs.BITBLTBUF = r->BITBLTBUF;
}

void GSState::GIFRegHandlerTRXPOS(GIFReg* r)
{
	LOG((_T("TRXPOS(SSAX=%d SSAY=%d DSAX=%d DSAY=%d DIR=%d)\n"),
		r->TRXPOS.SSAX,
		r->TRXPOS.SSAY,
		r->TRXPOS.DSAX,
		r->TRXPOS.DSAY,
		r->TRXPOS.DIR));

	m_rs.TRXPOS = r->TRXPOS;
}

void GSState::GIFRegHandlerTRXREG(GIFReg* r)
{
	LOG((_T("TRXREG(RRW=%d RRH=%d)\n"),
		r->TRXREG.RRW,
		r->TRXREG.RRH));

	m_rs.TRXREG = r->TRXREG;
}

void GSState::GIFRegHandlerTRXDIR(GIFReg* r)
{
	LOG((_T("TRXDIR(XDIR=%d)\n"), 
		r->TRXDIR.XDIR));

	m_rs.TRXDIR = r->TRXDIR;

	FlushPrim();

	switch(m_rs.TRXDIR.XDIR)
	{
	case 0: // host -> local
		m_x = m_rs.TRXPOS.DSAX;
		m_y = m_rs.TRXPOS.DSAY;
		m_rs.TRXREG.RRW += m_x;
		m_rs.TRXREG.RRH += m_y;
		break;
	case 1: // local -> host
		m_x = m_rs.TRXPOS.SSAX;
		m_y = m_rs.TRXPOS.SSAY;
		m_rs.TRXREG.RRW += m_x;
		m_rs.TRXREG.RRH += m_y;
		break;
	case 2: // local -> local
		break;
	case 3: 
		ASSERT(0);
		break;
	}
}

void GSState::GIFRegHandlerHWREG(GIFReg* r)
{
	LOG((_T("HWREG(DATA_LOWER=%08x DATA_UPPER=%08x)\n"),
		r->HWREG.DATA_LOWER,
		r->HWREG.DATA_UPPER));

	// TODO

	ASSERT(0);
}

void GSState::GIFRegHandlerSIGNAL(GIFReg* r)
{
	LOG((_T("SIGNAL(ID=%08x IDMSK=%08x)\n"), 
		r->SIGNAL.ID, 
		r->SIGNAL.IDMSK));

	m_rs.SIGLBLID.SIGID = (m_rs.SIGLBLID.SIGID&~r->SIGNAL.IDMSK)|(r->SIGNAL.ID&r->SIGNAL.IDMSK);

	if(m_rs.CSRw.SIGNAL) m_rs.CSRr.SIGNAL = 1;
	if(!m_rs.IMR.SIGMSK && m_fpGSirq) m_fpGSirq();
}

void GSState::GIFRegHandlerFINISH(GIFReg* r)
{
	LOG((_T("FINISH()\n")));

	if(m_rs.CSRw.FINISH) m_rs.CSRr.FINISH = 1;
	if(!m_rs.IMR.FINISHMSK && m_fpGSirq) m_fpGSirq();
}

void GSState::GIFRegHandlerLABEL(GIFReg* r)
{
	LOG((_T("LABEL(ID=%08x IDMSK=%08x)\n"), 
		r->LABEL.ID, 
		r->LABEL.IDMSK));

	m_rs.SIGLBLID.LBLID = (m_rs.SIGLBLID.LBLID&~r->LABEL.IDMSK)|(r->LABEL.ID&r->LABEL.IDMSK);
}

