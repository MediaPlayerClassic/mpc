#include "stdafx.h"
#include "GSTables.h"
#include "x86.h"
/*
#define loadunp(r0, r1, r2, r3)				\
	__m128i xmm##r0 = ((__m128i*)src)[0];	\
	__m128i xmm##r1 = ((__m128i*)src)[1];	\
	__m128i xmm##r2 = ((__m128i*)src)[2];	\
	__m128i xmm##r3 = ((__m128i*)src)[3];	\

#define unpack(op, sd0, sd2, s1, s3, d1, d3)				\
	xmm##d1 = xmm##sd0;										\
	xmm##d3 = _mm_shuffle_epi32(xmm##sd2, 0xe4);			\
	xmm##sd0 = _mm_unpacklo_epi##op(xmm##sd0, xmm##s1);		\
	xmm##d1 = _mm_unpackhi_epi##op(xmm##d1, xmm##s1);		\
	xmm##sd2 = _mm_unpacklo_epi##op(xmm##sd2, xmm##s3);		\
	xmm##d3 = _mm_unpackhi_epi##op(xmm##d3, xmm##s3);		\

void __fastcall unSwizzleBlock32_c_sse2(BYTE* src, BYTE* dst, int dstpitch)
{
	for(int i = 0; i < 4; i++, src += 64)
	{
		loadunp(0, 1, 2, 3);

		__m128i xmm4, xmm6;
		unpack(64, 0, 2, 1, 3, 4, 6)

		((__m128i*)dst)[0] = xmm0;
		((__m128i*)dst)[1] = xmm2;
		((__m128i*)(dst+dstpitch))[0] = xmm4;
		((__m128i*)(dst+dstpitch))[1] = xmm6;
		dst += dstpitch*2;
	}
}

void __fastcall unSwizzleBlock16_c_sse2(BYTE* src, BYTE* dst, int dstpitch)
{
	for(int i = 0; i < 4; i++, src += 64)
	{
		loadunp(0, 1, 2, 3);

		__m128i xmm4, xmm6;
		unpack(16, 0, 2, 1, 3, 4, 6)
		unpack(32, 0, 4, 2, 6, 1, 3)
		unpack(16, 0, 4, 1, 3, 2, 6)

		((__m128i*)dst)[0] = xmm0;
		((__m128i*)dst)[1] = xmm2;
		((__m128i*)(dst+dstpitch))[0] = xmm4;
		((__m128i*)(dst+dstpitch))[1] = xmm6;
		dst += dstpitch*2;
	}
}
*/
void __fastcall unSwizzleBlock32_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			((DWORD*)dst)[i] = ((DWORD*)src)[s[i]];
}

void __fastcall unSwizzleBlock16_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable16[0][0];

	for(int j = 0; j < 8; j++, s += 16, dst += dstpitch)
		for(int i = 0; i < 16; i++)
			((WORD*)dst)[i] = ((WORD*)src)[s[i]];
}

void __fastcall unSwizzleBlock8_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable8[0][0];

	for(int j = 0; j < 16; j++, s += 16, dst += dstpitch)
		for(int i = 0; i < 16; i++)
			dst[i] = src[s[i]];
}

void __fastcall unSwizzleBlock4_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, s += 32, dst += dstpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = s[i];
			BYTE c = (src[addr>>1] >> ((addr&1) << 2)) & 0x0f;
			int shift = (i&1) << 2;
			dst[i >> 1] = (dst[i >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}

void __fastcall unSwizzleBlock8HP_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			dst[i] = (BYTE)(((DWORD*)src)[s[i]]>>24);
}

void __fastcall unSwizzleBlock4HLP_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			dst[i] = (BYTE)(((DWORD*)src)[s[i]]>>24)&0xf;
}

void __fastcall unSwizzleBlock4HHP_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable32[0][0];

	for(int j = 0; j < 8; j++, s += 8, dst += dstpitch)
		for(int i = 0; i < 8; i++)
			dst[i] = (BYTE)(((DWORD*)src)[s[i]]>>28);
}

void __fastcall unSwizzleBlock4P_c(BYTE* src, BYTE* dst, int dstpitch)
{
	DWORD* s = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, s += 32, dst += dstpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = s[i];
			dst[i] = (src[addr>>1] >> ((addr&1) << 2)) & 0x0f;
		}
	}
}

/*
static class wertyu
{
public:
	class wertyu()
	{
		DWORD* d = &columnTable4[0][0];
		__declspec(align(16)) static BYTE src[16][16];
		for(int j = 0, k = 0; j < 16; j++)
		{
			for(int i = 0; i < 32; i++, k++)
			{
				if(d[k]&1) ((BYTE*)src)[d[k]>>1] = (((BYTE*)src)[d[k]>>1]&0x0f) | ((k&0xf)<<4);
				else ((BYTE*)src)[d[k]>>1] = (((BYTE*)src)[d[k]>>1]&0xf0) | (k&0xf);
			}
		}
		__declspec(align(16)) static BYTE dst[16][256];
		int dstpitch = sizeof(dst[16]);

		unSwizzleBlock4P_sse2((BYTE*)src, (BYTE*)dst, dstpitch);
	}
} gertgrtgrt;
*/
//

void __fastcall SwizzleBlock32_c(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask)
{
	DWORD* d = &columnTable32[0][0];

	if(WriteMask == 0xffffffff)
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)dst)[d[i]] = ((DWORD*)src)[i];
	}
	else
	{
		for(int j = 0; j < 8; j++, d += 8, src += srcpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)dst)[d[i]] = (((DWORD*)dst)[d[i]] & ~WriteMask) | (((DWORD*)src)[i] & WriteMask);
	}
}

void __fastcall SwizzleBlock16_c(BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable16[0][0];

	for(int j = 0; j < 8; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			((WORD*)dst)[d[i]] = ((WORD*)src)[i];
}

void __fastcall SwizzleBlock8_c(BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable8[0][0];

	for(int j = 0; j < 16; j++, d += 16, src += srcpitch)
		for(int i = 0; i < 16; i++)
			dst[d[i]] = src[i];
}

void __fastcall SwizzleBlock4_c(BYTE* dst, BYTE* src, int srcpitch)
{
	DWORD* d = &columnTable4[0][0];

	for(int j = 0; j < 16; j++, d += 32, src += srcpitch)
	{
		for(int i = 0; i < 32; i++)
		{
			DWORD addr = d[i];
			BYTE c = (src[i>>1] >> ((i&1) << 2)) & 0x0f;
			int shift = (addr&1) << 2;
			dst[addr >> 1] = (dst[addr >> 1] & (0xf0 >> shift)) | (c << shift);
		}
	}
}

//

void __fastcall ExpandBlock24_c(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	DWORD TA0 = (DWORD)pTEXA->TA0 << 24;

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
			for(int i = 0; i < 8; i++)
				dst[i] = TA0 | (src[i]&0xffffff);
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
			for(int i = 0; i < 8; i++)
				dst[i] = ((src[i]&0xffffff) ? TA0 : 0) | (src[i]&0xffffff);
	}
}

void __fastcall ExpandBlock16_c(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	DWORD TA0 = (DWORD)pTEXA->TA0 << 24;
	DWORD TA1 = (DWORD)pTEXA->TA1 << 24;

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
			for(int i = 0; i < 16; i++)
				dst[i] = ((src[i]&0x8000) ? TA1 : TA0)
					| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
			for(int i = 0; i < 16; i++)
				dst[i] = ((src[i]&0x8000) ? TA1 : (src[i]&0x7fff) ? TA0 : 0)
					| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
}

// TODO: asm version

extern "C" void ExpandBlock24_amd64(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	ExpandBlock24_c(src, dst, dstpitch, pTEXA);
}

extern "C" void ExpandBlock16_amd64(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	ExpandBlock16_c(src, dst, dstpitch, pTEXA);
}
/*
extern "C" void __fastcall ExpandBlock24_sse2(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	ExpandBlock24_c(src, dst, dstpitch, pTEXA);
}
*/
extern "C" void __fastcall ExpandBlock16_sse2(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	ExpandBlock16_c(src, dst, dstpitch, pTEXA);
}

//

void __fastcall UVMinMax_c(int nVertices, vertex_t* pVertices, uvmm_t* uv)
{
	uv->umin = uv->vmin = +1e10;
	uv->umax = uv->vmax = -1e10;

	for(; nVertices-- > 0; pVertices++)
	{
		float u = pVertices->u;
		if(uv->umax < u) uv->umax = u;
		if(uv->umin > u) uv->umin = u;
		float v = pVertices->v;
		if(uv->vmax < v) uv->vmax = v;
		if(uv->vmin > v) uv->vmin = v;
	}
}
