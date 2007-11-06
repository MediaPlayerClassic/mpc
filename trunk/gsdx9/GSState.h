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
	GSState(int w, int h);
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
	void Transfer1(BYTE* mem, UINT32 addr);
	void Transfer2(BYTE* mem, UINT32 size);
	void Transfer3(BYTE* mem, UINT32 size);
	void Transfer(BYTE* mem, UINT32 size, int index);
	void TransferMT(BYTE* mem, UINT32 size, int index);
	void VSync(int field);
	UINT32 MakeSnapshot(char* path);
	void Capture();
	void ToggleOSD();
	void SetGSirq(void (*fpGSirq)()) {m_fpGSirq = fpGSirq;}
	void SetMultiThreaded(bool mt) {m_fMultiThreaded = mt;}

private:
	void (*m_fpGSirq)();
	bool m_fMultiThreaded;
	int m_iOSD;

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
	int m_width, m_height;
	CComPtr<IDirect3D9> m_pD3D;
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CComPtr<ID3DXFont> m_pD3DXFont;
	CComPtr<IDirect3DSurface9> m_pOrgRenderTarget;
	CComPtr<IDirect3DTexture9> m_pTmpRenderTarget;
	CComPtr<IDirect3DPixelShader9> m_pPixelShaders[20];
	CComPtr<IDirect3DPixelShader9> m_pHLSLTFX[38], m_pHLSLMerge[3], m_pHLSLRedBlue;
	enum {PS_M16 = 0, PS_M24 = 1, PS_M32 = 2};
	D3DPRESENT_PARAMETERS m_d3dpp;
	DDCAPS m_ddcaps;
	D3DCAPS9 m_caps;
	D3DSURFACE_DESC m_bd;
	D3DFORMAT m_fmtDepthStencil;
	bool m_fEnablePalettizedTextures;
	bool m_fNloopHack;
	bool m_fField;
	D3DTEXTUREFILTERTYPE m_texfilter;

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

	struct FlipInfo {CComPtr<IDirect3DTexture9> pRT; D3DSURFACE_DESC rd; scale_t scale;};
	void FinishFlip(FlipInfo rt[2]);

	void Flush();

private:

	typedef void (__fastcall GSState::*GIFPackedRegHandler)(GIFPackedReg* r);
	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16];

	void __fastcall GIFPackedRegHandlerNull(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerPRIM(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerRGBA(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerSTQ(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerUV(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerXYZF2(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerXYZ2(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerTEX0_1(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerTEX0_2(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerCLAMP_1(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerCLAMP_2(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerFOG(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerXYZF3(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerXYZ3(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerA_D(GIFPackedReg* r);
	void __fastcall GIFPackedRegHandlerNOP(GIFPackedReg* r);

	typedef void (__fastcall GSState::*GIFRegHandler)(GIFReg* r);
	GIFRegHandler m_fpGIFRegHandlers[256];

	void __fastcall GIFRegHandlerNull(GIFReg* r);
	void __fastcall GIFRegHandlerPRIM(GIFReg* r);
	void __fastcall GIFRegHandlerRGBAQ(GIFReg* r);
	void __fastcall GIFRegHandlerST(GIFReg* r);
	void __fastcall GIFRegHandlerUV(GIFReg* r);
	void __fastcall GIFRegHandlerXYZF2(GIFReg* r);
	void __fastcall GIFRegHandlerXYZ2(GIFReg* r);
	void __fastcall GIFRegHandlerTEX0_1(GIFReg* r);
	void __fastcall GIFRegHandlerTEX0_2(GIFReg* r);
	void __fastcall GIFRegHandlerCLAMP_1(GIFReg* r);
	void __fastcall GIFRegHandlerCLAMP_2(GIFReg* r);
	void __fastcall GIFRegHandlerFOG(GIFReg* r);
	void __fastcall GIFRegHandlerXYZF3(GIFReg* r);
	void __fastcall GIFRegHandlerXYZ3(GIFReg* r);
	void __fastcall GIFRegHandlerNOP(GIFReg* r);
	void __fastcall GIFRegHandlerTEX1_1(GIFReg* r);
	void __fastcall GIFRegHandlerTEX1_2(GIFReg* r);
	void __fastcall GIFRegHandlerTEX2_1(GIFReg* r);
	void __fastcall GIFRegHandlerTEX2_2(GIFReg* r);
	void __fastcall GIFRegHandlerXYOFFSET_1(GIFReg* r);
	void __fastcall GIFRegHandlerXYOFFSET_2(GIFReg* r);
	void __fastcall GIFRegHandlerPRMODECONT(GIFReg* r);
	void __fastcall GIFRegHandlerPRMODE(GIFReg* r);
	void __fastcall GIFRegHandlerTEXCLUT(GIFReg* r);
	void __fastcall GIFRegHandlerSCANMSK(GIFReg* r);
	void __fastcall GIFRegHandlerMIPTBP1_1(GIFReg* r);
	void __fastcall GIFRegHandlerMIPTBP1_2(GIFReg* r);
	void __fastcall GIFRegHandlerMIPTBP2_1(GIFReg* r);
	void __fastcall GIFRegHandlerMIPTBP2_2(GIFReg* r);
	void __fastcall GIFRegHandlerTEXA(GIFReg* r);
	void __fastcall GIFRegHandlerFOGCOL(GIFReg* r);
	void __fastcall GIFRegHandlerTEXFLUSH(GIFReg* r);
	void __fastcall GIFRegHandlerSCISSOR_1(GIFReg* r);
	void __fastcall GIFRegHandlerSCISSOR_2(GIFReg* r);
	void __fastcall GIFRegHandlerALPHA_1(GIFReg* r);
	void __fastcall GIFRegHandlerALPHA_2(GIFReg* r);
	void __fastcall GIFRegHandlerDIMX(GIFReg* r);
	void __fastcall GIFRegHandlerDTHE(GIFReg* r);
	void __fastcall GIFRegHandlerCOLCLAMP(GIFReg* r);
	void __fastcall GIFRegHandlerTEST_1(GIFReg* r);
	void __fastcall GIFRegHandlerTEST_2(GIFReg* r);
	void __fastcall GIFRegHandlerPABE(GIFReg* r);
	void __fastcall GIFRegHandlerFBA_1(GIFReg* r);
	void __fastcall GIFRegHandlerFBA_2(GIFReg* r);
	void __fastcall GIFRegHandlerFRAME_1(GIFReg* r);
	void __fastcall GIFRegHandlerFRAME_2(GIFReg* r);
	void __fastcall GIFRegHandlerZBUF_1(GIFReg* r);
	void __fastcall GIFRegHandlerZBUF_2(GIFReg* r);
	void __fastcall GIFRegHandlerBITBLTBUF(GIFReg* r);
	void __fastcall GIFRegHandlerTRXPOS(GIFReg* r);
	void __fastcall GIFRegHandlerTRXREG(GIFReg* r);
	void __fastcall GIFRegHandlerTRXDIR(GIFReg* r);
	void __fastcall GIFRegHandlerHWREG(GIFReg* r);
	void __fastcall GIFRegHandlerSIGNAL(GIFReg* r);
	void __fastcall GIFRegHandlerFINISH(GIFReg* r);
	void __fastcall GIFRegHandlerLABEL(GIFReg* r);

private:

	GIFPath m_path2[3];

	class GSTransferBuffer
	{
	public:
		BYTE* m_data;
		UINT32 m_size;
		int m_index;

		GSTransferBuffer()
		{
			m_data = NULL;
			m_size = 0;
		}

		~GSTransferBuffer()
		{
			delete [] m_data;
		}
	};

	GSQueue<GSTransferBuffer*> m_queue;

	CAMEvent m_evThreadQuit;
	CAMEvent m_evThreadIdle;
	HANDLE m_hThread;
	DWORD m_ThreadId;

	HANDLE CreateThread()
	{
		m_ThreadId = 0;
		m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);

		m_evThreadIdle.Set();

		return m_hThread;
	}

	void QuitThread()
	{
		m_evThreadQuit.Set();
			
		if(WaitForSingleObject(m_hThread, 30000) == WAIT_TIMEOUT) 
		{
			ASSERT(0); 
			TerminateThread(m_hThread, (DWORD)-1);
		}
	}

	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);

	DWORD ThreadProc();

	void SyncThread()
	{
		m_queue.GetDequeueEvent().Wait();
		m_evThreadIdle.Wait();
	}
};
