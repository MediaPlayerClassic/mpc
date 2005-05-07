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
#include "x86.h"

//

static class cdscd
{
public:
	cdscd()
	{
		return;
////
		GSLocalMemory lm;

		for(int i = 0; i < 1024*1024; i++)
			((DWORD*)lm.GetVM())[i] = rand()*0x12345678;

/*
		GIFRegTEX0 TEX0;
		TEX0.TBP0 = 0;
		TEX0.TBW = 16;
		GIFRegTEXA TEXA;
		TEXA.AEM = 0; // 1

		__declspec(align(32)) static DWORD dst[1024*1024];

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 1000; i++)
			{
				lm.unSwizzleTexture16(1024, 1024, (BYTE*)dst, 1024*4, TEX0, TEXA);
			}

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
*/
/*
		__declspec(align(16)) static vertex_t v[100000];
		for(int i = 0; i < countof(v); i++)
		{
			v[i].u = 1.0f*rand() - RAND_MAX/2;
			v[i].v = 1.0f*rand() - RAND_MAX/2;
		}

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			uvmm_t uvmm;
			for(int i = 0; i < 1000; i++)
				UVMinMax_sse2(countof(v), v, &uvmm);

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d, u %.2f %.2f, v %.2f %.2f"), diff, uvmm.umin, uvmm.umax, uvmm.vmin, uvmm.vmax);
			AfxMessageBox(str);
		}
*/
		GIFRegTEX0 TEX0;
		GIFRegTEXCLUT TEXCLUT;
		TEX0.PSM = PSM_PSMT8;
		TEX0.CLD = 1;
		TEX0.CSA = 0;
		TEX0.CPSM = PSM_PSMCT32; // PSM_PSMCT16S
		TEX0.CBP = 0;
		TEX0.CSM = 0; // 0
		TEXCLUT.CBW = 1;
		TEXCLUT.COU = 0;
		TEXCLUT.COV = 0;

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 10000000; i++)
				lm.writeCLUT(TEX0, TEXCLUT);

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
	}
} sddscsd;

//

__forceinline static DWORD From24To32(DWORD c, BYTE TCC, GIFRegTEXA& TEXA)
{
	BYTE A = (!TEXA.AEM|(c&0xffffff)) ? TEXA.TA0 : 0;
	return (A<<24) | (c&0xffffff);
}

__forceinline static DWORD From16To32(WORD c, GIFRegTEXA& TEXA)
{
	BYTE A = (c&0x8000) ? TEXA.TA1 : (!TEXA.AEM|(c&0x7fff)) ? TEXA.TA0 : 0;
	return (A << 24) | ((c&0x7c00) << 9) | ((c&0x03e0) << 6) | ((c&0x001f) << 3);
}

//

WORD GSLocalMemory::pageOffset32[32][32][64];
WORD GSLocalMemory::pageOffset32Z[32][32][64];
WORD GSLocalMemory::pageOffset16[32][64][64];
WORD GSLocalMemory::pageOffset16S[32][64][64];
WORD GSLocalMemory::pageOffset16Z[32][64][64];
WORD GSLocalMemory::pageOffset16SZ[32][64][64];
WORD GSLocalMemory::pageOffset8[32][64][128];
WORD GSLocalMemory::pageOffset4[32][128][128];

//

GSLocalMemory::GSLocalMemory()
{
	int len = 1024*1024*4*2; // *2 for safety...

	m_vm8 = (BYTE*)_aligned_malloc(len, 16);
	memset(m_vm8, 0, len);

	m_pCLUT = (WORD*)_aligned_malloc(256*2*sizeof(WORD)*2, 16);
	m_pCLUT32 = (DWORD*)_aligned_malloc(256*sizeof(DWORD), 16);

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

	m_fCLUTMayBeDirty = true;
}

GSLocalMemory::~GSLocalMemory()
{
	_aligned_free(m_vm8);
	_aligned_free(m_pCLUT);
	_aligned_free(m_pCLUT32);
}

////////////////////

CSize GSLocalMemory::GetBlockSize(DWORD PSM)
{
	CSize size;

	switch(PSM)
	{
	default:
	case PSM_PSMCT32: 
	case PSM_PSMCT24:
	case PSM_PSMT8H: 
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
	case PSM_PSMZ32:
	case PSM_PSMZ24:
		size.SetSize(8, 8); 
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:
		size.SetSize(16, 8); 
		break;
	case PSM_PSMT8: 
		size.SetSize(16, 16);
		break;
	case PSM_PSMT4:
		size.SetSize(32, 16);
		break;
	}

	return size;
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
	default: //ASSERT(0);
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
	TRACE(_T("%4d\n"), pixelAddress32(x, y, bp, bw));
	return m_vm32[pixelAddress32(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(int x, int y, DWORD bp, DWORD bw)
{
	TRACE(_T("%4d\n"), pixelAddress16(x, y, bp, bw));
	return (DWORD)m_vm16[pixelAddress16(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16S(int x, int y, DWORD bp, DWORD bw)
{
	TRACE(_T("%4d\n"), pixelAddress16S(x, y, bp, bw));
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
	return From24To32(m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)], TEX0.ai32[1]&4, TEXA);
}

DWORD GSLocalMemory::readTexel16(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[pixelAddress16(x, y, TEX0.TBP0, TEX0.TBW)], TEXA);
}

DWORD GSLocalMemory::readTexel16S(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[pixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], TEXA);
}

DWORD GSLocalMemory::readTexel8(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel8H(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8H(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HL(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HH(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW)];
}

GSLocalMemory::readTexel GSLocalMemory::GetReadTexel(DWORD psm)
{
	readTexel rt = NULL;

	switch(psm)
	{
	default: ;//ASSERT(0);
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

DWORD GSLocalMemory::readTexel32P(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel32(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel16P(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel16(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel16SP(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel16S(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel8P(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel8(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel8HP(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel8H(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4P(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel4(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4HLP(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4HHP(int x, int y, GIFRegTEX0& TEX0)
{
	return readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW);
}

GSLocalMemory::readTexelP GSLocalMemory::GetReadTexelP(DWORD psm)
{
	readTexelP rt = NULL;

	switch(psm)
	{
	default: ;//ASSERT(0);
	case PSM_PSMCT32: rt = &GSLocalMemory::readTexel32P; break;
	case PSM_PSMCT24: rt = &GSLocalMemory::readTexel32P; break;
	case PSM_PSMCT16: rt = &GSLocalMemory::readTexel16P; break;
	case PSM_PSMCT16S: rt = &GSLocalMemory::readTexel16SP; break;
	case PSM_PSMT8: rt = &GSLocalMemory::readTexel8P; break;
	case PSM_PSMT4: rt = &GSLocalMemory::readTexel4P; break;
	case PSM_PSMT8H: rt = &GSLocalMemory::readTexel8HP; break;
	case PSM_PSMT4HL: rt = &GSLocalMemory::readTexel4HLP; break;
	case PSM_PSMT4HH: rt = &GSLocalMemory::readTexel4HHP; break;
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
	default: //ASSERT(0);
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
	return From24To32(m_vm32[addr], TEX0.ai32[1]&4, TEXA);
}

DWORD GSLocalMemory::readTexel16(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[addr], TEXA);
}

DWORD GSLocalMemory::readTexel16S(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[addr], TEXA);
}

DWORD GSLocalMemory::readTexel8(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8(x, y, addr)];
}

DWORD GSLocalMemory::readTexel8H(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8H(x, y, addr)];
}

DWORD GSLocalMemory::readTexel4(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4(x, y, addr)];
}

DWORD GSLocalMemory::readTexel4HL(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HL(x, y, addr)];
}

DWORD GSLocalMemory::readTexel4HH(int x, int y, DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HH(x, y, addr)];
}

GSLocalMemory::readTexelAddr GSLocalMemory::GetReadTexelAddr(DWORD psm)
{
	readTexelAddr rt = NULL;

	switch(psm)
	{
	default: //ASSERT(0);
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

void GSLocalMemory::writeCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT)
{
	switch(TEX0.CLD)
	{
	default:
	case 0: return;
	case 1: break;
	case 2: m_CBP[0] = TEX0.CBP; break;
	case 3: m_CBP[1] = TEX0.CBP; break;
	case 4: if(m_CBP[0] == TEX0.CBP) return; break;
	case 5: if(m_CBP[1] == TEX0.CBP) return; break;
	}

/*
	if(!m_fCLUTMayBeDirty && m_prevTEX0.i64 == TEX0.i64 && m_prevTEXCLUT.i64 == TEXCLUT.i64)
		return;
*/
	m_prevTEX0 = TEX0;
	m_prevTEXCLUT = TEXCLUT;

	m_fCLUTMayBeDirty = false;


	readPixel rp = GetReadPixel(TEX0.CPSM);

	DWORD bp = TEX0.CBP;
	DWORD bw = TEX0.CSM == 0 ? 1 : TEXCLUT.CBW;

	WORD* CLUTLW = m_pCLUT + (TEX0.CSA<<4);
	WORD* CLUTHW = CLUTLW + 256;

	if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
	{
		if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			ASSERT(TEX0.CSA <= 16);

//			WORD* CLUTLW = m_pCLUT + (min(TEX0.CSA, 16) << 4);

			if(TEX0.CSM == 0)
			{
#if 1 && (defined(_M_AMD64) || _M_IX86_FP >= 2)

				__m128i* src = (__m128i*)&m_vm16[TEX0.CPSM == PSM_PSMCT16 ? blockAddress16(0, 0, bp, bw) : blockAddress16S(0, 0, bp, bw)];
				__m128i* dst = (__m128i*)CLUTLW;

				for(int i = 0; i < 32; i += 4)
				{
					__m128i r0 = _mm_load_si128(&src[i+0]);
					__m128i r1 = _mm_load_si128(&src[i+1]);
					__m128i r2 = _mm_load_si128(&src[i+2]);
					__m128i r3 = _mm_load_si128(&src[i+3]);

					__m128i r4 = _mm_unpacklo_epi16(r0, r1);
					__m128i r5 = _mm_unpackhi_epi16(r0, r1);
					__m128i r6 = _mm_unpacklo_epi16(r2, r3);
					__m128i r7 = _mm_unpackhi_epi16(r2, r3);

					r0 = _mm_unpacklo_epi32(r4, r6);
					r1 = _mm_unpackhi_epi32(r4, r6);
					r2 = _mm_unpacklo_epi32(r5, r7);
					r3 = _mm_unpackhi_epi32(r5, r7);

					r4 = _mm_unpacklo_epi16(r0, r1);
					r5 = _mm_unpackhi_epi16(r0, r1);
					r6 = _mm_unpacklo_epi16(r2, r3);
					r7 = _mm_unpackhi_epi16(r2, r3);

					_mm_store_si128(&dst[i+0], r4);
					_mm_store_si128(&dst[i+1], r6);
					_mm_store_si128(&dst[i+2], r5);
					_mm_store_si128(&dst[i+3], r7);
				}
#else
/*
				for(int y = 0, cy = 0; y < 8; y++, CLUTLW += 32)
				{
					int i = 0;
					for(int x = 0; x < 8; x++, i++) {CLUTLW[x] = (WORD)(this->*rp)(i, cy, bp, bw);}
					for(int x = 16; x < 24; x++, i++) {CLUTLW[x] = (WORD)(this->*rp)(i, cy, bp, bw);}
					cy++;
					i = 0;
					for(int x = 8; x < 16; x++, i++) {CLUTLW[x] = (WORD)(this->*rp)(i, cy, bp, bw);}
					for(int x = 24; x < 32; x++, i++) {CLUTLW[x] = (WORD)(this->*rp)(i, cy, bp, bw);}
					cy++;
				}
*/
				static DWORD map[] = 
				{
					0, 2, 8, 10, 16, 18, 24, 26,
					4, 6, 12, 14, 20, 22, 28, 30,
					1, 3, 9, 11, 17, 19, 25, 27, 
					5, 7, 13, 15, 21, 23, 29, 31
				};

				WORD* ptr = &m_vm16[TEX0.CPSM == PSM_PSMCT16 ? blockAddress16(0, 0, bp, bw) : blockAddress16S(0, 0, bp, bw)];

				for(int j = 0; j < 8; j++, CLUTLW += 32, ptr += 32) 
					for(int i = 0; i < 32; i++) 
						CLUTLW[i] = ptr[map[i]];

#endif
			}
			else
			{
				for(int i = 0; i < 256; i++)
				{
					CLUTLW[i] = (WORD)(this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
				}
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT32)
		{
			ASSERT(TEX0.CSA == 0);

//			WORD* CLUTLW = m_pCLUT;
//			WORD* CLUTHW = CLUTLW + 256;

			if(TEX0.CSM == 0)
			{
#if 1 && (defined(_M_AMD64) || _M_IX86_FP >= 2)

				__declspec(align(16)) DWORD tmp[256];

				__m128i* src = (__m128i*)&m_vm32[blockAddress32(0, 0, bp, bw)];
				__m128i* dst = (__m128i*)tmp; // TODO

				for(int j = 0; j < 64; j += 32, src += 32, dst += 32)
				{
					for(int i = 0; i < 16; i += 4)
					{
						__m128i r0 = _mm_load_si128(&src[i+0]);
						__m128i r1 = _mm_load_si128(&src[i+1]);
						__m128i r2 = _mm_load_si128(&src[i+2]);
						__m128i r3 = _mm_load_si128(&src[i+3]);

						_mm_store_si128(&dst[i*2+0], _mm_unpacklo_epi64(r0, r1));
						_mm_store_si128(&dst[i*2+1], _mm_unpacklo_epi64(r2, r3));
						_mm_store_si128(&dst[i*2+2], _mm_unpackhi_epi64(r0, r1));
						_mm_store_si128(&dst[i*2+3], _mm_unpackhi_epi64(r2, r3));

						__m128i r4 = _mm_load_si128(&src[i+0+16]);
						__m128i r5 = _mm_load_si128(&src[i+1+16]);
						__m128i r6 = _mm_load_si128(&src[i+2+16]);
						__m128i r7 = _mm_load_si128(&src[i+3+16]);

						_mm_store_si128(&dst[i*2+4], _mm_unpacklo_epi64(r4, r5));
						_mm_store_si128(&dst[i*2+5], _mm_unpacklo_epi64(r6, r7));
						_mm_store_si128(&dst[i*2+6], _mm_unpackhi_epi64(r4, r5));
						_mm_store_si128(&dst[i*2+7], _mm_unpackhi_epi64(r6, r7));
					}
				}

				for(int i = 0; i < 32; i++)
				{
					__m128i r1 = ((__m128i*)tmp)[i*2];
					__m128i r2 = ((__m128i*)tmp)[i*2+1];
					__m128i r3 = _mm_unpacklo_epi16(r1, r2);
					__m128i r4 = _mm_unpackhi_epi16(r1, r2);
					r1 = _mm_unpacklo_epi16(r3, r4);
					r2 = _mm_unpackhi_epi16(r3, r4);
					r3 = _mm_unpacklo_epi16(r1, r2);
					r4 = _mm_unpackhi_epi16(r1, r2);
					_mm_store_si128(&((__m128i*)CLUTLW)[i], r3);
					_mm_store_si128(&((__m128i*)CLUTHW)[i], r4);
				}
#else
/*
				for(int y = 0, cy = 0; y < 8; y++, CLUTLW += 32, CLUTHW += 32)
				{
					int i = 0;
					for(int x = 0; x < 8; x++, i++) {DWORD dw = (this->*rp)(i, cy, bp, bw); CLUTLW[x] = (WORD)(dw & 0xffff); CLUTHW[x] = (WORD)(dw >> 16);}
					for(int x = 16; x < 24; x++, i++) {DWORD dw = (this->*rp)(i, cy, bp, bw); CLUTLW[x] = (WORD)(dw & 0xffff); CLUTHW[x] = (WORD)(dw >> 16);}
					cy++;
					i = 0;
					for(int x = 8; x < 16; x++, i++) {DWORD dw = (this->*rp)(i, cy, bp, bw); CLUTLW[x] = (WORD)(dw & 0xffff); CLUTHW[x] = (WORD)(dw >> 16);}
					for(int x = 24; x < 32; x++, i++) {DWORD dw = (this->*rp)(i, cy, bp, bw); CLUTLW[x] = (WORD)(dw & 0xffff); CLUTHW[x] = (WORD)(dw >> 16);}
					cy++;
				}
*/
				static DWORD map[] = 
				{
					0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15, 
					64, 65, 68, 69, 72, 73, 76, 77, 66, 67, 70, 71, 74, 75, 78, 79, 
					16, 17, 20, 21, 24, 25, 28, 29, 18, 19, 22, 23, 26, 27, 30, 31, 
					80, 81, 84, 85, 88, 89, 92, 93, 82, 83, 86, 87, 90, 91, 94, 95, 
					32, 33, 36, 37, 40, 41, 44, 45, 34, 35, 38, 39, 42, 43, 46, 47, 
					96, 97, 100, 101, 104, 105, 108, 109, 98, 99, 102, 103, 106, 107, 110, 111, 
					48, 49, 52, 53, 56, 57, 60, 61, 50, 51, 54, 55, 58, 59, 62, 63, 
					112, 113, 116, 117, 120, 121, 124, 125, 114, 115, 118, 119, 122, 123, 126, 127
				};

				DWORD* ptr = &m_vm32[blockAddress32(0, 0, bp, bw)];

				for(int j = 0; j < 2; j++, CLUTLW += 128, CLUTHW += 128, ptr += 128)
				{
					for(int i = 0; i < 128; i++) 
					{
						DWORD dw = ptr[map[i]];
						CLUTLW[i] = (WORD)(dw & 0xffff);
						CLUTHW[i] = (WORD)(dw >> 16);
					}
				}
#endif
			}
			else
			{
				for(int i = 0; i < 256; i++)
				{
					DWORD dw = (this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
					CLUTLW[i] = (WORD)(dw & 0xffff);
					CLUTHW[i] = (WORD)(dw >> 16);
				}
			}
		}
	}
	else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
	{
		if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			ASSERT(TEX0.CSA <= 31);

//			WORD* CLUTLW = m_pCLUT + (min(TEX0.CSA, 32) << 4);

			if(TEX0.CSM == 0)
			{
				// TODO: sse2
/*
				for(int y = 0; y < 2; y++, CLUTLW += 8)
				{
					for(int x = 0; x < 8; x++)
					{
						CLUTLW[x] = (WORD)(this->*rp)(x, y, bp, bw);
					}
				}
*/
				static DWORD map[] = 
				{
					0, 2, 8, 10, 16, 18, 24, 26,
					4, 6, 12, 14, 20, 22, 28, 30
				};

				WORD* ptr = &m_vm16[TEX0.CPSM == PSM_PSMCT16 ? blockAddress16(0, 0, bp, bw) : blockAddress16S(0, 0, bp, bw)];

				for(int i = 0; i < 16; i++) 
				{
					CLUTLW[i] = ptr[map[i]];
				}
			}
			else
			{
				for(int i = 0; i < 16; i++)
				{
					CLUTLW[i] = (WORD)(this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
				}
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT32)
		{
			ASSERT(TEX0.CSA <= 15);

//			WORD* CLUTLW = m_pCLUT + (min(TEX0.CSA, 15) << 4);
//			WORD* CLUTHW = CLUTLW + 256;

			if(TEX0.CSM == 0)
			{
#if 1 && (defined(_M_AMD64) || _M_IX86_FP >= 2)

				__declspec(align(16)) DWORD tmp[16];

				__m128i* src = (__m128i*)&m_vm32[blockAddress32(0, 0, bp, bw)];
				__m128i* dst = (__m128i*)tmp; // TODO

				__m128i r0 = _mm_load_si128(&src[0]);
				__m128i r1 = _mm_load_si128(&src[1]);
				__m128i r2 = _mm_load_si128(&src[2]);
				__m128i r3 = _mm_load_si128(&src[3]);

				_mm_store_si128(&dst[0], _mm_unpacklo_epi64(r0, r1));
				_mm_store_si128(&dst[1], _mm_unpacklo_epi64(r2, r3));
				_mm_store_si128(&dst[2], _mm_unpackhi_epi64(r0, r1));
				_mm_store_si128(&dst[3], _mm_unpackhi_epi64(r2, r3));

				for(int i = 0; i < 2; i++)
				{
					__m128i r1 = ((__m128i*)tmp)[i*2];
					__m128i r2 = ((__m128i*)tmp)[i*2+1];
					__m128i r3 = _mm_unpacklo_epi16(r1, r2);
					__m128i r4 = _mm_unpackhi_epi16(r1, r2);
					r1 = _mm_unpacklo_epi16(r3, r4);
					r2 = _mm_unpackhi_epi16(r3, r4);
					r3 = _mm_unpacklo_epi16(r1, r2);
					r4 = _mm_unpackhi_epi16(r1, r2);
					_mm_store_si128(&((__m128i*)CLUTLW)[i], r3);
					_mm_store_si128(&((__m128i*)CLUTHW)[i], r4);
				}
#else
/*
				for(int y = 0; y < 2; y++, CLUTLW += 8, CLUTHW += 8)
				{
					for(int x = 0; x < 8; x++)
					{
						DWORD dw = (this->*rp)(x, y, bp, bw);
						CLUTLW[x] = (WORD)(dw & 0xffff);
						CLUTHW[x] = (WORD)(dw >> 16);
					}
				}
*/
				static DWORD map[] = 
				{
					0, 1, 4, 5, 8, 9, 12, 13,
					2, 3, 6, 7, 10, 11, 14, 15
				};

				DWORD* ptr = &m_vm32[blockAddress32(0, 0, bp, bw)];

				for(int i = 0; i < 16; i++) 
				{
					DWORD dw = ptr[map[i]];
					CLUTLW[i] = (WORD)(dw & 0xffff);
					CLUTHW[i] = (WORD)(dw >> 16);
				}
#endif
			}
			else
			{
				for(int i = 0; i < 16; i++)
				{
					DWORD dw = (this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
					CLUTLW[i] = (WORD)(dw & 0xffff);
					CLUTHW[i] = (WORD)(dw >> 16);
				}
			}
		}
	}
}

void GSLocalMemory::readCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT)
{
	// FIXME: 16-bit palette (what if TEXA is changed after TEX0/2?)

	ASSERT(pCLUT);

	WORD* CLUTLW = m_pCLUT + (TEX0.CSA<<4);
	WORD* CLUTHW = CLUTLW + 256;

	int nPaletteEntries = PaletteEntries(TEX0.PSM);

	if(TEX0.CPSM == PSM_PSMCT32)
	{
#if 1 && (defined(_M_AMD64) || _M_IX86_FP >= 2)
		nPaletteEntries >>= 3;

		for(int i = 0; i < nPaletteEntries; i++)
		{
			__m128i rl = ((__m128i*)CLUTLW)[i];
			__m128i rh = ((__m128i*)CLUTHW)[i];
			_mm_store_si128(&((__m128i*)pCLUT)[i*2], _mm_unpacklo_epi16(rl, rh));
			_mm_store_si128(&((__m128i*)pCLUT)[i*2+1], _mm_unpackhi_epi16(rl, rh));
		}
#else
		for(int i = 0; i < nPaletteEntries; i++)
		{
			pCLUT[i] = ((DWORD)CLUTHW[i] << 16) | CLUTLW[i];
		}
#endif
	}
	else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
	{
		Expand16(CLUTLW, pCLUT, nPaletteEntries, &TEXA);
/*
		for(int i = 0; i < nPaletteEntries; i++)
		{
			pCLUT[i] = From16To32(CLUTLW[i], TEXA);
		}
*/
	}
}

void GSLocalMemory::setupCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA)
{
	readCLUT(TEX0, TEXA, m_pCLUT32);
}

void GSLocalMemory::getCLUT(DWORD* pCLUT, int nPaletteEntries)
{
	memcpy(pCLUT, m_pCLUT32, sizeof(DWORD)*nPaletteEntries);
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
		for(int ys = y, ye = y + h; ys < ye; ys++)
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
			for(int x = clip.left; x < clip.right; x += w)
				memsetd(&m_vm8[(this->*pa)(x, y, fbp, fbw) << 2 >> shift], c, dwords);
	}

	for(int y = clip.bottom; y < r.bottom; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	return(true);
}

////////////////////

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
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*4;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 7) || (tx & 7) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		if((TRXPOS.DSAX & 7) == 0 && (tx & 7) == 0 && TRXPOS.DSAX == tx && (ty & 7) == 0 && tw >= 8 && th >= 8)
		{
			len -= (th & ~7) * srcpitch;

			th += ty;

			int tw2 = tw & ~7;
			int th2 = th & ~7;

			for(int y = ty; y < th2; )
			{
				for(int x = tx; x < tw2; x += 8)
					SwizzleBlock32_c((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);

				for(int ye = y + 8; y < ye; y++, src += srcpitch)
					for(int x = tw2; x < tw; x++)
						writePixel32(x, y, ((DWORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = th2;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
			for(int x = tx; x < tw; x += 8)
				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*3;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 7) || (tx & 7) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)*3;
				DWORD* d = block;

				for(int j = 0, diff = srcpitch - 8*3; j < 8; j++, s += diff, d += 8)
					for(int i = 0; i < 8; i++, s += 3)
						d[i] = (s[2]<<16)|(s[1]<<8)|s[0];

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x00ffffff);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*2;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 15) || (tx & 15) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		if((TRXPOS.DSAX & 15) == 0 && (tx & 15) == 0 && TRXPOS.DSAX == tx && (ty & 7) == 0 && tw >= 16 && th >= 8)
		{
			len -= (th & ~7) * srcpitch;

			th += ty;

			int tw2 = tw & ~15;
			int th2 = th & ~7;

			for(int y = ty; y < th2; )
			{
				for(int x = tx; x < tw2; x += 16)
					SwizzleBlock16_c((BYTE*)&m_vm16[blockAddress16(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int ye = y + 8; y < ye; y++, src += srcpitch)
					for(int x = tw2; x < tw; x++)
						writePixel16(x, y, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = th2;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
			for(int x = tx; x < tw; x += 16)
				SwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16S(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*2;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 15) || (tx & 15) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		if((TRXPOS.DSAX & 15) == 0 && (tx & 15) == 0 && TRXPOS.DSAX == tx && (ty & 7) == 0 && tw >= 16 && th >= 8)
		{
			len -= (th & ~7) * srcpitch;

			th += ty;

			int tw2 = tw & ~15;
			int th2 = th & ~7;

			for(int y = ty; y < th2; )
			{
				for(int x = tx; x < tw2; x += 16)
					SwizzleBlock16_c((BYTE*)&m_vm16[blockAddress16S(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int ye = y + 8; y < ye; y++, src += srcpitch)
					for(int x = tw2; x < tw; x++)
						writePixel16S(x, y, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = th2;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
			for(int x = tx; x < tw; x += 16)
				SwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 15) || (tx & 15) || TRXPOS.DSAX != tx || (ty & 15) || (tw & 15) || (th & 15) || (len % srcpitch))
	{
		if((TRXPOS.DSAX & 15) == 0 && (tx & 15) == 0 && TRXPOS.DSAX == tx && (ty & 15) == 0 && tw >= 16 && th >= 16)
		{
			len -= (th & ~15) * srcpitch;

			th += ty;

			int tw2 = tw & ~15;
			int th2 = th & ~15;

			for(int y = ty; y < th2; )
			{
				for(int x = tx; x < tw2; x += 16)
					SwizzleBlock8_c((BYTE*)&m_vm8[blockAddress8(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);

				for(int ye = y + 16; y < ye; y++, src += srcpitch)
					for(int x = tw2; x < tw; x++)
						writePixel8(x, y, src[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = th2;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 16, src += srcpitch*16)
			for(int x = tx; x < tw; x += 16)
				SwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 7) || (tx & 7) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx);
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8; i++)
						d[i] = s[i] << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0xff000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 31) || (tx & 31) || TRXPOS.DSAX != tx || (ty & 15) || (tw & 31) || (th & 15) || (len % srcpitch))
	{
		if((TRXPOS.DSAX & 31) == 0 && (tx & 31) == 0 && TRXPOS.DSAX == tx && (ty & 15) == 0 && tw >= 32 && th >= 16)
		{
			len -= (th & ~15) * srcpitch;

			th += ty;

			int tw2 = tw & ~31;
			int th2 = th & ~15;

			for(int y = ty; y < th2; )
			{
				for(int x = tx; x < tw2; x += 32)
					SwizzleBlock4_c((BYTE*)&m_vm8[blockAddress4(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);

				for(int ye = y + 16; y < ye; y++, src += srcpitch)
				{
					BYTE* s = src + (tw2 - tx)/2;

					for(int x = tw2; x < tw; x += 2, s++)
					{
						writePixel4(x, y, *s&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
						writePixel4(x+1, y, *s>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
					}
				}
			}

			ty = th2;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 16, src += srcpitch*16)
			for(int x = tx; x < tw; x += 32)
				SwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 7) || (tx & 7) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)/2;
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8/2; i++)
						d[i*2] = (s[i]&0x0f) << 24, 
						d[i*2+1] = (s[i]&0xf0) << 20;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x0f000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	if((TRXPOS.DSAX & 7) || (tx & 7) || TRXPOS.DSAX != tx || (ty & 7) || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)/2;
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8/2; i++)
						d[i*2] = (s[i]&0x0f) << 28, 
						d[i*2+1] = (s[i]&0xf0) << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x0f000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTextureX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(len <= 0) return;

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

void GSLocalMemory::unSwizzleTexture32(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
		for(int x = 0; x < tw; x += 8)
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + x*4, dstpitch);
}

void GSLocalMemory::unSwizzleTexture24(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	BYTE TCC = TEX0.TCC;

	__declspec(align(16)) DWORD block[8*8];

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
	{
		for(int x = 0; x < tw; x += 8)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock24(block, (DWORD*)dst + x, dstpitch, &TEXA);
		}
	}
}

void GSLocalMemory::unSwizzleTexture16(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 16 || th < 8 || (tw & 15) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) WORD block[16*8];

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
	{
		for(int x = 0; x < tw; x += 16)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock16(block, (DWORD*)dst + x, dstpitch, &TEXA);
		}
	}
}

void GSLocalMemory::unSwizzleTexture16S(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 16 || th < 8 || (tw & 15) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) WORD block[16*8];

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
	{
		for(int x = 0; x < tw; x += 16)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock16(block, (DWORD*)dst + x, dstpitch, &TEXA);
		}
	}
}

void GSLocalMemory::unSwizzleTexture8(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 16 || th < 16 || (tw & 15) || (th & 15))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) BYTE block[16*16];

	for(int y = 0; y < th; y += 16, dst += dstpitch*16)
	{
		for(int x = 0; x < tw; x += 16)
		{
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/16);

            BYTE* s = block;
			DWORD* d = (DWORD*)dst + x;

			for(int j = 0; j < 16; j++, s += 16, d += dstpitch>>2)
				for(int i = 0; i < 16; i++)
					d[i] = m_pCLUT32[s[i]];
		}
	}
}

void GSLocalMemory::unSwizzleTexture8H(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) DWORD block[8*8];

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
	{
		for(int x = 0; x < tw; x += 8)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst + x;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch>>2)
				for(int i = 0; i < 8; i++)
					d[i] = m_pCLUT32[s[i] >> 24];
		}
	}
}

void GSLocalMemory::unSwizzleTexture4(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 32 || th < 16 || (tw & 31) || (th & 15))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) BYTE block[(32/2)*16];

	for(int y = 0; y < th; y += 16, dst += dstpitch*16)
	{
		for(int x = 0; x < tw; x += 32)
		{
			unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

            BYTE* s = block;
			DWORD* d = (DWORD*)dst + x;

			for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch>>2)
				for(int i = 0; i < 32/2; i++)
					d[i*2] = m_pCLUT32[s[i]&0x0f],
					d[i*2+1] = m_pCLUT32[s[i]>>4];
		}
	}
}

void GSLocalMemory::unSwizzleTexture4HL(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) DWORD block[8*8];

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
	{
		for(int x = 0; x < tw; x += 8)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst + x;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch>>2)
				for(int i = 0; i < 8; i++)
					d[i] = m_pCLUT32[(s[i] >> 24)&0x0f];
		}
	}
}

void GSLocalMemory::unSwizzleTexture4HH(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureX(tw, th, dst, dstpitch, TEX0, TEXA);
		return;
	}

	__declspec(align(16)) DWORD block[8*8];

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
	{
		for(int x = 0; x < tw; x += 8)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

            DWORD* s = block;
			DWORD* d = (DWORD*)dst + x;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch>>2)
				for(int i = 0; i < 8; i++)
					d[i] = m_pCLUT32[s[i] >> 28];
		}
	}
}

void GSLocalMemory::unSwizzleTextureX(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	readTexel rt = GetReadTexel(TEX0.PSM);

	for(int y = 0; y < th; y++, dst += dstpitch)
		for(int x = 0; x < tw; x++)
			((DWORD*)dst)[x] = (this->*rt)(x, y, TEX0, TEXA);
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

///////////////////

void GSLocalMemory::ReadTexture(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP)
{
	if((CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		ReadTextureClamp<DWORD>(dst, dstpitch, TEX0, TEXA, CLAMP);
	}
	else
	{
		unSwizzleTexture st = GetUnSwizzleTexture(TEX0.PSM);
		(this->*st)(tw, th, dst, dstpitch, TEX0, TEXA);
	}
}

///////////////////

void GSLocalMemory::unSwizzleTexture32P(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
		for(int x = 0; x < tw; x += 8)
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + x*4, dstpitch);
}

void GSLocalMemory::unSwizzleTexture16P(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 16 || th < 8 || (tw & 15) || (th & 7))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
		for(int x = 0; x < tw; x += 16)
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], dst + x*2, dstpitch);
}

void GSLocalMemory::unSwizzleTexture16SP(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 16 || th < 8 || (tw & 15) || (th & 7))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
		for(int x = 0; x < tw; x += 16)
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], dst + x*2, dstpitch);
}

void GSLocalMemory::unSwizzleTexture8P(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 16 || th < 16 || (tw & 15) || (th & 15))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 16, dst += dstpitch*16)
		for(int x = 0; x < tw; x += 16)
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], dst + x, dstpitch);
}

void GSLocalMemory::unSwizzleTexture8HP(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch*8)
		for(int x = 0; x < tw; x += 8)
			unSwizzleBlock8HP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + x, dstpitch);
}

void GSLocalMemory::unSwizzleTexture4P(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 32 || th < 16 || (tw & 31) || (th & 15))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 16, dst += dstpitch*16)
		for(int x = 0; x < tw; x += 32)
			unSwizzleBlock4P((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], dst + x, dstpitch);
}

void GSLocalMemory::unSwizzleTexture4HLP(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch)
		for(int x = 0; x < tw; x += 8)
			unSwizzleBlock4HLP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + x, dstpitch);
}

void GSLocalMemory::unSwizzleTexture4HHP(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	if(tw < 8 || th < 8 || (tw & 7) || (th & 7))
	{
		unSwizzleTextureXP(tw, th, dst, dstpitch, TEX0);
		return;
	}

	for(int y = 0; y < th; y += 8, dst += dstpitch)
		for(int x = 0; x < tw; x += 8)
			unSwizzleBlock4HHP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + x, dstpitch);
}

void GSLocalMemory::unSwizzleTextureXP(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0)
{
	readTexelP rt = GetReadTexelP(TEX0.PSM);

	switch(TEX0.PSM)
	{
	default:
	case PSM_PSMCT32:
	case PSM_PSMCT24:
		for(int y = 0; y < th; y++, dst += dstpitch)
			for(int x = 0; x < tw; x++)
				((DWORD*)dst)[x] = (DWORD)(this->*rt)(x, y, TEX0);
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
		for(int y = 0; y < th; y++, dst += dstpitch)
			for(int x = 0; x < tw; x++)
				((WORD*)dst)[x] = (WORD)(this->*rt)(x, y, TEX0);
		break;
	case PSM_PSMT8:
	case PSM_PSMT8H:
	case PSM_PSMT4:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		for(int y = 0; y < th; y++, dst += dstpitch)
			for(int x = 0; x < tw; x++)
				dst[x] = (BYTE)(this->*rt)(x, y, TEX0);
		break;
	}
}

GSLocalMemory::unSwizzleTextureP GSLocalMemory::GetUnSwizzleTextureP(DWORD psm)
{
	unSwizzleTextureP st = NULL;

	switch(psm)
	{
	default: st = &GSLocalMemory::unSwizzleTextureXP; break;
	case PSM_PSMCT32: st = &GSLocalMemory::unSwizzleTexture32P; break;
	case PSM_PSMCT24: st = &GSLocalMemory::unSwizzleTexture32P; break;
	case PSM_PSMCT16: st = &GSLocalMemory::unSwizzleTexture16P; break;
	case PSM_PSMCT16S: st = &GSLocalMemory::unSwizzleTexture16SP; break;
	case PSM_PSMT8: st = &GSLocalMemory::unSwizzleTexture8P; break;
	case PSM_PSMT8H: st = &GSLocalMemory::unSwizzleTexture8HP; break;
	case PSM_PSMT4: st = &GSLocalMemory::unSwizzleTexture4P; break;
	case PSM_PSMT4HL: st = &GSLocalMemory::unSwizzleTexture4HLP; break;
	case PSM_PSMT4HH: st = &GSLocalMemory::unSwizzleTexture4HHP; break;
	}

	return st;
}

///////////////////

void GSLocalMemory::ReadTextureP(int tw, int th, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegCLAMP& CLAMP)
{
	if((CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		switch(TEX0.PSM)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTextureClampP<DWORD>(dst, dstpitch, TEX0, CLAMP);
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTextureClampP<WORD>(dst, dstpitch, TEX0, CLAMP);
			break;
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadTextureClampP<BYTE>(dst, dstpitch, TEX0, CLAMP);
			break;
		}
	}
	else
	{
		unSwizzleTextureP st = GetUnSwizzleTextureP(TEX0.PSM);
		(this->*st)(tw, th, dst, dstpitch, TEX0);
	}
}
