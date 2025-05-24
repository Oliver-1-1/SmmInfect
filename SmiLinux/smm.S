.section .rodata
str:
    .string "R12 changed: Kallsymslookupname: %llx\n"

.text
.global smi_linux
.extern smi_print

smi_linux:
    push %r12
    push %rax
    push %rbx
    push %rdi
    push %rsi

    movabsq $0x4848484848484848, %r12
    movq %r12, %rbx
	
    mov $0x42, %eax  
    outb %al, $0xB2    

    leaq str(%rip), %rdi  
    movq %r12, %rsi        
    xorl %eax, %eax        

    call smi_print

    pop %rsi
    pop %rdi
    pop %rbx
    pop %rax
    pop %r12
    jmp __x86_return_thunk

.size smi_linux, .-smi_linux

