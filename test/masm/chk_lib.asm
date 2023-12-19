;##############################################################################
text	segment public para 'CODE'
	assume cs:text, ds:text, ss:text
;##############################################################################
public	wait_key_input
public	print_dx
public	program_end

wait_key_input proc
	mov	ah,08h
	int	21h
	ret
wait_key_input endp

print_dx proc
	mov	ah, 09h
	int	21h
	ret
print_dx endp

program_end proc
	mov	ah, 4ch
	int	21h
	ret
program_end endp

text	ends
end
