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

bool BitBltFromI420(BYTE* dst, int pitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int bpp)
{
	if(w<=0 || h<=0 || (w&7) || (h&1))
		return(false);

	switch(bpp) {
	case 16:
		do {
			asm_YUVtoRGB16_row(dst + pitch, dst, srcy + w, srcy, srcu, srcv, w/2);

			dst += 2*pitch;
			srcy += w*2;
			srcu += w/2;
			srcv += w/2;
		} while(h -= 2);
		break;

	case 24:
		do {
			asm_YUVtoRGB24_row(dst + pitch, dst, srcy + w, srcy, srcu, srcv, w/2);

			dst += 2*pitch;
			srcy += w*2;
			srcu += w/2;
			srcv += w/2;
		} while(h -= 2);
		break;

	case 32:
		do {
			asm_YUVtoRGB32_row(dst + pitch, dst, srcy + w, srcy, srcu, srcv, w/2);

			dst += 2*pitch;
			srcy += w*2;
			srcu += w/2;
			srcv += w/2;
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
