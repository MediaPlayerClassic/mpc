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

#pragma warning(disable: 4244) // warning C4244: '=' : conversion from 'const UINT64' to 'int', possible loss of data

#include "GS.h"
#include "GSTables.h"

class GSLocalMemory
{
protected:
	static WORD pageOffset32[32][32][64];
	static WORD pageOffset32Z[32][32][64];
	static WORD pageOffset16[32][64][64];
	static WORD pageOffset16S[32][64][64];
	static WORD pageOffset16Z[32][64][64];
	static WORD pageOffset16SZ[32][64][64];
	static WORD pageOffset8[32][64][128];
	static WORD pageOffset4[32][128][128];

	union {BYTE* m_vm8; WORD* m_vm16; DWORD* m_vm32;};

	DWORD m_CBP[2];
	WORD* m_pCLUT;
	DWORD* m_pCLUT32;

	GIFRegTEX0 m_prevTEX0;
	GIFRegTEXCLUT m_prevTEXCLUT;
	bool m_fCLUTMayBeDirty;

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	static CSize GetBlockSize(DWORD PSM);

	BYTE* GetVM() {return m_vm8;}

	typedef DWORD (GSLocalMemory::*pixelAddress)(int x, int y, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writePixel)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writeFrame)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readPixel)(int x, int y, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readTexel)(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef DWORD (GSLocalMemory::*readTexelP)(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef DWORD (GSLocalMemory::*readTexelNP)(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*writePixelAddr)(DWORD addr, DWORD c);
	typedef void (GSLocalMemory::*writeFrameAddr)(DWORD addr, DWORD c);
	typedef DWORD (GSLocalMemory::*readPixelAddr)(DWORD addr);
	typedef DWORD (GSLocalMemory::*readTexelAddr)(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*SwizzleTexture)(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	typedef void (GSLocalMemory::*unSwizzleTexture)(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*unSwizzleTextureP)(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*readTexture)(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);

	// address

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

	// raw pixel R/W

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

	void writePixel32(DWORD addr, DWORD c);
	void writePixel24(DWORD addr, DWORD c);
	void writePixel16(DWORD addr, DWORD c);
	void writePixel16S(DWORD addr, DWORD c);
	void writePixel8(DWORD addr, DWORD c);
	void writePixel8H(DWORD addr, DWORD c);
	void writePixel4(DWORD addr, DWORD c);
    void writePixel4HL(DWORD addr, DWORD c);
	void writePixel4HH(DWORD addr, DWORD c);
	void writePixel32Z(DWORD addr, DWORD c);
	void writePixel24Z(DWORD addr, DWORD c);
	void writePixel16Z(DWORD addr, DWORD c);
	void writePixel16SZ(DWORD addr, DWORD c);

	writePixelAddr GetWritePixelAddr(DWORD psm);

	void writeFrame16(DWORD addr, DWORD c);
	void writeFrame16S(DWORD addr, DWORD c);

	writeFrameAddr GetWriteFrameAddr(DWORD psm);

	DWORD readPixel32(DWORD addr);
	DWORD readPixel24(DWORD addr);
	DWORD readPixel16(DWORD addr);
	DWORD readPixel16S(DWORD addr);
	DWORD readPixel8(DWORD addr);
	DWORD readPixel8H(DWORD addr);
	DWORD readPixel4(DWORD addr);
	DWORD readPixel4HL(DWORD addr);
	DWORD readPixel4HH(DWORD addr);
	DWORD readPixel32Z(DWORD addr);
	DWORD readPixel24Z(DWORD addr);
	DWORD readPixel16Z(DWORD addr);
	DWORD readPixel16SZ(DWORD addr);

	readPixelAddr GetReadPixelAddr(DWORD psm);

	// FillRect

	bool FillRect(CRect& r, DWORD c, DWORD psm, DWORD fbp, DWORD fbw);

	// CLUT

	void InvalidateCLUT() {m_fCLUTMayBeDirty = true;}
	void WriteCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT);

	void ReadCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32);
	void SetupCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA);

	// expands 16->32
	void ReadCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32);
	void SetupCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA);
	void CopyCLUT32(DWORD* pCLUT32, int nPaletteEntries);

	// 32-only

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

	DWORD readTexel32(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel24(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16S(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8H(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HL(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HH(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	readTexelAddr GetReadTexelAddr(DWORD psm);

	void SwizzleTexture32(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture16(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture16S(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture8(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture4(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTextureX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	SwizzleTexture GetSwizzleTexture(DWORD psm);

	void unSwizzleTexture32(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture24(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16S(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8H(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HL(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HH(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	unSwizzleTexture GetUnSwizzleTexture(DWORD psm);

	void ReadTexture(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);

	// 32/16/8P

	DWORD readTexel32P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel16SP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel8HP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HLP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	DWORD readTexel4HHP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	readTexelP GetReadTexelP(DWORD psm);

	void unSwizzleTexture32P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16SP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8HP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HLP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HHP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	unSwizzleTextureP GetUnSwizzleTextureP(DWORD psm);

	void ReadTextureP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);

	//

	template<typename DstT> 
	void ReadTexture(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP, readTexel rt)
	{
		if((CLAMP.WMS&2) || (CLAMP.WMT&2))
		{
			for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
			{
				for(int x = r.left; x < r.right; x++)
				{
					int tx, ty;

					switch(CLAMP.WMS)
					{
					default: tx = x; break;
					case 2: tx = x < CLAMP.MINU ? CLAMP.MINU : x > CLAMP.MAXU ? CLAMP.MAXU : x; break;
					case 3: tx = (x & CLAMP.MINU) | CLAMP.MAXU; break;
					}

					switch(CLAMP.WMT)
					{
					default: ty = y; break;
					case 2: ty = y < CLAMP.MINV ? CLAMP.MINV : y > CLAMP.MAXV ? CLAMP.MAXV : y; break;
					case 3: ty = (y & CLAMP.MINV) | CLAMP.MAXV; break;
					}

					((DstT*)dst)[(x-r.left)] = (DstT)(this->*rt)(tx, ty, TEX0, TEXA);
				}
			}
		}
		else
		{
			for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
				for(int x = r.left; x < r.right; x++)
					((DstT*)dst)[(x-r.left)] = (DstT)(this->*rt)(x, y, TEX0, TEXA);
		}
	}
};

#pragma warning(default: 4244)