;******************************************************************************
; DOS-Extender Version chcker
;******************************************************************************
;[TAB=8]
;
;##############################################################################
extrn	bin2hex:near
extrn	reg_dump:near
public	print_edx
;##############################################################################
	.386p
text	segment dword public 'code' use32
	assume	cs:text,ds:data
;##############################################################################
print_edx proc
	mov	ah, 09h
	int	21h
	ret
print_edx endp

	align	4
start proc
	mov	edx, offset msg
	call	print_edx

	;
	; Phar Lap Version Information
	;
	mov	edx, offset check1
	call	print_edx

	mov	ah ,30h
	mov	ebx,'PHAR'	; 50484152h
	int	21h
	call	reg_dump

	;
	; Free386 Version Information
	;
	mov	edx, offset check2
	call	print_edx

	mov	ah ,30h
	mov	ebx,'683F'	; 36383346h
	int	21h
	call	reg_dump

	;
	; end
	;
	mov	ah,4ch
	mov	al,10
	int	21h
start endp


text	ends

;##############################################################################
data	segment dword public 'data' use32
;##############################################################################

msg	db	'DOS-Extender Version checker',13,10,13,10,'$'

check1	db	'PharLap Version information',13,10,'$'
check2	db	'Free386 Version information',13,10,'$'

data	ends
end	start

