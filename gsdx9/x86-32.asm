
	.686
	.model flat
	.mmx
	.xmm
	.code
	
;
; memsetd
;

_memsetd proc public

	push	edi

	mov		edi, [esp+4+4]
	mov		eax, [esp+8+4]
	mov		ecx, [esp+12+4]
	cld
	rep		stosd
	
	pop		edi

	ret
	
_memsetd endp

;
; ticks
;

_ticks proc public

	rdtsc
	ret

_ticks endp

;
; SaturateColor
;
			
_SaturateColor_sse2 proc public

	mov			eax, [esp+4]

	pxor		xmm0, xmm0
	movaps		xmm1, [eax]
	packssdw	xmm1, xmm0
	packuswb	xmm1, xmm0
	punpcklbw	xmm1, xmm0
	punpcklwd	xmm1, xmm0
	movaps		[eax], xmm1

	ret

_SaturateColor_sse2 endp

_SaturateColor_asm proc public

	push	esi

	mov		esi, [esp+4+4]

	xor		eax, eax
	mov		edx, 000000ffh

	mov		ecx, [esi]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi], ecx

	mov		ecx, [esi+4]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi+4], ecx

	mov		ecx, [esi+8]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi+8], ecx

	mov		ecx, [esi+12]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi+12], ecx
	
	pop		esi
	
_SaturateColor_asm endp

;
; swizzling
;

punpck macro op, sd0, sd2, s1, s3, d1, d3

	movaps					@CatStr(xmm, %d1),	@CatStr(xmm, %sd0)
	pshufd					@CatStr(xmm, %d3),	@CatStr(xmm, %sd2), 0e4h
	
	@CatStr(punpckl, op)	@CatStr(xmm, %sd0),	@CatStr(xmm, %s1)
	@CatStr(punpckh, op)	@CatStr(xmm, %d1),	@CatStr(xmm, %s1)
	@CatStr(punpckl, op)	@CatStr(xmm, %sd2),	@CatStr(xmm, %s3)
	@CatStr(punpckh, op)	@CatStr(xmm, %d3),	@CatStr(xmm, %s3)

	endm
	
punpcknb macro

	movaps	xmm4, xmm0
	pshufd	xmm5, xmm1, 0e4h

	psllq	xmm1, 4
	psrlq	xmm4, 4

	movaps	xmm6, xmm7
	pand	xmm0, xmm7
	pandn	xmm6, xmm1
	por		xmm0, xmm6

	movaps	xmm6, xmm7
	pand	xmm4, xmm7
	pandn	xmm6, xmm5
	por		xmm4, xmm6

	movaps	xmm1, xmm4

	movaps	xmm4, xmm2
	pshufd	xmm5, xmm3, 0e4h

	psllq	xmm3, 4
	psrlq	xmm4, 4

	movaps	xmm6, xmm7
	pand	xmm2, xmm7
	pandn	xmm6, xmm3
	por		xmm2, xmm6

	movaps	xmm6, xmm7
	pand	xmm4, xmm7
	pandn	xmm6, xmm5
	por		xmm4, xmm6

	movaps	xmm3, xmm4

	punpck	bw, 0, 2, 1, 3, 4, 6

	endm

;
; unSwizzleBlock32
;

_unSwizzleBlock32_sse2 proc public

	push		esi
	push		edi

	mov			esi, [esp+4+8]
	mov			edi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 4

	align 16
@@:
	movaps		xmm0, [esi+16*0]
	movaps		xmm1, [esi+16*1]
	movaps		xmm2, [esi+16*2]
	movaps		xmm3, [esi+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6

	movaps		[edi], xmm0
	movaps		[edi+16], xmm2
	movaps		[edi+edx], xmm4
	movaps		[edi+edx+16], xmm6

	add			esi, 64
	lea			edi, [edi+edx*2]

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret

_unSwizzleBlock32_sse2 endp

;
; unSwizzleBlock16
;

_unSwizzleBlock16_sse2 proc public

	push		esi
	push		edi

	mov			esi, [esp+4+8]
	mov			edi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 4
	
	align 16
@@:
	movaps		xmm0, [esi+16*0]
	movaps		xmm1, [esi+16*1]
	movaps		xmm2, [esi+16*2]
	movaps		xmm3, [esi+16*3]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		dq, 0, 4, 2, 6, 1, 3
	punpck		wd, 0, 4, 1, 3, 2, 6

	movaps		[edi], xmm0
	movaps		[edi+16], xmm2
	movaps		[edi+edx], xmm4
	movaps		[edi+edx+16], xmm6

	add			esi, 64
	lea			edi, [edi+edx*2]

	dec			ecx
	jnz			@B
	
	pop			edi
	pop			esi
	
	ret
	
_unSwizzleBlock16_sse2 endp

;
; unSwizzleBlock8
;

_unSwizzleBlock8_sse2 proc public

	push		esi
	push		edi

	mov			esi, [esp+4+8]
	mov			edi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 2

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [esi+16*0]
	movaps		xmm1, [esi+16*1]
	movaps		xmm4, [esi+16*2]
	movaps		xmm5, [esi+16*3]

	punpck		bw,  0, 4, 1, 5, 2, 6
	punpck		wd,  0, 2, 4, 6, 1, 3
	punpck		bw,  0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshufd		xmm1, xmm1, 0b1h
	pshufd		xmm3, xmm3, 0b1h

	movaps		[edi], xmm0
	movaps		[edi+edx], xmm2
	lea			edi, [edi+edx*2]

	movaps		[edi], xmm1
	movaps		[edi+edx], xmm3
	lea			edi, [edi+edx*2]

	; col 1, 3

	movaps		xmm0, [esi+16*4]
	movaps		xmm1, [esi+16*5]
	movaps		xmm4, [esi+16*6]
	movaps		xmm5, [esi+16*7]

	punpck		bw,  0, 4, 1, 5, 2, 6
	punpck		wd,  0, 2, 4, 6, 1, 3
	punpck		bw,  0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshufd		xmm0, xmm0, 0b1h
	pshufd		xmm2, xmm2, 0b1h

	movaps		[edi], xmm0
	movaps		[edi+edx], xmm2
	lea			edi, [edi+edx*2]

	movaps		[edi], xmm1
	movaps		[edi+edx], xmm3
	lea			edi, [edi+edx*2]

	add			esi, 128

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi
	
	ret

_unSwizzleBlock8_sse2 endp

;
; unSwizzleBlock4
;

_unSwizzleBlock4_sse2 proc public

	push		esi
	push		edi

	mov			esi, [esp+4+8]
	mov			edi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 2

	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [esi+16*0]
	movaps		xmm1, [esi+16*1]
	movaps		xmm4, [esi+16*2]
	movaps		xmm3, [esi+16*3]

	punpck		dq, 0, 4, 1, 3, 2, 6
	punpck		dq, 0, 2, 4, 6, 1, 3
	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		wd, 0, 2, 1, 3, 4, 6

	pshufd		xmm0, xmm0, 0d8h
	pshufd		xmm2, xmm2, 0d8h
	pshufd		xmm4, xmm4, 0d8h
	pshufd		xmm6, xmm6, 0d8h

	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	movaps		[edi], xmm0
	movaps		[edi+edx], xmm2
	lea			edi, [edi+edx*2]

	movaps		[edi], xmm1
	movaps		[edi+edx], xmm3
	lea			edi, [edi+edx*2]

	; col 1, 3

	movaps		xmm0, [esi+16*4]
	movaps		xmm1, [esi+16*5]
	movaps		xmm4, [esi+16*6]
	movaps		xmm3, [esi+16*7]

	punpck		dq, 0, 4, 1, 3, 2, 6
	punpck		dq, 0, 2, 4, 6, 1, 3
	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		wd, 0, 2, 1, 3, 4, 6

	pshufd		xmm0, xmm0, 0d8h
	pshufd		xmm2, xmm2, 0d8h
	pshufd		xmm4, xmm4, 0d8h
	pshufd		xmm6, xmm6, 0d8h

	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	movaps		[edi], xmm0
	movaps		[edi+edx], xmm2
	lea			edi, [edi+edx*2]

	movaps		[edi], xmm1
	movaps		[edi+edx], xmm3
	lea			edi, [edi+edx*2]

	add			esi, 128

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi
	
	ret

_unSwizzleBlock4_sse2 endp

;
; swizzling
;

;
; SwizzleBlock32_sse2
;

_SwizzleBlock32_sse2 proc public

	push		esi
	push		edi

	mov			edi, [esp+4+8]
	mov			esi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 4

	mov			eax, [esp+16+8]
	test		eax, 0ffffffffh
	jnz			SwizzleBlock32_sse2@WM

	align 16
@@:
	movaps		xmm0, [esi]
	movaps		xmm4, [esi+16]
	movaps		xmm1, [esi+edx]
	movaps		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movaps		[edi+16*0], xmm0
	movaps		[edi+16*1], xmm2
	movaps		[edi+16*2], xmm4
	movaps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret

SwizzleBlock32_sse2@WM:

	movd		xmm7, eax
	pshufd		xmm7, xmm7, 0
	
	align 16
@@:
	movaps		xmm0, [esi]
	movaps		xmm4, [esi+16]
	movaps		xmm1, [esi+edx]
	movaps		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movaps		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*0]
	pand		xmm0, xmm7
	por			xmm0, xmm3
	movaps		[edi+16*0], xmm0

	pandn		xmm5, [edi+16*1]
	pand		xmm2, xmm7
	por			xmm2, xmm5
	movaps		[edi+16*1], xmm2

	movaps		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*2]
	pand		xmm4, xmm7
	por			xmm4, xmm3
	movaps		[edi+16*2], xmm4

	pandn		xmm5, [edi+16*3]
	pand		xmm6, xmm7
	por			xmm6, xmm5
	movaps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret
	
_SwizzleBlock32_sse2 endp

;
; SwizzleBlock16
;

_SwizzleBlock16_sse2 proc public

	push		esi
	push		edi

	mov			edi, [esp+4+8]
	mov			esi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 4

	align 16
@@:
	movaps		xmm0, [esi]
	movaps		xmm1, [esi+16]
	movaps		xmm2, [esi+edx]
	movaps		xmm3, [esi+edx+16]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 5

	movaps		[edi+16*0], xmm0
	movaps		[edi+16*1], xmm1
	movaps		[edi+16*2], xmm4
	movaps		[edi+16*3], xmm5

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret

_SwizzleBlock16_sse2 endp

;
; SwizzleBlock8
;

_SwizzleBlock8_sse2 proc public

	push		esi
	push		edi

	mov			edi, [esp+4+8]
	mov			esi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 2

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [esi]
	movaps		xmm2, [esi+edx]
	lea			esi, [esi+edx*2]

	pshufd		xmm1, [esi], 0b1h
	pshufd		xmm3, [esi+edx], 0b1h
	lea			esi, [esi+edx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movaps		[edi+16*0], xmm0
	movaps		[edi+16*1], xmm4
	movaps		[edi+16*2], xmm1
	movaps		[edi+16*3], xmm5

	; col 1, 3

	pshufd		xmm0, [esi], 0b1h
	pshufd		xmm2, [esi+edx], 0b1h
	lea			esi, [esi+edx*2]

	movaps		xmm1, [esi]
	movaps		xmm3, [esi+edx]
	lea			esi, [esi+edx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movaps		[edi+16*4], xmm0
	movaps		[edi+16*5], xmm4
	movaps		[edi+16*6], xmm1
	movaps		[edi+16*7], xmm5

	add			edi, 128

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret
	
_SwizzleBlock8_sse2 endp

;
; SwizzleBlock4
;

_SwizzleBlock4_sse2 proc public

	push		esi
	push		edi
	
	mov			edi, [esp+4+8]
	mov			esi, [esp+8+8]
	mov			edx, [esp+12+8]
	mov			ecx, 2

	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [esi]
	movaps		xmm2, [esi+edx]
	lea			esi, [esi+edx*2]

	movaps		xmm1, [esi]
	movaps		xmm3, [esi+edx]
	lea			esi, [esi+edx*2]

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movaps		[edi+16*0], xmm0
	movaps		[edi+16*1], xmm1
	movaps		[edi+16*2], xmm4
	movaps		[edi+16*3], xmm3

	; col 1, 3

	movaps		xmm0, [esi]
	movaps		xmm2, [esi+edx]
	lea			esi, [esi+edx*2]

	movaps		xmm1, [esi]
	movaps		xmm3, [esi+edx]
	lea			esi, [esi+edx*2]

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movaps		[edi+16*4], xmm0
	movaps		[edi+16*5], xmm1
	movaps		[edi+16*6], xmm4
	movaps		[edi+16*7], xmm3

	add			edi, 128

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret

_SwizzleBlock4_sse2 endp

	end
