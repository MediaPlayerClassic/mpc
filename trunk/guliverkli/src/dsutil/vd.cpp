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

#pragma warning(disable : 4799) // no emms... blahblahblah

extern "C" {
	bool MMX_enabled = true, ISSE_enabled = false; // TODO
};

void memcpy_mmx(void* dst, const void* src, size_t len)
{
    __asm 
    {
        mov     esi, dword ptr [src]
        mov     edi, dword ptr [dst]
        mov     ecx, len
        shr     ecx, 6
align 8
CopyLoop:
        movq    mm0, qword ptr[esi]
        movq    mm1, qword ptr[esi+8*1]
        movq    mm2, qword ptr[esi+8*2]
        movq    mm3, qword ptr[esi+8*3]
        movq    mm4, qword ptr[esi+8*4]
        movq    mm5, qword ptr[esi+8*5]
        movq    mm6, qword ptr[esi+8*6]
        movq    mm7, qword ptr[esi+8*7]
        movq    qword ptr[edi], mm0
        movq    qword ptr[edi+8*1], mm1
        movq    qword ptr[edi+8*2], mm2
        movq    qword ptr[edi+8*3], mm3
        movq    qword ptr[edi+8*4], mm4
        movq    qword ptr[edi+8*5], mm5
        movq    qword ptr[edi+8*6], mm6
        movq    qword ptr[edi+8*7], mm7
        add     esi, 64
        add     edi, 64
        loop CopyLoop
        mov     ecx, len
        and     ecx, 63
        cmp     ecx, 0
        je EndCopyLoop
align 8
CopyLoop2:
        mov dl, byte ptr[esi] 
        mov byte ptr[edi], dl
        inc esi
        inc edi
        dec ecx
        jne CopyLoop2
EndCopyLoop:
		emms
    }
}

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

bool BitBltFromI420ToRGB(BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int bpp, int srcpitch)
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

static void __declspec(naked) yuvtoyuy2row(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
	/*
		WORD* dstw = (WORD*)dst;
		for(; width > 1; width -= 2)
		{
			*dstw++ = (*srcu++<<8)|*srcy++;
			*dstw++ = (*srcv++<<8)|*srcy++;
		}
	*/

	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi, [esp+20] // dst
		mov		ebp, [esp+24] // srcy
		mov		ebx, [esp+28] // srcu
		mov		esi, [esp+32] // srcv
		mov		ecx, [esp+36] // width

		shr		ecx, 3

yuvtoyuy2row_loop:

		movd		mm0, [ebx]
		punpcklbw	mm0, [esi]

		movq		mm1, [ebp]
		movq		mm2, mm1
		punpcklbw	mm1, mm0
		punpckhbw	mm2, mm0

		movq		[edi], mm1
		movq		[edi+8], mm2

		add		ebp, 8
		add		ebx, 4
		add		esi, 4
        add		edi, 16

		loop	yuvtoyuy2row_loop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void __declspec(naked) yuvtoyuy2row_avg(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
	/*
		WORD* dstw = (WORD*)dst;
		for(; width > 1; width -= 2, srcu++, srcv++)
		{
			*dstw++ = (((srcu[0]+srcu[pitchuv])>>1)<<8)|*srcy++;
			*dstw++ = (((srcv[0]+srcv[pitchuv])>>1)<<8)|*srcy++;
		}
	*/

	static const __int64 mask = 0x7f7f7f7f7f7f7f7fi64;

	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		movq	mm7, mask

		mov		edi, [esp+20] // dst
		mov		ebp, [esp+24] // srcy
		mov		ebx, [esp+28] // srcu
		mov		esi, [esp+32] // srcv
		mov		ecx, [esp+36] // width
		mov		eax, [esp+40] // pitchuv

		shr		ecx, 3

yuvtoyuy2row_avg_loop:

		movd		mm0, [ebx]
		punpcklbw	mm0, [esi]
		movq		mm1, mm0

		movd		mm2, [ebx + eax]
		punpcklbw	mm2, [esi + eax]
		movq		mm3, mm2

		// (x+y)>>1 == (x&y)+((x^y)>>1)

		pand		mm0, mm2
		pxor		mm1, mm3
		psrlq		mm1, 1
		pand		mm1, mm7
		paddb		mm0, mm1

		movq		mm1, [ebp]
		movq		mm2, mm1
		punpcklbw	mm1, mm0
		punpckhbw	mm2, mm0

		movq		[edi], mm1
		movq		[edi+8], mm2

		add		ebp, 8
		add		ebx, 4
		add		esi, 4
        add		edi, 16

		loop	yuvtoyuy2row_avg_loop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

bool BitBltFromI420ToYUY2(BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int srcpitch)
{
	if(w<=0 || h<=0 || (w&7) || (h&1))
		return(false);

	if(srcpitch == 0) srcpitch = w;

	do {
		yuvtoyuy2row(dst, srcy, srcu, srcv, w);
		yuvtoyuy2row_avg(dst + dstpitch, srcy + srcpitch, srcu, srcv, w, srcpitch/2);

		dst += 2*dstpitch;
		srcy += srcpitch*2;
		srcu += srcpitch/2;
		srcv += srcpitch/2;
	} while((h -= 2) > 2);

	yuvtoyuy2row(dst, srcy, srcu, srcv, w);
	yuvtoyuy2row(dst + dstpitch, srcy + srcpitch, srcu, srcv, w);

	if (MMX_enabled)
		__asm emms

	if (ISSE_enabled)
		__asm sfence

	return(true);
}

static void __declspec(naked) asm_blend_row_clipped_MMX(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch)
{
	static const __int64 _x0001000100010001 = 0x0001000100010001;

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

		shr		ebp, 1

		movq	mm6, _x0001000100010001
		pxor	mm7, mm7

xloop:
		movq		mm0, [esi]
		movq		mm3, mm0
		punpcklbw	mm0, mm7
		punpckhbw	mm3, mm7

		movq		mm1, [esi+edx]
		movq		mm4, mm1
		punpcklbw	mm1, mm7
		punpckhbw	mm4, mm7

		paddw		mm1, mm0
		paddw		mm1, mm6
		psrlw		mm1, 1

		paddw		mm4, mm3
		paddw		mm4, mm6
		psrlw		mm4, 1

		add			esi, 8
		packuswb	mm1, mm4
		movq		[edi+esi-8], mm1

		dec		ebp
		jne		xloop

/*
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
*/
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
	static const __int64 _x0002000200020002 = 0x0002000200020002;

	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi, [esp+20]
		mov		esi, [esp+24]
		sub		edi, esi
		mov		ebp, [esp+28]
		mov		edx, [esp+32]

		shr		ebp, 1

		movq	mm6, _x0002000200020002
		pxor	mm7, mm7

xloop:
		movq		mm0, [esi]
		movq		mm3, mm0
		punpcklbw	mm0, mm7
		punpckhbw	mm3, mm7

		movq		mm1, [esi+edx]
		movq		mm4, mm1
		punpcklbw	mm1, mm7
		punpckhbw	mm4, mm7

		movq		mm2, [esi+edx*2]
		movq		mm5, mm2
		punpcklbw	mm2, mm7
		punpckhbw	mm5, mm7

		psllw		mm1, 1
		paddw		mm1, mm0
		paddw		mm1, mm2
		paddw		mm1, mm6
		psrlw		mm1, 2

		psllw		mm4, 1
		paddw		mm4, mm3
		paddw		mm4, mm5
		paddw		mm4, mm6
		psrlw		mm4, 2

		add			esi, 8
		packuswb	mm1, mm4
		movq		[edi+esi-8], mm1

		dec		ebp
		jne		xloop

		// sadly the original code makes a lot of visible banding artifacts on yuv
		// (it seems those shiftings without rounding introduce too much error)
/*
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
*/
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

	asm_blend_row_clipped_MMX(dst, src, rowbytes, pitch);

	if(h -= 2) do
	{
		dst += pitch;
		asm_blend_row_MMX(dst, src, rowbytes, pitch);
        src += pitch;
	}
	while(--h);

	asm_blend_row_clipped_MMX(dst + pitch, src, rowbytes, pitch);

	__asm emms
}

