#include "stdafx.h"
#include "GSTables.h"
#include "x86.h"

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
				dst[i] = ((src[i]&0x8000) ? TA1 : src[i] ? TA0 : 0)
					| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
}

void __fastcall Expand16_c(WORD* src, DWORD* dst, int w, GIFRegTEXA* pTEXA)
{
	DWORD TA0 = (DWORD)pTEXA->TA0 << 24;
	DWORD TA1 = (DWORD)pTEXA->TA1 << 24;

	if(!pTEXA->AEM)
	{
		for(int i = 0; i < w; i++)
			dst[i] = ((src[i]&0x8000) ? TA1 : TA0)
				| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
	else
	{
		for(int i = 0; i < w; i++)
			dst[i] = ((src[i]&0x8000) ? TA1 : src[i] ? TA0 : 0)
				| ((src[i]&0x7c00) << 9) | ((src[i]&0x03e0) << 6) | ((src[i]&0x001f) << 3);
	}
}

#if defined(_M_AMD64) || _M_IX86_FP >= 2

static __m128i s_zero = _mm_setzero_si128();
static __m128i s_bgrm = _mm_set1_epi32(0x00ffffff);
static __m128i s_am = _mm_set1_epi32(0x00008000);
static __m128i s_bm = _mm_set1_epi32(0x00007c00);
static __m128i s_gm = _mm_set1_epi32(0x000003e0);
static __m128i s_rm = _mm_set1_epi32(0x0000001f);

void __fastcall ExpandBlock24_sse2(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	__m128i TA0 = _mm_set1_epi32((DWORD)pTEXA->TA0 << 24);

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
		{
			for(int i = 0; i < 8; i += 4)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);
				c = _mm_and_si128(c, s_bgrm);
				c = _mm_or_si128(c, TA0);
				_mm_store_si128((__m128i*)&dst[i], c);
			}
		}
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 8, dst += dstpitch>>2)
		{
			for(int i = 0; i < 8; i += 4)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);
				c = _mm_and_si128(c, s_bgrm);
				__m128i a = _mm_andnot_si128(_mm_cmpeq_epi16(c, s_zero), TA0);
				c = _mm_or_si128(c, a);
				_mm_store_si128((__m128i*)&dst[i], c);
			}
		}
	}

}

void __fastcall ExpandBlock16_sse2(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA)
{
	__m128i TA0 = _mm_set1_epi32((DWORD)pTEXA->TA0 << 24);
	__m128i TA1 = _mm_set1_epi32((DWORD)pTEXA->TA1 << 24);
	__m128i a, b, g, r;

	if(!pTEXA->AEM)
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
		{
			for(int i = 0; i < 16; i += 8)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);

				__m128i cl = _mm_unpacklo_epi16(c, s_zero);
				__m128i ch = _mm_unpackhi_epi16(c, s_zero);

				__m128i alm = _mm_cmplt_epi32(cl, s_am);
				__m128i ahm = _mm_cmplt_epi32(ch, s_am);

				// lo

				b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));

				cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i], cl);

				// hi

				b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));

				ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i+4], ch);
			}
		}
	}
	else
	{
		for(int j = 0; j < 8; j++, src += 16, dst += dstpitch>>2)
		{
			for(int i = 0; i < 16; i += 8)
			{
				__m128i c = _mm_load_si128((__m128i*)&src[i]);

				__m128i cl = _mm_unpacklo_epi16(c, s_zero);
				__m128i ch = _mm_unpackhi_epi16(c, s_zero);

				__m128i alm = _mm_cmplt_epi32(cl, s_am);
				__m128i ahm = _mm_cmplt_epi32(ch, s_am);

				__m128i trm = _mm_cmpeq_epi16(c, s_zero);
				__m128i trlm = _mm_unpacklo_epi16(trm, trm);
				__m128i trhm = _mm_unpackhi_epi16(trm, trm);

				// lo

				b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));
				a = _mm_andnot_si128(trlm, a);

				cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i], cl);

				// hi

				b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
				g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
				r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
				a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));
				a = _mm_andnot_si128(trhm, a);

				ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

				_mm_store_si128((__m128i*)&dst[i+4], ch);
			}
		}
	}
}

void __fastcall Expand16_sse2(WORD* src, DWORD* dst, int w, GIFRegTEXA* pTEXA)
{
	ASSERT(!(w&7));

	__m128i TA0 = _mm_set1_epi32((DWORD)pTEXA->TA0 << 24);
	__m128i TA1 = _mm_set1_epi32((DWORD)pTEXA->TA1 << 24);
	__m128i a, b, g, r;

	if(!pTEXA->AEM)
	{
		for(int i = 0; i < w; i += 8)
		{
			__m128i c = _mm_load_si128((__m128i*)&src[i]);

			__m128i cl = _mm_unpacklo_epi16(c, s_zero);
			__m128i ch = _mm_unpackhi_epi16(c, s_zero);

			__m128i alm = _mm_cmplt_epi32(cl, s_am);
			__m128i ahm = _mm_cmplt_epi32(ch, s_am);

			// lo

			b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));

			cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i], cl);

			// hi

			b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));

			ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i+4], ch);
		}
	}
	else
	{
		for(int i = 0; i < w; i += 8)
		{
			__m128i c = _mm_load_si128((__m128i*)&src[i]);

			__m128i cl = _mm_unpacklo_epi16(c, s_zero);
			__m128i ch = _mm_unpackhi_epi16(c, s_zero);

			__m128i alm = _mm_cmplt_epi32(cl, s_am);
			__m128i ahm = _mm_cmplt_epi32(ch, s_am);

			__m128i trm = _mm_cmpeq_epi16(c, s_zero);
			__m128i trlm = _mm_unpacklo_epi16(trm, trm);
			__m128i trhm = _mm_unpackhi_epi16(trm, trm);

			// lo

			b = _mm_slli_epi32(_mm_and_si128(cl, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(cl, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(cl, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(alm, TA0), _mm_andnot_si128(alm, TA1));
			a = _mm_andnot_si128(trlm, a);

			cl = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i], cl);

			// hi

			b = _mm_slli_epi32(_mm_and_si128(ch, s_bm), 9);
			g = _mm_slli_epi32(_mm_and_si128(ch, s_gm), 6);
			r = _mm_slli_epi32(_mm_and_si128(ch, s_rm), 3);
			a = _mm_or_si128(_mm_and_si128(ahm, TA0), _mm_andnot_si128(ahm, TA1));
			a = _mm_andnot_si128(trhm, a);

			ch = _mm_or_si128(_mm_or_si128(a, b), _mm_or_si128(g, r));

			_mm_store_si128((__m128i*)&dst[i+4], ch);
		}
	}
}

#endif

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

#if defined(_M_AMD64) || _M_IX86_FP >= 2

static __m128 s_uvmin = _mm_set1_ps(+1e10);
static __m128 s_uvmax = _mm_set1_ps(-1e10);

void __fastcall UVMinMax_sse2(int nVertices, vertex_t* pVertices, uvmm_t* uv)
{
	__m128 uvmin = s_uvmin;
	__m128 uvmax = s_uvmax;

	__m128* p = (__m128*)pVertices + 1;

	for(int i = 0; i < nVertices; i++)
	{
		uvmin = _mm_min_ps(uvmin, p[i*2]);
		uvmax = _mm_max_ps(uvmax, p[i*2]);
	}

	_mm_storeh_pi((__m64*)uv, uvmin);
	_mm_storeh_pi((__m64*)uv + 1, uvmax);
}

#endif
