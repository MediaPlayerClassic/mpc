#include "stdafx.h"
#include "GSTables.h"

void __fastcall unSwizzleBlock32_c(BYTE* src, BYTE* dst, int dstpitch)
{
	WORD* s = &columnTable32[0][0];

	for(int j = 0, diff = dstpitch - 8*4; j < 8; j++, dst += diff)
		for(int i = 0; i < 8; i++, dst += 4)
			*(DWORD*)dst = ((DWORD*)src)[*s++];
}

void __fastcall unSwizzleBlock16_c(BYTE* src, BYTE* dst, int dstpitch)
{
	WORD* s = &columnTable16[0][0];

	for(int j = 0, diff = dstpitch - 16*2; j < 8; j++, dst += diff)
		for(int i = 0; i < 16; i++, dst += 2)
			*(WORD*)dst = ((WORD*)src)[*s++];
}

void __fastcall unSwizzleBlock8_c(BYTE* src, BYTE* dst, int dstpitch)
{
	WORD* s = &columnTable8[0][0];

	for(int j = 0, diff = dstpitch - 16; j < 16; j++, dst += diff)
		for(int i = 0; i < 16; i++)
			*dst++ = src[*s++];
}

void __fastcall unSwizzleBlock4_c(BYTE* src, BYTE* dst, int dstpitch)
{
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
}

void __fastcall SwizzleBlock32_c(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask)
{
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
}

void __fastcall SwizzleBlock16_c(BYTE* dst, BYTE* src, int srcpitch)
{
	WORD* d = &columnTable16[0][0];

	for(int j = 0, diff = srcpitch - 16*2; j < 8; j++, src += diff)
		for(int i = 0; i < 16; i++, src += 2)
			((WORD*)dst)[*d++] = *(WORD*)src;
}

void __fastcall SwizzleBlock8_c(BYTE* dst, BYTE* src, int srcpitch)
{
	WORD* d = &columnTable8[0][0];

	for(int j = 0, diff = srcpitch - 16; j < 16; j++, src += diff)
		for(int i = 0; i < 16; i++, src++)
			dst[*d++] = *src;
}

void __fastcall SwizzleBlock4_c(BYTE* dst, BYTE* src, int srcpitch)
{
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
}