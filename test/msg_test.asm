;
;msg test
;
extern msg
;----------------------------------------------------------------------------
segment	text class=CODE align=4 use16
;-----------------------------------------------------------------------------
start:
	mov	word [msg    ], 'aa'
	mov	word [msg + 4], 'cc'

	mov	dx, msg
	mov	ah, 09h
	int	21h

	mov	ax,4c00h
	int	21h
