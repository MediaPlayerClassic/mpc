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
#include "vd.h"

#pragma warning(disable : 4799) // no emms... blahblahblah

CCpuID::CCpuID()
{
	DWORD flags = 0;

	__asm
	{
		mov			eax, 1
		cpuid
		test		edx, 0x00800000		// STD MMX
		jz			TEST_SSE
		or			[flags], 1
TEST_SSE:
		test		edx, 0x02000000		// STD SSE
		jz			TEST_SSE2
		or			[flags], 2
		or			[flags], 4
TEST_SSE2:
		test		edx, 0x04000000		// SSE2	
		jz			TEST_3DNOW
		or			[flags], 8
TEST_3DNOW:
		mov			eax, 0x80000001
		cpuid
		test		edx, 0x80000000		// 3D NOW
		jz			TEST_SSEMMX
		or			[flags], 16
TEST_SSEMMX:
		test		edx, 0x00400000		// SSE MMX
		jz			TEST_END
		or			[flags], 2
TEST_END:
	}

	m_flags = (flag_t)flags;
}

CCpuID g_cpuid;

void memcpy_accel(void* dst, const void* src, size_t len)
{
	if((g_cpuid.m_flags & CCpuID::flag_t::ssefpu) && len >= 128 
		&& !((DWORD)src&15) && !((DWORD)dst&15))
	{
		__asm
		{
			mov     esi, dword ptr [src]
			mov     edi, dword ptr [dst]
			mov     ecx, len
			shr     ecx, 7
	memcpy_accel_sse_loop:
			prefetchnta	[esi]
			movaps		xmm0, [esi]
			movaps		xmm1, [esi+16*1]
			movaps		xmm2, [esi+16*2]
			movaps		xmm3, [esi+16*3]
			movaps		xmm4, [esi+16*4]
			movaps		xmm5, [esi+16*5]
			movaps		xmm6, [esi+16*6]
			movaps		xmm7, [esi+16*7]
			movntps		[edi], xmm0
			movntps		[edi+16*1], xmm1
			movntps		[edi+16*2], xmm2
			movntps		[edi+16*3], xmm3
			movntps		[edi+16*4], xmm4
			movntps		[edi+16*5], xmm5
			movntps		[edi+16*6], xmm6
			movntps		[edi+16*7], xmm7
			add			esi, 128
			add			edi, 128
			loop	memcpy_accel_sse_loop
			mov     ecx, len
			and     ecx, 127
			cmp     ecx, 0
			je		memcpy_accel_sse_end
	memcpy_accel_sse_loop2:
			mov		dl, byte ptr[esi] 
			mov		byte ptr[edi], dl
			inc		esi
			inc		edi
			dec		ecx
			jne		memcpy_accel_sse_loop2
	memcpy_accel_sse_end:
			sfence
		}
	}
	else if((g_cpuid.m_flags & CCpuID::flag_t::mmx) && len >= 64
		&& !((DWORD)src&7) && !((DWORD)dst&7))
	{
		__asm 
		{
			mov     esi, dword ptr [src]
			mov     edi, dword ptr [dst]
			mov     ecx, len
			shr     ecx, 6
	memcpy_accel_mmx_loop:
			movq    mm0, qword ptr [esi]
			movq    mm1, qword ptr [esi+8*1]
			movq    mm2, qword ptr [esi+8*2]
			movq    mm3, qword ptr [esi+8*3]
			movq    mm4, qword ptr [esi+8*4]
			movq    mm5, qword ptr [esi+8*5]
			movq    mm6, qword ptr [esi+8*6]
			movq    mm7, qword ptr [esi+8*7]
			movq    qword ptr [edi], mm0
			movq    qword ptr [edi+8*1], mm1
			movq    qword ptr [edi+8*2], mm2
			movq    qword ptr [edi+8*3], mm3
			movq    qword ptr [edi+8*4], mm4
			movq    qword ptr [edi+8*5], mm5
			movq    qword ptr [edi+8*6], mm6
			movq    qword ptr [edi+8*7], mm7
			add     esi, 64
			add     edi, 64
			loop	memcpy_accel_mmx_loop
			mov     ecx, len
			and     ecx, 63
			cmp     ecx, 0
			je		memcpy_accel_mmx_end
	memcpy_accel_mmx_loop2:
			mov		dl, byte ptr [esi] 
			mov		byte ptr [edi], dl
			inc		esi
			inc		edi
			dec		ecx
			jne		memcpy_accel_mmx_loop2
	memcpy_accel_mmx_end:
			emms
		}
	}
	else
	{
		memcpy(dst, src, len);
	}
}

extern "C" void asm_YUVtoRGB32_row(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB24_row(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB16_row(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB32_row_MMX(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB24_row_MMX(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB16_row_MMX(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB32_row_ISSE(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB24_row_ISSE(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB16_row_ISSE(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);

bool BitBltFromI420ToRGB(BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int bpp, int srcpitch)
{
	if(w<=0 || h<=0 || (w&7) || (h&1))
		return(false);

	if(srcpitch == 0) srcpitch = w;

	void (*asm_YUVtoRGB_row)(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width) = NULL;;

	if(g_cpuid.m_flags & CCpuID::flag_t::ssefpu)
	{
		switch(bpp)
		{
		case 16: asm_YUVtoRGB_row = asm_YUVtoRGB16_row/*_ISSE*/; break; // TODO: fix _ISSE (555->565)
		case 24: asm_YUVtoRGB_row = asm_YUVtoRGB24_row_ISSE; break;
		case 32: asm_YUVtoRGB_row = asm_YUVtoRGB32_row_ISSE; break;
		}
	}
	else if(g_cpuid.m_flags & CCpuID::flag_t::mmx)
	{
		switch(bpp)
		{
		case 16: asm_YUVtoRGB_row = asm_YUVtoRGB16_row/*_MMX*/; break; // TODO: fix _MMX (555->565)
		case 24: asm_YUVtoRGB_row = asm_YUVtoRGB24_row_MMX; break;
		case 32: asm_YUVtoRGB_row = asm_YUVtoRGB32_row_MMX; break;
		}
	}
	
	else
	{
		switch(bpp)
		{
		case 16: asm_YUVtoRGB_row = asm_YUVtoRGB16_row; break;
		case 24: asm_YUVtoRGB_row = asm_YUVtoRGB24_row; break;
		case 32: asm_YUVtoRGB_row = asm_YUVtoRGB32_row; break;
		}
	}

	if(!asm_YUVtoRGB_row) 
		return(false);

	do
	{
		asm_YUVtoRGB_row(dst + dstpitch, dst, srcy + srcpitch, srcy, srcu, srcv, w/2);

		dst += 2*dstpitch;
		srcy += srcpitch*2;
		srcu += srcpitch/2;
		srcv += srcpitch/2;
	}
	while(h -= 2);
		
	if(g_cpuid.m_flags & CCpuID::flag_t::mmx)
		__asm emms

	if(g_cpuid.m_flags & CCpuID::flag_t::ssefpu)
		__asm sfence

	return true;
}

static void yuvtoyuy2row_c(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
	WORD* dstw = (WORD*)dst;
	for(; width > 1; width -= 2)
	{
		*dstw++ = (*srcu++<<8)|*srcy++;
		*dstw++ = (*srcv++<<8)|*srcy++;
	}
}

static void __declspec(naked) yuvtoyuy2row_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
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

static void yuvtoyuy2row_avg_c(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
	WORD* dstw = (WORD*)dst;
	for(; width > 1; width -= 2, srcu++, srcv++)
	{
		*dstw++ = (((srcu[0]+srcu[pitchuv])>>1)<<8)|*srcy++;
		*dstw++ = (((srcv[0]+srcv[pitchuv])>>1)<<8)|*srcy++;
	}
}

static void __declspec(naked) yuvtoyuy2row_avg_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
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

	void (*yuvtoyuy2row)(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width) = NULL;
	void (*yuvtoyuy2row_avg)(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv) = NULL;

	if(g_cpuid.m_flags & CCpuID::flag_t::mmx)
	{
		yuvtoyuy2row = yuvtoyuy2row_MMX;
		yuvtoyuy2row_avg = yuvtoyuy2row_avg_MMX;
	}
	else
	{
		yuvtoyuy2row = yuvtoyuy2row_c;
		yuvtoyuy2row_avg = yuvtoyuy2row_avg_c;
	}

	if(!yuvtoyuy2row) 
		return(false);

	do
	{
		yuvtoyuy2row(dst, srcy, srcu, srcv, w);
		yuvtoyuy2row_avg(dst + dstpitch, srcy + srcpitch, srcu, srcv, w, srcpitch/2);

		dst += 2*dstpitch;
		srcy += srcpitch*2;
		srcu += srcpitch/2;
		srcv += srcpitch/2;
	}
	while((h -= 2) > 2);

	yuvtoyuy2row(dst, srcy, srcu, srcv, w);
	yuvtoyuy2row(dst + dstpitch, srcy + srcpitch, srcu, srcv, w);

	if(g_cpuid.m_flags & CCpuID::flag_t::mmx)
		__asm emms

	return(true);
}

static void asm_blend_row_clipped_c(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch)
{
	BYTE* src2 = src + srcpitch;
	do {*dst++ = (*src++ + *src2++ + 1) >> 1;}
	while(w--);
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

		shr		ebp, 3

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

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void asm_blend_row_c(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch)
{
	BYTE* src2 = src + srcpitch;
	BYTE* src3 = src2 + srcpitch;
	do {*dst++ = (*src++ + (*src2++ << 1) + *src3++ + 2) >> 2;}
	while(w--);
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

		shr		ebp, 3

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
	void (*asm_blend_row_clipped)(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch) = NULL;
	void (*asm_blend_row)(BYTE* dst, BYTE* src, DWORD w, DWORD srcpitch) = NULL;

	if(g_cpuid.m_flags & CCpuID::flag_t::mmx)
	{
		asm_blend_row_clipped = asm_blend_row_clipped_MMX;
		asm_blend_row = asm_blend_row_MMX;
	}
	else
	{
		asm_blend_row_clipped = asm_blend_row_clipped_c;
		asm_blend_row = asm_blend_row_c;
	}

	if(!asm_blend_row_clipped)
		return;

	asm_blend_row_clipped(dst, src, rowbytes, pitch);

	if(h -= 2) do
	{
		dst += pitch;
		asm_blend_row(dst, src, rowbytes, pitch);
        src += pitch;
	}
	while(--h);

	asm_blend_row_clipped(dst + pitch, src, rowbytes, pitch);

	if(g_cpuid.m_flags & CCpuID::flag_t::mmx)
		__asm emms
}

