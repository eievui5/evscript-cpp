SECTION "EVScript Driver", ROM0
; @param de: Variable pool
; @param hl: Script pointer
ExecuteScript::
	ld a, h
	or a, l
	ret z
.next
	ld a, [hli]
	push hl
	add a, LOW(EVScriptBytecodeTable >> 1)
	ld l, a
	adc a, HIGH(EVScriptBytecodeTable >> 1)
	sub a, l
	ld h, a
	add hl, hl
	ld a, [hli]
	ld h, [hl]
	ld l, a
	ld b, h
	ld c, l
	pop hl
	push de
	call .callBC
	pop de
	jr ExecuteScript.next

.callBC
	push bc
	ret

SECTION "EVScript Bytecode table", ROM0, ALIGN[1]
EVScriptBytecodeTable:
