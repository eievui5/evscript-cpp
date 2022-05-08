MACRO std_bytecode16
	; 16-bit ops
	dw StdAdd16
	dw StdSub16
	dw StdMul16
	dw StdDiv16
	dw StdEqu16
	dw StdNot16
	dw StdLogicalAnd16
	dw StdLogicalOr16
	; Constant 16-bit ops
	dw StdAddConst16
	dw StdSubConst16
	dw StdMulConst16
	dw StdDivConst16
	dw StdEquConst16
	dw StdNotConst16
	; Copy 16
	dw StdCopy16
	dw StdLoad16
	dw StdStore16
	dw StdCopyConst16
	dw StdLoadConst16
	dw StdStoreConst16
	; 16-bit casts
	dw StdCast8to16
	dw StdCast16to8
ENDM

SECTION "EVScript 16-bit operation", ROM0
; @param de: pool
; @param hl: script pointer
; @return hl: lhs
; @return bc: rhs
ConstantOperandPrologue16:
	push de
	ld a, [hli] ; lhs offset
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	; de is preserved & variable is pointed to by de
	ld a, [hli]
	ld h, [hl]
	ld l, a

	ld a, [de]
	ld b, a
	inc de
	ld a, [de]
	ld c, d
	pop de
	ret

; @param de: pool
; @param hl: script pointer
; @return hl: lhs
; @return bc: rhs
OperandPrologue16:
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
	ld a, [bc]
	ld l, a
	inc bc
	ld a, [bc]
	ld h, a
	ld a, [de]
	ld c, a
	inc de
	ld a, [de]
	ld b, a
	pop de
	ret

StdAdd16:
	push hl
	call OperandPrologue16
	add hl, bc
	ld b, h
	ld c, l
	jp StoreEpilogue16

StdSub16:
	push hl
	call OperandPrologue16
	; hl - bc == hl + -bc
	ld a, b
	cpl
	ld b, a
	ld a, c
	cpl
	ld c, a
	inc bc
	add hl, bc
	ld b, h
	ld c, l
	jp StoreEpilogue16

StdMul16:
	push hl
	call OperandPrologue16
	push de
	ld d, h
	ld e, l
	ld hl, 0
	inc bc
:
	dec c
	jr nz, :+
	dec b
	jr z, .store
:	
	add hl, de
	jr :--

.store
	pop de
	ld b, h
	ld c, l
	jr StoreEpilogue16

StdDiv16:
	push hl
	call OperandPrologue16
	push de
	; bc = -bc
	ld a, b
	cpl
	ld b, a
	ld a, c
	cpl
	ld c, a
	inc bc

	ld de, 0
:
	add hl, bc
	jr c, .store
	inc de
	jr :-

.store
	ld b, d
	ld c, e
	pop de
	jr StoreEpilogue16

StdEqu16:
	push hl
	call OperandPrologue16
	ld a, h
	cp a, b
	jr nz, .fail
	ld a, l
	cp a, c
	jr nz, .fail
	ld bc, 1
	jr StoreEpilogue16

.fail
	ld bc, 0
	jr StoreEpilogue16

StdNot16:
	push hl
	call OperandPrologue16
	ld a, h
	cp a, b
	jr nz, .true
	ld a, l
	cp a, c
	jr nz, .true
	ld bc, 0
	jr StoreEpilogue16

.true
	ld bc, 1
	jr StoreEpilogue16

StdLogicalAnd16:
	push hl
	call OperandPrologue16
	ld a, b
	or a, c
	jr z, .fail
	ld a, h
	or a, l
	jr z, .fail
	ld bc, 1
	jr StoreEpilogue16

.fail
	ld bc, 0
	jr StoreEpilogue16

StdLogicalOr16:
	push hl
	call OperandPrologue16
	ld a, b
	or a, c
	jr nz, .true
	ld a, h
	or a, l
	jr nz, .true
	ld bc, 0
	jr StoreEpilogue16

.true
	ld bc, 1
	; Fallthrough
StoreEpilogue16:
	pop hl
	ld a, [hli]
	add a, e
	ld e, a
	adc a, d
	sub a, e
	ld d, a
	ld a, c
	ld [de], a
	inc de
	ld a, b
	ld [de], a
	ret

StdAddConst16:
	push hl
	call ConstantOperandPrologue16
	add hl, bc
	ld b, h
	ld c, l
	pop hl
	jr StoreEpilogue16

StdSubConst16:
	push hl
	call ConstantOperandPrologue16
	; hl - bc == hl + -bc
	ld a, b
	cpl
	ld b, a
	ld a, c
	cpl
	ld c, a
	inc bc
	add hl, bc
	ld b, h
	ld c, l
	pop hl
	jr StoreEpilogue16

StdMulConst16:
	push hl
	call ConstantOperandPrologue16
	push de
	ld d, h
	ld e, l
	ld hl, 0
	inc bc
:
	dec c
	jr nz, :+
	dec b
	jr z, .store
:	
	add hl, de
	jr :--

.store
	pop de
	ld b, h
	ld c, l
	pop hl
	jr StoreEpilogue16

StdDivConst16:
	push hl
	call ConstantOperandPrologue16
	push de
	; bc = -bc
	ld a, b
	cpl
	ld b, a
	ld a, c
	cpl
	ld c, a
	inc bc

	ld de, 0
:
	add hl, bc
	jr c, .store
	inc de
	jr :-

.store
	ld b, d
	ld c, e
	pop de
	pop hl
	jr StoreEpilogue16

StdEquConst16:
	push hl
	call ConstantOperandPrologue16
	ld a, h
	cp a, b
	jr nz, .fail
	ld a, l
	cp a, c
	jr nz, .fail
	ld bc, 1
	pop hl
	jr StoreEpilogue16

.fail
	ld bc, 0
	pop hl
	jr StoreEpilogue16

StdNotConst16:
	push hl
	call ConstantOperandPrologue16
	ld a, h
	cp a, b
	jr nz, .true
	ld a, l
	cp a, c
	jr nz, .true
	ld bc, 0
	pop hl
	jp StoreEpilogue16

.true
	ld bc, 1
	pop hl
	jp StoreEpilogue16

SECTION "EVScript Copy16", ROM0
StdCopy16:
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
	inc de
	inc bc
	ld a, [de]
	ld [bc], a
	ret

SECTION "EVScript Load16", ROM0
StdLoad16:
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
	ld a, [hli]
	ld [bc], a
	inc bc
	ld a, [hl]
	ld [bc], a
	pop hl
	ret

SECTION "EVScript Store16", ROM0
StdStore16:
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
	inc de
	inc bc
	ld a, [de]
	ld [bc], a
	ret

SECTION "EVScript CopyConst16", ROM0
StdCopyConst16:
	ld a, [hli]
	add a, e
	ld c, a
	adc a, d
	sub a, c
	ld b, a
	ld a, [hli]
	ld [bc], a
	inc bc
	ld a, [hli]
	ld [bc], a
	ret

SECTION "EVScript LoadConst16", ROM0
StdLoadConst16:
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
	ld a, [hli]
	ld [bc], a
	inc bc
	ld a, [hl]
	ld [bc], a
	pop hl
	inc hl
	ret

SECTION "EVScript StoreConst16", ROM0
StdStoreConst16:
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
	inc de
	inc bc
	ld a, [de]
	ld [bc], a
	ret
