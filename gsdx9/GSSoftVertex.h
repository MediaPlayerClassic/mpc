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
 */

#pragma once

#include <xmmintrin.h>

__declspec(align(16)) union GSSoftVertex
{
	struct
	{
		float x, y, z, q;
		float r, g, b, a;
		float u, v, fog, reserved;
	};

	struct
	{
		float f[12];
	};

	struct
	{
		__m128 xmm[3];
	};

	GSSoftVertex& operator = (GSSoftVertex& v)
	{
//		memcpy(f, v.f, sizeof(f));
		for(int i = 0; i < countof(xmm); i++) xmm[i] = v.xmm[i];
		return *this;
	}

	void operator += (GSSoftVertex& v)
	{
//		for(int i = 0; i < countof(f); i++) f[i] += v.f[i];
		for(int i = 0; i < countof(xmm); i++) xmm[i] = _mm_add_ps(xmm[i], v.xmm[i]);
	}

	friend GSSoftVertex operator + (GSSoftVertex& v1, GSSoftVertex& v2);
	friend GSSoftVertex operator - (GSSoftVertex& v1, GSSoftVertex& v2);
	friend GSSoftVertex operator * (GSSoftVertex& v1, float f);
	friend GSSoftVertex operator / (GSSoftVertex& v1, float f);
};

inline GSSoftVertex operator + (GSSoftVertex& v1, GSSoftVertex& v2)
{
	GSSoftVertex v0;
//	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] + v2.f[i];
	for(int i = 0; i < countof(v0.xmm); i++) v0.xmm[i] = _mm_add_ps(v1.xmm[i], v2.xmm[i]);
	return v0;
}

inline GSSoftVertex operator - (GSSoftVertex& v1, GSSoftVertex& v2)
{
	GSSoftVertex v0;
//	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] - v2.f[i];
	for(int i = 0; i < countof(v0.xmm); i++) v0.xmm[i] = _mm_sub_ps(v1.xmm[i], v2.xmm[i]);
	return v0;
}

inline GSSoftVertex operator * (GSSoftVertex& v1, float f)
{
	GSSoftVertex v0;
//	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] * f;
	__m128 f128 = _mm_set_ps1(f);
	for(int i = 0; i < countof(v0.xmm); i++) v0.xmm[i] = _mm_mul_ps(v1.xmm[i], f128);
	return v0;
}

inline GSSoftVertex operator / (GSSoftVertex& v1, float f)
{
	GSSoftVertex v0;
//	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] / f;
	__m128 f128 = _mm_set_ps1(f);
	for(int i = 0; i < countof(v0.xmm); i++) v0.xmm[i] = _mm_div_ps(v1.xmm[i], f128);
	return v0;
}

