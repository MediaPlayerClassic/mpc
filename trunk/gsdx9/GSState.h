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
#include "GSSoftVertex.h"
#include "GSVertexList.h"
#include "GSCapture.h"
#include "GSPerfMon.h"
#include "GSQueue.h"

class GSState : public CWnd
{
	DECLARE_MESSAGE_MAP()

	friend class GSTextureCache;

public:
	GSState();
	virtual ~GSState();

	virtual bool Create(LPCTSTR title);

	void Show();
	void Hide();

	void OnClose();

	UINT32 Freeze(freezeData* fd, bool fSizeOnly);
	UINT32 Defrost(const freezeData* fd);
	void Reset();
	void WriteCSR(UINT32 csr);
	void ReadFIFO(BYTE* mem);
	void Transfer(BYTE* mem, UINT32 size, int index);
	void VSync(int field);
	UINT32 MakeSnapshot(char* path);
	void SetIrq(void (*irq)()) {m_irq = irq;}
	void SetMT(bool mt) {m_mt = mt;}
	void SetGameCRC(int crc, int options);

private:
	void (*m_irq)();
	bool m_mt;
	int m_crc;
	int m_options;
	bool m_nloophack;
	int m_osd;
	int m_field;

private:
	static const int m_nTrMaxBytes = 1024*1024*4;
	int m_nTransferBytes;
	BYTE* m_pTransferBuffer;
	int m_x, m_y;
	void WriteStep();
	void ReadStep();
	void WriteTransfer(BYTE* mem, int len);
	void FlushWriteTransfer();
	void ReadTransfer(BYTE* mem, int len);
	void MoveTransfer();

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
	GSCapture m_capture;
	// FIXME: savestate
	GIFRegPRIM* m_pPRIM;

protected:
	CComPtr<IDirect3D9> m_pD3D;
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CComPtr<ID3DXFont> m_pD3DXFont;
	CComPtr<IDirect3DSurface9> m_pBackBuffer;
	CComPtr<IDirect3DTexture9> m_pMergeTexture;
	CComPtr<IDirect3DTexture9> m_pInterlaceTexture;
	CComPtr<IDirect3DTexture9> m_pDeinterlaceTexture;
	CComPtr<IDirect3DPixelShader9> m_pPixelShaders[20];
	CComPtr<IDirect3DPixelShader9> m_pHLSLTFX[38], m_pHLSLMerge[3], m_pHLSLInterlace[3];
	enum {PS_M16 = 0, PS_M24 = 1, PS_M32 = 2};
	D3DPRESENT_PARAMETERS m_d3dpp;
	DDCAPS m_ddcaps;
	D3DCAPS9 m_caps;
	D3DFORMAT m_fmtDepthStencil;
	bool m_fPalettizedTextures;
	bool m_fDeinterlace;
	D3DTEXTUREFILTERTYPE m_nTextureFilter;

	virtual void ResetState();
	virtual HRESULT ResetDevice(bool fForceWindowed = false);

	virtual void VertexKick(bool skip) = 0;
	virtual int DrawingKick(bool skip) = 0;
	virtual void NewPrim() = 0;
	virtual void FlushPrim() = 0;
	virtual void Flip() = 0;
	virtual void InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r) {}
	virtual void InvalidateLocalMem(DWORD TBP0, DWORD BW, DWORD PSM, CRect r) {}
	virtual void MinMaxUV(int w, int h, CRect& r) {r.SetRect(0, 0, w, h);}

	struct FlipInfo {CComPtr<IDirect3DTexture9> tex; D3DSURFACE_DESC desc; scale_t scale;};
	void FinishFlip(FlipInfo src[2], float yscale = 1.0f);

	void Merge(FlipInfo src[2], IDirect3DSurface9* dst, float yscale = 1.0f);
	void Interlace(IDirect3DTexture9* src, IDirect3DSurface9* dst, int field);

	void Flush();

private:

	typedef void (GSState::*GIFPackedRegHandler)(GIFPackedReg* r);
	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16];

	void GIFPackedRegHandlerNull(GIFPackedReg* r);
	void GIFPackedRegHandlerPRIM(GIFPackedReg* r);
	void GIFPackedRegHandlerRGBA(GIFPackedReg* r);
	void GIFPackedRegHandlerSTQ(GIFPackedReg* r);
	void GIFPackedRegHandlerUV(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF2(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ2(GIFPackedReg* r);
	void GIFPackedRegHandlerTEX0_1(GIFPackedReg* r);
	void GIFPackedRegHandlerTEX0_2(GIFPackedReg* r);
	void GIFPackedRegHandlerCLAMP_1(GIFPackedReg* r);
	void GIFPackedRegHandlerCLAMP_2(GIFPackedReg* r);
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
	void GIFRegHandlerTEX0_1(GIFReg* r);
	void GIFRegHandlerTEX0_2(GIFReg* r);
	void GIFRegHandlerCLAMP_1(GIFReg* r);
	void GIFRegHandlerCLAMP_2(GIFReg* r);
	void GIFRegHandlerFOG(GIFReg* r);
	void GIFRegHandlerXYZF3(GIFReg* r);
	void GIFRegHandlerXYZ3(GIFReg* r);
	void GIFRegHandlerNOP(GIFReg* r);
	void GIFRegHandlerTEX1_1(GIFReg* r);
	void GIFRegHandlerTEX1_2(GIFReg* r);
	void GIFRegHandlerTEX2_1(GIFReg* r);
	void GIFRegHandlerTEX2_2(GIFReg* r);
	void GIFRegHandlerXYOFFSET_1(GIFReg* r);
	void GIFRegHandlerXYOFFSET_2(GIFReg* r);
	void GIFRegHandlerPRMODECONT(GIFReg* r);
	void GIFRegHandlerPRMODE(GIFReg* r);
	void GIFRegHandlerTEXCLUT(GIFReg* r);
	void GIFRegHandlerSCANMSK(GIFReg* r);
	void GIFRegHandlerMIPTBP1_1(GIFReg* r);
	void GIFRegHandlerMIPTBP1_2(GIFReg* r);
	void GIFRegHandlerMIPTBP2_1(GIFReg* r);
	void GIFRegHandlerMIPTBP2_2(GIFReg* r);
	void GIFRegHandlerTEXA(GIFReg* r);
	void GIFRegHandlerFOGCOL(GIFReg* r);
	void GIFRegHandlerTEXFLUSH(GIFReg* r);
	void GIFRegHandlerSCISSOR_1(GIFReg* r);
	void GIFRegHandlerSCISSOR_2(GIFReg* r);
	void GIFRegHandlerALPHA_1(GIFReg* r);
	void GIFRegHandlerALPHA_2(GIFReg* r);
	void GIFRegHandlerDIMX(GIFReg* r);
	void GIFRegHandlerDTHE(GIFReg* r);
	void GIFRegHandlerCOLCLAMP(GIFReg* r);
	void GIFRegHandlerTEST_1(GIFReg* r);
	void GIFRegHandlerTEST_2(GIFReg* r);
	void GIFRegHandlerPABE(GIFReg* r);
	void GIFRegHandlerFBA_1(GIFReg* r);
	void GIFRegHandlerFBA_2(GIFReg* r);
	void GIFRegHandlerFRAME_1(GIFReg* r);
	void GIFRegHandlerFRAME_2(GIFReg* r);
	void GIFRegHandlerZBUF_1(GIFReg* r);
	void GIFRegHandlerZBUF_2(GIFReg* r);
	void GIFRegHandlerBITBLTBUF(GIFReg* r);
	void GIFRegHandlerTRXPOS(GIFReg* r);
	void GIFRegHandlerTRXREG(GIFReg* r);
	void GIFRegHandlerTRXDIR(GIFReg* r);
	void GIFRegHandlerHWREG(GIFReg* r);
	void GIFRegHandlerSIGNAL(GIFReg* r);
	void GIFRegHandlerFINISH(GIFReg* r);
	void GIFRegHandlerLABEL(GIFReg* r);
};
