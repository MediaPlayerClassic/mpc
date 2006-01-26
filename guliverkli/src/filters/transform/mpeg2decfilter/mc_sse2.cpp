/* 
 *	Copyright (C) 2003-2006 Gabest
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
 *  Based on Intel's AP-942
 *
 */

#include "stdafx.h"
#include "libmpeg2.h"

__declspec(align(16)) static BYTE const_1_16_bytes[] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

static void MC_put_o_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov esi, height
		mov eax, stride
		lea edi, [eax+eax]

	MC_put_o_16_sse2_loop:

		movdqu xmm0, [edx]
		movdqu xmm1, [edx+eax] 
		movdqa [ecx], xmm0
		movdqa [ecx+eax], xmm1 
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_put_o_16_sse2_loop
	}
}

static void MC_put_o_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov esi, height
		mov eax, stride
		lea edi, [eax+eax]

	MC_put_o_8_sse2_loop:

		movlpd xmm0, [edx]
		movhpd xmm0, [edx+eax] 
		movlpd [ecx], xmm0
		movhpd [ecx+eax], xmm0 
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_put_o_8_sse2_loop
	}
}

static void MC_put_x_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

	MC_put_x_16_sse2_loop:

		movdqu xmm0, [edx]
		movdqu xmm1, [edx+1]
		movdqu xmm2, [edx+eax]
		movdqu xmm3, [edx+eax+1]
		pavgb xmm0, xmm1
		pavgb xmm2, xmm3
		movdqa [ecx], xmm0
		movdqa [ecx+eax], xmm2
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_put_x_16_sse2_loop
	}
}

static void MC_put_x_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

	MC_put_x_8_sse2_loop:

		movlpd xmm0, [edx]
		movlpd xmm1, [edx+1]
		movhpd xmm0, [edx+eax]
		movhpd xmm1, [edx+eax+1]
		pavgb xmm0, xmm1
		movlpd [ecx], xmm0
		movhpd [ecx+eax], xmm0
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_put_x_8_sse2_loop
	}
}

static void MC_put_y_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

		movdqu xmm0, [edx] 

	MC_put_y_16_sse2_loop:

		movdqu xmm1, [edx+eax] 
		movdqu xmm2, [edx+edi] 
		pavgb xmm0, xmm1 
		pavgb xmm1, xmm2 
		movdqa [ecx], xmm0 
		movdqa [ecx+eax], xmm1 
		movdqa xmm0, xmm2 
		add edx, edi 
		add ecx, edi
		sub esi, 2

		jg MC_put_y_16_sse2_loop
	}
}

static void MC_put_y_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

		movlpd xmm0, [edx] 

	MC_put_y_8_sse2_loop:

		movlpd xmm1, [edx+eax] 
		movlpd xmm2, [edx+edi] 
		pavgb xmm0, xmm1 
		pavgb xmm1, xmm2
		movlpd [ecx], xmm0 
		movlpd [ecx+eax], xmm1 
		movdqa xmm0, xmm2 
		add edx, edi 
		add ecx, edi
		sub esi, 2

		jg MC_put_y_8_sse2_loop
	}
}

static void MC_put_xy_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref 
		mov ecx, dest
		mov eax, stride 
		mov esi, height 
		lea edi, [eax+eax] 
		
		movdqa xmm7, [const_1_16_bytes] 
		movdqu xmm0, [edx] 
		movdqu xmm1, [edx+1] 

	MC_put_xy_16_sse2_loop:

		movdqu xmm2, [edx+eax] 
		movdqu xmm3, [edx+eax+1] 
		movdqu xmm4, [edx+edi] 
		movdqu xmm5, [edx+edi+1] 
		pavgb xmm0, xmm1 
		pavgb xmm2, xmm3 
		movdqa xmm1, xmm5 
		pavgb xmm5, xmm4 
		psubusb xmm2, xmm7 
		pavgb xmm0, xmm2 
		pavgb xmm2, xmm5
		movdqa [ecx], xmm0
		movdqa xmm0, xmm4
		movdqa [ecx+eax], xmm2
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_put_xy_16_sse2_loop
	}
}

static void MC_put_xy_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref 
		mov ecx, dest
		mov eax, stride 
		mov esi, height 
		lea edi, [eax+eax] 
		
		movdqa xmm7, [const_1_16_bytes] 
		movlpd xmm0, [edx] 
		movlpd xmm1, [edx+1] 

	MC_put_xy_8_sse2_loop:

		movlpd xmm2, [edx+eax] 
		movlpd xmm3, [edx+eax+1] 
		movlpd xmm4, [edx+edi] 
		movlpd xmm5, [edx+edi+1] 
		pavgb xmm0, xmm1 
		pavgb xmm2, xmm3 
		movdqa xmm1, xmm5 
		pavgb xmm5, xmm4 
		psubusb xmm2, xmm7 
		pavgb xmm0, xmm2 
		pavgb xmm2, xmm5
		movlpd [ecx], xmm0
		movdqa xmm0, xmm4
		movlpd [ecx+eax], xmm2
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_put_xy_8_sse2_loop
	}

}

static void MC_avg_o_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov esi, height
		mov eax, stride
		lea edi, [eax+eax]

	MC_avg_o_16_sse2_loop:

		movdqu xmm0, [edx]
		movdqu xmm1, [edx+eax] 
		movdqa xmm2, [ecx]
		movdqa xmm3, [ecx+eax]
		pavgb xmm0, xmm2
		pavgb xmm1, xmm3
		movdqa [ecx], xmm0
		movdqa [ecx+eax], xmm1 
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_avg_o_16_sse2_loop
	}
}

static void MC_avg_o_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov esi, height
		mov eax, stride
		lea edi, [eax+eax]

	MC_avg_o_16_sse2_loop:

		movlpd xmm0, [edx]
		movhpd xmm0, [edx+eax] 
		movlpd xmm1, [ecx]
		movhpd xmm1, [ecx+eax]
		pavgb xmm0, xmm1
		movlpd [ecx], xmm0
		movhpd [ecx+eax], xmm0
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_avg_o_16_sse2_loop
	}
}

static void MC_avg_x_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

	MC_avg_x_16_sse2_loop:

		movdqu xmm0, [edx]
		movdqu xmm1, [edx+1]
		movdqu xmm2, [edx+eax]
		movdqu xmm3, [edx+eax+1]
		pavgb xmm0, xmm1
		pavgb xmm2, xmm3
		movdqa xmm4, [ecx]
		movdqa xmm5, [ecx+eax]
		pavgb xmm0, xmm4
		pavgb xmm2, xmm5
		movdqa [ecx], xmm0
		movdqa [ecx+eax], xmm2
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_avg_x_16_sse2_loop
	}
}

static void MC_avg_x_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

	MC_avg_x_8_sse2_loop:

		movlpd xmm0, [edx]
		movlpd xmm1, [edx+1]
		movhpd xmm0, [edx+eax]
		movhpd xmm1, [edx+eax+1]
		pavgb xmm0, xmm1
		movlpd xmm2, [ecx]
		movhpd xmm2, [ecx+eax]
		pavgb xmm0, xmm2
		movlpd [ecx], xmm0
		movhpd [ecx+eax], xmm0
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_avg_x_8_sse2_loop
	}
}

static void MC_avg_y_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

		movdqu xmm0, [edx] 

	MC_avg_y_16_sse2_loop:

		movdqu xmm1, [edx+eax] 
		movdqu xmm2, [edx+edi] 
		pavgb xmm0, xmm1 
		pavgb xmm1, xmm2 
		movdqa xmm3, [ecx] 
		movdqa xmm4, [ecx+eax] 
		pavgb xmm0, xmm3
		pavgb xmm1, xmm4
		movdqa [ecx], xmm0 
		movdqa xmm0, xmm2 
		movdqa [ecx+eax], xmm1 
		add edx, edi 
		add ecx, edi
		sub esi, 2

		jg MC_avg_y_16_sse2_loop
	}
}

static void MC_avg_y_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

		movhpd xmm0, [edx] 
		movlpd xmm0, [edx+eax] 

	MC_put_y_8_sse2_loop:

		movhpd xmm1, [edx+eax] 
		movlpd xmm1, [edx+edi] 
		pavgb xmm0, xmm1 
		movhpd xmm2, [ecx] 
		movlpd xmm2, [ecx+eax] 
		pavgb xmm0, xmm2
		movhpd [ecx], xmm0 
		movlpd [ecx+eax], xmm0 
		movdqa xmm0, xmm1 
		add edx, edi 
		add ecx, edi
		sub esi, 2

		jg MC_put_y_8_sse2_loop
	}
}

static void MC_avg_xy_16_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref 
		mov ecx, dest
		mov eax, stride 
		mov esi, height 
		lea edi, [eax+eax] 
		
		movdqa xmm7, [const_1_16_bytes] 
		movdqu xmm0, [edx] 
		movdqu xmm1, [edx+1] 

	MC_avg_xy_16_sse2_loop:

		movdqu xmm2, [edx+eax] 
		movdqu xmm3, [edx+eax+1] 
		movdqu xmm4, [edx+edi] 
		movdqu xmm5, [edx+edi+1] 
		pavgb xmm0, xmm1 
		pavgb xmm2, xmm3 
		movdqa xmm1, xmm5 
		pavgb xmm5, xmm4 
		psubusb xmm2, xmm7 
		pavgb xmm0, xmm2 
		pavgb xmm2, xmm5
		movdqa xmm5, [ecx]
		movdqa xmm6, [ecx+eax]
		pavgb xmm0, xmm5 
		pavgb xmm2, xmm6
		movdqa [ecx], xmm0
		movdqa xmm0, xmm4
		movdqa [ecx+eax], xmm2
		add edx, edi
		add ecx, edi
		sub esi, 2

		jg MC_avg_xy_16_sse2_loop
	}
}

static void MC_avg_xy_8_sse2(uint8_t* dest, const uint8_t* ref, const int stride, int height)
{
	__asm
	{
		mov edx, ref
		mov ecx, dest
		mov eax, stride
		mov esi, height
		lea edi, [eax+eax]

		movdqa xmm7, [const_1_16_bytes] 
		movhpd xmm0, [edx] 
		movlpd xmm0, [edx+eax] 
		movhpd xmm2, [edx+1] 
		movlpd xmm2, [edx+eax+1] 

	MC_avg_xy_8_sse2_loop:

		movhpd xmm1, [edx+eax] 
		movlpd xmm1, [edx+edi] 
		movhpd xmm3, [edx+eax+1] 
		movlpd xmm3, [edx+edi+1] 
		pavgb xmm0, xmm1 
		pavgb xmm2, xmm3 
		psubusb xmm0, xmm7 
		pavgb xmm0, xmm2 
		movhpd xmm4, [ecx]
		movlpd xmm4, [ecx+eax]
		pavgb xmm0, xmm4 
		movhpd [ecx], xmm0 
		movlpd [ecx+eax], xmm0 
		movdqa xmm0, xmm1 
		movdqa xmm2, xmm3 
		add edx, edi 
		add ecx, edi
		sub esi, 2

		jg MC_avg_xy_8_sse2_loop
	}
}

mpeg2_mc_t mpeg2_mc_sse2 = 
{
	{MC_put_o_16_sse2, MC_put_x_16_sse2, MC_put_y_16_sse2, MC_put_xy_16_sse2,
	MC_put_o_8_sse2,  MC_put_x_8_sse2,  MC_put_y_8_sse2,  MC_put_xy_8_sse2},
	{MC_avg_o_16_sse2, MC_avg_x_16_sse2, MC_avg_y_16_sse2, MC_avg_xy_16_sse2,
	MC_avg_o_8_sse2,  MC_avg_x_8_sse2,  MC_avg_y_8_sse2,  MC_avg_xy_8_sse2}
};