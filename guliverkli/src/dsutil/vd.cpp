//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2001 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"

extern "C" {
	bool MMX_enabled = true, ISSE_enabled = false; // TODO
};

extern "C" void asm_YUVtoRGB32_row(
		void *ARGB1_pointer,
		void *ARGB2_pointer,
		BYTE *Y1_pointer,
		BYTE *Y2_pointer,
		BYTE *U_pointer,
		BYTE *V_pointer,
		long width
		);

extern "C" void asm_YUVtoRGB24_row(
		void *ARGB1_pointer,
		void *ARGB2_pointer,
		BYTE *Y1_pointer,
		BYTE *Y2_pointer,
		BYTE *U_pointer,
		BYTE *V_pointer,
		long width
		);

extern "C" void asm_YUVtoRGB16_row(
		void *ARGB1_pointer,
		void *ARGB2_pointer,
		BYTE *Y1_pointer,
		BYTE *Y2_pointer,
		BYTE *U_pointer,
		BYTE *V_pointer,
		long width
		);

bool BitBltFromI420(BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int bpp, int srcpitch)
{
	if(w<=0 || h<=0 || (w&7) || (h&1))
		return(false);

	if(srcpitch == 0) srcpitch = w;

	switch(bpp) {
	case 16:
		do {
			asm_YUVtoRGB16_row(dst + dstpitch, dst, srcy + srcpitch, srcy, srcu, srcv, w/2);

			dst += 2*dstpitch;
			srcy += srcpitch*2;
			srcu += srcpitch/2;
			srcv += srcpitch/2;
		} while(h -= 2);
		break;

	case 24:
		do {
			asm_YUVtoRGB24_row(dst + dstpitch, dst, srcy + srcpitch, srcy, srcu, srcv, w/2);

			dst += 2*dstpitch;
			srcy += srcpitch*2;
			srcu += srcpitch/2;
			srcv += srcpitch/2;
		} while(h -= 2);
		break;

	case 32:
		do {
			asm_YUVtoRGB32_row(dst + dstpitch, dst, srcy + srcpitch, srcy, srcu, srcv, w/2);

			dst += 2*dstpitch;
			srcy += srcpitch*2;
			srcu += srcpitch/2;
			srcv += srcpitch/2;
		} while(h -= 2);
		break;

	default:
		__assume(false);
	}

	if (MMX_enabled)
		__asm emms

	if (ISSE_enabled)
		__asm sfence

	return true;
}

static void __declspec(naked) asm_blend_row_clipped(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch)
{
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi,[esp+20]
		mov		esi,[esp+24]
		sub		edi,esi
		mov		ebp,[esp+28]
		mov		edx,[esp+32]

xloop:
		mov		ecx,[esi]
		mov		eax,0fefefefeh

		mov		ebx,[esi+edx]
		and		eax,ecx

		shr		eax,1
		and		ebx,0fefefefeh

		shr		ebx,1
		add		esi,4

		add		eax,ebx
		dec		ebp

		mov		[edi+esi-4],eax
		jnz		xloop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void __declspec(naked) asm_blend_row_MMX(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch)
{
	static const __int64 mask0 = 0xfcfcfcfcfcfcfcfci64;
	static const __int64 mask1 = 0x7f7f7f7f7f7f7f7fi64;
	static const __int64 mask2 = 0x3f3f3f3f3f3f3f3fi64;
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi,[esp+20]
		mov		esi,[esp+24]
		sub		edi,esi
		mov		ebp,[esp+28]
		mov		edx,[esp+32]

		movq	mm5,mask0
		movq	mm6,mask1
		movq	mm7,mask2
		shr		ebp,1
		jz		oddpart

xloop:
		movq	mm2,[esi]
		movq	mm0,mm5

		movq	mm1,[esi+edx]
		pand	mm0,mm2

		psrlq	mm1,1
		movq	mm2,[esi+edx*2]

		psrlq	mm2,2
		pand	mm1,mm6

		psrlq	mm0,2
		pand	mm2,mm7

		paddb	mm0,mm1
		add		esi,8

		paddb	mm0,mm2
		dec		ebp

		movq	[edi+esi-8],mm0
		jne		xloop

oddpart:
		test	byte ptr [esp+28],1
		jz		nooddpart

		mov		ecx,[esi]
		mov		eax,0fcfcfcfch
		mov		ebx,[esi+edx]
		and		eax,ecx
		shr		ebx,1
		mov		ecx,[esi+edx*2]
		shr		ecx,2
		and		ebx,07f7f7f7fh
		shr		eax,2
		and		ecx,03f3f3f3fh
		add		eax,ebx
		add		eax,ecx
		mov		[edi+esi],eax

nooddpart:

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

void DeinterlaceBlend(BYTE* dst, BYTE* src, DWORD rowbytes, DWORD h, DWORD pitch)
{
	rowbytes >>= 2;

	asm_blend_row_clipped(dst, src, rowbytes, pitch);

	if(h -= 2) do
	{
		dst += pitch;
		asm_blend_row_MMX(dst, src, rowbytes, pitch);
        src += pitch;
	}
	while(--h);

	asm_blend_row_clipped(dst + pitch, src, rowbytes, pitch);

	__asm emms
}

