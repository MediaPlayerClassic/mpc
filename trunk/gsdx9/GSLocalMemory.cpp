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
 *	Special Notes: 
 *
 *	Based on Page.c from GSSoft
 *	Copyright (C) 2002-2004 GSsoft Team
 *
 */
 
#include "StdAfx.h"
#include "GSLocalMemory.h"

__forceinline static DWORD From24To32(DWORD c, BYTE TCC, GIFRegTEXA& TEXA, BYTE* bbt)
{
//	BYTE A = TEX0.TCC == 0 ? 0x80 : (!TEXA.AEM|c[0]|c[1]|c[2]) ? TEXA.TA0 : 0;
	BYTE MASK1 = bbt[TCC]; // TEX0.TCC
	BYTE MASK2 = bbt[!TEXA.AEM|(c&0xff)|((c>>8)&0xff)|((c>>16)&0xff)];
	BYTE A = (~MASK1 & 0x80) | (MASK1 & (MASK2 & TEXA.TA0));
	return (A<<24) | (c&0x00ffffff);
}

__forceinline static DWORD From16To32(WORD c, GIFRegTEXA& TEXA, BYTE* bbt)
{
//	BYTE A = (c[1]&0x80) ? TEXA.TA1 : (!TEXA.AEM|c[0]|c[1]) ? TEXA.TA0 : 0;
	BYTE MASK1 = bbt[c>>15];
	BYTE MASK2 = bbt[!TEXA.AEM|(c&0xff)|((c>>8)&0x7f)];
	BYTE A = (MASK1 & TEXA.TA1) | (~MASK1 & (MASK2 & TEXA.TA0));
	return (A << 24) | ((c&0x7c00) << 9) | ((c&0x03e0) << 6) | ((c&0x001f) << 3);
}

__forceinline static DWORD From24To32ARGB(DWORD c, BYTE TCC, GIFRegTEXA& TEXA, BYTE* bbt)
{
	BYTE MASK1 = bbt[TCC]; // TEX0.TCC
	BYTE MASK2 = bbt[!TEXA.AEM|(c&0xff)|((c>>8)&0xff)|((c>>16)&0xff)];
	BYTE A = (~MASK1 & 0x80) | (MASK1 & (MASK2 & TEXA.TA0));
	return (A<<24) | ((c&0x00ff0000)>>16) | (c&0x0000ff00) | ((c&0x000000ff)<<16);
}

__forceinline static DWORD From16To32ARGB(WORD c, GIFRegTEXA& TEXA, BYTE* bbt)
{
	BYTE MASK1 = bbt[c>>15];
	BYTE MASK2 = bbt[!TEXA.AEM|(c&0xff)|((c>>8)&0x7f)];
	BYTE A = (MASK1 & TEXA.TA1) | (~MASK1 & (MASK2 & TEXA.TA0));
	return (A << 24) | ((c&0x7c00) >> 7) | ((c&0x03e0) << 6) | ((c&0x001f) << 19);
}

//

GSLocalMemory::GSLocalMemory()
{
	int len = 1024*1024*4*2; // *2 for safety...

	m_vm8 = new BYTE[len];
	memset(m_vm8, 0, len);

	memset(m_clut, 0, sizeof(m_clut));

	for(int bp = 0; bp < 32; bp++)
	{
		for(int y = 0; y < 32; y++) for(int x = 0; x < 64; x++)
		{
			pageOffset32[bp][y][x] = (WORD)pixelAddressOrg32(x, y, bp, 0);
			pageOffset32Z[bp][y][x] = (WORD)pixelAddressOrg32Z(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 64; x++) 
		{
			pageOffset16[bp][y][x] = (WORD)pixelAddressOrg16(x, y, bp, 0);
			pageOffset16S[bp][y][x] = (WORD)pixelAddressOrg16S(x, y, bp, 0);
			pageOffset16Z[bp][y][x] = (WORD)pixelAddressOrg16Z(x, y, bp, 0);
			pageOffset16SZ[bp][y][x] = (WORD)pixelAddressOrg16SZ(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset8[bp][y][x] = (WORD)pixelAddressOrg8(x, y, bp, 0);
		}

		for(int y = 0; y < 128; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset4[bp][y][x] = (WORD)pixelAddressOrg4(x, y, bp, 0);
		}
	}

	m_bbt[0] = 0;
	for(int i = 1; i < 256; i++)
		m_bbt[i] = 0xff;
}

GSLocalMemory::~GSLocalMemory()
{
	delete [] m_vm8;
}

////////////////////

DWORD GSLocalMemory::pageAddress32(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 5) * bw + (x >> 6)) << 11; 
}

DWORD GSLocalMemory::pageAddress16(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 6) * bw + (x >> 6)) << 12;
}

DWORD GSLocalMemory::pageAddress8(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7)) << 13; 
}

DWORD GSLocalMemory::pageAddress4(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7)) << 14;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetPageAddress(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::pageAddress16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::pageAddress16; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::pageAddress16; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::pageAddress16; break;	
	case PSM_PSMT8: pa = &GSLocalMemory::pageAddress8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::pageAddress4; break;
	}

	return pa;
}

////////////////////

DWORD GSLocalMemory::blockAddress32(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
	DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	return (page + block) << 6;
}

DWORD GSLocalMemory::blockAddress16(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD GSLocalMemory::blockAddress16S(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD GSLocalMemory::blockAddress8(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	return (page + block) << 8;
}

DWORD GSLocalMemory::blockAddress4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	return (page + block) << 9;
}

DWORD GSLocalMemory::blockAddress32Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	return (page + block) << 6;
}

DWORD GSLocalMemory::blockAddress16Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD GSLocalMemory::blockAddress16SZ(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetBlockAddress(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::blockAddress16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::blockAddress16S; break;
	case PSM_PSMT8: pa = &GSLocalMemory::blockAddress8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::blockAddress4; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::blockAddress32Z; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::blockAddress32Z; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::blockAddress16Z; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::blockAddress16SZ; break;
	}
	
	return pa;
}

////////////////////

DWORD GSLocalMemory::pixelAddressOrg32(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable32[(y >> 3) & 3][(x >> 3) & 7];
//	DWORD word = (((page << 5) + block) << 6) + columnTable32[y & 7][x & 7];
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
	DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + columnTable16[y & 7][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16S(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + columnTable16[y & 7][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg8(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7); 
//	DWORD block = (bp & 0x1f) + blockTable8[(y >> 4) & 3][(x >> 4) & 7];
//	DWORD word = (((page << 5) + block) << 8) + columnTable8[y & 15][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	DWORD word = ((page + block) << 8) + columnTable8[y & 15][x & 15];
//	ASSERT(word < 1024*1024*4);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg4(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7);
//	DWORD block = (bp & 0x1f) + blockTable4[(y >> 4) & 7][(x >> 5) & 3];
//	DWORD word = (((page << 5) + block) << 9) + columnTable4[y & 15][x & 31];
	DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	DWORD word = ((page + block) << 9) + columnTable4[y & 15][x & 31];
	ASSERT(word < 1024*1024*8);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg32Z(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
//	DWORD word = (((page << 5) + block) << 6) + ((y & 7) << 3) + (x & 7);
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	DWORD word = ((page + block) << 6) + ((y & 7) << 3) + (x & 7);
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16Z(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + ((y & 7) << 4) + (x & 15);
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + ((y & 7) << 4) + (x & 15);
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16SZ(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + ((y & 7) << 4) + (x & 15);
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + ((y & 7) << 4) + (x & 15);
	ASSERT(word < 1024*1024*2);
	return word;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetPixelAddressOrg(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::pixelAddressOrg16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::pixelAddressOrg16S; break;
	case PSM_PSMT8: pa = &GSLocalMemory::pixelAddressOrg8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::pixelAddressOrg4; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::pixelAddressOrg32Z; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::pixelAddressOrg32Z; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::pixelAddressOrg16Z; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::pixelAddressOrg16SZ; break;
	}
	
	return pa;
}

////////////////////

DWORD GSLocalMemory::pixelAddress32(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	DWORD word = (page << 11) + pageOffset32[bp & 0x1f][y & 0x1f][x & 0x3f];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddress16(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddress16S(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16S[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddress8(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7); 
	DWORD word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];
	ASSERT(word < 1024*1024*4);
	return word;
}

DWORD GSLocalMemory::pixelAddress4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7);
	DWORD word = (page << 14) + pageOffset4[bp & 0x1f][y & 0x7f][x & 0x7f];
	ASSERT(word < 1024*1024*8);
	return word;
}

DWORD GSLocalMemory::pixelAddress32Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	DWORD word = (page << 11) + pageOffset32Z[bp & 0x1f][y & 0x1f][x & 0x3f];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddress16Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16Z[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddress16SZ(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16SZ[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetPixelAddress(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::pixelAddress16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::pixelAddress16S; break;
	case PSM_PSMT8: pa = &GSLocalMemory::pixelAddress8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::pixelAddress4; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::pixelAddress32Z; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::pixelAddress32Z; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::pixelAddress16Z; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::pixelAddress16SZ; break;
	}
	
	return pa;
}

////////////////////

void GSLocalMemory::writePixel32(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16S(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel8(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress8(x, y, bp, bw);
	m_vm8[addr] = (BYTE)c;
}

void GSLocalMemory::writePixel8H(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
}

void GSLocalMemory::writePixel4(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress4(x, y, bp, bw);
	int shift = (addr&1) << 2; addr >>= 1;
	m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
}

void GSLocalMemory::writePixel4HL(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
}

void GSLocalMemory::writePixel4HH(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
}

void GSLocalMemory::writePixel32Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32Z(x, y, bp, bw);
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32Z(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16Z(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16SZ(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

GSLocalMemory::writePixel GSLocalMemory::GetWritePixel(DWORD psm)
{
	writePixel wp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: wp = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wp = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wp = &GSLocalMemory::writePixel16; break;
	case PSM_PSMCT16S: wp = &GSLocalMemory::writePixel16S; break;
	case PSM_PSMT8: wp = &GSLocalMemory::writePixel8; break;
	case PSM_PSMT4: wp = &GSLocalMemory::writePixel4; break;
	case PSM_PSMT8H: wp = &GSLocalMemory::writePixel8H; break;
	case PSM_PSMT4HL: wp = &GSLocalMemory::writePixel4HL; break;
	case PSM_PSMT4HH: wp = &GSLocalMemory::writePixel4HH; break;
	case PSM_PSMZ32: wp = &GSLocalMemory::writePixel32Z; break;
	case PSM_PSMZ24: wp = &GSLocalMemory::writePixel24Z; break;
	case PSM_PSMZ16: wp = &GSLocalMemory::writePixel16Z; break;
	case PSM_PSMZ16S: wp = &GSLocalMemory::writePixel16SZ; break;
	}
	
	return wp;
}

////////////////////

void GSLocalMemory::writeFrame16(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16(x, y, c, bp, bw);
}

void GSLocalMemory::writeFrame16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16S(x, y, c, bp, bw);
}

GSLocalMemory::writeFrame GSLocalMemory::GetWriteFrame(DWORD psm)
{
	writeFrame wf = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: wf = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wf = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wf = &GSLocalMemory::writeFrame16; break;
	case PSM_PSMCT16S: wf = &GSLocalMemory::writeFrame16S; break;
	}
	
	return wf;
}

////////////////////

DWORD GSLocalMemory::readPixel32(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16S(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16S(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel8(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm8[pixelAddress8(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel8H(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] >> 24;
}

DWORD GSLocalMemory::readPixel4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress4(x, y, bp, bw);
	return (m_vm8[addr>>1] >> ((addr&1) << 2)) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HL(int x, int y, DWORD bp, DWORD bw)
{
	return (m_vm32[pixelAddress32(x, y, bp, bw)] >> 24) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HH(int x, int y, DWORD bp, DWORD bw)
{
	return (m_vm32[pixelAddress32(x, y, bp, bw)] >> 28) & 0x0f;
}

DWORD GSLocalMemory::readPixel32Z(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32Z(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24Z(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32Z(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16Z(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16Z(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16SZ(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16SZ(x, y, bp, bw)];
}

GSLocalMemory::readPixel GSLocalMemory::GetReadPixel(DWORD psm)
{
	readPixel rp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: rp = &GSLocalMemory::readPixel32; break;
	case PSM_PSMCT24: rp = &GSLocalMemory::readPixel24; break;
	case PSM_PSMCT16: rp = &GSLocalMemory::readPixel16; break;
	case PSM_PSMCT16S: rp = &GSLocalMemory::readPixel16S; break;
	case PSM_PSMT8: rp = &GSLocalMemory::readPixel8; break;
	case PSM_PSMT4: rp = &GSLocalMemory::readPixel4; break;
	case PSM_PSMT8H: rp = &GSLocalMemory::readPixel8H; break;
	case PSM_PSMT4HL: rp = &GSLocalMemory::readPixel4HL; break;
	case PSM_PSMT4HH: rp = &GSLocalMemory::readPixel4HH; break;
	case PSM_PSMZ32: rp = &GSLocalMemory::readPixel32Z; break;
	case PSM_PSMZ24: rp = &GSLocalMemory::readPixel24Z; break;
	case PSM_PSMZ16: rp = &GSLocalMemory::readPixel16Z; break;
	case PSM_PSMZ16S: rp = &GSLocalMemory::readPixel16SZ; break;
	}
	
	return rp;
}

////////////////////

DWORD GSLocalMemory::readTexel32(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel24(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From24To32(m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)], TEX0.ai32[1]&4, TEXA, m_bbt);
}

DWORD GSLocalMemory::readTexel16(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[pixelAddress16(x, y, TEX0.TBP0, TEX0.TBW)], TEXA, m_bbt);
}

DWORD GSLocalMemory::readTexel16S(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[pixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], TEXA, m_bbt);
}

DWORD GSLocalMemory::readTexel8(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel8(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel8H(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel8H(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel4(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HL(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HH(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW)];
}

GSLocalMemory::readTexel GSLocalMemory::GetReadTexel(DWORD psm)
{
	readTexel rt = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: rt = &GSLocalMemory::readTexel32; break;
	case PSM_PSMCT24: rt = &GSLocalMemory::readTexel24; break;
	case PSM_PSMCT16: rt = &GSLocalMemory::readTexel16; break;
	case PSM_PSMCT16S: rt = &GSLocalMemory::readTexel16S; break;
	case PSM_PSMT8: rt = &GSLocalMemory::readTexel8; break;
	case PSM_PSMT4: rt = &GSLocalMemory::readTexel4; break;
	case PSM_PSMT8H: rt = &GSLocalMemory::readTexel8H; break;
	case PSM_PSMT4HL: rt = &GSLocalMemory::readTexel4HL; break;
	case PSM_PSMT4HH: rt = &GSLocalMemory::readTexel4HH; break;
	}
	
	return rt;
}

////////////////////

void GSLocalMemory::writePixel32(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16(int x, int y, DWORD c, DWORD addr)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16S(int x, int y, DWORD c, DWORD addr)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel8(int x, int y, DWORD c, DWORD addr)
{
	m_vm8[addr] = (BYTE)c;
}

void GSLocalMemory::writePixel8H(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
}

void GSLocalMemory::writePixel4(int x, int y, DWORD c, DWORD addr)
{
	int shift = (addr&1) << 2; addr >>= 1;
	m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
}

void GSLocalMemory::writePixel4HL(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
}

void GSLocalMemory::writePixel4HH(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
}

void GSLocalMemory::writePixel32Z(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24Z(int x, int y, DWORD c, DWORD addr)
{
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16Z(int x, int y, DWORD c, DWORD addr)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16SZ(int x, int y, DWORD c, DWORD addr)
{
	m_vm16[addr] = (WORD)c;
}

GSLocalMemory::writePixelAddr GSLocalMemory::GetWritePixelAddr(DWORD psm)
{
	writePixelAddr wp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: wp = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wp = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wp = &GSLocalMemory::writePixel16; break;
	case PSM_PSMCT16S: wp = &GSLocalMemory::writePixel16S; break;
	case PSM_PSMT8: wp = &GSLocalMemory::writePixel8; break;
	case PSM_PSMT4: wp = &GSLocalMemory::writePixel4; break;
	case PSM_PSMT8H: wp = &GSLocalMemory::writePixel8H; break;
	case PSM_PSMT4HL: wp = &GSLocalMemory::writePixel4HL; break;
	case PSM_PSMT4HH: wp = &GSLocalMemory::writePixel4HH; break;
	case PSM_PSMZ32: wp = &GSLocalMemory::writePixel32Z; break;
	case PSM_PSMZ24: wp = &GSLocalMemory::writePixel24Z; break;
	case PSM_PSMZ16: wp = &GSLocalMemory::writePixel16Z; break;
	case PSM_PSMZ16S: wp = &GSLocalMemory::writePixel16SZ; break;
	}
	
	return wp;
}

////////////////////

void GSLocalMemory::writeFrame16(int x, int y, DWORD c, DWORD addr)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16(x, y, c, addr);
}

void GSLocalMemory::writeFrame16S(int x, int y, DWORD c, DWORD addr)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16S(x, y, c, addr);
}

GSLocalMemory::writeFrameAddr GSLocalMemory::GetWriteFrameAddr(DWORD psm)
{
	writeFrameAddr wf = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: wf = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wf = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wf = &GSLocalMemory::writeFrame16; break;
	case PSM_PSMCT16S: wf = &GSLocalMemory::writeFrame16S; break;
	}
	
	return wf;
}

////////////////////

DWORD GSLocalMemory::readPixel32(int x, int y, DWORD addr)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readPixel24(int x, int y, DWORD addr)
{
	return m_vm32[addr] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(int x, int y, DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel16S(int x, int y, DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel8(int x, int y, DWORD addr)
{
	return (DWORD)m_vm8[addr];
}

DWORD GSLocalMemory::readPixel8H(int x, int y, DWORD addr)
{
	return m_vm32[addr] >> 24;
}

DWORD GSLocalMemory::readPixel4(int x, int y, DWORD addr)
{
	return (m_vm8[addr>>1] >> ((addr&1) << 2)) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HL(int x, int y, DWORD addr)
{
	return (m_vm32[addr] >> 24) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HH(int x, int y, DWORD addr)
{
	return (m_vm32[addr] >> 28) & 0x0f;
}

DWORD GSLocalMemory::readPixel32Z(int x, int y, DWORD addr)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readPixel24Z(int x, int y, DWORD addr)
{
	return m_vm32[addr] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16Z(int x, int y, DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel16SZ(int x, int y, DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

GSLocalMemory::readPixelAddr GSLocalMemory::GetReadPixelAddr(DWORD psm)
{
	readPixelAddr rp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: rp = &GSLocalMemory::readPixel32; break;
	case PSM_PSMCT24: rp = &GSLocalMemory::readPixel24; break;
	case PSM_PSMCT16: rp = &GSLocalMemory::readPixel16; break;
	case PSM_PSMCT16S: rp = &GSLocalMemory::readPixel16S; break;
	case PSM_PSMT8: rp = &GSLocalMemory::readPixel8; break;
	case PSM_PSMT4: rp = &GSLocalMemory::readPixel4; break;
	case PSM_PSMT8H: rp = &GSLocalMemory::readPixel8H; break;
	case PSM_PSMT4HL: rp = &GSLocalMemory::readPixel4HL; break;
	case PSM_PSMT4HH: rp = &GSLocalMemory::readPixel4HH; break;
	case PSM_PSMZ32: rp = &GSLocalMemory::readPixel32Z; break;
	case PSM_PSMZ24: rp = &GSLocalMemory::readPixel24Z; break;
	case PSM_PSMZ16: rp = &GSLocalMemory::readPixel16Z; break;
	case PSM_PSMZ16S: rp = &GSLocalMemory::readPixel16SZ; break;
	}
	
	return rp;
}

////////////////////

DWORD GSLocalMemory::readTexel32(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readTexel24(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From24To32(m_vm32[addr], TEX0.ai32[1]&4, TEXA, m_bbt);
}

DWORD GSLocalMemory::readTexel16(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[addr], TEXA, m_bbt);
}

DWORD GSLocalMemory::readTexel16S(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[addr], TEXA, m_bbt);
}

DWORD GSLocalMemory::readTexel8(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel8(x, y, addr)];
}

DWORD GSLocalMemory::readTexel8H(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel8H(x, y, addr)];
}

DWORD GSLocalMemory::readTexel4(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel4(x, y, addr)];
}

DWORD GSLocalMemory::readTexel4HL(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel4HL(x, y, addr)];
}

DWORD GSLocalMemory::readTexel4HH(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_clut[readPixel4HH(x, y, addr)];
}

GSLocalMemory::readTexelAddr GSLocalMemory::GetReadTexelAddr(DWORD psm)
{
	readTexelAddr rt = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: rt = &GSLocalMemory::readTexel32; break;
	case PSM_PSMCT24: rt = &GSLocalMemory::readTexel24; break;
	case PSM_PSMCT16: rt = &GSLocalMemory::readTexel16; break;
	case PSM_PSMCT16S: rt = &GSLocalMemory::readTexel16S; break;
	case PSM_PSMT8: rt = &GSLocalMemory::readTexel8; break;
	case PSM_PSMT4: rt = &GSLocalMemory::readTexel4; break;
	case PSM_PSMT8H: rt = &GSLocalMemory::readTexel8H; break;
	case PSM_PSMT4HL: rt = &GSLocalMemory::readTexel4HL; break;
	case PSM_PSMT4HH: rt = &GSLocalMemory::readTexel4HH; break;
	}
	
	return rt;
}

////////////////////

void GSLocalMemory::setupCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT& TEXCLUT, GIFRegTEXA& TEXA)
{
	TEX0.TBP0 = TEX0.CBP;
	TEX0.TBW = TEX0.CSM == 0 ? 1 : TEXCLUT.CBW;

	if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
	{
		readTexel rt = GetReadTexel(TEX0.CPSM);

		if(TEX0.CSM == 0)
		{
			for(int y = 0, cy = 0; y < 8; y++)
			{
				int i = 0;
				for(int x = 0; x < 8; x++, i++) m_clut[y*32 + x] = (this->*rt)(i, cy, TEX0, TEXA);
				for(int x = 16; x < 24; x++, i++) m_clut[y*32 + x] = (this->*rt)(i, cy, TEX0, TEXA);
				cy++;
				i = 0;
				for(int x = 8; x < 16; x++, i++) m_clut[y*32 + x] = (this->*rt)(i, cy, TEX0, TEXA);
				for(int x = 24; x < 32; x++, i++) m_clut[y*32 + x] = (this->*rt)(i, cy, TEX0, TEXA);
				cy++;
			}
		}
		else
		{
			for(int i = 0; i < 256; i++)
				m_clut[i] = (this->*rt)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, TEX0, TEXA);
		}
	}
	else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
	{
		readTexel rt = GetReadTexel(TEX0.CPSM);

		if(TEX0.CSM == 0)
		{
			for(int y = 0; y < 2; y++)
				for(int x = 0; x < 8; x++)
					m_clut[y*8 + x] = (this->*rt)(x, y, TEX0, TEXA);
		}
		else
		{
			for(int i = 0; i < 16; i++)
				m_clut[i] = (this->*rt)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, TEX0, TEXA);
		}
	}
}

DWORD GSLocalMemory::readCLUT(BYTE c)
{
	return m_clut[c];
}

////////////////////

bool GSLocalMemory::FillRect(CRect& r, DWORD c, DWORD psm, DWORD fbp, DWORD fbw)
{
	int w, h, bpp;

	writePixel wp = GetWritePixel(psm);
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0); 
	case PSM_PSMCT32: w = 8; h = 8; bpp = 32; break;
	case PSM_PSMCT24: w = 8; h = 8; bpp = 32; break;
	case PSM_PSMCT16: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMCT16S: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMZ32: w = 8; h = 8; bpp = 32; break;
	case PSM_PSMZ16: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMZ16S: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMT8: w = 16; h = 16; bpp = 8; break;
	case PSM_PSMT4: w = 32; h = 16; bpp = 4; break;
	}

	pa = GetBlockAddress(psm);

	int dwords = w*h*bpp/8/4;
	int shift = 0;

	switch(bpp)
	{
	case 32: shift = 0; break;
	case 16: shift = 1; c = (c&0xffff)*0x00010001; break;
	case 8: shift = 2; c = (c&0xff)*0x01010101; break;
	case 4: shift = 3; c = (c&0xf)*0x11111111; break;
	}

	CRect clip((r.left+(w-1))&~(w-1), (r.top+(h-1))&~(h-1), r.right&~(w-1), r.bottom&~(h-1));

	for(int y = r.top; y < clip.top; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	for(int y = clip.top; y < clip.bottom; y += h)
	{
		for(int ys = y, ye = y + w; ys < ye; ys++)
		{
			for(int x = r.left; x < clip.left; x++)
				(this->*wp)(x, ys, c, fbp, fbw);
			for(int x = clip.right; x < r.right; x++)
				(this->*wp)(x, ys, c, fbp, fbw);
		}
	}

	if(psm == PSM_PSMCT24)
	{
		c &= 0x00ffffff;
		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				DWORD* p = &m_vm32[(this->*pa)(x, y, fbp, fbw)];
				for(int size = dwords; size-- > 0; p++)
					*p = (*p&0xff000000)|c;
			}
		}
	}
	else
	{
		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				void* p = &m_vm8[(this->*pa)(x, y, fbp, fbw) << 2 >> shift];

				__asm
				{
					mov eax, c
					mov ecx, dwords
					mov edi, p
					cld
					rep stosd
				}
			}
		}
	}

	for(int y = clip.bottom; y < r.bottom; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	return(true);
}

////////////////////////

#define punpck(op, sd0, sd2, s1, s3, d1, d3)	\
	__asm movaps		xmm##d1, xmm##sd0		\
	__asm pshufd		xmm##d3, xmm##sd2, 0xe4	\
	__asm punpckl##op	xmm##sd0, xmm##s1		\
	__asm punpckh##op	xmm##d1, xmm##s1		\
	__asm punpckl##op	xmm##sd2, xmm##s3		\
	__asm punpckh##op	xmm##d3, xmm##s3		\

// unpacks nibble to byte (0, 1, 2, 3 -> 0, 2, 4, 6), xmm7 is expected to be 0x0f..0f
#define punpcknb						\
	__asm movaps	xmm4, xmm0			\
	__asm pshufd	xmm5, xmm1, 0xe4	\
										\
	__asm psllq		xmm1, 4				\
	__asm psrlq		xmm4, 4				\
										\
	__asm movaps	xmm6, xmm7			\
	__asm pand		xmm0, xmm7			\
	__asm pandn		xmm6, xmm1			\
	__asm por		xmm0, xmm6			\
										\
	__asm movaps	xmm6, xmm7			\
	__asm pand		xmm4, xmm7			\
	__asm pandn		xmm6, xmm5			\
	__asm por		xmm4, xmm6			\
										\
	__asm movaps	xmm1, xmm4			\
										\
	__asm movaps	xmm4, xmm2			\
	__asm pshufd	xmm5, xmm3, 0xe4	\
										\
	__asm psllq		xmm3, 4				\
	__asm psrlq		xmm4, 4				\
										\
	__asm movaps	xmm6, xmm7			\
	__asm pand		xmm2, xmm7			\
	__asm pandn		xmm6, xmm3			\
	__asm por		xmm2, xmm6			\
										\
	__asm movaps	xmm6, xmm7			\
	__asm pand		xmm4, xmm7			\
	__asm pandn		xmm6, xmm5			\
	__asm por		xmm4, xmm6			\
										\
	__asm movaps	xmm3, xmm4			\
										\
	punpck(bw, 0, 2, 1, 3, 4, 6)		\

void GSLocalMemory::unSwizzleBlock32(BYTE* src, BYTE* dst, int dstpitch)
{
#if _M_IX86_FP >= 2
/*
clock_t start = clock();
for(int i = 0; i < 100000000; i++)
{
}
clock_t diff = clock() - start;
CString str;
str.Format(_T("%d"), diff);
AfxMessageBox(str, MB_OK);
*/
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, dstpitch
		mov			ecx, 4

		align 16
unSwizzleBlock32_loop:
		movaps		xmm0, [esi+16*0]
		movaps		xmm1, [esi+16*1]
		movaps		xmm2, [esi+16*2]
		movaps		xmm3, [esi+16*3]

		punpck(qdq, 0, 2, 1, 3, 4, 6)

		movaps		[edi], xmm0
		movaps		[edi+16], xmm2
		movaps		[edi+edx], xmm4
		movaps		[edi+edx+16], xmm6

		add			esi, 64
		lea			edi, [edi+edx*2]

		dec			ecx
		jnz			unSwizzleBlock32_loop
	}
#else
	WORD* s = &columnTable32[0][0];
	for(int j = 0, diff = dstpitch - 8*4; j < 8; j++, dst += diff)
		for(int i = 0; i < 8; i++, dst += 4)
			*(DWORD*)dst = ((DWORD*)src)[*s++];
#endif
}

void GSLocalMemory::unSwizzleBlock16(BYTE* src, BYTE* dst, int dstpitch)
{
/*	for(int j = 0, k = 0; j < 8; j++)
		for(int i = 0; i < 16; i++)
			((WORD*)src)[columnTable16[j][i]] = k++;
*/
#if _M_IX86_FP >= 2
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, dstpitch
		mov			ecx, 4

		align 16
unSwizzleBlock16_loop:
		movaps		xmm0, [esi+16*0]
		movaps		xmm1, [esi+16*1]
		movaps		xmm2, [esi+16*2]
		movaps		xmm3, [esi+16*3]

		punpck(wd, 0, 2, 1, 3, 4, 6);
		punpck(dq, 0, 4, 2, 6, 1, 3);
		punpck(wd, 0, 4, 1, 3, 2, 6);

		movaps		[edi], xmm0
		movaps		[edi+16], xmm2
		movaps		[edi+edx], xmm4
		movaps		[edi+edx+16], xmm6

		add			esi, 64
		lea			edi, [edi+edx*2]

		dec			ecx
		jnz			unSwizzleBlock16_loop
	}
#else
	WORD* s = &columnTable16[0][0];
	for(int j = 0, diff = dstpitch - 16*2; j < 8; j++, dst += diff)
		for(int i = 0; i < 16; i++, dst += 2)
			*(WORD*)dst = ((WORD*)src)[*s++];
#endif
}

void GSLocalMemory::unSwizzleBlock8(BYTE* src, BYTE* dst, int dstpitch)
{
/*
	for(int j = 0, k = 0; j < 16; j++)
		for(int i = 0; i < 16; i++)
			src[columnTable8[j][i]] = k++;
*/

#if _M_IX86_FP >= 2
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, dstpitch
		mov			ecx, 2

		align 16
unSwizzleBlock8_loop:

		// col 0, 2

		movaps		xmm0, [esi+16*0]
		movaps		xmm1, [esi+16*1]
		movaps		xmm4, [esi+16*2]
		movaps		xmm5, [esi+16*3]

		punpck(bw,  0, 4, 1, 5, 2, 6);
		punpck(wd,  0, 2, 4, 6, 1, 3);
		punpck(bw,  0, 2, 1, 3, 4, 6);
		punpck(qdq, 0, 2, 4, 6, 1, 3);

		pshufd		xmm1, xmm1, 0xb1
		pshufd		xmm3, xmm3, 0xb1

		movaps		[edi], xmm0
		movaps		[edi+edx], xmm2
		lea			edi, [edi+edx*2]

		movaps		[edi], xmm1
		movaps		[edi+edx], xmm3
		lea			edi, [edi+edx*2]

		// col 1, 3

		movaps		xmm0, [esi+16*4]
		movaps		xmm1, [esi+16*5]
		movaps		xmm4, [esi+16*6]
		movaps		xmm5, [esi+16*7]

		punpck(bw,  0, 4, 1, 5, 2, 6);
		punpck(wd,  0, 2, 4, 6, 1, 3);
		punpck(bw,  0, 2, 1, 3, 4, 6);
		punpck(qdq, 0, 2, 4, 6, 1, 3);

		pshufd		xmm0, xmm0, 0xb1
		pshufd		xmm2, xmm2, 0xb1

		movaps		[edi], xmm0
		movaps		[edi+edx], xmm2
		lea			edi, [edi+edx*2]

		movaps		[edi], xmm1
		movaps		[edi+edx], xmm3
		lea			edi, [edi+edx*2]

		add			esi, 128

		dec			ecx
		jnz			unSwizzleBlock8_loop
	}
#else
	WORD* s = &columnTable8[0][0];
	for(int j = 0, diff = dstpitch - 16; j < 16; j++, dst += diff)
		for(int i = 0; i < 16; i++)
			*dst++ = src[*s++];
#endif
}

void GSLocalMemory::unSwizzleBlock4(BYTE* src, BYTE* dst, int dstpitch)
{
/*
	BYTE tmp[32*16];
	for(int j = 0, k = 0; j < 16; j++)
		for(int i = 0; i < 32; i++)
			tmp[columnTable4[j][i]] = k++;
*/
/*
	memset(src, 0, 16*16);
	for(int j = 0, k = 0; j < 16; j++)
	{
		for(int i = 0; i < 32; i += 2)
		{
			WORD addr = columnTable4[j][i];
			if(addr&1) src[addr>>1] |= k++ << 4;
			else src[addr>>1] |= k++ & 15;
			addr = columnTable4[j][i+1];
			if(addr&1) src[addr>>1] |= k++ << 4;
			else src[addr>>1] |= k++ & 15;
		}
	}
*/
#if _M_IX86_FP >= 2
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, dstpitch
		mov			ecx, 2

		mov         eax, 0x0f0f0f0f
		movd        xmm7, eax 
		pshufd      xmm7, xmm7, 0

		align 16
unSwizzleBlock4_loop:

		// col 0, 2

		movaps		xmm0, [esi+16*0]
		movaps		xmm1, [esi+16*1]
		movaps		xmm4, [esi+16*2]
		movaps		xmm3, [esi+16*3]

		punpck(dq, 0, 4, 1, 3, 2, 6);
		punpck(dq, 0, 2, 4, 6, 1, 3);
		punpcknb
		punpck(bw, 0, 2, 4, 6, 1, 3);
		punpck(wd, 0, 2, 1, 3, 4, 6);

		pshufd		xmm0, xmm0, 0xd8
		pshufd		xmm2, xmm2, 0xd8
		pshufd		xmm4, xmm4, 0xd8
		pshufd		xmm6, xmm6, 0xd8

		punpck(qdq, 0, 2, 4, 6, 1, 3);

		pshuflw		xmm1, xmm1, 0xb1
		pshuflw		xmm3, xmm3, 0xb1
		pshufhw		xmm1, xmm1, 0xb1
		pshufhw		xmm3, xmm3, 0xb1

		movaps		[edi], xmm0
		movaps		[edi+edx], xmm2
		lea			edi, [edi+edx*2]

		movaps		[edi], xmm1
		movaps		[edi+edx], xmm3
		lea			edi, [edi+edx*2]

		// col 1, 3

		movaps		xmm0, [esi+16*4]
		movaps		xmm1, [esi+16*5]
		movaps		xmm4, [esi+16*6]
		movaps		xmm3, [esi+16*7]

		punpck(dq, 0, 4, 1, 3, 2, 6);
		punpck(dq, 0, 2, 4, 6, 1, 3);
		punpcknb
		punpck(bw, 0, 2, 4, 6, 1, 3);
		punpck(wd, 0, 2, 1, 3, 4, 6);

		pshufd		xmm0, xmm0, 0xd8
		pshufd		xmm2, xmm2, 0xd8
		pshufd		xmm4, xmm4, 0xd8
		pshufd		xmm6, xmm6, 0xd8

		punpck(qdq, 0, 2, 4, 6, 1, 3);

		pshuflw		xmm0, xmm0, 0xb1
		pshuflw		xmm2, xmm2, 0xb1
		pshufhw		xmm0, xmm0, 0xb1
		pshufhw		xmm2, xmm2, 0xb1

		movaps		[edi], xmm0
		movaps		[edi+edx], xmm2
		lea			edi, [edi+edx*2]

		movaps		[edi], xmm1
		movaps		[edi+edx], xmm3
		lea			edi, [edi+edx*2]

		add			esi, 128

		dec			ecx
		jnz			unSwizzleBlock4_loop
	}
#else
	WORD* s = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, dst += dstpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = *s++;
			BYTE c = (src[addr>>1] >> ((addr&1) << 2)) & 0x0f;
			int shift = (i&1) << 2;
			dst[i >> 1] = (dst[i >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
#endif
}

////////////////////

void GSLocalMemory::unSwizzleTexture32(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;
/*
	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
		for(int x = 0; x < tw; x += 8, dst += 8*4)
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)dst, dstpitch);
*/
	__declspec(align(16)) DWORD block[8*8];

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 8, dst += 8*4)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 8; j < 8; j++, d += diff2)
				for(int i = 0; i < 8; i++)
					*d++ = SwapRB(*s++);
		}
	}

}

void GSLocalMemory::unSwizzleTexture24(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	BYTE TCC = TEX0.TCC;

	__declspec(align(16)) DWORD block[8*8];

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 8, dst += 8*4)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 8; j < 8; j++, d += diff2)
				for(int i = 0; i < 8; i++)
					*d++ = From24To32ARGB(*s++, TCC, TEXA, m_bbt);
		}
	}
}

void GSLocalMemory::unSwizzleTexture16(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 16 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) WORD block[16*8];

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 16, dst += 16*4)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            WORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 16; j < 8; j++, d += diff2)
				for(int i = 0; i < 16; i++)
					*d++ = From16To32ARGB(*s++, TEXA, m_bbt);
		}
	}
}

void GSLocalMemory::unSwizzleTexture16S(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 16 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) WORD block[16*8];

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 16, dst += 16*4)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            WORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 16; j < 8; j++, d += diff2)
				for(int i = 0; i < 16; i++)
					*d++ = From16To32ARGB(*s++, TEXA, m_bbt);
		}
	}
}

void GSLocalMemory::unSwizzleTexture8(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 16 || th < 16)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) BYTE block[16*16];

	DWORD clut[256];
	for(int i = 0; i < 256; i++) clut[i] = SwapRB(m_clut[i]);

	for(int y = 0, diff = dstpitch*16 - tw*4; y < th; y += 16, dst += diff)
	{
		for(int x = 0; x < tw; x += 16, dst += 16*4)
		{
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, bp, bw)], (BYTE*)block, sizeof(block)/16);

            BYTE* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 16; j < 16; j++, d += diff2)
				for(int i = 0; i < 16; i++)
					*d++ = clut[*s++];
		}
	}
}

void GSLocalMemory::unSwizzleTexture8H(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) DWORD block[8*8];

	DWORD clut[256];
	for(int i = 0; i < 256; i++) clut[i] = SwapRB(m_clut[i]);

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 8, dst += 8*4)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 8; j < 8; j++, d += diff2)
				for(int i = 0; i < 8; i++)
					*d++ = clut[*s++ >> 24];
		}
	}
}

void GSLocalMemory::unSwizzleTexture4(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 32 || th < 16)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) BYTE block[(32/2)*16];

	DWORD clut[16];
	for(int i = 0; i < 16; i++) clut[i] = SwapRB(m_clut[i]);

	for(int y = 0, diff = dstpitch*16 - tw*4; y < th; y += 16, dst += diff)
	{
		for(int x = 0; x < tw; x += 32, dst += 32*4)
		{
			unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, bp, bw)>>1], (BYTE*)block, sizeof(block)/16);

            BYTE* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 32; j < 16; j++, d += diff2)
				for(int i = 0; i < 32; i += 2)
					*d++ = clut[*s&0x0f], *d++ = clut[*s++>>4];
		}
	}
}

void GSLocalMemory::unSwizzleTexture4HL(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) DWORD block[8*8];

	DWORD clut[16];
	for(int i = 0; i < 16; i++) clut[i] = SwapRB(m_clut[i]);

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 8, dst += 8*4)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 8; j < 8; j++, d += diff2)
				for(int i = 0; i < 8; i++)
					*d++ = clut[(*s++ >> 24)&0x0f];
		}
	}
}

void GSLocalMemory::unSwizzleTexture4HH(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8)
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

    DWORD bp = TEX0.TBP0;
	DWORD bw = TEX0.TBW;

	__declspec(align(16)) DWORD block[8*8];

	DWORD clut[16];
	for(int i = 0; i < 16; i++) clut[i] = SwapRB(m_clut[i]);

	for(int y = 0, diff = dstpitch*8 - tw*4; y < th; y += 8, dst += diff)
	{
		for(int x = 0; x < tw; x += 8, dst += 8*4)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst;

			for(int j = 0, diff2 = (dstpitch>>2) - 8; j < 8; j++, d += diff2)
				for(int i = 0; i < 8; i++)
					*d++ = clut[*s++ >> 28];
		}
	}
}

void GSLocalMemory::unSwizzleTextureX(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	readTexel rt = GetReadTexel(TEX0.PSM);

	for(int y = 0, diff = dstpitch - tw*4; y < th; y++, dst += diff)
		for(int x = 0; x < tw; x++, dst += 4)
			*(DWORD*)dst = SwapRB((this->*rt)(x, y, TEX0, TEXA));
}

GSLocalMemory::unSwizzleTexture GSLocalMemory::GetUnSwizzleTexture(DWORD psm)
{
	unSwizzleTexture st = NULL;

	switch(psm)
	{
	default: st = &GSLocalMemory::unSwizzleTextureX; break;
	case PSM_PSMCT32: st = &GSLocalMemory::unSwizzleTexture32; break;
	case PSM_PSMCT24: st = &GSLocalMemory::unSwizzleTexture24; break;
	case PSM_PSMCT16: st = &GSLocalMemory::unSwizzleTexture16; break;
	case PSM_PSMCT16S: st = &GSLocalMemory::unSwizzleTexture16S; break;
	case PSM_PSMT8: st = &GSLocalMemory::unSwizzleTexture8; break;
	case PSM_PSMT8H: st = &GSLocalMemory::unSwizzleTexture8H; break;
	case PSM_PSMT4: st = &GSLocalMemory::unSwizzleTexture4; break;
	case PSM_PSMT4HL: st = &GSLocalMemory::unSwizzleTexture4HL; break;
	case PSM_PSMT4HH: st = &GSLocalMemory::unSwizzleTexture4HH; break;
	}

	return st;
}

////////////////////

void GSLocalMemory::SwizzleBlock32(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask)
{
#if _M_IX86_FP >= 2
	if(WriteMask == 0xffffffff) __asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, srcpitch
		mov			ecx, 4

		align 16
SwizzleBlock32_loop:
		movaps		xmm0, [esi]
		movaps		xmm4, [esi+16]
		movaps		xmm1, [esi+edx]
		movaps		xmm5, [esi+edx+16]

		punpck(qdq, 0, 4, 1, 5, 2, 6);

		movaps		[edi+16*0], xmm0
		movaps		[edi+16*1], xmm2
		movaps		[edi+16*2], xmm4
		movaps		[edi+16*3], xmm6

		lea			esi, [esi+edx*2]
		add			edi, 64

		dec			ecx
		jnz			SwizzleBlock32_loop
	}
	else __asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, srcpitch
		mov			ecx, 4

		movd		xmm7, WriteMask
		pshufd		xmm7, xmm7, 0

		align 16
SwizzleBlock32WM_loop:
		movaps		xmm0, [esi]
		movaps		xmm4, [esi+16]
		movaps		xmm1, [esi+edx]
		movaps		xmm5, [esi+edx+16]

		punpck(qdq, 0, 4, 1, 5, 2, 6);

		movaps		xmm3, xmm7
		pshufd		xmm5, xmm7, 0xe4

		pandn		xmm3, [edi+16*0]
		pand		xmm0, xmm7
		por			xmm0, xmm3
		movaps		[edi+16*0], xmm0

		pandn		xmm5, [edi+16*1]
		pand		xmm2, xmm7
		por			xmm2, xmm5
		movaps		[edi+16*1], xmm2

		movaps		xmm3, xmm7
		pshufd		xmm5, xmm7, 0xe4

		pandn		xmm3, [edi+16*2]
		pand		xmm4, xmm7
		por			xmm4, xmm3
		movaps		[edi+16*2], xmm4

		pandn		xmm5, [edi+16*3]
		pand		xmm6, xmm7
		por			xmm6, xmm5
		movaps		[edi+16*3], xmm6

		lea			esi, [esi+edx*2]
		add			edi, 64

		dec			ecx
		jnz			SwizzleBlock32WM_loop
	}
#else
	WORD* d = &columnTable32[0][0];

	if(WriteMask == 0xffffffff)
	{
		for(int j = 0, diff = srcpitch - 8*4; j < 8; j++, src += diff)
			for(int i = 0; i < 8; i++, src += 4)
				((DWORD*)dst)[*d++] = *(DWORD*)src;
	}
	else
	{
		for(int j = 0, diff = srcpitch - 8*4; j < 8; j++, src += diff)
			for(int i = 0; i < 8; i++, src += 4, d++)
				((DWORD*)dst)[*d] = (((DWORD*)dst)[*d] & ~WriteMask) | (*(DWORD*)src & WriteMask);
	}

#endif
}

void GSLocalMemory::SwizzleBlock16(BYTE* dst, BYTE* src, int srcpitch)
{
/*
	WORD* tmp = &columnTable16[0][0];
    for(int j = 0; j < 8; j++)
	    for(int i = 0; i < 16; i++)
			((WORD*)src)[j*srcpitch/2+i] = *tmp++;
*/
#if _M_IX86_FP >= 2
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, srcpitch
		mov			ecx, 4

		align 16
SwizzleBlock16_loop:
		movaps		xmm0, [esi]
		movaps		xmm1, [esi+16]
		movaps		xmm2, [esi+edx]
		movaps		xmm3, [esi+edx+16]

		punpck(wd, 0, 2, 1, 3, 4, 6);
		punpck(qdq, 0, 4, 2, 6, 1, 5);

		movaps		[edi+16*0], xmm0
		movaps		[edi+16*1], xmm1
		movaps		[edi+16*2], xmm4
		movaps		[edi+16*3], xmm5

		lea			esi, [esi+edx*2]
		add			edi, 64

		dec			ecx
		jnz			SwizzleBlock16_loop
	}
#else
	WORD* d = &columnTable16[0][0];
	for(int j = 0, diff = srcpitch - 16*2; j < 8; j++, src += diff)
		for(int i = 0; i < 16; i++, src += 2)
			((WORD*)dst)[*d++] = *(WORD*)src;
#endif
}

void GSLocalMemory::SwizzleBlock8(BYTE* dst, BYTE* src, int srcpitch)
{
/*
	WORD* tmp = &columnTable8[0][0];
    for(int j = 0; j < 16; j++)
	    for(int i = 0; i < 16; i++)
			src[j*srcpitch+i] = *tmp++;
*/
#if _M_IX86_FP >= 2
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, srcpitch
		mov			ecx, 2

		align 16
SwizzleBlock8_loop:

		// col 0, 2

		movaps		xmm0, [esi]
		movaps		xmm2, [esi+edx]
		lea			esi, [esi+edx*2]

		pshufd		xmm1, [esi], 0xb1
		pshufd		xmm3, [esi+edx], 0xb1
		lea			esi, [esi+edx*2]

		punpck(bw, 0, 2, 1, 3, 4, 6);
		punpck(wd, 0, 2, 4, 6, 1, 3);
		punpck(qdq, 0, 1, 2, 3, 4, 5);

		movaps		[edi+16*0], xmm0
		movaps		[edi+16*1], xmm4
		movaps		[edi+16*2], xmm1
		movaps		[edi+16*3], xmm5

		// col 1, 3

		pshufd		xmm0, [esi], 0xb1
		pshufd		xmm2, [esi+edx], 0xb1
		lea			esi, [esi+edx*2]

		movaps		xmm1, [esi]
		movaps		xmm3, [esi+edx]
		lea			esi, [esi+edx*2]

		punpck(bw, 0, 2, 1, 3, 4, 6);
		punpck(wd, 0, 2, 4, 6, 1, 3);
		punpck(qdq, 0, 1, 2, 3, 4, 5);

		movaps		[edi+16*4], xmm0
		movaps		[edi+16*5], xmm4
		movaps		[edi+16*6], xmm1
		movaps		[edi+16*7], xmm5

		add			edi, 128

		dec			ecx
		jnz			SwizzleBlock8_loop
	}
#else
	WORD* d = &columnTable8[0][0];
	for(int j = 0, diff = srcpitch - 16; j < 16; j++, src += diff)
		for(int i = 0; i < 16; i++, src++)
			dst[*d++] = *src;
#endif
}

void GSLocalMemory::SwizzleBlock4(BYTE* dst, BYTE* src, int srcpitch)
{
#if _M_IX86_FP >= 2
	__asm
	{
		mov			esi, src
		mov			edi, dst
		mov			edx, srcpitch
		mov			ecx, 2

		mov         eax, 0x0f0f0f0f
		movd        xmm7, eax 
		pshufd      xmm7, xmm7, 0

		align 16
SwizzleBlock4_loop:

		// col 0, 2

		movaps		xmm0, [esi]
		movaps		xmm2, [esi+edx]
		lea			esi, [esi+edx*2]

		movaps		xmm1, [esi]
		movaps		xmm3, [esi+edx]
		lea			esi, [esi+edx*2]

		pshuflw		xmm1, xmm1, 0xb1
		pshuflw		xmm3, xmm3, 0xb1
		pshufhw		xmm1, xmm1, 0xb1
		pshufhw		xmm3, xmm3, 0xb1

		punpcknb
		punpck(bw, 0, 2, 4, 6, 1, 3);
		punpck(bw, 0, 2, 1, 3, 4, 6);
		punpck(qdq, 0, 4, 2, 6, 1, 3);

		movaps		[edi+16*0], xmm0
		movaps		[edi+16*1], xmm1
		movaps		[edi+16*2], xmm4
		movaps		[edi+16*3], xmm3

		// col 1, 3

		movaps		xmm0, [esi]
		movaps		xmm2, [esi+edx]
		lea			esi, [esi+edx*2]

		movaps		xmm1, [esi]
		movaps		xmm3, [esi+edx]
		lea			esi, [esi+edx*2]

		pshuflw		xmm0, xmm0, 0xb1
		pshuflw		xmm2, xmm2, 0xb1
		pshufhw		xmm0, xmm0, 0xb1
		pshufhw		xmm2, xmm2, 0xb1

		punpcknb
		punpck(bw, 0, 2, 4, 6, 1, 3);
		punpck(bw, 0, 2, 1, 3, 4, 6);
		punpck(qdq, 0, 4, 2, 6, 1, 3);

		movaps		[edi+16*4], xmm0
		movaps		[edi+16*5], xmm1
		movaps		[edi+16*6], xmm4
		movaps		[edi+16*7], xmm3

		add			edi, 128

		dec			ecx
		jnz			SwizzleBlock4_loop
	}
#else
	WORD* d = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, src += srcpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = *d++;
			BYTE c = (src[i>>1] >> ((i&1) << 2)) & 0x0f;
			int shift = (addr&1) << 2;
			dst[addr >> 1] = (dst[addr >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
#endif
}

///////////////////

static void SwizzleTextureStep(int& tx, int& ty, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
//	if(ty == TRXREG.RRH && tx == TRXPOS.DSAX) ASSERT(0);

	if(++tx == TRXREG.RRW)
	{
		tx = TRXPOS.DSAX;
		ty++;
	}
}

void GSLocalMemory::SwizzleTexture32(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw*4;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw*4; y < th; y += 8, src += diff)
			for(int x = 0; x < tw; x += 8, src += 8*4)
				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)src, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw*3;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw*3; y < th; y += 8, src += diff)
		{
			for(int x = 0; x < tw; x += 8, src += 8*3)
			{
				BYTE* s = src;
				DWORD* d = block;

				for(int j = 0, diff2 = srcpitch - 8*3; j < 8; j++, s += diff2)
					for(int i = 0; i < 8; i++, s += 3)
						*d++ = (s[2]<<16)|(s[1]<<8)|s[0];

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8, 0x00ffffff);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw*2;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw*2; y < th; y += 8, src += diff)
			for(int x = 0; x < tw; x += 16, src += 16*2)
				SwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, bp, bw)], (BYTE*)src, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16S(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw*2;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw*2; y < th; y += 8, src += diff)
			for(int x = 0; x < tw; x += 16, src += 16*2)
				SwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, bp, bw)], (BYTE*)src, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 15) || (tw & 15) || (th & 15) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty, diff = srcpitch*16 - tw; y < th; y += 16, src += diff)
			for(int x = 0; x < tw; x += 16, src += 16)
				SwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, bp, bw)], (BYTE*)src, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw; y < th; y += 8, src += diff)
		{
			for(int x = 0; x < tw; x += 8, src += 8)
			{
				BYTE* s = src;
				DWORD* d = block;

				for(int j = 0, diff2 = srcpitch - 8; j < 8; j++, s += diff2)
					for(int i = 0; i < 8; i++)
						*d++ = *s++ << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8, 0xff000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw/2;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 15) || (tw & 31) || (th & 15) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty, diff = srcpitch*16 - tw/2; y < th; y += 16, src += diff)
			for(int x = 0; x < tw; x += 32, src += 32/2)
				SwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, bp, bw)>>1], (BYTE*)src, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw/2;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw/2; y < th; y += 8, src += diff)
		{
			for(int x = 0; x < tw; x += 8, src += 8/2)
			{
				BYTE* s = src;
				DWORD* d = block;

				for(int j = 0, diff2 = srcpitch - 8/2; j < 8; j++, s += diff2)
					for(int i = 0; i < 8/2; i++, s++)
						*d++ = (*s&0x0f) << 24, *d++ = (*s&0xf0) << 20;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8, 0x0f000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	int tw = TRXREG.RRW, srcpitch = tw/2;
	int th = len / srcpitch;

	DWORD bp = BITBLTBUF.DBP;
	DWORD bw = BITBLTBUF.DBW;

	if(TRXPOS.DSAX || tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty, diff = srcpitch*8 - tw/2; y < th; y += 8, src += diff)
		{
			for(int x = 0; x < tw; x += 8, src += 8/2)
			{
				BYTE* s = src;
				DWORD* d = block;

				for(int j = 0, diff2 = srcpitch - 8/2; j < 8; j++, s += diff2)
					for(int i = 0; i < 8/2; i++, s++)
						*d++ = (*s&0x0f) << 28, *d++ = (*s&0xf0) << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, bp, bw)], (BYTE*)block, sizeof(block)/8, 0xf0000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTextureX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	BYTE* pb = (BYTE*)src;
	WORD* pw = (WORD*)src;
	DWORD* pd = (DWORD*)src;

	// if(ty >= (int)TRXREG.RRH) {ASSERT(0); return;}

	switch(BITBLTBUF.DPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pd++)
			writePixel32(tx, ty, *pd, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb+=3)
			writePixel24(tx, ty, *(DWORD*)pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16S(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel8(tx, ty, *pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel8H(tx, ty, *pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4HL(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4HL(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4HH(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4HH(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pd++)
			writePixel32Z(tx, ty, *pd, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb+=3)
			writePixel24Z(tx, ty, *(DWORD*)pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16Z(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16SZ(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	}
}

GSLocalMemory::SwizzleTexture GSLocalMemory::GetSwizzleTexture(DWORD psm)
{
	SwizzleTexture st = NULL;

	switch(psm)
	{
	default: st = &GSLocalMemory::SwizzleTextureX; break;
	case PSM_PSMCT32: st = &GSLocalMemory::SwizzleTexture32; break;
	case PSM_PSMCT24: st = &GSLocalMemory::SwizzleTexture24; break;
	case PSM_PSMCT16: st = &GSLocalMemory::SwizzleTexture16; break;
	case PSM_PSMCT16S: st = &GSLocalMemory::SwizzleTexture16S; break;
	case PSM_PSMT8: st = &GSLocalMemory::SwizzleTexture8; break;
	case PSM_PSMT8H: st = &GSLocalMemory::SwizzleTexture8H; break;
	case PSM_PSMT4: st = &GSLocalMemory::SwizzleTexture4; break;
	case PSM_PSMT4HL: st = &GSLocalMemory::SwizzleTexture4HL; break;
	case PSM_PSMT4HH: st = &GSLocalMemory::SwizzleTexture4HH; break;
	//case PSM_PSMZ32: st = &GSLocalMemory::SwizzleTexture32Z; break;
	//case PSM_PSMZ24: st = &GSLocalMemory::SwizzleTexture24Z; break;
	//case PSM_PSMZ16: st = &GSLocalMemory::SwizzleTexture16Z; break;
	//case PSM_PSMZ16S: st = &GSLocalMemory::SwizzleTexture16SZ; break;
	}

	return st;
}

///////////////////

WORD GSLocalMemory::blockTable32[4][8] = {
	{  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

WORD GSLocalMemory::blockTable32Z[4][8] = {
	{ 24, 25, 28, 29,  8,  9, 12, 13},
	{ 26, 27, 30, 31, 10, 11, 14, 15},
	{ 16, 17, 20, 21,  0,  1,  4,  5},
	{ 18, 19, 22, 23,  2,  3,  6,  7}
};

WORD GSLocalMemory::blockTable16[8][4] = {
	{  0,  2,  8, 10 },
	{  1,  3,  9, 11 },
	{  4,  6, 12, 14 },
	{  5,  7, 13, 15 },
	{ 16, 18, 24, 26 },
	{ 17, 19, 25, 27 },
	{ 20, 22, 28, 30 },
	{ 21, 23, 29, 31 }
};

WORD GSLocalMemory::blockTable16S[8][4] = {
	{  0,  2, 16, 18 },
	{  1,  3, 17, 19 },
	{  8, 10, 24, 26 },
	{  9, 11, 25, 27 },
	{  4,  6, 20, 22 },
	{  5,  7, 21, 23 },
	{ 12, 14, 28, 30 },
	{ 13, 15, 29, 31 }
};

WORD GSLocalMemory::blockTable16Z[8][4] = {
	{ 24, 26, 16, 18 },
	{ 25, 27, 17, 19 },
	{ 28, 30, 20, 22 },
	{ 29, 31, 21, 23 },
	{  8, 10,  0,  2 },
	{  9, 11,  1,  3 },
	{ 12, 14,  4,  6 },
	{ 13, 15,  5,  7 }
};

WORD GSLocalMemory::blockTable16SZ[8][4] = {
	{ 24, 26,  8, 10 },
	{ 25, 27,  9, 11 },
	{ 16, 18,  0,  2 },
	{ 17, 19,  1,  3 },
	{ 28, 30, 12, 14 },
	{ 29, 31, 13, 15 },
	{ 20, 22,  4,  6 },
	{ 21, 23,  5,  7 }
};

WORD GSLocalMemory::blockTable8[4][8] = {
	{  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

WORD GSLocalMemory::blockTable4[8][4] = {
	{  0,  2,  8, 10 },
	{  1,  3,  9, 11 },
	{  4,  6, 12, 14 },
	{  5,  7, 13, 15 },
	{ 16, 18, 24, 26 },
	{ 17, 19, 25, 27 },
	{ 20, 22, 28, 30 },
	{ 21, 23, 29, 31 }
};

WORD GSLocalMemory::columnTable32[8][8] = {
	{  0,  1,  4,  5,  8,  9, 12, 13 },
	{  2,  3,  6,  7, 10, 11, 14, 15 },
	{ 16, 17, 20, 21, 24, 25, 28, 29 },
	{ 18, 19, 22, 23, 26, 27, 30, 31 },
	{ 32, 33, 36, 37, 40, 41, 44, 45 },
	{ 34, 35, 38, 39, 42, 43, 46, 47 },
	{ 48, 49, 52, 53, 56, 57, 60, 61 },
	{ 50, 51, 54, 55, 58, 59, 62, 63 },
};

WORD GSLocalMemory::columnTable16[8][16] = {
	{   0,   2,   8,  10,  16,  18,  24,  26, 
	    1,   3,   9,  11,  17,  19,  25,  27 },
	{   4,   6,  12,  14,  20,  22,  28,  30, 
	    5,   7,  13,  15,  21,  23,  29,  31 },
	{  32,  34,  40,  42,  48,  50,  56,  58,
	   33,  35,  41,  43,  49,  51,  57,  59 },
	{  36,  38,  44,  46,  52,  54,  60,  62,
	   37,  39,  45,  47,  53,  55,  61,  63 },
	{  64,  66,  72,  74,  80,  82,  88,  90,
	   65,  67,  73,  75,  81,  83,  89,  91 },
	{  68,  70,  76,  78,  84,  86,  92,  94,
	   69,  71,  77,  79,  85,  87,  93,  95 },
	{  96,  98, 104, 106, 112, 114, 120, 122,
	   97,  99, 105, 107, 113, 115, 121, 123 },
	{ 100, 102, 108, 110, 116, 118, 124, 126,
	  101, 103, 109, 111, 117, 119, 125, 127 },
};

WORD GSLocalMemory::columnTable8[16][16] = {
	{   0,   4,  16,  20,  32,  36,  48,  52,	// column 0
	    2,   6,  18,  22,  34,  38,  50,  54 },
	{   8,  12,  24,  28,  40,  44,  56,  60,
	   10,  14,  26,  30,  42,  46,  58,  62 },
	{  33,  37,  49,  53,   1,   5,  17,  21,
	   35,  39,  51,  55,   3,   7,  19,  23 },
	{  41,  45,  57,  61,   9,  13,  25,  29,
	   43,  47,  59,  63,  11,  15,  27,  31 },
	{  96, 100, 112, 116,  64,  68,  80,  84, 	// column 1
	   98, 102, 114, 118,  66,  70,  82,  86 },
	{ 104, 108, 120, 124,  72,  76,  88,  92, 
	  106, 110, 122, 126,  74,  78,  90,  94 },
	{  65,  69,  81,  85,  97, 101, 113, 117,
	   67,  71,  83,  87,  99, 103, 115, 119 },
	{  73,  77,  89,  93, 105, 109, 121, 125,
	   75,  79,  91,  95, 107, 111, 123, 127 },
	{ 128, 132, 144, 148, 160, 164, 176, 180,	// column 2
	  130, 134, 146, 150, 162, 166, 178, 182 },
	{ 136, 140, 152, 156, 168, 172, 184, 188,
	  138, 142, 154, 158, 170, 174, 186, 190 },
	{ 161, 165, 177, 181, 129, 133, 145, 149,
	  163, 167, 179, 183, 131, 135, 147, 151 },
	{ 169, 173, 185, 189, 137, 141, 153, 157,
	  171, 175, 187, 191, 139, 143, 155, 159 },
	{ 224, 228, 240, 244, 192, 196, 208, 212,	// column 3
	  226, 230, 242, 246, 194, 198, 210, 214 },
	{ 232, 236, 248, 252, 200, 204, 216, 220,
	  234, 238, 250, 254, 202, 206, 218, 222 },
	{ 193, 197, 209, 213, 225, 229, 241, 245,
	  195, 199, 211, 215, 227, 231, 243, 247 },
	{ 201, 205, 217, 221, 233, 237, 249, 253,
	  203, 207, 219, 223, 235, 239, 251, 255 },
};

WORD GSLocalMemory::columnTable4[16][32] = {
	{   0,   8,  32,  40,  64,  72,  96, 104,	// column 0
	    2,  10,  34,  42,  66,  74,  98, 106,
	    4,  12,  36,  44,  68,  76, 100, 108,
	    6,  14,  38,  46,  70,  78, 102, 110 },
	{  16,  24,  48,  56,  80,  88, 112, 120,
	   18,  26,  50,  58,  82,  90, 114, 122,
	   20,  28,  52,  60,  84,  92, 116, 124,
	   22,  30,  54,  62,  86,  94, 118, 126 },
	{  65,  73,  97, 105,   1,   9,  33,  41,
	   67,  75,  99, 107,   3,  11,  35,  43,
	   69,  77, 101, 109,   5,  13,  37,  45, 
	   71,  79, 103, 111,   7,  15,  39,  47 },
	{  81,  89, 113, 121,  17,  25,  49,  57,
	   83,  91, 115, 123,  19,  27,  51,  59,
	   85,  93, 117, 125,  21,  29,  53,  61,
	   87,  95, 119, 127,  23,  31,  55,  63 },
	{ 192, 200, 224, 232, 128, 136, 160, 168,	// column 1
	  194, 202, 226, 234, 130, 138, 162, 170,
	  196, 204, 228, 236, 132, 140, 164, 172,
	  198, 206, 230, 238, 134, 142, 166, 174 },
	{ 208, 216, 240, 248, 144, 152, 176, 184,
	  210, 218, 242, 250, 146, 154, 178, 186,
	  212, 220, 244, 252, 148, 156, 180, 188,
	  214, 222, 246, 254, 150, 158, 182, 190 },
	{ 129, 137, 161, 169, 193, 201, 225, 233,
	  131, 139, 163, 171, 195, 203, 227, 235,
	  133, 141, 165, 173, 197, 205, 229, 237, 
	  135, 143, 167, 175, 199, 207, 231, 239 },
	{ 145, 153, 177, 185, 209, 217, 241, 249,
	  147, 155, 179, 187, 211, 219, 243, 251,
	  149, 157, 181, 189, 213, 221, 245, 253,
	  151, 159, 183, 191, 215, 223, 247, 255 },
	{ 256, 264, 288, 296, 320, 328, 352, 360,	// column 2
	  258, 266, 290, 298, 322, 330, 354, 362,
	  260, 268, 292, 300, 324, 332, 356, 364,
	  262, 270, 294, 302, 326, 334, 358, 366 },
	{ 272, 280, 304, 312, 336, 344, 368, 376,
	  274, 282, 306, 314, 338, 346, 370, 378,
	  276, 284, 308, 316, 340, 348, 372, 380,
	  278, 286, 310, 318, 342, 350, 374, 382 },
	{ 321, 329, 353, 361, 257, 265, 289, 297,
	  323, 331, 355, 363, 259, 267, 291, 299,
	  325, 333, 357, 365, 261, 269, 293, 301, 
	  327, 335, 359, 367, 263, 271, 295, 303 },
	{ 337, 345, 369, 377, 273, 281, 305, 313,
	  339, 347, 371, 379, 275, 283, 307, 315,
	  341, 349, 373, 381, 277, 285, 309, 317,
	  343, 351, 375, 383, 279, 287, 311, 319 },
	{ 448, 456, 480, 488, 384, 392, 416, 424,	// column 3
	  450, 458, 482, 490, 386, 394, 418, 426,
	  452, 460, 484, 492, 388, 396, 420, 428,
	  454, 462, 486, 494, 390, 398, 422, 430 },
	{ 464, 472, 496, 504, 400, 408, 432, 440,
	  466, 474, 498, 506, 402, 410, 434, 442,
	  468, 476, 500, 508, 404, 412, 436, 444,
	  470, 478, 502, 510, 406, 414, 438, 446 },
	{ 385, 393, 417, 425, 449, 457, 481, 489,
	  387, 395, 419, 427, 451, 459, 483, 491,
	  389, 397, 421, 429, 453, 461, 485, 493, 
	  391, 399, 423, 431, 455, 463, 487, 495 },
	{ 401, 409, 433, 441, 465, 473, 497, 505,
	  403, 411, 435, 443, 467, 475, 499, 507,
	  405, 413, 437, 445, 469, 477, 501, 509,
	  407, 415, 439, 447, 471, 479, 503, 511 },
};

WORD GSLocalMemory::pageOffset32[32][32][64];
WORD GSLocalMemory::pageOffset32Z[32][32][64];
WORD GSLocalMemory::pageOffset16[32][64][64];
WORD GSLocalMemory::pageOffset16S[32][64][64];
WORD GSLocalMemory::pageOffset16Z[32][64][64];
WORD GSLocalMemory::pageOffset16SZ[32][64][64];
WORD GSLocalMemory::pageOffset8[32][64][128];
WORD GSLocalMemory::pageOffset4[32][128][128];
