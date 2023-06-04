%include "words.inc"
%include "lib.inc"
%define BUFF_SIZE 256
%define DQ_SIZE 8

extern find_word

global _start

section .data

not_found:
	db "Ключ не найден!", 0
buffer_overflow:
	db "ERROR. Не могу прочитать слово.", 0
string_buffer:
	times BUFF_SIZE db 0

section .text

_start: 
	xor rax, rax
	mov rdi, string_buffer
	mov rsi, BUFF_SIZE
	call read_word
	test rax, rax
	jne .success_read_buffer
	mov rdi, buffer_overflow
	call print_err
	call print_newline
	call exit
	.success_read_buffer:
		mov rdi, rax
		mov rsi, first_word
		push rdx
		call find_word
		test rax, rax
		jne .success_key_found
		mov rdi, not_found
		call print_err
		call print_newline
		call exit
	.success_key_found:
		pop rdx
		add rax, DQ_SIZE
		add rax, rdx
		add rax, 1
		mov rdi, rax
		call print_string
		call print_newline
		call exit

print_err:
	xor rax, rax
	mov rsi, rdi
	call string_length
	mov rdx, rax
	mov rdi, 2
	mov rax, 1
	syscall
	ret
