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

#pragma once

#include "GS.h"
#include "GSDrawingContext.h"
#include "GSDrawingEnvironment.h"
#include "GSRegSet.h"
#include "GSVertex.h"
#include "GSTextureCache.h"
#include "GSVertexSW.h"
#include "GSVertexList.h"
#include "GSPerfMon.h"
#include "GSDevice.h"

class GSState : public CWnd
{
	DECLARE_MESSAGE_MAP()

	friend class GSTexture;
	friend class GSTextureCache;
	friend class GSTextureCache;

public:
	GSState();
	virtual ~GSState();

	virtual bool Create(LPCTSTR title);

	void Show();
	void Hide();

	bool OnMsg(const MSG& msg);
	void OnClose();

	virtual void Reset();

	UINT32 Freeze(freezeData* fd, bool fSizeOnly);
	UINT32 Defrost(const freezeData* fd);
	void WriteCSR(UINT32 csr);
	void ReadFIFO(BYTE* mem, UINT32 size);
	void Transfer(BYTE* mem, UINT32 size, int index);
	void VSync(int field);
	UINT32 MakeSnapshot(char* path);
	void SetIrq(void (*irq)()) {m_irq = irq;}
	void SetMT(bool mt) {m_mt = mt;}
	void SetGameCRC(int crc, int options);
	void GetLastTag(UINT32* tag) {*tag = m_path3hack; m_path3hack = 0;}
	void SetFrameSkip(int frameskip);

protected:
	void (*m_irq)();
	bool m_mt;
	int m_options;
	bool m_nloophack;
	bool m_vsync;
	bool m_osd;
	int m_field;
	int m_path3hack;
	int m_crc;
	int m_frameskip;

private:
	static const int m_nTrMaxBytes = 1024*1024*4;
	int m_nTransferBytes;
	BYTE* m_pTransferBuffer;
	void WriteTransfer(BYTE* mem, int len);
	void FlushWriteTransfer();
	void ReadTransfer(BYTE* mem, int len);
	void MoveTransfer();

	int m_x, m_y;

	__forceinline void WriteStep()
	{
	//	if(m_y == m_env.TRXREG.RRH && m_x == m_env.TRXPOS.DSAX) ASSERT(0);

		if(++m_x == m_env.TRXREG.RRW)
		{
			m_x = m_env.TRXPOS.DSAX;
			m_y++;
		}
	}

	__forceinline void ReadStep()
	{
	//	if(m_y == m_env.TRXREG.RRH && m_x == m_env.TRXPOS.SSAX) ASSERT(0);

		if(++m_x == m_env.TRXREG.RRW)
		{
			m_x = m_env.TRXPOS.SSAX;
			m_y++;
		}
	}

protected:
	static const int m_version = 4;

	GIFPath m_path[3];
	GSLocalMemory m_mem;
	GSDrawingEnvironment m_env;
	GSDrawingContext* m_context;
	GSRegSet m_regs;
	GSVertex m_v;
	float m_q;
	GSPerfMon m_perfmon;
	// FIXME: savestate
	GIFRegPRIM* m_pPRIM;

protected:
	GSDevice m_dev;

	int m_interlace;
	int m_aspectratio;
	int m_filter;

	virtual void VertexKick(bool skip) = 0;
	virtual void DrawingKick(bool skip) = 0;
	virtual void NewPrim() = 0;
	virtual void FlushPrim() = 0;
	virtual void Flip() = 0;
	virtual void InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r) {}
	virtual void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r) {}
	virtual void MinMaxUV(int w, int h, CRect& r) {r.SetRect(0, 0, w, h);}

	struct FlipInfo 
	{
		GSTexture2D t; 
		GSScale s;
	};

	virtual void ResetDevice() {}

	void FinishFlip(FlipInfo src[2]);
	void Merge(FlipInfo src[2], GSTexture2D& dst);
	void Present();
	void Flush();

private:
	void ResetHandlers();

	typedef void (GSState::*GIFPackedRegHandler)(GIFPackedReg* r);
	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16];

	void GIFPackedRegHandlerNull(GIFPackedReg* r);
	void GIFPackedRegHandlerPRIM(GIFPackedReg* r);
	void GIFPackedRegHandlerRGBA(GIFPackedReg* r);
	void GIFPackedRegHandlerSTQ(GIFPackedReg* r);
	void GIFPackedRegHandlerUV(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF2(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ2(GIFPackedReg* r);
	template<int i> void GIFPackedRegHandlerTEX0(GIFPackedReg* r);
	template<int i> void GIFPackedRegHandlerCLAMP(GIFPackedReg* r);
	void GIFPackedRegHandlerFOG(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF3(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ3(GIFPackedReg* r);
	void GIFPackedRegHandlerA_D(GIFPackedReg* r);
	void GIFPackedRegHandlerNOP(GIFPackedReg* r);

	typedef void (GSState::*GIFRegHandler)(GIFReg* r);
	GIFRegHandler m_fpGIFRegHandlers[256];

	void GIFRegHandlerNull(GIFReg* r);
	void GIFRegHandlerPRIM(GIFReg* r);
	void GIFRegHandlerRGBAQ(GIFReg* r);
	void GIFRegHandlerST(GIFReg* r);
	void GIFRegHandlerUV(GIFReg* r);
	void GIFRegHandlerXYZF2(GIFReg* r);
	void GIFRegHandlerXYZ2(GIFReg* r);
	template<int i> void GIFRegHandlerTEX0(GIFReg* r);
	template<int i> void GIFRegHandlerCLAMP(GIFReg* r);
	void GIFRegHandlerFOG(GIFReg* r);
	void GIFRegHandlerXYZF3(GIFReg* r);
	void GIFRegHandlerXYZ3(GIFReg* r);
	void GIFRegHandlerNOP(GIFReg* r);
	template<int i> void GIFRegHandlerTEX1(GIFReg* r);
	template<int i> void GIFRegHandlerTEX2(GIFReg* r);
	template<int i> void GIFRegHandlerXYOFFSET(GIFReg* r);
	void GIFRegHandlerPRMODECONT(GIFReg* r);
	void GIFRegHandlerPRMODE(GIFReg* r);
	void GIFRegHandlerTEXCLUT(GIFReg* r);
	void GIFRegHandlerSCANMSK(GIFReg* r);
	template<int i> void GIFRegHandlerMIPTBP1(GIFReg* r);
	template<int i> void GIFRegHandlerMIPTBP2(GIFReg* r);
	void GIFRegHandlerTEXA(GIFReg* r);
	void GIFRegHandlerFOGCOL(GIFReg* r);
	void GIFRegHandlerTEXFLUSH(GIFReg* r);
	template<int i> void GIFRegHandlerSCISSOR(GIFReg* r);
	template<int i> void GIFRegHandlerALPHA(GIFReg* r);
	void GIFRegHandlerDIMX(GIFReg* r);
	void GIFRegHandlerDTHE(GIFReg* r);
	void GIFRegHandlerCOLCLAMP(GIFReg* r);
	template<int i> void GIFRegHandlerTEST(GIFReg* r);
	void GIFRegHandlerPABE(GIFReg* r);
	template<int i> void GIFRegHandlerFBA(GIFReg* r);
	template<int i> void GIFRegHandlerFRAME(GIFReg* r);
	template<int i> void GIFRegHandlerZBUF(GIFReg* r);
	void GIFRegHandlerBITBLTBUF(GIFReg* r);
	void GIFRegHandlerTRXPOS(GIFReg* r);
	void GIFRegHandlerTRXREG(GIFReg* r);
	void GIFRegHandlerTRXDIR(GIFReg* r);
	void GIFRegHandlerHWREG(GIFReg* r);
	void GIFRegHandlerSIGNAL(GIFReg* r);
	void GIFRegHandlerFINISH(GIFReg* r);
	void GIFRegHandlerLABEL(GIFReg* r);
};
