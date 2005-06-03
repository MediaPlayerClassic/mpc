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

//
// GSSoftVertexFP
//

__declspec(align(16)) union GSSoftVertexFP
{
	struct
	{
		float r, g, b, a;
		float x, y, z, q;
		float u, v, fog, reserved;
	};

	struct {float f[12];};
	struct {__m128 xmm[3];};

#if _M_IX86_FP >= 1 || defined(_M_AMD64)
	GSSoftVertexFP& operator = (GSSoftVertexFP& v)
	{
		xmm[0] = v.xmm[0];
		xmm[1] = v.xmm[1];
		xmm[2] = v.xmm[2];
		return *this;
	}
#endif

	void operator += (GSSoftVertexFP& v)
	{
#if _M_IX86_FP >= 1 || defined(_M_AMD64)
		xmm[0] = _mm_add_ps(xmm[0], v.xmm[0]);
		xmm[1] = _mm_add_ps(xmm[1], v.xmm[1]);
		xmm[2] = _mm_add_ps(xmm[2], v.xmm[2]);
#else
		for(int i = 0; i < countof(f); i++) f[i] += v.f[i];
#endif
	}

	operator CPoint() {CPoint p(Int(x), Int(y)); return p;}

	friend GSSoftVertexFP operator + (GSSoftVertexFP& v1, GSSoftVertexFP& v2);
	friend GSSoftVertexFP operator - (GSSoftVertexFP& v1, GSSoftVertexFP& v2);
	friend GSSoftVertexFP operator * (GSSoftVertexFP& v1, float f);
	friend GSSoftVertexFP operator / (GSSoftVertexFP& v1, float f);

	typedef float scalar_t;
	static scalar_t Scalar(int i) {return (scalar_t)i;}
	static scalar_t Div(scalar_t f1, scalar_t f2) {return f1 / f2;}
	static scalar_t Mul(scalar_t f1, scalar_t f2) {return f1 * f2;}
	static scalar_t Ceil(scalar_t f) {return ceil(f);}
	static int CeilInt(scalar_t f) {return (int)ceil(f);}
	static int Int(scalar_t f) {return (int)f;}
	static void Exchange(GSSoftVertexFP& v1, GSSoftVertexFP& v2)
	{
#if _M_IX86_FP >= 1 || defined(_M_AMD64)
		__m128 r0 = v1.xmm[0], r1 = v2.xmm[0];
		__m128 r2 = v1.xmm[1], r3 = v2.xmm[1];
		__m128 r4 = v1.xmm[2], r5 = v2.xmm[2];
		v1.xmm[0] = r1, v2.xmm[0] = r0;
		v1.xmm[1] = r3, v2.xmm[1] = r2;
		v1.xmm[2] = r5, v2.xmm[2] = r4;
#else
		GSSoftVertexFP v = v1; v1 = v2; v2 = v;
#endif
	}
	
	DWORD GetZ() {return (DWORD)(z * UINT_MAX);}
	BYTE GetFog() {return (BYTE)z;}
	void GetColor(void* pRGBA)
	{
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		*(__m128i*)pRGBA = _mm_cvttps_epi32(xmm[0]);
#else
		int* p = (int*)pRGBA;
		for(int i = 0; i < 4; i++) *p++ = (int)f[i];
#endif
	}
};

inline GSSoftVertexFP operator + (GSSoftVertexFP& v1, GSSoftVertexFP& v2)
{
	GSSoftVertexFP v0;
#if _M_IX86_FP >= 1 || defined(_M_AMD64)
	v0.xmm[0] = _mm_add_ps(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_add_ps(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_add_ps(v1.xmm[2], v2.xmm[2]);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] + v2.f[i];
#endif
	return v0;
}

inline GSSoftVertexFP operator - (GSSoftVertexFP& v1, GSSoftVertexFP& v2)
{
	GSSoftVertexFP v0;
#if _M_IX86_FP >= 1 || defined(_M_AMD64)
	v0.xmm[0] = _mm_sub_ps(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_sub_ps(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_sub_ps(v1.xmm[2], v2.xmm[2]);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] - v2.f[i];
#endif
	return v0;
}

inline GSSoftVertexFP operator * (GSSoftVertexFP& v1, float f)
{
	GSSoftVertexFP v0;
#if _M_IX86_FP >= 1 || defined(_M_AMD64)
	__m128 f128 = _mm_set_ps1(f);
	v0.xmm[0] = _mm_mul_ps(v1.xmm[0], f128);
	v0.xmm[1] = _mm_mul_ps(v1.xmm[1], f128);
	v0.xmm[2] = _mm_mul_ps(v1.xmm[2], f128);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] * f;
#endif
	return v0;
}

inline GSSoftVertexFP operator / (GSSoftVertexFP& v1, float f)
{
	GSSoftVertexFP v0;
#if _M_IX86_FP >= 1 || defined(_M_AMD64)
	__m128 f128 = _mm_set_ps1(f);
	v0.xmm[0] = _mm_div_ps(v1.xmm[0], f128);
	v0.xmm[1] = _mm_div_ps(v1.xmm[1], f128);
	v0.xmm[2] = _mm_div_ps(v1.xmm[2], f128);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] / f;
#endif
	return v0;
}

//
// GSSoftVertexFX
//

__declspec(align(16)) union GSSoftVertexFX
{
	typedef signed int s32;
	typedef unsigned int u32;
	typedef signed __int64 s64;
	typedef unsigned __int64 u64;

	struct
	{
		s32 r, g, b, a;
		s32 x, y, fog, reserved;
		s64 z, q;
		s64 u, v;
	};

	struct {s32 f[16];};
	struct {s32 dw[16];};
	struct {s64 qw[8];};
	struct {__m128i xmm[4];};

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
	GSSoftVertexFX& operator = (GSSoftVertexFX& v)
	{
		xmm[0] = v.xmm[0];
		xmm[1] = v.xmm[1];
		xmm[2] = v.xmm[2];
		xmm[3] = v.xmm[3];
		return *this;
	}
#endif

	void operator += (GSSoftVertexFX& v)
	{
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		xmm[0] = _mm_add_epi32(xmm[0], v.xmm[0]);
		xmm[1] = _mm_add_epi32(xmm[1], v.xmm[1]);
		xmm[2] = _mm_add_epi64(xmm[2], v.xmm[2]);
		xmm[3] = _mm_add_epi64(xmm[3], v.xmm[3]);
#else
		for(int i = 0; i < 8; i++) dw[i] += v.dw[i];
		for(int i = 4; i < 8; i++) qw[i] += v.qw[i];
#endif
	}

	operator CPoint() {CPoint p(Int(x), Int(y)); return p;}

	friend GSSoftVertexFX operator + (GSSoftVertexFX& v1, GSSoftVertexFX& v2);
	friend GSSoftVertexFX operator - (GSSoftVertexFX& v1, GSSoftVertexFX& v2);
	friend GSSoftVertexFX operator * (GSSoftVertexFX& v1, s32 f);
	friend GSSoftVertexFX operator / (GSSoftVertexFX& v1, s32 f);

	typedef int scalar_t;
	static scalar_t Scalar(int i) {return (scalar_t)(i << 16);}
	static scalar_t Mul(scalar_t i1, scalar_t i2) {return (int)(((__int64)i1 * i2 + 0x8000) >> 16);}
	static scalar_t Div(scalar_t i1, scalar_t i2) {return (int)(((__int64)i1 << 16) / i2);}
	static scalar_t Ceil(scalar_t i) {return (i + 0xffff) & 0xffff0000;}
	static scalar_t CeilInt(scalar_t i) {return (i + 0xffff) >> 16;}
	static int Int(scalar_t i) {return i >> 16;}
	static void Exchange(GSSoftVertexFX& v1, GSSoftVertexFX& v2)
	{
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		__m128i r0 = v1.xmm[0], r1 = v2.xmm[0];
		__m128i r2 = v1.xmm[1], r3 = v2.xmm[1];
		__m128i r4 = v1.xmm[2], r5 = v2.xmm[2];
		__m128i r6 = v1.xmm[3], r7 = v2.xmm[3];
		v1.xmm[0] = r1, v2.xmm[0] = r0;
		v1.xmm[1] = r3, v2.xmm[1] = r2;
		v1.xmm[2] = r5, v2.xmm[2] = r4;
		v1.xmm[3] = r7, v2.xmm[3] = r6;
#else
		GSSoftVertexFX v = v1; v1 = v2; v2 = v;
#endif
	}

	DWORD GetZ() {return (DWORD)(z >> 32);}
	BYTE GetFog() {return (BYTE)(fog >> 16);}
	void GetColor(void* pRGBA)
	{
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		*(__m128i*)pRGBA = _mm_srai_epi32(xmm[0], 16);
#else
		int* p = (int*)pRGBA;
		for(int i = 0; i < 4; i++) *p++ = dw[i] >> 16;
#endif
	}
};

//
// GSSoftVertexFX
//

inline GSSoftVertexFX operator + (GSSoftVertexFX& v1, GSSoftVertexFX& v2)
{
	GSSoftVertexFX v0;
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
	v0.xmm[0] = _mm_add_epi32(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_add_epi32(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_add_epi64(v1.xmm[2], v2.xmm[2]);
	v0.xmm[3] = _mm_add_epi64(v1.xmm[3], v2.xmm[3]);
#else
	for(int i = 0; i < 8; i++) v0.dw[i] = v1.dw[i] + v2.dw[i];
	for(int i = 4; i < 8; i++) v0.qw[i] = v1.qw[i] + v2.qw[i];
#endif
	return v0;
}

inline GSSoftVertexFX operator - (GSSoftVertexFX& v1, GSSoftVertexFX& v2)
{
	GSSoftVertexFX v0;
#if _M_IX86_FP >= 2 || defined(_M_AMD64)
	v0.xmm[0] = _mm_sub_epi32(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_sub_epi32(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_sub_epi64(v1.xmm[2], v2.xmm[2]);
	v0.xmm[3] = _mm_sub_epi64(v1.xmm[3], v2.xmm[3]);
#else
	for(int i = 0; i < 8; i++) v0.dw[i] = v1.dw[i] - v2.dw[i];
	for(int i = 4; i < 8; i++) v0.qw[i] = v1.qw[i] - v2.qw[i];
#endif
	return v0;
}

inline GSSoftVertexFX operator * (GSSoftVertexFX& v1, GSSoftVertexFX::s32 f)
{
	GSSoftVertexFX v0;
	float f2 = (float)f / 65536.0f;
#ifndef _M_AMD64
	GSSoftVertexFX::s32* src = v1.dw;
	GSSoftVertexFX::s32* dst = v0.dw;
	__asm
	{
		push	ebx
		mov		esi, src
		mov		edi, dst
		sub		edi, esi
		mov		ebx, f
		mov		ecx, 8
	l1:
		mov		eax, [esi]
		imul	ebx
		shrd	eax, edx, 16
		mov		[esi+edi], eax
		lea		esi, [esi+4]
		dec		ecx
		jnz		l1
		pop		ebx
	}
#else
	for(int i = 0; i < 8; i++) v0.dw[i] = (GSSoftVertexFX::s32)((float)v1.dw[i] * f2);
#endif
	for(int i = 4; i < 8; i++) v0.qw[i] = (GSSoftVertexFX::s64)((float)v1.qw[i] * f2);
	return v0;
}

inline GSSoftVertexFX operator / (GSSoftVertexFX& v1, GSSoftVertexFX::s32 f)
{
	GSSoftVertexFX v0;
	float f2 = 65536.0f / f;
	for(int i = 0; i < 8; i++) v0.dw[i] = (GSSoftVertexFX::s32)((float)v1.dw[i] * f2);
	for(int i = 4; i < 8; i++) v0.qw[i] = (GSSoftVertexFX::s64)((float)v1.qw[i] * f2);
	return v0;
}
