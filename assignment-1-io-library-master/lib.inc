
section .text
 
 
; Принимает код возврата и завершает текущий процесс
exit: 
    mov rax, 60
    syscall
    ret

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
    xor rax, rax				
	.loop:
		cmp byte [rdi+rax], 0	
		je .end
		inc rax					
		jmp .loop
	.end:
    		ret


; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    xor rax, rax
    	mov rsi, rdi
    	call string_length
    	mov rdx, rax
    	mov rdi, 1
    	mov rax, 1
    	syscall
    ret

; Принимает код символа и выводит его в stdout
print_char:
    xor rax, rax
	push rdi
	mov rax, 1
	mov rsi, rsp
	pop rdi
	mov rdx, 1
	mov rdi, 1
	syscall
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    mov rsi, 0xA
    call print_char
    ret

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:
    xor rax, rax
	xor rdx, rdx		
	mov r10, 10
	mov r8, 0
	mov rax, rdi
	.loop:
		div r10
    		mov rsi, rdx
    		add rsi, 48
    		inc r8
    		dec rsp
    		mov [rsp], sil
		xor rdx, rdx
    		cmp rax, 0
    		jne .loop
	mov rsi, rsp
    	mov rax, 1
    	mov rdx, r8
    	mov rdi, 1
    	syscall
    	add rsp, r8
     ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int:
    xor rax, rax
    mov rsi, rdi
    cmp rdi, 0
    jge .positive
    mov rdi, '-'
    push rsi
    call print_char
    pop rsi
    mov rdi, rsi
    neg rdi
    .positive:
    	call print_uint
    ret

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
    xor rax, rax
	push rbx
	xor rbx, rbx
	.loop:
		mov cl, byte [rdi + rbx]
		cmp cl, byte [rsi + rbx]
		jnz .nequals
		cmp byte [rdi + rbx], 0
		jz .equals
		inc rbx	
		jmp .loop
	.equals:
		mov rax, 1				
		jmp .end
    	.nequals:
		mov rax, 0				
		jmp .end
	.end:
		pop rbx
		ret

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
    xor rax, rax
    	mov rdx, 1
    	mov rdi, 0
    	dec rsp
    	mov rsi, rsp
    	mov rax, 0
    	syscall
    	cmp rax, 0
    	je .endZero
    	mov rax, [rsp]
    	inc rsp
    ret 
    	.endZero:
    		mov rax, 0
    		inc rsp
    		ret
 

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор

read_word:
	mov r8, rdi
	mov r9, rsi
	.delete_spaces:
		call read_char
		cmp al, 0x20
		je .delete_spaces
		cmp al, 0x9
		je .delete_spaces
		cmp al, 0xA
		je .delete_spaces
	xor rcx, rcx 
	jmp .save

	.loop:
		push rcx
		call read_char
		pop rcx
	.save:
		cmp al ,0xA
		je .finish
		cmp al, 0x20
		je .finish
		cmp al, 0x4
		je .finish
		cmp al, 0x9
		je .finish
		cmp al, 0
		je .finish
		inc rcx
		cmp rcx, r9
		jge .error
		dec rcx
		mov [r8 + rcx], al
		inc rcx
		jmp .loop
	.error:
		xor rax, rax			
		jmp .end
	.finish:
		mov byte [r8 + rcx], 0
		mov rdx, rcx
		mov rax, r8
		jmp. end
   	.end:
		ret

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint:
    xor rax, rax
    	xor r8, r8 
	xor rdx, rdx
	xor rsi, rsi
	mov r9, 10 
	.loop:
		mov sil, [rdi+r8] 	
		cmp sil, 48			
		jl .end			
		cmp sil, 57			
		jg .end			
		inc r8
		sub sil, 48			
		mul r9				
		add rax, rsi		
		jmp .loop	
	.end:
		mov rdx, r8
		ret


; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    xor rax, rax
    	cmp byte[rdi], '-'
    	jne parse_uint
    	inc rdi
    	call parse_uint
    	neg rax
    	inc rdx 
    ret 

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    xor rax, rax
	xor rcx, rcx 			
	xor r8, r8				
	call string_length 		
	cmp rax, rdx
	jle .loop
	mov rax, 0
	ret
	.loop:
		mov r8b, [rdi+rcx] 
		mov [rsi+rcx], r8b
		inc rcx
		cmp byte [rsi+rcx], 0
		jne .loop
	mov rax, rdx
	ret
