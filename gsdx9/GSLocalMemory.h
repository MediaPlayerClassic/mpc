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

class GSLocalMemory
{
protected:
	static int blockTable32[4][8];
	static int blockTable32Z[4][8];
	static int blockTable16[8][4];
	static int blockTable16S[8][4];
	static int blockTable16Z[8][4];
	static int blockTable16SZ[8][4];
	static int blockTable8[4][8];
	static int blockTable4[8][4];
	static int columnTable32[8][8];
	static int columnTable16[8][16];
	static int columnTable8[16][16];
	static int columnTable4[16][32];

	union {BYTE* m_vm8; WORD* m_vm16; DWORD* m_vm32;};
	union {BYTE* m_sm8; WORD* m_sm16; DWORD* m_sm32;};

	DWORD pixelAddress32(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress24(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16S(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress8(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress4(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress32Z(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16Z(int x, int y, DWORD bp, DWORD bw);
	DWORD pixelAddress16SZ(int x, int y, DWORD bp, DWORD bw);

	typedef DWORD (GSLocalMemory::*pixelAddress)(int x, int y, DWORD bp, DWORD bw);
	pixelAddress GetPixelAddress(DWORD psm);

	DWORD m_clut[256];

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	BYTE* GetVM() {return m_vm8;}
	BYTE* GetSM() {return m_sm8;}

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

	typedef void (GSLocalMemory::*writePixel)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	writePixel GetWritePixel(DWORD psm);

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

	typedef DWORD (GSLocalMemory::*readPixel)(int x, int y, DWORD bp, DWORD bw);
	readPixel GetReadPixel(DWORD psm);

	DWORD testPixel32(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel24(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel16(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel16S(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel8(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel8H(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel4(int x, int y, DWORD bp, DWORD bw);
    DWORD testPixel4HL(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel4HH(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel32Z(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel24Z(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel16Z(int x, int y, DWORD bp, DWORD bw);
	DWORD testPixel16SZ(int x, int y, DWORD bp, DWORD bw);

	typedef DWORD (GSLocalMemory::*testPixel)(int x, int y, DWORD bp, DWORD bw);
	testPixel GetTestPixel(DWORD psm);

	bool IsDirty(GIFRegTEX0& TEX0);
	bool IsPalDirty(GIFRegTEX0& TEX0);

	DWORD readTexel32(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel24(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel16(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel16S(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel8(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel8H(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel4(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel4HL(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	DWORD readTexel4HH(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);

	typedef DWORD (GSLocalMemory::*readTexel)(int x, int y, DWORD bp, DWORD bw, BYTE TCC, GIFRegTEXA& TEXA);
	readTexel GetReadTexel(DWORD psm);

	void setupCLUT(GIFRegTEX0& TEX0, GIFRegTEXCLUT& TEXCLUT, GIFRegTEXA& TEXA);
	DWORD readCLUT(BYTE c);

};
