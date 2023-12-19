;******************************************************************************
	.386p
subtxt	segment para public 'code' use32
	assume	cs:subtxt,ds:datax
;******************************************************************************
public	bin2hex
public	reg_dump
extrn	print_edx:near
;------------------------------------------------------------------------------
bin2hex proc
;------------------------------------------------------------------------------
; in	eax  value
;	esi  hexadecimal digit
;	edi  store memory
;
	push	eax
	push	ebx
	push	ecx
	push	edx

	mov	edx, esi
	shl	edx, 2		; *4
	mov	cl, 32
	sub	cl, dl
	shl	eax, cl
	mov	ecx, esi

bin2hex_loop:
	rol	eax, 4
	movzx	ebx, al
	and	bl, 0fh
	mov	dl, [hex_str + ebx]

	cmp	byte ptr [edi], '_'
	jne	bin2hex_skip
	inc	edi
bin2hex_skip:
	mov	[edi], dl
	inc	edi
	loop	bin2hex_loop

	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret
bin2hex	endp

;------------------------------------------------------------------------------
reg_dump proc
	mov	esi, 8

	mov	edi, offset _eax
	call	bin2hex

	mov	eax, ebx
	mov	edi, offset _ebx
	call	bin2hex

	mov	eax, ecx
	mov	edi, offset _ecx
	call	bin2hex

	mov	eax, edx
	mov	edi, offset _edx
	call	bin2hex

	mov	edx, offset _dump
	call	print_edx
	ret
reg_dump endp
;------------------------------------------------------------------------------



subtxt	ends

;##############################################################################
datax	segment dword public 'data' use32
;##############################################################################

hex_str	db	'0123456789ABCDEF';

_dump	db	'eax='
_eax	db	'####_#### ebx='
_ebx	db	'####_#### ebx='
_ecx	db	'####_#### ebx='
_edx	db	'####_####',13,10,'$'

	db	0,0,0

;******************************************************************************
datax	ends

end
