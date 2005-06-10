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
#include "GSWnd.h"
#include "GSLocalMemory.h"
#include "GSTextureCache.h"
#include "GSSoftVertex.h"
#include "GSVertexList.h"
#include "GSCapture.h"
#include "GSPerfMon.h"
//
#define ENABLE_CAPTURE_STATE
////


//#define DEBUG_SAVETEXTURES
/*#define DEBUG_LOG
#define DEBUG_LOG2
#define DEBUG_LOGVERTICES
#define DEBUG_RENDERTARGETS
*/

struct GSDrawingContext
{
	struct GSDrawingContext() {memset(this, 0, sizeof(*this));}

	GIFRegXYOFFSET	XYOFFSET;
	GIFRegTEX0		TEX0;
	GIFRegTEX1		TEX1;
	GIFRegTEX2		TEX2;
	GIFRegCLAMP		CLAMP;
	GIFRegMIPTBP1	MIPTBP1;
	GIFRegMIPTBP2	MIPTBP2;
	GIFRegSCISSOR	SCISSOR;
	GIFRegALPHA		ALPHA;
	GIFRegTEST		TEST;
	GIFRegFBA		FBA;
	GIFRegFRAME		FRAME;
	GIFRegZBUF		ZBUF;

	GSLocalMemory::psmtbl_t* ftbl;
	GSLocalMemory::psmtbl_t* ztbl;
	GSLocalMemory::psmtbl_t* ttbl;
};

struct GSDrawingEnvironment
{
	struct GSDrawingEnvironment() {memset(this, 0, sizeof(*this));}

	GIFRegPRIM			PRIM;
	GIFRegPRMODE		PRMODE;
	GIFRegPRMODECONT	PRMODECONT;
	GIFRegTEXCLUT		TEXCLUT;
	GIFRegSCANMSK		SCANMSK;
	GIFRegTEXA			TEXA;
	GIFRegFOGCOL		FOGCOL;
	GIFRegDIMX			DIMX;
	GIFRegDTHE			DTHE;
	GIFRegCOLCLAMP		COLCLAMP;
	GIFRegPABE			PABE;
	GSDrawingContext	CTXT[2];
};

struct GSRegSet
{
	struct GSRegSet() {memset(this, 0, sizeof(*this));}

	CSize GetDispSize(int en)
	{
		ASSERT(en >= 0 && en < 2);
		CSize size;
		size.cx = (DISPLAY[en].DW + 1) / (DISPLAY[en].MAGH + 1);
		size.cy = (DISPLAY[en].DH + 1) / (DISPLAY[en].MAGV + 1);
		if(SMODE2.INT && SMODE2.FFMD && size.cy > 1) size.cy >>= 1;
		return size;
	}

	CRect GetDispRect(int en)
	{
		ASSERT(en >= 0 && en < 2);
		return CRect(CPoint(DISPFB[en].DBX, DISPFB[en].DBY), GetDispSize(en));
	}

	bool IsEnabled(int en)
	{
		ASSERT(en >= 0 && en < 2);
		if(en == 0 && PMODE.EN1) {return(DISPLAY[0].DW || DISPLAY[0].DH);}
		else if(en == 1 && PMODE.EN2) {return(DISPLAY[1].DW || DISPLAY[1].DH);}
		return(false);
	}

	int GetFPS()
	{
		return ((SMODE1.CMOD&1) ? 50 : 60) / (SMODE2.INT ? 1 : 2);
	}

	GSRegBGCOLOR	BGCOLOR;
	GSRegBUSDIR		BUSDIR;
	GSRegCSR		CSRw, CSRr;
	GSRegDISPFB		DISPFB[2];
	GSRegDISPLAY	DISPLAY[2];
	GSRegEXTBUF		EXTBUF;
	GSRegEXTDATA	EXTDATA;
	GSRegEXTWRITE	EXTWRITE;
	GSRegIMR		IMR;
	GSRegPMODE		PMODE;
	GSRegSIGLBLID	SIGLBLID;
	GSRegSMODE1		SMODE1;
	GSRegSMODE2		SMODE2;

	GIFRegBITBLTBUF	BITBLTBUF;
	GIFRegTRXDIR	TRXDIR;
	GIFRegTRXPOS	TRXPOS;
	GIFRegTRXREG	TRXREG, TRXREG2;
};

struct GSVertex
{
	GIFRegRGBAQ		RGBAQ;
	GIFRegST		ST;
	GIFRegUV		UV;
	GIFRegXYZ		XYZ;
	GIFRegFOG		FOG;
};

class GSState
{
	friend class GSTextureCache;

protected:
	static const int m_version = 2;

	GSLocalMemory m_lm;
	GSDrawingEnvironment m_de;
	GSDrawingContext* m_ctxt;
	GSRegSet m_rs;
	GSVertex m_v;
	float m_q;
	GSPerfMon m_perfmon;
	GSCapture m_capture;

	static const int m_nTrMaxBytes = 1024*1024*4;
	int m_nTrBytes;
	BYTE* m_pTrBuff;
	int m_x, m_y;
	void WriteStep();
	void ReadStep();
	void WriteTransfer(BYTE* pMem, int len);
	void FlushWriteTransfer();
	void ReadTransfer(BYTE* pMem, int len);
	void MoveTransfer();

	HWND m_hWnd;
	int m_width, m_height;
	CComPtr<IDirect3D9> m_pD3D;
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CComPtr<ID3DXFont> m_pD3DXFont;
	CComPtr<IDirect3DSurface9> m_pOrgRenderTarget;
	CComPtr<IDirect3DTexture9> m_pTmpRenderTarget;
	CComPtr<IDirect3DPixelShader9> m_pPixelShaders[20];
	CComPtr<IDirect3DPixelShader9> m_pHLSLTFX[38], m_pHLSLMerge[3], m_pHLSLRedBlue;
	enum {PS11_EN11 = 12, PS11_EN01 = 13, PS11_EN10 = 14, PS11_EN00 = 15};
	enum {PS14_EN11 = 16, PS14_EN01 = 17, PS14_EN10 = 18, PS14_EN00 = 19};
	enum {PS_M16 = 0, PS_M24 = 1, PS_M32 = 2};
	D3DPRESENT_PARAMETERS m_d3dpp;
	DDCAPS m_ddcaps;
	D3DCAPS9 m_caps;
	D3DSURFACE_DESC m_bd;
	D3DFORMAT m_fmtDepthStencil;
	bool m_fEnablePalettizedTextures;
	D3DTEXTUREFILTERTYPE m_texfilter;

	virtual void Reset();
	virtual void VertexKick(bool fSkip) = 0;
	virtual int DrawingKick(bool fSkip) = 0;
	virtual void NewPrim() = 0;
	virtual void FlushPrim() = 0;
	virtual void Flip() = 0;
	virtual void EndFrame() = 0;
	virtual void InvalidateTexture(DWORD TBP0, DWORD PSM, CRect r) {}
	virtual void InvalidateLocalMem(DWORD TBP0, DWORD BW, DWORD PSM, CRect r) {}
	virtual void MinMaxUV(int w, int h, CRect& r) {r.SetRect(0, 0, w, h);}

	DWORD m_nVSync;
	CAMEvent m_evVSync;
	CComPtr<IReferenceClock> m_pRefClock;

	struct FlipInfo {CComPtr<IDirect3DTexture9> pRT; D3DSURFACE_DESC rd; scale_t scale;};
	void FinishFlip(FlipInfo rt[2]);

	void FlushPrimInternal();

	//

	GIFTag m_tag;
	int m_nreg;

	// FIXME: savestate
	GIFRegPRIM* m_pPRIM;
	UINT32 m_PRIM;

	GSRegCSR* m_pCSRr;
	void (*m_fpGSirq)();

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

public:
	GSState(int w, int h, HWND hWnd, HRESULT& hr);
	virtual ~GSState();

	virtual HRESULT ResetDevice(bool fForceWindowed = false);

	UINT32 Freeze(freezeData* fd, bool fSizeOnly);
	UINT32 Defrost(const freezeData* fd);
	void Write(GS_REG mem, GSReg* r, UINT64 mask);
	UINT64 Read(GS_REG mem);
	void ReadFIFO(BYTE* pMem);
	void Transfer1(BYTE* pMem, UINT32 addr);
	void Transfer(BYTE* pMem);
	void Transfer(BYTE* pMem, UINT32 size);
	void VSync();

	void GSirq(void (*fpGSirq)()) {m_fpGSirq = fpGSirq;}
	void GSsetCSR(UINT64* pCSRr) {m_pCSRr = pCSRr ? (GSRegCSR*)pCSRr : &m_rs.CSRr;}

	UINT32 MakeSnapshot(char* path);
	void Capture();

	CString m_strDefaultTitle;
	int m_iOSD;
	void ToggleOSD();

	// state
	void CaptureState(CString fn);
	FILE* m_sfp;

	// logging
	FILE* m_fp;

#ifdef DEBUG_LOGVERTICES
    #define LOGV(_x_) LOGVERTEX _x_
#else
    #define LOGV(_x_)
#endif

#ifdef DEBUG_LOG
	void LOG(LPCTSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		////////////
		/*
		if(_tcsstr(fmt, _T("VSync")) 
		|| _tcsstr(fmt, _T("*** WARNING ***"))
		// || _tcsstr(fmt, _T("Flush"))
		|| _tcsstr(fmt, _T("CSR"))
		|| _tcsstr(fmt, _T("DISP"))
		|| _tcsstr(fmt, _T("FRAME"))
		// || _tcsstr(fmt, _T("ZBUF"))
		 || _tcsstr(fmt, _T("SMODE"))
		 || _tcsstr(fmt, _T("PMODE"))
		// || _tcsstr(fmt, _T("BITBLTBUF"))
		// || _tcsstr(fmt, _T("TRX"))
		// || _tcsstr(fmt, _T("PRIM"))
		// || _tcsstr(fmt, _T("RGB"))
		// || _tcsstr(fmt, _T("XYZ"))
		// || _tcsstr(fmt, _T("ST"))
		|| _tcsstr(fmt, _T("XYOFFSET"))
		// || _tcsstr(fmt, _T("TEX"))
		// || _tcsstr(fmt, _T("TEX0"))
		// || _tcsstr(fmt, _T("TEX2"))
		// || _tcsstr(fmt, _T("TEXFLUSH"))
		// || _tcsstr(fmt, _T("UV"))
		// || _tcsstr(fmt, _T("FOG"))
		// || _tcsstr(fmt, _T("ALPHA"))
		// || _tcsstr(fmt, _T("TBP0")) == fmt
		// || _tcsstr(fmt, _T("CBP")) == fmt
		// || _tcsstr(fmt, _T("*TC2 ")) == fmt
		)
		*/
		if(m_fp)
		{
			TCHAR buff[2048];
			_stprintf(buff, _T("%d: "), clock());
			_vstprintf(buff + strlen(buff), fmt, args);
			_fputts(buff, m_fp); 
			fflush(m_fp);
		}
		va_end(args);
	}
#else
#define LOG __noop
#endif

#ifdef DEBUG_LOG2
	void LOG2(LPCTSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		if(m_fp)
		{
			TCHAR buff[2048];
			_stprintf(buff, _T("%d: "), clock());
			_vstprintf(buff + strlen(buff), fmt, args);
			_fputts(buff, m_fp); 
		}
		va_end(args);
	}
#else
#define LOG2 __noop
#endif
};
