default rel

section .bss
    buffer: resb 21

section .text
    global print_int
    global allocate_page

; Procedure: print_int
; Input: RDI = integer to print (64-bit, non-negative)
; Output: Prints integer to stdout followed by newline
; Clobbers: RAX, RCX, RDX, RSI, R8, R9, R10, R11
print_int:
    push rbp
    mov rbp, rsp

    push rax
    push rbx
    push rdx
    push rsi
    
    mov rax, [rbp+16]            ; Number to print
    lea rbx, [buffer + 20]  ; Point to end of buffer
    mov byte [rbx], 0       ; Null terminator (not strictly needed)
    dec rbx
    
    mov rcx, 10             ; Divisor
    
    ; Handle zero case
    test rax, rax
    jnz .convert
    mov byte [rbx], '0'
    dec rbx
    jmp .write
    
.convert:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rbx], dl
    dec rbx
    test rax, rax
    jnz .convert
    
.write:
    inc rbx                 ; Move back to first digit
    
    ; Calculate length
    lea rdx, [buffer + 20]  ; End of buffer
    sub rdx, rbx            ; Length without newline
    
    ; Store newline after the number
    lea rsi, [rbx + rdx]    ; Point to position after last digit
    mov byte [rsi], 10      ; Add newline (ASCII 10)
    inc rdx                 ; Include newline in length
    
    ; Write syscall
    mov rax, 1              ; sys_write = 1
    mov rdi, 1              ; stdout = 1
    mov rsi, rbx            ; String pointer
    ; RDX already has length (including newline)
    syscall
    
    pop rsi
    pop rdx
    pop rbx
    pop rax

    mov rsp, rbp
    pop rbp 
    ret

allocate_page:
    ;push rax
    ;push rdi
    ;push rsi
    push r10
    push r8
    push r9
    ; mmap syscall: void *mmap(void *addr, size_t length, int prot,
    ;                          int flags, int fd, off_t offset)
    ; Syscall number 9 (x86_64)
    mov rax, 9                  ; sys_mmap
    xor rdi, rdi                ; addr = NULL (kernel chooses)
    mov rsi, 4096               ; length = one page (typical page size)
    mov rdx, 0x3                 ; prot = PROT_READ | PROT_WRITE (1|2 = 3)
    mov r10, 0x22                ; flags = MAP_PRIVATE | MAP_ANONYMOUS (0x02 | 0x20 = 0x22)
    mov r8, -1                   ; fd = -1 (ignored for MAP_ANONYMOUS)
    xor r9, r9                   ; offset = 0
    syscall

    ;mov r15, rax                ; return value (pointer or -1) is in rax

    pop r9
    pop r8
    pop r10
    ;pop rsi
    ;pop rdi
    ;pop rax
    ret                          

%if 0
_start:
    ; Test with number 42
    push 1234
    call print_int
    
    ; Exit
    mov rax, 60
    xor rdi, rdi
    syscall
%endif
