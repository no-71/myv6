.section .text
.global _entry
_entry:
    csrr tp, mhartid
    la sp, kstack_for_scheduler
    li a0, 0x1000
    add sp, sp, a0
    call start
spin:
    j spin
