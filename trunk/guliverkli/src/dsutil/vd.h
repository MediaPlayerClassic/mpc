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

#pragma once

class CCpuID {public: CCpuID(); enum flag_t {mmx=1, ssemmx=2, ssefpu=4, sse2=8, _3dnow=16} m_flags;};
extern CCpuID g_cpuid;
extern void memcpy_accel(void* dst, const void* src, size_t len);
extern bool BitBltFromI420ToRGB(BYTE* dst, int pitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int bpp, int srcpitch = 0);
extern bool BitBltFromI420ToYUY2(BYTE* dst, int pitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int srcpitch = 0);
extern void DeinterlaceBlend(BYTE* dst, BYTE* src, DWORD rowbytes, DWORD h, DWORD pitch);
