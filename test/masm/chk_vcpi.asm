;##############################################################################
text	segment public para 'CODE'
	assume cs:text, ds:text, ss:text
;##############################################################################
extrn	print_dx   :proc
extrn	program_end:proc
;##############################################################################
	org	100h
main proc
	;
	; check EMS
	;
	mov	ax,3567h	;int 67h
	int	21h		;get vector to es:[bx]
	mov	bx,000ah	;es's offset: for 'EMMXXXX0'

	mov	ax,es:[bx  ]	; EMMX
	mov	dx,es:[bx+2]
	cmp	ax,'ME'
	jne	not_found
	cmp	dx,'XM'
	jne	not_found

	mov	ax,es:[bx+4]	; XXX0
	mov	dx,es:[bx+6]	;
	cmp	ax,'XX'
	jne	not_found
	cmp	dx,'0X'
	jne	not_found

	mov	dx, offset ems_found
	call	print_dx

	;
	; check VCPI
	;
	mov	ax,0de00h	; AL=00 : VCPI check!
	int	67h		; VCPI call
	test	ah,ah		; check
	jnz	not_found

	mov	dx, offset vcpi_found
	call	print_dx
	jmp	program_end
main	endp


not_found proc
	mov	dx, offset vcpi_not_found
	call	print_dx
	jmp	program_end
not_found endp

;##############################################################################

ems_found	db 'EMS found!',13,10,'$'
vcpi_found	db 'VCPI found!',13,10,'$'
vcpi_not_found	db 'VCPI not found!',13,10,'$'

;##############################################################################
text	ends
end	main
