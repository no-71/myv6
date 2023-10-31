.section .text
.global _entry
_entry:
    la sp, kstack
    li a0, 0x1000
    add sp, sp, a0
    call main
spin:
    j spin
