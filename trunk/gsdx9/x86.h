#pragma once

#include "GS.h"

extern "C" void __fastcall memsetd(void* dst, unsigned int c, size_t len);
extern "C" UINT64 ticks();

extern "C" void unSwizzleBlock32_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock16_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock8_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock4_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock8HP_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock4HLP_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock4HHP_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock4P_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void SwizzleBlock32_amd64(BYTE* dst, BYTE* src, __int64 srcpitch, DWORD WriteMask = 0xffffffff);
extern "C" void SwizzleBlock16_amd64(BYTE* dst, BYTE* src, __int64 srcpitch);
extern "C" void SwizzleBlock8_amd64(BYTE* dst, BYTE* src, __int64 srcpitch);
extern "C" void SwizzleBlock4_amd64(BYTE* dst, BYTE* src, __int64 srcpitch);
extern "C" void __fastcall unSwizzleBlock32_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock16_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock8_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock4_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock8HP_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock4HLP_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock4HHP_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall unSwizzleBlock4P_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void __fastcall SwizzleBlock32_sse2(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask = 0xffffffff);
extern "C" void __fastcall SwizzleBlock16_sse2(BYTE* dst, BYTE* src, int srcpitch);
extern "C" void __fastcall SwizzleBlock8_sse2(BYTE* dst, BYTE* src, int srcpitch);
extern "C" void __fastcall SwizzleBlock4_sse2(BYTE* dst, BYTE* src, int srcpitch);
extern void __fastcall unSwizzleBlock32_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock16_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock8_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock4_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock8HP_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock4HLP_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock4HHP_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall unSwizzleBlock4P_c(BYTE* src, BYTE* dst, int dstpitch);
extern void __fastcall SwizzleBlock32_c(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask = 0xffffffff);
extern void __fastcall SwizzleBlock16_c(BYTE* dst, BYTE* src, int srcpitch);
extern void __fastcall SwizzleBlock8_c(BYTE* dst, BYTE* src, int srcpitch);
extern void __fastcall SwizzleBlock4_c(BYTE* dst, BYTE* src, int srcpitch);

extern "C" void ExpandBlock24_amd64(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA);
extern "C" void ExpandBlock16_amd64(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA);
extern "C" void __fastcall ExpandBlock24_sse2(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA);
extern "C" void __fastcall ExpandBlock16_sse2(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA);
extern void __fastcall ExpandBlock24_c(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA);
extern void __fastcall ExpandBlock16_c(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* pTEXA);

extern "C" void SaturateColor_amd64(int* c);
extern "C" void __fastcall SaturateColor_sse2(int* c);
extern "C" void __fastcall SaturateColor_asm(int* c);

struct uvmm_t {float umin, vmin, umax, vmax;};
struct vertex_t {float xyzw[4]; DWORD color[2]; float u, v;};
extern "C" void UVMinMax_amd64(int nVertices, vertex_t* pVertices, uvmm_t* uv);
extern "C" void __fastcall UVMinMax_sse2(int nVertices, vertex_t* pVertices, uvmm_t* uv);
extern "C" void __fastcall UVMinMax_c(int nVertices, vertex_t* pVertices, uvmm_t* uv);

#ifdef _M_AMD64

#define SaturateColor SaturateColor_amd64

#define unSwizzleBlock32 unSwizzleBlock32_amd64
#define unSwizzleBlock16 unSwizzleBlock16_amd64
#define unSwizzleBlock8 unSwizzleBlock8_amd64
#define unSwizzleBlock4 unSwizzleBlock4_amd64
#define unSwizzleBlock8HP unSwizzleBlock8HP_amd64
#define unSwizzleBlock4HLP unSwizzleBlock4HLP_amd64
#define unSwizzleBlock4HHP unSwizzleBlock4HHP_amd64
#define unSwizzleBlock4P unSwizzleBlock4P_amd64
#define SwizzleBlock32 SwizzleBlock32_amd64
#define SwizzleBlock16 SwizzleBlock16_amd64
#define SwizzleBlock8 SwizzleBlock8_amd64
#define SwizzleBlock4 SwizzleBlock4_amd64

#define ExpandBlock24 ExpandBlock24_amd64
#define ExpandBlock16 ExpandBlock16_amd64

#define UVMinMax UVMinMax_amd64

#elif _M_IX86_FP >= 2

#define SaturateColor SaturateColor_sse2

#define unSwizzleBlock32 unSwizzleBlock32_sse2
#define unSwizzleBlock16 unSwizzleBlock16_sse2
#define unSwizzleBlock8 unSwizzleBlock8_sse2
#define unSwizzleBlock4 unSwizzleBlock4_sse2
#define unSwizzleBlock8HP unSwizzleBlock8HP_sse2
#define unSwizzleBlock4HLP unSwizzleBlock4HLP_sse2
#define unSwizzleBlock4HHP unSwizzleBlock4HHP_sse2
#define unSwizzleBlock4P unSwizzleBlock4P_sse2
#define SwizzleBlock32 SwizzleBlock32_sse2
#define SwizzleBlock16 SwizzleBlock16_sse2
#define SwizzleBlock8 SwizzleBlock8_sse2
#define SwizzleBlock4 SwizzleBlock4_sse2

#define ExpandBlock24 ExpandBlock24_sse2
#define ExpandBlock16 ExpandBlock16_sse2

#define UVMinMax UVMinMax_sse2

#else

#define SaturateColor SaturateColor_asm

#define unSwizzleBlock32 unSwizzleBlock32_c
#define unSwizzleBlock16 unSwizzleBlock16_c
#define unSwizzleBlock8 unSwizzleBlock8_c
#define unSwizzleBlock4 unSwizzleBlock4_c
#define unSwizzleBlock8HP unSwizzleBlock8HP_c
#define unSwizzleBlock4HLP unSwizzleBlock4HLP_c
#define unSwizzleBlock4HHP unSwizzleBlock4HHP_c
#define unSwizzleBlock4P unSwizzleBlock4P_c
#define SwizzleBlock32 SwizzleBlock32_c
#define SwizzleBlock16 SwizzleBlock16_c
#define SwizzleBlock8 SwizzleBlock8_c
#define SwizzleBlock4 SwizzleBlock4_c

#define ExpandBlock24 ExpandBlock24_c
#define ExpandBlock16 ExpandBlock16_c

#define UVMinMax UVMinMax_c

#endif
