
%define DQ_SIZE 8

global find_word
extern string_length
extern string_equals

find_word:
	xor rax, rax
	.loop:
		push rsi
		add rsi, DQ_SIZE
		call string_equals
		pop rsi
		cmp rax, 0
		jne .end
		mov rsi, [rsi]
		cmp rsi, 0
		je .error
		jmp .loop
	.end:
		add rsi, DQ_SIZE
		push rsi
		call string_length
		pop rsi
		add rax, rsi
		inc rax
		ret
	.error:
		xor rax, rax
		ret
