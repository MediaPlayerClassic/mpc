#pragma once

extern "C" void memsetd(void* dst, unsigned int c, size_t len);
extern "C" UINT64 ticks();

#ifdef _M_AMD64

extern "C" void SaturateColor_amd64(int* c);
#define SaturateColor SaturateColor_amd64

extern "C" void unSwizzleBlock32_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock16_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock8_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void unSwizzleBlock4_amd64(BYTE* src, BYTE* dst, __int64 dstpitch);
extern "C" void SwizzleBlock32_amd64(BYTE* dst, BYTE* src, __int64 srcpitch, DWORD WriteMask = 0xffffffff);
extern "C" void SwizzleBlock16_amd64(BYTE* dst, BYTE* src, __int64 srcpitch);
extern "C" void SwizzleBlock8_amd64(BYTE* dst, BYTE* src, __int64 srcpitch);
extern "C" void SwizzleBlock4_amd64(BYTE* dst, BYTE* src, __int64 srcpitch);

#define unSwizzleBlock32 unSwizzleBlock32_amd64
#define unSwizzleBlock16 unSwizzleBlock16_amd64
#define unSwizzleBlock8 unSwizzleBlock8_amd64
#define unSwizzleBlock4 unSwizzleBlock4_amd64
#define SwizzleBlock32 SwizzleBlock32_amd64
#define SwizzleBlock16 SwizzleBlock16_amd64
#define SwizzleBlock8 SwizzleBlock8_amd64
#define SwizzleBlock4 SwizzleBlock4_amd64

#elif _M_IX86_FP >= 2

extern "C" void SaturateColor_sse2(int* c);
#define SaturateColor SaturateColor_sse2

extern "C" void unSwizzleBlock32_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void unSwizzleBlock16_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void unSwizzleBlock8_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void unSwizzleBlock4_sse2(BYTE* src, BYTE* dst, int dstpitch);
extern "C" void SwizzleBlock32_sse2(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask = 0xffffffff);
extern "C" void SwizzleBlock16_sse2(BYTE* dst, BYTE* src, int srcpitch);
extern "C" void SwizzleBlock8_sse2(BYTE* dst, BYTE* src, int srcpitch);
extern "C" void SwizzleBlock4_sse2(BYTE* dst, BYTE* src, int srcpitch);

#define unSwizzleBlock32 unSwizzleBlock32_sse2
#define unSwizzleBlock16 unSwizzleBlock16_sse2
#define unSwizzleBlock8 unSwizzleBlock8_sse2
#define unSwizzleBlock4 unSwizzleBlock4_sse2
#define SwizzleBlock32 SwizzleBlock32_sse2
#define SwizzleBlock16 SwizzleBlock16_sse2
#define SwizzleBlock8 SwizzleBlock8_sse2
#define SwizzleBlock4 SwizzleBlock4_sse2

#else

extern "C" void SaturateColor_asm(int* c);
#define SaturateColor SaturateColor_asm

extern void unSwizzleBlock32_c(BYTE* src, BYTE* dst, int dstpitch);
extern void unSwizzleBlock16_c(BYTE* src, BYTE* dst, int dstpitch);
extern void unSwizzleBlock8_c(BYTE* src, BYTE* dst, int dstpitch);
extern void unSwizzleBlock4_c(BYTE* src, BYTE* dst, int dstpitch);
extern void SwizzleBlock32_c(BYTE* dst, BYTE* src, int srcpitch, DWORD WriteMask = 0xffffffff);
extern void SwizzleBlock16_c(BYTE* dst, BYTE* src, int srcpitch);
extern void SwizzleBlock8_c(BYTE* dst, BYTE* src, int srcpitch);
extern void SwizzleBlock4_c(BYTE* dst, BYTE* src, int srcpitch);

#define unSwizzleBlock32 unSwizzleBlock32_c
#define unSwizzleBlock16 unSwizzleBlock16_c
#define unSwizzleBlock8 unSwizzleBlock8_c
#define unSwizzleBlock4 unSwizzleBlock4_c
#define SwizzleBlock32 SwizzleBlock32_c
#define SwizzleBlock16 SwizzleBlock16_c
#define SwizzleBlock8 SwizzleBlock8_c
#define SwizzleBlock4 SwizzleBlock4_c

#endif
