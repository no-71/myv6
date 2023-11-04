.text
.global _entry
_entry:
    csrr tp, mhartid

    la sp, kstack_for_scheduler
    li a0, 0x1000
    mv a1, tp
    add a1, a1, 1
    mul a0, a0, a1
    add sp, sp, a0

    call start
spin:
    j spin
