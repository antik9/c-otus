    bits 64
    extern malloc, puts, printf, fflush, abort, free
    global main

    section   .data
empty_str: db 0x0
int_format: db "%ld ", 0x0
data: dq 4, 8, 15, 16, 23, 42
data_length: equ ($-data) / 8

    section   .text
;;; print_int proc
print_int:
    mov rsi, rdi
    mov rdi, int_format
    xor rax, rax
    call printf

    xor rdi, rdi
    call fflush

    ret

;;; p proc
p:
    mov rax, rdi
    and rax, 1
    ret

;;; add_element proc
add_element:
    push rbp
    push rbx

    mov rbp, rdi
    mov rbx, rsi

    mov rdi, 16
    call malloc
    test rax, rax
    jz abort

    mov [rax], rbp
    mov [rax + 8], rbx

    pop rbx
    pop rbp

    ret

;;; map proc
map:
    push rbp
    push rbx

map_loop:
    test rdi, rdi
    jz out_map

    mov rbx, rdi
    mov rbp, rsi

    mov rdi, [rdi]
    call rsi

    mov rdi, [rbx + 8]
    mov rsi, rbp
    jmp map_loop

out_map:
    pop rbx
    pop rbp

    ret

;;; map_free proc
map_free:
    push rbx

map_free_loop:
    test rdi, rdi
    jz out_map_free

	mov rbx, [rdi + 8]
    call free

    mov rdi, rbx
    jmp map_free_loop

out_map_free:
    pop rbx
    ret

;;; f proc
filter:
    push rbx
    push r11
    push r12
    push r13
    mov r11, rsi

filter_loop:
    test rdi, rdi
    jz out_filter

    mov rbx, rdi
    mov r12, rsi
    mov r13, rdx

    mov rdi, [rdi]
    call rdx

    test rax, rax
    jz filter_next

    mov rdi, [rbx]
    mov rsi, r11
    call add_element
    mov r11, rax

filter_next:
    mov rsi, r12
    mov rdi, [rbx + 8]
    mov rdx, r13

    jmp filter_loop

out_filter:
    mov rax, r11
    pop r13
    pop r12
    pop r11
    pop rbx
    ret

;;; main proc
main:
    push rbx
    push r12

    xor rax, rax
    mov rbx, data_length
adding_loop:
    mov rdi, [data - 8 + rbx * 8]
    mov rsi, rax
    call add_element
    dec rbx
    jnz adding_loop

    mov rbx, rax

    mov rdi, rax
    mov rsi, print_int
    call map

    mov rdi, empty_str
    call puts

    mov rdx, p
    xor rsi, rsi
    mov rdi, rbx
    call filter

    mov r12, rax

    mov rdi, rax
    mov rsi, print_int
    call map

    mov rdi, empty_str
    call puts

    mov rdi, rbx
    call map_free

    mov rdi, r12
    call map_free

    pop rbx
    pop r12

    xor rax, rax
    ret
