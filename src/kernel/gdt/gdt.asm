; GDT flush function
global gdt_flush
extern gp
gdt_flush:
    lgdt [gp]
    mov ax, 0x10 ; Data segment offset of GDT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush2 ; Code segment offset
flush2:
    ret ; Returns to C code

global tss_flush
tss_flush:
    mov ax, 0x2B
    ltr ax
    ret