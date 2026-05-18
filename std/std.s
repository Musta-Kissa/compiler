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


global print_double

section .data
    mul_const:    dq  1000000.0
    half:         dq  0.5
    max_uint64:   dq  18446744073709551615.0     ; 2^64 - 1

section .text
print_double:
    push rbp
    mov rbp, rsp

    push   r8
    push   r9
    push   r10
    push   r12
    push   r13
    push   r14
    push   r15

    sub    rsp, 64
    mov    r12, rsp                     ; r12 = write pointer into buffer

    ; ---------- 1. Extract bits, handle special values ----------
    mov    rax, [rbp+16]                    ; rax = raw bits
    mov    rcx, rax                     ; keep original for sign

    ; exponent = (rax >> 52) & 0x7FF
    mov    r8, rax
    shr    r8, 52
    and    r8d, 0x7FF                   ; r8d = exponent

    ; mantissa = rax & 0x000FFFFFFFFFFFFF
    mov    r9, rax
    mov    r10, 0x000FFFFFFFFFFFFF
    and    r9, r10                      ; r9 = mantissa

    cmp    r8d, 0x7FF
    jne    .check_zero
    test   r9, r9
    jz     .write_inf
    ; NaN
.write_nan:
    mov    byte [r12], 'n'
    mov    byte [r12+1], 'a'
    mov    byte [r12+2], 'n'
    add    r12, 3
    jmp    .write_and_exit

.write_inf:
    test   rax, rax
    jns    .no_sign_inf
    mov    byte [r12], '-'
    inc    r12
.no_sign_inf:
    mov    byte [r12], 'i'
    mov    byte [r12+1], 'n'
    mov    byte [r12+2], 'f'
    add    r12, 3
    jmp    .write_and_exit

.check_zero:
    cmp    r8d, 0
    jne    .finite_number
    test   r9, r9
    jnz    .finite_number               ; subnormal
    ; Zero
    test   rax, rax
    jns    .no_sign_zero
    mov    byte [r12], '-'
    inc    r12
.no_sign_zero:
    mov    byte [r12], '0'
    inc    r12
    jmp    .write_and_exit

.finite_number:
    ; ---------- 2. Obtain |x| using integer bit clear ----------
    ; Clear sign bit (bit 63) in the raw bits and move back to xmm0
    btr    rax, 63                      ; rax now holds positive bit pattern
    movq   xmm0, rax

    ; scaled = |x| * 1e6 + 0.5
    mulsd  xmm0, [mul_const]
    addsd  xmm0, [half]

    ; Clamp to 2^64-1 if necessary
    comisd xmm0, [max_uint64]
    jb     .no_clamp
    movsd  xmm0, [max_uint64]
.no_clamp:

    ; Convert scaled double to uint64 safely
    mov    r13, 0x43E0000000000000     ; 2^63 (double)
    movq   xmm1, r13
    comisd xmm0, xmm1
    jb     .conv_small
    ; large case: scaled >= 2^63
    subsd  xmm0, xmm1
    cvttsd2si r14, xmm0
    bts    r14, 63                     ; set bit 63
    jmp    .conv_done
.conv_small:
    cvttsd2si r14, xmm0               ; r14 = unsigned value (<2^63)
.conv_done:
    mov    r15, r14                     ; r15 = intval (uint64_t)

    ; ---------- 3. Split integer and fraction ----------
    mov    rax, r15
    mov    rbx, 1000000
    xor    rdx, rdx
    div    rbx
    mov    r13, rax                     ; r13 = integer part
    mov    r14, rdx                     ; r14 = fraction part

    ; Sign
    test   rcx, rcx                     ; rcx still holds original bits
    jns    .no_sign
    mov    byte [r12], '-'
    inc    r12
.no_sign:

    ; Convert integer part to decimal string
    mov    rax, r13
    mov    rdi, r12
    call   ulltoa
    mov    r12, rax                     ; rax = pointer past integer string

    mov    byte [r12], '.'
    inc    r12

    ; Output 6 fraction digits with leading zeros
    mov    rax, r14
    mov    ecx, 6
    add    r12, 5                       ; start from rightmost position
    mov    r13, r12                     ; remember end
.fraction_loop:
    xor    edx, edx
    mov    rbx, 10
    div    rbx
    add    dl, '0'
    mov    [r12], dl
    dec    r12
    sub    ecx, 1
    jnz    .fraction_loop
    add    r12, 6                       ; restore pointer to end+1

.write_and_exit:
    ; ---------- 4. write(1, buf, len) ----------
    mov byte [r12], 10      ; newline character ('\n')
    inc r12

    mov    rdx, r12
    sub    rdx, rsp
    mov    rsi, rsp
    mov    edi, 1
    mov    eax, 1
    syscall

    push   r15
    push   r14
    push   r13
    push   r12
    push   r10
    push   r9
    push   r8
    mov rsp, rbp
    pop rbp 
    ret

; ---------- Helper: ulltoa ----------
; Input:  RAX = unsigned 64‑bit integer
;         RDI = pointer to output buffer
; Output: RAX = pointer to byte after the string
; Clobbers: RAX, RCX, RDX, RDI
ulltoa:
    push   rbx
    sub    rsp, 24
    mov    rcx, rsp                     ; temp buffer for reversed digits
    mov    rbx, 10
.next_digit:
    xor    edx, edx
    div    rbx
    add    dl, '0'
    mov    [rcx], dl
    inc    rcx
    test   rax, rax
    jnz    .next_digit
.copy:
    dec    rcx
    cmp    rcx, rsp
    jb     .done
    mov    al, [rcx]
    mov    [rdi], al
    inc    rdi
    jmp    .copy
.done:
    mov    rax, rdi
    add    rsp, 24
    pop    rbx
    ret
