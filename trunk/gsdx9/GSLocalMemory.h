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

#pragma once

#include "GS.h"

__forceinline DWORD SwapRB(DWORD c)
{
	return (c&0xff000000)|((c<<16)&0x00ff0000)|(c&0x0000ff00)|((c>>16)&0x000000ff);
}

class GSLocalMemory
{
protected:
	static WORD blockTable32[4][8];
	static WORD blockTable32Z[4][8];
	static WORD blockTable16[8][4];
	static WORD blockTable16S[8][4];
	static WORD blockTable16Z[8][4];
	static WORD blockTable16SZ[8][4];
	static WORD blockTable8[4][8];
	static WORD blockTable4[8][4];
	static WORD columnTable32[8][8];
	static WORD columnTable16[8][16];
	static WORD columnTable8[16][16];
	static WORD columnTable4[16][32];
	static WORD pageOffset32[32][32][64];
	static WORD pageOffset32Z[32][32][64];
	static WORD pageOffset16[32][64][64];
	static WORD pageOffset16S[32][64][64];
	static WORD pageOffset16Z[32][64][64];
	static WORD pageOffset16SZ[32][64][64];
	static WORD pageOffset8[32][64][128];
	static WORD pageOffset4[32][128][128];

	union {BYTE* m_vm8; WORD* m_vm16; DWORD* m_vm32;};

	DWORD m_clut[256];
	BYTE m_bbt[256];

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	BYTE* GetVM() {return m_vm8;}

	typedef DWORD (GSLocalMemory::*pixelAddress)(int x, int y, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writePixel)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writeFrame)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readPixel)(int x, int y, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readTexel)(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*writePixelAddr)(int x, int y, DWORD c, DWORD addr);
	typedef void (GSLocalMemory::*writeFrameAddr)(int x, int y, DWORD c, DWORD addr);
	typedef DWORD (GSLocalMemory::*readPixelAddr)(int x, int y, DWORD addr);
	typedef DWORD (GSLocalMemory::*readTexelAddr)(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*unSwizzleTexture)(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);	

	DWORD pageAddress32(int x, int y, DWORD bp, DWORD bw);
	DWORD pageAddress16(int x, int y, DWORD bp, DWORD bw);
	DWORD pageAddress8(int x, int y, DWORD bp, DWORD bw);
	DWORD pageAddress4(int x, int y, DWORD bp, DWORD bw);

	pixelAddress GetPageAddress(DWORD psm);

	DWORD blockAddress32(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress24(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress16(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress16S(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress8(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress4(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress32Z(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress16Z(int x, int y, DWORD bp, DWORD bw);
	DWORD blockAddress16SZ(int x, int y, DWORD bp, DWORD bw);

	pixelAddress GetBlockAddress(DWORD psm);

	DWORD pixelAddressOrg32(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg24(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg16(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg16S(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg8(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg4(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg32Z(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg16Z(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddressOrg16SZ(int x, int y, DWORD bp, DWORD bw);

	pixelAddress GetPixelAddressOrg(DWORD psm);

	DWORD pixelAddress32(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress24(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16S(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress8(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress4(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress32Z(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16Z(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16SZ(int x, int y, DWORD bp, DWORD bw);

	pixelAddress GetPixelAddress(DWORD psm);

	void writePixel32(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel24(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel16(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel16S(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel8(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel8H(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel4(int x, int y, DWORD c, DWORD bp, DWORD bw);
    void writePixel4HL(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel4HH(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel32Z(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel24Z(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel16Z(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writePixel16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw);

	writePixel GetWritePixel(DWORD psm);

	void writeFrame16(int x, int y, DWORD c, DWORD bp, DWORD bw);
	void writeFrame16S(int x, int y, DWORD c, DWORD bp, DWORD bw);

	writeFrame GetWriteFrame(DWORD psm);

	DWORD readPixel32(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel24(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel16(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel16S(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel8(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel8H(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel4(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel4HL(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel4HH(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel32Z(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel24Z(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel16Z(int x, int y, DWORD bp, DWORD bw);
	DWORD readPixel16SZ(int x, int y, DWORD bp, DWORD bw);

	readPixel GetReadPixel(DWORD psm);

	DWORD readTexel32(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel24(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16S(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8H(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HL(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HH(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	readTexel GetReadTexel(DWORD psm);

	void writePixel32(int x, int y, DWORD c, DWORD addr);
	void writePixel24(int x, int y, DWORD c, DWORD addr);
	void writePixel16(int x, int y, DWORD c, DWORD addr);
	void writePixel16S(int x, int y, DWORD c, DWORD addr);
	void writePixel8(int x, int y, DWORD c, DWORD addr);
	void writePixel8H(int x, int y, DWORD c, DWORD addr);
	void writePixel4(int x, int y, DWORD c, DWORD addr);
    void writePixel4HL(int x, int y, DWORD c, DWORD addr);
	void writePixel4HH(int x, int y, DWORD c, DWORD addr);
	void writePixel32Z(int x, int y, DWORD c, DWORD addr);
	void writePixel24Z(int x, int y, DWORD c, DWORD addr);
	void writePixel16Z(int x, int y, DWORD c, DWORD addr);
	void writePixel16SZ(int x, int y, DWORD c, DWORD addr);

	writePixelAddr GetWritePixelAddr(DWORD psm);

	void writeFrame16(int x, int y, DWORD c, DWORD addr);
	void writeFrame16S(int x, int y, DWORD c, DWORD addr);

	writeFrameAddr GetWriteFrameAddr(DWORD psm);

	DWORD readPixel32(int x, int y, DWORD addr);
	DWORD readPixel24(int x, int y, DWORD addr);
	DWORD readPixel16(int x, int y, DWORD addr);
	DWORD readPixel16S(int x, int y, DWORD addr);
	DWORD readPixel8(int x, int y, DWORD addr);
	DWORD readPixel8H(int x, int y, DWORD addr);
	DWORD readPixel4(int x, int y, DWORD addr);
	DWORD readPixel4HL(int x, int y, DWORD addr);
	DWORD readPixel4HH(int x, int y, DWORD addr);
	DWORD readPixel32Z(int x, int y, DWORD addr);
	DWORD readPixel24Z(int x, int y, DWORD addr);
	DWORD readPixel16Z(int x, int y, DWORD addr);
	DWORD readPixel16SZ(int x, int y, DWORD addr);

	readPixelAddr GetReadPixelAddr(DWORD psm);

	DWORD readTexel32(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel24(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16S(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8H(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HL(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HH(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	readTexelAddr GetReadTexelAddr(DWORD psm);

	void setupCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT& TEXCLUT, GIFRegTEXA& TEXA); // modifies TEX0!
	DWORD readCLUT(BYTE c);

	bool FillRect(CRect& r, DWORD c, DWORD psm, DWORD fbp, DWORD fbw);

	void unSwizzleBlock32(BYTE* src, BYTE* dst, int dstpitch);
	void unSwizzleBlock16(BYTE* src, BYTE* dst, int dstpitch);
	void unSwizzleBlock8(BYTE* src, BYTE* dst, int dstpitch);
	void unSwizzleBlock4(BYTE* src, BYTE* dst, int dstpitch);

	// ARGB!
	void unSwizzleTexture32(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture24(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16S(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8H(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HL(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HH(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTextureX(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	unSwizzleTexture GetUnSwizzleTexture(DWORD psm);
};
