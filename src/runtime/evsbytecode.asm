IF !DEF(swap_bank)
	FAIL "Define the swap_bank macro in runtime.asm"
ENDC

MACRO std_bytecode
	; Control
	dw StdReturn
	dw StdYield
	dw StdGoto
	dw StdGotoFar
	dw StdGotoConditional
	dw StdGotoConditionalNot
	dw StdGotoConditionalFar
	dw StdGotoConditionalNotFar
	dw StdCallAsm
	dw StdCallAsmFar
	; 8-bit ops
	dw StdAdd
	dw StdSub
	dw StdMul
	dw StdDiv
	dw StdEqu
	dw StdNot
	dw StdLogicalAnd
	dw StdLogicalOr
	; Constant 8-bit ops
	dw StdAddConst
	dw StdSubConst
	dw StdMulConst
	dw StdDivConst
	dw StdEquConst
	dw StdNotConst
	; Copy
	dw StdCopy
	dw StdLoad
	dw StdStore
	dw StdCopyConst
	dw StdLoadConst
	dw StdStoreConst
ENDM

SECTION "EVScript Return", ROM0
StdReturn:
	ld hl, 0
StdYield:
	pop de ; pop return address
	pop de ; pop pool pointer
	ret

SECTION "EVScript Goto", ROM0
StdGoto:
	ld a, [hli]
	ld h, [hl]
	ld l, a
	ret

StdGotoConditional:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [bc]
	and a, a
	jr nz, StdGoto
.fail
	inc hl
	inc hl
	ret

StdGotoConditionalNot:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [bc]
	and a, a
	jr z, StdGoto
.fail
	inc hl
	inc hl
	ret

SECTION "EVScript GotoFar", ROM0
StdGotoFar:
	ld a, [hli]
	ld c, a
	ld a, [hli]
	ld b, a
	ld a, [hl]
	swap_bank
	ld l, c
	ld h, b
	ret

StdGotoConditionalFar:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [bc]
	and a, a
	jr nz, StdGotoFar
.fail
	inc hl
	inc hl
	ret

StdGotoConditionalNotFar:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [bc]
	and a, a
	jr z, StdGotoFar
.fail
	inc hl
	inc hl
	ret

SECTION "EVScript CallAsm", ROM0
StdCallAsm:
	push hl
	ld a, [hli]
	ld h, [hl]
	ld l, a
	call .hl
	pop hl
	ret
.hl
	jp hl

SECTION "EVScript CallAsmFar", ROM0
StdCallAsmFar:
	push hl
	ld a, [hli]
	ld c, a
	ld a, [hli]
	ld b, a
	ld a, [hli]
	swap_bank
	ld h, b
	ld l, c
	call .hl
	pop hl
	ret
.hl
	jp hl

SECTION "EVScript 8-bit Operations", ROM0
; @param de: pool
; @param hl: script pointer
; @return a: lhs
; @return b: rhs
ConstantOperandPrologue:
	ld a, [hli] ; lhs offset
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	; de is preserved & variable is pointed to by bc
	ld a, [bc]
	ld b, [hl]
	inc hl
	ret

; @param de: pool
; @param hl: script pointer
; @return a: lhs
; @return b: rhs
OperandPrologue:
	ld a, [hli] ; lhs offset
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	; de is preserved & variable is pointed to by bc
	push de
	ld a, [hli]
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	ld a, [de]
	pop de
	ld b, a
	ld a, [bc]
	ret

StdAdd:
	call OperandPrologue
	add a, b ; Here is the actual operation
	jr StoreEpilogue

StdSub:
	call OperandPrologue
	add a, b ; Here is the actual operation
	jr StoreEpilogue

; This is a VERY simple multiply routine. It is meant to be compact, not
; fast. Rewrite if speed is needed.
StdMul:
	call OperandPrologue
	ld c, a
	xor a, a
	inc b
:
	dec b
	jr z, StoreEpilogue
	add a, c
	jr :-

; This is a VERY simple divide routine. It is meant to be compact, not
; fast. Rewrite if speed is needed.
StdDiv:
	call OperandPrologue
	ld c, 0
:
	sub a, b
	jr c, StoreEpilogue
	inc c
	jr :-

StdEqu:
	call OperandPrologue
	cp a, b
	ld a, 0
	jr nz, StoreEpilogue
	inc a
	jr StoreEpilogue

StdNot:
	call OperandPrologue
	cp a, b
	ld a, 0
	jr z, StoreEpilogue
	inc a
	jr StoreEpilogue

StdLogicalAnd:
	call OperandPrologue
	and a, a
	jr z, StoreEpilogue
	ld a, b
	and a, a
	jr z, StoreEpilogue
	ld a, 1
	jr StoreEpilogue

StdLogicalOr:
	call OperandPrologue
	and a, a
	jr nz, .true
	ld a, b
	and a, a
	jr z, StoreEpilogue
.true
	ld a, 1
	; fallthrough
; This is stored in the middle so both variable and constant operations can
; reach it.
StoreEpilogue:
	ld b, a
	ld a, [hli]
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	ld a, b
	ld [de], a
	ret

StdAddConst:
	call ConstantOperandPrologue
	add a, b ; Here is the actual operation
	jr StoreEpilogue

StdSubConst:
	call ConstantOperandPrologue
	add a, b ; Here is the actual operation
	jr StoreEpilogue

; This is a VERY simple multiply routine. It is meant to be compact, not
; fast. Rewrite if speed is needed.
StdMulConst:
	call ConstantOperandPrologue
	ld c, a
	xor a, a
	inc b
:
	dec b
	jr z, StoreEpilogue
	add a, c
	jr :-

; This is a VERY simple divide routine. It is meant to be compact, not
; fast. Rewrite if speed is needed.
StdDivConst:
	call ConstantOperandPrologue
	ld c, 0
:
	sub a, b
	jr c, StoreEpilogue
	inc c
	jr :-

StdEquConst:
	call ConstantOperandPrologue
	cp a, b
	ld a, 0
	jr nz, StoreEpilogue
	inc a
	jr StoreEpilogue

StdNotConst:
	call ConstantOperandPrologue
	cp a, b
	ld a, 0
	jr z, StoreEpilogue
	inc a
	jr StoreEpilogue

StdLogicalAndConst:
	call ConstantOperandPrologue
	and a, a
	jr z, StoreEpilogue
	ld a, b
	and a, a
	jr z, StoreEpilogue
	ld a, 1
	jr StoreEpilogue

StdLogicalOrConst:
	call ConstantOperandPrologue
	and a, a
	jr nz, .true
	ld a, b
	and a, a
	jr z, StoreEpilogue
.true
	ld a, 1
	jr StoreEpilogue

SECTION "EVScript Copy", ROM0
StdCopy:
	push de
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [hli]
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	ld a, [de]
	ld [bc], a
	pop de
	ret

SECTION "EVScript Load", ROM0
StdLoad:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [hli]
	push hl
	add a, e
	ld l, a
	adc a, d
	sub a, l
	ld h, a
	ld a, [hli]
	ld h, [hl]
	ld l, a
	ld a, [hl]
	ld [bc], a
	pop hl
	ret

SECTION "EVScript Store", ROM0
StdStore:
	push de
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [bc]
	inc bc
	ld d, a
	ld a, [bc]
	ld b, a
	ld c, d
	ld a, [hli]
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	ld a, [de]
	ld [bc], a
	pop de
	ret

SECTION "EVScript CopyConst", ROM0
StdCopyConst:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [hli]
	ld [bc], a
	ret

SECTION "EVScript LoadConst", ROM0
StdLoadConst:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [hli]
	push hl
	ld h, [hl]
	ld l, a
	ld a, [hl]
	ld [bc], a
	pop hl
	inc hl
	ret

SECTION "EVScript StoreConst", ROM0
StdStoreConst:
	ld a, [hli]
	ld c, a
	ld a, [hli]
	ld b, a
	ld a, [bc]
	inc bc
	push de
	ld e, a
	ld a, [bc]
	ld b, a
	ld c, e
	pop de
	ld a, [hli]
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	ld a, [de]
	ld [bc], a
