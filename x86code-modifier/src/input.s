    .intel_syntax noprefix
    .global input_routine_begin, input_routine_end
    .text
    .align 4
input_routine_begin:
    xor edi, edi
    dec edi
    call func
    ret
func:
    mov eax, edi
    mov ecx, 0
    loop:
    xor ecx, eax
    dec eax
    cmp eax, 0
    jnz loop
    ret
input_routine_end:
