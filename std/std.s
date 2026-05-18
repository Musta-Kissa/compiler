default rel

section .bss
    buffer: resb 21
    print_int_buffer:     resb 22                     ; 20 digits + sign + newline + null

section .data
    min_int_str db "-9223372036854775808",10  ; exact output for INT64_MIN

section .text
    global print_int
    global allocate_page

;----------------------------------------------------------------------
; print_int
; Input:  [rbp+16] = signed 64‑bit integer
; Output: prints decimal representation to stdout, then a newline
; Saves:  only r8–r15 if they are used (none used here)
; Clobbers: RAX, RCX, RDX, RSI, RDI, RBX, R8–R11 (unchanged)
;----------------------------------------------------------------------
print_int:
    push rbp
    mov rbp, rsp

    ; The argument is at [rbp+16] (return address + saved rbp)
    mov rax, [rbp+16]           ; RAX = number to print

    ; Special case: INT64_MIN cannot be negated safely
    mov rcx, 0x8000000000000000     ; load 64-bit constant
    cmp rax, rcx
    je .print_min

    ; Determine sign, keep it in ECX (1 = negative, 0 = positive/zero)
    xor ecx, ecx
    test rax, rax
    jns .convert_nonneg
    neg rax                     ; make positive (safe: not INT64_MIN)
    inc ecx                     ; flag = 1

.convert_nonneg:
    ; RAX = absolute value
    lea rbx, [print_int_buffer + 20]      ; point past end of buffer
    mov byte [rbx], 0           ; null terminator (optional)
    dec rbx

    mov edi, 10                 ; divisor (clobbers RDI, which is fine)

.convert_loop:
    xor edx, edx
    div rdi                     ; divide RDX:RAX by 10
    add dl, '0'                 ; convert remainder to ASCII
    mov [rbx], dl               ; store digit
    dec rbx                     ; move left
    test rax, rax
    jnz .convert_loop

    inc rbx                     ; RBX now points to the first digit

    ; Prepend '-' if the original number was negative
    test ecx, ecx
    jz .no_sign
    dec rbx
    mov byte [rbx], '-'
.no_sign:

    ; Calculate string length (digits + optional sign)
    lea rdx, [print_int_buffer + 20]      ; end of buffer
    sub rdx, rbx                ; length without newline

    ; Append newline immediately after the digits
    mov byte [rbx + rdx], 10    ; ASCII newline
    inc rdx                     ; include newline in length

    ; sys_write(fd=1, buf=rbx, len=rdx)
    mov eax, 1                  ; syscall number for write
    mov edi, 1                  ; stdout
    mov rsi, rbx                ; string pointer
    syscall

    pop rbp
    ret

.print_min:
    ; Directly print the exact string for -2^63
    lea rsi, [min_int_str]
    mov edx, 21                 ; length of the string (including newline)
    mov eax, 1
    mov edi, 1
    syscall
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
